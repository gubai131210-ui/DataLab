#include "domain/statistics/normal_probability.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

double inverse_normal(double p)
{
    constexpr double a1 = -39.6968302866538;
    constexpr double a2 = 220.946098424521;
    constexpr double a3 = -275.928510446969;
    constexpr double a4 = 138.357751867269;
    constexpr double a5 = -30.6647980661472;
    constexpr double a6 = 2.50662827745924;
    constexpr double b1 = -54.4760987982241;
    constexpr double b2 = 161.585836858041;
    constexpr double b3 = -155.698979859887;
    constexpr double b4 = 66.8013118877197;
    constexpr double b5 = -13.2806815528857;
    constexpr double c1 = -0.00778489400243029;
    constexpr double c2 = -0.322396458041136;
    constexpr double c3 = -2.40075827716184;
    constexpr double c4 = -2.54973253934373;
    constexpr double c5 = 4.37466414146497;
    constexpr double c6 = 2.93816398269878;
    constexpr double d1 = 0.00778469570904146;
    constexpr double d2 = 0.32246712907004;
    constexpr double d3 = 2.445134137143;
    constexpr double d4 = 3.75440866190742;
    const double q = p - 0.5;
    if (std::abs(q) <= 0.425) {
        const double r = 0.180625 - q * q;
        return q * (((((a1 * r + a2) * r + a3) * r + a4) * r + a5) * r + a6)
            / (((((b1 * r + b2) * r + b3) * r + b4) * r + b5) * r + 1.0);
    }
    const double r = q < 0.0 ? p : 1.0 - p;
    const double s = std::sqrt(-std::log(r));
    double value = (((((c1 * s + c2) * s + c3) * s + c4) * s + c5) * s + c6)
        / ((((d1 * s + d2) * s + d3) * s + d4) * s + 1.0);
    return q < 0.0 ? value : -value;
}

}  // namespace

NormalProbabilityResult normal_probability_plot(
    const std::vector<double>& observations)
{
    NormalProbabilityResult result;
    result.ordered_values = observations;
    std::sort(result.ordered_values.begin(), result.ordered_values.end());
    const std::size_t n = result.ordered_values.size();
    result.theoretical_quantiles.reserve(n);
    for (std::size_t index = 0; index < n; ++index) {
        const double probability = (static_cast<double>(index) + 0.625)
            / (static_cast<double>(n) + 0.25);
        result.theoretical_quantiles.push_back(inverse_normal(probability));
    }
    if (n >= 2) {
        const double mean_x = std::accumulate(
            result.theoretical_quantiles.begin(), result.theoretical_quantiles.end(), 0.0)
            / static_cast<double>(n);
        const double mean_y = std::accumulate(
            result.ordered_values.begin(), result.ordered_values.end(), 0.0)
            / static_cast<double>(n);
        double covariance = 0.0;
        double variance_x = 0.0;
        double variance_y = 0.0;
        for (std::size_t index = 0; index < n; ++index) {
            const double x = result.theoretical_quantiles[index] - mean_x;
            const double y = result.ordered_values[index] - mean_y;
            covariance += x * y;
            variance_x += x * x;
            variance_y += y * y;
        }
        if (variance_x > 0.0 && variance_y > 0.0) {
            result.correlation = covariance / std::sqrt(variance_x * variance_y);
        }
    }
    return result;
}

}  // namespace datalab::domain::statistics
