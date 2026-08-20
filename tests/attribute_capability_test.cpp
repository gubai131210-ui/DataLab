#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/attribute_capability.h"
#include "domain/statistics/normal_distribution.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

class AttributeCapabilityTest final : public QObject {
    Q_OBJECT

private slots:
    void binomialAveragePMatchesWorkedExample();
    void binomialDiagnosesDefectivesExceedInspected();
    void poissonMeanDpuMatchesWorkedExample();
    void serviceBuildsBinomialTablesWithoutCpk();
    void serviceCountsMissingStarCells();
    void interpretationDoesNotClaimCapability();
};

void AttributeCapabilityTest::binomialAveragePMatchesWorkedExample()
{
    // # source: formula_reference — D={1,2,1}, N={50,50,50} → p̄=4/150
    const std::vector<datalab::domain::statistics::AttributeSample> samples = {
        {1.0, 50.0, 0}, {2.0, 50.0, 1}, {1.0, 50.0, 2}};
    const auto result = datalab::domain::statistics::binomial_capability(samples, 0);
    QVERIFY(result.average_p.has_value());
    QCOMPARE(*result.average_p, 4.0 / 150.0);
    QCOMPARE(*result.percent_defective, 100.0 * 4.0 / 150.0);
    QCOMPARE(*result.ppm_defective, 1.0e6 * 4.0 / 150.0);
    const double expected_z =
        datalab::domain::statistics::standard_normal_quantile(1.0 - 4.0 / 150.0);
    QVERIFY(result.process_z.has_value());
    QVERIFY(std::abs(*result.process_z - expected_z) < 1.0e-8);
    QVERIFY(result.average_p_interval.lower.has_value());
    QVERIFY(result.average_p_interval.upper.has_value());
    QVERIFY(*result.average_p_interval.lower < *result.average_p);
    QVERIFY(*result.average_p < *result.average_p_interval.upper);
    QCOMPARE(result.cumulative_values.size(), std::size_t{3});
    QVERIFY(std::abs(result.cumulative_values.back() - *result.percent_defective) < 1.0e-12);
    QCOMPARE(result.method, std::string("binomial"));
    QCOMPARE(result.assumption_status, std::string("not_verified"));
}

void AttributeCapabilityTest::binomialDiagnosesDefectivesExceedInspected()
{
    const auto result = datalab::domain::statistics::binomial_capability(
        {{6.0, 5.0, 0}}, 0);
    QVERIFY(!result.average_p.has_value());
    QVERIFY(std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const datalab::domain::DiagnosticMessage& diagnostic) {
            return diagnostic.code == "defectives_exceed_inspected"
                || diagnostic.code == "insufficient_data";
        }));
}

void AttributeCapabilityTest::poissonMeanDpuMatchesWorkedExample()
{
    // # source: formula_reference — D={1,2,1}, N={50,50,50} → DPU=4/150
    const std::vector<datalab::domain::statistics::AttributeSample> samples = {
        {1.0, 50.0, 0}, {2.0, 50.0, 1}, {1.0, 50.0, 2}};
    const auto result = datalab::domain::statistics::poisson_capability(samples, 0);
    QVERIFY(result.mean_dpu.has_value());
    QCOMPARE(*result.mean_dpu, 4.0 / 150.0);
    QCOMPARE(*result.mean_defective, 4.0 / 3.0);
    QVERIFY(result.minimum_dpu.has_value());
    QVERIFY(result.maximum_dpu.has_value());
    QCOMPARE(*result.minimum_dpu, 1.0 / 50.0);
    QCOMPARE(*result.maximum_dpu, 2.0 / 50.0);
    QVERIFY(result.mean_dpu_interval.lower.has_value());
    QVERIFY(result.mean_dpu_interval.upper.has_value());
    QVERIFY(*result.mean_dpu_interval.lower < *result.mean_dpu);
    QVERIFY(*result.mean_dpu < *result.mean_dpu_interval.upper);
    QCOMPARE(result.method, std::string("poisson"));
}

void AttributeCapabilityTest::serviceBuildsBinomialTablesWithoutCpk()
{
    datalab::domain::DataTable table;
    table.columns = {"不合格品", "检验数"};
    table.rows = {{"1", "50"}, {"2", "50"}, {"1", "50"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.selection.defect_count_column = 0;
    configuration.selection.inspected_count_column = 1;
    configuration.capability_method = "binomial";
    const auto page = datalab::application::AnalysisService::binomial_capability(
        table, configuration);
    QCOMPARE(page.method_name, std::string("Binomial Capability"));
    QVERIFY(std::any_of(page.tables.cbegin(), page.tables.cend(),
                        [](const datalab::domain::StatisticTable& table_item) {
                            return table_item.title == "过程数据";
                        }));
    QVERIFY(std::any_of(page.tables.cbegin(), page.tables.cend(),
                        [](const datalab::domain::StatisticTable& table_item) {
                            return table_item.title.find("能力") != std::string::npos;
                        }));
    QVERIFY(!std::any_of(page.tables.cbegin(), page.tables.cend(),
                         [](const datalab::domain::StatisticTable& table_item) {
                             return table_item.title.find("Cpk") != std::string::npos
                                 || std::any_of(table_item.rows.cbegin(), table_item.rows.cend(),
                                                [](const std::vector<std::string>& row) {
                                                    return !row.empty() && row.front() == "Cpk";
                                                });
                         }));
    QVERIFY(std::any_of(page.plots.cbegin(), page.plots.cend(),
                        [](const datalab::domain::PlotSpec& plot) {
                            return plot.title.find("累计") != std::string::npos;
                        }));
    QVERIFY(page.facts.capability.has_value());
    QCOMPARE(page.facts.capability->method, std::string("binomial"));
    QVERIFY(page.facts.capability->average_p.has_value());
    QVERIFY(!page.facts.capability->cpk.has_value());
    QCOMPARE(page.facts.capability->assumption_status, std::string("not_verified"));
}

void AttributeCapabilityTest::serviceCountsMissingStarCells()
{
    datalab::domain::DataTable table;
    table.columns = {"缺陷", "单位"};
    table.rows = {{"1", "10"}, {"*", "10"}, {"2", ""}, {"3", "10"}};
    datalab::domain::AnalysisConfiguration configuration;
    configuration.selection.defect_count_column = 0;
    configuration.selection.inspected_count_column = 1;
    const auto page = datalab::application::AnalysisService::poisson_capability(
        table, configuration);
    QCOMPARE(page.method_name, std::string("Poisson Capability"));
    QVERIFY(page.facts.capability.has_value());
    QCOMPARE(page.facts.capability->method, std::string("poisson"));
    QVERIFY(page.facts.capability->mean_dpu.has_value());
    QVERIFY(page.facts.capability->mean_dpu.has_value());
}

void AttributeCapabilityTest::interpretationDoesNotClaimCapability()
{
    datalab::domain::OutputPage page;
    page.method_name = "Binomial Capability";
    datalab::domain::CapabilityFacts facts;
    facts.method = "binomial";
    facts.average_p = 0.04;
    facts.percent_defective = 4.0;
    facts.process_z = 1.75;
    facts.assumption_status = "not_verified";
    page.facts.capability = facts;
    datalab::application::InterpretationService::enrich(page);
    QVERIFY(!page.interpretation.empty());
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
        }
    }
}

QTEST_APPLESS_MAIN(AttributeCapabilityTest)
#include "attribute_capability_test.moc"
