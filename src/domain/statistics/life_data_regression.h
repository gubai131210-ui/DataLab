#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct LifeDataRegressionOptions {
    std::vector<double> percentile_levels = {1.0, 5.0};
    double confidence_level = 0.95;
};

struct LifeDataRegressionCoefficient {
    std::string term;
    double estimate = 0.0;
    double standard_error = 0.0;
    double z_statistic = 0.0;
    double p_value = 0.0;
    double confidence_lower = 0.0;
    double confidence_upper = 0.0;
};

struct LifeDataRegressionPercentile {
    double percentile = 0.0;
    double life = 0.0;
    std::string covariate_profile;
};

struct LifeDataRegressionResult {
    std::size_t observation_count = 0;
    std::size_t failure_count = 0;
    std::size_t censored_count = 0;
    std::size_t covariate_count = 0;
    bool converged = false;
    std::string distribution = "weibull";
    double shape = 0.0;
    double log_likelihood = 0.0;
    std::vector<LifeDataRegressionCoefficient> coefficients;
    std::vector<LifeDataRegressionPercentile> percentiles;
    std::vector<DiagnosticMessage> diagnostics;
    std::string algorithm_id = "life_data_regression_weibull_mle";
    std::string evidence_type = "formula_reference";
};

LifeDataRegressionResult fit_life_data_regression_weibull(
    const std::vector<double>& times,
    const std::vector<bool>& events,
    const std::vector<std::vector<double>>& covariates,
    const std::vector<std::string>& covariate_labels = {},
    const std::vector<std::size_t>& source_rows = {},
    const LifeDataRegressionOptions& options = {});

}  // namespace datalab::domain::statistics
