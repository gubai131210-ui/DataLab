#include "domain/statistics/normal_distribution.h"

#include <cmath>
#include <limits>

namespace datalab::domain::statistics {

double standard_normal_cdf(double z)
{
    if (!std::isfinite(z)) {
        return z > 0.0 ? 1.0 : 0.0;
    }
    return 0.5 * (1.0 + std::erf(z / std::sqrt(2.0)));
}

double standard_normal_quantile(double probability)
{
    if (!std::isfinite(probability) || probability <= 0.0 || probability >= 1.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double a1 = -39.6968302866538;
    const double a2 = 220.946098424521;
    const double a3 = -275.928510446969;
    const double a4 = 138.357751867269;
    const double a5 = -30.6647980661472;
    const double a6 = 2.50662827745924;
    const double b1 = -54.4760987982241;
    const double b2 = 161.585836858041;
    const double b3 = -155.698979859887;
    const double b4 = 66.8013118877197;
    const double b5 = -13.2806815528857;
    const double c1 = -0.00778489400243029;
    const double c2 = -0.322396458041136;
    const double c3 = -2.40075827716184;
    const double c4 = -2.54973253934373;
    const double c5 = 4.37466414146497;
    const double c6 = 2.93816398269878;
    const double d1 = 0.00778469570904146;
    const double d2 = 0.32246712907004;
    const double d3 = 2.445134137143;
    const double d4 = 3.75440866190742;
    if (probability < 0.02425) {
        const double q = std::sqrt(-2.0 * std::log(probability));
        return -(((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6)
            / ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
    }
    if (probability > 1.0 - 0.02425) {
        const double q = std::sqrt(-2.0 * std::log(1.0 - probability));
        return (((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6)
            / ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
    }
    const double q = probability - 0.5;
    const double r = q * q;
    return (((((a1 * r + a2) * r + a3) * r + a4) * r + a5) * r + a6) * q
        / (((((b1 * r + b2) * r + b3) * r + b4) * r + b5) * r + 1.0);
}

double normal_pdf(double x, double mean, double sigma)
{
    if (!(sigma > 0.0) || !std::isfinite(sigma) || !std::isfinite(x) || !std::isfinite(mean)) {
        return 0.0;
    }
    const double z = (x - mean) / sigma;
    return std::exp(-0.5 * z * z) / (sigma * std::sqrt(2.0 * 3.14159265358979323846));
}

double expected_ppm_below(double mean, double sigma, double lower)
{
    if (!(sigma > 0.0)) {
        return 0.0;
    }
    return 1.0e6 * standard_normal_cdf((lower - mean) / sigma);
}

double expected_ppm_above(double mean, double sigma, double upper)
{
    if (!(sigma > 0.0)) {
        return 0.0;
    }
    return 1.0e6 * (1.0 - standard_normal_cdf((upper - mean) / sigma));
}

}  // namespace datalab::domain::statistics
