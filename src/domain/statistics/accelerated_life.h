#pragma once
#include "domain/quality_types.h"

#include <cstddef>

#include <optional>

#include <string>

#include <vector>

namespace datalab::domain::statistics {
struct AcceleratedLifeCoefficient {
    std::string term;
    double estimate = 0.0;
    double standard_error = 0.0;
    double z_statistic = 0.0;
    double p_value = 0.0;
    double confidence_lower = 0.0;
    double confidence_upper = 0.0;
};

struct AcceleratedLifePercentile {
    double stress_celsius = 0.0;
    double percentile = 0.0;
    double life = 0.0;
};

struct LifeStressCurvePoint {
    double stress_celsius = 0.0;
    double percentile = 0.0;
    double life = 0.0;
};

struct AcceleratedLifeResult {
    std::size_t observation_count = 0;
    std::size_t failure_count = 0;
    std::size_t censored_count = 0;
    std::size_t stress_level_count = 0;
    bool converged = false;
    std::string transform = "arrhenius";
    std::string distribution = "weibull";
    double intercept = 0.0;
    double slope = 0.0;
    double log_shape = 0.0;
    double shape = 0.0;
    double shape_se = 0.0;
    double log_likelihood = 0.0;
    double use_stress_celsius = 25.0;
    std::optional<double> b10_at_use_stress;
    std::optional<double> b50_at_use_stress;
    std::optional<double> b90_at_use_stress;
    std::vector<AcceleratedLifeCoefficient> coefficients;
    std::vector<AcceleratedLifePercentile> percentiles_at_stress_levels;
    std::vector<AcceleratedLifePercentile> percentiles_at_use_stress;
    std::vector<LifeStressCurvePoint> life_stress_curve;
    std::vector<DiagnosticMessage> diagnostics;
};

double accelerated_life_percentile_at_stress(
    double intercept,
    double slope,
    double shape,
    double stress_celsius,
    double percentile);
AcceleratedLifeResult fit_accelerated_life_weibull_arrhenius(
    const std::vector<double>& times,
    const std::vector<bool>& events,
    const std::vector<double>& stress_celsius,
    const std::vector<std::size_t>& source_rows = {},
    double confidence_level = 0.95,
    double use_stress_celsius = 25.0);
}  // namespace datalab::domain::statistics
