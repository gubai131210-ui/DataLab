#pragma once

#include "domain/statistics/doe_factorial.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class TaguchiArray {
    L8,   // 2-level, 8 runs, ≤7 factors
    L9,   // 3-level, 9 runs, ≤4 factors
    L12   // 2-level, 12 runs, ≤11 factors
};

struct TaguchiOrthogonalOptions {
    TaguchiArray array = TaguchiArray::L8;
    std::vector<DoeFactor> factors;  // mid_level used for L9 coded 0
    bool randomize = false;
    std::uint64_t random_seed = 1;
};

struct TaguchiOrthogonalDesign {
    TaguchiArray array = TaguchiArray::L8;
    std::string array_name = "L8";
    std::size_t factor_count = 0;
    std::size_t run_count = 0;
    std::size_t levels_per_factor = 2;
    std::size_t max_factors = 7;
    std::vector<DoeFactor> factors;
    std::vector<DoeRun> runs;
    std::string design_kind = "taguchi_orthogonal";
    std::vector<DiagnosticMessage> diagnostics;
};

TaguchiArray parse_taguchi_array(const std::string& text);
std::string taguchi_array_name(TaguchiArray array);
std::size_t taguchi_max_factors(TaguchiArray array);
std::size_t taguchi_levels(TaguchiArray array);

TaguchiOrthogonalDesign generate_taguchi_orthogonal(
    const TaguchiOrthogonalOptions& options);

// Convert to DoeFactorialDesign for doe_design_page / worksheet export.
DoeFactorialDesign taguchi_to_factorial_design(const TaguchiOrthogonalDesign& design);

}  // namespace datalab::domain::statistics
