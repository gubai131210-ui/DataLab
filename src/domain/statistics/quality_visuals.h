#pragma once

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

}  // namespace datalab::domain::statistics
