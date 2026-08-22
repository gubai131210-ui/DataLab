#include "domain/statistics/hartigan_dip.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

namespace datalab::domain::statistics {
namespace {

constexpr std::size_t k_min_n = 8;

// Greatest convex minorant of (t[k], y[k]) for k in [0, end_inclusive], returned as
// gcm_y[k] on the same t grid (piecewise-linear lower convex hull).
std::vector<double> greatest_convex_minorant(
    const std::vector<double>& t,
    const std::vector<double>& y,
    const std::size_t end_inclusive)
{
    std::vector<double> gcm(end_inclusive + 1, 0.0);
    if (end_inclusive == 0) {
        gcm[0] = y[0];
        return gcm;
    }
    std::vector<std::size_t> hull;
    hull.reserve(end_inclusive + 1);
    for (std::size_t i = 0; i <= end_inclusive; ++i) {
        while (hull.size() >= 2) {
            const std::size_t b = hull[hull.size() - 1];
            const std::size_t a = hull[hull.size() - 2];
            const double cross = (t[b] - t[a]) * (y[i] - y[a])
                - (t[i] - t[a]) * (y[b] - y[a]);
            // Lower hull (greatest convex minorant): drop left turns / collinear.
            if (cross >= 0.0) {
                hull.pop_back();
            } else {
                break;
            }
        }
        hull.push_back(i);
    }
    std::size_t h = 0;
    for (std::size_t i = 0; i <= end_inclusive; ++i) {
        while (h + 1 < hull.size() && hull[h + 1] <= i) {
            ++h;
        }
        if (hull[h] == i) {
            gcm[i] = y[i];
        } else {
            const std::size_t a = hull[h];
            const std::size_t b = hull[h + 1];
            const double w = (t[i] - t[a]) / (t[b] - t[a]);
            gcm[i] = y[a] + w * (y[b] - y[a]);
        }
    }
    return gcm;
}

// Least concave majorant on [start, n).
std::vector<double> least_concave_majorant(
    const std::vector<double>& t,
    const std::vector<double>& y,
    const std::size_t start)
{
    const std::size_t n = t.size();
    std::vector<double> lcm(n, 0.0);
    if (start >= n) {
        return lcm;
    }
    if (start + 1 == n) {
        lcm[start] = y[start];
        return lcm;
    }
    // Reflect for concave = -convex of -y
    std::vector<double> t_suf;
    std::vector<double> y_neg;
    t_suf.reserve(n - start);
    y_neg.reserve(n - start);
    for (std::size_t i = start; i < n; ++i) {
        t_suf.push_back(t[i]);
        y_neg.push_back(-y[i]);
    }
    const auto gcm_neg = greatest_convex_minorant(t_suf, y_neg, t_suf.size() - 1);
    for (std::size_t i = 0; i < gcm_neg.size(); ++i) {
        lcm[start + i] = -gcm_neg[i];
    }
    return lcm;
}

double dip_for_modal_interval(
    const std::vector<double>& x,
    const std::vector<double>& F,
    const std::size_t left,
    const std::size_t right)
{
    const std::size_t n = x.size();
    // Left of modal interval: GCM of F on [0, left]
    const auto gcm = greatest_convex_minorant(x, F, left);
    // Right of modal interval: LCM of F on [right, n)
    const auto lcm = least_concave_majorant(x, F, right);

    double max_diff = 0.0;
    for (std::size_t i = 0; i <= left; ++i) {
        max_diff = std::max(max_diff, std::abs(F[i] - gcm[i]));
    }
    for (std::size_t i = right; i < n; ++i) {
        max_diff = std::max(max_diff, std::abs(F[i] - lcm[i]));
    }
    // Inside modal interval, the closest unimodal cdf is linear in F between endpoints.
    if (right > left) {
        const double F_l = F[left];
        const double F_r = F[right];
        const double x_l = x[left];
        const double x_r = x[right];
        const double dx = x_r - x_l;
        for (std::size_t i = left; i <= right; ++i) {
            const double g = (dx <= 0.0)
                ? F_l
                : F_l + (F_r - F_l) * ((x[i] - x_l) / dx);
            max_diff = std::max(max_diff, std::abs(F[i] - g));
        }
    }
    // Hartigan dip is half the L∞ distance to the closest unimodal cdf.
    return 0.5 * max_diff;
}

double dip_statistic_sorted(const std::vector<double>& x_sorted)
{
    const std::size_t n = x_sorted.size();
    if (n < 2) {
        return 0.0;
    }
    if (!(x_sorted.back() > x_sorted.front())) {
        return 0.0;
    }

    std::vector<double> F(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        F[i] = static_cast<double>(i + 1) / static_cast<double>(n);
    }

    double best = std::numeric_limits<double>::infinity();
    // Exhaustive modal endpoints (formula_reference O(n^2); fine for capability n).
    for (std::size_t left = 0; left < n; ++left) {
        for (std::size_t right = left; right < n; ++right) {
            best = std::min(best, dip_for_modal_interval(x_sorted, F, left, right));
        }
    }
    if (!std::isfinite(best)) {
        return 0.0;
    }
    return std::max(0.0, best);
}

std::vector<double> finite_sorted(const std::vector<double>& observations)
{
    std::vector<double> values;
    values.reserve(observations.size());
    for (double value : observations) {
        if (std::isfinite(value)) {
            values.push_back(value);
        }
    }
    std::sort(values.begin(), values.end());
    return values;
}

}  // namespace

HartiganDipResult compute_hartigan_dip(
    const std::vector<double>& observations,
    const std::size_t mc_reps,
    const std::uint64_t seed)
{
    HartiganDipResult result;
    result.evidence_type = "formula_reference";
    result.algorithm_id = "hartigan_dip_1985";
    result.mc_reps = mc_reps;

    auto values = finite_sorted(observations);
    result.n = values.size();
    if (result.n < k_min_n) {
        result.status = "insufficient_n";
        result.dip = 0.0;
        return result;
    }

    result.dip = dip_statistic_sorted(values);

    if (mc_reps == 0) {
        // Without a calibrated null, do not claim evidence_against from dip alone.
        result.status = "consistent";
        return result;
    }

    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::size_t ge = 0;
    std::vector<double> draw(result.n);
    for (std::size_t rep = 0; rep < mc_reps; ++rep) {
        for (std::size_t i = 0; i < result.n; ++i) {
            draw[i] = unit(rng);
        }
        std::sort(draw.begin(), draw.end());
        const double d = dip_statistic_sorted(draw);
        if (d >= result.dip - 1e-15) {
            ++ge;
        }
    }
    result.p_value = static_cast<double>(ge + 1) / static_cast<double>(mc_reps + 1);
    // α=0.05 research screen under Uniform null — not commercial software α.
    result.status = (*result.p_value < 0.05) ? "evidence_against" : "consistent";
    return result;
}

}  // namespace datalab::domain::statistics
