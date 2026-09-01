#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct SplitPlotAnalyzeOptions {
    bool include_htc_etc_interaction = true;
    bool include_etc_interaction = true;
};

struct SplitPlotAnovaEffect {
    std::string term;
    std::string error_layer;  // WP | SP
    std::optional<double> sum_of_squares;
    std::size_t degrees_of_freedom = 0;
    std::optional<double> mean_square;
    std::optional<double> f_statistic;
    std::optional<double> p_value;
};

struct SplitPlotFitRow {
    std::size_t source_row = 0;
    double observed = 0.0;
    double fitted = 0.0;
    double residual = 0.0;
    double whole_plot_residual = 0.0;
    std::string whole_plot_id;
};

struct SplitPlotAnalyzeResult {
    std::size_t observation_count = 0;
    std::size_t whole_plot_count = 0;
    bool include_htc_etc_interaction = true;
    bool include_etc_interaction = true;
    double wp_r_squared = 0.0;
    double sp_r_squared = 0.0;
    std::vector<SplitPlotAnovaEffect> anova_effects;
    std::vector<SplitPlotFitRow> fits;
    std::vector<std::size_t> observation_source_rows;
    std::string algorithm_id = "split_plot_analyze_wp_sp";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

SplitPlotAnalyzeResult split_plot_analyze(
    const std::vector<double>& response,
    const std::vector<std::string>& htc_factor,
    const std::vector<std::string>& etc_factor_a,
    const std::vector<std::string>& whole_plot_ids,
    const std::vector<std::string>& etc_factor_b = {},
    const std::vector<std::size_t>& source_rows = {},
    const SplitPlotAnalyzeOptions& options = {});

}  // namespace datalab::domain::statistics
