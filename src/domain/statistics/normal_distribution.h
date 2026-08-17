#pragma once

namespace datalab::domain::statistics {

double standard_normal_cdf(double z);
double normal_pdf(double x, double mean, double sigma);
double expected_ppm_below(double mean, double sigma, double lower);
double expected_ppm_above(double mean, double sigma, double upper);

}  // namespace datalab::domain::statistics
