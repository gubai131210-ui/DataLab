#include "domain/statistics/censoring_contract.h"

#include "domain/statistics/reliability.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>
#include <string>

namespace datalab::domain::statistics {
namespace {

std::string lower_copy(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

DiagnosticMessage error_msg(const std::string& code, const std::string& message)
{
    return {DiagnosticMessage::Severity::error, code, message};
}

DiagnosticMessage warning_msg(const std::string& code, const std::string& message)
{
    return {DiagnosticMessage::Severity::warning, code, message};
}

DiagnosticMessage info_msg(const std::string& code, const std::string& message)
{
    return {DiagnosticMessage::Severity::info, code, message};
}

}  // namespace

std::optional<CensoringType> parse_censoring_type(const std::string& text)
{
    const std::string value = lower_copy(text);
    if (value == "exact" || value == "failure" || value == "event" || value == "1"
        || value == "fail") {
        return CensoringType::exact;
    }
    if (value == "right" || value == "right-censored" || value == "right_censored"
        || value == "censored" || value == "0" || value == "suspension") {
        return CensoringType::right;
    }
    if (value == "left" || value == "left-censored" || value == "left_censored") {
        return CensoringType::left;
    }
    if (value == "interval" || value == "interval-censored"
        || value == "interval_censored") {
        return CensoringType::interval;
    }
    return std::nullopt;
}

std::string censoring_type_id(CensoringType type)
{
    switch (type) {
    case CensoringType::exact:
        return "exact";
    case CensoringType::right:
        return "right";
    case CensoringType::left:
        return "left";
    case CensoringType::interval:
        return "interval";
    }
    return "exact";
}

namespace {

std::optional<std::size_t> find_column_index(
    const DataTable& table, const std::vector<std::string>& aliases)
{
    for (std::size_t index = 0; index < table.columns.size(); ++index) {
        const std::string name = lower_copy(table.columns[index]);
        for (const std::string& alias : aliases) {
            if (name == alias) {
                return index;
            }
        }
    }
    return std::nullopt;
}

std::string cell_at(const DataTable& table, std::size_t row, std::size_t column)
{
    if (row >= table.rows.size() || column >= table.rows[row].size()) {
        return {};
    }
    return table.rows[row][column];
}

bool parse_optional_double(const std::string& text, std::optional<double>* out)
{
    if (text.empty() || text == "—" || text == "-") {
        *out = std::nullopt;
        return true;
    }
    char* end = nullptr;
    const double value = std::strtod(text.c_str(), &end);
    if (end == text.c_str() || !std::isfinite(value)) {
        return false;
    }
    *out = value;
    return true;
}

}  // namespace

DataTable censoring_observations_to_worksheet(
    const std::vector<CensoringObservation>& observations)
{
    DataTable table;
    table.name = "censoring_observations";
    table.columns = {
        "source_row",
        "time",
        "censoring_type",
        "event",
        "failure_mode",
        "group",
        "interval_left",
        "interval_right",
        "exposure",
        "time_unit"};
    table.rows.reserve(observations.size());
    for (const auto& observation : observations) {
        std::vector<std::string> row;
        row.push_back(std::to_string(observation.source_row + 1));
        row.push_back(std::to_string(observation.time));
        row.push_back(censoring_type_id(observation.type));
        row.push_back(observation.type == CensoringType::exact ? "1" : "0");
        row.push_back(observation.failure_mode);
        row.push_back(observation.group);
        if (observation.type == CensoringType::interval) {
            row.push_back(std::to_string(observation.interval_left));
            row.push_back(std::to_string(observation.interval_right));
        } else {
            row.push_back("");
            row.push_back("");
        }
        if (observation.exposure.has_value()) {
            row.push_back(std::to_string(*observation.exposure));
        } else {
            row.push_back("");
        }
        row.push_back(observation.time_unit);
        table.rows.push_back(std::move(row));
    }
    return table;
}

CensoringWorksheetImportResult censoring_observations_from_worksheet(
    const DataTable& table)
{
    CensoringWorksheetImportResult result;
    if (table.rows.empty()) {
        result.diagnostics.push_back(error_msg(
            "censoring_worksheet_empty", "删失工作表为空。"));
        return result;
    }
    const auto type_col = find_column_index(
        table, {"censoring_type", "censor_type", "censoring", "type"});
    const auto time_col = find_column_index(table, {"time", "t", "lifetime"});
    if (!type_col.has_value() || !time_col.has_value()) {
        result.diagnostics.push_back(error_msg(
            "censoring_worksheet_missing_columns",
            "删失工作表需要 censoring_type 与 time 列。"));
        return result;
    }
    const auto source_col = find_column_index(
        table, {"source_row", "source_rows", "row", "row_id"});
    const auto mode_col = find_column_index(
        table, {"failure_mode", "mode", "failuremode"});
    const auto group_col = find_column_index(table, {"group", "stratum"});
    const auto left_col = find_column_index(
        table, {"interval_left", "left", "t_left"});
    const auto right_col = find_column_index(
        table, {"interval_right", "right", "t_right"});
    const auto exposure_col = find_column_index(table, {"exposure"});
    const auto unit_col = find_column_index(table, {"time_unit", "unit"});

    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        const std::string type_text = cell_at(table, row, *type_col);
        const auto typed = parse_censoring_type(type_text);
        if (!typed.has_value()) {
            result.diagnostics.push_back(error_msg(
                "censoring_worksheet_invalid_type",
                "第 " + std::to_string(row + 1) + " 行 censoring_type 无法解析。"));
            return result;
        }
        std::optional<double> time_value;
        if (!parse_optional_double(cell_at(table, row, *time_col), &time_value)
            || !time_value.has_value()) {
            result.diagnostics.push_back(error_msg(
                "censoring_worksheet_invalid_time",
                "第 " + std::to_string(row + 1) + " 行 time 不是有限数。"));
            return result;
        }
        CensoringObservation observation;
        observation.type = *typed;
        observation.time = *time_value;
        if (source_col.has_value()) {
            std::optional<double> source_value;
            if (parse_optional_double(cell_at(table, row, *source_col), &source_value)
                && source_value.has_value() && *source_value >= 1.0) {
                observation.source_row =
                    static_cast<std::size_t>(*source_value) - 1;
            } else {
                observation.source_row = row;
            }
        } else {
            observation.source_row = row;
        }
        if (mode_col.has_value()) {
            observation.failure_mode = cell_at(table, row, *mode_col);
        }
        if (group_col.has_value()) {
            observation.group = cell_at(table, row, *group_col);
        }
        if (unit_col.has_value()) {
            observation.time_unit = cell_at(table, row, *unit_col);
        }
        if (observation.type == CensoringType::interval) {
            if (!left_col.has_value() || !right_col.has_value()) {
                result.diagnostics.push_back(error_msg(
                    "censoring_worksheet_missing_interval_bounds",
                    "interval 行需要 interval_left / interval_right 列。"));
                return result;
            }
            std::optional<double> left;
            std::optional<double> right;
            if (!parse_optional_double(cell_at(table, row, *left_col), &left)
                || !left.has_value()
                || !parse_optional_double(cell_at(table, row, *right_col), &right)
                || !right.has_value()) {
                result.diagnostics.push_back(error_msg(
                    "censoring_worksheet_invalid_interval_bounds",
                    "第 " + std::to_string(row + 1)
                        + " 行区间界缺失或非法（不静默补齐）。"));
                return result;
            }
            observation.interval_left = *left;
            observation.interval_right = *right;
        }
        if (exposure_col.has_value()) {
            std::optional<double> exposure;
            if (!parse_optional_double(cell_at(table, row, *exposure_col), &exposure)) {
                result.diagnostics.push_back(error_msg(
                    "censoring_worksheet_invalid_exposure",
                    "第 " + std::to_string(row + 1) + " 行 exposure 非法。"));
                return result;
            }
            observation.exposure = exposure;
        }
        result.observations.push_back(std::move(observation));
    }
    result.diagnostics.push_back(info_msg(
        "censoring_worksheet_import_ok",
        "已从工作表导入 " + std::to_string(result.observations.size())
            + " 条删失观测；证据类型仍为 formula_reference，不是 vendor_oracle。"));
    result.ok = true;
    return result;
}

CensoringContractResult validate_censoring_contract(
    const std::vector<CensoringObservation>& observations,
    bool allow_left_interval_for_km)
{
    CensoringContractResult result;
    if (observations.empty()) {
        result.diagnostics.push_back(error_msg(
            "censoring_empty", "删失契约至少需要一条观测。"));
        return result;
    }

    std::string unit;
    bool unit_conflict = false;
    for (const CensoringObservation& observation : observations) {
        if (!observation.time_unit.empty()) {
            if (unit.empty()) {
                unit = observation.time_unit;
            } else if (unit != observation.time_unit) {
                unit_conflict = true;
            }
        }

        switch (observation.type) {
        case CensoringType::exact:
        case CensoringType::right:
        case CensoringType::left: {
            if (!std::isfinite(observation.time) || observation.time < 0.0) {
                result.diagnostics.push_back(error_msg(
                    "censoring_negative_or_nonfinite_time",
                    "时间必须为有限非负数；负时间阻止分析。"));
                return result;
            }
            if (observation.time == 0.0
                && observation.type == CensoringType::exact) {
                result.diagnostics.push_back(warning_msg(
                    "censoring_zero_failure_time",
                    "出现时间为 0 的失效；请确认单位与记录口径。"));
            }
            break;
        }
        case CensoringType::interval: {
            if (!std::isfinite(observation.interval_left)
                || !std::isfinite(observation.interval_right)
                || observation.interval_left < 0.0
                || observation.interval_right < 0.0) {
                result.diagnostics.push_back(error_msg(
                    "censoring_interval_nonfinite",
                    "区间删失边界必须为有限非负数。"));
                return result;
            }
            if (!(observation.interval_left < observation.interval_right)) {
                result.diagnostics.push_back(error_msg(
                    "censoring_interval_reversed",
                    "区间删失要求左端严格小于右端；反向区间阻止。"));
                return result;
            }
            break;
        }
        }

        if (observation.exposure.has_value()) {
            if (!std::isfinite(*observation.exposure) || *observation.exposure < 0.0) {
                result.diagnostics.push_back(error_msg(
                    "censoring_invalid_exposure", "暴露量必须为有限非负数。"));
                return result;
            }
        }

        ++result.valid_count;
        switch (observation.type) {
        case CensoringType::exact:
            ++result.exact_count;
            ++result.failure_count;
            break;
        case CensoringType::right:
            ++result.right_censored_count;
            break;
        case CensoringType::left:
            ++result.left_censored_count;
            break;
        case CensoringType::interval:
            ++result.interval_censored_count;
            break;
        }

        const bool km_compatible =
            observation.type == CensoringType::exact
            || observation.type == CensoringType::right
            || (allow_left_interval_for_km
                && (observation.type == CensoringType::left
                    || observation.type == CensoringType::interval));
        if (km_compatible
            && (observation.type == CensoringType::exact
                || observation.type == CensoringType::right)) {
            // Classic product-limit path: right-censored must NOT be treated as failures.
            result.times_for_right_censored_km.push_back(observation.time);
            result.events_for_right_censored_km.push_back(
                observation.type == CensoringType::exact);
            result.source_rows_for_km.push_back(observation.source_row);
        }
    }

    if (unit_conflict) {
        result.diagnostics.push_back(error_msg(
            "censoring_time_unit_conflict", "观测时间单位不一致，阻止合并分析。"));
        return result;
    }
    result.time_unit = unit;

    if (!allow_left_interval_for_km
        && (result.left_censored_count > 0 || result.interval_censored_count > 0)) {
        result.diagnostics.push_back(error_msg(
            "censoring_left_interval_not_for_classic_km",
            "经典 Kaplan–Meier 路径不接受左删失/区间删失；请改用区间删失 KM，"
            "或先过滤为 exact/right。"));
        return result;
    }

    if (result.failure_count == 0) {
        result.diagnostics.push_back(warning_msg(
            "censoring_zero_failures",
            "零失效（全删失或无 exact 事件）；生存/参数估计可能不可识别。"));
    }
    if (result.failure_count == result.valid_count && result.valid_count > 0
        && result.right_censored_count == 0 && result.left_censored_count == 0
        && result.interval_censored_count == 0) {
        result.diagnostics.push_back(info_msg(
            "censoring_all_failures", "全部为失效事件，无删失。"));
    }
    if (result.failure_count == 0 && result.valid_count > 0) {
        result.diagnostics.push_back(warning_msg(
            "censoring_all_censored", "全部删失；中位寿命等点估计通常不可得。"));
    }

    result.ok = true;
    result.diagnostics.push_back(info_msg(
        "censoring_formula_reference",
        "删失契约证据类型 formula_reference；右删失不得当作失效。"));
    return result;
}

WarrantySummaryResult summarize_warranty(const WarrantySummaryOptions& options)
{
    WarrantySummaryResult result;
    result.warranty_time = options.warranty_time;
    result.time_unit = options.time_unit;
    result.exposure = options.exposure;
    result.reliability_at_warranty = options.reliability_at_warranty;
    result.observed_failures = options.observed_failures;
    result.censored_count = options.censored_count;
    result.valid_count = options.valid_count;
    result.model_name = options.model_name;
    result.quantity_label =
        options.reliability_is_prediction ? "prediction" : "observation";
    result.evidence_type = "formula_reference";

    if (!std::isfinite(options.warranty_time) || options.warranty_time <= 0.0) {
        result.diagnostics.push_back(error_msg(
            "warranty_invalid_window", "保修窗口 T_w 必须为正有限数。"));
        return result;
    }
    if (options.time_unit.empty()) {
        result.diagnostics.push_back(error_msg(
            "warranty_missing_time_unit", "保修摘要需要明确时间单位。"));
        return result;
    }
    if (!std::isfinite(options.exposure) || options.exposure <= 0.0) {
        result.diagnostics.push_back(error_msg(
            "warranty_zero_exposure", "暴露量必须为正有限数。"));
        return result;
    }
    if (!std::isfinite(options.reliability_at_warranty)
        || options.reliability_at_warranty < 0.0
        || options.reliability_at_warranty > 1.0) {
        result.diagnostics.push_back(error_msg(
            "warranty_invalid_reliability", "R(T_w) 必须落在 [0,1]。"));
        return result;
    }
    if (options.valid_count > 0 && options.observed_failures == 0
        && options.censored_count == options.valid_count) {
        result.diagnostics.push_back(warning_msg(
            "warranty_all_censored_input",
            "输入为全删失；预测摘要仍可计算，但不得宣称为观察失效率。"));
    }

    result.failure_probability = 1.0 - options.reliability_at_warranty;
    result.expected_failures = options.exposure * result.failure_probability;
    result.claims_per_1000 = 1000.0 * result.failure_probability;
    result.ok = true;
    result.diagnostics.push_back(info_msg(
        "warranty_formula_reference",
        "claims/1000 = 1000*(1-R(T_w))；证据类型 formula_reference，非 vendor_oracle。"));
    if (options.reliability_is_prediction) {
        result.diagnostics.push_back(info_msg(
            "warranty_prediction_label",
            "当前摘要标记为 prediction，不得与观察失效计数混读为同一口径。"));
    }
    return result;
}

WarrantyStrataSummaryResult summarize_warranty_strata(
    const WarrantySummaryOptions& overall_options,
    const std::vector<WarrantyStratumInput>& strata)
{
    WarrantyStrataSummaryResult result;
    result.overall = summarize_warranty(overall_options);
    result.uses_pooled_reliability = true;
    result.evidence_type = "formula_reference";
    if (!result.overall.ok) {
        // Surface overall errors without duplicating when caller already ran summarize_warranty.
        result.diagnostics = result.overall.diagnostics;
        return result;
    }
    if (strata.empty()) {
        result.diagnostics.push_back(warning_msg(
            "warranty_strata_empty",
            "未提供分层输入；仅输出总体保修摘要。"));
        result.ok = true;
        return result;
    }

    double measured_exposure_sum = 0.0;
    std::size_t labeled_valid = 0;
    for (const auto& stratum : strata) {
        if (stratum.kind.empty()) {
            result.diagnostics.push_back(error_msg(
                "warranty_stratum_kind_missing",
                "分层 kind 必须为 failure_mode 或 group。"));
            return result;
        }
        if (result.stratum_kind.empty()) {
            result.stratum_kind = stratum.kind;
        } else if (result.stratum_kind != stratum.kind) {
            result.diagnostics.push_back(error_msg(
                "warranty_stratum_kind_mixed",
                "同一次分层摘要不得混用 failure_mode 与 group。"));
            return result;
        }
        if (!(stratum.exposure >= 0.0) || !std::isfinite(stratum.exposure)) {
            result.diagnostics.push_back(error_msg(
                "warranty_stratum_invalid_exposure",
                "分层暴露量必须为有限非负数。"));
            return result;
        }
        measured_exposure_sum += stratum.exposure;
        labeled_valid += stratum.valid_count;
    }

    const bool use_proportional =
        measured_exposure_sum <= 0.0 && result.overall.exposure > 0.0;
    if (use_proportional) {
        if (labeled_valid == 0) {
            result.diagnostics.push_back(error_msg(
                "warranty_stratum_no_denominator",
                "分层无测量暴露量且 valid_count 全为 0，无法追溯分母。"));
            return result;
        }
        result.diagnostics.push_back(warning_msg(
            "warranty_stratum_exposure_proportional",
            "分层暴露量按 valid_count 比例分摊标量总暴露量；"
            "这不是实测分母，报告须标注 exposure_attribution=proportional_scalar。"));
    } else if (measured_exposure_sum > 0.0
               && std::fabs(measured_exposure_sum - result.overall.exposure) > 1.0e-9
                   * std::max(1.0, result.overall.exposure)) {
        result.diagnostics.push_back(warning_msg(
            "warranty_stratum_exposure_sum_mismatch",
            "分层暴露量之和与总体暴露量不一致；分层 expected_failures 使用各层自身暴露量，"
            "不得静默重标度到总体。"));
    }

    const double f_pooled = result.overall.failure_probability;
    bool any_mode_specific = false;
    bool any_pooled = false;
    for (const auto& stratum : strata) {
        WarrantyStratumResult out;
        out.label = stratum.label.empty() ? "(unlabeled)" : stratum.label;
        out.kind = stratum.kind;
        out.observed_failures = stratum.observed_failures;
        out.censored_count = stratum.censored_count;
        out.valid_count = stratum.valid_count;
        out.source_rows = stratum.source_rows;
        if (use_proportional) {
            const double share = static_cast<double>(stratum.valid_count)
                / static_cast<double>(labeled_valid);
            out.exposure = result.overall.exposure * share;
            out.share_of_total_exposure = share;
            out.exposure_attribution = "proportional_scalar";
        } else {
            out.exposure = stratum.exposure;
            out.share_of_total_exposure = result.overall.exposure > 0.0
                ? stratum.exposure / result.overall.exposure
                : 0.0;
            out.exposure_attribution =
                stratum.exposure > 0.0 ? "measured_column" : "zero";
        }
        double f = f_pooled;
        if (stratum.reliability_at_warranty.has_value()
            && stratum.kind == "failure_mode") {
            const double r = *stratum.reliability_at_warranty;
            if (std::isfinite(r) && r >= 0.0 && r <= 1.0) {
                f = 1.0 - r;
                out.reliability_at_warranty = r;
                out.uses_mode_specific_reliability = true;
                any_mode_specific = true;
            } else {
                result.diagnostics.push_back(warning_msg(
                    "warranty_stratum_invalid_mode_reliability",
                    "分层 " + out.label
                        + " 提供的分模式 R(T_w) 非法，已回退到池化 R。"));
                any_pooled = true;
            }
        } else {
            any_pooled = true;
        }
        out.expected_failures = out.exposure * f;
        if (!out.reliability_at_warranty.has_value()) {
            out.reliability_at_warranty = result.overall.reliability_at_warranty;
        }
        result.strata.push_back(std::move(out));
    }

    result.uses_mode_specific_reliability = any_mode_specific;
    result.uses_pooled_reliability = any_pooled || !any_mode_specific;
    if (any_mode_specific && any_pooled) {
        result.diagnostics.push_back(info_msg(
            "warranty_strata_mixed_reliability",
            "部分 failure_mode 层使用 cause-specific R(T_w)，其余仍用池化 R；"
            "证据类型 formula_reference，不是 vendor_oracle。"));
    } else if (any_mode_specific) {
        result.diagnostics.push_back(info_msg(
            "warranty_strata_mode_specific_reliability",
            "分层 expected_failures = 层暴露量 * F_mode(T_w)，"
            "F_mode 来自 cause-specific 分模式拟合（竞争失效作右删失）；"
            "formula_reference，不是 vendor_oracle。"));
    } else {
        result.diagnostics.push_back(info_msg(
            "warranty_strata_pooled_reliability",
            "分层 expected_failures = 层暴露量 * F(T_w)，F 来自总体池化 R(T_w)；"
            "未使用分模式可靠度，不是 vendor_oracle。"));
    }
    result.ok = true;
    return result;
}

namespace {

std::optional<double> km_reliability_at_time(
    const KaplanMeierResult& km, double time)
{
    if (!(time >= 0.0) || !std::isfinite(time) || km.points.empty()) {
        return std::nullopt;
    }
    double survival = 1.0;
    for (const auto& point : km.points) {
        if (point.time <= time) {
            survival = point.survival;
        } else {
            break;
        }
    }
    if (!std::isfinite(survival) || survival < 0.0 || survival > 1.0) {
        return std::nullopt;
    }
    return survival;
}

std::optional<double> parametric_reliability_at_time(
    const std::string& model,
    double time,
    const ReliabilityModeFitResult& fit)
{
    if (!(time >= 0.0) || !std::isfinite(time) || !fit.identifiable) {
        return std::nullopt;
    }
    if (model == "weibull" || model == "weibull2") {
        if (!fit.shape.has_value() || !fit.scale.has_value()) {
            return std::nullopt;
        }
        const double f = cdf_weibull3(time, *fit.shape, *fit.scale, 0.0);
        if (!std::isfinite(f)) {
            return std::nullopt;
        }
        return 1.0 - f;
    }
    if (model == "exponential" || model == "exponential1") {
        if (!fit.rate.has_value()) {
            return std::nullopt;
        }
        const double f = cdf_exponential2(time, *fit.rate, 0.0);
        if (!std::isfinite(f)) {
            return std::nullopt;
        }
        return 1.0 - f;
    }
    if (model == "lognormal" || model == "lognormal2") {
        if (!fit.location.has_value() || !fit.scale.has_value()) {
            return std::nullopt;
        }
        const double f = cdf_lognormal3(time, *fit.location, *fit.scale, 0.0);
        if (!std::isfinite(f)) {
            return std::nullopt;
        }
        return 1.0 - f;
    }
    return std::nullopt;
}

}  // namespace

ReliabilityModeFitsResult fit_reliability_by_failure_mode(
    const std::vector<CensoringObservation>& observations,
    const std::string& model,
    double warranty_time)
{
    ReliabilityModeFitsResult result;
    result.model = model;
    result.warranty_time = warranty_time;
    result.fitting_scheme = "cause_specific";

    std::string normalized = model;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    if (normalized == "weibull3" || normalized == "lognormal3"
        || normalized == "exponential2") {
        result.diagnostics.push_back(warning_msg(
            "mode_fit_threshold_model_unsupported",
            "分模式拟合当前仅支持二参数 Weibull / Lognormal / Exponential / KM；"
            "阈值模型请用总体拟合。"));
        return result;
    }
    const bool use_km = normalized == "kaplan_meier" || normalized == "km"
        || normalized.empty();
    const bool use_weibull = normalized == "weibull" || normalized == "weibull2";
    const bool use_lognormal = normalized == "lognormal" || normalized == "lognormal2";
    const bool use_exponential =
        normalized == "exponential" || normalized == "exponential1";
    if (!use_km && !use_weibull && !use_lognormal && !use_exponential) {
        result.diagnostics.push_back(warning_msg(
            "mode_fit_model_unsupported",
            "分模式拟合不支持模型 '" + model + "'。"));
        return result;
    }

    std::set<std::string> modes;
    std::size_t unlabeled_exact = 0;
    std::size_t skipped_left_interval = 0;
    for (const auto& observation : observations) {
        if (observation.type == CensoringType::left
            || observation.type == CensoringType::interval) {
            ++skipped_left_interval;
            continue;
        }
        if (observation.type == CensoringType::exact) {
            if (observation.failure_mode.empty()) {
                ++unlabeled_exact;
            } else {
                modes.insert(observation.failure_mode);
            }
        }
    }
    if (modes.empty()) {
        result.diagnostics.push_back(info_msg(
            "mode_fit_no_labeled_failures",
            "无带 failure_mode 标签的 exact 失效，跳过分模式拟合。"));
        return result;
    }
    if (unlabeled_exact > 0) {
        result.diagnostics.push_back(warning_msg(
            "mode_fit_unlabeled_exact_excluded",
            "有 " + std::to_string(unlabeled_exact)
                + " 条 exact 失效缺少 failure_mode，已从分模式拟合排除"
                  "（不作任一模式的竞争删失）。"));
    }
    if (skipped_left_interval > 0) {
        result.diagnostics.push_back(info_msg(
            "mode_fit_left_interval_omitted",
            "分模式拟合省略 left/interval 行 "
                + std::to_string(skipped_left_interval)
                + " 条；请用 km_interval / 总体路径处理。"));
    }
    result.diagnostics.push_back(info_msg(
        "mode_fit_cause_specific_scope",
        "分模式可靠度 = cause-specific：目标模式 exact 为失效，"
        "其他已标注模式的 exact 作为右删失，原始 right 仍为右删失；"
        "evidence_type=formula_reference，algorithm_id="
        "cause_specific_right_censored_competing；不是 vendor_oracle。"));

    result.ran = true;
    for (const std::string& mode : modes) {
        ReliabilityModeFitResult fit;
        fit.failure_mode = mode;
        fit.evidence_type = "formula_reference";
        fit.algorithm_id = "cause_specific_right_censored_competing";

        std::vector<double> times;
        std::vector<bool> events;
        for (const auto& observation : observations) {
            if (observation.type == CensoringType::left
                || observation.type == CensoringType::interval) {
                continue;
            }
            if (observation.type == CensoringType::exact
                && observation.failure_mode.empty()) {
                continue;
            }
            if (!std::isfinite(observation.time) || observation.time < 0.0) {
                continue;
            }
            times.push_back(observation.time);
            fit.source_rows.push_back(observation.source_row);
            if (observation.type == CensoringType::exact
                && observation.failure_mode == mode) {
                events.push_back(true);
                ++fit.failure_count;
            } else if (observation.type == CensoringType::exact) {
                events.push_back(false);
                ++fit.competing_failure_count;
            } else {
                events.push_back(false);
                ++fit.right_censored_count;
            }
        }
        fit.valid_count = times.size();
        if (fit.failure_count == 0) {
            fit.not_computed_reason = "no_mode_failures";
            fit.diagnostics.push_back(warning_msg(
                "mode_fit_no_failures",
                "模式 " + mode + " 无 exact 失效，无法拟合。"));
            result.modes.push_back(std::move(fit));
            continue;
        }

        if (use_km) {
            const auto km = kaplan_meier(times, events, 0.95, fit.source_rows);
            fit.identifiable = km.survival_identifiable;
            fit.converged = km.survival_identifiable;
            fit.median_life = km.median_life;
            fit.not_computed_reason = km.not_computed_reason;
            fit.reliability_at_warranty = km_reliability_at_time(km, warranty_time);
            fit.diagnostics.insert(
                fit.diagnostics.end(), km.diagnostics.begin(), km.diagnostics.end());
        } else if (use_weibull) {
            const auto fitted = fit_weibull(times, events);
            fit.identifiable = fitted.identifiable;
            fit.converged = fitted.converged;
            if (fitted.identifiable) {
                fit.shape = fitted.shape;
                fit.scale = fitted.scale;
            }
            fit.median_life = fitted.median_life;
            fit.not_computed_reason = fitted.not_computed_reason;
            fit.diagnostics.insert(
                fit.diagnostics.end(),
                fitted.diagnostics.begin(),
                fitted.diagnostics.end());
            ReliabilityModeFitResult probe = fit;
            fit.reliability_at_warranty =
                parametric_reliability_at_time("weibull", warranty_time, probe);
        } else if (use_lognormal) {
            const auto fitted = fit_lognormal(times, events);
            fit.identifiable = fitted.identifiable;
            fit.converged = fitted.converged;
            if (fitted.identifiable) {
                fit.location = fitted.location;
                fit.scale = fitted.scale;
            }
            fit.median_life = fitted.median_life;
            fit.not_computed_reason = fitted.not_computed_reason;
            fit.diagnostics.insert(
                fit.diagnostics.end(),
                fitted.diagnostics.begin(),
                fitted.diagnostics.end());
            ReliabilityModeFitResult probe = fit;
            fit.reliability_at_warranty =
                parametric_reliability_at_time("lognormal", warranty_time, probe);
        } else if (use_exponential) {
            const auto fitted = fit_exponential(times, events);
            fit.identifiable = fitted.identifiable;
            fit.converged = fitted.converged;
            if (fitted.identifiable) {
                fit.rate = fitted.rate;
            }
            fit.median_life = fitted.b50;
            fit.not_computed_reason = fitted.not_computed_reason;
            fit.diagnostics.insert(
                fit.diagnostics.end(),
                fitted.diagnostics.begin(),
                fitted.diagnostics.end());
            ReliabilityModeFitResult probe = fit;
            fit.reliability_at_warranty =
                parametric_reliability_at_time("exponential", warranty_time, probe);
        }

        if (warranty_time > 0.0 && !fit.reliability_at_warranty.has_value()
            && fit.identifiable) {
            fit.diagnostics.push_back(warning_msg(
                "mode_fit_reliability_unavailable",
                "模式 " + mode + " 已拟合但未能在 T_w 计算 R。"));
        }
        result.modes.push_back(std::move(fit));
    }
    return result;
}

}  // namespace datalab::domain::statistics
