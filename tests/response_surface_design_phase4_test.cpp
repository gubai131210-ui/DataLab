#include "application/analysis_service.h"
#include "domain/quality_types.h"
#include "domain/statistics/response_surface_design.h"
#include "domain/statistics/rsm_analysis.h"
#include "infrastructure/output_serialization.h"

#include <QtTest/QtTest>

#include <cmath>
#include <limits>
#include <set>
#include <string>
#include <vector>

using datalab::application::AnalysisService;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::statistics::CcdVariant;
using datalab::domain::statistics::ResponseSurfaceDesignKind;
using datalab::domain::statistics::ResponseSurfaceDesignOptions;
using datalab::domain::statistics::ResponseSurfaceFactor;
using datalab::domain::statistics::actual_to_coded;
using datalab::domain::statistics::coded_to_actual;
using datalab::domain::statistics::generate_bbd;
using datalab::domain::statistics::generate_ccd;

namespace {

ResponseSurfaceFactor make_factor(
    const std::string& id,
    double low,
    double high,
    double center)
{
    ResponseSurfaceFactor factor;
    factor.id = id;
    factor.name = id;
    factor.unit = "1";
    factor.type = "continuous";
    factor.low = low;
    factor.high = high;
    factor.center = center;
    return factor;
}

bool nearly_equal(double a, double b, double tol = 1e-9)
{
    return std::fabs(a - b) <= tol;
}

}  // namespace

class ResponseSurfaceDesignPhase4Test final : public QObject {
    Q_OBJECT

private slots:
    void rejects_invalid_bounds_and_duplicate_ids()
    {
        ResponseSurfaceDesignOptions options;
        options.design_kind = ResponseSurfaceDesignKind::ccd;
        options.factors = {
            make_factor("A", 10.0, 10.0, 10.0),
            make_factor("B", 0.0, 1.0, 0.5)};
        auto bad_bounds = generate_ccd(options);
        QVERIFY(!bad_bounds.ok);
        QVERIFY(!bad_bounds.diagnostics.empty());

        options.factors = {
            make_factor("A", 0.0, 1.0, 0.5),
            make_factor("A", 2.0, 3.0, 2.5)};
        auto dup = generate_ccd(options);
        QVERIFY(!dup.ok);
    }

    void rejects_categorical_and_nonfinite()
    {
        ResponseSurfaceDesignOptions options;
        options.ccd_variant = CcdVariant::ccf;
        options.factors = {
            make_factor("A", 0.0, 1.0, 0.5),
            make_factor("B", 0.0, 1.0, 0.5)};
        options.factors[0].type = "categorical";
        QVERIFY(!generate_ccd(options).ok);

        options.factors[0].type = "continuous";
        options.factors[0].low = std::numeric_limits<double>::quiet_NaN();
        QVERIFY(!generate_ccd(options).ok);

        options.factors[0].low = 0.0;
        options.factors[0].high = std::numeric_limits<double>::infinity();
        QVERIFY(!generate_ccd(options).ok);
    }

    void ccd_cci_differs_from_ccf_geometry()
    {
        ResponseSurfaceDesignOptions base;
        base.center_point_count = 1;
        base.randomize = false;
        base.factors = {
            make_factor("A", -1.0, 1.0, 0.0),
            make_factor("B", -1.0, 1.0, 0.0)};

        auto ccf = base;
        ccf.ccd_variant = CcdVariant::ccf;
        auto cci = base;
        cci.ccd_variant = CcdVariant::cci;

        const auto d_ccf = generate_ccd(ccf);
        const auto d_cci = generate_ccd(cci);
        QVERIFY(d_ccf.ok && d_cci.ok);
        QVERIFY(d_cci.alpha > 1.0);

        bool found_scaled_cube = false;
        for (const auto& run : d_cci.runs) {
            if (run.point_type != "cube") {
                continue;
            }
            QVERIFY(std::fabs(run.coded_levels[0]) < 1.0 - 1e-9);
            QVERIFY(std::fabs(run.coded_levels[1]) < 1.0 - 1e-9);
            found_scaled_cube = true;
        }
        QVERIFY(found_scaled_cube);

        for (const auto& run : d_ccf.runs) {
            if (run.point_type != "cube") {
                continue;
            }
            QVERIFY(nearly_equal(std::fabs(run.coded_levels[0]), 1.0));
            QVERIFY(nearly_equal(std::fabs(run.coded_levels[1]), 1.0));
        }
    }

    void ccd_ccf_k2_matches_nist_point_counts_and_roundtrip()
    {
        // samples/phase0_baselines/doe_ccd_k2_factors.json — formula_reference
        ResponseSurfaceDesignOptions options;
        options.design_kind = ResponseSurfaceDesignKind::ccd;
        options.ccd_variant = CcdVariant::ccf;
        options.center_point_count = 1;
        options.randomize = true;
        options.random_seed = 42;
        options.factors = {
            make_factor("A", 60.0, 80.0, 70.0),
            make_factor("B", 100.0, 140.0, 120.0)};
        options.factors[0].name = "Temperature";
        options.factors[0].unit = "C";
        options.factors[1].name = "Pressure";
        options.factors[1].unit = "kPa";

        const auto design = generate_ccd(options);
        QVERIFY(design.ok);
        QCOMPARE(static_cast<int>(design.cube_count), 4);
        QCOMPARE(static_cast<int>(design.star_count), 4);
        QCOMPARE(static_cast<int>(design.center_count), 1);
        QCOMPARE(static_cast<int>(design.run_count), 9);
        QCOMPARE(design.alpha, 1.0);
        QCOMPARE(QString::fromStdString(design.ccd_variant_id), QStringLiteral("ccf"));

        std::size_t cube = 0;
        std::size_t star = 0;
        std::size_t center = 0;
        for (const auto& run : design.runs) {
            QVERIFY(!run.run_id.empty());
            QCOMPARE(static_cast<int>(run.coded_levels.size()), 2);
            QCOMPARE(static_cast<int>(run.actual_levels.size()), 2);
            for (std::size_t i = 0; i < 2; ++i) {
                const double roundtrip =
                    actual_to_coded(run.actual_levels[i], design.factors[i]);
                QVERIFY(nearly_equal(roundtrip, run.coded_levels[i]));
                const double back =
                    coded_to_actual(run.coded_levels[i], design.factors[i]);
                QVERIFY(nearly_equal(back, run.actual_levels[i]));
            }
            if (run.point_type == "cube") {
                ++cube;
            } else if (run.point_type == "star") {
                ++star;
            } else if (run.point_type == "center") {
                ++center;
            }
        }
        QCOMPARE(static_cast<int>(cube), 4);
        QCOMPARE(static_cast<int>(star), 4);
        QCOMPARE(static_cast<int>(center), 1);
    }

    void same_seed_same_order_different_seed_same_point_set()
    {
        ResponseSurfaceDesignOptions base;
        base.ccd_variant = CcdVariant::ccf;
        base.center_point_count = 1;
        base.randomize = true;
        base.factors = {
            make_factor("A", -1.0, 1.0, 0.0),
            make_factor("B", -1.0, 1.0, 0.0)};

        auto a = base;
        a.random_seed = 11;
        auto b = base;
        b.random_seed = 11;
        auto c = base;
        c.random_seed = 99;

        const auto da = generate_ccd(a);
        const auto db = generate_ccd(b);
        const auto dc = generate_ccd(c);
        QVERIFY(da.ok && db.ok && dc.ok);

        for (std::size_t i = 0; i < da.runs.size(); ++i) {
            QCOMPARE(da.runs[i].run_order, db.runs[i].run_order);
            QCOMPARE(da.runs[i].standard_order, db.runs[i].standard_order);
            QCOMPARE(QString::fromStdString(da.runs[i].run_id),
                     QString::fromStdString(db.runs[i].run_id));
        }

        std::set<std::string> set_a;
        std::set<std::string> set_c;
        bool order_differs = false;
        for (std::size_t i = 0; i < da.runs.size(); ++i) {
            std::string key;
            for (double v : da.runs[i].coded_levels) {
                key += std::to_string(v) + ",";
            }
            key += da.runs[i].point_type;
            set_a.insert(key);
            std::string key_c;
            for (double v : dc.runs[i].coded_levels) {
                key_c += std::to_string(v) + ",";
            }
            key_c += dc.runs[i].point_type;
            set_c.insert(key_c);
            if (da.runs[i].standard_order != dc.runs[i].standard_order) {
                order_differs = true;
            }
        }
        QCOMPARE(set_a, set_c);
        QVERIFY(order_differs);
    }

    void ccd_ccc_blocks_beyond_range_unless_allowed()
    {
        ResponseSurfaceDesignOptions options;
        options.ccd_variant = CcdVariant::ccc;
        options.allow_beyond_range = false;
        options.center_point_count = 1;
        options.randomize = false;
        options.factors = {
            make_factor("A", 0.0, 1.0, 0.5),
            make_factor("B", 0.0, 1.0, 0.5)};
        const auto blocked = generate_ccd(options);
        QVERIFY(!blocked.ok);

        options.allow_beyond_range = true;
        const auto allowed = generate_ccd(options);
        QVERIFY(allowed.ok);
        QVERIFY(allowed.beyond_range_detected);
        QVERIFY(allowed.alpha > 1.0);
    }

    void bbd_k3_no_corners_and_rejects_k2()
    {
        // samples/phase0_baselines/doe_bbd_k3_factors.json
        ResponseSurfaceDesignOptions bad;
        bad.design_kind = ResponseSurfaceDesignKind::bbd;
        bad.factors = {
            make_factor("X1", -1.0, 1.0, 0.0),
            make_factor("X2", -1.0, 1.0, 0.0)};
        QVERIFY(!generate_bbd(bad).ok);

        ResponseSurfaceDesignOptions options;
        options.design_kind = ResponseSurfaceDesignKind::bbd;
        options.center_point_count = 1;
        options.randomize = true;
        options.random_seed = 7;
        options.factors = {
            make_factor("X1", -1.0, 1.0, 0.0),
            make_factor("X2", -1.0, 1.0, 0.0),
            make_factor("X3", -1.0, 1.0, 0.0)};
        const auto design = generate_bbd(options);
        QVERIFY(design.ok);
        QCOMPARE(static_cast<int>(design.edge_count), 12);
        QCOMPARE(static_cast<int>(design.center_count), 1);
        QCOMPARE(static_cast<int>(design.run_count), 13);

        for (const auto& run : design.runs) {
            if (run.point_type != "edge") {
                continue;
            }
            int extremes = 0;
            for (double coded : run.coded_levels) {
                if (nearly_equal(std::fabs(coded), 1.0)) {
                    ++extremes;
                }
            }
            QVERIFY(extremes == 2);
            QVERIFY(!(nearly_equal(std::fabs(run.coded_levels[0]), 1.0)
                      && nearly_equal(std::fabs(run.coded_levels[1]), 1.0)
                      && nearly_equal(std::fabs(run.coded_levels[2]), 1.0)));
        }
    }

    void application_page_and_facts_roundtrip()
    {
        AnalysisConfiguration configuration;
        configuration.chart_type = "doe_ccd";
        configuration.analysis_name = "中心复合设计";
        auto& cfg = configuration.response_surface_design;
        cfg.design_kind = "ccd";
        cfg.ccd_variant = "ccf";
        cfg.factor_ids = {"A", "B"};
        cfg.factor_names = {"Temperature", "Pressure"};
        cfg.factor_units = {"C", "kPa"};
        cfg.low_levels = {60.0, 100.0};
        cfg.high_levels = {80.0, 140.0};
        cfg.centers = {70.0, 120.0};
        cfg.center_point_count = 1;
        cfg.randomize = true;
        cfg.random_seed = 42;

        DataTable table;
        const auto page = AnalysisService::doe_response_surface_design(table, configuration);
        QVERIFY(page.facts.design_generation.has_value());
        QCOMPARE(QString::fromStdString(page.facts.design_generation->design_kind),
                 QStringLiteral("ccd"));
        QCOMPARE(static_cast<int>(page.facts.design_generation->run_count), 9);
        QCOMPARE(QString::fromStdString(page.facts.design_generation->evidence_type),
                 QStringLiteral("formula_reference"));
        QVERIFY(!page.facts.design_generation->design_source_id.empty());
        QVERIFY(page.tables.size() >= 3);
        QVERIFY(page.worksheet_export.has_value());
        QCOMPARE(static_cast<int>(page.worksheet_export->rows.size()), 9);
        QVERIFY(page.worksheet_export->columns.size() >= 8);  // meta + 2 factors + Response
        QCOMPARE(QString::fromStdString(page.worksheet_export->columns.back()),
                 QStringLiteral("Response"));
        // Response column is empty placeholders for experiment entry.
        for (const auto& row : page.worksheet_export->rows) {
            QVERIFY(!row.empty());
            QCOMPARE(QString::fromStdString(row.back()), QString());
        }

        const QJsonObject serialized =
            datalab::infrastructure::output_page_to_json(page);
        const auto restored =
            datalab::infrastructure::output_page_from_json(serialized);
        QVERIFY(restored.facts.design_generation.has_value());
        QCOMPARE(restored.facts.design_generation->run_count,
                 page.facts.design_generation->run_count);
        QCOMPARE(restored.facts.design_generation->cube_count,
                 page.facts.design_generation->cube_count);
    }

    void design_to_rsm_closed_loop_with_design_bounds()
    {
        ResponseSurfaceDesignOptions options;
        options.ccd_variant = CcdVariant::ccf;
        options.center_point_count = 1;
        options.randomize = false;
        options.factors = {
            make_factor("A", -1.0, 1.0, 0.0),
            make_factor("B", -1.0, 1.0, 0.0)};
        const auto design = generate_ccd(options);
        QVERIFY(design.ok);

        DataTable table;
        table.name = "ccd_rsm_loop";
        table.columns = {"Y", "A", "B"};
        for (const auto& run : design.runs) {
            const double x1 = run.actual_levels[0];
            const double x2 = run.actual_levels[1];
            const double y = 1.0 + 2.0 * x1 - x2 + 0.5 * x1 * x2 + x1 * x1;
            table.rows.push_back({
                std::to_string(y), std::to_string(x1), std::to_string(x2)});
        }

        AnalysisConfiguration configuration;
        configuration.chart_type = "rsm_response";
        configuration.variable_columns = {0, 1, 2};
        configuration.response_surface_design.design_source_id = "ccd-ccf-loop";
        configuration.response_surface_design.design_kind = "ccd";
        configuration.response_surface_design.low_levels = {-1.0, -1.0};
        configuration.response_surface_design.high_levels = {1.0, 1.0};
        configuration.response_surface_design.centers = {0.0, 0.0};

        const auto page = AnalysisService::rsm_response(table, configuration);
        QVERIFY(page.facts.rsm.has_value());
        QCOMPARE(QString::fromStdString(page.facts.rsm->design_source_id),
                 QStringLiteral("ccd-ccf-loop"));
        QCOMPARE(QString::fromStdString(page.facts.rsm->coding_mode),
                 QStringLiteral("design_bounds"));
        QVERIFY(page.facts.rsm->center_point_count >= 1);
        QVERIFY(page.facts.rsm->surface_is_static);
        QCOMPARE(QString::fromStdString(page.facts.rsm->evidence_type),
                 QStringLiteral("formula_reference"));
        QVERIFY(page.facts.rsm->contour_plot_available);

        bool saw_static_surface = false;
        for (const auto& plot : page.plots) {
            if (plot.kind == datalab::domain::PlotKind::surface) {
                QVERIFY(plot.title.find("非可旋转") != std::string::npos);
                saw_static_surface = true;
            }
        }
        QVERIFY(saw_static_surface);
    }

    void ccd_and_bbd_match_pinned_reference_implementation_golden()
    {
        // Frozen goldens from scripts/doe_rsm_reference_points.py (source_evidence_type=
        // reference_implementation). Not vendor_oracle.
        ResponseSurfaceDesignOptions ccd_options;
        ccd_options.design_kind = ResponseSurfaceDesignKind::ccd;
        ccd_options.ccd_variant = CcdVariant::ccf;
        ccd_options.center_point_count = 1;
        ccd_options.randomize = false;
        ccd_options.factors = {
            make_factor("A", -1.0, 1.0, 0.0),
            make_factor("B", -1.0, 1.0, 0.0)};
        const auto ccd = generate_ccd(ccd_options);
        QVERIFY(ccd.ok);
        QCOMPARE(static_cast<int>(ccd.runs.size()), 9);

        const std::vector<std::pair<std::string, std::vector<double>>> ccd_golden = {
            {"cube", {-1.0, -1.0}},
            {"cube", {1.0, -1.0}},
            {"cube", {-1.0, 1.0}},
            {"cube", {1.0, 1.0}},
            {"star", {-1.0, 0.0}},
            {"star", {1.0, 0.0}},
            {"star", {0.0, -1.0}},
            {"star", {0.0, 1.0}},
            {"center", {0.0, 0.0}},
        };
        for (std::size_t index = 0; index < ccd_golden.size(); ++index) {
            QCOMPARE(ccd.runs[index].standard_order, index + 1);
            QCOMPARE(ccd.runs[index].run_order, index + 1);
            QCOMPARE(QString::fromStdString(ccd.runs[index].point_type),
                     QString::fromStdString(ccd_golden[index].first));
            QCOMPARE(static_cast<int>(ccd.runs[index].coded_levels.size()), 2);
            QVERIFY(nearly_equal(ccd.runs[index].coded_levels[0],
                                 ccd_golden[index].second[0]));
            QVERIFY(nearly_equal(ccd.runs[index].coded_levels[1],
                                 ccd_golden[index].second[1]));
        }

        ResponseSurfaceDesignOptions bbd_options;
        bbd_options.design_kind = ResponseSurfaceDesignKind::bbd;
        bbd_options.center_point_count = 1;
        bbd_options.randomize = false;
        bbd_options.factors = {
            make_factor("X1", -1.0, 1.0, 0.0),
            make_factor("X2", -1.0, 1.0, 0.0),
            make_factor("X3", -1.0, 1.0, 0.0)};
        const auto bbd = generate_bbd(bbd_options);
        QVERIFY(bbd.ok);
        QCOMPARE(static_cast<int>(bbd.runs.size()), 13);

        const std::vector<std::pair<std::string, std::vector<double>>> bbd_golden = {
            {"edge", {-1.0, -1.0, 0.0}},
            {"edge", {-1.0, 1.0, 0.0}},
            {"edge", {1.0, -1.0, 0.0}},
            {"edge", {1.0, 1.0, 0.0}},
            {"edge", {-1.0, 0.0, -1.0}},
            {"edge", {-1.0, 0.0, 1.0}},
            {"edge", {1.0, 0.0, -1.0}},
            {"edge", {1.0, 0.0, 1.0}},
            {"edge", {0.0, -1.0, -1.0}},
            {"edge", {0.0, -1.0, 1.0}},
            {"edge", {0.0, 1.0, -1.0}},
            {"edge", {0.0, 1.0, 1.0}},
            {"center", {0.0, 0.0, 0.0}},
        };
        for (std::size_t index = 0; index < bbd_golden.size(); ++index) {
            QCOMPARE(bbd.runs[index].standard_order, index + 1);
            QCOMPARE(QString::fromStdString(bbd.runs[index].point_type),
                     QString::fromStdString(bbd_golden[index].first));
            QCOMPARE(static_cast<int>(bbd.runs[index].coded_levels.size()), 3);
            for (std::size_t f = 0; f < 3; ++f) {
                QVERIFY(nearly_equal(bbd.runs[index].coded_levels[f],
                                     bbd_golden[index].second[f]));
            }
            // No full-factorial corner among edge points.
            if (bbd.runs[index].point_type == "edge") {
                const int extremes = static_cast<int>(
                    (std::fabs(bbd.runs[index].coded_levels[0]) > 0.5)
                    + (std::fabs(bbd.runs[index].coded_levels[1]) > 0.5)
                    + (std::fabs(bbd.runs[index].coded_levels[2]) > 0.5));
                QCOMPARE(extremes, 2);
            }
        }
    }

    void rsm_lack_of_fit_uses_replicated_coded_points_not_residual_ms()
    {
        using datalab::domain::statistics::fit_rsm_analysis;

        // Unique factorial + axial-like points plus three center replicates with
        // pure-error noise. Evidence type remains formula_reference.
        const std::vector<std::vector<double>> coded = {
            {-1.0, -1.0}, {1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0},
            {-1.0, 0.0}, {1.0, 0.0}, {0.0, -1.0}, {0.0, 1.0},
            {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}};
        std::vector<double> response;
        response.reserve(coded.size());
        for (std::size_t i = 0; i < coded.size(); ++i) {
            const double x1 = coded[i][0];
            const double x2 = coded[i][1];
            double y = 10.0 + 3.0 * x1 - 2.0 * x2 + 1.5 * x1 * x2
                + 0.8 * x1 * x1 - 0.4 * x2 * x2;
            if (std::fabs(x1) < 1.0e-9 && std::fabs(x2) < 1.0e-9) {
                y += (i == 8) ? 0.4 : ((i == 9) ? -0.2 : -0.1);
            }
            response.push_back(y);
        }

        const auto fit = fit_rsm_analysis(response, coded, {"A", "B"}, "Y");
        QVERIFY(fit.pure_error_anova_row.has_value());
        QVERIFY(fit.lack_of_fit_anova_row.has_value());
        QCOMPARE(static_cast<int>(fit.pure_error_anova_row->degrees_of_freedom), 2);
        QVERIFY(fit.pure_error_anova_row->sum_of_squares > 0.0);
        QVERIFY(fit.lack_of_fit_anova_row->degrees_of_freedom > 0);
        QVERIFY(fit.lack_of_fit_anova_row->p_value.has_value());

        bool saw_pe = false;
        bool saw_lof = false;
        bool saw_honesty = false;
        for (const auto& diagnostic : fit.diagnostics) {
            if (diagnostic.code == "rsm_replicated_coded_points") {
                saw_pe = true;
            }
            if (diagnostic.code == "rsm_lof_formula_reference") {
                saw_honesty = true;
            }
            if (diagnostic.code == "rsm_insufficient_pure_error") {
                QFAIL("unexpected insufficient pure-error with replicates");
            }
            if (diagnostic.message.find("失拟") != std::string::npos
                || diagnostic.code == "rsm_lof_formula_reference") {
                saw_lof = true;
            }
        }
        QVERIFY(saw_pe);
        QVERIFY(saw_lof);
        QVERIFY(saw_honesty);

        DataTable table;
        table.name = "rsm_lof";
        table.columns = {"Y", "A", "B"};
        for (std::size_t i = 0; i < coded.size(); ++i) {
            table.rows.push_back({
                std::to_string(response[i]),
                std::to_string(coded[i][0]),
                std::to_string(coded[i][1])});
        }
        AnalysisConfiguration configuration;
        configuration.chart_type = "rsm_response";
        configuration.variable_columns = {0, 1, 2};
        const auto page = AnalysisService::rsm_response(table, configuration);
        QVERIFY(page.facts.rsm.has_value());
        QVERIFY(page.facts.rsm->pure_error_available);
        QVERIFY(page.facts.rsm->lack_of_fit_available);
        QCOMPARE(static_cast<int>(page.facts.rsm->pure_error_df), 2);
        QVERIFY(page.facts.rsm->lack_of_fit_f.has_value());
        QVERIFY(page.facts.rsm->lack_of_fit_p.has_value());
        QCOMPARE(QString::fromStdString(page.facts.rsm->evidence_type),
                 QStringLiteral("formula_reference"));

        bool anova_has_lof = false;
        bool anova_has_pe = false;
        for (const auto& table_item : page.tables) {
            if (table_item.title != "方差分析") {
                continue;
            }
            for (const auto& row : table_item.rows) {
                if (!row.empty() && row.front() == "失拟") {
                    anova_has_lof = true;
                }
                if (!row.empty() && row.front() == "纯误差") {
                    anova_has_pe = true;
                }
            }
        }
        QVERIFY(anova_has_lof);
        QVERIFY(anova_has_pe);

        const QJsonObject serialized =
            datalab::infrastructure::output_page_to_json(page);
        const auto restored =
            datalab::infrastructure::output_page_from_json(serialized);
        QVERIFY(restored.facts.rsm.has_value());
        QVERIFY(restored.facts.rsm->lack_of_fit_available);
        QCOMPARE(static_cast<int>(restored.facts.rsm->pure_error_df), 2);
    }

    void rsm_without_replicates_refuses_fake_pure_error()
    {
        using datalab::domain::statistics::fit_rsm_analysis;
        const std::vector<std::vector<double>> coded = {
            {-1.0, -1.0}, {1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0},
            {-1.0, 0.0}, {1.0, 0.0}, {0.0, -1.0}, {0.0, 1.0}, {0.0, 0.0}};
        std::vector<double> response;
        for (const auto& row : coded) {
            response.push_back(1.0 + row[0] + row[1] + row[0] * row[1]
                               + row[0] * row[0]);
        }
        const auto fit = fit_rsm_analysis(response, coded, {"A", "B"}, "Y");
        QVERIFY(!fit.pure_error_anova_row.has_value());
        QVERIFY(!fit.lack_of_fit_anova_row.has_value());
        bool saw_warning = false;
        for (const auto& diagnostic : fit.diagnostics) {
            if (diagnostic.code == "rsm_insufficient_pure_error") {
                saw_warning = true;
            }
        }
        QVERIFY(saw_warning);
    }
};

QTEST_MAIN(ResponseSurfaceDesignPhase4Test)
#include "response_surface_design_phase4_test.moc"
