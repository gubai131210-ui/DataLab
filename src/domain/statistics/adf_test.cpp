#include "domain/statistics/adf_test.h"

#include <cmath>
#include <vector>

namespace datalab::domain::statistics {
namespace {

struct MacKinnonCritical {
    double c1 = 0.0;
    double c5 = 0.0;
    double c10 = 0.0;
};

// Large-sample MacKinnon-style critical values for tau (unit-root null).
MacKinnonCritical critical_values(AdfRegression regression)
{
    switch (regression) {
    case AdfRegression::none:
        return {-2.5658, -1.9410, -1.6167};
    case AdfRegression::drift:
        return {-3.4304, -2.8615, -2.5668};
    case AdfRegression::trend:
        return {-3.9588, -3.4105, -3.1271};
    }
    return {-3.4304, -2.8615, -2.5668};
}

bool solve_normal_equations(
    std::vector<std::vector<double>> normal,
    std::vector<double> rhs,
    std::vector<double>& beta)
{
    const std::size_t columns = rhs.size();
    if (columns == 0 || normal.size() != columns) {
        return false;
    }
    std::vector<std::vector<double>> augmented = normal;
    for (std::size_t row = 0; row < columns; ++row) {
        augmented[row].push_back(rhs[row]);
    }
    for (std::size_t column = 0; column < columns; ++column) {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < columns; ++row) {
            if (std::abs(augmented[row][column]) > std::abs(augmented[pivot][column])) {
                pivot = row;
            }
        }
        if (std::abs(augmented[pivot][column]) < 1.0e-12) {
            return false;
        }
        std::swap(augmented[pivot], augmented[column]);
        const double divisor = augmented[column][column];
        for (double& value : augmented[column]) {
            value /= divisor;
        }
        for (std::size_t row = 0; row < columns; ++row) {
            if (row == column) {
                continue;
            }
            const double factor = augmented[row][column];
            for (std::size_t value = 0; value <= columns; ++value) {
                augmented[row][value] -= factor * augmented[column][value];
            }
        }
    }
    beta.assign(columns, 0.0);
    for (std::size_t row = 0; row < columns; ++row) {
        beta[row] = augmented[row][columns];
        if (!std::isfinite(beta[row])) {
            return false;
        }
    }
    return true;
}

}  // namespace

AdfResult augmented_dickey_fuller(
    const std::vector<double>& series,
    const AdfOptions& options)
{
    AdfResult result;
    result.regression = options.regression;
    std::vector<double> values;
    values.reserve(series.size());
    for (double value : series) {
        if (std::isfinite(value)) {
            values.push_back(value);
        } else {
            ++result.missing_count;
        }
    }
    result.n = values.size();
    if (values.size() < 8) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "adf_insufficient_n",
            "ADF 至少需要约 8 个有效观测。"});
        return result;
    }

    std::size_t lags = options.lags;
    if (lags == 0) {
        const double base = static_cast<double>(values.size() - 1);
        lags = static_cast<std::size_t>(std::floor(std::pow(base, 1.0 / 3.0)));
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::info, "adf_default_lags",
            "默认滞后 p = floor((T-1)^(1/3)) = " + std::to_string(lags) + "。"});
    }
    result.lags = lags;

    const std::size_t start = lags + 1;
    if (values.size() <= start + 2) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "adf_lags_too_large",
            "滞后过大，有效回归行不足。"});
        return result;
    }

    std::size_t column_count = 1 + lags;  // gamma + delta lags
    if (options.regression == AdfRegression::drift
        || options.regression == AdfRegression::trend) {
        ++column_count;  // intercept
    }
    if (options.regression == AdfRegression::trend) {
        ++column_count;  // trend
    }

    std::vector<std::vector<double>> design;
    std::vector<double> response;
    for (std::size_t t = start; t < values.size(); ++t) {
        std::vector<double> row;
        row.reserve(column_count);
        if (options.regression == AdfRegression::drift
            || options.regression == AdfRegression::trend) {
            row.push_back(1.0);
        }
        if (options.regression == AdfRegression::trend) {
            row.push_back(static_cast<double>(t));
        }
        row.push_back(values[t - 1]);  // y_{t-1} coefficient = gamma
        for (std::size_t lag = 1; lag <= lags; ++lag) {
            row.push_back(values[t - lag] - values[t - lag - 1]);
        }
        design.push_back(std::move(row));
        response.push_back(values[t] - values[t - 1]);
    }
    result.used_observations = response.size();

    std::vector<std::vector<double>> normal(
        column_count, std::vector<double>(column_count, 0.0));
    std::vector<double> rhs(column_count, 0.0);
    for (std::size_t row = 0; row < design.size(); ++row) {
        for (std::size_t i = 0; i < column_count; ++i) {
            rhs[i] += design[row][i] * response[row];
            for (std::size_t j = 0; j < column_count; ++j) {
                normal[i][j] += design[row][i] * design[row][j];
            }
        }
    }

    std::vector<double> beta;
    if (!solve_normal_equations(normal, rhs, beta)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "adf_singular",
            "ADF 设计矩阵奇异，无法估计。"});
        return result;
    }

    double sse = 0.0;
    for (std::size_t row = 0; row < design.size(); ++row) {
        double fitted = 0.0;
        for (std::size_t column = 0; column < column_count; ++column) {
            fitted += design[row][column] * beta[column];
        }
        const double residual = response[row] - fitted;
        sse += residual * residual;
    }
    const std::size_t df = response.size() > column_count
        ? response.size() - column_count : 0;
    if (df == 0) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "adf_no_df",
            "自由度不足，无法计算标准误。"});
        return result;
    }
    const double sigma2 = sse / static_cast<double>(df);

    // Inverse of X'X via solving e_i columns.
    std::vector<double> diag_inv(column_count, 0.0);
    for (std::size_t column = 0; column < column_count; ++column) {
        std::vector<double> unit(column_count, 0.0);
        unit[column] = 1.0;
        std::vector<double> solution;
        if (!solve_normal_equations(normal, unit, solution)) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "adf_se_failed",
                "无法计算系数标准误。"});
            return result;
        }
        diag_inv[column] = solution[column];
    }

    auto push_coef = [&](const std::string& name, std::size_t index) {
        AdfCoefficient coefficient;
        coefficient.name = name;
        coefficient.estimate = beta[index];
        coefficient.standard_error =
            std::sqrt(std::max(0.0, sigma2 * diag_inv[index]));
        coefficient.t_statistic =
            coefficient.standard_error > 0.0
                ? coefficient.estimate / coefficient.standard_error
                : 0.0;
        result.coefficients.push_back(coefficient);
    };

    std::size_t index = 0;
    if (options.regression == AdfRegression::drift
        || options.regression == AdfRegression::trend) {
        push_coef("Intercept", index++);
    }
    if (options.regression == AdfRegression::trend) {
        push_coef("Trend", index++);
    }
    const std::size_t gamma_index = index;
    push_coef("y(t-1)", index++);
    for (std::size_t lag = 1; lag <= lags; ++lag) {
        push_coef("dY(t-" + std::to_string(lag) + ")", index++);
    }

    result.gamma = beta[gamma_index];
    result.tau = result.coefficients[gamma_index].t_statistic;
    const MacKinnonCritical critical = critical_values(options.regression);
    result.critical_1 = critical.c1;
    result.critical_5 = critical.c5;
    result.critical_10 = critical.c10;
    result.reject_unit_root_at_5 =
        result.tau.has_value() && result.tau < critical.c5;
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "adf_critical_source",
        "临界值为大样本 MacKinnon 风格常数表；非 Minitab 导出 golden。"});
    return result;
}

}  // namespace datalab::domain::statistics
