#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <vector>

namespace datalab::domain::statistics {

struct TPowerResult {
    std::size_t sample_size = 0;
    double power = 0.0;
    double effect_size = 0.0;
    // Extended fields are populated by the exact non-central t calculation.
    // sample_size remains the historical per-test value: total n for one
    // sample and n per group for two samples.
    std::size_t sample_size_per_group = 0;
    std::size_t total_sample_size = 0;
    double degrees_of_freedom = 0.0;
    double noncentrality_parameter = 0.0;
    double critical_value = 0.0;
    std::vector<DiagnosticMessage> diagnostics;
};

enum class PowerAlternative {
    two_sided,
    greater,
    less
};

enum class ProportionVarianceMethod {
    pooled,
    unpooled
};

using PowerResult = TPowerResult;

TPowerResult one_sample_t_power(std::size_t sample_size, double effect_size,
                                double alpha = 0.05);
TPowerResult two_sample_t_power(std::size_t sample_size_per_group, double effect_size,
                                double alpha = 0.05);
TPowerResult one_sample_t_sample_size(double effect_size, double target_power = 0.8,
                                      double alpha = 0.05);
TPowerResult two_sample_t_sample_size(double effect_size, double target_power = 0.8,
                                      double alpha = 0.05);

PowerResult one_way_anova_power(std::size_t sample_size_per_group, std::size_t group_count,
                                double effect_size, double alpha = 0.05);
PowerResult one_way_anova_sample_size(std::size_t group_count, double effect_size,
                                      double target_power = 0.8, double alpha = 0.05);

PowerResult one_sample_proportion_power(std::size_t sample_size, double null_proportion,
                                        double alternative_proportion,
                                        double alpha = 0.05,
                                        PowerAlternative alternative =
                                            PowerAlternative::two_sided);
PowerResult one_sample_proportion_sample_size(
    double null_proportion, double alternative_proportion, double target_power = 0.8,
    double alpha = 0.05, PowerAlternative alternative = PowerAlternative::two_sided);

PowerResult two_proportion_power(
    std::size_t sample_size_per_group, double first_proportion, double second_proportion,
    double alpha = 0.05, PowerAlternative alternative = PowerAlternative::two_sided,
    ProportionVarianceMethod variance_method = ProportionVarianceMethod::pooled);
PowerResult two_proportion_sample_size(
    double first_proportion, double second_proportion, double target_power = 0.8,
    double alpha = 0.05, PowerAlternative alternative = PowerAlternative::two_sided,
    ProportionVarianceMethod variance_method = ProportionVarianceMethod::pooled);

PowerResult one_variance_power(
    std::size_t sample_size, double standard_deviation_ratio, double alpha = 0.05,
    PowerAlternative alternative = PowerAlternative::two_sided);
PowerResult one_variance_sample_size(
    double standard_deviation_ratio, double target_power = 0.8, double alpha = 0.05,
    PowerAlternative alternative = PowerAlternative::two_sided);

PowerResult two_variance_power(
    std::size_t sample_size_per_group, double standard_deviation_ratio, double alpha = 0.05,
    PowerAlternative alternative = PowerAlternative::two_sided);
PowerResult two_variance_sample_size(
    double standard_deviation_ratio, double target_power = 0.8, double alpha = 0.05,
    PowerAlternative alternative = PowerAlternative::two_sided);

PowerResult one_poisson_rate_power(
    std::size_t sample_size, double null_rate, double comparison_rate,
    double observation_length = 1.0, double alpha = 0.05,
    PowerAlternative alternative = PowerAlternative::two_sided);
PowerResult one_poisson_rate_sample_size(
    double null_rate, double comparison_rate, double target_power = 0.8,
    double observation_length = 1.0, double alpha = 0.05,
    PowerAlternative alternative = PowerAlternative::two_sided);

PowerResult two_poisson_rate_power(
    std::size_t sample_size_per_group, double first_rate, double second_rate,
    double observation_length = 1.0, double alpha = 0.05,
    PowerAlternative alternative = PowerAlternative::two_sided);
PowerResult two_poisson_rate_sample_size(
    double first_rate, double second_rate, double target_power = 0.8,
    double observation_length = 1.0, double alpha = 0.05,
    PowerAlternative alternative = PowerAlternative::two_sided);

// Equivalence TOST power (difference / σ units; known-σ normal planning approx).
PowerResult equivalence_one_sample_power(
    std::size_t sample_size, double lower_limit, double upper_limit,
    double true_difference = 0.0, double alpha = 0.05);
PowerResult equivalence_one_sample_sample_size(
    double lower_limit, double upper_limit, double target_power = 0.8,
    double true_difference = 0.0, double alpha = 0.05);
PowerResult equivalence_two_sample_power(
    std::size_t sample_size_per_group, double lower_limit, double upper_limit,
    double true_difference = 0.0, double alpha = 0.05);
PowerResult equivalence_two_sample_sample_size(
    double lower_limit, double upper_limit, double target_power = 0.8,
    double true_difference = 0.0, double alpha = 0.05);

// 2-level factorial design power (effect = |ȳ₊-ȳ₋|/σ).
PowerResult doe_factorial_power(
    std::size_t factor_count, std::size_t fraction_p, std::size_t replicates,
    double effect_over_sigma, double alpha = 0.05);
PowerResult doe_factorial_sample_size(
    std::size_t factor_count, std::size_t fraction_p, double effect_over_sigma,
    double target_power = 0.8, double alpha = 0.05);

// Normal (Howe) tolerance interval: minimal n with k(n) ≤ max_k.
PowerResult tolerance_normal_sample_size(
    double coverage = 0.95, double confidence_level = 0.95,
    double max_k_factor = 4.0);

}  // namespace datalab::domain::statistics
