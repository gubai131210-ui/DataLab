#include "domain/statistics/fine_gray.h"

#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <utility>
#include <vector>

namespace datalab::domain::statistics {
namespace {

DiagnosticMessage info_msg(const std::string& code, const std::string& message)
{
    return {DiagnosticMessage::Severity::info, code, message};
}

DiagnosticMessage warning_msg(const std::string& code, const std::string& message)
{
    return {DiagnosticMessage::Severity::warning, code, message};
}

enum class FgStatus { right_censored = 0, target = 1, competing = 2 };

struct FgRow {
    double time = 0.0;
    FgStatus status = FgStatus::right_censored;
    std::vector<double> x;
};

constexpr std::size_t kMaxCovariates = 5;

std::map<double, double> censoring_survival_km(const std::vector<FgRow>& rows)
{
    std::vector<std::size_t> order(rows.size());
    for (std::size_t i = 0; i < order.size(); ++i) {
        order[i] = i;
    }
    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return rows[a].time < rows[b].time;
    });

    std::map<double, double> g_at;
    double survival = 1.0;
    std::size_t at_risk = rows.size();
    std::size_t index = 0;
    while (index < order.size()) {
        const double time = rows[order[index]].time;
        std::size_t end = index;
        std::size_t censored = 0;
        while (end < order.size() && rows[order[end]].time == time) {
            if (rows[order[end]].status == FgStatus::right_censored) {
                ++censored;
            }
            ++end;
        }
        if (censored > 0 && at_risk > 0) {
            survival *= 1.0 - static_cast<double>(censored) / static_cast<double>(at_risk);
            if (survival < 0.0) {
                survival = 0.0;
            }
        }
        g_at[time] = survival;
        at_risk -= (end - index);
        index = end;
    }
    return g_at;
}

double g_before(const std::map<double, double>& g_at, double time)
{
    double value = 1.0;
    for (const auto& [t, g] : g_at) {
        if (t >= time) {
            break;
        }
        value = g;
    }
    return std::max(value, 1e-12);
}

double linear_predictor(const std::vector<double>& beta, const std::vector<double>& x)
{
    double value = 0.0;
    for (std::size_t k = 0; k < beta.size(); ++k) {
        value += beta[k] * x[k];
    }
    return value;
}

// Solve A x = b in-place on A (p×p row-major) and b; returns false if singular.
bool solve_linear_system(std::vector<double>& a, std::vector<double>& b, std::size_t p)
{
    for (std::size_t col = 0; col < p; ++col) {
        std::size_t pivot = col;
        double best = std::fabs(a[col * p + col]);
        for (std::size_t row = col + 1; row < p; ++row) {
            const double candidate = std::fabs(a[row * p + col]);
            if (candidate > best) {
                best = candidate;
                pivot = row;
            }
        }
        if (!(best > 1e-14)) {
            return false;
        }
        if (pivot != col) {
            for (std::size_t j = 0; j < p; ++j) {
                std::swap(a[col * p + j], a[pivot * p + j]);
            }
            std::swap(b[col], b[pivot]);
        }
        const double diag = a[col * p + col];
        for (std::size_t row = col + 1; row < p; ++row) {
            const double factor = a[row * p + col] / diag;
            a[row * p + col] = 0.0;
            for (std::size_t j = col + 1; j < p; ++j) {
                a[row * p + j] -= factor * a[col * p + j];
            }
            b[row] -= factor * b[col];
        }
    }
    for (std::size_t row = p; row-- > 0;) {
        double sum = b[row];
        for (std::size_t j = row + 1; j < p; ++j) {
            sum -= a[row * p + j] * b[j];
        }
        const double diag = a[row * p + row];
        if (!(std::fabs(diag) > 1e-14)) {
            return false;
        }
        b[row] = sum / diag;
        if (!std::isfinite(b[row])) {
            return false;
        }
    }
    return true;
}

void publish_single_fields(FineGrayResult& result)
{
    if (result.terms.empty()) {
        return;
    }
    const auto& term = result.terms.front();
    result.covariate_name = term.name;
    result.covariate_mean = term.mean;
    result.beta = term.beta;
    result.se_beta = term.se_beta;
    result.hazard_ratio = term.hazard_ratio;
    result.p_value = term.p_value;
}

FineGrayResult fit_fine_gray_core(
    std::vector<FgRow> rows,
    const std::string& target,
    const std::string& kind,
    const std::string& algorithm_id,
    const std::vector<std::string>& covariate_names,
    const std::string& group0,
    const std::string& group1,
    const std::vector<double>& covariate_means,
    std::vector<DiagnosticMessage> prior_diagnostics)
{
    FineGrayResult result;
    result.evidence_type = "formula_reference";
    result.algorithm_id = algorithm_id;
    result.kind = kind;
    result.target_failure_mode = target;
    result.group_level_0 = group0;
    result.group_level_1 = group1;
    result.diagnostics = std::move(prior_diagnostics);

    const std::size_t p =
        rows.empty() ? covariate_names.size() : rows.front().x.size();
    for (const auto& row : rows) {
        if (row.status == FgStatus::target) {
            ++result.target_failures;
        } else if (row.status == FgStatus::competing) {
            ++result.competing_failures;
        } else {
            ++result.right_censored;
        }
    }
    result.n = rows.size();
    if (p == 0 || p > kMaxCovariates) {
        result.not_computed_reason = "invalid_covariate_dimension";
        result.diagnostics.push_back(warning_msg(
            "fine_gray_invalid_dimension",
            "Fine-Gray 协变量维数无效（允许 1.." + std::to_string(kMaxCovariates)
                + "）。"));
        return result;
    }
    if (result.n < 10 || result.target_failures < 2) {
        result.not_computed_reason = "insufficient_events";
        result.diagnostics.push_back(warning_msg(
            "fine_gray_insufficient_events",
            "Fine-Gray 需要 n≥10 且目标原因失效≥2（当前 n="
                + std::to_string(result.n) + "，target="
                + std::to_string(result.target_failures) + "）。"));
        return result;
    }
    if (p > 1 && result.target_failures < 5 * p) {
        result.not_computed_reason = "insufficient_events_for_p";
        result.diagnostics.push_back(warning_msg(
            "fine_gray_insufficient_events_for_p",
            "多协变量 Fine-Gray 需要目标失效数 ≥ 5×p（当前 target="
                + std::to_string(result.target_failures) + "，p="
                + std::to_string(p) + "）。"));
        return result;
    }

    for (std::size_t k = 0; k < p; ++k) {
        double x_min = rows.front().x[k];
        double x_max = rows.front().x[k];
        for (const auto& row : rows) {
            x_min = std::min(x_min, row.x[k]);
            x_max = std::max(x_max, row.x[k]);
        }
        if (!(x_max > x_min)) {
            result.not_computed_reason = "covariate_no_variation";
            result.diagnostics.push_back(warning_msg(
                "fine_gray_covariate_no_variation",
                "Fine-Gray 协变量无变异（列="
                    + (k < covariate_names.size() ? covariate_names[k] : ("x" + std::to_string(k)))
                    + "），子分布风险对比不可识别。"));
            return result;
        }
    }

    std::string names_joined;
    for (std::size_t k = 0; k < covariate_names.size(); ++k) {
        if (k > 0) {
            names_joined += ",";
        }
        names_joined += covariate_names[k];
    }
    result.diagnostics.push_back(info_msg(
        "fine_gray_scope",
        "Fine-Gray = formula_reference / " + algorithm_id
            + "：IPCW 偏似然；kind=" + kind + "；p=" + std::to_string(p)
            + "；目标原因=" + target + "；协变量=" + names_joined
            + "。不是 cause-specific Cox，不是 vendor_oracle"
            + (p > 1 ? "；未对齐 pinned R survival::finegray。"
                     : "；多协变量请用 fine_gray_multi_ipcw。")));
    result.diagnostics.push_back(warning_msg(
        "fine_gray_not_vendor_oracle",
        "不得把本 Fine-Gray 结果写成商业软件对齐或 pinned R survival::finegray 黄金标准。"));

    const auto g_at = censoring_survival_km(rows);
    result.ran = true;

    auto score_and_info = [&](const std::vector<double>& beta)
        -> std::pair<std::vector<double>, std::vector<double>> {
        std::vector<double> score(p, 0.0);
        std::vector<double> info(p * p, 0.0);
        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].status != FgStatus::target) {
                continue;
            }
            const double t = rows[i].time;
            const double g_t = g_before(g_at, t);
            std::vector<double> swx(p, 0.0);
            std::vector<double> swxx(p * p, 0.0);
            double sw = 0.0;
            for (std::size_t j = 0; j < rows.size(); ++j) {
                double w = 0.0;
                if (rows[j].time >= t) {
                    w = 1.0;
                } else if (rows[j].status == FgStatus::competing) {
                    const double g_tj = g_before(g_at, rows[j].time);
                    w = g_t / g_tj;
                } else {
                    continue;
                }
                if (!(w > 0.0) || !std::isfinite(w)) {
                    continue;
                }
                const double e = std::exp(linear_predictor(beta, rows[j].x));
                if (!std::isfinite(e)) {
                    continue;
                }
                const double we = w * e;
                sw += we;
                for (std::size_t a = 0; a < p; ++a) {
                    swx[a] += we * rows[j].x[a];
                    for (std::size_t b = 0; b < p; ++b) {
                        swxx[a * p + b] += we * rows[j].x[a] * rows[j].x[b];
                    }
                }
            }
            if (!(sw > 0.0)) {
                continue;
            }
            std::vector<double> mean_x(p, 0.0);
            for (std::size_t a = 0; a < p; ++a) {
                mean_x[a] = swx[a] / sw;
                score[a] += rows[i].x[a] - mean_x[a];
            }
            for (std::size_t a = 0; a < p; ++a) {
                for (std::size_t b = 0; b < p; ++b) {
                    info[a * p + b] +=
                        (swxx[a * p + b] / sw) - mean_x[a] * mean_x[b];
                }
            }
        }
        return {score, info};
    };

    std::vector<double> beta(p, 0.0);
    bool converged = false;
    int iters = 0;
    for (iters = 1; iters <= 50; ++iters) {
        auto [score, info] = score_and_info(beta);
        std::vector<double> step = score;
        if (!solve_linear_system(info, step, p)) {
            result.not_computed_reason = "singular_information";
            result.diagnostics.push_back(warning_msg(
                "fine_gray_singular",
                "Fine-Gray 信息量退化，未能估计 β。"));
            return result;
        }
        double max_abs = 0.0;
        for (std::size_t k = 0; k < p; ++k) {
            beta[k] += step[k];
            max_abs = std::max(max_abs, std::fabs(step[k]));
            if (!std::isfinite(beta[k]) || std::fabs(beta[k]) > 20.0) {
                result.not_computed_reason = "beta_diverged";
                result.diagnostics.push_back(warning_msg(
                    "fine_gray_diverged",
                    "Fine-Gray β 迭代发散。"));
                return result;
            }
        }
        if (max_abs < 1e-8) {
            converged = true;
            break;
        }
    }
    result.iterations = iters;
    result.converged = converged;
    if (!converged) {
        result.not_computed_reason = "not_converged";
        result.diagnostics.push_back(warning_msg(
            "fine_gray_not_converged",
            "Fine-Gray 未在迭代上限内收敛。"));
        return result;
    }

    auto [score_final, info_final] = score_and_info(beta);
    (void)score_final;
    // Invert information for SE: solve I * e_k = unit vectors.
    std::vector<double> inv_diag(p, std::numeric_limits<double>::quiet_NaN());
    for (std::size_t k = 0; k < p; ++k) {
        std::vector<double> a = info_final;
        std::vector<double> e(p, 0.0);
        e[k] = 1.0;
        if (!solve_linear_system(a, e, p) || !(e[k] > 0.0) || !std::isfinite(e[k])) {
            result.not_computed_reason = "singular_information";
            result.diagnostics.push_back(warning_msg(
                "fine_gray_singular",
                "Fine-Gray 信息量不可逆，未能估计 SE。"));
            return result;
        }
        inv_diag[k] = e[k];
    }

    result.terms.reserve(p);
    for (std::size_t k = 0; k < p; ++k) {
        FineGrayTerm term;
        term.name = k < covariate_names.size()
            ? covariate_names[k]
            : ("x" + std::to_string(k));
        if (k < covariate_means.size()) {
            term.mean = covariate_means[k];
        }
        term.beta = beta[k];
        term.se_beta = std::sqrt(inv_diag[k]);
        term.hazard_ratio = std::exp(beta[k]);
        const double z = beta[k] / *term.se_beta;
        term.p_value = 2.0 * (1.0 - standard_normal_cdf(std::fabs(z)));
        result.terms.push_back(std::move(term));
    }
    publish_single_fields(result);
    return result;
}

struct PreparedRows {
    std::vector<FgRow> rows;
    std::string target;
    std::vector<DiagnosticMessage> diagnostics;
    std::string early_reason;
};

PreparedRows prepare_rows(
    const std::vector<CensoringObservation>& observations,
    const std::vector<std::vector<double>>& x_matrix,
    const std::string& target_failure_mode)
{
    PreparedRows prepared;
    if (x_matrix.size() != observations.size()) {
        prepared.early_reason = "covariate_length_mismatch";
        prepared.diagnostics.push_back(warning_msg(
            "fine_gray_covariate_length_mismatch",
            "Fine-Gray 协变量矩阵行数与观测不一致。"));
        return prepared;
    }

    std::set<std::string> modes;
    std::size_t skipped_left_interval = 0;
    std::size_t unlabeled_exact = 0;
    std::size_t missing_x = 0;
    struct Raw {
        double time = 0.0;
        bool is_exact = false;
        std::string mode;
        std::vector<double> x;
    };
    std::vector<Raw> raw;
    for (std::size_t i = 0; i < observations.size(); ++i) {
        const auto& observation = observations[i];
        if (observation.type == CensoringType::left
            || observation.type == CensoringType::interval) {
            ++skipped_left_interval;
            continue;
        }
        if (!(observation.time >= 0.0) || !std::isfinite(observation.time)) {
            continue;
        }
        const auto& x_row = x_matrix[i];
        bool bad_x = x_row.empty();
        for (double value : x_row) {
            if (!std::isfinite(value)) {
                bad_x = true;
                break;
            }
        }
        if (bad_x) {
            ++missing_x;
            continue;
        }
        if (observation.type == CensoringType::right) {
            raw.push_back({observation.time, false, {}, x_row});
            continue;
        }
        if (observation.failure_mode.empty()) {
            ++unlabeled_exact;
            continue;
        }
        modes.insert(observation.failure_mode);
        raw.push_back({observation.time, true, observation.failure_mode, x_row});
    }

    if (skipped_left_interval > 0) {
        prepared.diagnostics.push_back(info_msg(
            "fine_gray_left_interval_omitted",
            "Fine-Gray 省略 left/interval 行 " + std::to_string(skipped_left_interval)
                + " 条。"));
    }
    if (unlabeled_exact > 0) {
        prepared.diagnostics.push_back(warning_msg(
            "fine_gray_unlabeled_exact_excluded",
            "有 " + std::to_string(unlabeled_exact)
                + " 条 exact 失效缺少 failure_mode，已从 Fine-Gray 排除。"));
    }
    if (missing_x > 0) {
        prepared.diagnostics.push_back(warning_msg(
            "fine_gray_missing_covariate_excluded",
            "有 " + std::to_string(missing_x)
                + " 条因协变量缺失/非有限从 Fine-Gray 排除。"));
    }
    if (modes.empty()) {
        prepared.early_reason = "no_labeled_failures";
        prepared.diagnostics.push_back(warning_msg(
            "fine_gray_no_labeled_failures",
            "Fine-Gray 需要带 failure_mode 的 exact 失效。"));
        return prepared;
    }

    prepared.target = target_failure_mode;
    if (prepared.target.empty() || modes.count(prepared.target) == 0) {
        prepared.target = *modes.begin();
    }

    for (const auto& item : raw) {
        FgRow row;
        row.time = item.time;
        row.x = item.x;
        if (!item.is_exact) {
            row.status = FgStatus::right_censored;
        } else if (item.mode == prepared.target) {
            row.status = FgStatus::target;
        } else {
            row.status = FgStatus::competing;
        }
        prepared.rows.push_back(std::move(row));
    }
    return prepared;
}

void mean_center(std::vector<FgRow>& rows, std::vector<double>& means)
{
    if (rows.empty()) {
        return;
    }
    const std::size_t p = rows.front().x.size();
    means.assign(p, 0.0);
    for (const auto& row : rows) {
        for (std::size_t k = 0; k < p; ++k) {
            means[k] += row.x[k];
        }
    }
    for (std::size_t k = 0; k < p; ++k) {
        means[k] /= static_cast<double>(rows.size());
    }
    for (auto& row : rows) {
        for (std::size_t k = 0; k < p; ++k) {
            row.x[k] -= means[k];
        }
    }
}

}  // namespace

FineGrayResult fine_gray_multi(
    const std::vector<CensoringObservation>& observations,
    const std::vector<std::vector<double>>& x_matrix,
    const std::vector<std::string>& covariate_names,
    const std::string& target_failure_mode)
{
    FineGrayResult early;
    early.evidence_type = "formula_reference";
    early.algorithm_id = "fine_gray_multi_ipcw";
    early.kind = "multi";

    std::vector<std::string> names = covariate_names;
    if (!x_matrix.empty()) {
        const std::size_t p = x_matrix.front().size();
        while (names.size() < p) {
            names.push_back("x" + std::to_string(names.size()));
        }
        names.resize(p);
        for (const auto& row : x_matrix) {
            if (row.size() != p) {
                early.not_computed_reason = "covariate_ragged_matrix";
                early.diagnostics.push_back(warning_msg(
                    "fine_gray_ragged_matrix",
                    "Fine-Gray 协变量矩阵行长度不一致。"));
                return early;
            }
        }
        if (p < 2) {
            early.not_computed_reason = "need_at_least_two_covariates";
            early.diagnostics.push_back(warning_msg(
                "fine_gray_need_multi",
                "fine_gray_multi 需要至少两个协变量；单列请用 fine_gray_continuous。"));
            return early;
        }
    }

    auto prepared = prepare_rows(observations, x_matrix, target_failure_mode);
    if (!prepared.early_reason.empty()) {
        early.not_computed_reason = prepared.early_reason;
        early.diagnostics = std::move(prepared.diagnostics);
        return early;
    }

    std::vector<double> means;
    mean_center(prepared.rows, means);
    return fit_fine_gray_core(
        std::move(prepared.rows),
        prepared.target,
        "multi",
        "fine_gray_multi_ipcw",
        names,
        {},
        {},
        means,
        std::move(prepared.diagnostics));
}

FineGrayResult fine_gray_continuous(
    const std::vector<CensoringObservation>& observations,
    const std::vector<double>& x_values,
    const std::string& target_failure_mode,
    const std::string& covariate_name)
{
    FineGrayResult early;
    early.evidence_type = "formula_reference";
    early.algorithm_id = "fine_gray_continuous_ipcw";
    early.kind = "continuous";
    early.covariate_name = covariate_name.empty() ? "x" : covariate_name;

    std::vector<std::vector<double>> matrix;
    matrix.reserve(x_values.size());
    for (double value : x_values) {
        matrix.push_back({value});
    }
    auto prepared = prepare_rows(observations, matrix, target_failure_mode);
    if (!prepared.early_reason.empty()) {
        early.not_computed_reason = prepared.early_reason;
        early.diagnostics = std::move(prepared.diagnostics);
        if (early.not_computed_reason == "covariate_length_mismatch") {
            // keep prior message style for continuous
            early.diagnostics.clear();
            early.diagnostics.push_back(warning_msg(
                "fine_gray_covariate_length_mismatch",
                "Fine-Gray 连续协变量长度与观测不一致。"));
        }
        return early;
    }

    std::vector<double> means;
    mean_center(prepared.rows, means);
    return fit_fine_gray_core(
        std::move(prepared.rows),
        prepared.target,
        "continuous",
        "fine_gray_continuous_ipcw",
        {early.covariate_name},
        {},
        {},
        means,
        std::move(prepared.diagnostics));
}

FineGrayResult fine_gray_binary(
    const std::vector<CensoringObservation>& observations,
    const std::string& target_failure_mode)
{
    FineGrayResult early;
    early.evidence_type = "formula_reference";
    early.algorithm_id = "fine_gray_binary_ipcw";
    early.kind = "binary";

    std::set<std::string> modes;
    std::set<std::string> groups;
    std::size_t skipped_left_interval = 0;
    std::size_t unlabeled_exact = 0;
    std::size_t missing_group = 0;
    struct Raw {
        double time = 0.0;
        bool is_exact = false;
        std::string mode;
        std::string group;
    };
    std::vector<Raw> raw;
    for (const auto& observation : observations) {
        if (observation.type == CensoringType::left
            || observation.type == CensoringType::interval) {
            ++skipped_left_interval;
            continue;
        }
        if (!(observation.time >= 0.0) || !std::isfinite(observation.time)) {
            continue;
        }
        if (observation.group.empty()) {
            ++missing_group;
            continue;
        }
        groups.insert(observation.group);
        if (observation.type == CensoringType::right) {
            raw.push_back({observation.time, false, {}, observation.group});
            continue;
        }
        if (observation.failure_mode.empty()) {
            ++unlabeled_exact;
            continue;
        }
        modes.insert(observation.failure_mode);
        raw.push_back(
            {observation.time, true, observation.failure_mode, observation.group});
    }

    std::vector<DiagnosticMessage> diagnostics;
    if (skipped_left_interval > 0) {
        diagnostics.push_back(info_msg(
            "fine_gray_left_interval_omitted",
            "Fine-Gray 省略 left/interval 行 " + std::to_string(skipped_left_interval)
                + " 条。"));
    }
    if (unlabeled_exact > 0) {
        diagnostics.push_back(warning_msg(
            "fine_gray_unlabeled_exact_excluded",
            "有 " + std::to_string(unlabeled_exact)
                + " 条 exact 失效缺少 failure_mode，已从 Fine-Gray 排除。"));
    }
    if (missing_group > 0) {
        diagnostics.push_back(warning_msg(
            "fine_gray_missing_group_excluded",
            "有 " + std::to_string(missing_group)
                + " 条因分组缺失从 Fine-Gray 排除。"));
    }
    if (modes.empty()) {
        early.not_computed_reason = "no_labeled_failures";
        early.diagnostics = std::move(diagnostics);
        early.diagnostics.push_back(warning_msg(
            "fine_gray_no_labeled_failures",
            "Fine-Gray 需要带 failure_mode 的 exact 失效。"));
        return early;
    }
    if (groups.size() != 2) {
        early.not_computed_reason = "group_not_binary";
        early.diagnostics = std::move(diagnostics);
        early.diagnostics.push_back(warning_msg(
            "fine_gray_group_not_binary",
            "Fine-Gray 二分类需要恰好两个分组水平（当前 "
                + std::to_string(groups.size())
                + "）；连续协变量请用 covariate 列。"));
        return early;
    }

    std::string target = target_failure_mode;
    if (target.empty() || modes.count(target) == 0) {
        target = *modes.begin();
    }
    const std::string group0 = *groups.begin();
    const std::string group1 = *std::next(groups.begin());

    std::vector<FgRow> rows;
    bool target_in_both = false;
    bool seen0 = false;
    bool seen1 = false;
    for (const auto& item : raw) {
        FgRow row;
        row.time = item.time;
        row.x = {item.group == group1 ? 1.0 : 0.0};
        if (!item.is_exact) {
            row.status = FgStatus::right_censored;
        } else if (item.mode == target) {
            row.status = FgStatus::target;
            if (item.group == group0) {
                seen0 = true;
            } else {
                seen1 = true;
            }
        } else {
            row.status = FgStatus::competing;
        }
        rows.push_back(std::move(row));
    }
    target_in_both = seen0 && seen1;
    if (!target_in_both) {
        diagnostics.push_back(warning_msg(
            "fine_gray_target_one_group_only",
            "目标原因失效只出现在一个分组中；估计仍可运行但对比识别较弱。"));
    }

    return fit_fine_gray_core(
        std::move(rows),
        target,
        "binary",
        "fine_gray_binary_ipcw",
        {"group"},
        group0,
        group1,
        {},
        std::move(diagnostics));
}

}  // namespace datalab::domain::statistics
