#include "domain/statistics/nonlinear_regression.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

using Matrix = std::vector<std::vector<double>>;

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const std::string& message)
{
    diagnostics.push_back({severity, std::string{code}, message, {}});
}

struct ModelSpec {
    std::size_t parameter_count = 0;
    std::function<double(const std::vector<double>&, double)> predict;
    std::function<std::vector<double>(const std::vector<double>&, double)> jacobian;
};

std::optional<ModelSpec> model_spec_for(const std::string& model_id)
{
    if (model_id == "growth") {
        return ModelSpec{
            3,
            [](const std::vector<double>& theta, double x) {
                return theta[0] - theta[1] * std::exp(-theta[2] * x);
            },
            [](const std::vector<double>& theta, double x) {
                const double exp_term = std::exp(-theta[2] * x);
                return std::vector<double>{1.0, -exp_term, theta[1] * x * exp_term};
            }};
    }
    if (model_id == "decay") {
        return ModelSpec{
            2,
            [](const std::vector<double>& theta, double x) {
                return theta[0] * std::exp(-theta[1] * x);
            },
            [](const std::vector<double>& theta, double x) {
                const double exp_term = std::exp(-theta[1] * x);
                return std::vector<double>{exp_term, -theta[0] * x * exp_term};
            }};
    }
    if (model_id == "logistic_saturation") {
        return ModelSpec{
            3,
            [](const std::vector<double>& theta, double x) {
                return theta[0] / (1.0 + std::exp(-theta[1] * (x - theta[2])));
            },
            [](const std::vector<double>& theta, double x) {
                const double z = std::exp(-theta[1] * (x - theta[2]));
                const double denom = 1.0 + z;
                const double denom2 = denom * denom;
                return std::vector<double>{1.0 / denom,
                        theta[0] * (x - theta[2]) * z / denom2,
                        -theta[0] * theta[1] * z / denom2};
            }};
    }
    if (model_id == "michaelis_menten") {
        return ModelSpec{
            2,
            [](const std::vector<double>& theta, double x) {
                return theta[0] * x / (theta[1] + x);
            },
            [](const std::vector<double>& theta, double x) {
                const double denom = theta[1] + x;
                const double denom2 = denom * denom;
                return std::vector<double>{x / denom, -theta[0] * x / denom2};
            }};
    }
    if (model_id == "power") {
        return ModelSpec{
            2,
            [](const std::vector<double>& theta, double x) {
                return theta[0] * std::pow(std::max(x, 1.0e-12), theta[1]);
            },
            [](const std::vector<double>& theta, double x) {
                const double safe_x = std::max(x, 1.0e-12);
                const double power = std::pow(safe_x, theta[1]);
                return std::vector<double>{power, theta[0] * power * std::log(safe_x)};
            }};
    }
    return std::nullopt;
}

std::vector<double> default_starting_values(
    const std::string& model_id,
    const std::vector<double>& response,
    const std::vector<double>& predictor)
{
    const double y_max = *std::max_element(response.cbegin(), response.cend());
    const double y_min = *std::min_element(response.cbegin(), response.cend());
    const double x_max = *std::max_element(predictor.cbegin(), predictor.cend());
    if (model_id == "growth") {
        return {y_max, y_max - y_min, 0.1};
    }
    if (model_id == "decay") {
        return {y_max, 0.1};
    }
    if (model_id == "logistic_saturation") {
        return {y_max, 1.0, x_max * 0.5};
    }
    if (model_id == "michaelis_menten") {
        return {y_max, x_max * 0.5};
    }
    if (model_id == "power") {
        return {1.0, 1.0};
    }
    return {};
}

bool invert_matrix(Matrix matrix, Matrix& inverse)
{
    const std::size_t n = matrix.size();
    if (n == 0) {
        return false;
    }
    inverse.assign(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        inverse[i][i] = 1.0;
    }
    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivot = col;
        for (std::size_t row = col + 1; row < n; ++row) {
            if (std::abs(matrix[row][col]) > std::abs(matrix[pivot][col])) {
                pivot = row;
            }
        }
        if (std::abs(matrix[pivot][col]) < 1.0e-12) {
            return false;
        }
        std::swap(matrix[pivot], matrix[col]);
        std::swap(inverse[pivot], inverse[col]);
        const double div = matrix[col][col];
        for (std::size_t j = 0; j < n; ++j) {
            matrix[col][j] /= div;
            inverse[col][j] /= div;
        }
        for (std::size_t row = 0; row < n; ++row) {
            if (row == col) {
                continue;
            }
            const double factor = matrix[row][col];
            for (std::size_t j = 0; j < n; ++j) {
                matrix[row][j] -= factor * matrix[col][j];
                inverse[row][j] -= factor * inverse[col][j];
            }
        }
    }
    return true;
}

Matrix multiply(const Matrix& left, const Matrix& right)
{
    const std::size_t n = left.size();
    const std::size_t m = right.empty() ? 0 : right.front().size();
    const std::size_t inner = right.size();
    Matrix product(n, std::vector<double>(m, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < m; ++j) {
            for (std::size_t k = 0; k < inner; ++k) {
                product[i][j] += left[i][k] * right[k][j];
            }
        }
    }
    return product;
}

Matrix transpose(const Matrix& input)
{
    if (input.empty()) {
        return {};
    }
    const std::size_t rows = input.size();
    const std::size_t cols = input.front().size();
    Matrix output(cols, std::vector<double>(rows, 0.0));
    for (std::size_t i = 0; i < rows; ++i) {
        for (std::size_t j = 0; j < cols; ++j) {
            output[j][i] = input[i][j];
        }
    }
    return output;
}

double compute_sse(
    const ModelSpec& spec,
    const std::vector<double>& theta,
    const std::vector<double>& response,
    const std::vector<double>& predictor)
{
    double sse = 0.0;
    for (std::size_t i = 0; i < response.size(); ++i) {
        const double fitted = spec.predict(theta, predictor[i]);
        const double residual = response[i] - fitted;
        sse += residual * residual;
    }
    return sse;
}

std::vector<std::string> parameter_names(const std::string& model_id)
{
    if (model_id == "growth") {
        return {"a", "b", "c"};
    }
    if (model_id == "decay") {
        return {"a", "b"};
    }
    if (model_id == "logistic_saturation") {
        return {"a", "b", "c"};
    }
    if (model_id == "michaelis_menten") {
        return {"V", "K"};
    }
    if (model_id == "power") {
        return {"a", "b"};
    }
    return {};
}

}  // namespace

NonlinearRegressionResult fit_nonlinear_regression(
    const std::vector<double>& response,
    const std::vector<double>& predictor,
    const std::vector<std::size_t>& source_rows,
    const NonlinearRegressionOptions& options)
{
    NonlinearRegressionResult result;
    result.model_id = options.model_id;
    result.algorithm = options.algorithm;
    if (response.size() != predictor.size() || response.size() < 3) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "nonlinear_insufficient_data",
                       "非线性回归需要至少 3 个有效观测。");
        return result;
    }
    result.observation_count = response.size();
    if (!source_rows.empty()) {
        result.observation_source_rows = source_rows;
    } else {
        result.observation_source_rows.resize(response.size());
        std::iota(result.observation_source_rows.begin(),
                  result.observation_source_rows.end(), 0);
    }

    const auto spec_opt = model_spec_for(options.model_id);
    if (!spec_opt.has_value()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "nonlinear_unknown_model", "未知内置模型 id。");
        return result;
    }
    const ModelSpec spec = *spec_opt;

    std::vector<double> theta = options.starting_values;
    if (theta.size() != spec.parameter_count) {
        theta = default_starting_values(options.model_id, response, predictor);
    }
    if (theta.size() != spec.parameter_count) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "nonlinear_bad_start", "初值数量与模型参数不匹配。");
        return result;
    }

    const bool use_lm = options.algorithm == "lm";
    double lambda = options.lm_lambda;
    result.converged = false;

    for (std::size_t iteration = 0; iteration < options.max_iterations; ++iteration) {
        result.iteration_count = iteration + 1;
        Matrix jacobian(response.size(), std::vector<double>(spec.parameter_count, 0.0));
        std::vector<double> residuals(response.size(), 0.0);
        for (std::size_t i = 0; i < response.size(); ++i) {
            const double fitted = spec.predict(theta, predictor[i]);
            residuals[i] = response[i] - fitted;
            const std::vector<double> row = spec.jacobian(theta, predictor[i]);
            for (std::size_t p = 0; p < spec.parameter_count; ++p) {
                jacobian[i][p] = row[p];
            }
        }

        const Matrix jt = transpose(jacobian);
        Matrix jtj = multiply(jt, jacobian);
        if (use_lm) {
            for (std::size_t p = 0; p < spec.parameter_count; ++p) {
                jtj[p][p] += lambda;
            }
        }
        Matrix jtj_inv;
        if (!invert_matrix(jtj, jtj_inv)) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "nonlinear_singular_jacobian",
                           "Jacobian 矩阵奇异，无法继续迭代。");
            break;
        }
        const Matrix jt_r = multiply(jt, Matrix{residuals});
        Matrix delta = multiply(jtj_inv, jt_r);
        if (delta.empty() || delta.front().empty()) {
            break;
        }

        double max_relative = 0.0;
        for (std::size_t p = 0; p < spec.parameter_count; ++p) {
            const double step = delta[p][0];
            const double denom = std::max(std::abs(theta[p]), 1.0e-8);
            max_relative = std::max(max_relative, std::abs(step / denom));
            theta[p] += step;
        }

        if (use_lm) {
            const double old_sse = compute_sse(spec, theta, response, predictor);
            (void)old_sse;
            lambda = max_relative < options.tolerance ? lambda * 0.5 : lambda * 2.0;
        }

        if (max_relative < options.tolerance) {
            result.converged = true;
            break;
        }
    }

    if (!result.converged) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "nonlinear_not_converged",
                       "迭代未在容差内收敛；报告末次参数估计。");
    }

    result.sse = compute_sse(spec, theta, response, predictor);
    result.error_df = response.size() > spec.parameter_count
        ? response.size() - spec.parameter_count : 0;
    result.mse = result.error_df > 0 ? result.sse / static_cast<double>(result.error_df) : 0.0;
    result.s = std::sqrt(result.mse);

    const double y_mean = std::accumulate(response.cbegin(), response.cend(), 0.0)
        / static_cast<double>(response.size());
    double sst = 0.0;
    for (double y : response) {
        const double diff = y - y_mean;
        sst += diff * diff;
    }
    result.r_squared = sst > 0.0 ? 1.0 - result.sse / sst : 0.0;

    Matrix jacobian(response.size(), std::vector<double>(spec.parameter_count, 0.0));
    for (std::size_t i = 0; i < response.size(); ++i) {
        const std::vector<double> row = spec.jacobian(theta, predictor[i]);
        for (std::size_t p = 0; p < spec.parameter_count; ++p) {
            jacobian[i][p] = row[p];
        }
    }
    const Matrix jt = transpose(jacobian);
    Matrix jtj = multiply(jt, jacobian);
    Matrix cov;
    if (invert_matrix(jtj, cov) && result.mse > 0.0) {
        const auto names = parameter_names(options.model_id);
        for (std::size_t p = 0; p < spec.parameter_count; ++p) {
            NonlinearParameterEstimate estimate;
            estimate.name = p < names.size() ? names[p] : ("theta" + std::to_string(p));
            estimate.estimate = theta[p];
            estimate.standard_error = std::sqrt(std::max(0.0, cov[p][p] * result.mse));
            const double margin = 1.96 * estimate.standard_error;
            estimate.lower_ci = estimate.estimate - margin;
            estimate.upper_ci = estimate.estimate + margin;
            result.parameters.push_back(std::move(estimate));
        }
    }

    return result;
}

}  // namespace datalab::domain::statistics
