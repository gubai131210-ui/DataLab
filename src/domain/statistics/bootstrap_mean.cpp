#include "domain/statistics/bootstrap_mean.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace datalab::domain::statistics {

BootstrapMeanResult bootstrap_mean_ci(
    const std::vector<double>& sample,
    const BootstrapMeanOptions& options)
{
    BootstrapMeanResult result;
    result.replicates = options.replicates;
    result.confidence_level = options.confidence_level;
    std::vector<double> values;
    for (double value : sample) {
        if (std::isfinite(value)) {
            values.push_back(value);
        } else {
            ++result.missing_count;
        }
    }
    result.n = values.size();
    if (values.size() < 2 || options.replicates < 50
        || !(options.confidence_level > 0.0 && options.confidence_level < 1.0)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "bootstrap_invalid",
            "Bootstrap 均值 CI 需要 n≥2、B≥50 与有效置信水平。"});
        return result;
    }
    result.sample_mean =
        std::accumulate(values.cbegin(), values.cend(), 0.0)
        / static_cast<double>(values.size());

    std::mt19937 rng(options.seed);
    std::uniform_int_distribution<std::size_t> dist(0, values.size() - 1);
    result.bootstrap_means.reserve(options.replicates);
    for (std::size_t replicate = 0; replicate < options.replicates; ++replicate) {
        double sum = 0.0;
        for (std::size_t index = 0; index < values.size(); ++index) {
            sum += values[dist(rng)];
        }
        result.bootstrap_means.push_back(sum / static_cast<double>(values.size()));
    }
    std::vector<double> ordered = result.bootstrap_means;
    std::sort(ordered.begin(), ordered.end());
    const double alpha = 1.0 - options.confidence_level;
    const double lower_p = alpha / 2.0;
    const double upper_p = 1.0 - alpha / 2.0;
    auto percentile = [&](double p) {
        const double pos = p * static_cast<double>(ordered.size() - 1);
        const std::size_t left = static_cast<std::size_t>(std::floor(pos));
        const std::size_t right = static_cast<std::size_t>(std::ceil(pos));
        if (left == right) {
            return ordered[left];
        }
        const double weight = pos - static_cast<double>(left);
        return ordered[left] * (1.0 - weight) + ordered[right] * weight;
    };
    result.ci_lower = percentile(lower_p);
    result.ci_upper = percentile(upper_p);
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "bootstrap_method",
        "百分位 bootstrap CI；非 BCa；非 Minitab golden。"});
    return result;
}

}  // namespace datalab::domain::statistics
