#include "domain/statistics/normality_test.h"

#include "domain/statistics/descriptive_statistics.h"

#include <algorithm>
#include <cmath>

namespace datalab::domain::statistics {
namespace {

double normal_cdf(double value, double mean, double standard_deviation)
{
    return 0.5 * (1.0 + std::erf(
        (value - mean) / (standard_deviation * std::sqrt(2.0))));
}

double anderson_darling_p_value(double adjusted)
{
    if (adjusted > 0.6) {
        return std::exp(1.2937 - 5.709 * adjusted + 0.0186 * adjusted * adjusted);
    }
    if (adjusted > 0.34) {
        return std::exp(0.9177 - 4.279 * adjusted - 1.38 * adjusted * adjusted);
    }
    if (adjusted > 0.2) {
        return 1.0 - std::exp(-8.318 + 42.796 * adjusted - 59.938 * adjusted * adjusted);
    }
    return 1.0 - std::exp(-13.436 + 101.14 * adjusted - 223.73 * adjusted * adjusted);
}

}  // namespace

NormalityTestResult normality_test(const std::vector<double>& observations)
{
    NormalityTestResult result;
    result.count = observations.size();
    result.probability_plot = normal_probability_plot(observations);
    const auto summary = DescriptiveStatistics::calculate(observations);
    if (!summary.has_value()) {
        result.diagnostics.push_back("正态性检验需要至少一个有效数值观测。");
        return result;
    }
    result.mean = summary->mean;
    if (!summary->sample_standard_deviation.has_value()) {
        result.diagnostics.push_back("正态性检验至少需要 2 个观测；当前仅返回概率图点。");
        return result;
    }
    result.sample_standard_deviation = *summary->sample_standard_deviation;
    if (observations.size() < 3 || !(result.sample_standard_deviation > 0.0)) {
        result.diagnostics.push_back("正态性检验至少需要 3 个且不能全部相同的观测。");
        return result;
    }

    std::vector<double> ordered = observations;
    std::sort(ordered.begin(), ordered.end());
    constexpr double kEpsilon = 1.0e-12;
    double sum = 0.0;
    for (std::size_t index = 0; index < ordered.size(); ++index) {
        const double cdf = std::clamp(
            normal_cdf(ordered[index], result.mean, result.sample_standard_deviation),
            kEpsilon, 1.0 - kEpsilon);
        const double left_weight = static_cast<double>(2 * index + 1);
        const double right_weight =
            static_cast<double>(2 * ordered.size() + 1 - 2 * (index + 1));
        sum += left_weight * std::log(cdf)
            + right_weight * std::log(1.0 - cdf);
    }
    const double statistic = -static_cast<double>(ordered.size())
        - sum / static_cast<double>(ordered.size());
    // Stephens' normality correction used by Minitab's AD approximation.
    const double adjusted = statistic * (1.0 + 0.75 / ordered.size()
        + 2.25 / (ordered.size() * ordered.size()));
    result.anderson_darling = statistic;
    result.p_value = std::clamp(anderson_darling_p_value(adjusted), 0.0, 1.0);
    return result;
}

}  // namespace datalab::domain::statistics
