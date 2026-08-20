#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/tolerance_intervals.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

class ToleranceIntervalTest final : public QObject {
    Q_OBJECT

private slots:
    void matchesNistHoweTwoSidedK();
    void matchesNistNatrellaOneSidedK();
    void nonparametricReportsAchievedConfidence();
    void diagnosesInsufficientData();
    void serviceBuildsTablesAndCountsMissing();
    void interpretationDoesNotClaimCapability();
};

void ToleranceIntervalTest::matchesNistHoweTwoSidedK()
{
    // # source: formula_reference — NIST 7.2.6.3 published k2=2.217
    // (N=43, p=0.90, confidence=0.99). k depends only on N, P, and 1-α.
    std::vector<double> values;
    values.reserve(43);
    for (int index = 0; index < 43; ++index) {
        values.push_back(10.0 + 0.1 * static_cast<double>(index));
    }
    const auto result = datalab::domain::statistics::normal_tolerance_interval(
        values, {}, 0.90, 0.99, "two_sided");
    QVERIFY(result.k_factor.has_value());
    QVERIFY(std::abs(*result.k_factor - 2.217) < 0.005);
    QVERIFY(result.lower.has_value());
    QVERIFY(result.upper.has_value());
    QCOMPARE(result.method, std::string("howe_two_sided"));
    QVERIFY(std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "two_sided_howe_approximation";
        }));
}

void ToleranceIntervalTest::matchesNistNatrellaOneSidedK()
{
    // # source: formula_reference — NIST 7.2.6.3 published k1=1.8752
    // (N=43, p=0.90, confidence=0.99, upper).
    std::vector<double> values;
    values.reserve(43);
    for (int index = 0; index < 43; ++index) {
        values.push_back(10.0 + 0.1 * static_cast<double>(index));
    }
    const auto result = datalab::domain::statistics::normal_tolerance_interval(
        values, {}, 0.90, 0.99, "upper");
    QVERIFY(result.k_factor.has_value());
    QVERIFY(std::abs(*result.k_factor - 1.8752) < 0.002);
    QVERIFY(!result.lower.has_value());
    QVERIFY(result.upper.has_value());
    QCOMPARE(result.method, std::string("natrella_one_sided"));
}

void ToleranceIntervalTest::nonparametricReportsAchievedConfidence()
{
    const std::vector<double> values = {
        10.0, 10.2, 10.4, 10.8, 11.0, 11.3, 11.6, 12.0, 12.1, 12.3};
    const auto result = datalab::domain::statistics::nonparametric_tolerance_interval(
        values, {}, 0.90, 0.95, "two_sided");
    QCOMPARE(result.method_family, std::string("nonparametric"));
    QVERIFY(result.lower.has_value());
    QVERIFY(result.upper.has_value());
    QVERIFY(result.achieved_confidence.has_value());
    QVERIFY(*result.achieved_confidence > 0.0);
}

void ToleranceIntervalTest::diagnosesInsufficientData()
{
    const auto result = datalab::domain::statistics::normal_tolerance_interval(
        {1.0}, {}, 0.95, 0.95, "two_sided");
    QVERIFY(!result.lower.has_value());
    QVERIFY(!result.upper.has_value());
    QVERIFY(std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "insufficient_data";
        }));
}

void ToleranceIntervalTest::serviceBuildsTablesAndCountsMissing()
{
    datalab::domain::DataTable table;
    table.columns = {"测量"};
    table.rows = {
        {"10"}, {"12"}, {"*"}, {"11"}, {"13"}, {""}, {"14"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.selection.measurement_column = 0;
    configuration.variable_columns = {0};
    configuration.inference.coverage_proportion = 0.95;
    configuration.inference.confidence_level = 0.95;
    configuration.inference.alternative = "two_sided";
    const auto page = datalab::application::AnalysisService::tolerance_intervals(
        table, configuration);
    QCOMPARE(page.method_name, std::string("Tolerance Intervals"));
    QVERIFY(std::any_of(page.tables.cbegin(), page.tables.cend(),
                        [](const datalab::domain::StatisticTable& table_item) {
                            return table_item.title == "过程数据";
                        }));
    QVERIFY(std::any_of(page.tables.cbegin(), page.tables.cend(),
                        [](const datalab::domain::StatisticTable& table_item) {
                            return table_item.title == "正态容差区间";
                        }));
    QVERIFY(page.facts.tolerance.has_value());
    QCOMPARE(page.facts.tolerance->valid_count, std::size_t{5});
    QCOMPARE(page.facts.tolerance->missing_count, std::size_t{2});
    QCOMPARE(page.facts.tolerance->assumption_status, std::string("not_verified"));
    QVERIFY(page.facts.tolerance->lower.has_value());
    QVERIFY(page.facts.tolerance->upper.has_value());
}

void ToleranceIntervalTest::interpretationDoesNotClaimCapability()
{
    datalab::domain::OutputPage page;
    page.method_name = "Tolerance Intervals";
    datalab::domain::ToleranceFacts facts;
    facts.valid_count = 5;
    facts.coverage = 0.95;
    facts.confidence_level = 0.95;
    facts.lower = 8.0;
    facts.upper = 16.0;
    facts.method = "howe_two_sided";
    facts.assumption_status = "not_verified";
    page.facts.tolerance = facts;
    datalab::application::InterpretationService::enrich(page);
    QVERIFY(!page.interpretation.empty());
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
            QVERIFY(bullet.find("规格已覆盖") == std::string::npos);
        }
    }
}

QTEST_APPLESS_MAIN(ToleranceIntervalTest)
#include "tolerance_interval_test.moc"
