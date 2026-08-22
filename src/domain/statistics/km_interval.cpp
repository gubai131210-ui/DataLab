#include "domain/statistics/km_interval.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <set>

namespace datalab::domain::statistics {
namespace {

constexpr double k_inf = std::numeric_limits<double>::infinity();

bool is_right_open(const IntervalObservation& obs)
{
    return !std::isfinite(obs.right);
}

}  // namespace

KmIntervalResult kaplan_meier_interval(
    const std::vector<IntervalObservation>& observations,
    std::size_t max_iterations,
    double tolerance)
{
    KmIntervalResult result;
    result.observation_count = observations.size();
    if (observations.size() < 3) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "km_interval_n",
            "区间删失 KM 至少需要约 3 条观测。"});
        return result;
    }

    std::vector<IntervalObservation> cleaned;
    cleaned.reserve(observations.size());
    for (const auto& obs : observations) {
        if (!std::isfinite(obs.left)) {
            continue;
        }
        IntervalObservation copy = obs;
        if (is_right_open(copy)) {
            ++result.right_censored_count;
        } else if (copy.left <= 0.0 && copy.right > copy.left
                   && std::abs(copy.left) < 1.0e-15) {
            // treat L≈0 as possible left-censored style if flagged by caller via L=0
            if (copy.right > copy.left + 1.0e-12) {
                // distinguish exact vs interval below
            }
        }
        if (std::isfinite(copy.right) && copy.right < copy.left) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "km_interval_order",
                "存在 R < L 的无效区间。"});
            return result;
        }
        if (std::isfinite(copy.right) && std::abs(copy.right - copy.left) < 1.0e-12) {
            ++result.exact_count;
        } else if (std::isfinite(copy.right) && copy.left <= 0.0
                   && copy.right > copy.left) {
            ++result.left_censored_count;
        } else if (std::isfinite(copy.right)) {
            ++result.interval_censored_count;
        }
        cleaned.push_back(copy);
    }
    result.observation_count = cleaned.size();
    if (cleaned.size() < 3) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "km_interval_clean",
            "有效区间过少。"});
        return result;
    }

    // Candidate mass points: unique finite endpoints (Turnbull grid).
    std::set<double> grid;
    for (const auto& obs : cleaned) {
        grid.insert(obs.left);
        if (std::isfinite(obs.right)) {
            grid.insert(obs.right);
        }
    }
    std::vector<double> times(grid.begin(), grid.end());
    if (times.size() < 2) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "km_interval_grid",
            "无法构造 Turnbull 时间网格。"});
        return result;
    }

    // Use midpoints of consecutive grid intervals as mass locations (simplified Turnbull).
    std::vector<double> mass_times;
    for (std::size_t i = 0; i + 1 < times.size(); ++i) {
        mass_times.push_back(0.5 * (times[i] + times[i + 1]));
    }
    // Also allow mass at exact failure times.
    for (const auto& obs : cleaned) {
        if (std::isfinite(obs.right) && std::abs(obs.right - obs.left) < 1.0e-12) {
            mass_times.push_back(obs.left);
        }
    }
    std::sort(mass_times.begin(), mass_times.end());
    mass_times.erase(std::unique(mass_times.begin(), mass_times.end()), mass_times.end());
    const std::size_t m = mass_times.size();
    if (m == 0) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "km_interval_mass",
            "无候选质量点。"});
        return result;
    }

    auto covers = [&](const IntervalObservation& obs, double t) {
        if (is_right_open(obs)) {
            return t > obs.left;  // (L, +∞)
        }
        if (std::abs(obs.right - obs.left) < 1.0e-12) {
            return std::abs(t - obs.left) < 1.0e-9;
        }
        // (L, R]
        return t > obs.left && t <= obs.right + 1.0e-15;
    };

    std::vector<double> p(m, 1.0 / static_cast<double>(m));
    for (std::size_t iter = 0; iter < max_iterations; ++iter) {
        std::vector<double> next(m, 0.0);
        for (const auto& obs : cleaned) {
            double denom = 0.0;
            for (std::size_t j = 0; j < m; ++j) {
                if (covers(obs, mass_times[j])) {
                    denom += p[j];
                }
            }
            if (!(denom > 0.0)) {
                continue;
            }
            for (std::size_t j = 0; j < m; ++j) {
                if (covers(obs, mass_times[j])) {
                    next[j] += p[j] / denom;
                }
            }
        }
        const double total =
            std::accumulate(next.begin(), next.end(), 0.0);
        if (!(total > 0.0)) {
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "km_interval_collapse",
                "Turnbull 质量塌缩。"});
            return result;
        }
        double delta = 0.0;
        for (std::size_t j = 0; j < m; ++j) {
            next[j] /= total;
            delta += std::abs(next[j] - p[j]);
        }
        p = next;
        result.iteration_count = iter + 1;
        if (delta < tolerance) {
            result.converged = true;
            break;
        }
    }
    if (!result.converged) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "km_interval_max_iter",
            "Turnbull 已达最大迭代。"});
    }

    // Survival function: S(t) = 1 - F(t), F = cumulative mass at times ≤ t.
    double cum = 0.0;
    result.points.push_back({mass_times.front() > 0.0 ? 0.0 : mass_times.front() - 1.0,
                             1.0, 0.0});
    for (std::size_t j = 0; j < m; ++j) {
        if (p[j] < 1.0e-12) {
            continue;
        }
        cum += p[j];
        KmIntervalPoint point;
        point.time = mass_times[j];
        point.mass = p[j];
        point.survival = std::max(0.0, 1.0 - cum);
        result.points.push_back(point);
        if (!result.median_life.has_value() && point.survival <= 0.5) {
            result.median_life = point.time;
        }
    }
    result.identifiable = cum > 0.5 || result.median_life.has_value()
        || result.exact_count + result.interval_censored_count
            + result.left_censored_count
            > 0;
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "km_interval_scope",
        "Turnbull NPMLE（简化网格）；非右删失 product-limit；非 Minitab golden。"});
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "km_interval_evidence",
        "证据类型 = formula_reference（algorithm_id=turnbull_npmle_simplified_grid）；"
        "不得写成 vendor_oracle / golden / 商业软件对齐。"});
    return result;
}

}  // namespace datalab::domain::statistics
