#include "domain/statistics/doe_factorial.h"

#include <QtTest/QtTest>

namespace {

bool has_code(
    const std::vector<datalab::domain::DiagnosticMessage>& diagnostics,
    const std::string& code)
{
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

datalab::domain::statistics::DoeFactorialDesign two_factor_design()
{
    datalab::domain::statistics::DoeDesignOptions options;
    options.factors = {{"A", "-1", "1"}, {"B", "-1", "1"}};
    options.center_point_count = 2;
    return datalab::domain::statistics::generate_2_level_factorial(options);
}

}  // namespace

class DoeResponseTest final : public QObject {
    Q_OBJECT

private slots:
    void fitsMainAndInteractionEffects();
    void includesBlocksAndSeparatesErrorSources();
    void summarizesCentersAndTestsCurvature();
    void diagnosesMissingAndDuplicateRuns();
    void rejectsRankDeficientDesign();
};

void DoeResponseTest::fitsMainAndInteractionEffects()
{
    const auto design = two_factor_design();
    std::vector<double> responses;
    for (const auto& run : design.runs) {
        const double a = run.coded_levels[0];
        const double b = run.coded_levels[1];
        responses.push_back(10.0 + 2.0 * a + 3.0 * b + 4.0 * a * b);
    }
    const auto result = datalab::domain::statistics::fit_response_analysis(
        design, responses, "Response");
    QVERIFY(!has_code(result.diagnostics, "rank_deficient_design"));
    QCOMPARE(result.term_names.size(), std::size_t{4});
    QCOMPARE(result.coefficients.size(), std::size_t{4});
    QCOMPARE(result.coefficients[0], 10.0);
    QCOMPARE(result.coefficients[1], 2.0);
    QCOMPARE(result.coefficients[2], 3.0);
    QCOMPARE(result.coefficients[3], 4.0);
    QCOMPARE(result.anova_rows.size(), std::size_t{5});
    QCOMPARE(result.residuals.size(), responses.size());
}

void DoeResponseTest::includesBlocksAndSeparatesErrorSources()
{
    datalab::domain::statistics::DoeDesignOptions options;
    options.factors = {
        {"A", "-1", "1"}, {"B", "-1", "1"}, {"C", "-1", "1"}};
    options.center_point_count = 4;
    options.block_count = 2;
    auto design = datalab::domain::statistics::generate_2_level_factorial(options);
    for (std::size_t index = 0; index < design.runs.size(); ++index) {
        design.runs[index].block = (index % 4 == 0 || index % 4 == 3) ? 1 : 2;
    }
    std::vector<double> responses;
    for (const auto& run : design.runs) {
        const double a = run.coded_levels[0];
        const double b = run.coded_levels[1];
        const double c = run.coded_levels[2];
        responses.push_back(20.0 + 2.0 * a + 3.0 * b + 4.0 * c
            + 5.0 * a * b + (run.block == 2 ? 1.5 : 0.0));
    }
    const auto result = datalab::domain::statistics::fit_response_analysis(
        design, responses);
    QVERIFY(has_code(result.diagnostics, "block_terms_included"));
    QCOMPARE(result.block_anova_rows.size(), std::size_t{1});
    QVERIFY(result.pure_error_anova_row.has_value());
    QVERIFY(result.lack_of_fit_anova_row.has_value());
    QCOMPARE(result.anova_rows.size(), std::size_t{9});
}

void DoeResponseTest::summarizesCentersAndTestsCurvature()
{
    const auto design = two_factor_design();
    std::vector<double> responses;
    for (std::size_t index = 0; index < design.runs.size(); ++index) {
        const auto& run = design.runs[index];
        if (run.center_point) {
            responses.push_back(index == 4 ? 11.0 : 13.0);
        } else {
            responses.push_back(10.0 + 2.0 * run.coded_levels[0]
                + 3.0 * run.coded_levels[1]);
        }
    }
    const auto result = datalab::domain::statistics::fit_response_analysis(
        design, responses);
    QCOMPARE(result.center_points.count, std::size_t{2});
    QCOMPARE(result.center_points.mean, 12.0);
    QCOMPARE(result.center_points.degrees_of_freedom, std::size_t{1});
    QVERIFY(result.curvature.available);
    QCOMPARE(result.curvature.factorial_mean, 10.0);
    QCOMPARE(result.curvature.center_mean, 12.0);
    QCOMPARE(result.curvature.degrees_of_freedom, std::size_t{1});
    QVERIFY(result.curvature.p_value.has_value());
}

void DoeResponseTest::diagnosesMissingAndDuplicateRuns()
{
    auto design = two_factor_design();
    design.runs.resize(3);
    std::vector<double> responses(design.runs.size(), 1.0);
    const auto missing_result = datalab::domain::statistics::fit_response_analysis(
        design, responses);
    QVERIFY(has_code(missing_result.diagnostics, "missing_doe_runs"));

    design = two_factor_design();
    design.runs[1] = design.runs[0];
    responses.assign(design.runs.size(), 1.0);
    const auto duplicate_result = datalab::domain::statistics::fit_response_analysis(
        design, responses);
    QVERIFY(has_code(duplicate_result.diagnostics, "duplicate_doe_runs"));
    QVERIFY(has_code(duplicate_result.diagnostics, "replicated_doe_runs"));
}

void DoeResponseTest::rejectsRankDeficientDesign()
{
    datalab::domain::statistics::DoeFactorialDesign design;
    design.factors = {{"A", "-1", "1"}, {"B", "-1", "1"}};
    for (int index = 0; index < 6; ++index) {
        design.runs.push_back({static_cast<std::size_t>(index),
            static_cast<std::size_t>(index), 1, false, {-1, -1}});
    }
    const auto result = datalab::domain::statistics::fit_response_analysis(
        design, std::vector<double>(6, 1.0));
    QVERIFY(has_code(result.diagnostics, "rank_deficient_design"));
}

QTEST_APPLESS_MAIN(DoeResponseTest)

#include "doe_response_test.moc"
