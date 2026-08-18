#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct DescriptiveStatisticsResult {
    std::size_t count = 0;
    std::size_t missing_count = 0;
    std::size_t total_count = 0;
    double mean = 0.0;
    double minimum = 0.0;
    double maximum = 0.0;
    double median = 0.0;
    double first_quartile = 0.0;
    double third_quartile = 0.0;
    double variance = 0.0;
    double interquartile_range = 0.0;
    double range = 0.0;
    double sum = 0.0;
    std::optional<double> skewness;
    std::optional<double> excess_kurtosis;
    std::optional<double> sample_standard_deviation;
    std::optional<double> standard_error_of_mean;
    double population_standard_deviation = 0.0;
    std::string group_label;
};

class DescriptiveStatistics final {
public:
    static std::optional<DescriptiveStatisticsResult> calculate(
        const std::vector<double>& observations);

    static std::optional<DescriptiveStatisticsResult> calculate(
        const std::vector<double>& observations,
        std::size_t missing_count,
        std::size_t total_count);
};

}  // namespace datalab::domain::statistics
