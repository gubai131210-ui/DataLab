#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/quality_types.h"
#include "domain/statistics/censoring_contract.h"
#include "domain/statistics/aalen_johansen_cif.h"
#include "domain/statistics/fine_gray.h"
#include "domain/statistics/reliability.h"
#include "infrastructure/output_serialization.h"

#include <QtTest/QtTest>

#include <cmath>
#include <string>
#include <vector>

using datalab::application::AnalysisService;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::statistics::aalen_johansen_cif;
using datalab::domain::statistics::AalenJohansenCifModeResult;
using datalab::domain::statistics::CensoringObservation;
using datalab::domain::statistics::CensoringType;
using datalab::domain::statistics::kaplan_meier;
using datalab::domain::statistics::fit_lognormal;
using datalab::domain::statistics::fit_weibull;
using datalab::domain::statistics::summarize_warranty;
using datalab::domain::statistics::summarize_warranty_strata;
using datalab::domain::statistics::validate_censoring_contract;
using datalab::domain::statistics::WarrantyStratumInput;
using datalab::domain::statistics::WarrantySummaryOptions;

namespace {

bool nearly_equal(double a, double b, double tol = 1e-9)
{
    return std::fabs(a - b) <= tol;
}

}  // namespace

class ReliabilityPhase5Test final : public QObject {
    Q_OBJECT

private slots:
    void censoring_contract_rejects_negative_and_reversed_interval()
    {
        std::vector<CensoringObservation> bad_time = {
            {CensoringType::exact, -1.0, 0.0, 0.0, "hours", "", "", std::nullopt, 0}};
        QVERIFY(!validate_censoring_contract(bad_time).ok);

        std::vector<CensoringObservation> reversed = {
            {CensoringType::interval, 0.0, 20.0, 10.0, "hours", "", "", std::nullopt, 0}};
        QVERIFY(!validate_censoring_contract(reversed).ok);
    }

    void right_censored_not_treated_as_failure_and_left_blocks_classic_km()
    {
        std::vector<CensoringObservation> observations = {
            {CensoringType::exact, 10.0, 0, 0, "hours", "B1", "wear", std::nullopt, 0},
            {CensoringType::right, 15.0, 0, 0, "hours", "B1", "", std::nullopt, 1},
            {CensoringType::exact, 20.0, 0, 0, "hours", "B1", "wear", std::nullopt, 2},
            {CensoringType::right, 25.0, 0, 0, "hours", "B1", "", std::nullopt, 3},
            {CensoringType::exact, 30.0, 0, 0, "hours", "B1", "wear", std::nullopt, 4}};
        const auto contract = validate_censoring_contract(observations);
        QVERIFY(contract.ok);
        QCOMPARE(static_cast<int>(contract.failure_count), 3);
        QCOMPARE(static_cast<int>(contract.right_censored_count), 2);
        QCOMPARE(static_cast<int>(contract.events_for_right_censored_km.size()), 5);
        QCOMPARE(contract.events_for_right_censored_km[1], false);
        QCOMPARE(contract.events_for_right_censored_km[3], false);

        observations.push_back(
            {CensoringType::left, 5.0, 0, 0, "hours", "B1", "", std::nullopt, 5});
        QVERIFY(!validate_censoring_contract(observations).ok);
    }

    void km_handcalc_baseline_formula_reference()
    {
        // samples/phase0_baselines/reliability_km_handcalc.md
        const std::vector<double> times = {10, 15, 20, 25, 30};
        const std::vector<bool> events = {true, false, true, false, true};
        const auto km = kaplan_meier(times, events, 0.95, {0, 1, 2, 3, 4});
        QVERIFY(km.failure_count == 3);
        QVERIFY(km.censored_count == 2);
        QVERIFY(km.points.size() >= 3);

        // Failure times only: 10, 20, 30 — censor times must not create steps.
        std::vector<double> failure_times;
        for (const auto& point : km.points) {
            if (point.failures > 0) {
                failure_times.push_back(point.time);
            }
        }
        QCOMPARE(static_cast<int>(failure_times.size()), 3);
        QVERIFY(nearly_equal(failure_times[0], 10.0));
        QVERIFY(nearly_equal(failure_times[1], 20.0));
        QVERIFY(nearly_equal(failure_times[2], 30.0));

        QVERIFY(nearly_equal(km.points[0].survival, 0.8, 1e-9));
        // Find point at t=20
        bool found20 = false;
        for (const auto& point : km.points) {
            if (nearly_equal(point.time, 20.0) && point.failures > 0) {
                QVERIFY(nearly_equal(point.survival, 0.8 * 2.0 / 3.0, 1e-6));
                found20 = true;
            }
            if (nearly_equal(point.time, 30.0) && point.failures > 0) {
                QVERIFY(nearly_equal(point.survival, 0.0, 1e-9));
            }
        }
        QVERIFY(found20);
        // Evidence policy for this baseline is formula_reference (documented; not vendor_oracle).
    }

    void weibull_and_lognormal_have_separate_assertions()
    {
        const std::vector<double> times = {10, 20, 30, 40, 50, 25, 35};
        const std::vector<bool> events = {true, true, true, true, true, false, false};
        const auto weibull = fit_weibull(times, events);
        const auto lognormal = fit_lognormal(times, events);
        QVERIFY(weibull.identifiable);
        QVERIFY(lognormal.identifiable);
        QVERIFY(weibull.shape > 0.0);
        QVERIFY(lognormal.scale > 0.0);
        // Distinct parameterization: Weibull shape/scale vs Lognormal location/scale.
        QVERIFY(std::fabs(weibull.shape - lognormal.location) > 1e-6
                || std::fabs(weibull.scale - lognormal.scale) > 1e-6);
        QVERIFY(weibull.median_life.has_value());
        QVERIFY(lognormal.median_life.has_value());
        // Lognormal median is exp(location) on time scale (2-param, no threshold).
        QVERIFY(nearly_equal(*lognormal.median_life, std::exp(lognormal.location), 1e-6));
    }

    void warranty_claims_per_1000_formula()
    {
        WarrantySummaryOptions options;
        options.warranty_time = 1000.0;
        options.time_unit = "hours";
        options.exposure = 5000.0;
        options.reliability_at_warranty = 0.98;
        options.observed_failures = 12;
        options.censored_count = 3;
        options.valid_count = 40;
        options.model_name = "weibull";
        options.reliability_is_prediction = true;
        const auto summary = summarize_warranty(options);
        QVERIFY(summary.ok);
        QVERIFY(nearly_equal(summary.failure_probability, 0.02));
        QVERIFY(nearly_equal(summary.claims_per_1000, 20.0));
        QVERIFY(nearly_equal(summary.expected_failures, 100.0));
        QCOMPARE(QString::fromStdString(summary.quantity_label), QStringLiteral("prediction"));
        QCOMPARE(QString::fromStdString(summary.evidence_type),
                 QStringLiteral("formula_reference"));

        options.exposure = 0.0;
        QVERIFY(!summarize_warranty(options).ok);
    }

    void warranty_service_page_and_facts_roundtrip()
    {
        AnalysisConfiguration configuration;
        configuration.chart_type = "reliability_warranty";
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.exposure = 1000.0;
        configuration.reliability.reliability_at_warranty = 0.95;
        configuration.reliability.model = "weibull";
        configuration.reliability.warranty_observed_failures = 2;
        configuration.reliability.warranty_censored_count = 1;
        configuration.reliability.warranty_valid_count = 10;
        DataTable table;
        const auto page = AnalysisService::reliability_warranty(table, configuration);
        QVERIFY(page.facts.warranty.has_value());
        QVERIFY(nearly_equal(page.facts.warranty->claims_per_1000, 50.0));
        QCOMPARE(QString::fromStdString(page.facts.warranty->evidence_type),
                 QStringLiteral("formula_reference"));

        const auto json = datalab::infrastructure::output_page_to_json(page);
        const auto restored = datalab::infrastructure::output_page_from_json(json);
        QVERIFY(restored.facts.warranty.has_value());
        QVERIFY(nearly_equal(restored.facts.warranty->claims_per_1000, 50.0));
    }

    void censoring_type_column_drives_contract_without_event_column()
    {
        DataTable table;
        table.columns = {"time", "censor_type", "mode"};
        table.rows = {
            {"10", "exact", "wear"},
            {"15", "right", ""},
            {"20", "failure", "wear"},
            {"25", "censored", ""},
            {"30", "exact", "wear"}};
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.censoring_type_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.model = "kaplan_meier";
        configuration.reliability.time_unit = "hours";
        const auto page = AnalysisService::reliability(table, configuration);
        QVERIFY(page.facts.reliability.has_value());
        QVERIFY(page.facts.reliability->failure_count.has_value());
        QVERIFY(page.facts.reliability->censored_count.has_value());
        QCOMPARE(static_cast<int>(*page.facts.reliability->failure_count), 3);
        QCOMPARE(static_cast<int>(*page.facts.reliability->censored_count), 2);
        QCOMPARE(static_cast<int>(page.facts.reliability->failure_mode_distinct_count), 1);
        QCOMPARE(page.facts.reliability->failure_modes.size(), static_cast<std::size_t>(1));
        QCOMPARE(QString::fromStdString(page.facts.reliability->failure_modes.front()),
                 QStringLiteral("wear"));
    }

    void censoring_type_column_conflicts_with_event_are_rejected()
    {
        DataTable table;
        table.columns = {"time", "event", "censor_type"};
        table.rows = {
            {"10", "1", "right"},  // conflict: event failure vs right censor
            {"20", "0", "right"}};
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.event_column = 1;
        configuration.reliability.censoring_type_column = 2;
        configuration.reliability.model = "kaplan_meier";
        const auto page = AnalysisService::reliability(table, configuration);
        QVERIFY(!page.diagnostics.empty());
        QCOMPARE(QString::fromStdString(page.diagnostics.front().code),
                 QStringLiteral("invalid_censoring_type_value"));
    }

    void interval_bounds_required_and_still_block_classic_km()
    {
        DataTable missing_bounds;
        missing_bounds.columns = {"time", "censor_type"};
        missing_bounds.rows = {
            {"10", "exact"},
            {"15", "interval"}};
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.censoring_type_column = 1;
        configuration.reliability.model = "kaplan_meier";
        const auto missing = AnalysisService::reliability(missing_bounds, configuration);
        QVERIFY(!missing.diagnostics.empty());
        QCOMPARE(QString::fromStdString(missing.diagnostics.front().code),
                 QStringLiteral("missing_interval_bounds"));

        DataTable with_bounds;
        with_bounds.columns = {"time", "censor_type", "L", "R"};
        with_bounds.rows = {
            {"10", "exact", "", ""},
            {"0", "interval", "12", "18"},
            {"20", "right", "", ""}};
        configuration.reliability.interval_left_column = 2;
        configuration.reliability.interval_right_column = 3;
        const auto blocked = AnalysisService::reliability(with_bounds, configuration);
        QVERIFY(!blocked.diagnostics.empty());
        // Bounds accepted into contract, then classic KM path rejects left/interval.
        bool found_block = false;
        for (const auto& diagnostic : blocked.diagnostics) {
            if (diagnostic.code == "censoring_left_interval_not_for_classic_km") {
                found_block = true;
                break;
            }
        }
        QVERIFY(found_block);

        const auto json = datalab::infrastructure::output_page_to_json(blocked);
        QCOMPARE(json.value(QStringLiteral("configuration")).toObject()
                     .value(QStringLiteral("reliability_interval_left_column")).toInt(-1),
                 2);
        QCOMPARE(json.value(QStringLiteral("configuration")).toObject()
                     .value(QStringLiteral("reliability_interval_right_column")).toInt(-1),
                 3);
        const auto restored = datalab::infrastructure::output_page_from_json(json);
        QCOMPARE(restored.configuration.reliability.interval_left_column,
                 std::optional<std::size_t>{2});
        QCOMPARE(restored.configuration.reliability.interval_right_column,
                 std::optional<std::size_t>{3});
    }

    void exposure_column_sums_for_reliability_and_warranty()
    {
        DataTable table;
        table.columns = {"time", "censor_type", "exposure"};
        table.rows = {
            {"10", "exact", "1.5"},
            {"15", "right", "2.5"},
            {"20", "exact", "1.0"}};
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.censoring_type_column = 1;
        configuration.reliability.exposure_column = 2;
        configuration.reliability.model = "kaplan_meier";
        configuration.reliability.time_unit = "hours";
        const auto page = AnalysisService::reliability(table, configuration);
        QVERIFY(page.facts.reliability.has_value());
        QVERIFY(page.facts.reliability->total_exposure.has_value());
        QVERIFY(nearly_equal(*page.facts.reliability->total_exposure, 5.0));
        QCOMPARE(page.facts.reliability->exposure_row_count, std::size_t{3});
        QCOMPARE(QString::fromStdString(page.facts.reliability->exposure_source),
                 QStringLiteral("column_sum"));

        AnalysisConfiguration warranty;
        warranty.reliability.warranty_time = 1000.0;
        warranty.reliability.time_unit = "hours";
        warranty.reliability.exposure = 9999.0;  // must be ignored when column present
        warranty.reliability.reliability_at_warranty = 0.98;
        warranty.reliability.exposure_column = 2;
        warranty.reliability.model = "weibull";
        const auto summary = AnalysisService::reliability_warranty(table, warranty);
        QVERIFY(summary.facts.warranty.has_value());
        QVERIFY(nearly_equal(summary.facts.warranty->exposure, 5.0));
        QVERIFY(nearly_equal(summary.facts.warranty->expected_failures, 0.1));
        QCOMPARE(QString::fromStdString(summary.facts.warranty->exposure_source),
                 QStringLiteral("column_sum"));
        QCOMPARE(summary.facts.warranty->exposure_row_count, std::size_t{3});
        bool override_noted = false;
        for (const auto& diagnostic : summary.diagnostics) {
            if (diagnostic.code == "warranty_exposure_column_overrides_scalar") {
                override_noted = true;
                break;
            }
        }
        QVERIFY(override_noted);

        table.rows[1][2] = "-1";
        const auto bad = AnalysisService::reliability_warranty(table, warranty);
        QVERIFY(!bad.diagnostics.empty());
        QCOMPARE(QString::fromStdString(bad.diagnostics.front().code),
                 QStringLiteral("invalid_exposure_value"));

        DataTable bad_rel = table;
        bad_rel.rows = {
            {"10", "exact", "1.0"},
            {"15", "right", "-2.0"}};
        AnalysisConfiguration bad_rel_cfg = configuration;
        const auto bad_reliability = AnalysisService::reliability(bad_rel, bad_rel_cfg);
        QVERIFY(!bad_reliability.diagnostics.empty());
        QCOMPARE(QString::fromStdString(bad_reliability.diagnostics.front().code),
                 QStringLiteral("invalid_exposure_value"));

        DataTable excluded_table;
        excluded_table.columns = {"exposure"};
        excluded_table.rows = {{"10"}, {"20"}, {"30"}};
        AnalysisConfiguration excluded_cfg;
        excluded_cfg.reliability.warranty_time = 1000.0;
        excluded_cfg.reliability.time_unit = "hours";
        excluded_cfg.reliability.reliability_at_warranty = 0.9;
        excluded_cfg.reliability.exposure_column = 0;
        excluded_cfg.excluded_rows = {1};  // skip 20
        const auto excluded_page =
            AnalysisService::reliability_warranty(excluded_table, excluded_cfg);
        QVERIFY(excluded_page.facts.warranty.has_value());
        QVERIFY(nearly_equal(excluded_page.facts.warranty->exposure, 40.0));
        QCOMPARE(excluded_page.facts.warranty->exposure_row_count, std::size_t{2});

        AnalysisConfiguration zero_cfg = excluded_cfg;
        zero_cfg.excluded_rows = {0, 1, 2};
        zero_cfg.reliability.exposure = 500.0;  // must NOT invent fallback
        const auto zero_page =
            AnalysisService::reliability_warranty(excluded_table, zero_cfg);
        QVERIFY(!zero_page.diagnostics.empty());
        QCOMPARE(QString::fromStdString(zero_page.diagnostics.front().code),
                 QStringLiteral("warranty_zero_exposure"));
    }

    void warranty_strata_use_pooled_r_and_measured_exposure()
    {
        WarrantySummaryOptions options;
        options.warranty_time = 1000.0;
        options.time_unit = "hours";
        options.exposure = 100.0;
        options.reliability_at_warranty = 0.9;  // F=0.1
        options.reliability_is_prediction = true;
        options.valid_count = 10;

        std::vector<WarrantyStratumInput> strata = {
            {"wear", "failure_mode", 60.0, 3, 1, 4, {0, 1}},
            {"early", "failure_mode", 40.0, 1, 1, 2, {2}}};
        const auto result = summarize_warranty_strata(options, strata);
        QVERIFY(result.ok);
        QVERIFY(result.uses_pooled_reliability);
        QCOMPARE(result.strata.size(), static_cast<std::size_t>(2));
        QVERIFY(nearly_equal(result.strata[0].expected_failures, 6.0));
        QVERIFY(nearly_equal(result.strata[1].expected_failures, 4.0));
        QCOMPARE(QString::fromStdString(result.strata[0].exposure_attribution),
                 QStringLiteral("measured_column"));

        // Zero measured exposure → proportional scalar with warning.
        strata[0].exposure = 0.0;
        strata[1].exposure = 0.0;
        const auto proportional = summarize_warranty_strata(options, strata);
        QVERIFY(proportional.ok);
        QVERIFY(nearly_equal(proportional.strata[0].exposure, 100.0 * 4.0 / 6.0));
        QCOMPARE(QString::fromStdString(proportional.strata[0].exposure_attribution),
                 QStringLiteral("proportional_scalar"));
        bool saw_proportional = false;
        for (const auto& diagnostic : proportional.diagnostics) {
            if (diagnostic.code == "warranty_stratum_exposure_proportional") {
                saw_proportional = true;
            }
        }
        QVERIFY(saw_proportional);
    }

    void warranty_failure_mode_denominators_on_page()
    {
        DataTable table;
        table.columns = {"time", "censor_type", "mode", "exposure"};
        table.rows = {
            {"10", "exact", "wear", "10"},
            {"12", "exact", "wear", "10"},
            {"15", "right", "wear", "20"},
            {"8", "exact", "early", "5"},
            {"30", "right", "", "5"}};
        AnalysisConfiguration configuration;
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.reliability_at_warranty = 0.95;  // F=0.05
        configuration.reliability.censoring_type_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.exposure_column = 3;
        configuration.reliability.model = "weibull";
        const auto page = AnalysisService::reliability_warranty(table, configuration);
        QVERIFY(page.facts.warranty.has_value());
        QCOMPARE(QString::fromStdString(page.facts.warranty->stratum_kind),
                 QStringLiteral("failure_mode"));
        QVERIFY(page.facts.warranty->uses_pooled_reliability);
        QCOMPARE(page.facts.warranty->strata.size(), static_cast<std::size_t>(3));
        QVERIFY(nearly_equal(page.facts.warranty->exposure, 50.0));

        bool found_wear = false;
        bool found_unlabeled = false;
        for (const auto& stratum : page.facts.warranty->strata) {
            if (stratum.label == "wear") {
                found_wear = true;
                QVERIFY(nearly_equal(stratum.exposure, 40.0));
                QCOMPARE(static_cast<int>(stratum.observed_failures), 2);
                QVERIFY(nearly_equal(stratum.expected_failures, 2.0));
                QCOMPARE(QString::fromStdString(stratum.exposure_attribution),
                         QStringLiteral("measured_column"));
                QVERIFY(!stratum.source_rows.empty());
            }
            if (stratum.label == "(unlabeled)") {
                found_unlabeled = true;
                QCOMPARE(static_cast<int>(stratum.censored_count), 1);
            }
        }
        QVERIFY(found_wear);
        QVERIFY(found_unlabeled);

        // Summary table must match Facts after stratum backfill (no stale zeros).
        QCOMPARE(static_cast<int>(page.facts.warranty->observed_failures), 3);
        QCOMPARE(static_cast<int>(page.facts.warranty->valid_count), 5);
        bool summary_counts_ok = false;
        for (const auto& table_item : page.tables) {
            if (table_item.title != "保修摘要") {
                continue;
            }
            for (const auto& row : table_item.rows) {
                if (row.size() >= 2 && row[0] == "Observed failures") {
                    QCOMPARE(QString::fromStdString(row[1]), QStringLiteral("3"));
                    summary_counts_ok = true;
                }
            }
        }
        QVERIFY(summary_counts_ok);

        int formula_ref_count = 0;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "warranty_formula_reference") {
                ++formula_ref_count;
            }
        }
        QCOMPARE(formula_ref_count, 1);

        bool saw_table = false;
        for (const auto& table_item : page.tables) {
            if (table_item.title == "失效模式分母追溯") {
                saw_table = true;
            }
        }
        QVERIFY(saw_table);

        const auto json = datalab::infrastructure::output_page_to_json(page);
        const auto restored = datalab::infrastructure::output_page_from_json(json);
        QVERIFY(restored.facts.warranty.has_value());
        QCOMPARE(restored.facts.warranty->strata.size(),
                 page.facts.warranty->strata.size());
        QCOMPARE(QString::fromStdString(restored.facts.warranty->stratum_kind),
                 QStringLiteral("failure_mode"));
    }

    void turnbull_interval_km_is_formula_reference_not_vendor()
    {
        DataTable table;
        table.columns = {"L", "R"};
        // Mixed exact + interval + right-censored.
        table.rows = {
            {"10", "10"},
            {"20", "20"},
            {"5", "15"},
            {"25", "Inf"},
            {"30", "30"}};
        AnalysisConfiguration configuration;
        configuration.km_interval.left_column = 0;
        configuration.km_interval.right_column = 1;
        auto page = AnalysisService::km_interval(table, configuration);
        QVERIFY(page.facts.km_interval.has_value());
        QCOMPARE(QString::fromStdString(page.facts.km_interval->evidence_type),
                 QStringLiteral("formula_reference"));
        QCOMPARE(QString::fromStdString(page.facts.km_interval->algorithm_id),
                 QStringLiteral("turnbull_npmle_simplified_grid"));
        QCOMPARE(page.facts.km_interval->classic_km_equivalent, false);
        QVERIFY(page.facts.km_interval->interval_censored_count
                    + page.facts.km_interval->right_censored_count >= 1);
        QVERIFY(page.facts.km_interval->evidence_type.find("vendor") == std::string::npos);

        bool saw_scope = false;
        bool saw_not_vendor = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "km_interval_scope"
                || diagnostic.code == "km_interval_evidence") {
                saw_scope = true;
            }
            if (diagnostic.code == "km_interval_not_vendor_oracle") {
                saw_not_vendor = true;
            }
        }
        QVERIFY(saw_scope);
        QVERIFY(saw_not_vendor);

        datalab::application::InterpretationService::enrich(page);
        bool saw_limit = false;
        for (const auto& section : page.interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("formula_reference") != std::string::npos
                    && bullet.find("vendor_oracle") != std::string::npos) {
                    saw_limit = true;
                }
            }
        }
        QVERIFY(saw_limit);

        const auto json = datalab::infrastructure::output_page_to_json(page);
        const auto restored = datalab::infrastructure::output_page_from_json(json);
        QVERIFY(restored.facts.km_interval.has_value());
        QCOMPARE(restored.facts.km_interval->evidence_type,
                 page.facts.km_interval->evidence_type);
        QCOMPARE(restored.facts.km_interval->algorithm_id,
                 page.facts.km_interval->algorithm_id);
        QCOMPARE(restored.facts.km_interval->classic_km_equivalent, false);
    }

    void turnbull_exact_only_marks_classic_km_equivalent_flag_without_vendor_claim()
    {
        DataTable table;
        table.columns = {"L", "R"};
        table.rows = {{"10", "10"}, {"20", "20"}, {"30", "30"}};
        AnalysisConfiguration configuration;
        configuration.km_interval.left_column = 0;
        configuration.km_interval.right_column = 1;
        const auto page = AnalysisService::km_interval(table, configuration);
        QVERIFY(page.facts.km_interval.has_value());
        QCOMPARE(page.facts.km_interval->classic_km_equivalent, true);
        QCOMPARE(QString::fromStdString(page.facts.km_interval->evidence_type),
                 QStringLiteral("formula_reference"));
        QVERIFY(page.facts.km_interval->converged || !page.plots.empty());
    }

    void deserialize_rejects_fake_vendor_oracle_on_km_interval()
    {
        DataTable table;
        table.columns = {"L", "R"};
        table.rows = {{"10", "10"}, {"20", "20"}, {"30", "30"}};
        AnalysisConfiguration configuration;
        configuration.km_interval.left_column = 0;
        configuration.km_interval.right_column = 1;
        const auto page = AnalysisService::km_interval(table, configuration);
        auto json = datalab::infrastructure::output_page_to_json(page);
        auto facts = json.value(QStringLiteral("facts")).toObject();
        auto km = facts.value(QStringLiteral("km_interval")).toObject();
        km.insert(QStringLiteral("evidence_type"), QStringLiteral("vendor_oracle"));
        facts.insert(QStringLiteral("km_interval"), km);
        json.insert(QStringLiteral("facts"), facts);
        const auto restored = datalab::infrastructure::output_page_from_json(json);
        QVERIFY(restored.facts.km_interval.has_value());
        QCOMPARE(QString::fromStdString(restored.facts.km_interval->evidence_type),
                 QStringLiteral("formula_reference"));

        km.insert(QStringLiteral("evidence_type"), QStringLiteral("golden"));
        facts.insert(QStringLiteral("km_interval"), km);
        json.insert(QStringLiteral("facts"), facts);
        const auto restored_golden = datalab::infrastructure::output_page_from_json(json);
        QVERIFY(restored_golden.facts.km_interval.has_value());
        QCOMPARE(QString::fromStdString(restored_golden.facts.km_interval->evidence_type),
                 QStringLiteral("formula_reference"));
    }

    void cause_specific_mode_fits_are_formula_reference()
    {
        using datalab::domain::statistics::fit_reliability_by_failure_mode;
        std::vector<CensoringObservation> observations = {
            {CensoringType::exact, 10.0, 0, 0, "h", "", "wear", std::nullopt, 0},
            {CensoringType::exact, 12.0, 0, 0, "h", "", "wear", std::nullopt, 1},
            {CensoringType::exact, 14.0, 0, 0, "h", "", "wear", std::nullopt, 2},
            {CensoringType::exact, 20.0, 0, 0, "h", "", "crack", std::nullopt, 3},
            {CensoringType::exact, 22.0, 0, 0, "h", "", "crack", std::nullopt, 4},
            {CensoringType::exact, 24.0, 0, 0, "h", "", "crack", std::nullopt, 5},
            {CensoringType::right, 30.0, 0, 0, "h", "", "", std::nullopt, 6},
            {CensoringType::right, 32.0, 0, 0, "h", "", "", std::nullopt, 7},
        };
        const auto fits = fit_reliability_by_failure_mode(observations, "weibull", 15.0);
        QVERIFY(fits.ran);
        QCOMPARE(fits.fitting_scheme, std::string("cause_specific"));
        QCOMPARE(static_cast<int>(fits.modes.size()), 2);
        bool saw_wear = false;
        bool saw_crack = false;
        for (const auto& mode : fits.modes) {
            QCOMPARE(mode.evidence_type, std::string("formula_reference"));
            QCOMPARE(mode.algorithm_id,
                     std::string("cause_specific_right_censored_competing"));
            QVERIFY(mode.competing_failure_count > 0);
            if (mode.failure_mode == "wear") {
                saw_wear = true;
                QCOMPARE(static_cast<int>(mode.failure_count), 3);
            }
            if (mode.failure_mode == "crack") {
                saw_crack = true;
                QCOMPARE(static_cast<int>(mode.failure_count), 3);
            }
        }
        QVERIFY(saw_wear);
        QVERIFY(saw_crack);

        // Warranty strata: mode-specific R changes expected failures vs pooled.
        WarrantySummaryOptions options;
        options.warranty_time = 15.0;
        options.exposure = 1000.0;
        options.reliability_at_warranty = 0.9;
        options.model_name = "weibull";
        std::vector<WarrantyStratumInput> strata;
        WarrantyStratumInput wear;
        wear.label = "wear";
        wear.kind = "failure_mode";
        wear.exposure = 500.0;
        wear.valid_count = 3;
        wear.observed_failures = 3;
        for (const auto& mode : fits.modes) {
            if (mode.failure_mode == "wear" && mode.reliability_at_warranty.has_value()) {
                wear.reliability_at_warranty = mode.reliability_at_warranty;
            }
        }
        strata.push_back(wear);
        WarrantyStratumInput crack;
        crack.label = "crack";
        crack.kind = "failure_mode";
        crack.exposure = 500.0;
        crack.valid_count = 3;
        crack.observed_failures = 3;
        strata.push_back(crack);  // pooled fallback
        const auto summary = summarize_warranty_strata(options, strata);
        QVERIFY(summary.ok);
        QVERIFY(summary.uses_mode_specific_reliability);
        QVERIFY(summary.uses_pooled_reliability);  // crack still pooled
        QCOMPARE(static_cast<int>(summary.strata.size()), 2);
        QVERIFY(summary.strata[0].uses_mode_specific_reliability);
        QVERIFY(!summary.strata[1].uses_mode_specific_reliability);
        // Mode-specific expected != pooled 500*0.1 when R differs from 0.9.
        if (wear.reliability_at_warranty.has_value()
            && !nearly_equal(*wear.reliability_at_warranty, 0.9, 1e-6)) {
            QVERIFY(!nearly_equal(summary.strata[0].expected_failures, 50.0, 1e-6));
        }
        QVERIFY(nearly_equal(summary.strata[1].expected_failures, 50.0, 1e-9));
    }

    void aalen_johansen_cif_is_formula_reference_not_fine_gray()
    {
        using datalab::domain::statistics::aalen_johansen_cif;
        std::vector<CensoringObservation> observations = {
            {CensoringType::exact, 10.0, 0, 0, "h", "", "wear", std::nullopt, 0},
            {CensoringType::exact, 20.0, 0, 0, "h", "", "crack", std::nullopt, 1},
            {CensoringType::exact, 30.0, 0, 0, "h", "", "wear", std::nullopt, 2},
            {CensoringType::right, 40.0, 0, 0, "h", "", "", std::nullopt, 3},
            {CensoringType::right, 50.0, 0, 0, "h", "", "", std::nullopt, 4},
        };
        const auto cif = aalen_johansen_cif(observations, 25.0);
        QVERIFY(cif.ran);
        QCOMPARE(cif.evidence_type, std::string("formula_reference"));
        QCOMPARE(cif.algorithm_id, std::string("aalen_johansen_cif"));
        QCOMPARE(static_cast<int>(cif.modes.size()), 2);

        // Hand check: n=5. At t=10 wear: CIF_wear = 1*(1/5)=0.2, S=0.8
        // At t=20 crack: CIF_crack = 0.8*(1/4)=0.2, S=0.6
        // At t=30 wear: CIF_wear = 0.2+0.6*(1/3)=0.4
        const AalenJohansenCifModeResult* wear = nullptr;
        const AalenJohansenCifModeResult* crack = nullptr;
        for (const auto& mode : cif.modes) {
            if (mode.failure_mode == "wear") {
                wear = &mode;
            }
            if (mode.failure_mode == "crack") {
                crack = &mode;
            }
        }
        QVERIFY(wear != nullptr);
        QVERIFY(crack != nullptr);
        QVERIFY(wear->cif_at_last_event.has_value());
        QVERIFY(crack->cif_at_last_event.has_value());
        QVERIFY(nearly_equal(*wear->cif_at_last_event, 0.4, 1e-9));
        QVERIFY(nearly_equal(*crack->cif_at_last_event, 0.2, 1e-9));
        QVERIFY(wear->cif_at_warranty.has_value());
        QVERIFY(nearly_equal(*wear->cif_at_warranty, 0.2, 1e-9));  // last event ≤25 is t=10
        QVERIFY(crack->cif_at_warranty.has_value());
        QVERIFY(nearly_equal(*crack->cif_at_warranty, 0.2, 1e-9));

        bool saw_not_fine_gray = false;
        bool saw_scope = false;
        for (const auto& diagnostic : cif.diagnostics) {
            if (diagnostic.code == "cif_not_fine_gray_multivar"
                || diagnostic.code == "cif_not_fine_gray") {
                saw_not_fine_gray = true;
                QVERIFY(diagnostic.message.find("Fine-Gray") != std::string::npos);
            }
            if (diagnostic.code == "cif_aalen_johansen_scope") {
                saw_scope = true;
                QVERIFY(diagnostic.message.find("vendor_oracle") != std::string::npos);
            }
        }
        QVERIFY(saw_not_fine_gray);
        QVERIFY(saw_scope);
    }

    void fine_gray_binary_ipcw_is_formula_reference()
    {
        using datalab::domain::statistics::fine_gray_binary;
        std::vector<CensoringObservation> observations;
        // Group A: earlier target (wear) failures
        for (int i = 0; i < 8; ++i) {
            observations.push_back(
                {CensoringType::exact, 10.0 + i, 0, 0, "h", "A", "wear", std::nullopt,
                 static_cast<std::size_t>(i)});
        }
        for (int i = 0; i < 4; ++i) {
            observations.push_back(
                {CensoringType::exact, 20.0 + i, 0, 0, "h", "A", "crack", std::nullopt,
                 static_cast<std::size_t>(20 + i)});
        }
        for (int i = 0; i < 4; ++i) {
            observations.push_back(
                {CensoringType::right, 40.0 + i, 0, 0, "h", "A", "", std::nullopt,
                 static_cast<std::size_t>(40 + i)});
        }
        // Group B: later / fewer wear failures
        for (int i = 0; i < 3; ++i) {
            observations.push_back(
                {CensoringType::exact, 30.0 + i, 0, 0, "h", "B", "wear", std::nullopt,
                 static_cast<std::size_t>(60 + i)});
        }
        for (int i = 0; i < 5; ++i) {
            observations.push_back(
                {CensoringType::exact, 15.0 + i, 0, 0, "h", "B", "crack", std::nullopt,
                 static_cast<std::size_t>(80 + i)});
        }
        for (int i = 0; i < 6; ++i) {
            observations.push_back(
                {CensoringType::right, 45.0 + i, 0, 0, "h", "B", "", std::nullopt,
                 static_cast<std::size_t>(100 + i)});
        }
        const auto fg = fine_gray_binary(observations, "wear");
        QVERIFY(fg.ran);
        QVERIFY(fg.converged);
        QCOMPARE(fg.evidence_type, std::string("formula_reference"));
        QCOMPARE(fg.algorithm_id, std::string("fine_gray_binary_ipcw"));
        QCOMPARE(fg.target_failure_mode, std::string("wear"));
        QVERIFY(fg.beta.has_value());
        QVERIFY(fg.hazard_ratio.has_value());
        QVERIFY(fg.p_value.has_value());
        QVERIFY(fg.target_failures >= 2);
        bool saw_scope = false;
        for (const auto& diagnostic : fg.diagnostics) {
            if (diagnostic.code == "fine_gray_scope") {
                saw_scope = true;
            }
            QVERIFY(diagnostic.message.find("vendor_oracle") == std::string::npos
                    || diagnostic.code == "fine_gray_scope"
                    || diagnostic.code == "fine_gray_not_vendor_oracle");
        }
        QVERIFY(saw_scope);
    }

    void fine_gray_continuous_ipcw_is_formula_reference()
    {
        using datalab::domain::statistics::fine_gray_continuous;
        std::vector<CensoringObservation> observations;
        std::vector<double> x;
        for (int i = 0; i < 12; ++i) {
            observations.push_back(
                {CensoringType::exact, 8.0 + 0.5 * i, 0, 0, "h", "", "wear", std::nullopt,
                 static_cast<std::size_t>(i)});
            x.push_back(static_cast<double>(i));  // higher x → earlier wear
        }
        for (int i = 0; i < 8; ++i) {
            observations.push_back(
                {CensoringType::exact, 20.0 + i, 0, 0, "h", "", "crack", std::nullopt,
                 static_cast<std::size_t>(20 + i)});
            x.push_back(static_cast<double>(i % 4));
        }
        for (int i = 0; i < 8; ++i) {
            observations.push_back(
                {CensoringType::right, 40.0 + i, 0, 0, "h", "", "", std::nullopt,
                 static_cast<std::size_t>(40 + i)});
            x.push_back(2.0);
        }
        const auto fg = fine_gray_continuous(observations, x, "wear", "dose");
        QVERIFY(fg.ran);
        QVERIFY(fg.converged);
        QCOMPARE(fg.kind, std::string("continuous"));
        QCOMPARE(fg.evidence_type, std::string("formula_reference"));
        QCOMPARE(fg.algorithm_id, std::string("fine_gray_continuous_ipcw"));
        QCOMPARE(fg.covariate_name, std::string("dose"));
        QVERIFY(fg.covariate_mean.has_value());
        QVERIFY(fg.beta.has_value());
        QVERIFY(fg.hazard_ratio.has_value());
    }

    void fine_gray_multi_ipcw_is_formula_reference()
    {
        using datalab::domain::statistics::fine_gray_multi;
        std::vector<CensoringObservation> observations;
        std::vector<std::vector<double>> x;
        // Enough wear failures for p=2 (need ≥10 target events).
        for (int i = 0; i < 12; ++i) {
            observations.push_back(
                {CensoringType::exact, 8.0 + 0.4 * i, 0, 0, "h", "", "wear", std::nullopt,
                 static_cast<std::size_t>(i)});
            x.push_back({static_cast<double>(i), static_cast<double>(i % 3)});
        }
        for (int i = 0; i < 8; ++i) {
            observations.push_back(
                {CensoringType::exact, 22.0 + i, 0, 0, "h", "", "crack", std::nullopt,
                 static_cast<std::size_t>(20 + i)});
            x.push_back({static_cast<double>(i % 5), 1.0});
        }
        for (int i = 0; i < 8; ++i) {
            observations.push_back(
                {CensoringType::right, 40.0 + i, 0, 0, "h", "", "", std::nullopt,
                 static_cast<std::size_t>(40 + i)});
            x.push_back({2.0, 0.0});
        }
        const auto fg = fine_gray_multi(observations, x, {"dose", "batch"}, "wear");
        QVERIFY(fg.ran);
        QVERIFY(fg.converged);
        QCOMPARE(fg.kind, std::string("multi"));
        QCOMPARE(fg.evidence_type, std::string("formula_reference"));
        QCOMPARE(fg.algorithm_id, std::string("fine_gray_multi_ipcw"));
        QCOMPARE(static_cast<int>(fg.terms.size()), 2);
        QCOMPARE(fg.terms[0].name, std::string("dose"));
        QCOMPARE(fg.terms[1].name, std::string("batch"));
        QVERIFY(fg.terms[0].beta.has_value());
        QVERIFY(fg.terms[1].beta.has_value());
        QVERIFY(fg.terms[0].hazard_ratio.has_value());
        QVERIFY(fg.beta.has_value());  // first term mirrored
    }

    void reliability_page_populates_fine_gray_multi()
    {
        DataTable table;
        table.columns = {"t", "event", "mode", "dose", "temp"};
        table.rows = {
            {"8", "1", "wear", "0", "1"}, {"8.4", "1", "wear", "1", "0"},
            {"8.8", "1", "wear", "2", "1"}, {"9.2", "1", "wear", "3", "0"},
            {"9.6", "1", "wear", "4", "1"}, {"10", "1", "wear", "5", "0"},
            {"10.4", "1", "wear", "6", "1"}, {"10.8", "1", "wear", "7", "0"},
            {"11.2", "1", "wear", "8", "1"}, {"11.6", "1", "wear", "9", "0"},
            {"12", "1", "wear", "10", "1"}, {"12.4", "1", "wear", "11", "0"},
            {"22", "1", "crack", "1", "1"}, {"23", "1", "crack", "2", "0"},
            {"24", "1", "crack", "3", "1"}, {"25", "1", "crack", "4", "0"},
            {"26", "1", "crack", "0", "1"}, {"27", "1", "crack", "1", "0"},
            {"28", "1", "crack", "2", "1"}, {"29", "1", "crack", "3", "0"},
            {"40", "0", "", "2", "0"}, {"41", "0", "", "2", "1"},
            {"42", "0", "", "2", "0"}, {"43", "0", "", "2", "1"},
            {"44", "0", "", "2", "0"}, {"45", "0", "", "2", "1"},
            {"46", "0", "", "2", "0"}, {"47", "0", "", "2", "1"},
        };
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.event_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.covariate_columns = {3, 4};
        configuration.reliability.model = "kaplan_meier";
        const auto page = AnalysisService::reliability(table, configuration);
        QVERIFY(page.facts.reliability.has_value());
        QCOMPARE(page.facts.reliability->fine_gray_algorithm_id,
                 std::string("fine_gray_multi_ipcw"));
        QCOMPARE(page.facts.reliability->fine_gray_kind, std::string("multi"));
        QCOMPARE(page.facts.reliability->fine_gray_evidence_type,
                 std::string("formula_reference"));
        QVERIFY(page.facts.reliability->fine_gray_converged);
        QCOMPARE(static_cast<int>(page.facts.reliability->fine_gray_terms.size()), 2);
        QCOMPARE(page.facts.reliability->fine_gray_terms[0].name, std::string("dose"));
        QCOMPARE(page.facts.reliability->fine_gray_terms[1].name, std::string("temp"));

        const auto json = datalab::infrastructure::output_page_to_json(page);
        auto restored = datalab::infrastructure::output_page_from_json(json);
        QVERIFY(restored.facts.reliability.has_value());
        QCOMPARE(restored.facts.reliability->fine_gray_algorithm_id,
                 std::string("fine_gray_multi_ipcw"));
        QCOMPARE(static_cast<int>(restored.facts.reliability->fine_gray_terms.size()), 2);

        auto facts = json.value(QStringLiteral("facts")).toObject();
        auto reliability = facts.value(QStringLiteral("reliability")).toObject();
        reliability.insert(QStringLiteral("fine_gray_evidence_type"),
                           QStringLiteral("vendor_oracle"));
        reliability.insert(QStringLiteral("fine_gray_algorithm_id"),
                           QStringLiteral("vendor_finegray"));
        facts.insert(QStringLiteral("reliability"), reliability);
        auto tampered = json;
        tampered.insert(QStringLiteral("facts"), facts);
        const auto clamped = datalab::infrastructure::output_page_from_json(tampered);
        QVERIFY(clamped.facts.reliability.has_value());
        QCOMPARE(clamped.facts.reliability->fine_gray_evidence_type,
                 std::string("formula_reference"));
        QCOMPARE(clamped.facts.reliability->fine_gray_algorithm_id,
                 std::string("fine_gray_multi_ipcw"));
    }

    void reliability_page_populates_fine_gray_continuous()
    {
        DataTable table;
        table.columns = {"t", "event", "mode", "dose"};
        for (int i = 0; i < 10; ++i) {
            table.rows.push_back(
                {std::to_string(8.0 + i), "1", "wear", std::to_string(i)});
        }
        for (int i = 0; i < 6; ++i) {
            table.rows.push_back(
                {std::to_string(25.0 + i), "1", "crack", std::to_string(i % 3)});
        }
        for (int i = 0; i < 6; ++i) {
            table.rows.push_back({std::to_string(40.0 + i), "0", "", "2"});
        }
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.event_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.covariate_column = 3;
        configuration.reliability.model = "kaplan_meier";
        const auto page = AnalysisService::reliability(table, configuration);
        QVERIFY(page.facts.reliability.has_value());
        QCOMPARE(page.facts.reliability->fine_gray_algorithm_id,
                 std::string("fine_gray_continuous_ipcw"));
        QCOMPARE(page.facts.reliability->fine_gray_kind, std::string("continuous"));
        QCOMPARE(page.facts.reliability->fine_gray_evidence_type,
                 std::string("formula_reference"));
        QVERIFY(page.facts.reliability->fine_gray_converged);
        QVERIFY(page.facts.reliability->fine_gray_beta.has_value());

        const auto json = datalab::infrastructure::output_page_to_json(page);
        auto facts = json.value(QStringLiteral("facts")).toObject();
        auto reliability = facts.value(QStringLiteral("reliability")).toObject();
        reliability.insert(QStringLiteral("fine_gray_evidence_type"),
                           QStringLiteral("golden"));
        reliability.insert(QStringLiteral("fine_gray_algorithm_id"),
                           QStringLiteral("commercial_fg"));
        facts.insert(QStringLiteral("reliability"), reliability);
        auto tampered = json;
        tampered.insert(QStringLiteral("facts"), facts);
        const auto clamped = datalab::infrastructure::output_page_from_json(tampered);
        QVERIFY(clamped.facts.reliability.has_value());
        QCOMPARE(clamped.facts.reliability->fine_gray_evidence_type,
                 std::string("formula_reference"));
        QCOMPARE(clamped.facts.reliability->fine_gray_algorithm_id,
                 std::string("fine_gray_continuous_ipcw"));
        QCOMPARE(clamped.facts.reliability->fine_gray_kind, std::string("continuous"));
    }

    void reliability_page_populates_fine_gray_when_group_binary()
    {
        DataTable table;
        table.columns = {"t", "event", "mode", "grp"};
        table.rows = {
            {"10", "1", "wear", "A"}, {"11", "1", "wear", "A"}, {"12", "1", "wear", "A"},
            {"13", "1", "wear", "A"}, {"14", "1", "wear", "A"}, {"15", "1", "wear", "A"},
            {"20", "1", "crack", "A"}, {"21", "1", "crack", "A"},
            {"40", "0", "", "A"}, {"41", "0", "", "A"},
            {"30", "1", "wear", "B"}, {"31", "1", "wear", "B"}, {"32", "1", "wear", "B"},
            {"16", "1", "crack", "B"}, {"17", "1", "crack", "B"}, {"18", "1", "crack", "B"},
            {"45", "0", "", "B"}, {"46", "0", "", "B"}, {"47", "0", "", "B"},
        };
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.event_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.group_column = 3;
        configuration.reliability.model = "kaplan_meier";
        const auto page = AnalysisService::reliability(table, configuration);
        QVERIFY(page.facts.reliability.has_value());
        QCOMPARE(page.facts.reliability->fine_gray_algorithm_id,
                 std::string("fine_gray_binary_ipcw"));
        QCOMPARE(page.facts.reliability->fine_gray_evidence_type,
                 std::string("formula_reference"));
        QVERIFY(page.facts.reliability->fine_gray_converged);
        QVERIFY(page.facts.reliability->fine_gray_beta.has_value());

        const auto json = datalab::infrastructure::output_page_to_json(page);
        auto restored = datalab::infrastructure::output_page_from_json(json);
        QVERIFY(restored.facts.reliability.has_value());
        QCOMPARE(restored.facts.reliability->fine_gray_algorithm_id,
                 page.facts.reliability->fine_gray_algorithm_id);
        QCOMPARE(restored.facts.reliability->fine_gray_beta.has_value(), true);

        auto facts = json.value(QStringLiteral("facts")).toObject();
        auto reliability = facts.value(QStringLiteral("reliability")).toObject();
        reliability.insert(QStringLiteral("fine_gray_evidence_type"),
                           QStringLiteral("vendor_oracle"));
        reliability.insert(QStringLiteral("fine_gray_algorithm_id"),
                           QStringLiteral("vendor_finegray"));
        facts.insert(QStringLiteral("reliability"), reliability);
        auto tampered = json;
        tampered.insert(QStringLiteral("facts"), facts);
        const auto clamped = datalab::infrastructure::output_page_from_json(tampered);
        QVERIFY(clamped.facts.reliability.has_value());
        QCOMPARE(clamped.facts.reliability->fine_gray_evidence_type,
                 std::string("formula_reference"));
        QCOMPARE(clamped.facts.reliability->fine_gray_algorithm_id,
                 std::string("fine_gray_binary_ipcw"));
    }

    void reliability_page_populates_cif_facts()
    {
        DataTable table;
        table.columns = {"t", "event", "mode"};
        table.rows = {
            {"10", "1", "wear"},
            {"20", "1", "crack"},
            {"30", "1", "wear"},
            {"40", "0", ""},
            {"50", "0", ""},
        };
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.event_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.model = "kaplan_meier";
        configuration.reliability.warranty_time = 25.0;
        const auto page = AnalysisService::reliability(table, configuration);
        QVERIFY(page.facts.reliability.has_value());
        QCOMPARE(page.facts.reliability->cif_algorithm_id,
                 std::string("aalen_johansen_cif"));
        QCOMPARE(page.facts.reliability->cif_evidence_type,
                 std::string("formula_reference"));
        QVERIFY(!page.facts.reliability->cif_modes.empty());

        bool has_cif_table = false;
        for (const auto& table_item : page.tables) {
            if (table_item.title.find("CIF") != std::string::npos
                || table_item.title.find("Aalen") != std::string::npos) {
                has_cif_table = true;
            }
        }
        QVERIFY(has_cif_table);

        const auto json = datalab::infrastructure::output_page_to_json(page);
        auto restored = datalab::infrastructure::output_page_from_json(json);
        QVERIFY(restored.facts.reliability.has_value());
        QCOMPARE(restored.facts.reliability->cif_modes.size(),
                 page.facts.reliability->cif_modes.size());
        QCOMPARE(restored.facts.reliability->cif_algorithm_id,
                 std::string("aalen_johansen_cif"));

        auto facts = json.value(QStringLiteral("facts")).toObject();
        auto reliability = facts.value(QStringLiteral("reliability")).toObject();
        reliability.insert(QStringLiteral("cif_evidence_type"),
                           QStringLiteral("vendor_oracle"));
        reliability.insert(QStringLiteral("cif_algorithm_id"),
                           QStringLiteral("fine_gray"));
        facts.insert(QStringLiteral("reliability"), reliability);
        auto tampered = json;
        tampered.insert(QStringLiteral("facts"), facts);
        const auto clamped = datalab::infrastructure::output_page_from_json(tampered);
        QVERIFY(clamped.facts.reliability.has_value());
        QCOMPARE(clamped.facts.reliability->cif_evidence_type,
                 std::string("formula_reference"));
        QCOMPARE(clamped.facts.reliability->cif_algorithm_id,
                 std::string("aalen_johansen_cif"));
    }

    void reliability_page_populates_mode_fits_facts()
    {
        DataTable table;
        table.columns = {"t", "event", "mode"};
        table.rows = {
            {"10", "1", "wear"},
            {"12", "1", "wear"},
            {"14", "1", "wear"},
            {"20", "1", "crack"},
            {"22", "1", "crack"},
            {"24", "1", "crack"},
            {"30", "0", ""},
            {"32", "0", ""},
        };
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.event_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.model = "weibull";
        configuration.reliability.warranty_time = 15.0;
        const auto page = AnalysisService::reliability(table, configuration);
        QVERIFY(page.facts.reliability.has_value());
        QVERIFY(!page.facts.reliability->mode_fits.empty());
        QCOMPARE(page.facts.reliability->mode_fit_scheme, std::string("cause_specific"));
        QCOMPARE(page.facts.reliability->mode_fits.front().evidence_type,
                 std::string("formula_reference"));

        const auto json = datalab::infrastructure::output_page_to_json(page);
        auto restored = datalab::infrastructure::output_page_from_json(json);
        QVERIFY(restored.facts.reliability.has_value());
        QCOMPARE(restored.facts.reliability->mode_fits.size(),
                 page.facts.reliability->mode_fits.size());

        // Fake vendor_oracle on mode fit must clamp.
        auto facts = json.value(QStringLiteral("facts")).toObject();
        auto reliability = facts.value(QStringLiteral("reliability")).toObject();
        auto mode_fits = reliability.value(QStringLiteral("mode_fits")).toArray();
        QVERIFY(!mode_fits.isEmpty());
        auto first = mode_fits.at(0).toObject();
        first.insert(QStringLiteral("evidence_type"), QStringLiteral("vendor_oracle"));
        mode_fits.replace(0, first);
        reliability.insert(QStringLiteral("mode_fits"), mode_fits);
        facts.insert(QStringLiteral("reliability"), reliability);
        auto tampered = json;
        tampered.insert(QStringLiteral("facts"), facts);
        const auto clamped = datalab::infrastructure::output_page_from_json(tampered);
        QVERIFY(clamped.facts.reliability.has_value());
        QCOMPARE(clamped.facts.reliability->mode_fits.front().evidence_type,
                 std::string("formula_reference"));

        first.insert(QStringLiteral("evidence_type"), QStringLiteral("golden"));
        mode_fits.replace(0, first);
        reliability.insert(QStringLiteral("mode_fits"), mode_fits);
        facts.insert(QStringLiteral("reliability"), reliability);
        tampered.insert(QStringLiteral("facts"), facts);
        const auto clamped_golden =
            datalab::infrastructure::output_page_from_json(tampered);
        QVERIFY(clamped_golden.facts.reliability.has_value());
        QCOMPARE(clamped_golden.facts.reliability->mode_fits.front().evidence_type,
                 std::string("formula_reference"));

        auto enriched = page;
        datalab::application::InterpretationService::enrich(enriched);
        bool saw_mode_limit = false;
        for (const auto& section : enriched.interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("formula_reference") != std::string::npos
                    && bullet.find("vendor_oracle") != std::string::npos) {
                    saw_mode_limit = true;
                }
            }
        }
        QVERIFY(saw_mode_limit);
    }

    void censoring_worksheet_round_trip_preserves_types_and_rejects_bad_interval()
    {
        using datalab::domain::statistics::censoring_observations_from_worksheet;
        using datalab::domain::statistics::censoring_observations_to_worksheet;
        using datalab::domain::statistics::censoring_type_id;

        std::vector<CensoringObservation> observations = {
            {CensoringType::exact, 10.0, 0, 0, "h", "G1", "wear", 1.5, 0},
            {CensoringType::right, 20.0, 0, 0, "h", "G1", "", std::nullopt, 1},
            {CensoringType::left, 8.0, 0, 0, "h", "G1", "", std::nullopt, 3},
            {CensoringType::interval, 0.0, 5.0, 15.0, "h", "G2", "crack", 2.0, 2},
        };
        const auto sheet = censoring_observations_to_worksheet(observations);
        QCOMPARE(sheet.name, std::string("censoring_observations"));
        QCOMPARE(static_cast<int>(sheet.rows.size()), 4);
        QCOMPARE(sheet.rows[0][2], std::string("exact"));
        QCOMPARE(sheet.rows[1][2], censoring_type_id(CensoringType::right));
        QCOMPARE(sheet.rows[2][2], std::string("left"));
        QCOMPARE(sheet.rows[3][2], std::string("interval"));
        QVERIFY(!sheet.rows[3][6].empty());
        QVERIFY(!sheet.rows[3][7].empty());

        const auto imported = censoring_observations_from_worksheet(sheet);
        QVERIFY(imported.ok);
        QCOMPARE(static_cast<int>(imported.observations.size()), 4);
        QCOMPARE(imported.observations[0].type, CensoringType::exact);
        QCOMPARE(imported.observations[0].failure_mode, std::string("wear"));
        QCOMPARE(imported.observations[1].type, CensoringType::right);
        QCOMPARE(imported.observations[2].type, CensoringType::left);
        QCOMPARE(imported.observations[3].type, CensoringType::interval);
        QVERIFY(nearly_equal(imported.observations[3].interval_left, 5.0));
        QVERIFY(nearly_equal(imported.observations[3].interval_right, 15.0));
        QCOMPARE(imported.observations[0].source_row, std::size_t{0});
        QCOMPARE(imported.observations[3].source_row, std::size_t{2});

        DataTable bad = sheet;
        // Drop interval bounds columns content for interval row.
        bad.rows[3][6].clear();
        bad.rows[3][7].clear();
        const auto rejected = censoring_observations_from_worksheet(bad);
        QVERIFY(!rejected.ok);

        DataTable table;
        table.columns = {"t", "event"};
        table.rows = {{"10", "1"}, {"20", "0"}, {"30", "1"}};
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.event_column = 1;
        configuration.reliability.model = "kaplan_meier";
        const auto page = AnalysisService::reliability(table, configuration);
        QVERIFY(page.worksheet_export.has_value());
        QCOMPARE(page.worksheet_export->name, std::string("censoring_observations"));
        QCOMPARE(page.worksheet_export->rows.size(), std::size_t{3});
        bool saw_export_diag = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "censoring_worksheet_export_ready") {
                saw_export_diag = true;
            }
        }
        QVERIFY(saw_export_diag);
        const auto from_page =
            censoring_observations_from_worksheet(*page.worksheet_export);
        QVERIFY(from_page.ok);
        QCOMPARE(from_page.observations[0].type, CensoringType::exact);
        QCOMPARE(from_page.observations[1].type, CensoringType::right);

        const auto json = datalab::infrastructure::output_page_to_json(page);
        QVERIFY(json.contains(QStringLiteral("worksheet_export")));
        const auto restored = datalab::infrastructure::output_page_from_json(json);
        QVERIFY(restored.worksheet_export.has_value());
        QCOMPARE(restored.worksheet_export->name, std::string("censoring_observations"));
        QCOMPARE(restored.worksheet_export->rows.size(), page.worksheet_export->rows.size());
        QCOMPARE(restored.worksheet_export->columns, page.worksheet_export->columns);
        const auto from_json =
            censoring_observations_from_worksheet(*restored.worksheet_export);
        QVERIFY(from_json.ok);
        QCOMPARE(from_json.observations[0].type, CensoringType::exact);
        QCOMPARE(from_json.observations[1].type, CensoringType::right);
    }

    void warranty_page_attaches_mode_specific_reliability_when_time_present()
    {
        DataTable table;
        table.columns = {"t", "event", "mode", "exposure"};
        table.rows = {
            {"10", "1", "wear", "100"},
            {"12", "1", "wear", "100"},
            {"14", "1", "wear", "100"},
            {"20", "1", "crack", "100"},
            {"22", "1", "crack", "100"},
            {"24", "1", "crack", "100"},
            {"30", "0", "wear", "100"},
            {"32", "0", "crack", "100"},
        };
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.event_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.exposure_column = 3;
        configuration.reliability.warranty_time = 15.0;
        configuration.reliability.reliability_at_warranty = 0.9;
        configuration.reliability.model = "weibull";
        const auto page =
            AnalysisService::reliability_warranty(table, configuration);
        QVERIFY(page.facts.warranty.has_value());
        QVERIFY(page.facts.warranty->uses_mode_specific_reliability);
        bool any_mode = false;
        for (const auto& stratum : page.facts.warranty->strata) {
            if (stratum.uses_mode_specific_reliability) {
                any_mode = true;
                QVERIFY(stratum.reliability_at_warranty.has_value());
            }
        }
        QVERIFY(any_mode);

        bool table_has_cause_specific = false;
        for (const auto& table_spec : page.tables) {
            for (const auto& row : table_spec.rows) {
                for (const auto& cell : row) {
                    if (cell == "cause_specific") {
                        table_has_cause_specific = true;
                    }
                }
            }
        }
        QVERIFY(table_has_cause_specific);

        auto enriched = page;
        datalab::application::InterpretationService::enrich(enriched);
        bool saw_cause = false;
        for (const auto& section : enriched.interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("cause-specific") != std::string::npos
                    || bullet.find("分模式") != std::string::npos) {
                    saw_cause = true;
                }
            }
        }
        QVERIFY(saw_cause);
    }
};

QTEST_MAIN(ReliabilityPhase5Test)
#include "reliability_phase5_test.moc"
