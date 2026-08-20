#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct MultiVariPoint {
    std::size_t source_row = 0;
    double measurement = 0.0;
    std::vector<std::string> factor_levels;
    double x_position = 0.0;
};

struct MultiVariFactorMean {
    std::string factor_name;
    std::string level;
    std::size_t count = 0;
    double mean = 0.0;
};

struct MultiVariCellMean {
    std::vector<std::string> levels;
    std::size_t count = 0;
    double mean = 0.0;
    double x_position = 0.0;
};

struct MultiVariMeanSeries {
    std::string label;
    std::vector<double> x_values;
    std::vector<double> y_values;
};

struct MultiVariResult {
    std::vector<std::string> factor_names;
    std::vector<MultiVariPoint> points;
    std::vector<MultiVariFactorMean> factor_means;
    std::vector<MultiVariCellMean> cell_means;
    std::vector<MultiVariMeanSeries> mean_series;
    std::size_t factor_count = 0;
    std::size_t valid_count = 0;
    std::size_t possible_combinations = 0;
    std::size_t observed_combinations = 0;
    double combination_coverage = 0.0;
    bool plot_available = false;
    std::vector<DiagnosticMessage> diagnostics;
};

// measurements, factor_levels[i], and source_rows must be complete-case aligned.
// factor_levels[i][j] is the original text of factor j for observation i.
MultiVariResult multi_vari_chart(
    const std::vector<double>& measurements,
    const std::vector<std::vector<std::string>>& factor_levels,
    const std::vector<std::size_t>& source_rows,
    const std::vector<std::string>& factor_names);

}  // namespace datalab::domain::statistics
