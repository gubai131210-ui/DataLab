#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/hypothesis_tests.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct EquivalenceTestResult {
    std::string kind;
    std::size_t first_count = 0;
    std::size_t second_count = 0;
    double first_mean = 0.0;
    double second_mean = 0.0;
    double first_standard_deviation = 0.0;
    double second_standard_deviation = 0.0;
    double difference = 0.0;
    double standard_error = 0.0;
    double degrees_of_freedom = 0.0;
    double lower = 0.0;
    double upper = 0.0;
    double t_lower = 0.0;
    double t_upper = 0.0;
    std::optional<double> p_lower;
    std::optional<double> p_upper;
    std::optional<double> confidence_lower;
    std::optional<double> confidence_upper;
    double alpha = 0.05;
    double confidence_level = 0.95;
    std::string ci_method = "tost_1_minus_alpha";
    bool within_limits = false;
    bool both_pvalues_below_alpha = false;
    VarianceMethod variance_method = VarianceMethod::welch;
    std::vector<DiagnosticMessage> diagnostics;
};

EquivalenceTestResult one_sample_equivalence_test(
    const std::vector<double>& observations,
    double target,
    double lower,
    double upper,
    double confidence_level = 0.95);

EquivalenceTestResult two_sample_equivalence_test(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double lower,
    double upper,
    double confidence_level = 0.95,
    VarianceMethod variance_method = VarianceMethod::welch);

// Ratio TOST for μ_test / μ_reference (non-log). Bounds are on the ratio scale.
// EquivalenceTestResult::difference holds ρ̂; kind = "two_sample_ratio".
EquivalenceTestResult two_sample_equivalence_ratio_test(
    const std::vector<double>& test_sample,
    const std::vector<double>& reference_sample,
    double lower,
    double upper,
    double confidence_level = 0.95,
    VarianceMethod variance_method = VarianceMethod::welch);

EquivalenceTestResult paired_equivalence_test(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double lower,
    double upper,
    double confidence_level = 0.95);

// Wald z-TOST for a single binomial proportion versus target p0.
// SE uses the sample proportion; CI is 100(1−2α)% Wald.
EquivalenceTestResult one_proportion_equivalence_test(
    std::size_t events,
    std::size_t trials,
    double hypothesized,
    double lower,
    double upper,
    double confidence_level = 0.95,
    std::size_t row_count = 1,
    std::size_t missing_count = 0);

// Wald z-TOST for the difference of two independent binomial proportions.
EquivalenceTestResult two_proportion_equivalence_test(
    std::size_t first_events,
    std::size_t first_trials,
    std::size_t second_events,
    std::size_t second_trials,
    double lower,
    double upper,
    double confidence_level = 0.95,
    std::size_t first_row_count = 1,
    std::size_t second_row_count = 1,
    std::size_t first_missing = 0,
    std::size_t second_missing = 0);

}  // namespace datalab::domain::statistics
