#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct ProbitReliabilityCoefficient {
    std::string term;
    double coefficient = 0.0;
    double standard_error = std::numeric_limits<double>::quiet_NaN();
    double z_statistic = std::numeric_limits<double>::quiet_NaN();
    double p_value = std::numeric_limits<double>::quiet_NaN();
    double confidence_lower = std::numeric_limits<double>::quiet_NaN();
    double confidence_upper = std::numeric_limits<double>::quiet_NaN();
};

struct ProbitReliabilityObservation {
    double stress = 0.0;
    std::size_t events = 0;
    std::size_t trials = 0;
    double proportion = 0.0;
    double fitted_probability = 0.0;
    double linear_predictor = 0.0;
    double pearson_residual = 0.0;
};

struct ProbitReliabilityResult {
    std::size_t observation_count = 0;
    std::size_t iteration_count = 0;
    bool converged = false;
    std::string link = "logit";
    std::vector<ProbitReliabilityCoefficient> coefficients;
    std::vector<ProbitReliabilityObservation> observations;
    double log_likelihood = std::numeric_limits<double>::quiet_NaN();
    double deviance = std::numeric_limits<double>::quiet_NaN();
    double aic = std::numeric_limits<double>::quiet_NaN();
    std::optional<double> ld50;
    std::optional<double> ld50_standard_error;
    std::optional<double> ld50_confidence_lower;
    std::optional<double> ld50_confidence_upper;
    std::vector<DiagnosticMessage> diagnostics;
};

ProbitReliabilityResult fit_probit_reliability(
    const std::vector<std::size_t>& events,
    const std::vector<std::size_t>& trials,
    const std::vector<double>& stress,
    double confidence_level = 0.95,
    std::size_t max_iterations = 100,
    double tolerance = 1.0e-8);

}  // namespace datalab::domain::statistics
