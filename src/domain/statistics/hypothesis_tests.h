#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class TestAlternative {
    two_sided,
    less,
    greater
};

enum class VarianceMethod {
    welch,
    pooled
};

struct TTestResult {
    std::size_t count = 0;
    double mean = 0.0;
    double sample_standard_deviation = 0.0;
    double standard_error = 0.0;
    double hypothesized_mean = 0.0;
    double difference = 0.0;
    double t_statistic = 0.0;
    double degrees_of_freedom = 0.0;
    std::optional<double> p_value;
    std::optional<double> confidence_lower;
    std::optional<double> confidence_upper;
    double confidence_level = 0.95;
    std::vector<DiagnosticMessage> diagnostics;
};

struct TwoSampleTTestResult {
    TTestResult first;
    TTestResult second;
    double mean_difference = 0.0;
    double standard_error_difference = 0.0;
    double t_statistic = 0.0;
    double degrees_of_freedom = 0.0;
    std::optional<double> p_value;
    std::optional<double> confidence_lower;
    std::optional<double> confidence_upper;
    double confidence_level = 0.95;
    VarianceMethod variance_method = VarianceMethod::welch;
    std::vector<DiagnosticMessage> diagnostics;
};

struct AnovaGroupSummary {
    std::string label;
    std::size_t count = 0;
    double mean = 0.0;
    double sample_standard_deviation = 0.0;
};

struct AnovaResult {
    std::vector<AnovaGroupSummary> groups;
    std::size_t total_count = 0;
    double grand_mean = 0.0;
    double between_sum_of_squares = 0.0;
    double error_sum_of_squares = 0.0;
    double total_sum_of_squares = 0.0;
    std::size_t between_degrees_of_freedom = 0;
    std::size_t error_degrees_of_freedom = 0;
    std::size_t total_degrees_of_freedom = 0;
    double between_mean_square = 0.0;
    double error_mean_square = 0.0;
    double f_statistic = 0.0;
    std::optional<double> p_value;
    QualityEvidence evidence;
    std::vector<AssumptionCheck> assumptions;
    std::vector<RuleEvidence> rules;
    std::vector<double> residuals;
    std::vector<double> fitted;
    std::optional<double> levene_p_value;
    std::optional<double> residual_normality_p;
    std::vector<DiagnosticMessage> diagnostics;
};

TTestResult one_sample_t_test(
    const std::vector<double>& observations,
    double hypothesized_mean,
    double confidence_level = 0.95,
    TestAlternative alternative = TestAlternative::two_sided);

struct ZTestResult {
    std::size_t count = 0;
    double mean = 0.0;
    double known_sigma = 0.0;
    double standard_error = 0.0;
    double hypothesized_mean = 0.0;
    double difference = 0.0;
    double z_statistic = 0.0;
    std::optional<double> p_value;
    std::optional<double> confidence_lower;
    std::optional<double> confidence_upper;
    double confidence_level = 0.95;
    std::vector<DiagnosticMessage> diagnostics;
};

ZTestResult one_sample_z_test(
    const std::vector<double>& observations,
    double hypothesized_mean,
    double known_sigma,
    double confidence_level = 0.95,
    TestAlternative alternative = TestAlternative::two_sided);

TwoSampleTTestResult two_sample_t_test(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double confidence_level = 0.95,
    TestAlternative alternative = TestAlternative::two_sided,
    VarianceMethod variance_method = VarianceMethod::welch);

AnovaResult one_way_anova(
    const std::vector<std::vector<double>>& groups,
    const std::vector<std::string>& labels = {});

double student_t_cdf(double value, double degrees_of_freedom);
double student_t_quantile(double probability, double degrees_of_freedom);
double f_right_tail(double value, double numerator_df, double denominator_df);

}  // namespace datalab::domain::statistics
