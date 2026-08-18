#include "application/analysis_service.h"
#include "domain/statistics/arima.h"
#include "domain/statistics/regression.h"
#include "fixtures/minitab/golden_loader.h"
#include "infrastructure/csv_importer.h"

#include <QtTest/QtTest>

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

datalab::domain::AnalysisConfiguration regression_configuration(
    const datalab::domain::DataTable& table)
{
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

class MinitabNumericalGoldenTest final : public QObject {
    Q_OBJECT

private slots:
    void regressionMatchesMinitabGolden();
    void arimaTrendMatchesMinitabGolden();
    void regressionDomainMatchesGoldenAnova();
    void arimaDomainBestModelMatchesGolden();
};

void MinitabNumericalGoldenTest::regressionMatchesMinitabGolden()
{
    const std::string golden_path = std::string(DATALAB_SOURCE_DIR)
        + "/tests/fixtures/minitab/expected/regression_golden.tsv";
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
}

void MinitabNumericalGoldenTest::regressionDomainMatchesGoldenAnova()
{
    const std::string golden_path = std::string(DATALAB_SOURCE_DIR)
        + "/tests/fixtures/minitab/expected/regression_golden.tsv";
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
    const std::string golden_path = std::string(DATALAB_SOURCE_DIR)
        + "/tests/fixtures/minitab/expected/arima_trend_golden.tsv";
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
    const std::string golden_path = std::string(DATALAB_SOURCE_DIR)
        + "/tests/fixtures/minitab/expected/arima_trend_golden.tsv";
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

QTEST_APPLESS_MAIN(MinitabNumericalGoldenTest)

#include "minitab_numerical_golden_test.moc"
