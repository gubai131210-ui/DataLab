#include "domain/statistics/gray_test.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <string>
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

struct GrayRow {
    double time = 0.0;
    bool is_failure = false;
    std::string group;
    std::string mode;
};

}  // namespace

GrayTestResult gray_test_cif(const std::vector<CensoringObservation>& observations)
{
    GrayTestResult result;
    std::set<std::string> groups;
    std::set<std::string> modes;
    std::map<std::string, std::size_t> failures_by_group;
    std::vector<GrayRow> rows;
    rows.reserve(observations.size());
    std::size_t skipped_left_interval = 0;

    for (const CensoringObservation& observation : observations) {
        if (observation.type == CensoringType::left
            || observation.type == CensoringType::interval) {
            ++skipped_left_interval;
            continue;
        }
        if (!(observation.time >= 0.0) || !std::isfinite(observation.time)) {
            continue;
        }
        if (observation.group.empty()) {
            continue;
        }
        groups.insert(observation.group);
        GrayRow row;
        row.time = observation.time;
        row.group = observation.group;
        if (observation.type == CensoringType::right) {
            row.is_failure = false;
            rows.push_back(std::move(row));
            continue;
        }
        if (observation.failure_mode.empty()) {
            continue;
        }
        row.is_failure = true;
        row.mode = observation.failure_mode;
        modes.insert(observation.failure_mode);
        ++failures_by_group[observation.group];
        rows.push_back(std::move(row));
    }

    if (groups.size() < 2) {
        result.not_computed_reason = "insufficient_groups";
        result.diagnostics.push_back(info_msg(
            "gray_insufficient_groups",
            "Gray 检验需要至少两个非空分组。"));
        return result;
    }
    if (modes.size() < 2) {
        result.not_computed_reason = "insufficient_failure_modes";
        result.diagnostics.push_back(info_msg(
            "gray_insufficient_modes",
            "Gray 检验需要至少两个标注失效模式。"));
        return result;
    }
    for (const std::string& group : groups) {
        if (failures_by_group[group] == 0) {
            result.not_computed_reason = "group_without_labeled_failure";
            result.diagnostics.push_back(info_msg(
                "gray_group_without_failure",
                "Gray 检验要求每组至少一次带 failure_mode 标签的 exact 失效。"));
            return result;
        }
    }

    result.group_count = groups.size();
    result.mode_count = modes.size();
    result.ran = true;

    if (skipped_left_interval > 0) {
        result.diagnostics.push_back(info_msg(
            "gray_left_interval_omitted",
            "Gray 检验省略 left/interval 行 "
                + std::to_string(skipped_left_interval) + " 条。"));
    }
    result.diagnostics.push_back(info_msg(
        "gray_scope",
        "Gray 1988 风格 CIF 组间比较（formula_reference / gray_cif_group_test）；"
        "非 Minitab 菜单项；Log-rank 不用于竞争风险 CIF。"));
    result.diagnostics.push_back(warning_msg(
        "gray_not_fine_gray",
        "Gray 检验比较 CIF 组间差异；不是 Fine-Gray 回归，也不是 cause-specific Cox。"));

    std::sort(rows.begin(), rows.end(), [](const GrayRow& left, const GrayRow& right) {
        return left.time < right.time;
    });

    std::vector<double> event_times;
    for (const GrayRow& row : rows) {
        if (row.is_failure) {
            event_times.push_back(row.time);
        }
    }
    std::sort(event_times.begin(), event_times.end());
    event_times.erase(std::unique(event_times.begin(), event_times.end()),
                      event_times.end());

    double chi_square = 0.0;
    for (const double time : event_times) {
        std::map<std::string, std::size_t> at_risk;
        for (const GrayRow& row : rows) {
            if (row.time >= time) {
                ++at_risk[row.group];
            }
        }
        std::size_t total_at_risk = 0;
        for (const auto& entry : at_risk) {
            total_at_risk += entry.second;
        }
        if (total_at_risk < 2) {
            continue;
        }

        std::map<std::string, std::size_t> failures_at_time;
        for (const GrayRow& row : rows) {
            if (row.time == time && row.is_failure) {
                ++failures_at_time[row.mode];
            }
        }
        for (const auto& mode_entry : failures_at_time) {
            const std::string& mode = mode_entry.first;
            const std::size_t total_failures = mode_entry.second;
            if (total_failures == 0) {
                continue;
            }
            for (const std::string& group : groups) {
                std::size_t observed = 0;
                for (const GrayRow& row : rows) {
                    if (row.time == time && row.is_failure && row.group == group
                        && row.mode == mode) {
                        ++observed;
                    }
                }
                const std::size_t group_at_risk = at_risk[group];
                const double expected = static_cast<double>(total_failures)
                    * static_cast<double>(group_at_risk)
                    / static_cast<double>(total_at_risk);
                if (expected > 1.0e-12) {
                    const double residual = static_cast<double>(observed) - expected;
                    chi_square += residual * residual / expected;
                }
            }
        }
    }

    if (!(chi_square >= 0.0) || !std::isfinite(chi_square)) {
        result.not_computed_reason = "invalid_statistic";
        result.diagnostics.push_back(warning_msg(
            "gray_invalid_statistic",
            "Gray 检验统计量无法计算。"));
        return result;
    }

    const double df = static_cast<double>((groups.size() - 1) * modes.size());
    result.chi_square = chi_square;
    result.df = df;
    result.p_value = chi_square_right_tail(chi_square, df);
    return result;
}

}  // namespace datalab::domain::statistics
