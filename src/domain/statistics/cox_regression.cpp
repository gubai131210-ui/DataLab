#include "domain/statistics/cox_regression.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

namespace datalab::domain::statistics {
namespace {

using Matrix = std::vector<std::vector<double>>;
constexpr double kPivotTolerance = 1.0e-12;

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const char* message)
{
    diagnostics.push_back({severity, code, message});
}

double normal_quantile(double probability)
{
    double lower = -9.0;
    double upper = 9.0;
    for (int iteration = 0; iteration < 80; ++iteration) {
        const double middle = (lower + upper) / 2.0;
        const double cdf = 0.5 * std::erfc(-middle / std::sqrt(2.0));
        if (cdf < probability) {
            lower = middle;
        } else {
            upper = middle;
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

std::string covariate_term(
    std::size_t index, const std::vector<std::string>& labels)
{
    return labels.empty() ? "X" + std::to_string(index + 1) : labels[index];
}

struct SubjectRow {
    double time = 0.0;
    bool event = false;
    std::vector<double> covariates;
    std::size_t source_row = 0;
};

double linear_predictor(
    const std::vector<double>& covariates,
    const std::vector<double>& beta)
{
    return std::inner_product(covariates.cbegin(), covariates.cend(),
                              beta.cbegin(), 0.0);
}

double partial_log_likelihood(
    const std::vector<SubjectRow>& rows,
    const std::vector<double>& beta)
{
    double loglik = 0.0;
    const std::size_t n = rows.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (!rows[i].event) {
            continue;
        }
        const double event_time = rows[i].time;
        const double eta_i = linear_predictor(rows[i].covariates, beta);
        double risk_sum = 0.0;
        for (std::size_t j = 0; j < n; ++j) {
            if (rows[j].time >= event_time) {
                risk_sum += std::exp(linear_predictor(rows[j].covariates, beta));
            }
        }
        if (!(risk_sum > 0.0) || !std::isfinite(risk_sum)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        loglik += eta_i - std::log(risk_sum);
    }
    return loglik;
}

bool compute_score_information(
    const std::vector<SubjectRow>& rows,
    const std::vector<double>& beta,
    std::vector<double>& score,
    Matrix& information)
{
    const std::size_t p = beta.size();
    const std::size_t n = rows.size();
    score.assign(p, 0.0);
    information.assign(p, std::vector<double>(p, 0.0));

    for (std::size_t i = 0; i < n; ++i) {
        if (!rows[i].event) {
            continue;
        }
        const double event_time = rows[i].time;
        double risk_sum = 0.0;
        std::vector<double> weighted_mean(p, 0.0);
        Matrix weighted_outer(p, std::vector<double>(p, 0.0));
        for (std::size_t j = 0; j < n; ++j) {
            if (rows[j].time < event_time) {
                continue;
            }
            const double weight = std::exp(linear_predictor(rows[j].covariates, beta));
            if (!std::isfinite(weight)) {
                return false;
            }
            risk_sum += weight;
            for (std::size_t k = 0; k < p; ++k) {
                weighted_mean[k] += weight * rows[j].covariates[k];
                for (std::size_t l = 0; l < p; ++l) {
                    weighted_outer[k][l] += weight * rows[j].covariates[k]
                        * rows[j].covariates[l];
                }
            }
        }
        if (!(risk_sum > 0.0)) {
            return false;
        }
        for (std::size_t k = 0; k < p; ++k) {
            weighted_mean[k] /= risk_sum;
            score[k] += rows[i].covariates[k] - weighted_mean[k];
            for (std::size_t l = 0; l < p; ++l) {
                information[k][l] += weighted_outer[k][l] / risk_sum
                    - weighted_mean[k] * weighted_mean[l];
            }
        }
    }
    return std::all_of(score.cbegin(), score.cend(),
                       [](double value) { return std::isfinite(value); });
}

}  // namespace

CoxRegressionResult fit_cox_regression(
    const std::vector<double>& times,
    const std::vector<bool>& events,
    const std::vector<std::vector<double>>& covariates,
    const std::vector<std::string>& covariate_labels,
    const std::vector<std::size_t>& source_rows,
    const double confidence_level,
    const std::string& ties_method,
    const std::size_t max_iterations,
    const double tolerance)
{
    CoxRegressionResult result;
    result.ties_method = ties_method;
    result.evidence_type = "formula_reference";
    result.algorithm_id = "cox_ph_fixed_covariates";
    result.source_rows = source_rows;

    if (times.size() < 2 || times.size() != events.size()
        || times.size() != covariates.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_cox_shape",
                       "Cox 回归要求寿命、事件与协变量矩阵行数一致且至少 2 行。");
        return result;
    }
    if (covariates.empty() || covariates.front().empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_cox_covariates",
                       "Cox 回归至少需要一个协变量。");
        return result;
    }
    if (!covariate_labels.empty()
        && covariate_labels.size() != covariates.front().size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_cox_labels",
                       "协变量标签数量必须与协变量列数一致。");
        return result;
    }
    if (!(confidence_level > 0.0 && confidence_level < 1.0)
        || !(tolerance > 0.0) || max_iterations == 0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_cox_options",
                       "置信水平、收敛容差和最大迭代次数必须有效。");
        return result;
    }
    if (ties_method != "breslow" && ties_method != "efron") {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_cox_ties",
                       "ties_method 必须为 breslow 或 efron。");
        return result;
    }
    if (ties_method == "efron") {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "cox_efron_narrow",
                       "Efron ties 为窄化实现；默认 Breslow 为 formula_reference 主路径。");
    }

    const std::size_t predictor_count = covariates.front().size();
    std::vector<SubjectRow> rows;
    rows.reserve(times.size());
    std::size_t event_count = 0;
    for (std::size_t index = 0; index < times.size(); ++index) {
        if (!std::isfinite(times[index]) || times[index] <= 0.0) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_cox_time",
                           "寿命时间必须为正有限数值。");
            return result;
        }
        if (covariates[index].size() != predictor_count) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "ragged_cox_covariates",
                           "每行协变量必须具有相同列数。");
            return result;
        }
        for (double value : covariates[index]) {
            if (!std::isfinite(value)) {
                add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                               "invalid_cox_covariate",
                               "协变量必须全部为有限数值。");
                return result;
            }
        }
        SubjectRow row;
        row.time = times[index];
        row.event = events[index];
        row.covariates = covariates[index];
        row.source_row = index < source_rows.size() ? source_rows[index] : index;
        if (row.event) {
            ++event_count;
        }
        rows.push_back(std::move(row));
    }

    result.n = rows.size();
    result.events = event_count;
    result.censored = rows.size() - event_count;
    if (event_count == 0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "cox_no_events",
                       "Cox 回归至少需要一次失效事件。");
        return result;
    }
    if (rows.size() <= predictor_count) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_cox_degrees_of_freedom",
                       "Cox 回归需要多于协变量数量的观测。");
        return result;
    }

    std::sort(rows.begin(), rows.end(), [](const SubjectRow& left, const SubjectRow& right) {
        if (left.time != right.time) {
            return left.time < right.time;
        }
        return left.event && !right.event;
    });

    std::vector<double> beta(predictor_count, 0.0);
    bool converged = false;
    for (std::size_t iteration = 0; iteration < max_iterations; ++iteration) {
        std::vector<double> score;
        Matrix information;
        if (!compute_score_information(rows, beta, score, information)) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "cox_singular_information",
                           "Cox 偏似然信息矩阵奇异，无法继续 Newton–Raphson。");
            return result;
        }
        Matrix inverse;
        if (!invert_matrix(information, inverse)) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "cox_singular_information",
                           "Cox 偏似然信息矩阵不可逆。");
            return result;
        }
        std::vector<double> step(predictor_count, 0.0);
        double maximum_step = 0.0;
        for (std::size_t k = 0; k < predictor_count; ++k) {
            for (std::size_t l = 0; l < predictor_count; ++l) {
                step[k] += inverse[k][l] * score[l];
            }
            maximum_step = std::max(maximum_step, std::abs(step[k]));
        }
        for (std::size_t k = 0; k < predictor_count; ++k) {
            beta[k] += step[k];
        }
        if (maximum_step < tolerance) {
            converged = true;
            break;
        }
    }

    result.converged = converged;
    result.log_likelihood = partial_log_likelihood(rows, beta);
    if (!std::isfinite(result.log_likelihood)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "cox_invalid_loglik",
                       "Cox 偏对数似然非有限值。");
        return result;
    }

    std::vector<double> final_score;
    Matrix final_information;
    if (!compute_score_information(rows, beta, final_score, final_information)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "cox_singular_information",
                       "收敛后无法计算 Cox 系数标准误。");
        return result;
    }
    Matrix covariance;
    if (!invert_matrix(final_information, covariance)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "cox_singular_information",
                       "无法求 Cox 信息矩阵逆以计算标准误。");
        return result;
    }

    const double critical = normal_quantile(0.5 + confidence_level / 2.0);
    result.coefficients.reserve(predictor_count);
    for (std::size_t index = 0; index < predictor_count; ++index) {
        CoxRegressionCoefficient coefficient;
        coefficient.term = covariate_term(index, covariate_labels);
        coefficient.beta = beta[index];
        coefficient.standard_error = std::sqrt(std::max(0.0, covariance[index][index]));
        coefficient.z_statistic = coefficient.standard_error > 0.0
            ? coefficient.beta / coefficient.standard_error : 0.0;
        coefficient.p_value = two_sided_normal_p(coefficient.z_statistic);
        coefficient.hazard_ratio = std::exp(std::clamp(coefficient.beta, -700.0, 700.0));
        coefficient.confidence_lower = std::exp(
            std::clamp(coefficient.beta - critical * coefficient.standard_error,
                       -700.0, 700.0));
        coefficient.confidence_upper = std::exp(
            std::clamp(coefficient.beta + critical * coefficient.standard_error,
                       -700.0, 700.0));
        result.coefficients.push_back(coefficient);
    }

    if (!converged) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "cox_not_converged",
                       "Cox Newton–Raphson 未在最大迭代次数内收敛。");
    }
    add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                   "cox_scope",
                   "固定协变量 Cox PH（Breslow ties 偏似然）；右删失 complete-case；"
                   "formula_reference ≠ Minitab golden。");
    add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                   "assumption_not_verified",
                   "比例风险与协变量线性未验证；HR 为相对风险证据，非因果证明。");
    return result;
}

}  // namespace datalab::domain::statistics
