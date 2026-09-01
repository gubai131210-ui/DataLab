#include "domain/statistics/life_data_regression.h"

#include "domain/statistics/reliability.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

namespace datalab::domain::statistics {
namespace {

using Matrix = std::vector<std::vector<double>>;

double normal_quantile(double p)
{
    if (p <= 0.0) {
        return -8.0;
    }
    if (p >= 1.0) {
        return 8.0;
    }
    const double a[] = {-3.969683028665376e+01, 2.209460984245205e+02,
                        -2.759285939946717e+02, 1.383577518672690e+02,
                        -3.066479806614716e+01, 2.506628277459239e+00};
    const double b[] = {-5.447609879822406e+01, 1.615858368580409e+02,
                        -1.556989798598866e+02, 6.680654618783063e+01,
                        -1.328068155288572e+01};
    const double c[] = {-7.784894002430293e-03, -3.226964378048109e-01,
                        -2.762148489337111e+00, -1.429488411088870e+00,
                        -5.473931734884816e+00};
    const double d[] = {7.784695709041462e-03, 3.224671290700397e-01,
                        2.445134137142996e+00, 3.754408661907416e+00};
    const double plow = 0.02425;
    const double phigh = 1.0 - plow;
    if (p < plow) {
        const double q = std::sqrt(-2.0 * std::log(p));
        return (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5])
            / ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    }
    if (p > phigh) {
        const double q = std::sqrt(-2.0 * std::log(1.0 - p));
        return -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5])
            / ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    }
    const double q = p - 0.5;
    const double r = q * q;
    return (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q
        / (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1.0);
}

double two_sided_normal_p(double z)
{
    return std::erfc(std::abs(z) / std::sqrt(2.0));
}

bool invert_matrix(const Matrix& input, Matrix& inverse)
{
    const std::size_t size = input.size();
    if (size == 0) {
        return false;
    }
    Matrix augmented(size, std::vector<double>(size * 2, 0.0));
    for (std::size_t row = 0; row < size; ++row) {
        if (input[row].size() != size) {
            return false;
        }
        for (std::size_t column = 0; column < size; ++column) {
            augmented[row][column] = input[row][column];
        }
        augmented[row][size + row] = 1.0;
    }
    for (std::size_t column = 0; column < size; ++column) {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < size; ++row) {
            if (std::abs(augmented[row][column])
                > std::abs(augmented[pivot][column])) {
                pivot = row;
            }
        }
        if (!std::isfinite(augmented[pivot][column])
            || std::abs(augmented[pivot][column]) <= 1.0e-12) {
            return false;
        }
        std::swap(augmented[pivot], augmented[column]);
        const double divisor = augmented[column][column];
        for (double& value : augmented[column]) {
            value /= divisor;
        }
        for (std::size_t row = 0; row < size; ++row) {
            if (row == column) {
                continue;
            }
            const double factor = augmented[row][column];
            for (std::size_t value = 0; value < size * 2; ++value) {
                augmented[row][value] -= factor * augmented[column][value];
            }
        }
    }
    inverse.assign(size, std::vector<double>(size, 0.0));
    for (std::size_t row = 0; row < size; ++row) {
        for (std::size_t column = 0; column < size; ++column) {
            inverse[row][column] = augmented[row][size + column];
        }
    }
    return true;
}

double weibull_regression_log_likelihood(
    const std::vector<double>& betas,
    double log_shape,
    const std::vector<double>& log_times,
    const std::vector<bool>& events,
    const std::vector<std::vector<double>>& covariates)
{
    const double shape = std::exp(log_shape);
    if (!(shape > 0.0)) {
        return -std::numeric_limits<double>::infinity();
    }
    double ll = 0.0;
    for (std::size_t i = 0; i < log_times.size(); ++i) {
        double eta = betas[0];
        for (std::size_t k = 0; k < covariates[i].size(); ++k) {
            eta += betas[k + 1] * covariates[i][k];
        }
        const double scale = std::exp(eta);
        if (!(scale > 0.0)) {
            return -std::numeric_limits<double>::infinity();
        }
        const double z = std::pow(std::exp(log_times[i]) / scale, shape);
        if (events[i]) {
            ll += std::log(shape / scale)
                + (shape - 1.0) * (log_times[i] - eta)
                - z;
        } else {
            ll -= z;
        }
    }
    return ll;
}

LifeDataRegressionCoefficient make_coefficient(
    const std::string& term,
    double estimate,
    double standard_error,
    double confidence_level)
{
    LifeDataRegressionCoefficient coefficient;
    coefficient.term = term;
    coefficient.estimate = estimate;
    coefficient.standard_error = standard_error;
    if (standard_error > 0.0) {
        coefficient.z_statistic = estimate / standard_error;
        coefficient.p_value = two_sided_normal_p(coefficient.z_statistic);
    }
    const double z_crit = normal_quantile(0.5 + confidence_level / 2.0);
    coefficient.confidence_lower = estimate - z_crit * standard_error;
    coefficient.confidence_upper = estimate + z_crit * standard_error;
    return coefficient;
}

}  // namespace

LifeDataRegressionResult fit_life_data_regression_weibull(
    const std::vector<double>& times,
    const std::vector<bool>& events,
    const std::vector<std::vector<double>>& covariates,
    const std::vector<std::string>& covariate_labels,
    const std::vector<std::size_t>& source_rows,
    const LifeDataRegressionOptions& options)
{
    (void)source_rows;
    LifeDataRegressionResult result;
    result.distribution = "weibull";
    const std::size_t p = covariates.empty() ? 0 : covariates.front().size();
    result.covariate_count = p;
    if (times.size() < 4 || times.size() != events.size()
        || times.size() != covariates.size() || p > 2) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "life_reg_invalid",
            "寿命回归需要 ≥4 行、1～2 协变量且列对齐。"});
        return result;
    }

    std::vector<double> log_times;
    std::vector<bool> aligned_events;
    std::vector<std::vector<double>> aligned_covariates;
    for (std::size_t i = 0; i < times.size(); ++i) {
        if (!(times[i] > 0.0) || covariates[i].size() != p) {
            continue;
        }
        bool finite = true;
        for (double value : covariates[i]) {
            if (!std::isfinite(value)) {
                finite = false;
                break;
            }
        }
        if (!finite) {
            continue;
        }
        log_times.push_back(std::log(times[i]));
        aligned_events.push_back(events[i]);
        aligned_covariates.push_back(covariates[i]);
        if (events[i]) {
            ++result.failure_count;
        } else {
            ++result.censored_count;
        }
    }
    result.observation_count = log_times.size();
    if (result.observation_count < 4 || result.failure_count < 2) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "life_reg_insufficient",
            "有效失败观测不足。"});
        return result;
    }

    const std::size_t param_count = p + 2;
    std::vector<double> betas(param_count - 1, 0.0);
    double log_shape = 0.0;
    betas[0] = std::accumulate(
        log_times.cbegin(), log_times.cend(), 0.0)
        / static_cast<double>(log_times.size());

    Matrix nr_cov;
    bool have_nr_cov = false;
    for (int iteration = 0; iteration < 40; ++iteration) {
        Matrix information(param_count, std::vector<double>(param_count, 0.0));
        std::vector<double> gradient(param_count, 0.0);
        const double shape = std::exp(log_shape);
        for (std::size_t i = 0; i < log_times.size(); ++i) {
            double eta = betas[0];
            for (std::size_t k = 0; k < p; ++k) {
                eta += betas[k + 1] * aligned_covariates[i][k];
            }
            const double scale = std::exp(eta);
            const double z = std::pow(std::exp(log_times[i]) / scale, shape);
            const double delta = aligned_events[i] ? 1.0 : 0.0;
            const double g_eta = shape * (z - delta);
            gradient[0] += g_eta;
            for (std::size_t k = 0; k < p; ++k) {
                gradient[k + 1] += aligned_covariates[i][k] * g_eta;
            }
            const double u = 1.0 + shape * (log_times[i] - eta);
            gradient[p + 1] += delta * (1.0 - z) * u - (1.0 - delta) * z * u;
            information[0][0] += shape * shape * z;
            for (std::size_t k = 0; k < p; ++k) {
                information[0][k + 1] += shape * shape * z * aligned_covariates[i][k];
                information[k + 1][0] = information[0][k + 1];
                information[k + 1][k + 1] += shape * shape * z
                    * aligned_covariates[i][k] * aligned_covariates[i][k];
            }
            information[0][p + 1] += shape * z * (log_times[i] - eta);
            information[p + 1][0] = information[0][p + 1];
            for (std::size_t k = 0; k < p; ++k) {
                information[k + 1][p + 1] += shape * z * (log_times[i] - eta)
                    * aligned_covariates[i][k];
                information[p + 1][k + 1] = information[k + 1][p + 1];
            }
            information[p + 1][p + 1] += delta * z * u * u - (1.0 - delta) * z * u * u;
        }
        Matrix inverse;
        if (!invert_matrix(information, inverse)) {
            break;
        }
        nr_cov = inverse;
        have_nr_cov = true;
        std::vector<double> step(param_count, 0.0);
        for (std::size_t row = 0; row < param_count; ++row) {
            for (std::size_t column = 0; column < param_count; ++column) {
                step[row] += inverse[row][column] * gradient[column];
            }
        }
        betas[0] += step[0];
        for (std::size_t k = 0; k < p; ++k) {
            betas[k + 1] += step[k + 1];
        }
        log_shape += step[p + 1];
        if (std::sqrt(std::inner_product(step.cbegin(), step.cend(), step.cbegin(), 0.0))
            < 1.0e-6) {
            result.converged = true;
            break;
        }
    }

    result.shape = std::exp(log_shape);
    result.log_likelihood = weibull_regression_log_likelihood(
        betas, log_shape, log_times, aligned_events, aligned_covariates);

    Matrix information(param_count, std::vector<double>(param_count, 0.0));
    const double eps = 1.0e-5;
    for (std::size_t column = 0; column < param_count; ++column) {
        std::vector<double> plus_betas = betas;
        double plus_shape = log_shape;
        if (column == 0) {
            plus_betas[0] += eps;
        } else if (column <= p) {
            plus_betas[column] += eps;
        } else {
            plus_shape += eps;
        }
        std::vector<double> minus_betas = betas;
        double minus_shape = log_shape;
        if (column == 0) {
            minus_betas[0] -= eps;
        } else if (column <= p) {
            minus_betas[column] -= eps;
        } else {
            minus_shape -= eps;
        }
        const double ll_plus = weibull_regression_log_likelihood(
            plus_betas, plus_shape, log_times, aligned_events, aligned_covariates);
        const double ll_minus = weibull_regression_log_likelihood(
            minus_betas, minus_shape, log_times, aligned_events, aligned_covariates);
        for (std::size_t row = 0; row < param_count; ++row) {
            std::vector<double> row_plus = betas;
            std::vector<double> row_minus = betas;
            double shape_plus = log_shape;
            double shape_minus = log_shape;
            if (row == 0) {
                row_plus[0] += eps;
                row_minus[0] -= eps;
            } else if (row <= p) {
                row_plus[row] += eps;
                row_minus[row] -= eps;
            } else {
                shape_plus += eps;
                shape_minus -= eps;
            }
            const double g_plus = weibull_regression_log_likelihood(
                row_plus, shape_plus, log_times, aligned_events, aligned_covariates);
            const double g_minus = weibull_regression_log_likelihood(
                row_minus, shape_minus, log_times, aligned_events, aligned_covariates);
            information[row][column] = -(g_plus - g_minus) / (2.0 * eps);
        }
        (void)ll_plus;
        (void)ll_minus;
    }
    Matrix cov;
    const bool have_cov = invert_matrix(information, cov)
        || (have_nr_cov && (cov = nr_cov, true));
    if (have_cov) {
        result.coefficients.push_back(make_coefficient(
            "Intercept", betas[0], std::sqrt(std::max(0.0, cov[0][0])),
            options.confidence_level));
        for (std::size_t k = 0; k < p; ++k) {
            const std::string label = k < covariate_labels.size()
                ? covariate_labels[k] : ("X" + std::to_string(k + 1));
            result.coefficients.push_back(make_coefficient(
                label, betas[k + 1], std::sqrt(std::max(0.0, cov[k + 1][k + 1])),
                options.confidence_level));
        }
        result.coefficients.push_back(make_coefficient(
            "Log(Shape)", log_shape, std::sqrt(std::max(0.0, cov[p + 1][p + 1])),
            options.confidence_level));
    }

    std::vector<double> profile(p, 0.0);
    for (const auto& row : aligned_covariates) {
        for (std::size_t k = 0; k < p; ++k) {
            profile[k] += row[k];
        }
    }
    for (double& value : profile) {
        value /= static_cast<double>(aligned_covariates.size());
    }
    double eta = betas[0];
    for (std::size_t k = 0; k < p; ++k) {
        eta += betas[k + 1] * profile[k];
    }
    const double scale = std::exp(eta);
    std::string profile_text = "mean covariates";
    for (const double percentile : options.percentile_levels) {
        LifeDataRegressionPercentile row;
        row.percentile = percentile;
        row.life = percentile_life_weibull(result.shape, scale, percentile);
        row.covariate_profile = profile_text;
        result.percentiles.push_back(row);
    }

    if (!result.converged) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "life_reg_not_converged",
            "Weibull MLE Newton-Raphson 未完全收敛。"});
    }
    return result;
}

}  // namespace datalab::domain::statistics
