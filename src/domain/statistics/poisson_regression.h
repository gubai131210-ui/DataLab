#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct PoissonCoefficient {
    std::string term;
    double coefficient = 0.0;
    double standard_error = 0.0;
    double z_statistic = 0.0;
    double p_value = 0.0;
    double confidence_lower = 0.0;
    double confidence_upper = 0.0;
};

struct PoissonObservation {
    double response = 0.0;
    double fitted = 0.0;
    double linear_predictor = 0.0;
    double pearson_residual = 0.0;
};

struct PoissonRegressionResult {
    std::size_t observation_count = 0;
    std::size_t predictor_count = 0;
    std::size_t iteration_count = 0;
    bool converged = false;
    double log_likelihood = 0.0;
    double deviance = 0.0;
    double aic = 0.0;
    std::vector<PoissonCoefficient> coefficients;
    std::vector<PoissonObservation> observations;
    std::vector<DiagnosticMessage> diagnostics;
};

PoissonRegressionResult fit_poisson_regression(
    const std::vector<double>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& predictor_labels = {},
    double confidence_level = 0.95,
    std::size_t max_iterations = 100,
    double tolerance = 1.0e-8);

}  // namespace datalab::domain::statistics
