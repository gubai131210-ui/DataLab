#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct LifeDataLognormalOptions {
    double confidence_level = 0.95;
    std::vector<double> percentile_levels = {1.0, 5.0, 50.0, 95.0, 99.0};
};

struct LifeDataLognormalCoefficient {
    std::string term;
    double estimate = 0.0;
    double standard_error = 0.0;
    double z_statistic = 0.0;
    double p_value = 0.0;
    double confidence_lower = 0.0;
    double confidence_upper = 0.0;
};

struct LifeDataLognormalPercentile {
    double percentile = 0.0;
    double life = 0.0;
    std::string covariate_profile;
};

struct LifeDataLognormalResult {
    std::size_t observation_count = 0;
    std::size_t failure_count = 0;
    std::size_t censored_count = 0;
    std::size_t covariate_count = 0;
    bool converged = false;
    double log_sigma = 0.0;
    double log_likelihood = 0.0;
    std::vector<LifeDataLognormalCoefficient> coefficients;
    std::vector<LifeDataLognormalPercentile> percentiles;
    std::vector<std::size_t> observation_source_rows;
    std::string algorithm_id = "life_data_lognormal_mle";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

LifeDataLognormalResult fit_life_data_lognormal(
    const std::vector<double>& times,
    const std::vector<bool>& events,
    const std::vector<std::vector<double>>& covariates,
    const std::vector<std::string>& covariate_labels = {},
    const std::vector<std::size_t>& source_rows = {},
    const LifeDataLognormalOptions& options = {});

}  // namespace datalab::domain::statistics
