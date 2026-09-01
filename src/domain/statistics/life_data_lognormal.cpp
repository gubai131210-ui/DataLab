#include "domain/statistics/life_data_lognormal.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

using Matrix = std::vector<std::vector<double>>;

constexpr double kPi = 3.14159265358979323846;

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const std::string& message)
{
    diagnostics.push_back({severity, std::string{code}, message, {}});
}

double normal_cdf(double x)
{
    return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

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

double lognormal_log_likelihood(
    const std::vector<double>& betas,
    double log_sigma,
    const std::vector<double>& log_times,
    const std::vector<bool>& events,
    const std::vector<std::vector<double>>& covariates)
{
    const double sigma = std::exp(log_sigma);
    if (!(sigma > 0.0)) {
        return -std::numeric_limits<double>::infinity();
    }
    double ll = 0.0;
    for (std::size_t i = 0; i < log_times.size(); ++i) {
        double eta = betas[0];
        for (std::size_t k = 0; k < covariates[i].size(); ++k) {
            eta += betas[k + 1] * covariates[i][k];
        }
        const double z = (log_times[i] - eta) / sigma;
        if (events[i]) {
            ll -= log_times[i] + log_sigma + 0.5 * std::log(2.0 * kPi)
                + 0.5 * z * z;
        } else {
            ll += std::log(std::max(1.0 - normal_cdf(z), 1.0e-15));
        }
    }
    return ll;
}

LifeDataLognormalCoefficient make_coefficient(
    const std::string& term,
    double estimate,
    double standard_error,
    double confidence_level)
{
    LifeDataLognormalCoefficient coefficient;
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

LifeDataLognormalResult fit_life_data_lognormal(
    const std::vector<double>& times,
    const std::vector<bool>& events,
    const std::vector<std::vector<double>>& covariates,
    const std::vector<std::string>& covariate_labels,
    const std::vector<std::size_t>& source_rows,
    const LifeDataLognormalOptions& options)
{
    LifeDataLognormalResult result;
    const std::size_t p = covariates.empty() ? 0 : covariates.front().size();
    result.covariate_count = p;
    if (times.size() < 4 || times.size() != events.size()
        || times.size() != covariates.size() || p > 2) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "life_lognormal_invalid",
                       "Lognormal 寿命回归需要 ≥4 行、0～2 协变量且列对齐。");
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
        const std::size_t source = i < source_rows.size() ? source_rows[i] : i;
        result.observation_source_rows.push_back(source);
    }
    result.observation_count = log_times.size();
    if (result.observation_count < 4 || result.failure_count < 2) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "life_lognormal_insufficient", "有效失败观测不足。");
        return result;
    }

    const std::size_t param_count = p + 2;
    std::vector<double> betas(p + 1, 0.0);
    double log_sigma = std::log(
        std::accumulate(log_times.cbegin(), log_times.cend(), 0.0)
        / static_cast<double>(log_times.size()));
    betas[0] = log_sigma;

    for (int iteration = 0; iteration < 50; ++iteration) {
        Matrix information(param_count, std::vector<double>(param_count, 0.0));
        std::vector<double> gradient(param_count, 0.0);
        const double sigma = std::exp(log_sigma);
        for (std::size_t i = 0; i < log_times.size(); ++i) {
            double eta = betas[0];
            for (std::size_t k = 0; k < p; ++k) {
                eta += betas[k + 1] * aligned_covariates[i][k];
            }
            const double z = (log_times[i] - eta) / sigma;
            if (aligned_events[i]) {
                gradient[0] += (log_times[i] - eta) / (sigma * sigma);
                for (std::size_t k = 0; k < p; ++k) {
                    gradient[k + 1] += aligned_covariates[i][k]
                        * (log_times[i] - eta) / (sigma * sigma);
                }
                gradient[p + 1] += -1.0 + z * z;
                information[0][0] += 1.0 / (sigma * sigma);
                for (std::size_t k = 0; k < p; ++k) {
                    information[0][k + 1] += aligned_covariates[i][k] / (sigma * sigma);
                    information[k + 1][0] = information[0][k + 1];
                    information[k + 1][k + 1] += aligned_covariates[i][k]
                        * aligned_covariates[i][k] / (sigma * sigma);
                }
                information[0][p + 1] += z / sigma;
                information[p + 1][0] = information[0][p + 1];
                for (std::size_t k = 0; k < p; ++k) {
                    information[k + 1][p + 1] += z * aligned_covariates[i][k] / sigma;
                    information[p + 1][k + 1] = information[k + 1][p + 1];
                }
                information[p + 1][p + 1] += 2.0 * z * z;
            } else {
                const double cdf = normal_cdf(z);
                const double pdf = std::exp(-0.5 * z * z) / std::sqrt(2.0 * kPi);
                const double hazard_ratio = pdf / std::max(1.0 - cdf, 1.0e-15);
                gradient[0] += hazard_ratio / sigma;
                for (std::size_t k = 0; k < p; ++k) {
                    gradient[k + 1] += aligned_covariates[i][k] * hazard_ratio / sigma;
                }
                gradient[p + 1] += hazard_ratio * z;
            }
        }
        Matrix inverse;
        if (!invert_matrix(information, inverse)) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "singular_information", "MLE 信息矩阵奇异。");
            return result;
        }
        std::vector<double> step(param_count, 0.0);
        for (std::size_t row = 0; row < param_count; ++row) {
            for (std::size_t col = 0; col < param_count; ++col) {
                step[row] += inverse[row][col] * gradient[col];
            }
        }
        betas[0] += step[0];
        for (std::size_t k = 0; k < p; ++k) {
            betas[k + 1] += step[k + 1];
        }
        log_sigma += step[p + 1];
        if (std::sqrt(std::inner_product(step.cbegin(), step.cend(), step.cbegin(), 0.0))
            < 1.0e-6) {
            result.converged = true;
            break;
        }
    }

    result.log_sigma = log_sigma;
    result.log_likelihood = lognormal_log_likelihood(
        betas, log_sigma, log_times, aligned_events, aligned_covariates);

    Matrix information(param_count, std::vector<double>(param_count, 0.0));
    const double sigma = std::exp(log_sigma);
    for (std::size_t i = 0; i < log_times.size(); ++i) {
        double eta = betas[0];
        for (std::size_t k = 0; k < p; ++k) {
            eta += betas[k + 1] * aligned_covariates[i][k];
        }
        const double z = (log_times[i] - eta) / sigma;
        if (aligned_events[i]) {
            information[0][0] += 1.0 / (sigma * sigma);
            for (std::size_t k = 0; k < p; ++k) {
                information[0][k + 1] += aligned_covariates[i][k] / (sigma * sigma);
                information[k + 1][0] = information[0][k + 1];
                information[k + 1][k + 1] += aligned_covariates[i][k]
                    * aligned_covariates[i][k] / (sigma * sigma);
            }
        }
    }
    Matrix cov;
    if (!invert_matrix(information, cov)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "covariance_failed", "无法计算系数协方差。");
    }

    result.coefficients.push_back(make_coefficient(
        "Intercept", betas[0],
        cov.empty() ? 0.0 : std::sqrt(std::max(0.0, cov[0][0])),
        options.confidence_level));
    for (std::size_t k = 0; k < p; ++k) {
        const std::string label = k < covariate_labels.size()
            ? covariate_labels[k] : ("X" + std::to_string(k + 1));
        result.coefficients.push_back(make_coefficient(
            label, betas[k + 1],
            cov.empty() ? 0.0 : std::sqrt(std::max(0.0, cov[k + 1][k + 1])),
            options.confidence_level));
    }

    double profile_eta = betas[0];
    for (const auto& row : options.percentile_levels) {
        LifeDataLognormalPercentile pct;
        pct.percentile = row;
        const double z_p = normal_quantile(row / 100.0);
        pct.life = std::exp(profile_eta + sigma * z_p);
        pct.covariate_profile = "reference";
        result.percentiles.push_back(pct);
    }

    if (!result.converged) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "life_lognormal_not_converged", "Lognormal MLE 未完全收敛。");
    }

    return result;
}

}  // namespace datalab::domain::statistics
