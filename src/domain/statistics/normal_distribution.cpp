#include "domain/statistics/normal_distribution.h"

#include <cmath>

namespace datalab::domain::statistics {

double standard_normal_cdf(double z)
{
    if (!std::isfinite(z)) {
        return z > 0.0 ? 1.0 : 0.0;
    }
    return 0.5 * (1.0 + std::erf(z / std::sqrt(2.0)));
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
