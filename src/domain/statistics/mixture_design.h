#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct MixtureDesignOptions {
    std::size_t component_count = 3;  // q = 3..4
    std::vector<std::string> component_names;
    bool randomize = false;
    std::uint64_t random_seed = 1;
};

struct MixtureRun {
    std::size_t standard_order = 0;
    std::size_t run_order = 0;
    std::vector<double> proportions;  // x1..xq, sum = 1
};

struct MixtureDesign {
    std::size_t component_count = 0;  // q
    std::size_t degree = 2;          // m
    std::size_t run_count = 0;       // N = q(q+1)/2
    std::string design_kind = "simplex_lattice";
    std::vector<std::string> component_names;
    std::vector<MixtureRun> runs;
    std::string algorithm_id = "mixture_simplex_lattice_m2";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

// Simplex-lattice {q, m=2}: vertices + edge midpoints; N = q(q+1)/2.
MixtureDesign generate_mixture_simplex_lattice(const MixtureDesignOptions& options);

}  // namespace datalab::domain::statistics
