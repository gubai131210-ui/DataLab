#pragma once

// Gray-style chi-square test for group comparison on competing-risk CIF.
// formula_reference / gray_cif_group_test — not a Minitab menu clone.

#include "domain/quality_types.h"
#include "domain/statistics/censoring_contract.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct GrayTestResult {
    std::optional<double> chi_square;
    std::optional<double> df;
    std::optional<double> p_value;
    std::size_t group_count = 0;
    std::size_t mode_count = 0;
    bool ran = false;
    std::string not_computed_reason;
    std::vector<DiagnosticMessage> diagnostics;
};

// Narrow gate: group labels + >=2 failure modes + each group has >=1 labeled exact failure.
// Exact + right censoring only; left/interval omitted.
GrayTestResult gray_test_cif(const std::vector<CensoringObservation>& observations);

}  // namespace datalab::domain::statistics
