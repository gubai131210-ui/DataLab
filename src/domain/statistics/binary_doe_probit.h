#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct BinaryDoeProbitOptions {
    std::string link = "probit";  // probit | gompit
    bool include_ab_interaction = true;
};

struct BinaryDoeProbitCoefficient {
    std::string term;
    double coefficient = 0.0;
    double standard_error = 0.0;
    double z_statistic = 0.0;
    double p_value = 0.0;
};

struct BinaryDoeProbitResult {
    std::size_t design_row_count = 0;
    std::size_t expanded_observation_count = 0;
    std::size_t factor_count = 0;
    std::size_t event_count = 0;
    std::size_t trial_count = 0;
    std::string link = "probit";
    bool include_ab_interaction = true;
    bool converged = false;
    std::size_t iteration_count = 0;
    double deviance = 0.0;
    double aic = 0.0;
    std::vector<BinaryDoeProbitCoefficient> coefficients;
    std::vector<std::size_t> observation_source_rows;
    std::string algorithm_id = "binary_doe_probit_irwls";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

BinaryDoeProbitResult analyze_binary_doe_probit(
    const std::vector<std::vector<std::string>>& factor_columns,
    const std::vector<int>& events,
    const std::vector<int>& trials,
    const std::vector<std::string>& factor_labels = {},
    const std::vector<std::size_t>& source_rows = {},
    const BinaryDoeProbitOptions& options = {});

}  // namespace datalab::domain::statistics
