#include "domain/statistics/multivariate_control.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace datalab::domain::statistics {
namespace {

void add_error(std::vector<DiagnosticMessage>& diagnostics,
               const char* code,
               const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

void add_warning(std::vector<DiagnosticMessage>& diagnostics,
                 const char* code,
                 const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::warning, code, message});
}

void add_info(std::vector<DiagnosticMessage>& diagnostics,
              const char* code,
              const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::info, code, message});
}

double clamp01(double value)
{
    return std::max(0.0, std::min(1.0, value));
}

double beta_continued_fraction(double a, double b, double x)
{
    constexpr int kMaxIterations = 200;
    constexpr double kEpsilon = 1.0e-12;
    double aa = 1.0;
    double c = 1.0;
    double d = 1.0 - (a + b) * x / (a + 1.0);
    if (std::abs(d) < 1.0e-30) {
        d = 1.0e-30;
    }
    d = 1.0 / d;
    double result = d;
    for (int m = 1; m <= kMaxIterations; ++m) {
        const int m2 = 2 * m;
        aa = m * (b - m) * x / ((a + m2 - 1.0) * (a + m2));
        d = 1.0 + aa * d;
        if (std::abs(d) < 1.0e-30) {
            d = 1.0e-30;
        }
        c = 1.0 + aa / c;
        if (std::abs(c) < 1.0e-30) {
            c = 1.0e-30;
        }
        d = 1.0 / d;
        result *= d * c;
        aa = -(a + m) * (a + b + m) * x / ((a + m2) * (a + m2 + 1.0));
        d = 1.0 + aa * d;
        if (std::abs(d) < 1.0e-30) {
            d = 1.0e-30;
        }
        c = 1.0 + aa / c;
        if (std::abs(c) < 1.0e-30) {
            c = 1.0e-30;
        }
        d = 1.0 / d;
        const double delta = d * c;
        result *= delta;
        if (std::abs(delta - 1.0) < kEpsilon) {
            break;
        }
    }
    return result;
}

double regularized_beta(double x, double a, double b)
{
    if (!(x >= 0.0) || !(x <= 1.0) || !(a > 0.0) || !(b > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (x == 0.0 || x == 1.0) {
        return x;
    }
    const double factor = std::exp(std::lgamma(a + b) - std::lgamma(a) - std::lgamma(b)
                                   + a * std::log(x) + b * std::log(1.0 - x));
    if (x < (a + 1.0) / (a + b + 2.0)) {
        return factor * beta_continued_fraction(a, b, x) / a;
    }
    return 1.0 - factor * beta_continued_fraction(b, a, 1.0 - x) / b;
}

double beta_quantile(double probability, double a, double b)
{
    probability = clamp01(probability);
    if (probability <= 0.0) {
        return 0.0;
    }
    if (probability >= 1.0) {
        return 1.0;
    }
    double low = 0.0;
    double high = 1.0;
    for (int iter = 0; iter < 80; ++iter) {
        const double mid = 0.5 * (low + high);
        if (regularized_beta(mid, a, b) < probability) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return 0.5 * (low + high);
}

double regularized_gamma_q(double shape, double value)
{
    if (!(shape > 0.0) || !(value >= 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (value == 0.0) {
        return 1.0;
    }
    // Series / continued fraction hybrid (same spirit as elsewhere in repo).
    if (value < shape + 1.0) {
        double sum = 1.0 / shape;
        double term = sum;
        for (int n = 1; n < 200; ++n) {
            term *= value / (shape + n);
            sum += term;
            if (std::abs(term) < 1.0e-12 * std::abs(sum)) {
                break;
            }
        }
        const double p = sum * std::exp(-value + shape * std::log(value) - std::lgamma(shape));
        return clamp01(1.0 - p);
    }
    double a = 1.0 + value - shape;
    double b = 1.0;
    double c = 1.0 / 1.0e-30;
    double d = 1.0 / a;
    double h = d;
    for (int i = 1; i < 200; ++i) {
        const double an = -i * (i - shape);
        a += 2.0;
        d = an * d + a;
        if (std::abs(d) < 1.0e-30) {
            d = 1.0e-30;
        }
        c = a + an / c;
        if (std::abs(c) < 1.0e-30) {
            c = 1.0e-30;
        }
        d = 1.0 / d;
        const double delta = d * c;
        h *= delta;
        if (std::abs(delta - 1.0) < 1.0e-12) {
            break;
        }
    }
    const double q = std::exp(-value + shape * std::log(value) - std::lgamma(shape)) * h;
    return clamp01(q);
}

double chi_square_quantile(double probability, double df)
{
    probability = clamp01(probability);
    if (probability <= 0.0) {
        return 0.0;
    }
    double low = 0.0;
    double high = std::max(1.0, df);
    while (1.0 - regularized_gamma_q(df / 2.0, high / 2.0) < probability && high < 1.0e8) {
        high *= 2.0;
    }
    for (int iter = 0; iter < 80; ++iter) {
        const double mid = 0.5 * (low + high);
        if (1.0 - regularized_gamma_q(df / 2.0, mid / 2.0) < probability) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return 0.5 * (low + high);
}

double f_quantile(double probability, double numerator_df, double denominator_df)
{
    probability = clamp01(probability);
    if (!(numerator_df > 0.0) || !(denominator_df > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    // F = (d2/d1) * (beta^{-1} / (1 - beta^{-1})) with beta(d1/2, d2/2)
    const double y = beta_quantile(probability, numerator_df / 2.0, denominator_df / 2.0);
    if (!(y > 0.0) || !(y < 1.0)) {
        return y <= 0.0 ? 0.0 : std::numeric_limits<double>::infinity();
    }
    return (denominator_df / numerator_df) * (y / (1.0 - y));
}

bool invert_matrix(std::vector<std::vector<double>> matrix,
                   std::vector<std::vector<double>>& inverse)
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
        if (!(std::abs(matrix[pivot][col]) > 1.0e-12)) {
            return false;
        }
        std::swap(matrix[col], matrix[pivot]);
        std::swap(inverse[col], inverse[pivot]);
        const double scale = matrix[col][col];
        for (std::size_t j = 0; j < n; ++j) {
            matrix[col][j] /= scale;
            inverse[col][j] /= scale;
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

double mahalanobis_squared(const std::vector<double>& centered,
                           const std::vector<std::vector<double>>& inverse)
{
    const std::size_t p = centered.size();
    std::vector<double> temp(p, 0.0);
    for (std::size_t i = 0; i < p; ++i) {
        for (std::size_t j = 0; j < p; ++j) {
            temp[i] += inverse[i][j] * centered[j];
        }
    }
    double value = 0.0;
    for (std::size_t i = 0; i < p; ++i) {
        value += centered[i] * temp[i];
    }
    return value;
}

bool build_mean_cov(const std::vector<std::vector<double>>& rows,
                    std::vector<double>& mean,
                    std::vector<std::vector<double>>& cov)
{
    if (rows.empty() || rows.front().empty()) {
        return false;
    }
    const std::size_t m = rows.size();
    const std::size_t p = rows.front().size();
    mean.assign(p, 0.0);
    for (const auto& row : rows) {
        if (row.size() != p) {
            return false;
        }
        for (std::size_t j = 0; j < p; ++j) {
            if (!std::isfinite(row[j])) {
                return false;
            }
            mean[j] += row[j];
        }
    }
    for (double& value : mean) {
        value /= static_cast<double>(m);
    }
    cov.assign(p, std::vector<double>(p, 0.0));
    if (m < 2) {
        return false;
    }
    for (const auto& row : rows) {
        for (std::size_t i = 0; i < p; ++i) {
            for (std::size_t j = 0; j < p; ++j) {
                cov[i][j] += (row[i] - mean[i]) * (row[j] - mean[j]);
            }
        }
    }
    const double denom = static_cast<double>(m - 1);
    for (std::size_t i = 0; i < p; ++i) {
        for (std::size_t j = 0; j < p; ++j) {
            cov[i][j] /= denom;
        }
    }
    return true;
}

}  // namespace

HotellingT2Result hotelling_t2_individuals(
    const std::vector<std::vector<double>>& rows,
    const std::vector<std::size_t>& source_rows,
    const HotellingT2Options& options)
{
    HotellingT2Result result;
    result.observation_count = rows.size();
    if (rows.empty()) {
        add_error(result.diagnostics, "empty_matrix", "需要至少一行多元观测。");
        return result;
    }
    result.variable_count = rows.front().size();
    const std::size_t m = rows.size();
    const std::size_t p = result.variable_count;
    if (p < 2) {
        add_error(result.diagnostics, "need_multivariate", "Hotelling T² 至少需要两个变量列。");
        return result;
    }
    if (m <= p + 1) {
        add_error(result.diagnostics, "insufficient_observations",
                  "个体 T² Phase I 需要 m > p+1。");
        return result;
    }
    if (!source_rows.empty() && source_rows.size() != m) {
        add_error(result.diagnostics, "source_row_mismatch", "source_rows 长度必须匹配观测数。");
        return result;
    }
    if (!build_mean_cov(rows, result.mean_vector, result.covariance)) {
        add_error(result.diagnostics, "invalid_matrix", "无法估计均值/协方差（非有限或不矩形）。");
        return result;
    }
    std::vector<std::vector<double>> inverse;
    if (!invert_matrix(result.covariance, inverse)) {
        add_error(result.diagnostics, "singular_covariance", "样本协方差奇异，无法计算 T²。");
        return result;
    }
    result.t2.assign(m, 0.0);
    result.source_rows = source_rows.empty()
        ? std::vector<std::size_t>(m)
        : source_rows;
    if (source_rows.empty()) {
        for (std::size_t i = 0; i < m; ++i) {
            result.source_rows[i] = i;
        }
    }
    for (std::size_t i = 0; i < m; ++i) {
        std::vector<double> centered(p, 0.0);
        for (std::size_t j = 0; j < p; ++j) {
            centered[j] = rows[i][j] - result.mean_vector[j];
        }
        result.t2[i] = mahalanobis_squared(centered, inverse);
    }

    const double alpha = options.alpha > 0.0 && options.alpha < 1.0 ? options.alpha : 0.05;
    if (options.phase == "phase2") {
        // NIST Phase II individuals UCL (F form).
        const double scale = static_cast<double>(p) * (static_cast<double>(m) + 1.0)
            * (static_cast<double>(m) - 1.0)
            / (static_cast<double>(m) * static_cast<double>(m)
               - static_cast<double>(m) * static_cast<double>(p));
        const double fcrit = f_quantile(1.0 - alpha, static_cast<double>(p),
                                        static_cast<double>(m - p));
        result.upper_control_limit = scale * fcrit;
        result.limit_method = "phase2_f";
    } else {
        const double a = static_cast<double>(p) / 2.0;
        const double b = static_cast<double>(m - p - 1) / 2.0;
        const double beta = beta_quantile(1.0 - alpha, a, b);
        result.upper_control_limit =
            (static_cast<double>((m - 1) * (m - 1)) / static_cast<double>(m)) * beta;
        result.limit_method = "phase1_tracy_young_mason_beta";
        const double beta_low = beta_quantile(alpha, a, b);
        result.lower_control_limit =
            (static_cast<double>((m - 1) * (m - 1)) / static_cast<double>(m)) * beta_low;
    }
    for (const double value : result.t2) {
        if (value > result.upper_control_limit) {
            ++result.out_of_control_count;
        }
    }
    add_info(result.diagnostics, "not_pca_empirical_t2",
             "本命令是正式多元 Hotelling T² 控制图，不是 PCA 经验分位 T²。");
    return result;
}

MewmaResult mewma_chart(
    const std::vector<std::vector<double>>& rows,
    const std::vector<std::size_t>& source_rows,
    const MewmaOptions& options)
{
    MewmaResult result;
    result.lambda = options.lambda > 0.0 && options.lambda <= 1.0 ? options.lambda : 0.1;
    result.observation_count = rows.size();
    if (rows.empty()) {
        add_error(result.diagnostics, "empty_matrix", "需要至少一行多元观测。");
        return result;
    }
    result.variable_count = rows.front().size();
    const std::size_t m = rows.size();
    const std::size_t p = result.variable_count;
    if (p < 2) {
        add_error(result.diagnostics, "need_multivariate", "MEWMA 至少需要两个变量列。");
        return result;
    }
    if (m < 3) {
        add_error(result.diagnostics, "insufficient_observations", "MEWMA 至少需要 3 个观测。");
        return result;
    }
    std::vector<double> mean;
    std::vector<std::vector<double>> cov;
    if (!build_mean_cov(rows, mean, cov)) {
        add_error(result.diagnostics, "invalid_matrix", "无法估计均值/协方差。");
        return result;
    }
    result.source_rows = source_rows.empty() ? std::vector<std::size_t>(m) : source_rows;
    if (source_rows.empty()) {
        for (std::size_t i = 0; i < m; ++i) {
            result.source_rows[i] = i;
        }
    }
    const double lambda = result.lambda;
    std::vector<double> z(p, 0.0);
    result.t2.assign(m, 0.0);
    for (std::size_t i = 0; i < m; ++i) {
        if (rows[i].size() != p) {
            add_error(result.diagnostics, "ragged_matrix", "观测矩阵必须矩形。");
            return result;
        }
        for (std::size_t j = 0; j < p; ++j) {
            const double centered = rows[i][j] - mean[j];
            z[j] = lambda * centered + (1.0 - lambda) * z[j];
        }
        // Exact cov of Z_i when λ_k = λ for all k (NIST).
        const double factor = (lambda / (2.0 - lambda))
            * (1.0 - std::pow(1.0 - lambda, 2.0 * static_cast<double>(i + 1)));
        std::vector<std::vector<double>> cov_z(p, std::vector<double>(p, 0.0));
        for (std::size_t r = 0; r < p; ++r) {
            for (std::size_t c = 0; c < p; ++c) {
                cov_z[r][c] = factor * cov[r][c];
            }
        }
        std::vector<std::vector<double>> inverse;
        if (!invert_matrix(cov_z, inverse)) {
            add_error(result.diagnostics, "singular_mewma_cov",
                      "MEWMA 协方差在当前步奇异。");
            return result;
        }
        result.t2[i] = mahalanobis_squared(z, inverse);
    }

    const double alpha = options.alpha > 0.0 && options.alpha < 1.0 ? options.alpha : 0.05;
    if (options.upper_control_limit.has_value() && *options.upper_control_limit > 0.0) {
        result.upper_control_limit = *options.upper_control_limit;
        result.ucl_method = "user_specified";
    } else {
        // Asymptotic χ² approximation — not ARL-calibrated simulation constant.
        result.upper_control_limit = chi_square_quantile(1.0 - alpha, static_cast<double>(p));
        result.ucl_method = "asymptotic_chi_square_approx";
        add_warning(result.diagnostics, "mewma_ucl_not_arl_calibrated",
                    "默认 UCL 使用渐近 χ² 近似，不是仿真 ARL 校准常数；可手工指定 ucl。");
    }
    for (const double value : result.t2) {
        if (value > result.upper_control_limit) {
            ++result.out_of_control_count;
        }
    }
    add_info(result.diagnostics, "not_univariate_ewma",
             "MEWMA 是多元向量平滑；不要与单变量 EWMA 图混淆。");
    return result;
}

double matrix_determinant(std::vector<std::vector<double>> matrix)
{
    const std::size_t n = matrix.size();
    if (n == 0) {
        return 0.0;
    }
    double det = 1.0;
    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivot = col;
        for (std::size_t row = col + 1; row < n; ++row) {
            if (std::abs(matrix[row][col]) > std::abs(matrix[pivot][col])) {
                pivot = row;
            }
        }
        if (!(std::abs(matrix[pivot][col]) > 1.0e-14)) {
            return 0.0;
        }
        if (pivot != col) {
            std::swap(matrix[col], matrix[pivot]);
            det = -det;
        }
        det *= matrix[col][col];
        const double scale = matrix[col][col];
        for (std::size_t row = col + 1; row < n; ++row) {
            const double factor = matrix[row][col] / scale;
            for (std::size_t j = col; j < n; ++j) {
                matrix[row][j] -= factor * matrix[col][j];
            }
        }
    }
    return det;
}

GeneralizedVarianceResult generalized_variance_chart(
    const std::vector<std::vector<std::vector<double>>>& subgroups,
    const std::vector<std::size_t>& subgroup_source_rows)
{
    GeneralizedVarianceResult result;
    if (subgroups.empty()) {
        add_error(result.diagnostics, "empty_subgroups", "需要至少一个子组。");
        return result;
    }
    const std::size_t n = subgroups.front().size();
    if (n == 0 || subgroups.front().front().empty()) {
        add_error(result.diagnostics, "empty_subgroup", "子组不能为空。");
        return result;
    }
    const std::size_t p = subgroups.front().front().size();
    result.variable_count = p;
    result.subgroup_size = static_cast<int>(n);
    if (p < 2) {
        add_error(result.diagnostics, "need_multivariate", "GV 图至少需要两个变量。");
        return result;
    }
    if (n <= p) {
        add_error(result.diagnostics, "subgroup_too_small",
                  "广义方差图要求每个子组大小 n > 变量数 p。");
        return result;
    }
    for (const auto& subgroup : subgroups) {
        if (subgroup.size() != n) {
            add_error(result.diagnostics, "unequal_subgroups",
                      "广义方差图要求等量子组。");
            return result;
        }
        for (const auto& row : subgroup) {
            if (row.size() != p) {
                add_error(result.diagnostics, "ragged_subgroup",
                          "子组观测必须具有相同变量数。");
                return result;
            }
        }
    }

    double b1 = 1.0;
    for (std::size_t i = 1; i <= p; ++i) {
        b1 *= static_cast<double>(n - i);
    }
    b1 /= std::pow(static_cast<double>(n - 1), static_cast<double>(p));
    double product = 1.0;
    for (std::size_t i = 1; i <= p; ++i) {
        product *= static_cast<double>(n - i + 2)
            / (static_cast<double>(n - 1) * static_cast<double>(n - 1));
    }
    const double b2 = b1 * (product - b1);
    result.b1 = b1;
    result.b2 = b2;
    if (!(b1 > 0.0) || !(b2 >= 0.0)) {
        add_error(result.diagnostics, "invalid_b_constants", "b1/b2 常数无效。");
        return result;
    }

    double sum_det = 0.0;
    result.plotted_determinants.reserve(subgroups.size());
    for (std::size_t g = 0; g < subgroups.size(); ++g) {
        std::vector<double> mean(p, 0.0);
        for (const auto& row : subgroups[g]) {
            for (std::size_t j = 0; j < p; ++j) {
                if (!std::isfinite(row[j])) {
                    add_error(result.diagnostics, "non_finite", "子组含非有限值。");
                    return result;
                }
                mean[j] += row[j];
            }
        }
        for (double& value : mean) {
            value /= static_cast<double>(n);
        }
        std::vector<std::vector<double>> cov(p, std::vector<double>(p, 0.0));
        for (const auto& row : subgroups[g]) {
            for (std::size_t i = 0; i < p; ++i) {
                for (std::size_t j = 0; j < p; ++j) {
                    cov[i][j] += (row[i] - mean[i]) * (row[j] - mean[j]);
                }
            }
        }
        for (std::size_t i = 0; i < p; ++i) {
            for (std::size_t j = 0; j < p; ++j) {
                cov[i][j] /= static_cast<double>(n - 1);
            }
        }
        const double det = matrix_determinant(cov);
        result.plotted_determinants.push_back(det);
        sum_det += det;
        result.source_rows.push_back(
            g < subgroup_source_rows.size() ? subgroup_source_rows[g] : g * n);
    }
    result.subgroup_count = subgroups.size();
    const double mean_det = sum_det / static_cast<double>(subgroups.size());
    result.center_line = mean_det;
    result.sigma_determinant = mean_det / b1;
    result.upper_control_limit =
        result.sigma_determinant * (b1 + 3.0 * std::sqrt(b2));
    result.lower_control_limit = std::max(
        0.0, result.sigma_determinant * (b1 - 3.0 * std::sqrt(b2)));
    for (const double value : result.plotted_determinants) {
        if (value > result.upper_control_limit || value < result.lower_control_limit) {
            ++result.out_of_control_count;
        }
    }
    add_info(result.diagnostics, "gv_montgomery_subgroup",
             "广义方差图按 Montgomery |S| 子组公式；个体观测路径不做假 |S|。");
    add_warning(result.diagnostics, "gv_variability_chart_caveat",
                "NIST 指出多元变差图存在争议；本输出仅作 |S| 探索信号，不是唯一变差判定。");
    return result;
}

EmpCrossedResult emp_classification_from_components(
    double part_variance,
    double repeatability,
    double operator_variance,
    double interaction_variance)
{
    EmpCrossedResult result;
    const double part = std::max(0.0, part_variance);
    const double repeat = std::max(0.0, repeatability);
    const double oper = std::max(0.0, operator_variance);
    const double interaction = std::max(0.0, interaction_variance);
    const double denom_no_bias = part + repeat;
    const double denom_bias = part + repeat + oper;
    const double denom_interaction = part + repeat + oper + interaction;
    result.icc_no_bias = denom_no_bias > 0.0 ? part / denom_no_bias : 0.0;
    result.icc_with_bias = denom_bias > 0.0 ? part / denom_bias : 0.0;
    result.icc_with_interaction =
        denom_interaction > 0.0 ? part / denom_interaction : 0.0;
    result.probable_error = 0.67449 * std::sqrt(repeat);
    const double icc = result.icc_with_interaction;
    if (icc >= 0.80) {
        result.classification = "First";
    } else if (icc >= 0.50) {
        result.classification = "Second";
    } else if (icc >= 0.20) {
        result.classification = "Third";
    } else {
        result.classification = "Fourth";
    }
    result.attenuation_percent = (1.0 - icc) * 100.0;
    if (!(repeat > 0.0)) {
        add_warning(result.diagnostics, "zero_repeatability",
                    "重复性方差为 0，Probable Error 为 0；请检查设计。");
    }
    add_info(result.diagnostics, "emp_not_aiag_pass_fail",
             "EMP 分级基于 Wheeler ICC，不是 AIAG %Study Var 合格判定。");
    return result;
}

}  // namespace datalab::domain::statistics
