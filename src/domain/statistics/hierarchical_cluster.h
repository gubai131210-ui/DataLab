#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <vector>

namespace datalab::domain::statistics {

struct HierarchicalClusterOptions {
    std::size_t cluster_count = 2;
    bool standardize = false;
};

struct HierarchicalMerge {
    std::size_t step = 0;
    int left_id = 0;
    int right_id = 0;
    int new_id = 0;
    double height = 0.0;
};

struct HierarchicalClusterResult {
    std::size_t observation_count = 0;
    std::size_t variable_count = 0;
    std::size_t cluster_count = 0;
    bool standardized = false;
    std::string linkage = "complete";
    std::vector<std::size_t> valid_rows;
    std::vector<HierarchicalMerge> merges;
    std::vector<std::size_t> assignments;  // 0-based after cut
    std::vector<std::size_t> cluster_sizes;
    std::vector<DiagnosticMessage> diagnostics;
};

HierarchicalClusterResult cluster_observations_complete(
    const std::vector<std::vector<double>>& rows,
    const HierarchicalClusterOptions& options = {});

}  // namespace datalab::domain::statistics
