#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/correlation.h"
#include "domain/statistics/quality_visuals.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct ScatterPlotResult {
    std::vector<double> x_values;
    std::vector<double> y_values;
    std::vector<std::size_t> source_rows;
    std::vector<std::string> point_labels;
    std::vector<std::string> point_groups;
    std::vector<DiagnosticMessage> diagnostics;
};

struct IntervalPlotResult {
    std::vector<std::string> labels;
    std::vector<double> means;
    std::vector<double> lower;
    std::vector<double> upper;
    std::vector<std::size_t> counts;
    std::vector<std::vector<std::size_t>> source_rows;
    std::vector<DiagnosticMessage> diagnostics;
};

struct BubblePlotResult {
    ScatterPlotResult points;
    std::vector<double> sizes;
    std::vector<DiagnosticMessage> diagnostics;
};

struct GraphCorrelationResult {
    CorrelationResult correlation;
    std::vector<std::string> labels;
};

struct EcdfPlotResult {
    std::vector<double> values;
    std::vector<double> proportions;
    std::vector<std::size_t> counts;
    std::vector<std::size_t> source_rows;
    std::vector<DiagnosticMessage> diagnostics;
};

struct ProbabilityPlotResult {
    std::vector<double> theoretical_quantiles;
    std::vector<double> ordered_values;
    std::vector<double> fitted;
    std::vector<double> lower;
    std::vector<double> upper;
    std::vector<std::size_t> source_rows;
    double correlation = 0.0;
    double location = 0.0;
    double scale = 1.0;
    std::vector<DiagnosticMessage> diagnostics;
};

struct MatrixScatterResult {
    std::vector<std::string> labels;
    std::vector<std::vector<double>> columns;
    std::vector<std::size_t> source_rows;
    std::vector<std::string> groups;
    std::vector<DiagnosticMessage> diagnostics;
};

struct MarginalPlotResult {
    ScatterPlotResult points;
    HistogramResult x_histogram;
    HistogramResult y_histogram;
};

struct ParallelPlotResult {
    std::vector<std::string> labels;
    std::vector<std::vector<double>> rows;
    std::vector<double> minima;
    std::vector<double> maxima;
    std::vector<std::size_t> source_rows;
    std::vector<std::string> groups;
    std::vector<DiagnosticMessage> diagnostics;
};

struct HeatmapPlotResult {
    std::vector<std::string> row_labels;
    std::vector<std::string> column_labels;
    std::vector<std::vector<double>> values;
    std::vector<std::vector<std::size_t>> counts;
    // Parallel to values[row][col]: worksheet rows aggregated into that cell.
    std::vector<std::vector<std::vector<std::size_t>>> cell_source_rows;
    double color_min = -1.0;
    double color_max = 1.0;
    std::vector<DiagnosticMessage> diagnostics;
};

struct TimeSeriesPlotResult {
    std::vector<double> x_values;
    std::vector<double> y_values;
    std::vector<std::string> time_labels;
    std::vector<std::size_t> source_rows;
    std::vector<std::string> groups;
    std::vector<DiagnosticMessage> diagnostics;
};

struct ContourPlotResult {
    std::vector<double> x;
    std::vector<double> y;
    std::vector<std::vector<double>> z;
    std::vector<double> levels;
    std::vector<DiagnosticMessage> diagnostics;
};

struct PiePlotResult {
    std::vector<std::string> labels;
    std::vector<double> values;
    std::vector<double> percents;
    std::vector<std::vector<std::size_t>> member_source_rows;
    std::vector<DiagnosticMessage> diagnostics;
};

ScatterPlotResult scatter_plot(
    const std::vector<double>& x_values,
    const std::vector<double>& y_values,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& point_groups = {},
    const std::vector<std::string>& point_labels = {});

IntervalPlotResult interval_plot(
    const std::vector<double>& values,
    const std::vector<std::string>& groups,
    const std::vector<std::size_t>& source_rows,
    double confidence_level = 0.95);

BubblePlotResult bubble_plot(
    const std::vector<double>& x_values,
    const std::vector<double>& y_values,
    const std::vector<double>& sizes,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& point_groups = {},
    const std::vector<std::string>& point_labels = {});

GraphCorrelationResult correlation_plot(
    const std::vector<std::vector<double>>& columns,
    const std::vector<std::string>& labels,
    const std::string& method,
    double confidence_level = 0.95);

EcdfPlotResult ecdf_plot(
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows);

ProbabilityPlotResult probability_plot(
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows);

MatrixScatterResult matrix_scatter_plot(
    const std::vector<std::vector<double>>& columns,
    const std::vector<std::string>& labels,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& groups = {});

MarginalPlotResult marginal_plot(
    const std::vector<double>& x_values,
    const std::vector<double>& y_values,
    const std::vector<std::size_t>& source_rows,
    int bin_count = 0);

ParallelPlotResult parallel_plot(
    const std::vector<std::vector<double>>& columns,
    const std::vector<std::string>& labels,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& groups = {});

HeatmapPlotResult heatmap_from_correlation(const GraphCorrelationResult& source);
HeatmapPlotResult heatmap_from_categories(
    const std::vector<std::string>& rows,
    const std::vector<std::string>& columns,
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows);

TimeSeriesPlotResult time_series_plot(
    const std::vector<double>& times,
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& time_labels = {},
    const std::vector<std::string>& groups = {});

ContourPlotResult contour_plot(
    const std::vector<double>& x_values,
    const std::vector<double>& y_values,
    const std::vector<double>& z_values,
    int level_count = 8);

PiePlotResult pie_plot(
    const std::vector<std::string>& categories,
    const std::vector<double>& weights,
    double other_threshold_percent = 5.0,
    const std::vector<std::size_t>& source_rows = {});

}  // namespace datalab::domain::statistics
