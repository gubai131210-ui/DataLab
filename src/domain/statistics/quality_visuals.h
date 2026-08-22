#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace datalab::domain::statistics {

struct BoxPlotSummary {
    double minimum = 0.0;
    double first_quartile = 0.0;
    double median = 0.0;
    double third_quartile = 0.0;
    double maximum = 0.0;
    double iqr = 0.0;
    double whisker_low = 0.0;
    double whisker_high = 0.0;
    std::size_t count = 0;
    std::vector<double> outliers;
};

struct ParetoItem {
    std::string category;
    std::size_t count = 0;
    double percent = 0.0;
    double cumulative_percent = 0.0;
};

struct ParetoOptions {
    std::optional<double> other_threshold_percent;
};

struct HistogramResult {
    std::vector<double> edges;
    std::vector<double> counts;
};

BoxPlotSummary box_plot_summary(const std::vector<double>& observations);

std::vector<ParetoItem> pareto(
    const std::vector<std::pair<std::string, std::size_t>>& counts);

std::vector<ParetoItem> pareto(
    const std::vector<std::pair<std::string, std::size_t>>& counts,
    const ParetoOptions& options);

HistogramResult histogram(const std::vector<double>& observations, int bin_count = 0);
HistogramResult histogram_with_edges(
    const std::vector<double>& observations,
    const std::vector<double>& edges);

struct CauseEffectCategory {
    std::string category;
    std::vector<std::string> causes;
};

struct CauseEffectResult {
    std::string effect;
    std::vector<CauseEffectCategory> categories;
    std::size_t cause_count = 0;
    std::vector<DiagnosticMessage> diagnostics;
};

// Pair list (category, cause); empty category/cause rows skipped with missing count.
CauseEffectResult cause_and_effect_summarize(
    const std::string& effect,
    const std::vector<std::pair<std::string, std::string>>& category_causes);

struct VariabilityCell {
    std::string label;
    std::size_t n = 0;
    double mean = 0.0;
    double sample_sd = 0.0;
    std::vector<std::size_t> source_rows;
};

struct VariabilityChartResult {
    std::vector<VariabilityCell> cells;
    std::size_t factor_count = 0;
    std::size_t valid_count = 0;
    std::vector<DiagnosticMessage> diagnostics;
};

// Cells from (measurement, factor labels). factor_count 1 or 2.
VariabilityChartResult variability_chart_summarize(
    const std::vector<double>& measurements,
    const std::vector<std::string>& factor_a,
    const std::vector<std::string>& factor_b = {},
    const std::vector<std::size_t>& source_rows = {});

}  // namespace datalab::domain::statistics
