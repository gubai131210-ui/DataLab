#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct ClusterVariablesOptions {
    std::string linkage = "complete";  // complete | average | single
    bool use_absolute_correlation = true;
};

struct ClusterVariablesMerge {
    std::size_t step = 0;
    int left_id = 0;
    int right_id = 0;
    int new_id = 0;
    double height = 0.0;
    double similarity = 0.0;
    std::string left_label;
    std::string right_label;
};

struct ClusterVariablesResult {
    std::size_t observation_count = 0;
    std::size_t variable_count = 0;
    std::string linkage;
    double max_distance = 0.0;
    std::vector<std::string> variable_labels;
    std::vector<ClusterVariablesMerge> merges;
    std::vector<std::vector<double>> correlation_matrix;
    std::vector<std::vector<double>> distance_matrix;
    std::vector<DiagnosticMessage> diagnostics;
    std::string algorithm_id = "cluster_variables_corr_hclust";
    std::string evidence_type = "formula_reference";
};

// Rows = observations, columns = variables (numeric).
ClusterVariablesResult cluster_variables_analyze(
    const std::vector<std::vector<double>>& data,
    const std::vector<std::string>& variable_labels = {},
    const ClusterVariablesOptions& options = {});

}  // namespace datalab::domain::statistics
