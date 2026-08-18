#include "domain/statistics/reliability.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <string>

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

void add_model_metrics(double shape, double scale, std::size_t n,
                       std::size_t failures, double log_likelihood,
                       WeibullResult& r)
{
    r.shape = shape;
    r.scale = scale;
    r.log_likelihood = log_likelihood;
    r.aic = 4.0 - 2.0 * log_likelihood;
    r.bic = std::log(static_cast<double>(n)) * 2.0 - 2.0 * log_likelihood;
    r.b10 = scale * std::pow(-std::log(0.9), 1.0 / shape);
    r.b50 = scale * std::pow(std::log(2.0), 1.0 / shape);
    r.b90 = scale * std::pow(-std::log(0.1), 1.0 / shape);
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
}

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
    }
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
    return r;
}

WeibullResult fit_weibull(const std::vector<double>& times,
                          const std::vector<bool>& events)
{
    WeibullResult r;
    if (!valid(times, events, r.diagnostics)) return r;
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
        return r;
    }
    if (failures < 2) {
        error(r.diagnostics, "few_failures",
              "只有一条失效记录时 Weibull 形状参数通常不可稳定识别。");
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
    return r;
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
    r.b10 = -std::log(0.9) / r.rate;
    r.b50 = std::log(2.0) / r.rate;
    r.b90 = -std::log(0.1) / r.rate;
    r.failures = failures;
    r.observations = times.size();
    r.identifiable = true;
    return r;
}
}  // namespace datalab::domain::statistics
