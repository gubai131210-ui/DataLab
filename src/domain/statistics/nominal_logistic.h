#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct NominalCoefficient {
    std::string logit_label;
    std::string term;
    double estimate = 0.0;
    double standard_error = 0.0;
    double z_statistic = 0.0;
    double p_value = 0.0;
    double odds_ratio = 0.0;
    double confidence_lower = 0.0;
    double confidence_upper = 0.0;
};

struct NominalLogisticResult {
    std::size_t observation_count = 0;
    std::size_t predictor_count = 0;
    std::size_t category_count = 0;
    std::size_t logit_count = 0;
    std::size_t iteration_count = 0;
    bool converged = false;
    double log_likelihood = 0.0;
    double aic = 0.0;
    double g_statistic = 0.0;
    double g_df = 0.0;
    double g_p_value = 0.0;
    std::string reference_category;
    std::vector<std::string> category_labels;
    std::vector<NominalCoefficient> coefficients;
    std::vector<DiagnosticMessage> diagnostics;
};

// response: category indices 0..K-1; reference = K-1 (last label).
NominalLogisticResult fit_nominal_logistic(
    const std::vector<std::size_t>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& category_labels,
    const std::vector<std::string>& predictor_labels = {},
    std::size_t max_iterations = 50,
    double tolerance = 1.0e-6,
    double confidence_level = 0.95);

}  // namespace datalab::domain::statistics
