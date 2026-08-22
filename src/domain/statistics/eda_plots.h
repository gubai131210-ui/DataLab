#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct KdeResult {
    double bandwidth = 0.0;
    std::size_t n = 0;
    std::vector<double> x;
    std::vector<double> density;
    std::vector<DiagnosticMessage> diagnostics;
};

struct HexbinResult {
    std::size_t n = 0;
    std::size_t x_bins = 0;
    std::size_t y_bins = 0;
    std::vector<double> x_edges;
    std::vector<double> y_edges;
    std::vector<std::vector<double>> counts;  // [y][x]
    // Parallel to counts[y][x]: worksheet rows falling in that bin.
    std::vector<std::vector<std::vector<std::size_t>>> cell_source_rows;
    double max_count = 0.0;
    std::vector<std::size_t> source_rows;
    std::vector<DiagnosticMessage> diagnostics;
};

struct ViolinGroup {
    std::string label;
    std::size_t n = 0;
    double whisker_low = 0.0;
    double q1 = 0.0;
    double median = 0.0;
    double q3 = 0.0;
    double whisker_high = 0.0;
    std::vector<double> density_y;
    std::vector<double> density_values;  // max-normalized to 1
    std::vector<double> outliers;
    std::vector<std::size_t> source_rows;
};

struct ViolinResult {
    double bandwidth = 0.0;
    std::vector<ViolinGroup> groups;
    std::vector<DiagnosticMessage> diagnostics;
};

struct BarChartResult {
    std::vector<std::string> categories;
    std::vector<double> values;
    std::size_t total_count = 0;
    std::vector<DiagnosticMessage> diagnostics;
};

KdeResult gaussian_kde(
    const std::vector<double>& values,
    std::size_t grid_count = 128,
    std::optional<double> bandwidth = std::nullopt);

HexbinResult hexbin_rectangular(
    const std::vector<double>& x_values,
    const std::vector<double>& y_values,
    const std::vector<std::size_t>& source_rows,
    int bin_hint = 0);

ViolinResult violin_plot(
    const std::vector<double>& values,
    const std::vector<std::string>& groups,
    const std::vector<std::size_t>& source_rows);

BarChartResult bar_chart_counts(
    const std::vector<std::string>& categories,
    const std::vector<double>* weights = nullptr);

}  // namespace datalab::domain::statistics
