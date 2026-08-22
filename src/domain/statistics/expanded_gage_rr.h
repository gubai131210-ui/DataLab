#pragma once

#include "domain/statistics/gage_rr.h"

#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct ExpandedGageRrResult {
    GageRrResult gage;
    std::string additional_factor_name = "Additional";
    std::size_t additional_level_count = 0;
    bool three_factor_model = false;
    std::string method = "expanded_3factor_balanced_random";
};

// Balanced crossed Part × Operator × Additional (all random) with replicates.
// Three-way interaction pooled into repeatability when replicates == 1 is diagnosed.
ExpandedGageRrResult expanded_gage_rr_three_factor(
    const std::vector<double>& measurements,
    const std::vector<std::string>& parts,
    const std::vector<std::string>& operators,
    const std::vector<std::string>& additional,
    double tolerance = 0.0,
    const std::string& additional_factor_name = "Additional");

}  // namespace datalab::domain::statistics
