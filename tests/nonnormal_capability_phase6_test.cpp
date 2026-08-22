#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/quality_types.h"
#include "domain/statistics/box_cox.h"
#include "domain/statistics/johnson_transform.h"
#include "domain/statistics/hartigan_dip.h"
#include "domain/statistics/gaussian_mixture_2.h"
#include "domain/statistics/process_capability.h"
#include "infrastructure/output_serialization.h"

#include <QtTest/QtTest>

#include <cmath>
#include <vector>

using datalab::application::AnalysisService;
using datalab::application::InterpretationService;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::SpecificationLimits;
using datalab::domain::statistics::box_cox_apply;
using datalab::domain::statistics::box_cox_limits_order_ok;
using datalab::domain::statistics::box_cox_transform;
using datalab::domain::statistics::box_cox_transform_limit;
using datalab::domain::statistics::fit_johnson_transform;
using datalab::domain::statistics::johnson_inverse_value;
using datalab::domain::statistics::johnson_transform_value;

namespace {

bool nearly_equal(double a, double b, double tol = 1e-9)
{
    return std::fabs(a - b) <= tol;
}

}  // namespace

class NonNormalCapabilityPhase6Test final : public QObject {
    Q_OBJECT

private slots:
    void box_cox_lambda_special_cases_and_limit_order()
    {
        const std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0};
        const auto identity = box_cox_transform(values, 1.0, false);
        QVERIFY(identity.diagnostics.empty());
        QCOMPARE(identity.lambda, 1.0);
        QVERIFY(nearly_equal(identity.transformed_values[0], 0.0));  // (1-1)/1
        QVERIFY(nearly_equal(identity.transformed_values[4], 4.0));

        const auto log_case = box_cox_transform(values, 0.0, false);
        QCOMPARE(log_case.lambda, 0.0);
        QVERIFY(nearly_equal(log_case.transformed_values[0], std::log(1.0)));
        QVERIFY(nearly_equal(log_case.transformed_values[1], std::log(2.0)));

        QVERIFY(nearly_equal(box_cox_apply(2.0, 1.0), 1.0));
        QVERIFY(nearly_equal(*box_cox_transform_limit(10.0, 0.0), std::log(10.0)));
        QVERIFY(box_cox_limits_order_ok(2.0, 8.0, 1.0));
        QVERIFY(box_cox_limits_order_ok(2.0, 8.0, 0.0));
        QVERIFY(box_cox_limits_order_ok(2.0, 8.0, -1.0));  // strictly increasing on (0,∞)

        const auto bad = box_cox_transform({-1.0, 2.0, 3.0}, 1.0, false);
        QVERIFY(!bad.diagnostics.empty());
        const auto zero = box_cox_transform({0.0, 1.0, 2.0}, 1.0, false);
        QVERIFY(!zero.diagnostics.empty());
    }

    void johnson_roundtrip_and_capability_gate()
    {
        // Mildly skewed positive sample for Johnson fit attempt.
        std::vector<double> values;
        for (int i = 1; i <= 40; ++i) {
            values.push_back(std::exp(0.05 * i) + 0.1 * (i % 3));
        }
        const auto fit = fit_johnson_transform(values);
        if (fit.found) {
            for (double x : {values.front(), values[values.size() / 2], values.back()}) {
                const auto z = johnson_transform_value(fit.parameters, x);
                QVERIFY(z.has_value());
                const auto back = johnson_inverse_value(fit.parameters, *z);
                QVERIFY(back.has_value());
                QVERIFY(nearly_equal(*back, x, 1e-5));
            }
        }

        DataTable table;
        table.columns = {"x"};
        for (double value : values) {
            table.rows.push_back({std::to_string(value)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "johnson";
        configuration.variable_columns = {0};
        configuration.specifications.lower = values.front();
        configuration.specifications.upper = values.back() * 1.2;
        auto page = AnalysisService::capability(table, configuration);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(QString::fromStdString(page.facts.capability->method),
                 QStringLiteral("johnson"));
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(page.facts.capability->research_preview, true);
        QCOMPARE(QString::fromStdString(page.facts.capability->gate_status),
                 QStringLiteral("gated_research"));
        QCOMPARE(QString::fromStdString(page.facts.capability->evidence_type),
                 QStringLiteral("formula_reference"));

        bool saw_gate_diagnostic = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "johnson_capability_gated") {
                saw_gate_diagnostic = true;
                break;
            }
        }
        QVERIFY(saw_gate_diagnostic);

        InterpretationService::enrich(page);
        bool saw_gate_limit = false;
        bool saw_pass_language = false;
        for (const auto& section : page.interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("gate_status") != std::string::npos
                    || bullet.find("研究/预览") != std::string::npos
                    || bullet.find("禁止写成过程合格") != std::string::npos) {
                    saw_gate_limit = true;
                }
                if (bullet.find("达到项目提示基准") != std::string::npos
                    && bullet.find("不是已验证的过程合格") == std::string::npos) {
                    saw_pass_language = true;
                }
            }
        }
        QVERIFY(saw_gate_limit);
        QVERIFY(!saw_pass_language);
    }

    void johnson_spec_outside_support_skips_overall_capability()
    {
        std::vector<double> values;
        for (int i = 1; i <= 40; ++i) {
            values.push_back(std::exp(0.05 * static_cast<double>(i))
                + 0.1 * static_cast<double>(i % 3));
        }
        const auto fit = fit_johnson_transform(values);
        QVERIFY2(fit.found, "fixture must fit Johnson transform for spec-limit gate test");

        SpecificationLimits specs;
        specs.lower = -1000.0;
        const auto domain_result =
            datalab::domain::statistics::ProcessCapability::calculate_johnson(
                values, specs);
        QCOMPARE(
            domain_result.evidence.not_computed_reason,
            std::string("johnson_spec_outside_support"));
        QVERIFY(!domain_result.pp.has_value());
        QVERIFY(!domain_result.ppk.has_value());
        bool saw_spec_outside = false;
        for (const auto& diagnostic : domain_result.diagnostics) {
            if (diagnostic.code == "johnson_spec_outside_support"
                && diagnostic.message
                       == "规格限落在 Johnson 变换定义域外，无法计算 Pp/Ppk。") {
                saw_spec_outside = true;
            }
        }
        QVERIFY(saw_spec_outside);

        DataTable table;
        table.columns = {"x"};
        for (double value : values) {
            table.rows.push_back({std::to_string(value)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "johnson";
        configuration.variable_columns = {0};
        configuration.specifications.lower = -1000.0;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(std::none_of(
            page.tables.cbegin(), page.tables.cend(),
            [](const datalab::domain::StatisticTable& table_out) {
                return table_out.title == "Overall Capability";
            }));
        bool saw_limitation = false;
        for (const auto& section : page.interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("已跳过 overall 能力指数表") != std::string::npos) {
                    saw_limitation = true;
                }
            }
        }
        QVERIFY(saw_limitation);
    }

    void non_normal_parametric_also_refuses_pass_fail_judgment()
    {
        DataTable table;
        table.columns = {"x"};
        for (int i = 1; i <= 30; ++i) {
            table.rows.push_back({std::to_string(0.5 * i + 1.0)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "non_normal";
        configuration.nonnormal_distribution = "weibull";
        configuration.variable_columns = {0};
        configuration.specifications.lower = 1.0;
        configuration.specifications.upper = 20.0;
        const auto page = AnalysisService::capability(table, configuration);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        // Must not silently open commercial-style pass/fail without golden.
        QVERIFY(page.facts.capability->evidence_type.find("vendor") == std::string::npos);
    }

    void missing_specs_block_capability()
    {
        DataTable table;
        table.columns = {"x"};
        table.rows = {{"1"}, {"2"}, {"3"}, {"4"}, {"5"}};
        AnalysisConfiguration configuration;
        configuration.capability_method = "normal";
        configuration.variable_columns = {0};
        const auto page = AnalysisService::capability(table, configuration);
        bool blocked = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code.find("spec") != std::string::npos
                || diagnostic.message.find("LSL") != std::string::npos
                || diagnostic.message.find("规格") != std::string::npos) {
                blocked = true;
            }
        }
        QVERIFY(blocked || !page.facts.capability.has_value()
                || !page.facts.capability->cpk.has_value());
    }

    void normal_capability_blocks_pass_fail_until_stability_verified()
    {
        DataTable table;
        table.columns = {"x"};
        // Stable-looking sequence around 10.
        for (int i = 0; i < 25; ++i) {
            table.rows.push_back({std::to_string(10.0 + ((i % 5) - 2) * 0.1)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "normal";
        configuration.variable_columns = {0};
        configuration.specifications.lower = 9.0;
        configuration.specifications.upper = 11.0;
        auto page = AnalysisService::capability(table, configuration);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(QString::fromStdString(page.facts.capability->gate_status),
                 QStringLiteral("stability_unverified"));
        QVERIFY(page.facts.capability->stability_screen_status == "clear"
                || page.facts.capability->stability_screen_status == "signals");

        bool saw_prerequisite = false;
        bool saw_block = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "capability_stability_prerequisite"
                || diagnostic.code == "capability_stability_screen_clear_not_verified"
                || diagnostic.code == "capability_stability_screen_signals") {
                saw_prerequisite = true;
            }
            if (diagnostic.code
                == "capability_pass_fail_blocked_by_stability_prerequisite") {
                saw_block = true;
            }
        }
        QVERIFY(saw_prerequisite);
        QVERIFY(saw_block);

        InterpretationService::enrich(page);
        bool saw_stability_language = false;
        bool saw_pass_language = false;
        for (const auto& section : page.interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("稳定性前置") != std::string::npos
                    || bullet.find("stability_screen") != std::string::npos) {
                    saw_stability_language = true;
                }
                if (bullet.find("达到项目提示基准") != std::string::npos
                    && bullet.find("不是已验证的过程合格") == std::string::npos) {
                    saw_pass_language = true;
                }
            }
        }
        QVERIFY(saw_stability_language);
        QVERIFY(!saw_pass_language);
    }

    void normal_capability_flags_imr_rule1_signals_without_opening_pass_fail()
    {
        DataTable table;
        table.columns = {"x"};
        for (int i = 0; i < 20; ++i) {
            table.rows.push_back({std::to_string(10.0)});
        }
        // Large spike for Rule 1.
        table.rows.push_back({"40.0"});
        AnalysisConfiguration configuration;
        configuration.capability_method = "normal";
        configuration.variable_columns = {0};
        configuration.specifications.lower = 5.0;
        configuration.specifications.upper = 15.0;
        const auto page = AnalysisService::capability(table, configuration);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(QString::fromStdString(page.facts.capability->stability_screen_status),
                 QStringLiteral("signals"));
        QVERIFY(page.facts.capability->stability_out_of_control_count >= 1);
        QCOMPARE(QString::fromStdString(page.facts.capability->gate_status),
                 QStringLiteral("stability_screen_signals"));
    }

    void normal_capability_flags_bimodality_without_opening_pass_fail()
    {
        DataTable table;
        table.columns = {"x"};
        // Interleave two modes so average MR stays large enough that Rule-1 is clear
        // while the histogram still shows separable peaks.
        for (int i = 0; i < 20; ++i) {
            table.rows.push_back({std::to_string(0.0 + (i % 3) * 0.05)});
            table.rows.push_back({std::to_string(8.0 + (i % 3) * 0.05)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "normal";
        configuration.variable_columns = {0};
        configuration.specifications.lower = -5.0;
        configuration.specifications.upper = 15.0;
        auto page = AnalysisService::capability(table, configuration);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(QString::fromStdString(page.facts.capability->bimodality_screen_status),
                 QStringLiteral("suspected"));
        QVERIFY(page.facts.capability->bimodality_peak_count >= 2);
        // With interleaved modes, Rule-1 should stay clear so gate can name bimodality.
        QCOMPARE(QString::fromStdString(page.facts.capability->stability_screen_status),
                 QStringLiteral("clear"));
        // Hartigan may also flag well-separated modes; gate prefers Hartigan over histogram.
        const QString gate = QString::fromStdString(page.facts.capability->gate_status);
        QVERIFY(gate == QStringLiteral("bimodality_suspected")
                || gate == QStringLiteral("hartigan_dip_evidence_against"));
        bool saw_bimodal = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "capability_bimodality_suspected") {
                saw_bimodal = true;
            }
        }
        QVERIFY(saw_bimodal);
        InterpretationService::enrich(page);
        bool saw_interp = false;
        for (const auto& section : page.interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("双峰初筛为 suspected") != std::string::npos
                    || bullet.find("bimodality_screen=suspected") != std::string::npos) {
                    saw_interp = true;
                }
            }
        }
        QVERIFY(saw_interp);

        const auto json = datalab::infrastructure::output_page_to_json(page);
        const auto restored = datalab::infrastructure::output_page_from_json(json);
        QVERIFY(restored.facts.capability.has_value());
        QCOMPARE(restored.facts.capability->bimodality_screen_status,
                 page.facts.capability->bimodality_screen_status);
        QCOMPARE(restored.facts.capability->bimodality_peak_count,
                 page.facts.capability->bimodality_peak_count);
        QCOMPARE(restored.facts.capability->pass_fail_judgment_allowed, false);
    }

    void gate_prefers_stability_signals_over_bimodality()
    {
        DataTable table;
        table.columns = {"x"};
        for (int i = 0; i < 20; ++i) {
            table.rows.push_back({std::to_string(0.0)});
        }
        for (int i = 0; i < 20; ++i) {
            table.rows.push_back({std::to_string(10.0)});
        }
        // Contiguous blocks → small average MR → Rule-1 OOC on the jump cluster.
        AnalysisConfiguration configuration;
        configuration.capability_method = "normal";
        configuration.variable_columns = {0};
        configuration.specifications.lower = -5.0;
        configuration.specifications.upper = 15.0;
        const auto page = AnalysisService::capability(table, configuration);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(QString::fromStdString(page.facts.capability->stability_screen_status),
                 QStringLiteral("signals"));
        QCOMPARE(QString::fromStdString(page.facts.capability->bimodality_screen_status),
                 QStringLiteral("suspected"));
        QCOMPARE(QString::fromStdString(page.facts.capability->gate_status),
                 QStringLiteral("stability_screen_signals"));
    }

    void bimodality_screen_insufficient_n_is_honest()
    {
        using datalab::domain::statistics::ProcessCapabilityResult;
        using datalab::domain::statistics::apply_capability_bimodality_screen;
        ProcessCapabilityResult result;
        apply_capability_bimodality_screen(result, {1.0, 2.0, 3.0});
        QCOMPARE(QString::fromStdString(result.bimodality_screen_status),
                 QStringLiteral("insufficient_n"));
        bool saw = false;
        for (const auto& diagnostic : result.diagnostics) {
            if (diagnostic.code == "capability_bimodality_insufficient_n") {
                saw = true;
            }
        }
        QVERIFY(saw);
    }

    void bimodality_screen_clear_is_not_unimodality_proof()
    {
        using datalab::domain::statistics::ProcessCapabilityResult;
        using datalab::domain::statistics::apply_capability_bimodality_screen;
        ProcessCapabilityResult result;
        std::vector<double> values;
        for (int i = 0; i < 40; ++i) {
            values.push_back(10.0 + 0.01 * static_cast<double>(i));
        }
        apply_capability_bimodality_screen(result, values);
        QCOMPARE(QString::fromStdString(result.bimodality_screen_status),
                 QStringLiteral("clear"));
        QCOMPARE(result.evidence.assumption_status == "verified", false);
        bool saw_clear_note = false;
        for (const auto& diagnostic : result.diagnostics) {
            if (diagnostic.code == "capability_bimodality_clear_not_verified") {
                saw_clear_note = true;
            }
        }
        QVERIFY(saw_clear_note);
    }

    void hartigan_dip_insufficient_n_is_honest()
    {
        using datalab::domain::statistics::ProcessCapabilityResult;
        using datalab::domain::statistics::apply_capability_hartigan_dip_screen;
        ProcessCapabilityResult result;
        apply_capability_hartigan_dip_screen(result, {1.0, 2.0, 3.0});
        QCOMPARE(QString::fromStdString(result.hartigan_dip_status),
                 QStringLiteral("insufficient_n"));
        bool saw = false;
        for (const auto& diagnostic : result.diagnostics) {
            if (diagnostic.code == "capability_hartigan_dip_insufficient_n") {
                saw = true;
            }
        }
        QVERIFY(saw);
    }

    void hartigan_dip_unimodal_never_opens_pass_fail()
    {
        using datalab::domain::statistics::compute_hartigan_dip;
        std::vector<double> values;
        // Near-linear ramp ≈ unimodal / Uniform-like for dip purposes.
        for (int i = 0; i < 60; ++i) {
            values.push_back(0.01 * static_cast<double>(i));
        }
        const auto dip = compute_hartigan_dip(values, 99, 7);
        QCOMPARE(QString::fromStdString(dip.status), QStringLiteral("consistent"));
        QVERIFY(dip.p_value.has_value());
        QVERIFY(*dip.p_value >= 0.05);
        QCOMPARE(QString::fromStdString(dip.evidence_type),
                 QStringLiteral("formula_reference"));

        DataTable table;
        table.columns = {"x"};
        for (double value : values) {
            table.rows.push_back({std::to_string(value)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "normal";
        configuration.variable_columns = {0};
        configuration.specifications.lower = -1.0;
        configuration.specifications.upper = 2.0;
        const auto page = AnalysisService::capability(table, configuration);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(QString::fromStdString(page.facts.capability->hartigan_dip_status),
                 QStringLiteral("consistent"));
        QVERIFY(page.facts.capability->gate_status != "open");
    }

    void hartigan_dip_bimodal_evidence_against_never_opens_pass_fail()
    {
        DataTable table;
        table.columns = {"x"};
        for (int i = 0; i < 30; ++i) {
            table.rows.push_back({std::to_string(-5.0 + 0.01 * (i % 5))});
            table.rows.push_back({std::to_string(5.0 + 0.01 * (i % 5))});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "normal";
        configuration.variable_columns = {0};
        configuration.specifications.lower = -10.0;
        configuration.specifications.upper = 10.0;
        auto page = AnalysisService::capability(table, configuration);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(QString::fromStdString(page.facts.capability->hartigan_dip_status),
                 QStringLiteral("evidence_against"));
        QVERIFY(page.facts.capability->hartigan_dip_p_value.has_value());
        QVERIFY(*page.facts.capability->hartigan_dip_p_value < 0.05);
        // Stability may also signal; never open pass/fail either way.
        QVERIFY(page.facts.capability->gate_status == "stability_screen_signals"
                || page.facts.capability->gate_status == "hartigan_dip_evidence_against"
                || page.facts.capability->gate_status == "bimodality_suspected"
                || page.facts.capability->gate_status == "stability_unverified");
        bool saw = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "capability_hartigan_dip_evidence_against") {
                saw = true;
            }
        }
        QVERIFY(saw);
        InterpretationService::enrich(page);
        bool saw_interp = false;
        for (const auto& section : page.interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("Hartigan dip") != std::string::npos
                    || bullet.find("hartigan_dip=") != std::string::npos) {
                    saw_interp = true;
                }
            }
        }
        QVERIFY(saw_interp);

        const auto json = datalab::infrastructure::output_page_to_json(page);
        const auto restored = datalab::infrastructure::output_page_from_json(json);
        QVERIFY(restored.facts.capability.has_value());
        QCOMPARE(restored.facts.capability->hartigan_dip_status,
                 page.facts.capability->hartigan_dip_status);
        QCOMPARE(restored.facts.capability->hartigan_dip_statistic,
                 page.facts.capability->hartigan_dip_statistic);
        QCOMPARE(restored.facts.capability->hartigan_dip_p_value.has_value(),
                 page.facts.capability->hartigan_dip_p_value.has_value());
        if (page.facts.capability->hartigan_dip_p_value.has_value()) {
            QCOMPARE(*restored.facts.capability->hartigan_dip_p_value,
                     *page.facts.capability->hartigan_dip_p_value);
        }
        QCOMPARE(restored.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(QString::fromStdString(restored.facts.capability->evidence_type),
                 QStringLiteral("formula_reference"));
    }

    void gate_prefers_stability_signals_over_hartigan()
    {
        DataTable table;
        table.columns = {"x"};
        for (int i = 0; i < 20; ++i) {
            table.rows.push_back({std::to_string(0.0)});
        }
        for (int i = 0; i < 20; ++i) {
            table.rows.push_back({std::to_string(10.0)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "normal";
        configuration.variable_columns = {0};
        configuration.specifications.lower = -5.0;
        configuration.specifications.upper = 15.0;
        const auto page = AnalysisService::capability(table, configuration);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(QString::fromStdString(page.facts.capability->stability_screen_status),
                 QStringLiteral("signals"));
        QCOMPARE(QString::fromStdString(page.facts.capability->gate_status),
                 QStringLiteral("stability_screen_signals"));
    }

    void gaussian_mixture_2_prefers_two_well_separated_components()
    {
        using datalab::domain::statistics::fit_gaussian_mixture_2;
        std::vector<double> values;
        values.reserve(80);
        for (int i = 0; i < 40; ++i) {
            values.push_back(-2.0 + 0.05 * (i % 5));
        }
        for (int i = 0; i < 40; ++i) {
            values.push_back(4.0 + 0.05 * (i % 5));
        }
        const auto mix = fit_gaussian_mixture_2(values);
        QVERIFY(mix.ok);
        QCOMPARE(mix.evidence_type, std::string("formula_reference"));
        QCOMPARE(mix.algorithm_id, std::string("gaussian_mixture_2_em"));
        QCOMPARE(mix.status, std::string("preferred_2comp"));
        QVERIFY(mix.delta_bic >= 2.0);
        QVERIFY(mix.weight1 > 0.1 && mix.weight1 < 0.9);
    }

    void gaussian_mixture_search_prefers_three_well_separated_components()
    {
        using datalab::domain::statistics::fit_gaussian_mixture_search;
        std::vector<double> values;
        values.reserve(90);
        for (int i = 0; i < 30; ++i) {
            values.push_back(-6.0 + 0.04 * (i % 4));
        }
        for (int i = 0; i < 30; ++i) {
            values.push_back(0.0 + 0.04 * (i % 4));
        }
        for (int i = 0; i < 30; ++i) {
            values.push_back(6.0 + 0.04 * (i % 4));
        }
        const auto mix = fit_gaussian_mixture_search(values, 4);
        QVERIFY(mix.ok);
        QCOMPARE(mix.evidence_type, std::string("formula_reference"));
        QCOMPARE(mix.algorithm_id, std::string("gaussian_mixture_k_bic"));
        QVERIFY(mix.k_selected >= 2);
        QVERIFY(mix.status == "preferred_2comp" || mix.status == "preferred_kcomp");
        QVERIFY(mix.delta_bic >= 2.0);
        QCOMPARE(static_cast<int>(mix.components.size()), mix.k_selected);
    }

    void capability_mixture_gate_sets_preferred_and_blocks_pass_fail()
    {
        DataTable table;
        table.columns = {"x"};
        // Interleave two tight clusters so I-MR Rule-1 is less likely than
        // sequential blocks, while mixture still prefers 2 components.
        for (int i = 0; i < 40; ++i) {
            table.rows.push_back({std::to_string(-2.0 + 0.02 * (i % 3))});
            table.rows.push_back({std::to_string(5.0 + 0.02 * (i % 3))});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "normal";
        configuration.variable_columns = {0};
        configuration.specifications.lower = -10.0;
        configuration.specifications.upper = 10.0;
        const auto page = AnalysisService::capability(table, configuration);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(QString::fromStdString(page.facts.capability->mixture_evidence_type),
                 QStringLiteral("formula_reference"));
        QCOMPARE(QString::fromStdString(page.facts.capability->mixture_algorithm_id),
                 QStringLiteral("gaussian_mixture_k_bic"));
        QCOMPARE(QString::fromStdString(page.facts.capability->mixture_status),
                 QStringLiteral("preferred_2comp"));
        QCOMPARE(page.facts.capability->mixture_k_selected, 2);
        QVERIFY(page.facts.capability->mixture_components.size() >= 2);
        if (page.facts.capability->stability_screen_status != "signals") {
            QCOMPARE(QString::fromStdString(page.facts.capability->gate_status),
                     QStringLiteral("mixture_preferred_2comp"));
        }
        bool saw_mix = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "capability_mixture_preferred_2comp"
                || diagnostic.code == "capability_mixture_preferred_kcomp") {
                saw_mix = true;
                QVERIFY(diagnostic.message.find("vendor_oracle") != std::string::npos);
            }
        }
        QVERIFY(saw_mix);
        InterpretationService::enrich(page);
        bool saw_interp = false;
        for (const auto& section : page.interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("preferred_2comp") != std::string::npos
                    || bullet.find("preferred_kcomp") != std::string::npos
                    || bullet.find("mixture=") != std::string::npos) {
                    saw_interp = true;
                }
            }
        }
        QVERIFY(saw_interp);

        const auto json = datalab::infrastructure::output_page_to_json(page);
        auto restored = datalab::infrastructure::output_page_from_json(json);
        QVERIFY(restored.facts.capability.has_value());
        QCOMPARE(restored.facts.capability->mixture_status,
                 page.facts.capability->mixture_status);
        QCOMPARE(restored.facts.capability->mixture_delta_bic,
                 page.facts.capability->mixture_delta_bic);
        QCOMPARE(restored.facts.capability->mixture_k_selected,
                 page.facts.capability->mixture_k_selected);
        QCOMPARE(restored.facts.capability->mixture_components.size(),
                 page.facts.capability->mixture_components.size());

        auto facts = json.value(QStringLiteral("facts")).toObject();
        auto capability = facts.value(QStringLiteral("capability")).toObject();
        capability.insert(QStringLiteral("mixture_evidence_type"),
                          QStringLiteral("vendor_oracle"));
        capability.insert(QStringLiteral("mixture_algorithm_id"),
                          QStringLiteral("vendor_mixture"));
        facts.insert(QStringLiteral("capability"), capability);
        auto tampered = json;
        tampered.insert(QStringLiteral("facts"), facts);
        const auto clamped = datalab::infrastructure::output_page_from_json(tampered);
        QVERIFY(clamped.facts.capability.has_value());
        QCOMPARE(clamped.facts.capability->mixture_evidence_type,
                 std::string("formula_reference"));
        QCOMPARE(clamped.facts.capability->mixture_algorithm_id,
                 std::string("gaussian_mixture_k_bic"));
        QCOMPARE(clamped.facts.capability->pass_fail_judgment_allowed, false);
    }
};

QTEST_MAIN(NonNormalCapabilityPhase6Test)
#include "nonnormal_capability_phase6_test.moc"
