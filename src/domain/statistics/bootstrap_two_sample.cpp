#include "domain/statistics/bootstrap_two_sample.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace datalab::domain::statistics {
namespace {

std::vector<double> finite_values(const std::vector<double>& sample, std::size_t& missing_count)
{
    std::vector<double> values;
    for (double value : sample) {
        if (std::isfinite(value)) {
            values.push_back(value);
        } else {
            ++missing_count;
        }
    }
    return values;
}

double sample_mean(const std::vector<double>& values)
{
    return std::accumulate(values.cbegin(), values.cend(), 0.0)
        / static_cast<double>(values.size());
}

double percentile(const std::vector<double>& ordered, double p)
{
    const double pos = p * static_cast<double>(ordered.size() - 1);
    const std::size_t left = static_cast<std::size_t>(std::floor(pos));
    const std::size_t right = static_cast<std::size_t>(std::ceil(pos));
    if (left == right) {
        return ordered[left];
    }
    const double weight = pos - static_cast<double>(left);
    return ordered[left] * (1.0 - weight) + ordered[right] * weight;
}

}  // namespace

BootstrapTwoSampleResult bootstrap_two_sample_mean_difference_ci(
    const std::vector<double>& first_sample,
    const std::vector<double>& second_sample,
    const BootstrapTwoSampleOptions& options)
{
    BootstrapTwoSampleResult result;
    result.replicates = options.replicates;
    result.confidence_level = options.confidence_level;

    std::size_t missing_first = 0;
    std::size_t missing_second = 0;
    const std::vector<double> first = finite_values(first_sample, missing_first);
    const std::vector<double> second = finite_values(second_sample, missing_second);
    result.n_first = first.size();
    result.n_second = second.size();
    result.missing_count = missing_first + missing_second;

    if (first.size() < 2 || second.size() < 2 || options.replicates < 50
        || !(options.confidence_level > 0.0 && options.confidence_level < 1.0)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "bootstrap_invalid",
            "Bootstrap 双样本均值差 CI 需要两组 n≥2、B≥50 与有效置信水平。"});
        return result;
    }

    result.mean_first = sample_mean(first);
    result.mean_second = sample_mean(second);
    result.mean_difference = *result.mean_first - *result.mean_second;

    std::mt19937 rng(options.seed);
    std::uniform_int_distribution<std::size_t> first_dist(0, first.size() - 1);
    std::uniform_int_distribution<std::size_t> second_dist(0, second.size() - 1);
    result.bootstrap_differences.reserve(options.replicates);
    for (std::size_t replicate = 0; replicate < options.replicates; ++replicate) {
        double first_sum = 0.0;
        for (std::size_t index = 0; index < first.size(); ++index) {
            first_sum += first[first_dist(rng)];
        }
        double second_sum = 0.0;
        for (std::size_t index = 0; index < second.size(); ++index) {
            second_sum += second[second_dist(rng)];
        }
        result.bootstrap_differences.push_back(
            first_sum / static_cast<double>(first.size())
            - second_sum / static_cast<double>(second.size()));
    }

    std::vector<double> ordered = result.bootstrap_differences;
    std::sort(ordered.begin(), ordered.end());
    const double alpha = 1.0 - options.confidence_level;
    result.ci_lower = percentile(ordered, alpha / 2.0);
    result.ci_upper = percentile(ordered, 1.0 - alpha / 2.0);
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "bootstrap_method",
        "百分位 bootstrap CI；非 BCa；非 Minitab golden。"});
    return result;
}

}  // namespace datalab::domain::statistics
