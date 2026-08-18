#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/hypothesis_tests.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace datalab::domain::statistics {

// Result of testing one population variance against a hypothesized value.
struct OneVarianceTestResult {
    std::size_t count = 0;
    double sample_variance = 0.0;
    double sample_standard_deviation = 0.0;
    double hypothesized_variance = 0.0;
    double chi_square_statistic = 0.0;
    double degrees_of_freedom = 0.0;
    double confidence_level = 0.95;
    std::optional<double> p_value;
    std::optional<double> p_value_less;
    std::optional<double> p_value_greater;
    std::optional<double> p_value_two_sided;
    std::optional<double> confidence_lower;
    std::optional<double> confidence_upper;
    std::vector<DiagnosticMessage> diagnostics;
};

// Result of the classical F test for the ratio of two independent variances.
struct TwoVarianceFTestResult {
    std::size_t first_count = 0;
    std::size_t second_count = 0;
    double first_variance = 0.0;
    double second_variance = 0.0;
    double variance_ratio = 0.0;
    double f_statistic = 0.0;
    double numerator_degrees_of_freedom = 0.0;
    double denominator_degrees_of_freedom = 0.0;
    double confidence_level = 0.95;
    std::optional<double> p_value;
    std::optional<double> p_value_less;
    std::optional<double> p_value_greater;
    std::optional<double> p_value_two_sided;
    std::optional<double> confidence_lower;
    std::optional<double> confidence_upper;
    std::vector<DiagnosticMessage> diagnostics;
};

enum class VarianceRobustMethod {
    levene_mean,
    brown_forsythe_median
};

// Result of Levene's or Brown-Forsythe's test across two or more groups.
struct LeveneTestResult {
    VarianceRobustMethod method = VarianceRobustMethod::levene_mean;
    std::size_t group_count = 0;
    std::size_t total_count = 0;
    double f_statistic = 0.0;
    double numerator_degrees_of_freedom = 0.0;
    double denominator_degrees_of_freedom = 0.0;
    double confidence_level = 0.95;
    std::optional<double> p_value;
    std::optional<double> p_value_less;
    std::optional<double> p_value_greater;
    std::optional<double> p_value_two_sided;
    std::optional<double> confidence_lower;
    std::optional<double> confidence_upper;
    std::vector<DiagnosticMessage> diagnostics;
};

OneVarianceTestResult chi_square_one_variance_test(
    const std::vector<double>& observations,
    double hypothesized_variance,
    double confidence_level = 0.95,
    TestAlternative alternative = TestAlternative::two_sided);

TwoVarianceFTestResult f_test_two_variances(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double confidence_level = 0.95,
    TestAlternative alternative = TestAlternative::two_sided);

LeveneTestResult levene_two_variances(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double confidence_level = 0.95,
    TestAlternative alternative = TestAlternative::two_sided);

LeveneTestResult levene_mean_two_variances(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double confidence_level = 0.95,
    TestAlternative alternative = TestAlternative::two_sided);

LeveneTestResult brown_forsythe_two_variances(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double confidence_level = 0.95,
    TestAlternative alternative = TestAlternative::two_sided);

LeveneTestResult levene_k_groups(
    const std::vector<std::vector<double>>& groups,
    double confidence_level = 0.95,
    VarianceRobustMethod method = VarianceRobustMethod::brown_forsythe_median);

}  // namespace datalab::domain::statistics

