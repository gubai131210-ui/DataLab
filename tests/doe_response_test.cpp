#include "domain/statistics/doe_factorial.h"
#include "domain/statistics/response_optimization.h"

#include <QtTest/QtTest>

#include <cmath>

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
    void usesLenthPseWhenNoErrorDegreesOfFreedom();
    void evaluatesCodedGridAtCorners();
    void predictionUsesCoefficientCovariance();
    void predictionWithoutCovarianceLeavesIntervalsEmpty();
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
    QCOMPARE(result.t_statistics.size(), result.coefficients.size());
    QCOMPARE(result.standard_errors.size(), result.coefficients.size());
    QCOMPARE(result.pareto_method, std::string{"standardized_t"});
    QVERIFY(result.pareto_reference > 0.0);
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

void DoeResponseTest::usesLenthPseWhenNoErrorDegreesOfFreedom()
{
    // # source: formula_reference — unreplicated 2^2 uses Lenth PSE.
    datalab::domain::statistics::DoeDesignOptions options;
    options.factors = {{"A", "-1", "1"}, {"B", "-1", "1"}};
    options.center_point_count = 0;
    const auto design = datalab::domain::statistics::generate_2_level_factorial(options);
    std::vector<double> responses;
    for (const auto& run : design.runs) {
        const double a = run.coded_levels[0];
        const double b = run.coded_levels[1];
        responses.push_back(10.0 + 4.0 * a + 0.5 * b + 0.2 * a * b);
    }
    const auto result = datalab::domain::statistics::fit_response_analysis(
        design, responses, "Y");
    QCOMPARE(result.residual_degrees_of_freedom, std::size_t{0});
    QCOMPARE(result.pareto_method, std::string{"lenth_pse"});
    QVERIFY(result.lenth_pse > 0.0);
    QVERIFY(result.pareto_reference > 0.0);
    QVERIFY(has_code(result.diagnostics, "lenth_pse_unreplicated"));
}

void DoeResponseTest::evaluatesCodedGridAtCorners()
{
    // # source: formula_reference — ŷ = 10 + 2A + 3B + 4AB
    const auto design = two_factor_design();
    std::vector<double> responses;
    for (const auto& run : design.runs) {
        const double a = run.coded_levels[0];
        const double b = run.coded_levels[1];
        responses.push_back(10.0 + 2.0 * a + 3.0 * b + 4.0 * a * b);
    }
    const auto fit = datalab::domain::statistics::fit_response_analysis(
        design, responses, "Response");
    const auto grid = datalab::domain::statistics::evaluate_coded_grid(
        fit, design, 0, 1, 3);
    QCOMPARE(grid.x.front(), -1.0);
    QCOMPARE(grid.x.back(), 1.0);
    QCOMPARE(grid.y.front(), -1.0);
    QCOMPARE(grid.y.back(), 1.0);
    QCOMPARE(grid.z.size(), std::size_t{3});
    QCOMPARE(grid.z.front().size(), std::size_t{3});
    QVERIFY(std::abs(grid.z.front().front() - (10.0 - 2.0 - 3.0 + 4.0)) < 1.0e-9);
    QVERIFY(std::abs(grid.z.back().back() - (10.0 + 2.0 + 3.0 + 4.0)) < 1.0e-9);
    QVERIFY(has_code(grid.diagnostics, "factorial_contour_no_quadratic"));
}

void DoeResponseTest::predictionUsesCoefficientCovariance()
{
    datalab::domain::statistics::ResponseModel model;
    model.response_name = "Y";
    model.factor_names = {"A", "B"};
    model.intercept = 10.0;
    model.main_effect_coefficients = {2.0, 3.0};
    model.interaction_coefficients = {{"A", "B", 4.0}};
    model.residual_standard_error = 1.0;
    model.residual_degrees_of_freedom = 4.0;
    model.confidence_level = 0.95;
    model.coefficient_covariance = {
        {0.25, 0.0, 0.0, 0.0},
        {0.0, 0.09, 0.0, 0.0},
        {0.0, 0.0, 0.16, 0.0},
        {0.0, 0.0, 0.0, 0.04}};
    const auto prediction = datalab::domain::statistics::predict_response(model, {1, 1});
    QVERIFY(prediction.interval.has_value());
    QCOMPARE(prediction.predicted_value, 19.0);
    QVERIFY(prediction.interval->standard_error > 0.0);
    QVERIFY(prediction.interval->prediction_standard_error
            > prediction.interval->standard_error);
}

void DoeResponseTest::predictionWithoutCovarianceLeavesIntervalsEmpty()
{
    datalab::domain::statistics::ResponseModel model;
    model.response_name = "Y";
    model.factor_names = {"A", "B"};
    model.intercept = 10.0;
    model.main_effect_coefficients = {2.0, 3.0};
    model.interaction_coefficients = {{"A", "B", 4.0}};
    model.residual_standard_error = 1.0;
    model.residual_degrees_of_freedom = 4.0;
    model.confidence_level = 0.95;
    model.observation_count = 0;
    const auto prediction = datalab::domain::statistics::predict_response(model, {1, 1});
    QVERIFY(!prediction.interval.has_value());
    QVERIFY(has_code(prediction.diagnostics, "approximate_confidence_standard_error"));
}

QTEST_APPLESS_MAIN(DoeResponseTest)

#include "doe_response_test.moc"
