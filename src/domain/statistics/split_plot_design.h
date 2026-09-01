#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/doe_factorial.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct SplitPlotDesignOptions {
    std::vector<DoeFactor> factors;
    std::size_t htc_factor_index = 0;
    std::size_t whole_plot_replicates = 1;
    bool randomize = true;
    std::uint64_t random_seed = 1;
    std::size_t etc_fraction_p = 0;
};

struct SplitPlotDesignRun {
    std::size_t standard_order = 0;
    std::size_t run_order = 0;
    std::size_t whole_plot = 1;
    std::size_t block = 1;
    std::string point_type = "Factorial";
    std::vector<int> coded_levels;
    std::vector<std::string> factor_levels;
};

struct SplitPlotDesignResult {
    std::vector<DoeFactor> factors;
    std::vector<std::string> factor_names;
    std::size_t factor_count = 0;
    std::size_t htc_factor_index = 0;
    std::string htc_factor_name;
    std::vector<SplitPlotDesignRun> runs;
    std::size_t whole_plot_count = 0;
    std::size_t etc_run_count = 0;
    std::size_t run_count = 0;
    bool randomized = false;
    std::uint64_t random_seed = 0;
    std::string algorithm_id = "split_plot_design_2level";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

SplitPlotDesignResult generate_split_plot_design(const SplitPlotDesignOptions& options);

}  // namespace datalab::domain::statistics
