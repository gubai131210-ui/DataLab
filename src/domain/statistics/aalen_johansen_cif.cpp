#include "domain/statistics/aalen_johansen_cif.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

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

}  // namespace

AalenJohansenCifResult aalen_johansen_cif(
    const std::vector<CensoringObservation>& observations,
    const double warranty_time)
{
    AalenJohansenCifResult result;
    result.warranty_time = warranty_time;
    result.evidence_type = "formula_reference";
    result.algorithm_id = "aalen_johansen_cif";

    std::set<std::string> modes;
    std::size_t unlabeled_exact = 0;
    std::size_t skipped_left_interval = 0;
    struct Row {
        double time = 0.0;
        bool is_failure = false;
        std::string mode;
    };
    std::vector<Row> rows;
    rows.reserve(observations.size());

    for (const auto& observation : observations) {
        if (observation.type == CensoringType::left
            || observation.type == CensoringType::interval) {
            ++skipped_left_interval;
            continue;
        }
        if (!(observation.time >= 0.0) || !std::isfinite(observation.time)) {
            continue;
        }
        if (observation.type == CensoringType::right) {
            rows.push_back({observation.time, false, {}});
            continue;
        }
        if (observation.failure_mode.empty()) {
            ++unlabeled_exact;
            continue;
        }
        modes.insert(observation.failure_mode);
        rows.push_back({observation.time, true, observation.failure_mode});
    }

    if (modes.empty()) {
        result.diagnostics.push_back(info_msg(
            "cif_no_labeled_failures",
            "无带 failure_mode 标签的 exact 失效，跳过 Aalen–Johansen CIF。"));
        return result;
    }
    if (unlabeled_exact > 0) {
        result.diagnostics.push_back(warning_msg(
            "cif_unlabeled_exact_excluded",
            "有 " + std::to_string(unlabeled_exact)
                + " 条 exact 失效缺少 failure_mode，已从 CIF 排除。"));
    }
    if (skipped_left_interval > 0) {
        result.diagnostics.push_back(info_msg(
            "cif_left_interval_omitted",
            "CIF 省略 left/interval 行 " + std::to_string(skipped_left_interval)
                + " 条；请用 km_interval 路径处理。"));
    }
    result.diagnostics.push_back(info_msg(
        "cif_aalen_johansen_scope",
        "累计发生函数 CIF = Aalen–Johansen（formula_reference / aalen_johansen_cif）："
        "总体生存把任一标注失效当作事件；CIF_k 为原因 k 的累计发生概率。"
        "不是 Fine-Gray 多协变量回归，不是 cause-specific（竞争删失）可靠度，不是 vendor_oracle。"
        "二分类 group 的 Fine-Gray 另有门禁路径。"));
    result.diagnostics.push_back(warning_msg(
        "cif_not_fine_gray_multivar",
        "Aalen–Johansen CIF 本身不是 Fine-Gray 回归；不得把 CIF 表写成 Fine-Gray 或商业软件对齐。"));

    result.ran = true;
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        return a.time < b.time;
    });

    std::map<std::string, std::size_t> mode_index;
    for (const std::string& mode : modes) {
        mode_index[mode] = result.modes.size();
        AalenJohansenCifModeResult curve;
        curve.failure_mode = mode;
        result.modes.push_back(std::move(curve));
    }
    for (const auto& row : rows) {
        if (row.is_failure) {
            ++result.modes[mode_index[row.mode]].failure_count;
        }
    }

    double survival = 1.0;
    std::vector<double> cif(result.modes.size(), 0.0);
    std::size_t index = 0;
    const std::size_t n = rows.size();
    while (index < n) {
        const double time = rows[index].time;
        std::size_t end = index;
        while (end < n && rows[end].time == time) {
            ++end;
        }
        const std::size_t at_risk = n - index;
        std::vector<std::size_t> cause_failures(result.modes.size(), 0);
        std::size_t all_failures = 0;
        for (std::size_t i = index; i < end; ++i) {
            if (!rows[i].is_failure) {
                continue;
            }
            ++all_failures;
            ++cause_failures[mode_index[rows[i].mode]];
        }

        if (all_failures > 0 && at_risk > 0) {
            const double s_prev = survival;
            for (std::size_t m = 0; m < result.modes.size(); ++m) {
                if (cause_failures[m] > 0) {
                    cif[m] += s_prev
                        * (static_cast<double>(cause_failures[m])
                           / static_cast<double>(at_risk));
                }
            }
            survival = s_prev
                * (1.0 - static_cast<double>(all_failures) / static_cast<double>(at_risk));
            if (survival < 0.0) {
                survival = 0.0;
            }
            for (std::size_t m = 0; m < result.modes.size(); ++m) {
                AalenJohansenCifPoint point;
                point.time = time;
                point.at_risk = at_risk;
                point.cause_failures = cause_failures[m];
                point.all_failures = all_failures;
                point.cif = cif[m];
                point.overall_survival = survival;
                result.modes[m].points.push_back(point);
                result.modes[m].cif_at_last_event = point.cif;
                if (warranty_time > 0.0 && time <= warranty_time) {
                    result.modes[m].cif_at_warranty = point.cif;
                }
            }
        }
        index = end;
    }

    return result;
}

}  // namespace datalab::domain::statistics
