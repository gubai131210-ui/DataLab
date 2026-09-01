#include "domain/statistics/binary_doe_probit.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace datalab::domain::statistics {
namespace {

using Matrix = std::vector<std::vector<double>>;

constexpr double kProbabilityFloor = 1.0e-9;
constexpr double kPivotTolerance = 1.0e-12;
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

double normal_pdf(double x)
{
    return std::exp(-0.5 * x * x) / std::sqrt(2.0 * kPi);
}

double normal_quantile(double p)
{
    double lower = -8.0;
    double upper = 8.0;
    for (int i = 0; i < 80; ++i) {
        const double mid = (lower + upper) / 2.0;
        if (normal_cdf(mid) < p) {
            lower = mid;
        } else {
            upper = mid;
        }
    }
    return (lower + upper) / 2.0;
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
    double scale = 0.0;
    for (std::size_t row = 0; row < size; ++row) {
        if (input[row].size() != size) {
            return false;
        }
        for (std::size_t column = 0; column < size; ++column) {
            augmented[row][column] = input[row][column];
            scale = std::max(scale, std::abs(input[row][column]));
        }
        augmented[row][size + row] = 1.0;
    }
    const double pivot_limit = std::max(1.0, scale) * kPivotTolerance;
    for (std::size_t column = 0; column < size; ++column) {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < size; ++row) {
            if (std::abs(augmented[row][column])
                > std::abs(augmented[pivot][column])) {
                pivot = row;
            }
        }
        if (!std::isfinite(augmented[pivot][column])
            || std::abs(augmented[pivot][column]) <= pivot_limit) {
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

double link_mu(const std::string& link, double eta)
{
    if (link == "gompit") {
        const double inner = std::exp(eta);
        return 1.0 - std::exp(-inner);
    }
    return normal_cdf(eta);
}

double link_derivative(const std::string& link, double eta, double mu)
{
    if (link == "gompit") {
        const double q = 1.0 - mu;
        if (q <= kProbabilityFloor || mu <= kProbabilityFloor) {
            return 1.0e-6;
        }
        const double log_q = std::log(q);
        if (std::abs(log_q) < 1.0e-12) {
            return 1.0e-6;
        }
        return -1.0 / (q * log_q);
    }
    const double pdf = normal_pdf(eta);
    return pdf > 0.0 ? 1.0 / pdf : 1.0e-6;
}

double inverse_link(const std::string& link, double mu)
{
    mu = std::clamp(mu, kProbabilityFloor, 1.0 - kProbabilityFloor);
    if (link == "gompit") {
        const double q = 1.0 - mu;
        return std::log(-std::log(q));
    }
    return normal_quantile(mu);
}

std::vector<std::vector<double>> build_factor_design(
    const std::vector<std::vector<std::string>>& factor_columns,
    const std::vector<std::string>& factor_labels,
    bool include_ab_interaction,
    std::vector<std::string>& predictor_labels)
{
    if (factor_columns.empty()) {
        return {};
    }
    const std::size_t row_count = factor_columns.front().size();
    std::vector<std::vector<std::string>> levels;
    levels.resize(factor_columns.size());
    for (std::size_t factor = 0; factor < factor_columns.size(); ++factor) {
        std::set<std::string> unique;
        for (std::size_t row = 0; row < row_count; ++row) {
            if (row < factor_columns[factor].size()
                && !factor_columns[factor][row].empty()) {
                unique.insert(factor_columns[factor][row]);
            }
        }
        levels[factor].assign(unique.cbegin(), unique.cend());
    }

    std::size_t column_count = 1;
    for (const auto& level_set : levels) {
        if (level_set.size() > 1) {
            column_count += level_set.size() - 1;
        }
    }
    if (include_ab_interaction && factor_columns.size() >= 2
        && levels[0].size() > 1 && levels[1].size() > 1) {
        column_count += (levels[0].size() - 1) * (levels[1].size() - 1);
    }

    predictor_labels.clear();
    predictor_labels.push_back("Intercept");
    for (std::size_t factor = 0; factor < levels.size(); ++factor) {
        const std::string prefix = factor < factor_labels.size()
            ? factor_labels[factor] : ("F" + std::to_string(factor + 1));
        for (std::size_t index = 1; index < levels[factor].size(); ++index) {
            predictor_labels.push_back(prefix + "[" + levels[factor][index] + "]");
        }
    }
    if (include_ab_interaction && factor_columns.size() >= 2) {
        for (std::size_t ia = 1; ia < levels[0].size(); ++ia) {
            for (std::size_t ib = 1; ib < levels[1].size(); ++ib) {
                predictor_labels.push_back(
                    "AB[" + levels[0][ia] + "×" + levels[1][ib] + "]");
            }
        }
    }

    std::vector<std::vector<double>> design(row_count,
                                            std::vector<double>(column_count, 0.0));
    for (std::size_t row = 0; row < row_count; ++row) {
        design[row][0] = 1.0;
        std::size_t column = 1;
        std::vector<std::vector<double>> dummy(factor_columns.size());
        for (std::size_t factor = 0; factor < factor_columns.size(); ++factor) {
            dummy[factor].assign(levels[factor].size() - 1, 0.0);
            if (row >= factor_columns[factor].size()) {
                continue;
            }
            for (std::size_t index = 1; index < levels[factor].size(); ++index) {
                if (factor_columns[factor][row] == levels[factor][index]) {
                    dummy[factor][index - 1] = 1.0;
                }
            }
        }
        for (const auto& block : dummy) {
            for (double value : block) {
                if (column < column_count) {
                    design[row][column++] = value;
                }
            }
        }
        if (include_ab_interaction && factor_columns.size() >= 2) {
            for (double value_a : dummy[0]) {
                for (double value_b : dummy[1]) {
                    if (column < column_count) {
                        design[row][column++] = value_a * value_b;
                    }
                }
            }
        }
    }
    return design;
}

}  // namespace

BinaryDoeProbitResult analyze_binary_doe_probit(
    const std::vector<std::vector<std::string>>& factor_columns,
    const std::vector<int>& events,
    const std::vector<int>& trials,
    const std::vector<std::string>& factor_labels,
    const std::vector<std::size_t>& source_rows,
    const BinaryDoeProbitOptions& options)
{
    BinaryDoeProbitResult result;
    result.link = options.link;
    result.include_ab_interaction = options.include_ab_interaction;
    result.factor_count = factor_columns.size();
    if (factor_columns.empty() || events.empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "binary_probit_empty", "需要至少一个因子列与响应。");
        return result;
    }
    const std::size_t row_count = factor_columns.front().size();
    result.design_row_count = row_count;

    std::vector<std::string> predictor_labels;
    const std::vector<std::vector<double>> design = build_factor_design(
        factor_columns, factor_labels, options.include_ab_interaction,
        predictor_labels);
    if (design.empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "binary_probit_design", "无法构建因子设计矩阵。");
        return result;
    }

    std::vector<int> response;
    std::vector<std::vector<double>> predictors;
    std::vector<std::size_t> expanded_source_rows;
    for (std::size_t row = 0; row < row_count; ++row) {
        const int event = row < events.size() ? events[row] : 0;
        const int trial = row < trials.size() ? trials[row] : 1;
        if (trial <= 0) {
            continue;
        }
        if (event < 0 || event > trial) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "binary_probit_events_trials",
                           "events 必须满足 0 ≤ events ≤ trials。");
            return result;
        }
        result.event_count += event;
        result.trial_count += trial;
        const std::size_t source = row < source_rows.size() ? source_rows[row] : row;
        for (int copy = 0; copy < trial; ++copy) {
            response.push_back(copy < event ? 1 : 0);
            predictors.push_back(row < design.size() ? design[row] : design.back());
            expanded_source_rows.push_back(source);
        }
    }
    result.expanded_observation_count = response.size();
    result.observation_source_rows = expanded_source_rows;
    if (response.size() < 4) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "binary_probit_insufficient",
                       "有效展开观测不足（至少 4 个 trial）。");
        return result;
    }

    const std::size_t k = predictors.front().size();
    std::vector<double> beta(k, 0.0);
    const std::string link = options.link == "gompit" ? "gompit" : "probit";

    for (std::size_t iteration = 0; iteration < 100; ++iteration) {
        std::vector<double> eta(response.size());
        std::vector<double> mu(response.size());
        std::vector<double> weight(response.size());
        std::vector<double> z(response.size());
        for (std::size_t i = 0; i < response.size(); ++i) {
            eta[i] = 0.0;
            for (std::size_t j = 0; j < k; ++j) {
                eta[i] += predictors[i][j] * beta[j];
            }
            mu[i] = link_mu(link, eta[i]);
            const double deriv = link_derivative(link, eta[i], mu[i]);
            weight[i] = deriv * deriv * mu[i] * (1.0 - mu[i]);
            weight[i] = std::max(weight[i], 1.0e-8);
            z[i] = eta[i] + (static_cast<double>(response[i]) - mu[i]) / deriv;
        }

        Matrix information(k, std::vector<double>(k, 0.0));
        std::vector<double> rhs(k, 0.0);
        for (std::size_t i = 0; i < response.size(); ++i) {
            for (std::size_t a = 0; a < k; ++a) {
                for (std::size_t b = 0; b < k; ++b) {
                    information[a][b] += predictors[i][a] * weight[i] * predictors[i][b];
                }
                rhs[a] += predictors[i][a] * weight[i] * z[i];
            }
        }
        Matrix inverse;
        if (!invert_matrix(information, inverse)) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "singular_information", "IRWLS 信息矩阵奇异。");
            return result;
        }
        std::vector<double> new_beta(k, 0.0);
        for (std::size_t a = 0; a < k; ++a) {
            for (std::size_t b = 0; b < k; ++b) {
                new_beta[a] += inverse[a][b] * rhs[b];
            }
        }
        double delta = 0.0;
        for (std::size_t j = 0; j < k; ++j) {
            delta += (new_beta[j] - beta[j]) * (new_beta[j] - beta[j]);
        }
        beta = std::move(new_beta);
        result.iteration_count = iteration + 1;
        if (std::sqrt(delta) < 1.0e-8) {
            result.converged = true;
            break;
        }
    }

    Matrix information(k, std::vector<double>(k, 0.0));
    for (std::size_t i = 0; i < response.size(); ++i) {
        double eta = 0.0;
        for (std::size_t j = 0; j < k; ++j) {
            eta += predictors[i][j] * beta[j];
        }
        const double mu = link_mu(link, eta);
        const double deriv = link_derivative(link, eta, mu);
        const double w = std::max(deriv * deriv * mu * (1.0 - mu), 1.0e-8);
        for (std::size_t a = 0; a < k; ++a) {
            for (std::size_t b = 0; b < k; ++b) {
                information[a][b] += predictors[i][a] * w * predictors[i][b];
            }
        }
    }
    Matrix cov;
    if (!invert_matrix(information, cov)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "covariance_failed", "无法计算系数协方差。");
    }

    double deviance = 0.0;
    for (std::size_t i = 0; i < response.size(); ++i) {
        double eta = 0.0;
        for (std::size_t j = 0; j < k; ++j) {
            eta += predictors[i][j] * beta[j];
        }
        const double mu = std::clamp(link_mu(link, eta), kProbabilityFloor,
                                     1.0 - kProbabilityFloor);
        if (response[i] == 1) {
            deviance += -2.0 * std::log(mu);
        } else {
            deviance += -2.0 * std::log(1.0 - mu);
        }
    }
    result.deviance = deviance;
    result.aic = deviance + 2.0 * static_cast<double>(k);

    for (std::size_t j = 0; j < k; ++j) {
        BinaryDoeProbitCoefficient coef;
        coef.term = predictor_labels[j];
        coef.coefficient = beta[j];
        if (!cov.empty()) {
            coef.standard_error = std::sqrt(std::max(0.0, cov[j][j]));
            if (coef.standard_error > 0.0) {
                coef.z_statistic = coef.coefficient / coef.standard_error;
                coef.p_value = two_sided_normal_p(coef.z_statistic);
            }
        }
        result.coefficients.push_back(coef);
    }

    if (!result.converged) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "binary_probit_not_converged", "Probit/Gompit IRWLS 未收敛。");
    }

    return result;
}

}  // namespace datalab::domain::statistics
