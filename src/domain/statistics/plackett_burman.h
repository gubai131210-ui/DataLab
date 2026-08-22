#pragma once

#include "domain/statistics/doe_factorial.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct PlackettBurmanOptions {
    std::vector<DoeFactor> factors;
    std::size_t center_point_count = 0;
    bool randomize = false;
    std::uint64_t random_seed = 0;
};

struct PlackettBurmanDesign {
    std::size_t run_count = 0;
    std::size_t factor_count = 0;
    std::string design_kind = "plackett_burman";
    std::vector<DoeFactor> factors;
    std::vector<DoeRun> runs;
    std::vector<DiagnosticMessage> diagnostics;
};

// Generate Plackett–Burman ±1 design for k factors (N = min 4m ≥ k+1).
PlackettBurmanDesign generate_plackett_burman(const PlackettBurmanOptions& options);

}  // namespace datalab::domain::statistics
