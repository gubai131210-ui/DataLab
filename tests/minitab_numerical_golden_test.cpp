#include "application/analysis_service.h"
#include "domain/statistics/arima.h"
#include "domain/statistics/control_charts.h"
#include "domain/statistics/gage_rr.h"
#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/normality_test.h"
#include "domain/statistics/process_capability.h"
#include "domain/statistics/regression.h"
#include "fixtures/minitab/golden_loader.h"
#include "infrastructure/csv_importer.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace {

datalab::domain::DataTable load_fixture_csv(const char* file_name, QString* error)
{
    const QString path = QStringLiteral(DATALAB_SOURCE_DIR
        "/tests/fixtures/minitab/converted/") + QString::fromUtf8(file_name);
    const auto table = datalab::infrastructure::CsvImporter::import_file(path, error);
    if (!table.has_value()) {
        return {};
    }
    return *table;
}

std::string expected_path(const char* file_name)
{
    return std::string(DATALAB_SOURCE_DIR) + "/tests/fixtures/minitab/expected/" + file_name;
}

// G-Trust lock-table: missing *_ref_golden.tsv must FAIL (never QSKIP).
bool load_required_ref_golden(
    const char* file_name,
    datalab::tests::minitab::GoldenDocument* out)
{
    std::string error;
    const auto golden = datalab::tests::minitab::load_golden_tsv(expected_path(file_name), &error);
    if (!golden.has_value()) {
        *out = {};
        QTest::qFail(
            qPrintable(QStringLiteral(
                "G-Trust ref-golden missing or unreadable (must FAIL, not QSKIP): %1 — %2")
                           .arg(QString::fromUtf8(file_name),
                                QString::fromStdString(error))),
            __FILE__,
            __LINE__);
        return false;
    }
    *out = *golden;
    return true;
}

datalab::tests::minitab::GoldenTolerance control_limit_tolerance()
{
    datalab::tests::minitab::GoldenTolerance tolerance;
    tolerance.abs_tol = 1.0e-6;
    tolerance.rel_tol = 1.0e-4;
    return tolerance;
}

datalab::tests::minitab::GoldenTolerance capability_index_tolerance()
{
    datalab::tests::minitab::GoldenTolerance tolerance;
    tolerance.abs_tol = 1.0e-3;
    tolerance.rel_tol = 0.0;
    return tolerance;
}

datalab::tests::minitab::GoldenTolerance sigma_tolerance()
{
    datalab::tests::minitab::GoldenTolerance tolerance;
    tolerance.abs_tol = 1.0e-6;
    tolerance.rel_tol = 1.0e-4;
    return tolerance;
}

datalab::tests::minitab::GoldenTolerance gage_percent_tolerance()
{
    datalab::tests::minitab::GoldenTolerance tolerance;
    tolerance.abs_tol = 0.05;
    tolerance.rel_tol = 0.0;
    return tolerance;
}

datalab::tests::minitab::GoldenTolerance inferential_tolerance()
{
    datalab::tests::minitab::GoldenTolerance tolerance;
    tolerance.abs_tol = 1.0e-6;
    tolerance.rel_tol = 1.0e-4;
    return tolerance;
}

datalab::tests::minitab::GoldenTolerance p_value_tolerance()
{
    datalab::tests::minitab::GoldenTolerance tolerance;
    tolerance.abs_tol = 1.0e-4;
    tolerance.rel_tol = 0.0;
    return tolerance;
}

std::vector<std::vector<double>> fixed_size_subgroups(
    const std::vector<double>& values,
    std::size_t subgroup_size)
{
    std::vector<std::vector<double>> subgroups;
    if (subgroup_size == 0 || values.size() % subgroup_size != 0) {
        return subgroups;
    }
    for (std::size_t index = 0; index < values.size(); index += subgroup_size) {
        subgroups.emplace_back(
            values.begin() + static_cast<std::ptrdiff_t>(index),
            values.begin() + static_cast<std::ptrdiff_t>(index + subgroup_size));
    }
    return subgroups;
}

bool expect_metric(
    const std::vector<std::vector<std::string>>& rows,
    const std::string& key,
    double actual,
    const datalab::tests::minitab::GoldenTolerance& tolerance,
    const char* label,
    QString* failure)
{
    const auto expected = datalab::tests::minitab::table_cell_as_double(
        rows, "Key", key, "Value");
    if (!expected.has_value()) {
        *failure = QStringLiteral("missing golden metric %1").arg(QString::fromUtf8(label));
        return false;
    }
    std::string message;
    if (!datalab::tests::minitab::compare_double(actual, *expected, tolerance, &message)) {
        *failure = QStringLiteral("%1 mismatch: actual=%2 expected=%3 (%4)")
                       .arg(QString::fromUtf8(label))
                       .arg(actual, 0, 'g', 17)
                       .arg(*expected, 0, 'g', 17)
                       .arg(QString::fromStdString(message));
        return false;
    }
    return true;
}

std::optional<std::size_t> column_index(
    const datalab::domain::DataTable& table,
    const std::string& name)
{
    for (std::size_t index = 0; index < table.columns.size(); ++index) {
        if (table.columns[index] == name) {
            return index;
        }
    }
    return std::nullopt;
}

std::vector<double> column_as_doubles(
    const datalab::domain::DataTable& table,
    std::size_t column)
{
    std::vector<double> values;
    values.reserve(table.rows.size());
    for (const auto& row : table.rows) {
        if (column >= row.size()) {
            continue;
        }
        bool ok = false;
        const double value = QString::fromStdString(row[column]).toDouble(&ok);
        if (ok && std::isfinite(value)) {
            values.push_back(value);
        }
    }
    return values;
}

std::vector<std::vector<double>> subgroups_by_label(
    const datalab::domain::DataTable& table,
    std::size_t value_column,
    std::size_t label_column)
{
    std::vector<std::string> order;
    std::map<std::string, std::vector<double>> groups;
    for (const auto& row : table.rows) {
        if (value_column >= row.size() || label_column >= row.size()) {
            continue;
        }
        bool ok = false;
        const double value = QString::fromStdString(row[value_column]).toDouble(&ok);
        if (!ok || !std::isfinite(value)) {
            continue;
        }
        const std::string& label = row[label_column];
        if (groups.find(label) == groups.end()) {
            order.push_back(label);
        }
        groups[label].push_back(value);
    }
    std::vector<std::vector<double>> subgroups;
    subgroups.reserve(order.size());
    for (const auto& label : order) {
        subgroups.push_back(groups[label]);
    }
    return subgroups;
}

datalab::domain::AnalysisConfiguration regression_configuration(
    const datalab::domain::DataTable& table)
{
    Q_UNUSED(table);
    datalab::domain::AnalysisConfiguration configuration;
    configuration.variable_columns = {0, 1, 2};
    configuration.inference.confidence_level = 0.95;
    return configuration;
}

datalab::domain::AnalysisConfiguration arima_configuration()
{
    datalab::domain::AnalysisConfiguration configuration;
    configuration.time_series.arima_time_column = 0;
    configuration.time_series.arima_value_column = 1;
    configuration.time_series.arima_selection_criterion = "aicc";
    configuration.time_series.forecast_periods = 4;
    configuration.time_series.arima_differencing = 1;
    configuration.inference.confidence_level = 0.95;
    return configuration;
}

std::optional<std::string> table_cell_as_string(
    const std::vector<std::vector<std::string>>& rows,
    const std::string& key_column,
    const std::string& key_value,
    const std::string& value_column)
{
    if (rows.empty()) {
        return std::nullopt;
    }
    std::size_t key_index = 0;
    std::size_t value_index = 0;
    for (std::size_t index = 0; index < rows.front().size(); ++index) {
        if (rows.front()[index] == key_column) {
            key_index = index;
        }
        if (rows.front()[index] == value_column) {
            value_index = index;
        }
    }
    Q_UNUSED(key_index);
    const auto row = datalab::tests::minitab::find_table_row(rows, key_column, key_value);
    if (!row.has_value() || value_index >= rows[*row].size()) {
        return std::nullopt;
    }
    return rows[*row][value_index];
}

std::optional<std::string> golden_best_order(
    const datalab::tests::minitab::GoldenDocument& golden)
{
    const auto best = golden.sections.find("best");
    if (best == golden.sections.end()) {
        return std::nullopt;
    }
    if (auto value = table_cell_as_string(best->second, "Key", "best_order", "Value")) {
        return value;
    }
    for (const auto& row : best->second) {
        if (row.size() >= 2 && row[0] == "best_order") {
            return row[1];
        }
    }
    return std::nullopt;
}

std::optional<double> table_header_metric(
    const datalab::domain::OutputPage& page,
    const std::string& table_title,
    const std::string& header_name)
{
    for (const auto& table : page.tables) {
        if (table.title != table_title || table.rows.empty()) {
            continue;
        }
        for (std::size_t index = 0; index < table.headers.size(); ++index) {
            if (table.headers[index] == header_name
                && index < table.rows.front().size()) {
                return datalab::tests::minitab::parse_double(table.rows.front()[index]);
            }
        }
    }
    return std::nullopt;
}

}  // namespace

/*
 * MinitabNumericalGoldenTest
 *
 * Historical slots (regression / arima):
 *   - Names retain MatchesMinitabGolden for compatibility.
 *   - Evidence is formula-mirror / non-vendor; missing files may QSKIP.
 *   - Historical QSKIP ≠ G-Trust Goal completion.
 *
 * G-Trust slots (*MatchesRefGolden):
 *   - Evidence: golden <- reference_implementation (NOT vendor_oracle).
 *   - Missing *_ref_golden.tsv must FAIL (QFAIL), never QSKIP.
 *   - Mounted on existing CMake target minitab_numerical_golden_test.
 */
class MinitabNumericalGoldenTest final : public QObject {
    Q_OBJECT

private slots:
    // Historical (formula mirror; QSKIP if missing — not G-Trust DoD)
    void regressionMatchesMinitabGolden();
    void arimaTrendMatchesMinitabGolden();
    void regressionDomainMatchesGoldenAnova();
    void arimaDomainBestModelMatchesGolden();

    // G-Trust Wave-1 ref-golden (FAIL if missing)
    void imrMatchesRefGolden();
    void xbarRMatchesRefGolden();
    void pChartMatchesRefGolden();

    // G-Trust Wave-2 ref-golden (FAIL if missing)
    void capabilityMatchesRefGolden();
    void capabilitySixpackMatchesRefGolden();
    void betweenWithinCapabilityMatchesRefGolden();

    // G-Trust Wave-3 ref-golden (FAIL if missing)
    void gageRrMatchesRefGolden();
    void twoSampleTMatchesRefGolden();
    void normalityTestMatchesRefGolden();
    void oneWayAnovaMatchesRefGolden();
};

void MinitabNumericalGoldenTest::regressionMatchesMinitabGolden()
{
    // Historical path: QSKIP if missing ≠ G-Trust Goal completion.
    const std::string golden_path = expected_path("regression_golden.tsv");
    std::string error;
    const auto golden = datalab::tests::minitab::load_golden_tsv(golden_path, &error);
    if (!golden.has_value()) {
        QSKIP(qPrintable(QString::fromStdString(error)));
    }

    QString import_error;
    const auto table = load_fixture_csv("regression.csv", &import_error);
    if (table.columns.empty()) {
        QSKIP(qPrintable(import_error));
    }

    const auto page = datalab::application::AnalysisService::regression(
        table, regression_configuration(table));
    QVERIFY2(page.diagnostics.empty()
                 || std::none_of(page.diagnostics.begin(), page.diagnostics.end(),
                                 [](const datalab::domain::DiagnosticMessage& message) {
                                     return message.severity
                                         == datalab::domain::DiagnosticMessage::Severity::error;
                                 }),
             "回归输出包含错误诊断");

    const auto summary = golden->sections.find("summary");
    QVERIFY(summary != golden->sections.end());
    datalab::tests::minitab::GoldenTolerance tolerance;
    tolerance.abs_tol = 1.0e-4;
    tolerance.rel_tol = 1.0e-4;

    const auto s = table_header_metric(page, "模型摘要", "S");
    const auto expected_s = datalab::tests::minitab::table_cell_as_double(
        summary->second, "Metric", "S", "Value");
    QVERIFY(s.has_value() && expected_s.has_value());
    QVERIFY2(datalab::tests::minitab::compare_double(*s, *expected_s, tolerance),
             "S 不匹配");

    const auto dw = table_header_metric(page, "模型摘要", "Durbin-Watson");
    const auto expected_dw = datalab::tests::minitab::table_cell_as_double(
        summary->second, "Metric", "Durbin-Watson", "Value");
    QVERIFY(dw.has_value() && expected_dw.has_value());
    QVERIFY2(datalab::tests::minitab::compare_double(*dw, *expected_dw, tolerance),
             "Durbin-Watson 不匹配");

    const auto anova = golden->sections.find("anova");
    QVERIFY(anova != golden->sections.end());
    for (const auto& table : page.tables) {
        if (table.title != "回归方差分析") {
            continue;
        }
        for (const auto& row : table.rows) {
            if (row.empty() || row[0] == "误差" || row[0] == "合计") {
                continue;
            }
            const auto expected_seq = datalab::tests::minitab::table_cell_as_double(
                anova->second, "Term", row[0], "Seq_SS");
            const auto expected_adj = datalab::tests::minitab::table_cell_as_double(
                anova->second, "Term", row[0], "Adj_SS");
            QVERIFY(expected_seq.has_value() && expected_adj.has_value());
            const auto actual_seq = datalab::tests::minitab::parse_double(row[1]);
            const auto actual_adj = datalab::tests::minitab::parse_double(row[2]);
            QVERIFY(actual_seq.has_value() && actual_adj.has_value());
            QVERIFY2(datalab::tests::minitab::compare_double(*actual_seq, *expected_seq, tolerance),
                     qPrintable(QStringLiteral("Seq SS 不匹配: %1").arg(
                         QString::fromStdString(row[0]))));
            QVERIFY2(datalab::tests::minitab::compare_double(*actual_adj, *expected_adj, tolerance),
                     qPrintable(QStringLiteral("Adj SS 不匹配: %1").arg(
                         QString::fromStdString(row[0]))));
        }
        break;
    }

    const auto coefficients = golden->sections.find("coefficients");
    QVERIFY(coefficients != golden->sections.end());
    for (const auto& table : page.tables) {
        if (table.title != "系数") {
            continue;
        }
        for (const auto& row : table.rows) {
            if (row.size() < 2) {
                continue;
            }
            const auto expected_coef = datalab::tests::minitab::table_cell_as_double(
                coefficients->second, "Term", row[0], "Coef");
            QVERIFY(expected_coef.has_value());
            const auto actual_coef = datalab::tests::minitab::parse_double(row[1]);
            QVERIFY(actual_coef.has_value());
            QVERIFY2(datalab::tests::minitab::compare_double(*actual_coef, *expected_coef, tolerance),
                     qPrintable(QStringLiteral("系数不匹配: %1").arg(
                         QString::fromStdString(row[0]))));
        }
        break;
    }
}

void MinitabNumericalGoldenTest::regressionDomainMatchesGoldenAnova()
{
    // Historical path: QSKIP if missing ≠ G-Trust Goal completion.
    const std::string golden_path = expected_path("regression_golden.tsv");
    std::string error;
    const auto golden = datalab::tests::minitab::load_golden_tsv(golden_path, &error);
    if (!golden.has_value()) {
        QSKIP(qPrintable(QString::fromStdString(error)));
    }

    const auto regression = datalab::domain::statistics::fit_linear_regression(
        {12.1, 13.4, 14.2, 15.8, 17.1, 18.5},
        {{20.0, 2.1}, {22.0, 2.4}, {24.0, 2.8}, {26.0, 3.0}, {28.0, 3.4}, {30.0, 3.7}},
        {"Temperature", "Pressure"},
        0.95);
    QCOMPARE(regression.anova_effects.size(), std::size_t{2});

    const auto anova = golden->sections.find("anova");
    QVERIFY(anova != golden->sections.end());
    datalab::tests::minitab::GoldenTolerance tolerance;
    for (const auto& effect : regression.anova_effects) {
        const auto expected_seq = datalab::tests::minitab::table_cell_as_double(
            anova->second, "Term", effect.term, "Seq_SS");
        const auto expected_adj = datalab::tests::minitab::table_cell_as_double(
            anova->second, "Term", effect.term, "Adj_SS");
        QVERIFY(expected_seq.has_value() && expected_adj.has_value());
        QVERIFY(effect.sequential_sum_of_squares.has_value());
        QVERIFY(effect.adjusted_sum_of_squares.has_value());
        QVERIFY(datalab::tests::minitab::compare_double(
            *effect.sequential_sum_of_squares, *expected_seq, tolerance));
        QVERIFY(datalab::tests::minitab::compare_double(
            *effect.adjusted_sum_of_squares, *expected_adj, tolerance));
    }
}

void MinitabNumericalGoldenTest::arimaTrendMatchesMinitabGolden()
{
    // Historical path: QSKIP if missing ≠ G-Trust Goal completion.
    const std::string golden_path = expected_path("arima_trend_golden.tsv");
    std::string error;
    const auto golden = datalab::tests::minitab::load_golden_tsv(golden_path, &error);
    if (!golden.has_value()) {
        QSKIP(qPrintable(QString::fromStdString(error)));
    }

    QString import_error;
    const auto table = load_fixture_csv("arima_trend.csv", &import_error);
    if (table.columns.empty()) {
        QSKIP(qPrintable(import_error));
    }

    const auto page = datalab::application::AnalysisService::arima(
        table, arima_configuration());
    QVERIFY(!page.tables.empty());

    const auto best_order = golden_best_order(*golden);
    QVERIFY(best_order.has_value());

    const auto candidates = golden->sections.find("candidates");
    QVERIFY(candidates != golden->sections.end());
    const auto golden_aicc = datalab::tests::minitab::table_cell_as_double(
        candidates->second, "Order", *best_order, "AICc");
    QVERIFY(golden_aicc.has_value());

    datalab::tests::minitab::GoldenTolerance info_tolerance;
    info_tolerance.abs_tol = 0.01;
    info_tolerance.rel_tol = 1.0e-4;

    bool matched_candidate = false;
    for (const auto& table_section : page.tables) {
        if (table_section.title != "候选模型比较") {
            continue;
        }
        for (const auto& row : table_section.rows) {
            if (row.empty() || row[0] != *best_order) {
                continue;
            }
            const auto aicc = datalab::tests::minitab::parse_double(row[3]);
            QVERIFY(aicc.has_value());
            QVERIFY2(datalab::tests::minitab::compare_double(*aicc, *golden_aicc, info_tolerance),
                     qPrintable(QStringLiteral("最优模型 %1 AICc 不匹配")
                                    .arg(QString::fromStdString(*best_order))));
            matched_candidate = true;
        }
    }
    QVERIFY(matched_candidate);

    const auto forecast_golden = golden->sections.find("forecast");
    QVERIFY(forecast_golden != golden->sections.end());
    datalab::tests::minitab::GoldenTolerance forecast_tolerance;
    forecast_tolerance.abs_tol = 0.05;
    forecast_tolerance.rel_tol = 1.0e-3;

    for (const auto& table_section : page.tables) {
        if (table_section.title != "模型摘要与预测") {
            continue;
        }
        for (std::size_t index = 0; index < table_section.rows.size(); ++index) {
            const auto& row = table_section.rows[index];
            if (row.size() < 7) {
                continue;
            }
            const auto period = datalab::tests::minitab::parse_double(row[3]);
            if (!period.has_value()) {
                continue;
            }
            const int period_index = static_cast<int>(*period);
            const auto expected_fc = datalab::tests::minitab::table_cell_as_double(
                forecast_golden->second, "Period", std::to_string(period_index), "Forecast");
            const auto expected_lo = datalab::tests::minitab::table_cell_as_double(
                forecast_golden->second, "Period", std::to_string(period_index), "Lower");
            const auto expected_up = datalab::tests::minitab::table_cell_as_double(
                forecast_golden->second, "Period", std::to_string(period_index), "Upper");
            QVERIFY(expected_fc.has_value() && expected_lo.has_value() && expected_up.has_value());
            QVERIFY(row.size() >= 7);
            const auto actual_fc = datalab::tests::minitab::parse_double(row[4]);
            const auto actual_lo = datalab::tests::minitab::parse_double(row[5]);
            const auto actual_up = datalab::tests::minitab::parse_double(row[6]);
            QVERIFY(actual_fc.has_value() && actual_lo.has_value() && actual_up.has_value());
            QVERIFY2(datalab::tests::minitab::compare_double(*actual_fc, *expected_fc, forecast_tolerance),
                     qPrintable(QStringLiteral("Forecast 期 %1 不匹配").arg(period_index)));
            QVERIFY(datalab::tests::minitab::compare_double(
                *actual_lo, *expected_lo, forecast_tolerance));
            QVERIFY(datalab::tests::minitab::compare_double(
                *actual_up, *expected_up, forecast_tolerance));
        }
        break;
    }
}

void MinitabNumericalGoldenTest::arimaDomainBestModelMatchesGolden()
{
    // Historical path: QSKIP if missing ≠ G-Trust Goal completion.
    const std::string golden_path = expected_path("arima_trend_golden.tsv");
    std::string error;
    const auto golden = datalab::tests::minitab::load_golden_tsv(golden_path, &error);
    if (!golden.has_value()) {
        QSKIP(qPrintable(QString::fromStdString(error)));
    }

    std::vector<double> values{
        100.0, 101.2, 100.8, 102.1, 103.0, 102.7, 104.2, 105.1, 104.8, 106.0,
        107.2, 106.8, 108.1, 109.0, 108.7, 110.2};
    const auto candidates = datalab::domain::statistics::fit_arima_candidates(
        values, 4, 3, 3, 1);
    QVERIFY(!candidates.empty());

    const auto best_order = golden_best_order(*golden);
    QVERIFY(best_order.has_value());

    const auto best = std::min_element(
        candidates.cbegin(), candidates.cend(),
        [](const auto& first, const auto& second) {
            return first.aicc < second.aicc;
        });
    QCOMPARE(datalab::domain::statistics::arima_order_label(best->order),
             *best_order);

    const auto candidates_golden = golden->sections.find("candidates");
    QVERIFY(candidates_golden != golden->sections.end());
    const auto expected_aicc = datalab::tests::minitab::table_cell_as_double(
        candidates_golden->second, "Order", *best_order, "AICc");
    QVERIFY(expected_aicc.has_value());
    datalab::tests::minitab::GoldenTolerance tolerance;
    tolerance.abs_tol = 0.01;
    QVERIFY(datalab::tests::minitab::compare_double(best->aicc, *expected_aicc, tolerance));
    QCOMPARE(best->forecasts.size(), std::size_t{4});
}

void MinitabNumericalGoldenTest::imrMatchesRefGolden()
{
    datalab::tests::minitab::GoldenDocument golden;
    QVERIFY(load_required_ref_golden("imr_ref_golden.tsv", &golden));
    QString import_error;
    const auto table = load_fixture_csv("imr_ref_golden_input.csv", &import_error);
    QVERIFY2(!table.columns.empty(), qPrintable(import_error));

    const auto value_col = column_index(table, "Value");
    QVERIFY(value_col.has_value());
    const auto values = column_as_doubles(table, *value_col);
    QCOMPARE(values.size(), std::size_t{4});

    datalab::domain::statistics::IndividualsMovingRangeOptions options;
    options.moving_range_length = 2;
    options.method = datalab::domain::statistics::SigmaEstimateMethod::average_moving_range;
    options.use_nelson_estimate = false;
    const auto dual =
        datalab::domain::statistics::ControlCharts::individuals_moving_range_dual(
            values, options);
    QVERIFY(dual.diagnostics.empty()
            || std::none_of(dual.diagnostics.begin(), dual.diagnostics.end(),
                            [](const datalab::domain::DiagnosticMessage& message) {
                                return message.severity
                                    == datalab::domain::DiagnosticMessage::Severity::error;
                            }));
    QVERIFY(!dual.primary.center_line.empty());
    QVERIFY(!dual.primary.upper_control_limit.empty());
    QVERIFY(!dual.primary.lower_control_limit.empty());
    QVERIFY(!dual.secondary.upper_control_limit.empty());

    const auto metrics = golden.sections.find("metrics");
    QVERIFY(metrics != golden.sections.end());
    const auto tolerance = control_limit_tolerance();
    QString failure;
    QVERIFY2(expect_metric(metrics->second, "i_cl", dual.primary.center_line[0], tolerance,
                           "i_cl", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(metrics->second, "i_ucl", dual.primary.upper_control_limit[0],
                           tolerance, "i_ucl", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(metrics->second, "i_lcl", dual.primary.lower_control_limit[0],
                           tolerance, "i_lcl", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(metrics->second, "sigma", dual.sigma, tolerance, "sigma", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(metrics->second, "mr_bar", dual.average_moving_range, tolerance,
                           "mr_bar", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(metrics->second, "mr_ucl", dual.secondary.upper_control_limit[0],
                           tolerance, "mr_ucl", &failure),
             qPrintable(failure));
}

void MinitabNumericalGoldenTest::xbarRMatchesRefGolden()
{
    datalab::tests::minitab::GoldenDocument golden;
    QVERIFY(load_required_ref_golden("xbar_r_ref_golden.tsv", &golden));
    QString import_error;
    const auto table = load_fixture_csv("CamshaftLength.csv", &import_error);
    QVERIFY2(!table.columns.empty(), qPrintable(import_error));

    const auto value_col = column_index(table, "Machine 1");
    const auto subgroup_col = column_index(table, "Subgroup ID");
    QVERIFY(value_col.has_value());
    QVERIFY(subgroup_col.has_value());
    const auto subgroups = subgroups_by_label(table, *value_col, *subgroup_col);
    QVERIFY(!subgroups.empty());
    QCOMPARE(subgroups.front().size(), std::size_t{5});

    const auto dual =
        datalab::domain::statistics::ControlCharts::xbar_range_dual(subgroups);
    QVERIFY(dual.diagnostics.empty()
            || std::none_of(dual.diagnostics.begin(), dual.diagnostics.end(),
                            [](const datalab::domain::DiagnosticMessage& message) {
                                return message.severity
                                    == datalab::domain::DiagnosticMessage::Severity::error;
                            }));
    QVERIFY(!dual.primary.center_line.empty());
    QVERIFY(!dual.secondary.center_line.empty());

    const auto xbar_limits = golden.sections.find("xbar_limits");
    const auto r_limits = golden.sections.find("r_limits");
    const auto summary = golden.sections.find("summary");
    QVERIFY(xbar_limits != golden.sections.end());
    QVERIFY(r_limits != golden.sections.end());
    QVERIFY(summary != golden.sections.end());
    const auto tolerance = control_limit_tolerance();
    QString failure;

    QVERIFY2(expect_metric(xbar_limits->second, "cl", dual.primary.center_line[0], tolerance,
                           "xbar_cl", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(xbar_limits->second, "ucl", dual.primary.upper_control_limit[0],
                           tolerance, "xbar_ucl", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(xbar_limits->second, "lcl", dual.primary.lower_control_limit[0],
                           tolerance, "xbar_lcl", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(r_limits->second, "cl", dual.secondary.center_line[0], tolerance,
                           "r_cl", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(r_limits->second, "ucl", dual.secondary.upper_control_limit[0],
                           tolerance, "r_ucl", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(r_limits->second, "lcl", dual.secondary.lower_control_limit[0],
                           tolerance, "r_lcl", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(summary->second, "r_bar", dual.average_moving_range, tolerance,
                           "r_bar", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(summary->second, "sigma", dual.sigma, tolerance, "sigma", &failure),
             qPrintable(failure));
}

void MinitabNumericalGoldenTest::pChartMatchesRefGolden()
{
    datalab::tests::minitab::GoldenDocument golden;
    QVERIFY(load_required_ref_golden("p_chart_ref_golden.tsv", &golden));
    QString import_error;
    const auto table = load_fixture_csv("UnansweredCalls.csv", &import_error);
    QVERIFY2(!table.columns.empty(), qPrintable(import_error));

    const auto defectives_col = column_index(table, "Unanswered Calls");
    const auto inspected_col = column_index(table, "Total Calls");
    QVERIFY(defectives_col.has_value());
    QVERIFY(inspected_col.has_value());

    std::vector<std::size_t> defectives;
    std::vector<std::size_t> inspected;
    defectives.reserve(table.rows.size());
    inspected.reserve(table.rows.size());
    for (const auto& row : table.rows) {
        bool ok_d = false;
        bool ok_n = false;
        const double d = QString::fromStdString(row[*defectives_col]).toDouble(&ok_d);
        const double n = QString::fromStdString(row[*inspected_col]).toDouble(&ok_n);
        QVERIFY(ok_d && ok_n);
        defectives.push_back(static_cast<std::size_t>(d));
        inspected.push_back(static_cast<std::size_t>(n));
    }
    QCOMPARE(defectives.size(), std::size_t{21});

    const auto result =
        datalab::domain::statistics::ControlCharts::p_chart(defectives, inspected);
    QVERIFY(result.diagnostics.empty()
            || std::none_of(result.diagnostics.begin(), result.diagnostics.end(),
                            [](const datalab::domain::DiagnosticMessage& message) {
                                return message.severity
                                    == datalab::domain::DiagnosticMessage::Severity::error;
                            }));
    QVERIFY(!result.center_line.empty());
    QCOMPARE(result.upper_control_limit.size(), defectives.size());

    const auto summary = golden.sections.find("summary");
    const auto limits = golden.sections.find("limits");
    QVERIFY(summary != golden.sections.end());
    QVERIFY(limits != golden.sections.end());
    const auto tolerance = control_limit_tolerance();
    QString failure;

    QVERIFY2(expect_metric(summary->second, "p_bar", result.center_line[0], tolerance, "p_bar",
                           &failure),
             qPrintable(failure));

    // Freeze index 0 + max-n + min-n variable limits (plan §5.9).
    std::size_t max_n_index = 0;
    std::size_t min_n_index = 0;
    for (std::size_t index = 1; index < inspected.size(); ++index) {
        if (inspected[index] > inspected[max_n_index]) {
            max_n_index = index;
        }
        if (inspected[index] < inspected[min_n_index]) {
            min_n_index = index;
        }
    }
    const std::vector<std::size_t> check_indices{0, max_n_index, min_n_index};
    for (const std::size_t index : check_indices) {
        const auto expected_ucl = datalab::tests::minitab::table_cell_as_double(
            limits->second, "Index", std::to_string(index), "UCL");
        const auto expected_lcl = datalab::tests::minitab::table_cell_as_double(
            limits->second, "Index", std::to_string(index), "LCL");
        QVERIFY2(expected_ucl.has_value() && expected_lcl.has_value(),
                 qPrintable(QStringLiteral("missing limits row Index=%1").arg(index)));
        QVERIFY2(datalab::tests::minitab::compare_double(
                     result.upper_control_limit[index], *expected_ucl, tolerance),
                 qPrintable(QStringLiteral("UCL[%1] mismatch").arg(index)));
        QVERIFY2(datalab::tests::minitab::compare_double(
                     result.lower_control_limit[index], *expected_lcl, tolerance),
                 qPrintable(QStringLiteral("LCL[%1] mismatch").arg(index)));
    }
}

void MinitabNumericalGoldenTest::capabilityMatchesRefGolden()
{
    datalab::tests::minitab::GoldenDocument golden;
    QVERIFY(load_required_ref_golden("capability_ref_golden.tsv", &golden));
    QString import_error;
    const auto table = load_fixture_csv("PistonRingDiameter.csv", &import_error);
    QVERIFY2(!table.columns.empty(), qPrintable(import_error));

    const auto value_col = column_index(table, "Diameter");
    QVERIFY(value_col.has_value());
    const auto values = column_as_doubles(table, *value_col);
    QCOMPARE(values.size(), std::size_t{125});
    const auto subgroups = fixed_size_subgroups(values, 5);
    QCOMPARE(subgroups.size(), std::size_t{25});

    const auto within =
        datalab::domain::statistics::estimate_within_subgroup_sigma(subgroups);
    QVERIFY(within.ok);
    QCOMPARE(QString::fromStdString(within.method), QStringLiteral("R̄ / d2(n)"));

    datalab::domain::SpecificationLimits specs;
    specs.lower = 73.95;
    specs.upper = 74.05;
    specs.target = 74.00;
    const auto result = datalab::domain::statistics::ProcessCapability::calculate(
        values, within.sigma, specs);
    QVERIFY(result.cp.has_value() && result.cpk.has_value());
    QVERIFY(result.pp.has_value() && result.ppk.has_value());

    const auto indices = golden.sections.find("indices");
    QVERIFY(indices != golden.sections.end());
    const auto index_tol = capability_index_tolerance();
    const auto sigma_tol = sigma_tolerance();
    QString failure;
    QVERIFY2(expect_metric(indices->second, "within_sigma", within.sigma, sigma_tol,
                           "within_sigma", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(indices->second, "Cp", *result.cp, index_tol, "Cp", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(indices->second, "Cpk", *result.cpk, index_tol, "Cpk",
                           &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(indices->second, "Pp", *result.pp, index_tol, "Pp", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(indices->second, "Ppk", *result.ppk, index_tol, "Ppk",
                           &failure),
             qPrintable(failure));
}

void MinitabNumericalGoldenTest::capabilitySixpackMatchesRefGolden()
{
    datalab::tests::minitab::GoldenDocument golden;
    QVERIFY(load_required_ref_golden("capability_sixpack_ref_golden.tsv", &golden));
    QString import_error;
    const auto table = load_fixture_csv("PistonRingDiameter.csv", &import_error);
    QVERIFY2(!table.columns.empty(), qPrintable(import_error));

    const auto value_col = column_index(table, "Diameter");
    QVERIFY(value_col.has_value());
    const auto values = column_as_doubles(table, *value_col);
    const auto subgroups = fixed_size_subgroups(values, 5);
    const auto within =
        datalab::domain::statistics::estimate_within_subgroup_sigma(subgroups);
    QVERIFY(within.ok);

    datalab::domain::SpecificationLimits specs;
    specs.lower = 73.95;
    specs.upper = 74.05;
    specs.target = 74.00;
    const auto capability = datalab::domain::statistics::ProcessCapability::calculate(
        values, within.sigma, specs);

    const auto indices = golden.sections.find("indices");
    const auto contract = golden.sections.find("contract");
    QVERIFY(indices != golden.sections.end());
    QVERIFY(contract != golden.sections.end());
    const auto index_tol = capability_index_tolerance();
    QString failure;
    QVERIFY2(expect_metric(indices->second, "Cp", *capability.cp, index_tol, "Cp",
                           &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(indices->second, "Cpk", *capability.cpk, index_tol, "Cpk",
                           &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(indices->second, "Pp", *capability.pp, index_tol, "Pp",
                           &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(indices->second, "Ppk", *capability.ppk, index_tol, "Ppk",
                           &failure),
             qPrintable(failure));

    const auto min_plots = datalab::tests::minitab::table_cell_as_double(
        contract->second, "Key", "min_plots", "Value");
    const auto expected_plots = datalab::tests::minitab::table_cell_as_double(
        contract->second, "Key", "expected_plots", "Value");
    QVERIFY(min_plots.has_value() && expected_plots.has_value());
    QCOMPARE(static_cast<int>(*min_plots), 5);
    QCOMPARE(static_cast<int>(*expected_plots), 6);

    datalab::domain::AnalysisConfiguration configuration;
    configuration.chart_type = "capability_sixpack";
    configuration.variable_columns = {*value_col};
    configuration.specifications = specs;
    configuration.control.subgroup_size = 5;
    const auto page = datalab::application::AnalysisService::capability_sixpack(
        table, configuration);
    QVERIFY(static_cast<int>(page.plots.size()) >= static_cast<int>(*min_plots));
    QCOMPARE(static_cast<int>(page.plots.size()), static_cast<int>(*expected_plots));
}

void MinitabNumericalGoldenTest::betweenWithinCapabilityMatchesRefGolden()
{
    datalab::tests::minitab::GoldenDocument golden;
    QVERIFY(load_required_ref_golden(
        "between_within_capability_ref_golden.tsv", &golden));
    QString import_error;
    const auto table = load_fixture_csv("cap_between_within.csv", &import_error);
    QVERIFY2(!table.columns.empty(), qPrintable(import_error));

    const auto value_col = column_index(table, "厚度_um");
    const auto subgroup_col = column_index(table, "子组");
    QVERIFY(value_col.has_value());
    QVERIFY(subgroup_col.has_value());
    const auto subgroups = subgroups_by_label(table, *value_col, *subgroup_col);
    QVERIFY(subgroups.size() >= 2);
    std::vector<double> observations;
    for (const auto& subgroup : subgroups) {
        observations.insert(observations.end(), subgroup.begin(), subgroup.end());
    }

    datalab::domain::SpecificationLimits specs;
    specs.lower = 95.0;
    specs.upper = 105.0;
    specs.target = 100.0;
    const auto result =
        datalab::domain::statistics::ProcessCapability::calculate_between_within(
            observations, subgroups, specs);
    QCOMPARE(QString::fromStdString(result.capability_method),
             QStringLiteral("between_within"));
    QVERIFY(result.subgroup_within_standard_deviation.has_value());
    QVERIFY(result.between_standard_deviation.has_value());
    QVERIFY(result.between_within_standard_deviation.has_value());
    QVERIFY(result.cp.has_value() && result.cpk.has_value());

    const auto sigma = golden.sections.find("sigma");
    const auto indices = golden.sections.find("indices");
    QVERIFY(sigma != golden.sections.end());
    QVERIFY(indices != golden.sections.end());
    const auto sigma_tol = sigma_tolerance();
    const auto index_tol = capability_index_tolerance();
    QString failure;
    QVERIFY2(expect_metric(sigma->second, "sigma_within",
                           *result.subgroup_within_standard_deviation, sigma_tol,
                           "sigma_within", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(sigma->second, "sigma_between",
                           *result.between_standard_deviation, sigma_tol,
                           "sigma_between", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(sigma->second, "sigma_bw",
                           *result.between_within_standard_deviation, sigma_tol,
                           "sigma_bw", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(indices->second, "Cp", *result.cp, index_tol, "Cp", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(indices->second, "Cpk", *result.cpk, index_tol, "Cpk",
                           &failure),
             qPrintable(failure));
}

void MinitabNumericalGoldenTest::gageRrMatchesRefGolden()
{
    datalab::tests::minitab::GoldenDocument golden;
    QVERIFY(load_required_ref_golden("gage_rr_ref_golden.tsv", &golden));
    QString import_error;
    const auto table = load_fixture_csv("gage_rr_crossed.csv", &import_error);
    QVERIFY2(!table.columns.empty(), qPrintable(import_error));

    const auto part_col = column_index(table, "Part");
    const auto operator_col = column_index(table, "Operator");
    const auto meas_col = column_index(table, "Measurement");
    QVERIFY(part_col.has_value() && operator_col.has_value() && meas_col.has_value());

    std::vector<double> measurements;
    std::vector<std::string> parts;
    std::vector<std::string> operators;
    measurements.reserve(table.rows.size());
    for (const auto& row : table.rows) {
        bool ok = false;
        const double value = QString::fromStdString(row[*meas_col]).toDouble(&ok);
        QVERIFY(ok);
        measurements.push_back(value);
        parts.push_back(row[*part_col]);
        operators.push_back(row[*operator_col]);
    }

    const auto result = datalab::domain::statistics::crossed_gage_rr(
        measurements, parts, operators, 0.0);
    QVERIFY(result.ndc_available);
    std::optional<double> percent_study;
    for (const auto& component : result.variance_components) {
        if (component.source == "Total Gage R&R") {
            percent_study = component.percent_study_variation;
            break;
        }
    }
    QVERIFY(percent_study.has_value());

    const auto summary = golden.sections.find("summary");
    QVERIFY(summary != golden.sections.end());
    const auto gage_tol = gage_percent_tolerance();
    QString failure;
    QVERIFY2(expect_metric(summary->second, "percent_study_variation_total_gage_rr",
                           *percent_study, gage_tol, "%StudyVar", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(summary->second, "ndc", result.ndc, gage_tol, "ndc",
                           &failure),
             qPrintable(failure));
}

void MinitabNumericalGoldenTest::twoSampleTMatchesRefGolden()
{
    datalab::tests::minitab::GoldenDocument golden;
    QVERIFY(load_required_ref_golden("two_sample_t_ref_golden.tsv", &golden));
    QString import_error;
    const auto table =
        load_fixture_csv("two_sample_t_ref_golden_input.csv", &import_error);
    QVERIFY2(!table.columns.empty(), qPrintable(import_error));

    const auto first_col = column_index(table, "Sample1");
    const auto second_col = column_index(table, "Sample2");
    QVERIFY(first_col.has_value() && second_col.has_value());
    const auto first = column_as_doubles(table, *first_col);
    const auto second = column_as_doubles(table, *second_col);

    const auto result = datalab::domain::statistics::two_sample_t_test(
        first,
        second,
        0.95,
        datalab::domain::statistics::TestAlternative::two_sided,
        datalab::domain::statistics::VarianceMethod::welch);
    QVERIFY(result.p_value.has_value());
    QVERIFY(result.confidence_lower.has_value() && result.confidence_upper.has_value());

    const auto summary = golden.sections.find("summary");
    QVERIFY(summary != golden.sections.end());
    const auto infer_tol = inferential_tolerance();
    const auto p_tol = p_value_tolerance();
    datalab::tests::minitab::GoldenTolerance welch_df_tol;
    welch_df_tol.abs_tol = 1.0e-6;
    welch_df_tol.rel_tol = 0.0;
    QString failure;
    QVERIFY2(expect_metric(summary->second, "mean_difference", result.mean_difference,
                           infer_tol, "mean_difference", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(summary->second, "t_statistic", result.t_statistic, infer_tol,
                           "t_statistic", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(summary->second, "degrees_of_freedom",
                           result.degrees_of_freedom, welch_df_tol, "df", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(summary->second, "p_value", *result.p_value, p_tol, "p_value",
                           &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(summary->second, "confidence_lower", *result.confidence_lower,
                           infer_tol, "ci_lo", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(summary->second, "confidence_upper", *result.confidence_upper,
                           infer_tol, "ci_hi", &failure),
             qPrintable(failure));
}

void MinitabNumericalGoldenTest::normalityTestMatchesRefGolden()
{
    datalab::tests::minitab::GoldenDocument golden;
    QVERIFY(load_required_ref_golden("normality_test_ref_golden.tsv", &golden));
    QString import_error;
    const auto table = load_fixture_csv("PistonRingDiameter.csv", &import_error);
    QVERIFY2(!table.columns.empty(), qPrintable(import_error));

    const auto value_col = column_index(table, "Diameter");
    QVERIFY(value_col.has_value());
    const auto values = column_as_doubles(table, *value_col);
    std::vector<std::size_t> source_rows(values.size());
    for (std::size_t index = 0; index < source_rows.size(); ++index) {
        source_rows[index] = index;
    }
    const auto result = datalab::domain::statistics::normality_test(
        values, source_rows, "anderson_darling");
    QCOMPARE(QString::fromStdString(result.method), QStringLiteral("anderson_darling"));
    QVERIFY(result.adjusted_anderson_darling.has_value());
    QVERIFY(result.p_value.has_value());

    const auto summary = golden.sections.find("summary");
    QVERIFY(summary != golden.sections.end());
    const auto method = table_cell_as_string(summary->second, "Key", "method", "Value");
    QVERIFY(method.has_value());
    QCOMPARE(QString::fromStdString(*method), QStringLiteral("anderson_darling"));

    const auto infer_tol = inferential_tolerance();
    const auto p_tol = p_value_tolerance();
    QString failure;
    if (result.anderson_darling.has_value()) {
        QVERIFY2(expect_metric(summary->second, "A2", *result.anderson_darling, infer_tol,
                               "A2", &failure),
                 qPrintable(failure));
    }
    QVERIFY2(expect_metric(summary->second, "A2_star", *result.adjusted_anderson_darling,
                           infer_tol, "A2_star", &failure),
             qPrintable(failure));
    QVERIFY2(expect_metric(summary->second, "p_value", *result.p_value, p_tol, "p_value",
                           &failure),
             qPrintable(failure));
}

void MinitabNumericalGoldenTest::oneWayAnovaMatchesRefGolden()
{
    datalab::tests::minitab::GoldenDocument golden;
    QVERIFY(load_required_ref_golden("one_way_anova_ref_golden.tsv", &golden));
    QString import_error;
    const auto table =
        load_fixture_csv("one_way_anova_ref_golden_input.csv", &import_error);
    QVERIFY2(!table.columns.empty(), qPrintable(import_error));

    const auto factor_col = column_index(table, "Factor");
    const auto response_col = column_index(table, "Response");
    QVERIFY(factor_col.has_value() && response_col.has_value());
    const auto grouped = subgroups_by_label(table, *response_col, *factor_col);
    std::vector<std::string> labels;
    {
        std::map<std::string, bool> seen;
        for (const auto& row : table.rows) {
            const std::string& label = row[*factor_col];
            if (seen.emplace(label, true).second) {
                labels.push_back(label);
            }
        }
    }
    QCOMPARE(grouped.size(), labels.size());

    const auto result = datalab::domain::statistics::one_way_anova(grouped, labels);
    QVERIFY(result.p_value.has_value());

    const auto anova = golden.sections.find("anova");
    QVERIFY(anova != golden.sections.end());
    const auto infer_tol = inferential_tolerance();
    const auto p_tol = p_value_tolerance();

    const auto expected_ss = datalab::tests::minitab::table_cell_as_double(
        anova->second, "Source", "Factor", "SS");
    const auto expected_df = datalab::tests::minitab::table_cell_as_double(
        anova->second, "Source", "Factor", "DF");
    const auto expected_ms = datalab::tests::minitab::table_cell_as_double(
        anova->second, "Source", "Factor", "MS");
    const auto expected_f = datalab::tests::minitab::table_cell_as_double(
        anova->second, "Source", "Factor", "F");
    const auto expected_p = datalab::tests::minitab::table_cell_as_double(
        anova->second, "Source", "Factor", "P");
    QVERIFY(expected_ss && expected_df && expected_ms && expected_f && expected_p);

    QVERIFY(datalab::tests::minitab::compare_double(
        result.between_sum_of_squares, *expected_ss, infer_tol));
    QCOMPARE(static_cast<double>(result.between_degrees_of_freedom), *expected_df);
    QVERIFY(datalab::tests::minitab::compare_double(
        result.between_mean_square, *expected_ms, infer_tol));
    QVERIFY(datalab::tests::minitab::compare_double(
        result.f_statistic, *expected_f, infer_tol));
    QVERIFY(datalab::tests::minitab::compare_double(*result.p_value, *expected_p, p_tol));

    const auto expected_error_ss = datalab::tests::minitab::table_cell_as_double(
        anova->second, "Source", "Error", "SS");
    const auto expected_error_df = datalab::tests::minitab::table_cell_as_double(
        anova->second, "Source", "Error", "DF");
    const auto expected_total_ss = datalab::tests::minitab::table_cell_as_double(
        anova->second, "Source", "Total", "SS");
    const auto expected_total_df = datalab::tests::minitab::table_cell_as_double(
        anova->second, "Source", "Total", "DF");
    QVERIFY(expected_error_ss && expected_error_df && expected_total_ss
            && expected_total_df);
    QVERIFY(datalab::tests::minitab::compare_double(
        result.error_sum_of_squares, *expected_error_ss, infer_tol));
    QCOMPARE(static_cast<double>(result.error_degrees_of_freedom), *expected_error_df);
    QVERIFY(datalab::tests::minitab::compare_double(
        result.total_sum_of_squares, *expected_total_ss, infer_tol));
    QCOMPARE(static_cast<double>(result.total_degrees_of_freedom), *expected_total_df);
}

QTEST_APPLESS_MAIN(MinitabNumericalGoldenTest)

#include "minitab_numerical_golden_test.moc"
