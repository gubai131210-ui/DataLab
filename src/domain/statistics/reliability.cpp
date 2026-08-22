#include "domain/statistics/reliability.h"

#include "domain/statistics/normal_distribution.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace datalab::domain::statistics {
namespace {
void error(std::vector<DiagnosticMessage>& d, const char* c, const char* m)
{
    d.push_back({DiagnosticMessage::Severity::error, c, m});
}
bool valid(const std::vector<double>& t, const std::vector<bool>& e,
           std::vector<DiagnosticMessage>& d)
{
    if (t.size() < 2 || t.size() != e.size()) {
        error(d, "invalid_reliability_shape", "寿命和删失指示列长度必须一致且至少包含两条记录。");
        return false;
    }
    for (double v : t) if (!std::isfinite(v) || v <= 0.0) {
        error(d, "invalid_reliability_time", "寿命必须为有限正数。");
        return false;
    }
    return true;
}

double normal_quantile(double confidence_level)
{
    // Acklam's rational approximation is sufficiently accurate for CI output.
    const double p = 0.5 + confidence_level / 2.0;
    const double a1 = -39.6968302866538;
    const double a2 = 220.946098424521;
    const double a3 = -275.928510446969;
    const double a4 = 138.357751867269;
    const double a5 = -30.6647980661472;
    const double a6 = 2.50662827745924;
    const double b1 = -54.4760987982241;
    const double b2 = 161.585836858041;
    const double b3 = -155.698979859887;
    const double b4 = 66.8013118877197;
    const double b5 = -13.2806815528857;
    const double c1 = -0.00778489400243029;
    const double c2 = -0.322396458041136;
    const double c3 = -2.40075827716184;
    const double c4 = -2.54973253934373;
    const double c5 = 4.37466414146497;
    const double c6 = 2.93816398269878;
    const double d1 = 0.00778469570904146;
    const double d2 = 0.32246712907004;
    const double d3 = 2.445134137143;
    const double d4 = 3.75440866190742;
    if (p < 0.02425) {
        const double q = std::sqrt(-2.0 * std::log(p));
        return -(((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) /
            ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
    }
    if (p > 1.0 - 0.02425) {
        const double q = std::sqrt(-2.0 * std::log(1.0 - p));
        return (((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) /
            ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
    }
    const double q = p - 0.5;
    const double r = q * q;
    return (((((a1 * r + a2) * r + a3) * r + a4) * r + a5) * r + a6) * q /
        (((((b1 * r + b2) * r + b3) * r + b4) * r + b5) * r + 1.0);
}

bool valid_confidence_level(double level)
{
    return std::isfinite(level) && level > 0.0 && level < 1.0;
}

double percentile_life_weibull_impl(double shape, double scale, double percentile)
{
    const double probability = std::clamp(percentile / 100.0, 1.0e-9, 1.0 - 1.0e-9);
    return scale * std::pow(-std::log(1.0 - probability), 1.0 / shape);
}

double percentile_life_exponential_impl(double rate, double percentile)
{
    const double probability = std::clamp(percentile / 100.0, 1.0e-9, 1.0 - 1.0e-9);
    return -std::log(1.0 - probability) / rate;
}

double percentile_life_lognormal_impl(double location, double scale, double percentile)
{
    const double probability = std::clamp(percentile / 100.0, 1.0e-9, 1.0 - 1.0e-9);
    return std::exp(location + scale * standard_normal_quantile(probability));
}

std::vector<double> threshold_candidates(double min_time, double max_time)
{
    const double span = std::max(max_time - min_time, min_time);
    std::vector<double> lambdas;
    const double fractions[] = {
        0.0005, 0.001, 0.002, 0.005, 0.01, 0.02, 0.04, 0.08, 0.12, 0.2,
        0.3, 0.45, 0.6, 0.8, 1.0, 1.5, 2.0};
    for (const double fraction : fractions) {
        lambdas.push_back(min_time - std::max(span, 1.0) * fraction);
    }
    for (int step = 1; step <= 16; ++step) {
        const double gap = min_time * 1.0e-4 * static_cast<double>(step);
        lambdas.push_back(min_time - std::max(gap, 1.0e-8));
    }
    return lambdas;
}

bool shift_by_threshold(const std::vector<double>& times, double lambda,
                        std::vector<double>* shifted)
{
    shifted->resize(times.size());
    for (std::size_t index = 0; index < times.size(); ++index) {
        (*shifted)[index] = times[index] - lambda;
        if (!((*shifted)[index] > 0.0) || !std::isfinite((*shifted)[index])) {
            return false;
        }
    }
    return true;
}

void lifetime_span(const std::vector<double>& times, const std::vector<bool>& events,
                   std::size_t* failures, double* min_time, double* max_time,
                   std::vector<double>* unique_failures)
{
    *failures = 0;
    *min_time = times.front();
    *max_time = times.front();
    unique_failures->clear();
    for (std::size_t index = 0; index < times.size(); ++index) {
        *min_time = std::min(*min_time, times[index]);
        *max_time = std::max(*max_time, times[index]);
        if (events[index]) {
            ++*failures;
            unique_failures->push_back(times[index]);
        }
    }
    std::sort(unique_failures->begin(), unique_failures->end());
    unique_failures->erase(std::unique(unique_failures->begin(), unique_failures->end()),
                           unique_failures->end());
}

double log_normal_survival(double z)
{
    const double cdf = standard_normal_cdf(z);
    if (cdf < 1.0 - 1.0e-12) {
        return std::log(std::max(1.0 - cdf, 1.0e-300));
    }
    constexpr double kInvSqrtTwoPi = 0.3989422804014327;
    const double pdf = kInvSqrtTwoPi * std::exp(-0.5 * z * z);
    return std::log(std::max(pdf / std::max(z, 1.0), 1.0e-300));
}

double lognormal_log_likelihood(
    double location,
    double scale,
    const std::vector<double>& times,
    const std::vector<bool>& events)
{
    if (!(scale > 0.0) || !std::isfinite(location) || !std::isfinite(scale)) {
        return -std::numeric_limits<double>::infinity();
    }
    constexpr double kLogTwoPi = 1.8378770664093453;
    double log_likelihood = 0.0;
    for (std::size_t index = 0; index < times.size(); ++index) {
        const double z = (std::log(times[index]) - location) / scale;
        if (events[index]) {
            log_likelihood += -std::log(scale) - 0.5 * kLogTwoPi - 0.5 * z * z;
        } else {
            log_likelihood += log_normal_survival(z);
        }
        if (!std::isfinite(log_likelihood)) {
            return -std::numeric_limits<double>::infinity();
        }
    }
    return log_likelihood;
}

void add_model_metrics(double shape, double scale, std::size_t n,
                       std::size_t failures, double log_likelihood,
                       WeibullResult& r, int parameter_count = 2)
{
    r.shape = shape;
    r.scale = scale;
    r.log_likelihood = log_likelihood;
    const double k = static_cast<double>(parameter_count);
    r.aic = 2.0 * k - 2.0 * log_likelihood;
    r.bic = std::log(static_cast<double>(n)) * k - 2.0 * log_likelihood;
    const double offset = r.threshold.value_or(0.0);
    r.b10 = offset + percentile_life_weibull_impl(shape, scale, 10.0);
    r.b50 = offset + percentile_life_weibull_impl(shape, scale, 50.0);
    r.b90 = offset + percentile_life_weibull_impl(shape, scale, 90.0);
    r.median_life = r.b50;
    r.failures = failures;
    r.observations = n;
    r.identifiable = true;
    r.converged = std::isfinite(shape) && std::isfinite(scale)
        && std::isfinite(log_likelihood);
    r.iterations = 100;
    r.censoring_fraction = n > 0
        ? 1.0 - static_cast<double>(failures) / static_cast<double>(n) : 0.0;
    r.parameter_boundary_hit = shape <= 1.0e-5 || shape >= 1.0e6;
}

WeibullResult fit_weibull_core(const std::vector<double>& times,
                               const std::vector<bool>& events)
{
    WeibullResult r;
    std::vector<double> failure_times;
    double failure_log_sum = 0.0;
    for (std::size_t i = 0; i < times.size(); ++i)
        if (events[i]) {
            failure_times.push_back(times[i]);
            failure_log_sum += std::log(times[i]);
        }
    std::sort(failure_times.begin(), failure_times.end());
    const std::size_t failures = failure_times.size();
    if (failures == 0) {
        error(r.diagnostics, "non_identifiable_weibull",
              "全为删失，无法识别 Weibull 参数。");
        r.not_computed_reason = "all_censored";
        r.rules.push_back({
            "identifiability", "triggered",
            "全为删失，Weibull 形状和尺度不可估计。", {},
            "不要输出默认形状参数或伪造分位寿命。"});
        return r;
    }
    if (failures < 2) {
        error(r.diagnostics, "few_failures",
              "只有一条失效记录时 Weibull 形状参数通常不可稳定识别。");
        r.not_computed_reason = "few_failures";
        r.rules.push_back({
            "identifiability", "triggered",
            "失效数不足，形状参数不可稳定识别。", {},
            "至少需要两次失效才能解释 Weibull 形状。"});
        return r;
    }
    auto score = [&](double shape) {
        double weighted_log_sum = 0.0;
        double weighted_sum = 0.0;
        for (double time : times) {
            const double power = std::pow(time, shape);
            weighted_sum += power;
            weighted_log_sum += power * std::log(time);
        }
        return 1.0 / shape + failure_log_sum / static_cast<double>(failures) -
            weighted_log_sum / weighted_sum;
    };
    double low = 1.0e-6;
    double high = 1.0;
    while (score(high) > 0.0 && high < 1.0e6) high *= 2.0;
    if (score(high) > 0.0) {
        error(r.diagnostics, "non_identifiable_weibull",
              "失效时间没有足够变化，无法有限地识别 Weibull 形状。");
        return r;
    }
    for (int iteration = 0; iteration < 100; ++iteration) {
        const double mid = std::sqrt(low * high);
        if (score(mid) > 0.0) low = mid;
        else high = mid;
    }
    const double shape = std::sqrt(low * high);
    double sum_power = 0.0;
    for (double time : times) sum_power += std::pow(time, shape);
    const double scale = std::pow(sum_power / static_cast<double>(failures),
                                  1.0 / shape);
    double log_likelihood = static_cast<double>(failures) *
        (std::log(shape) - shape * std::log(scale)) +
        (shape - 1.0) * failure_log_sum;
    for (double time : times)
        log_likelihood -= std::pow(time / scale, shape);
    add_model_metrics(shape, scale, times.size(), failures, log_likelihood, r);
    r.evidence.method_version = "2";
    r.evidence.valid_count = times.size();
    r.evidence.assumption_status = "not_verified";
    r.rules.push_back({
        "convergence",
        r.converged && !r.parameter_boundary_hit ? "not_triggered" : "triggered",
        r.parameter_boundary_hit
            ? "Weibull 形状参数落到数值边界，估计可能不稳定。"
            : (r.converged ? "Weibull 参数估计已收敛。" : "Weibull 参数未收敛。"),
        {},
        "报告形状、尺度、删失似然和置信区间；不要把分位寿命当成单件保证寿命。"});
    return r;
}

double chi_square_right_tail(double value, double degrees_of_freedom)
{
    if (!(value >= 0.0) || !(degrees_of_freedom > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double shape = degrees_of_freedom / 2.0;
    const double x = value / 2.0;
    if (x == 0.0) {
        return 1.0;
    }
    double term = 1.0 / shape;
    double sum = term;
    for (int index = 1; index < 200; ++index) {
        term *= x / (shape + index);
        sum += term;
        if (std::abs(term) < std::abs(sum) * 1.0e-14) {
            break;
        }
    }
    if (x < shape + 1.0) {
        return std::clamp(1.0 - sum * std::exp(-x + shape * std::log(x)
            - std::lgamma(shape)), 0.0, 1.0);
    }
    double continued = 1.0;
    double factor = 1.0;
    for (int index = 1; index < 200; ++index) {
        factor *= (shape - index) / x;
        continued += factor;
        if (std::abs(factor) < std::abs(continued) * 1.0e-14) {
            break;
        }
    }
    return std::clamp(std::exp(-x + shape * std::log(x)
        - std::lgamma(shape)) * continued, 0.0, 1.0);
}

bool invert_matrix(std::vector<std::vector<double>> matrix,
                   std::vector<std::vector<double>>* inverse)
{
    if (matrix.empty() || matrix.size() != matrix.front().size() || inverse == nullptr) {
        return false;
    }
    const std::size_t n = matrix.size();
    inverse->assign(n, std::vector<double>(n, 0.0));
    for (std::size_t index = 0; index < n; ++index) {
        (*inverse)[index][index] = 1.0;
    }
    for (std::size_t column = 0; column < n; ++column) {
        std::size_t pivot = column;
        double pivot_value = std::abs(matrix[column][column]);
        for (std::size_t row = column + 1; row < n; ++row) {
            const double candidate = std::abs(matrix[row][column]);
            if (candidate > pivot_value) {
                pivot_value = candidate;
                pivot = row;
            }
        }
        if (!(pivot_value > 0.0)) {
            return false;
        }
        if (pivot != column) {
            std::swap(matrix[column], matrix[pivot]);
            std::swap((*inverse)[column], (*inverse)[pivot]);
        }
        const double scale = matrix[column][column];
        for (std::size_t row = 0; row < n; ++row) {
            if (row == column) {
                continue;
            }
            const double factor = matrix[row][column] / scale;
            if (factor == 0.0) {
                continue;
            }
            for (std::size_t col = 0; col < n; ++col) {
                matrix[row][col] -= factor * matrix[column][col];
                (*inverse)[row][col] -= factor * (*inverse)[column][col];
            }
        }
        for (std::size_t col = 0; col < n; ++col) {
            matrix[column][col] /= scale;
            (*inverse)[column][col] /= scale;
        }
    }
    return true;
}

double quadratic_form(const std::vector<double>& vector,
                      const std::vector<std::vector<double>>& inverse)
{
    double value = 0.0;
    for (std::size_t row = 0; row < vector.size(); ++row) {
        for (std::size_t column = 0; column < vector.size(); ++column) {
            value += vector[row] * inverse[row][column] * vector[column];
        }
    }
    return value;
}

}  // namespace

std::optional<bool> parse_reliability_event(const std::string& text)
{
    if (text == "1" || text == "1.0" || text == "true" || text == "TRUE"
        || text == "True" || text == "fail" || text == "Fail"
        || text == "Failure" || text == "failure" || text == "F"
        || text == "event" || text == "Event") {
        return true;
    }
    if (text == "0" || text == "0.0" || text == "false" || text == "FALSE"
        || text == "False" || text == "censor" || text == "Censor"
        || text == "censored" || text == "Censored" || text == "suspension"
        || text == "S" || text == "C" || text == "删失") {
        return false;
    }
    return std::nullopt;
}

KaplanMeierResult kaplan_meier(const std::vector<double>& times,
                               const std::vector<bool>& events,
                               double confidence_level,
                               const std::vector<std::size_t>& source_rows)
{
    KaplanMeierResult r;
    if (!valid(times, events, r.diagnostics)) return r;
    if (!valid_confidence_level(confidence_level)) {
        error(r.diagnostics, "invalid_confidence_level", "置信水平必须在 0 和 1 之间。");
        return r;
    }
    r.confidence_level = confidence_level;
    r.evidence.method_version = "2";
    r.evidence.valid_count = times.size();
    r.evidence.source_rows = source_rows;
    r.evidence.assumption_status = "not_verified";
    const std::size_t failure_count = static_cast<std::size_t>(
        std::count(events.begin(), events.end(), true));
    r.failure_count = failure_count;
    r.censored_count = times.size() - failure_count;
    r.valid_count = times.size();
    r.censoring_fraction = static_cast<double>(times.size() - failure_count) /
        static_cast<double>(times.size());
    r.survival_identifiable = failure_count > 0;
    if (!r.survival_identifiable) {
        error(r.diagnostics, "non_identifiable_survival",
              "全为删失，无法识别失效分布或中位寿命。");
        r.not_computed_reason = "all_censored";
        r.rules.push_back({
            "identifiability", "triggered",
            "全部为删失，生存函数和中位寿命不可估计。", {},
            "不能把删失时间当作失效时间，也不能输出伪造的中位寿命。"});
        return r;
    }
    std::vector<std::size_t> order(times.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return times[a] < times[b];
    });
    double survival = 1.0;
    double greenwood_sum = 0.0;
    std::size_t at_risk = times.size();
    for (std::size_t pos = 0; pos < order.size();) {
        const double time = times[order[pos]];
        std::size_t failures = 0;
        std::size_t censored = 0;
        std::size_t end = pos;
        std::vector<std::size_t> point_rows;
        while (end < order.size() && times[order[end]] == time) {
            (events[order[end]] ? failures : censored)++;
            if (order[end] < source_rows.size()) {
                point_rows.push_back(source_rows[order[end]]);
            } else {
                point_rows.push_back(order[end]);
            }
            ++end;
        }
        if (failures > 0 && at_risk > failures) {
            survival *= 1.0 - static_cast<double>(failures) / at_risk;
            greenwood_sum += static_cast<double>(failures) /
                (static_cast<double>(at_risk) *
                 static_cast<double>(at_risk - failures));
        } else if (failures > 0) {
            survival = 0.0;
            greenwood_sum = std::numeric_limits<double>::infinity();
        }
        const double standard_error = survival > 0.0
            ? survival * std::sqrt(greenwood_sum) : 0.0;
        double lower = survival;
        double upper = survival;
        if (survival > 0.0 && survival < 1.0 && std::isfinite(greenwood_sum)) {
            const double z = normal_quantile(confidence_level);
            const double log_log = std::log(-std::log(survival));
            const double log_log_se = std::sqrt(greenwood_sum) /
                std::abs(std::log(survival));
            lower = std::exp(-std::exp(log_log + z * log_log_se));
            upper = std::exp(-std::exp(log_log - z * log_log_se));
        } else if (survival == 0.0) {
            lower = 0.0;
            upper = 0.0;
        }
        r.points.push_back({time, at_risk, failures, censored, survival,
                            standard_error, lower, upper, point_rows});
        if (!r.median_life.has_value() && survival <= 0.5) r.median_life = time;
        at_risk -= failures + censored;
        pos = end;
    }
    if (!r.points.empty() && r.points.back().censored > 0 && r.points.back().survival > 0.0) {
        r.diagnostics.push_back({DiagnosticMessage::Severity::warning, "not_estimable",
                                 "最大观测为删失，尾部生存函数不可估计到 0。"});
        if (r.not_computed_reason.empty()) {
            r.not_computed_reason = "max_observation_censored";
        }
        r.rules.push_back({
            "identifiability", "triggered",
            "最大观测为删失，尾部生存函数可能无法下降到 0。", {},
            "均值/尾部分位数在此时可能不可估计，应报告原因。"});
    }
    r.rules.push_back({
        "risk_set", "not_triggered",
        "Kaplan-Meier 已按同时刻合并失效并报告风险集。", {},
        "读取生存概率时必须同时看 at-risk、删失和置信区间。"});
    r.rules.push_back({
        "event_encoding", "not_triggered",
        "事件按失效/删失布尔语义处理。", {},
        "未知事件编码必须在导入阶段拒绝，不能静默当作删失。"});
    return r;
}

LogRankResult log_rank_test(const std::vector<double>& times,
                            const std::vector<bool>& events,
                            const std::vector<int>& groups)
{
    LogRankResult r;
    if (times.size() < 2 || times.size() != events.size()
        || times.size() != groups.size()) {
        error(r.diagnostics, "invalid_log_rank_shape",
              "Log-rank 检验要求寿命、事件和分组列长度一致。");
        return r;
    }
    std::size_t group_one = 0;
    std::size_t group_two = 0;
    for (std::size_t index = 0; index < times.size(); ++index) {
        if (!std::isfinite(times[index]) || times[index] <= 0.0
            || (groups[index] != 0 && groups[index] != 1)) {
            error(r.diagnostics, "invalid_log_rank_value",
                  "Log-rank 时间必须为正数，分组必须编码为 0 或 1。");
            return r;
        }
        groups[index] == 0 ? ++group_one : ++group_two;
    }
    r.group_one_n = group_one;
    r.group_two_n = group_two;
    if (group_one == 0 || group_two == 0) {
        error(r.diagnostics, "insufficient_log_rank_groups",
              "Log-rank 检验至少需要两个非空分组。");
        return r;
    }
    std::vector<double> event_times;
    for (std::size_t index = 0; index < times.size(); ++index) {
        if (events[index]) {
            event_times.push_back(times[index]);
        }
    }
    std::sort(event_times.begin(), event_times.end());
    event_times.erase(std::unique(event_times.begin(), event_times.end()),
                      event_times.end());
    double observed_minus_expected = 0.0;
    double variance = 0.0;
    for (const double time : event_times) {
        std::size_t at_risk_one = 0;
        std::size_t at_risk_two = 0;
        std::size_t events_one = 0;
        std::size_t events_two = 0;
        for (std::size_t index = 0; index < times.size(); ++index) {
            if (times[index] >= time) {
                groups[index] == 0 ? ++at_risk_one : ++at_risk_two;
            }
            if (times[index] == time && events[index]) {
                groups[index] == 0 ? ++events_one : ++events_two;
            }
        }
        const std::size_t total_at_risk = at_risk_one + at_risk_two;
        const std::size_t total_events = events_one + events_two;
        if (total_at_risk <= 1 || total_events == 0) {
            continue;
        }
        const double expected_one = static_cast<double>(total_events)
            * static_cast<double>(at_risk_one) / static_cast<double>(total_at_risk);
        observed_minus_expected += static_cast<double>(events_one) - expected_one;
        variance += static_cast<double>(total_events) * at_risk_one
            * at_risk_two * (total_at_risk - total_events)
            / (static_cast<double>(total_at_risk) * total_at_risk
               * (total_at_risk - 1));
        r.group_one_failures += events_one;
        r.group_two_failures += events_two;
    }
    if (!(variance > 0.0)) {
        error(r.diagnostics, "zero_log_rank_variance",
              "分组失效模式没有可估计的 Log-rank 方差。");
        return r;
    }
    r.chi_square = observed_minus_expected * observed_minus_expected / variance;
    r.p_value = std::clamp(std::erfc(std::sqrt(r.chi_square / 2.0)), 0.0, 1.0);
    r.group_one_censored = r.group_one_n - r.group_one_failures;
    r.group_two_censored = r.group_two_n - r.group_two_failures;
    r.evidence.method_version = "2";
    r.evidence.valid_count = times.size();
    r.rules.push_back({
        "risk_set", "not_triggered",
        "Log-rank 已按事件时间构造两组风险集并报告删失数。", {},
        "组间比较必须同时报告有效样本、失效数、删失数和自由度。"});
    return r;
}

LogRankKGroupsResult log_rank_k_groups(const std::vector<double>& times,
                                       const std::vector<bool>& events,
                                       const std::vector<int>& groups)
{
    LogRankKGroupsResult r;
    if (times.size() < 2 || times.size() != events.size()
        || times.size() != groups.size()) {
        error(r.diagnostics, "invalid_log_rank_shape",
              "Log-rank 检验要求寿命、事件和分组列长度一致。");
        return r;
    }
    int max_group = -1;
    for (const int group : groups) {
        if (group < 0) {
            error(r.diagnostics, "invalid_log_rank_value",
                  "Log-rank 分组编码必须为非负整数。");
            return r;
        }
        max_group = std::max(max_group, group);
    }
    const std::size_t group_count = static_cast<std::size_t>(max_group + 1);
    if (group_count < 2) {
        error(r.diagnostics, "insufficient_log_rank_groups",
              "Log-rank 检验至少需要两个非空分组。");
        return r;
    }
    std::vector<std::size_t> group_n(group_count, 0);
    std::vector<std::size_t> group_failures(group_count, 0);
    for (std::size_t index = 0; index < times.size(); ++index) {
        if (!std::isfinite(times[index]) || times[index] <= 0.0) {
            error(r.diagnostics, "invalid_log_rank_value",
                  "Log-rank 时间必须为正数。");
            return r;
        }
        const std::size_t group = static_cast<std::size_t>(groups[index]);
        if (group >= group_count) {
            error(r.diagnostics, "invalid_log_rank_value",
                  "Log-rank 分组编码超出已识别水平范围。");
            return r;
        }
        ++group_n[group];
        if (events[index]) {
            ++group_failures[group];
        }
    }
    r.group_summaries.resize(group_count);
    for (std::size_t group = 0; group < group_count; ++group) {
        if (group_n[group] == 0) {
            error(r.diagnostics, "insufficient_log_rank_groups",
                  "Log-rank 检验至少需要两个非空分组。");
            return r;
        }
        r.group_summaries[group] = {
            static_cast<int>(group),
            group_n[group],
            group_failures[group],
            group_n[group] - group_failures[group]};
    }
    std::vector<double> event_times;
    event_times.reserve(times.size());
    for (std::size_t index = 0; index < times.size(); ++index) {
        if (events[index]) {
            event_times.push_back(times[index]);
        }
    }
    std::sort(event_times.begin(), event_times.end());
    event_times.erase(std::unique(event_times.begin(), event_times.end()),
                      event_times.end());
    const std::size_t compare_groups = group_count - 1;
    std::vector<double> observed_minus_expected(compare_groups, 0.0);
    std::vector<std::vector<double>> variance(
        compare_groups, std::vector<double>(compare_groups, 0.0));
    for (const double time : event_times) {
        std::vector<std::size_t> at_risk(group_count, 0);
        std::vector<std::size_t> deaths(group_count, 0);
        for (std::size_t index = 0; index < times.size(); ++index) {
            if (times[index] >= time) {
                ++at_risk[static_cast<std::size_t>(groups[index])];
            }
            if (times[index] == time && events[index]) {
                ++deaths[static_cast<std::size_t>(groups[index])];
            }
        }
        std::size_t total_at_risk = 0;
        std::size_t total_events = 0;
        for (std::size_t group = 0; group < group_count; ++group) {
            total_at_risk += at_risk[group];
            total_events += deaths[group];
        }
        if (total_at_risk <= 1 || total_events == 0) {
            continue;
        }
        const double n = static_cast<double>(total_at_risk);
        const double d = static_cast<double>(total_events);
        const double tie_factor =
            static_cast<double>(total_at_risk - total_events)
            / static_cast<double>(total_at_risk - 1);
        for (std::size_t group = 0; group < compare_groups; ++group) {
            const double expected = d * static_cast<double>(at_risk[group]) / n;
            observed_minus_expected[group] +=
                static_cast<double>(deaths[group]) - expected;
            for (std::size_t other = 0; other < compare_groups; ++other) {
                if (group == other) {
                    variance[group][other] += d * static_cast<double>(at_risk[group]) / n
                        * (1.0 - static_cast<double>(at_risk[group]) / n)
                        * tie_factor;
                } else {
                    variance[group][other] -= d * static_cast<double>(at_risk[group]) / n
                        * static_cast<double>(at_risk[other]) / n
                        * tie_factor;
                }
            }
        }
    }
    std::vector<std::vector<double>> inverse;
    if (!invert_matrix(variance, &inverse)) {
        error(r.diagnostics, "zero_log_rank_variance",
              "分组失效模式没有可估计的 Log-rank 方差。");
        return r;
    }
    r.chi_square = quadratic_form(observed_minus_expected, inverse);
    if (!(r.chi_square >= 0.0) || !std::isfinite(r.chi_square)) {
        error(r.diagnostics, "zero_log_rank_variance",
              "分组失效模式没有可估计的 Log-rank 方差。");
        return r;
    }
    r.df = static_cast<double>(compare_groups);
    r.p_value = chi_square_right_tail(r.chi_square, r.df);
    r.evidence.method_version = "3";
    r.evidence.valid_count = times.size();
    r.rules.push_back({
        "risk_set", "not_triggered",
        "Log-rank 已按事件时间构造 K 组风险集并报告删失数。", {},
        "组间比较必须同时报告有效样本、失效数、删失数、自由度和 P 值。"});
    return r;
}

WeibullResult fit_weibull(const std::vector<double>& times,
                          const std::vector<bool>& events)
{
    WeibullResult r;
    if (!valid(times, events, r.diagnostics)) {
        return r;
    }
    return fit_weibull_core(times, events);
}

WeibullResult fit_weibull3(const std::vector<double>& times,
                           const std::vector<bool>& events)
{
    WeibullResult r;
    if (!valid(times, events, r.diagnostics)) {
        return r;
    }
    std::size_t failures = 0;
    std::vector<double> failure_times;
    double min_time = 0.0;
    double max_time = 0.0;
    lifetime_span(times, events, &failures, &min_time, &max_time, &failure_times);
    if (failures < 3) {
        error(r.diagnostics, "few_failures",
              "三参数 Weibull 至少需要三次失效才能识别阈值。");
        r.not_computed_reason = "few_failures";
        r.rules.push_back({
            "identifiability", "triggered",
            "失效数不足，三参数 Weibull 不可识别。", {},
            "不要输出伪造的 Shape=1 或 Threshold=0。"});
        return r;
    }
    if (failure_times.size() < 3) {
        error(r.diagnostics, "non_identifiable_weibull",
              "失效时间没有足够变化，无法识别三参数 Weibull。");
        r.not_computed_reason = "insufficient_variation";
        return r;
    }

    WeibullResult best;
    bool saw_unbounded = false;
    for (const double lambda : threshold_candidates(min_time, max_time)) {
        if (!(lambda < min_time)) {
            continue;
        }
        std::vector<double> shifted;
        if (!shift_by_threshold(times, lambda, &shifted)) {
            continue;
        }
        WeibullResult candidate = fit_weibull_core(shifted, events);
        if (!candidate.identifiable || !candidate.converged) {
            continue;
        }
        if (candidate.shape <= 1.0) {
            saw_unbounded = true;
            continue;
        }
        if (!best.identifiable || candidate.log_likelihood > best.log_likelihood) {
            best = std::move(candidate);
            best.threshold = lambda;
        }
    }
    if (!best.identifiable) {
        if (saw_unbounded) {
            error(r.diagnostics, "weibull3_likelihood_unbounded",
                  "三参数 Weibull 似然无界（常见于形状 ≤ 1）；不估计阈值，也不伪造参数。");
            r.not_computed_reason = "likelihood_unbounded";
        } else {
            error(r.diagnostics, "non_identifiable_weibull",
                  "未能找到形状大于 1 的有限三参数 Weibull 估计。");
            r.not_computed_reason = "no_interior_maximum";
        }
        r.rules.push_back({
            "identifiability", "triggered",
            "三参数 Weibull 不可识别。", {},
            "不可识别时只输出诊断，不要填写 Shape=1 或伪造 Threshold。"});
        return r;
    }
    add_model_metrics(best.shape, best.scale, best.observations, best.failures,
                      best.log_likelihood, best, 3);
    best.evidence.method_version = "weibull3-1";
    best.rules.push_back({
        "threshold", "not_triggered",
        "三参数 Weibull 使用剖面似然估计阈值；这不是 Minitab 无界似然 bias-correction。",
        {},
        "报告 Shape、Scale、Threshold 和分位寿命；拟合未拒绝假设不等于寿命服从该分布。"});
    return best;
}

ExponentialResult fit_exponential(const std::vector<double>& times,
                                  const std::vector<bool>& events)
{
    ExponentialResult r;
    if (!valid(times, events, r.diagnostics)) return r;
    double exposure = std::accumulate(times.begin(), times.end(), 0.0);
    const std::size_t failures = static_cast<std::size_t>(
        std::count(events.begin(), events.end(), true));
    if (failures == 0 || exposure <= 0.0) {
        error(r.diagnostics, "insufficient_failures", "指数模型至少需要一条失效记录。");
        return r;
    }
    r.rate = static_cast<double>(failures) / exposure;
    r.mean_life = 1.0 / r.rate;
    r.log_likelihood = static_cast<double>(failures) * std::log(r.rate) -
        r.rate * exposure;
    r.aic = 2.0 - 2.0 * r.log_likelihood;
    r.bic = std::log(static_cast<double>(times.size())) -
        2.0 * r.log_likelihood;
    r.b10 = percentile_life_exponential_impl(r.rate, 10.0);
    r.b50 = percentile_life_exponential_impl(r.rate, 50.0);
    r.b90 = percentile_life_exponential_impl(r.rate, 90.0);
    r.failures = failures;
    r.observations = times.size();
    r.identifiable = true;
    r.converged = true;
    r.evidence.method_version = "2";
    r.evidence.valid_count = times.size();
    r.rules.push_back({
        "identifiability", "not_triggered",
        "指数模型在恒定失效率假设下可估计。", {},
        "指数模型不能描述随时间上升或下降的失效率，应与 Weibull 比较。"});
    return r;
}

ExponentialResult fit_exponential2(const std::vector<double>& times,
                                   const std::vector<bool>& events)
{
    ExponentialResult r;
    if (!valid(times, events, r.diagnostics)) {
        return r;
    }
    std::size_t failures = 0;
    double min_time = 0.0;
    double max_time = 0.0;
    std::vector<double> unique_failures;
    lifetime_span(times, events, &failures, &min_time, &max_time, &unique_failures);
    if (failures < 1) {
        error(r.diagnostics, "insufficient_failures",
              "两参数指数至少需要一条失效记录。");
        r.not_computed_reason = "few_failures";
        r.rules.push_back({
            "identifiability", "triggered",
            "失效数不足，两参数指数不可识别。", {},
            "不要输出伪造的 Scale=1 或 Threshold=0。"});
        return r;
    }
    if (unique_failures.size() < 2) {
        error(r.diagnostics, "exponential2_likelihood_unbounded",
              "失效时间没有足够变化，两参数指数似然无界；不估计阈值，也不伪造参数。");
        r.not_computed_reason = "likelihood_unbounded";
        r.rules.push_back({
            "identifiability", "triggered",
            "两参数指数不可识别。", {},
            "不可识别时只输出诊断，不要填写 Scale=1 或伪造 Threshold。"});
        return r;
    }

    ExponentialResult best;
    for (const double lambda : threshold_candidates(min_time, max_time)) {
        if (!(lambda < min_time)) {
            continue;
        }
        std::vector<double> shifted;
        if (!shift_by_threshold(times, lambda, &shifted)) {
            continue;
        }
        ExponentialResult candidate = fit_exponential(shifted, events);
        if (!candidate.identifiable || !(candidate.rate > 0.0)
            || !std::isfinite(candidate.log_likelihood)) {
            continue;
        }
        if (1.0 / candidate.rate < 1.0e-12 * std::max(max_time - min_time, 1.0)) {
            continue;
        }
        if (!best.identifiable
            || candidate.log_likelihood > best.log_likelihood) {
            best = std::move(candidate);
            best.threshold = lambda;
        }
    }
    if (!best.identifiable) {
        error(r.diagnostics, "exponential2_likelihood_unbounded",
              "两参数指数未能找到有限阈值估计；不伪造参数。");
        r.not_computed_reason = "likelihood_unbounded";
        r.rules.push_back({
            "identifiability", "triggered",
            "两参数指数不可识别。", {},
            "不可识别时只输出诊断，不要填写 Scale=1 或伪造 Threshold。"});
        return r;
    }
    const double offset = *best.threshold;
    best.mean_life = 1.0 / best.rate + offset;
    best.aic = 4.0 - 2.0 * best.log_likelihood;
    best.bic = 2.0 * std::log(static_cast<double>(best.observations))
        - 2.0 * best.log_likelihood;
    best.b10 = offset + percentile_life_exponential_impl(best.rate, 10.0);
    best.b50 = offset + percentile_life_exponential_impl(best.rate, 50.0);
    best.b90 = offset + percentile_life_exponential_impl(best.rate, 90.0);
    best.converged = true;
    best.evidence.method_version = "exponential2-1";
    best.rules.push_back({
        "threshold", "not_triggered",
        "两参数指数使用剖面似然估计阈值；这不是 Minitab 无界似然 bias-correction。",
        {},
        "报告 Scale、Threshold 和分位寿命；拟合未拒绝假设不等于寿命服从该分布。"});
    return best;
}

LognormalResult fit_lognormal(const std::vector<double>& times,
                              const std::vector<bool>& events)
{
    LognormalResult r;
    if (!valid(times, events, r.diagnostics)) {
        return r;
    }
    std::vector<double> failure_logs;
    for (std::size_t index = 0; index < times.size(); ++index) {
        if (events[index]) {
            failure_logs.push_back(std::log(times[index]));
        }
    }
    const std::size_t failures = failure_logs.size();
    if (failures == 0) {
        error(r.diagnostics, "non_identifiable_lognormal",
              "全为删失，无法识别对数正态参数。");
        r.not_computed_reason = "all_censored";
        r.rules.push_back({
            "identifiability", "triggered",
            "全为删失，对数正态位置和尺度不可估计。", {},
            "不要输出默认位置参数或伪造分位寿命。"});
        return r;
    }
    if (failures < 2) {
        error(r.diagnostics, "few_failures",
              "只有一条失效记录时对数正态尺度通常不可稳定识别。");
        r.not_computed_reason = "few_failures";
        r.rules.push_back({
            "identifiability", "triggered",
            "失效数不足，尺度参数不可稳定识别。", {},
            "至少需要两次失效才能解释对数正态尺度。"});
        return r;
    }

    double location = std::accumulate(failure_logs.cbegin(), failure_logs.cend(), 0.0)
        / static_cast<double>(failures);
    double sum_sq = 0.0;
    for (const double value : failure_logs) {
        const double delta = value - location;
        sum_sq += delta * delta;
    }
    double scale = std::sqrt(std::max(sum_sq / static_cast<double>(failures), 1.0e-12));
    bool complete = failures == times.size();
    if (complete) {
        double all_mean = 0.0;
        for (const double time : times) {
            all_mean += std::log(time);
        }
        all_mean /= static_cast<double>(times.size());
        double all_sq = 0.0;
        for (const double time : times) {
            const double delta = std::log(time) - all_mean;
            all_sq += delta * delta;
        }
        location = all_mean;
        scale = std::sqrt(std::max(all_sq / static_cast<double>(times.size()), 1.0e-12));
    } else {
        double best = lognormal_log_likelihood(location, scale, times, events);
        double step = std::max(0.2, 0.25 * scale);
        for (int iteration = 0; iteration < 80; ++iteration) {
            bool improved = false;
            const double candidates_location[3] = {
                location, location - step, location + step};
            const double candidates_scale[3] = {
                scale, std::max(1.0e-6, scale - step), scale + step};
            for (const double next_location : candidates_location) {
                for (const double next_scale : candidates_scale) {
                    const double value = lognormal_log_likelihood(
                        next_location, next_scale, times, events);
                    if (value > best) {
                        best = value;
                        location = next_location;
                        scale = next_scale;
                        improved = true;
                    }
                }
            }
            if (!improved) {
                step *= 0.5;
            }
            if (step < 1.0e-8) {
                break;
            }
            r.iterations = iteration + 1;
        }
    }

    const double log_likelihood = lognormal_log_likelihood(location, scale, times, events);
    r.location = location;
    r.scale = scale;
    r.log_likelihood = log_likelihood;
    r.aic = 4.0 - 2.0 * log_likelihood;
    r.bic = std::log(static_cast<double>(times.size())) * 2.0 - 2.0 * log_likelihood;
    r.b10 = percentile_life_lognormal_impl(location, scale, 10.0);
    r.b50 = percentile_life_lognormal_impl(location, scale, 50.0);
    r.b90 = percentile_life_lognormal_impl(location, scale, 90.0);
    r.median_life = r.b50;
    r.failures = failures;
    r.observations = times.size();
    r.censoring_fraction = 1.0 - static_cast<double>(failures)
        / static_cast<double>(times.size());
    r.identifiable = true;
    r.converged = std::isfinite(location) && std::isfinite(scale)
        && std::isfinite(log_likelihood);
    r.evidence.method_version = "2";
    r.evidence.valid_count = times.size();
    r.evidence.assumption_status = "not_verified";
    r.rules.push_back({
        "convergence",
        r.converged ? "not_triggered" : "triggered",
        r.converged ? "对数正态参数估计已收敛。" : "对数正态参数未收敛。",
        {},
        "报告位置、尺度、删失似然和分位寿命；不要把分位寿命当成单件保证寿命。"});
    return r;
}

LognormalResult fit_lognormal3(const std::vector<double>& times,
                               const std::vector<bool>& events)
{
    LognormalResult r;
    if (!valid(times, events, r.diagnostics)) {
        return r;
    }
    std::size_t failures = 0;
    double min_time = 0.0;
    double max_time = 0.0;
    std::vector<double> unique_failures;
    lifetime_span(times, events, &failures, &min_time, &max_time, &unique_failures);
    if (failures < 2) {
        error(r.diagnostics, "few_failures",
              "三参数对数正态至少需要两次失效才能识别阈值。");
        r.not_computed_reason = "few_failures";
        r.rules.push_back({
            "identifiability", "triggered",
            "失效数不足，三参数对数正态不可识别。", {},
            "不要输出伪造的 Location=0 或 Threshold=0。"});
        return r;
    }
    if (unique_failures.size() < 2) {
        error(r.diagnostics, "lognormal3_likelihood_unbounded",
              "失效时间没有足够变化，三参数对数正态似然无界；不估计阈值，也不伪造参数。");
        r.not_computed_reason = "likelihood_unbounded";
        r.rules.push_back({
            "identifiability", "triggered",
            "三参数对数正态不可识别。", {},
            "不可识别时只输出诊断，不要填写伪造参数。"});
        return r;
    }

    LognormalResult best;
    bool saw_unbounded = false;
    for (const double lambda : threshold_candidates(min_time, max_time)) {
        if (!(lambda < min_time)) {
            continue;
        }
        std::vector<double> shifted;
        if (!shift_by_threshold(times, lambda, &shifted)) {
            continue;
        }
        LognormalResult candidate = fit_lognormal(shifted, events);
        if (!candidate.identifiable || !candidate.converged) {
            continue;
        }
        if (!(candidate.scale > 1.0e-6)) {
            saw_unbounded = true;
            continue;
        }
        if (!best.identifiable
            || candidate.log_likelihood > best.log_likelihood) {
            best = std::move(candidate);
            best.threshold = lambda;
        }
    }
    if (!best.identifiable) {
        error(r.diagnostics,
              saw_unbounded ? "lognormal3_likelihood_unbounded"
                            : "non_identifiable_lognormal",
              saw_unbounded
                  ? "三参数对数正态似然无界；不估计阈值，也不伪造参数。"
                  : "未能找到有限的三参数对数正态估计。");
        r.not_computed_reason = saw_unbounded
            ? "likelihood_unbounded" : "no_interior_maximum";
        r.rules.push_back({
            "identifiability", "triggered",
            "三参数对数正态不可识别。", {},
            "不可识别时只输出诊断，不要填写伪造 Location 或 Threshold。"});
        return r;
    }
    const double offset = *best.threshold;
    best.aic = 6.0 - 2.0 * best.log_likelihood;
    best.bic = 3.0 * std::log(static_cast<double>(best.observations))
        - 2.0 * best.log_likelihood;
    best.b10 = offset + percentile_life_lognormal_impl(
        best.location, best.scale, 10.0);
    best.b50 = offset + percentile_life_lognormal_impl(
        best.location, best.scale, 50.0);
    best.b90 = offset + percentile_life_lognormal_impl(
        best.location, best.scale, 90.0);
    best.median_life = best.b50;
    best.evidence.method_version = "lognormal3-1";
    best.rules.push_back({
        "threshold", "not_triggered",
        "三参数对数正态使用剖面似然估计阈值；这不是 Minitab 无界似然 bias-correction。",
        {},
        "报告 Location、Scale、Threshold 和分位寿命；拟合未拒绝假设不等于寿命服从该分布。"});
    return best;
}

double percentile_life_weibull(double shape, double scale, double percentile)
{
    return percentile_life_weibull_impl(shape, scale, percentile);
}

double percentile_life_weibull3(double shape, double scale, double threshold,
                                double percentile)
{
    return threshold + percentile_life_weibull_impl(shape, scale, percentile);
}

double percentile_life_exponential(double rate, double percentile)
{
    return percentile_life_exponential_impl(rate, percentile);
}

double percentile_life_exponential2(double rate, double threshold, double percentile)
{
    return threshold + percentile_life_exponential_impl(rate, percentile);
}

double percentile_life_lognormal(double location, double scale, double percentile)
{
    return percentile_life_lognormal_impl(location, scale, percentile);
}

double percentile_life_lognormal3(double location, double scale, double threshold,
                                  double percentile)
{
    return threshold + percentile_life_lognormal_impl(location, scale, percentile);
}

double cdf_weibull3(double time, double shape, double scale, double threshold)
{
    if (!(shape > 0.0) || !(scale > 0.0) || !std::isfinite(time)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (time <= threshold) {
        return 0.0;
    }
    return 1.0 - std::exp(-std::pow((time - threshold) / scale, shape));
}

double cdf_exponential2(double time, double rate, double threshold)
{
    if (!(rate > 0.0) || !std::isfinite(time)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (time <= threshold) {
        return 0.0;
    }
    return 1.0 - std::exp(-rate * (time - threshold));
}

double cdf_lognormal3(double time, double location, double scale, double threshold)
{
    if (!(scale > 0.0) || !std::isfinite(time)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (time <= threshold) {
        return 0.0;
    }
    return standard_normal_cdf((std::log(time - threshold) - location) / scale);
}

std::optional<double> percentile_life_km(
    const std::vector<KaplanMeierPoint>& points,
    double percentile)
{
    if (points.empty()) {
        return std::nullopt;
    }
    const double target_survival = 1.0 - std::clamp(percentile / 100.0, 0.0, 1.0);
    for (std::size_t index = 1; index < points.size(); ++index) {
        const auto& previous = points[index - 1];
        const auto& current = points[index];
        if (current.survival <= target_survival
            && previous.survival >= target_survival) {
            const double span = previous.survival - current.survival;
            if (span <= 0.0) {
                return current.time;
            }
            const double fraction = (previous.survival - target_survival) / span;
            return previous.time + fraction * (current.time - previous.time);
        }
    }
    if (points.back().survival > target_survival) {
        return std::nullopt;
    }
    return points.back().time;
}

std::vector<ParametricDistributionCandidate> compare_parametric_distributions(
    const std::vector<double>& times,
    const std::vector<bool>& events)
{
    std::vector<ParametricDistributionCandidate> candidates;
    const WeibullResult weibull = fit_weibull(times, events);
    ParametricDistributionCandidate weibull_candidate;
    weibull_candidate.name = "Weibull";
    weibull_candidate.aic = weibull.aic;
    weibull_candidate.bic = weibull.bic;
    weibull_candidate.converged = weibull.converged && weibull.identifiable;
    weibull_candidate.diagnostics = weibull.diagnostics;
    candidates.push_back(std::move(weibull_candidate));

    const ExponentialResult exponential = fit_exponential(times, events);
    ParametricDistributionCandidate exponential_candidate;
    exponential_candidate.name = "Exponential";
    exponential_candidate.aic = exponential.aic;
    exponential_candidate.bic = exponential.bic;
    exponential_candidate.converged = exponential.identifiable;
    exponential_candidate.diagnostics = exponential.diagnostics;
    candidates.push_back(std::move(exponential_candidate));

    const LognormalResult lognormal = fit_lognormal(times, events);
    ParametricDistributionCandidate lognormal_candidate;
    lognormal_candidate.name = "Lognormal";
    lognormal_candidate.aic = lognormal.aic;
    lognormal_candidate.bic = lognormal.bic;
    lognormal_candidate.converged = lognormal.converged && lognormal.identifiable;
    lognormal_candidate.diagnostics = lognormal.diagnostics;
    candidates.push_back(std::move(lognormal_candidate));
    return candidates;
}

}  // namespace datalab::domain::statistics
