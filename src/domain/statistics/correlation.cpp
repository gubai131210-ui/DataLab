#include "domain/statistics/correlation.h"

#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

void add_error(std::vector<DiagnosticMessage>& diagnostics, const char* code, const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

std::vector<double> average_ranks(const std::vector<double>& values)
{
    std::vector<std::size_t> order(values.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](std::size_t left, std::size_t right) {
        return values[left] < values[right];
    });
    std::vector<double> ranks(values.size());
    std::size_t index = 0;
    while (index < order.size()) {
        std::size_t end = index + 1;
        while (end < order.size() && values[order[end]] == values[order[index]]) {
            ++end;
        }
        const double rank = 0.5 * static_cast<double>(index + end - 1) + 1.0;
        for (std::size_t position = index; position < end; ++position) {
            ranks[order[position]] = rank;
        }
        index = end;
    }
    return ranks;
}

double normal_quantile(double probability);

CorrelationPairResult calculate_pair(
    const std::vector<double>& first,
    const std::vector<double>& second,
    std::size_t first_column,
    std::size_t second_column,
    CorrelationMethod method,
    double confidence_level)
{
    CorrelationPairResult result;
    result.first_column = first_column;
    result.second_column = second_column;
    const std::size_t count = std::min(first.size(), second.size());
    std::vector<double> x;
    std::vector<double> y;
    for (std::size_t index = 0; index < count; ++index) {
        if (std::isfinite(first[index]) && std::isfinite(second[index])) {
            x.push_back(first[index]);
            y.push_back(second[index]);
        }
    }
    result.count = x.size();
    if (x.size() < 3) {
        add_error(result.diagnostics, "insufficient_pairs",
                  "相关分析至少需要三个有效配对观测。");
        return result;
    }
    if (method == CorrelationMethod::spearman) {
        x = average_ranks(x);
        y = average_ranks(y);
    }
    const double mean_x = std::accumulate(x.cbegin(), x.cend(), 0.0)
        / static_cast<double>(x.size());
    const double mean_y = std::accumulate(y.cbegin(), y.cend(), 0.0)
        / static_cast<double>(y.size());
    double covariance = 0.0;
    double variance_x = 0.0;
    double variance_y = 0.0;
    for (std::size_t index = 0; index < x.size(); ++index) {
        const double centered_x = x[index] - mean_x;
        const double centered_y = y[index] - mean_y;
        covariance += centered_x * centered_y;
        variance_x += centered_x * centered_x;
        variance_y += centered_y * centered_y;
    }
    if (variance_x == 0.0 || variance_y == 0.0) {
        add_error(result.diagnostics, "constant_column",
                  "相关分析不支持有效配对数据中的常量列。");
        return result;
    }
    result.coefficient = covariance / std::sqrt(variance_x * variance_y);
    const double degrees_of_freedom = static_cast<double>(x.size() - 2);
    const double statistic = result.coefficient
        * std::sqrt(degrees_of_freedom
                    / (1.0 - result.coefficient * result.coefficient));
    result.p_value = 2.0 * (1.0 - student_t_cdf(
        std::abs(statistic), degrees_of_freedom));
    if (confidence_level <= 0.0 || confidence_level >= 1.0) {
        add_error(result.diagnostics, "invalid_confidence_level",
                  "置信水平必须大于 0 且小于 1。");
        return result;
    }
    if (x.size() > 3 && std::abs(result.coefficient) < 1.0) {
        const double transformed = std::atanh(result.coefficient);
        const double standard_error = 1.0 / std::sqrt(static_cast<double>(x.size() - 3));
        const double critical = normal_quantile(0.5 + confidence_level / 2.0);
        result.confidence_lower = std::tanh(transformed - critical * standard_error);
        result.confidence_upper = std::tanh(transformed + critical * standard_error);
    }
    return result;
}

double normal_quantile(double probability)
{
    double lower = -9.0;
    double upper = 9.0;
    for (int iteration = 0; iteration < 100; ++iteration) {
        const double middle = 0.5 * (lower + upper);
        if (standard_normal_cdf(middle) < probability) {
            lower = middle;
        } else {
            upper = middle;
        }
    }
    return 0.5 * (lower + upper);
}

}  // namespace

CorrelationResult correlation_matrix(
    const std::vector<std::vector<double>>& columns,
    CorrelationMethod method,
    double confidence_level)
{
    CorrelationResult result;
    result.method = method;
    result.confidence_level = confidence_level;
    result.coefficients.assign(columns.size(), std::vector<double>(columns.size(), 0.0));
    result.counts.assign(columns.size(), std::vector<std::size_t>(columns.size(), 0));
    for (std::size_t first = 0; first < columns.size(); ++first) {
        for (std::size_t second = first; second < columns.size(); ++second) {
            CorrelationPairResult pair = calculate_pair(
                columns[first], columns[second], first, second, method, confidence_level);
            result.coefficients[first][second] = pair.coefficient;
            result.coefficients[second][first] = pair.coefficient;
            result.counts[first][second] = pair.count;
            result.counts[second][first] = pair.count;
            if (!pair.diagnostics.empty()) {
                result.diagnostics.insert(
                    result.diagnostics.end(),
                    pair.diagnostics.cbegin(),
                    pair.diagnostics.cend());
            }
            result.pairs.push_back(std::move(pair));
        }
    }
    return result;
}

}  // namespace datalab::domain::statistics
