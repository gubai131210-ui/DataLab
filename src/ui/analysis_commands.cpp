#include "ui/analysis_commands.h"

#include "application/analysis_service.h"
#include "application/graph_service.h"
#include "domain/statistics/control_charts.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QString>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace analysis_commands {

namespace {

using datalab::application::AnalysisService;
using datalab::application::GraphService;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::OutputPage;

using ApplyFn = std::function<AnalysisApplyResult(
    AnalysisConfiguration&, const datalab::application::AnalysisIntent&)>;
using RunFn = std::function<OutputPage(
    const DataTable&, const AnalysisConfiguration&)>;

std::string normalize(std::string value)
{
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(),
                             [](unsigned char character) {
                                 return !std::isspace(character);
                             }));
    value.erase(std::find_if(value.rbegin(), value.rend(),
                             [](unsigned char character) {
                                 return !std::isspace(character);
                             }).base(),
                value.end());
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
    return value;
}

// 校验失败：弹出提示后中止（对应原 run_* 的 QMessageBox::information）。
AnalysisApplyResult apply_error(
    const QString& title,
    const QString& message,
    const QString& field_id = {})
{
    QString effective_field = field_id;
    if (effective_field.isEmpty()) {
        if (title.contains(QStringLiteral("变量"))
            || message.contains(QStringLiteral("至少需要选择"))
            || message.contains(QStringLiteral("请选择测量值"))
            || message.contains(QStringLiteral("变量不足"))) {
            effective_field = QStringLiteral("variables");
        } else if (message.contains(QStringLiteral("响应"))) {
            effective_field = QStringLiteral("response");
        } else if (message.contains(QStringLiteral("因子"))) {
            effective_field = QStringLiteral("factor_columns");
        } else if (message.contains(QStringLiteral("LSL"))
                   || message.contains(QStringLiteral("USL"))) {
            effective_field = QStringLiteral("lsl");
        }
    }
    return {false, title, message, effective_field};
}

// 校验失败：静默中止（对应原 run_* 中无提示的裸 return）。
AnalysisApplyResult apply_silent()
{
    return {false, {}, {}};
}

void apply_special_cause_selection(
    AnalysisConfiguration& configuration,
    const datalab::application::AnalysisIntent& intent)
{
    const std::string policy_text = normalize(intent.line_text("rule_policy"));
    if (policy_text == "minitab_like" || policy_text == "default_minitab_like") {
        configuration.control.special_cause_rule_policy = "minitab_like";
    } else if (policy_text == "all_applicable" || policy_text == "default_all_applicable"
               || policy_text.empty()) {
        configuration.control.special_cause_rule_policy = "all_applicable";
    } else {
        configuration.control.special_cause_rule_policy = policy_text;
    }

    const std::string text = intent.line_text("tests");
    if (text.empty()) {
        configuration.control.enabled_special_cause_tests.clear();
        return;
    }
    configuration.control.special_cause_rule_policy = "explicit";
    if (normalize(text) == "none") {
        configuration.control.enabled_special_cause_tests.clear();
        return;
    }
    configuration.control.enabled_special_cause_tests =
        datalab::domain::statistics::parse_special_cause_tests(text);
}

InputSpec special_cause_tests_input(const QString& chart_kind)
{
    return {
        QStringLiteral("tests"),
        QStringLiteral("特殊原因测试"),
        chart_kind};
}

InputSpec special_cause_policy_input()
{
    InputSpec spec{
        QStringLiteral("rule_policy"),
        QStringLiteral("规则默认策略"),
        QStringLiteral("all_applicable")};
    spec.kind = InputKind::choice;
    spec.choices = {
        {QStringLiteral("all_applicable"),
         QStringLiteral("all_applicable（全部适用特殊原因规则）")},
        {QStringLiteral("minitab_like"),
         QStringLiteral("minitab_like（仅「单点超出 3σ 控制限」）")}};
    spec.help = QStringLiteral(
        "空规则列表时按策略默认。all_applicable 与 Minitab 仅「单点超出 3σ 控制限」不同，误报风险更高。");
    return spec;
}

// DOE 2 水平全因子：doe_factorial 与 doe_response 共用配置构建与执行。
const ApplyFn doe_apply = [](AnalysisConfiguration& c,
                             const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
    auto split = [](const std::string& text) {
        std::vector<std::string> values;
        std::stringstream stream(text);
        std::string value;
        while (std::getline(stream, value, ',')) {
            const std::string normalized = normalize(value);
            if (!normalized.empty()) {
                values.push_back(normalized);
            }
        }
        return values;
    };
    c.analysis_name = "2 水平析因设计";
    c.chart_type = "doe_factorial";
    c.doe.factor_names = split(d.line_text("factors"));
    c.doe.low_levels = split(d.line_text("low"));
    c.doe.high_levels = split(d.line_text("high"));
    c.doe.center_point_count = static_cast<std::size_t>(
        d.line_int("centers").value_or(0));
    c.doe.block_count = static_cast<std::size_t>(
        std::max(1, d.line_int("blocks").value_or(1)));
    c.doe.fraction_p = static_cast<std::size_t>(
        std::max(0, d.line_int("fraction_p").value_or(0)));
    c.doe.generators_text = normalize(d.line_text("generators"));
    c.doe.randomize = true;
    c.doe.random_seed = static_cast<std::uint64_t>(
        std::max(0, d.line_int("seed").value_or(0)));
    c.doe.contour_x_factor = normalize(d.line_text("x_factor"));
    c.doe.contour_y_factor = normalize(d.line_text("y_factor"));
    c.doe.contour_hold_actual.clear();
    const QString hold_text = QString::fromStdString(d.line_text("hold")).trimmed();
    if (!hold_text.isEmpty()) {
        const QStringList parts = hold_text.split(QChar(';'), Qt::SkipEmptyParts);
        for (const QString& part : parts) {
            const int eq = part.indexOf(QChar('='));
            if (eq <= 0) {
                continue;
            }
            const std::string name = normalize(part.left(eq).toStdString());
            const std::string value = part.mid(eq + 1).trimmed().toStdString();
            if (!name.empty()) {
                c.doe.contour_hold_actual[name] = value;
            }
        }
    }
    const int response_column = d.first_role_index("response");
    if (response_column >= 0) {
        c.doe.response_column = static_cast<std::size_t>(response_column);
        const std::vector<int> factor_columns =
            d.role_indices("factor_columns");
        for (const int column : factor_columns) {
            c.doe.factor_columns.push_back(static_cast<std::size_t>(column));
        }
        if (c.doe.factor_columns.empty()) {
            return apply_error(QStringLiteral("因子列不足"),
                               QStringLiteral("响应分析至少需要选择一个已导入因子列。"));
        }
    }
    return {};
};

const RunFn doe_run = [](const DataTable& table,
                         const AnalysisConfiguration& configuration) {
    return AnalysisService::doe_factorial(table, configuration);
};

const ApplyFn response_optimization_apply = [](
    AnalysisConfiguration& c,
    const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
    const AnalysisApplyResult base = doe_apply(c, d);
    if (!base.valid) {
        return base;
    }
    const std::vector<int> response_columns = d.role_indices("response");
    if (response_columns.empty() || c.doe.factor_columns.empty()) {
        return apply_error(QStringLiteral("响应优化"),
                           QStringLiteral("响应优化需要选择至少一个响应列和一个因子列。"));
    }
    c.doe.response_columns.clear();
    for (const int column : response_columns) {
        c.doe.response_columns.push_back(static_cast<std::size_t>(column));
    }
    c.doe.response_column = c.doe.response_columns.front();
    c.analysis_name = "DOE 响应优化";
    c.chart_type = "response_optimization";
    const std::string goal = normalize(d.line_text("goal"));
    c.doe.optimization_goal = goal.empty() ? "maximize" : goal;
    c.doe.optimization_lower = d.line_number("lower");
    c.doe.optimization_upper = d.line_number("upper");
    c.doe.optimization_target = d.line_number("target");
    c.doe.optimization_weight = d.line_number("weight").value_or(1.0);
    c.doe.optimization_confidence = d.line_number("confidence").value_or(0.95);
    c.doe.optimization_objectives.clear();
    QJsonArray objective_rows;
    const QByteArray encoded = QByteArray::fromStdString(d.line_text("objectives"));
    if (!encoded.trimmed().isEmpty()) {
        objective_rows = QJsonDocument::fromJson(encoded).array();
    }
    auto optional_from_json = [](const QJsonValue& value) -> std::optional<double> {
        if (value.isDouble()) {
            return value.toDouble();
        }
        if (value.isString()) {
            bool ok = false;
            const double parsed = value.toString().trimmed().toDouble(&ok);
            return ok ? std::optional<double>(parsed) : std::nullopt;
        }
        return std::nullopt;
    };
    for (std::size_t index = 0; index < c.doe.response_columns.size(); ++index) {
        datalab::domain::DoeResponseObjectiveConfig objective;
        objective.goal = c.doe.optimization_goal;
        objective.lower = c.doe.optimization_lower;
        objective.upper = c.doe.optimization_upper;
        objective.target = c.doe.optimization_target;
        objective.weight = c.doe.optimization_weight;
        if (index < static_cast<std::size_t>(objective_rows.size())) {
            const QJsonObject row = objective_rows.at(static_cast<int>(index)).toObject();
            const std::string goal = normalize(row.value(QStringLiteral("goal"))
                                                   .toString().toStdString());
            if (!goal.empty()) {
                objective.goal = goal;
            }
            if (row.contains(QStringLiteral("lower"))
                && !(row.value(QStringLiteral("lower")).isString()
                     && row.value(QStringLiteral("lower")).toString().trimmed().isEmpty())) {
                objective.lower = optional_from_json(row.value(QStringLiteral("lower")));
            }
            if (row.contains(QStringLiteral("upper"))
                && !(row.value(QStringLiteral("upper")).isString()
                     && row.value(QStringLiteral("upper")).toString().trimmed().isEmpty())) {
                objective.upper = optional_from_json(row.value(QStringLiteral("upper")));
            }
            if (row.contains(QStringLiteral("target"))
                && !(row.value(QStringLiteral("target")).isString()
                     && row.value(QStringLiteral("target")).toString().trimmed().isEmpty())) {
                objective.target = optional_from_json(row.value(QStringLiteral("target")));
            }
            if (row.contains(QStringLiteral("weight"))) {
                objective.weight = optional_from_json(row.value(QStringLiteral("weight")))
                                       .value_or(objective.weight);
            }
        }
        c.doe.optimization_objectives.push_back(objective);
    }
    return {};
};

}  // namespace

InputSpec::InputSpec(QString input_id, QString input_label, QString input_placeholder)
    : id(std::move(input_id))
    , label(std::move(input_label))
    , placeholder(std::move(input_placeholder))
{
    const QString normalized = id.toLower();
    if (placeholder.contains(QStringLiteral("pearson"), Qt::CaseInsensitive)
        && placeholder.contains(QStringLiteral("spearman"), Qt::CaseInsensitive)) {
        kind = InputKind::choice;
        choices = {{QStringLiteral("pearson"), QStringLiteral("Pearson")},
                   {QStringLiteral("spearman"), QStringLiteral("Spearman")}};
    } else if (placeholder.contains(QStringLiteral("welch"), Qt::CaseInsensitive)
               && placeholder.contains(QStringLiteral("pooled"), Qt::CaseInsensitive)) {
        kind = InputKind::choice;
        choices = {{QStringLiteral("welch"), QStringLiteral("Welch")},
                   {QStringLiteral("pooled"), QStringLiteral("Pooled")}};
    } else if (placeholder.contains(QStringLiteral("exact"), Qt::CaseInsensitive)
               && placeholder.contains(QStringLiteral("normal"), Qt::CaseInsensitive)
               && placeholder.contains(QStringLiteral("wilson"), Qt::CaseInsensitive)) {
        kind = InputKind::choice;
        choices = {{QStringLiteral("exact"), QStringLiteral("精确 (Clopper-Pearson)")},
                   {QStringLiteral("normal"), QStringLiteral("正态近似 (Wald CI)")},
                   {QStringLiteral("wilson"), QStringLiteral("Wilson score CI")},
                   {QStringLiteral("agresti_coull"), QStringLiteral("Agresti-Coull CI")}};
    } else if (placeholder.contains(QStringLiteral("normal"), Qt::CaseInsensitive)
               && placeholder.contains(QStringLiteral("wilson"), Qt::CaseInsensitive)
               && !placeholder.contains(QStringLiteral("exact"), Qt::CaseInsensitive)) {
        kind = InputKind::choice;
        choices = {{QStringLiteral("normal"), QStringLiteral("正态近似 (Wald CI)")},
                   {QStringLiteral("wilson"), QStringLiteral("Newcombe-Wilson 差值 CI")}};
    } else if (placeholder.contains(QStringLiteral("exact"), Qt::CaseInsensitive)
               && placeholder.contains(QStringLiteral("normal"), Qt::CaseInsensitive)) {
        kind = InputKind::choice;
        choices = {{QStringLiteral("exact"), QStringLiteral("精确 (Clopper-Pearson)")},
                   {QStringLiteral("normal"), QStringLiteral("正态近似")}};
    } else if (placeholder.contains(QStringLiteral("single"), Qt::CaseInsensitive)
               && placeholder.contains(QStringLiteral("double"), Qt::CaseInsensitive)) {
        kind = InputKind::choice;
        choices = {{QStringLiteral("single"), QStringLiteral("单指数")},
                   {QStringLiteral("double"), QStringLiteral("双指数")}};
    } else if (normalized == QStringLiteral("goal")) {
        kind = InputKind::choice;
        choices = {{QStringLiteral("maximize"), QStringLiteral("最大化")},
                   {QStringLiteral("minimize"), QStringLiteral("最小化")},
                   {QStringLiteral("target"), QStringLiteral("目标值")}};
    } else if (normalized == QStringLiteral("objectives")) {
        kind = InputKind::response_objectives;
    }
    if (normalized == QStringLiteral("confidence")) {
        if (placeholder.contains(QStringLiteral("0.95"))) {
            kind = InputKind::number;
            minimum = 0.0;
            maximum = 1.0;
        } else {
            kind = InputKind::percentage;
            minimum = 0.0;
            maximum = 100.0;
            unit = QStringLiteral("%");
        }
    } else if (normalized == QStringLiteral("alpha")
               || normalized == QStringLiteral("beta")
               || normalized == QStringLiteral("gamma")
               || normalized == QStringLiteral("lambda")) {
        kind = InputKind::number;
        minimum = 0.0;
        maximum = 1.0;
    } else if (normalized == QStringLiteral("period")
               || normalized == QStringLiteral("periods")
               || normalized == QStringLiteral("forecast")
               || normalized == QStringLiteral("levels")
               || normalized == QStringLiteral("bins")
               || normalized == QStringLiteral("groups")
               || normalized == QStringLiteral("blocks")
               || normalized == QStringLiteral("centers")
               || normalized == QStringLiteral("iterations")
               || normalized == QStringLiteral("subgroup_size")
               || normalized == QStringLiteral("mr_length")
               || normalized == QStringLiteral("units")) {
        kind = InputKind::integer;
        minimum = 1.0;
    } else if (normalized == QStringLiteral("seed")) {
        kind = InputKind::integer;
        minimum = 0.0;
    } else if (normalized == QStringLiteral("ordinal")) {
        kind = InputKind::boolean;
    } else if (normalized == QStringLiteral("lsl")
               || normalized == QStringLiteral("usl")
               || normalized == QStringLiteral("tolerance")
               || normalized == QStringLiteral("sigma")
               || normalized == QStringLiteral("k")
               || normalized == QStringLiteral("h")
               || normalized == QStringLiteral("limit")
               || normalized == QStringLiteral("effect")
               || normalized == QStringLiteral("weight")) {
        kind = InputKind::number;
    }
    if (normalized == QStringLiteral("tests")) {
        kind = InputKind::special_cause_tests;
        help = QStringLiteral(
            "特殊原因规则使用稳定 rule_id 与具体名称（如「单点超出 3σ 控制限」）。"
            "默认勾选当前控制图的全部适用规则；"
            "R/S/MR 仅前四条规则，EWMA 仅「单点超出 3σ 控制限」，CUSUM 使用专用累计和信号。");
    }
    if (normalized == QStringLiteral("seed")
        || normalized == QStringLiteral("iterations")
        || normalized == QStringLiteral("tolerance")) {
        advanced = true;
        group = QStringLiteral("高级选项");
    }
    if (help.isEmpty()) {
        if (normalized == QStringLiteral("alpha")) {
            help = QStringLiteral("α：第一类错误概率，通常取 0.05。");
        } else if (normalized == QStringLiteral("confidence")) {
            help = QStringLiteral("置信水平：区间覆盖真值的目标比例；可输入 95 或 0.95。");
        } else if (normalized == QStringLiteral("sigma")) {
            help = QStringLiteral("过程 Sigma：过程波动的标准差估计，必须大于 0。");
        } else if (normalized == QStringLiteral("lsl")
                   || normalized == QStringLiteral("usl")) {
            help = QStringLiteral("规格限用于能力指数；LSL 应小于 USL。");
        } else if (normalized == QStringLiteral("subgroup_size")) {
            help = QStringLiteral("子组大小：每个连续子组包含的观测数量，必须为正整数。");
        }
    }
}

const std::vector<AnalysisCommand>& all()
{
    static const std::vector<AnalysisCommand> commands = {
        // ------------------------------------------------------------ 统计
        {
            QStringLiteral("descriptive"),
            QStringLiteral("显示描述性统计"),
            QStringLiteral("显示描述性统计"),
            QStringLiteral("统计"),
            QStringLiteral("descriptive"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), true, false},
             {QStringLiteral("by"), QStringLiteral("By 变量"), false, true}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "显示描述性统计";
                c.chart_type = "descriptive";
                for (const int index : d.role_indices("variables")) {
                    c.variable_columns.push_back(static_cast<std::size_t>(index));
                }
                if (d.first_role_index("by") >= 0) {
                    c.by_column = static_cast<std::size_t>(
                        d.first_role_index("by"));
                }
                if (c.variable_columns.empty()) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请从左侧选择至少一列到“变量”。"));
                }
                return {};
            },
            AnalysisService::descriptive,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("normality_test"),
            QStringLiteral("正态性检验"),
            QStringLiteral("正态性检验"),
            QStringLiteral("统计"),
            QStringLiteral("normality_test"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false}},
            {{QStringLiteral("method"), QStringLiteral("方法"),
              QStringLiteral("anderson_darling 或 ryan_joiner")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.analysis_name = "正态性检验";
                c.chart_type = "normality_test";
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                c.inference.normality_method = normalize(d.line_text("method"));
                if (c.inference.normality_method != "ryan_joiner") {
                    c.inference.normality_method = "anderson_darling";
                }
                return {};
            },
            AnalysisService::normality_test,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("outlier_test"),
            QStringLiteral("异常值检验"),
            QStringLiteral("异常值检验"),
            QStringLiteral("统计"),
            QStringLiteral("outlier_test"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false}},
            {{QStringLiteral("method"), QStringLiteral("方法"),
              QStringLiteral("grubbs 或 dixon_r10")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.analysis_name = "异常值检验";
                c.chart_type = "outlier_test";
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                c.inference.outlier_method = normalize(d.line_text("method"));
                if (c.inference.outlier_method != "dixon_r10") {
                    c.inference.outlier_method = "grubbs";
                }
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                if (c.inference.confidence_level > 1.0) {
                    c.inference.confidence_level /= 100.0;
                }
                c.inference.alternative = normalize(d.line_text("alternative"));
                if (c.inference.alternative.empty()) {
                    c.inference.alternative = "two_sided";
                }
                return {};
            },
            AnalysisService::outlier_test,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("correlation"),
            QStringLiteral("相关分析"),
            QStringLiteral("相关分析"),
            QStringLiteral("统计"),
            QStringLiteral("correlation"),
            true, true,
            {{QStringLiteral("variables"), QStringLiteral("变量（至少两列）"), true, false}},
            {{QStringLiteral("method"), QStringLiteral("方法"), QStringLiteral("pearson 或 spearman")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")},
             {QStringLiteral("partial"), QStringLiteral("偏相关"),
              QStringLiteral("yes / no（Pearson，≥3 列）")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const std::vector<int> columns = d.role_indices("variables");
                if (columns.size() < 2) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择至少两列数值变量。"));
                }
                c.analysis_name = "相关分析";
                c.chart_type = "correlation";
                for (const int column : columns) {
                    c.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                c.inference.correlation_method = normalize(d.line_text("method"));
                if (c.inference.correlation_method != "spearman") {
                    c.inference.correlation_method = "pearson";
                }
                const double confidence = d.line_number("confidence").value_or(95.0);
                c.inference.confidence_level = confidence > 1.0 ? confidence / 100.0 : confidence;
                const std::string partial = normalize(d.line_text("partial"));
                c.inference.compute_partial_correlation =
                    partial == "yes" || partial == "true" || partial == "1"
                    || partial == "partial";
                return {};
            },
            AnalysisService::correlation,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("one_sample_t"),
            QStringLiteral("单样本 t 检验"),
            QStringLiteral("单样本 t 检验"),
            QStringLiteral("统计"),
            QStringLiteral("one_sample_t"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("测量值"), false, false}},
            {{QStringLiteral("hypothesis_mean"), QStringLiteral("假设均值"), QStringLiteral("例如 10")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                const auto hypothesis = d.line_number("hypothesis_mean");
                if (column < 0 || !hypothesis.has_value()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量列并输入假设均值。"));
                }
                c.analysis_name = "单样本 t 检验";
                c.chart_type = "one_sample_t";
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                c.inference.hypothesis_mean = hypothesis;
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                if (c.inference.confidence_level > 1.0) {
                    c.inference.confidence_level /= 100.0;
                }
                c.inference.alternative = normalize(d.line_text("alternative"));
                return {};
            },
            AnalysisService::one_sample_t,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("one_sample_z"),
            QStringLiteral("单样本 Z 检验"),
            QStringLiteral("单样本 Z 检验"),
            QStringLiteral("统计"),
            QStringLiteral("one_sample_z"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("测量值"), false, false}},
            {{QStringLiteral("hypothesis_mean"), QStringLiteral("假设均值"), QStringLiteral("例如 10")},
             {QStringLiteral("known_sigma"), QStringLiteral("已知标准差 σ"), QStringLiteral("例如 1.5")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                const auto hypothesis = d.line_number("hypothesis_mean");
                const auto known_sigma = d.line_number("known_sigma");
                if (column < 0 || !hypothesis.has_value() || !known_sigma.has_value()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量列并输入假设均值与已知标准差。"));
                }
                if (!(*known_sigma > 0.0)) {
                    return apply_error(QStringLiteral("标准差无效"),
                                       QStringLiteral("已知标准差必须为正。"));
                }
                c.analysis_name = "单样本 Z 检验";
                c.chart_type = "one_sample_z";
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                c.inference.hypothesis_mean = hypothesis;
                c.inference.known_sigma = known_sigma;
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                if (c.inference.confidence_level > 1.0) {
                    c.inference.confidence_level /= 100.0;
                }
                c.inference.alternative = normalize(d.line_text("alternative"));
                return {};
            },
            AnalysisService::one_sample_z,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("one_proportion"),
            QStringLiteral("单比例检验"),
            QStringLiteral("单比例检验"),
            QStringLiteral("统计"),
            QStringLiteral("one_proportion"),
            false, true,
            {{QStringLiteral("events"), QStringLiteral("事件数"), false, false},
             {QStringLiteral("trials"), QStringLiteral("试验数"), false, false}},
            {{QStringLiteral("hypothesis_mean"), QStringLiteral("假设比例"), QStringLiteral("例如 0.5")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")},
             {QStringLiteral("method"), QStringLiteral("方法"),
              QStringLiteral("exact / normal / wilson / agresti_coull")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int events = d.first_role_index("events");
                const int trials = d.first_role_index("trials");
                const auto hypothesis = d.line_number("hypothesis_mean");
                if (events < 0 || trials < 0 || !hypothesis.has_value()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择事件列、试验列并输入假设比例。"));
                }
                if (*hypothesis <= 0.0 || *hypothesis >= 1.0) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("假设比例必须位于 0 和 1 之间。"));
                }
                c.analysis_name = "单比例检验";
                c.chart_type = "one_proportion";
                c.inference.first_events_column = static_cast<std::size_t>(events);
                c.inference.first_trials_column = static_cast<std::size_t>(trials);
                c.inference.hypothesis_mean = hypothesis;
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                if (c.inference.confidence_level > 1.0) {
                    c.inference.confidence_level /= 100.0;
                }
                c.inference.alternative = normalize(d.line_text("alternative"));
                c.inference.proportion_method = normalize(d.line_text("method"));
                if (c.inference.proportion_method != "normal"
                    && c.inference.proportion_method != "wilson"
                    && c.inference.proportion_method != "agresti_coull") {
                    c.inference.proportion_method = "exact";
                }
                return {};
            },
            AnalysisService::one_proportion,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("one_poisson_rate"),
            QStringLiteral("单样本泊松率"),
            QStringLiteral("单样本泊松率检验"),
            QStringLiteral("统计"),
            QStringLiteral("one_poisson_rate"),
            false, true,
            {{QStringLiteral("defects"), QStringLiteral("缺陷数"), false, false},
             {QStringLiteral("length"), QStringLiteral("观测长度（列）"), false, true}},
            {{QStringLiteral("inspected_constant"), QStringLiteral("观测长度（常数）"), QString()},
             {QStringLiteral("hypothesis_mean"), QStringLiteral("假设发生率"), QStringLiteral("例如 0.5")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")},
             {QStringLiteral("method"), QStringLiteral("方法"), QStringLiteral("exact 或 normal")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int defects = d.first_role_index("defects");
                const int length = d.first_role_index("length");
                const auto hypothesis = d.line_number("hypothesis_mean");
                if (defects < 0 || !hypothesis.has_value()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择缺陷列并输入假设发生率。"));
                }
                if (*hypothesis <= 0.0) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("假设发生率必须大于 0。"));
                }
                if (length < 0 && !d.line_int("inspected_constant").has_value()) {
                    return apply_error(QStringLiteral("未指定观测长度"),
                                       QStringLiteral("请选择观测长度列或输入观测长度常数。"));
                }
                c.analysis_name = "单样本泊松率";
                c.chart_type = "one_poisson_rate";
                c.inference.first_events_column = static_cast<std::size_t>(defects);
                if (length >= 0) {
                    c.inference.first_trials_column = static_cast<std::size_t>(length);
                }
                if (d.line_int("inspected_constant").has_value()) {
                    c.inspected_constant = static_cast<std::size_t>(
                        *d.line_int("inspected_constant"));
                }
                c.inference.hypothesis_mean = hypothesis;
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                if (c.inference.confidence_level > 1.0) {
                    c.inference.confidence_level /= 100.0;
                }
                c.inference.alternative = normalize(d.line_text("alternative"));
                c.inference.proportion_method = normalize(d.line_text("method"));
                if (c.inference.proportion_method != "normal") {
                    c.inference.proportion_method = "exact";
                }
                return {};
            },
            AnalysisService::one_poisson_rate,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("two_poisson_rate"),
            QStringLiteral("双样本泊松率"),
            QStringLiteral("双样本泊松率检验"),
            QStringLiteral("统计"),
            QStringLiteral("two_poisson_rate"),
            false, true,
            {{QStringLiteral("first_events"), QStringLiteral("第一组缺陷数"), false, false},
             {QStringLiteral("first_trials"), QStringLiteral("第一组观测长度"), false, false},
             {QStringLiteral("second_events"), QStringLiteral("第二组缺陷数"), false, false},
             {QStringLiteral("second_trials"), QStringLiteral("第二组观测长度"), false, false}},
            {{QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")},
             {QStringLiteral("method"), QStringLiteral("方法"), QStringLiteral("exact 或 normal")},
             {QStringLiteral("comparison"), QStringLiteral("比较量"),
              QStringLiteral("difference 或 ratio")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int first_events = d.first_role_index("first_events");
                const int first_trials = d.first_role_index("first_trials");
                const int second_events = d.first_role_index("second_events");
                const int second_trials = d.first_role_index("second_trials");
                if (first_events < 0 || first_trials < 0 || second_events < 0 || second_trials < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择两组缺陷数和观测长度列。"));
                }
                c.analysis_name = "双样本泊松率";
                c.chart_type = "two_poisson_rate";
                c.inference.first_events_column = static_cast<std::size_t>(first_events);
                c.inference.first_trials_column = static_cast<std::size_t>(first_trials);
                c.inference.second_events_column = static_cast<std::size_t>(second_events);
                c.inference.second_trials_column = static_cast<std::size_t>(second_trials);
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                if (c.inference.confidence_level > 1.0) {
                    c.inference.confidence_level /= 100.0;
                }
                c.inference.alternative = normalize(d.line_text("alternative"));
                c.inference.proportion_method = normalize(d.line_text("method"));
                if (c.inference.proportion_method != "normal") {
                    c.inference.proportion_method = "exact";
                }
                const std::string comparison = normalize(d.line_text("comparison"));
                c.inference.rate_comparison =
                    comparison == "ratio" ? "ratio" : "difference";
                return {};
            },
            AnalysisService::two_poisson_rate,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("two_sample_t"),
            QStringLiteral("双样本 t 检验"),
            QStringLiteral("双样本 t 检验"),
            QStringLiteral("统计"),
            QStringLiteral("two_sample_t"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("两列独立样本"), true, false}},
            {{QStringLiteral("variance"), QStringLiteral("方差方法"), QStringLiteral("welch 或 pooled")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const std::vector<int> columns = d.role_indices("variables");
                if (columns.size() != 2) {
                    return apply_error(QStringLiteral("变量数量错误"),
                                       QStringLiteral("请选择正好两列独立样本变量。"));
                }
                c.analysis_name = "双样本 t 检验";
                c.chart_type = "two_sample_t";
                c.variable_columns = {
                    static_cast<std::size_t>(columns[0]), static_cast<std::size_t>(columns[1])};
                c.inference.variance_method = normalize(d.line_text("variance"));
                if (c.inference.variance_method != "pooled") {
                    c.inference.variance_method = "welch";
                }
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                if (c.inference.confidence_level > 1.0) {
                    c.inference.confidence_level /= 100.0;
                }
                c.inference.alternative = normalize(d.line_text("alternative"));
                return {};
            },
            AnalysisService::two_sample_t,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("one_sample_equivalence"),
            QStringLiteral("单样本等价性检验"),
            QStringLiteral("单样本等价性检验"),
            QStringLiteral("统计"),
            QStringLiteral("equivalence_t"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("测量值"), false, false}},
            {{QStringLiteral("hypothesis_mean"), QStringLiteral("目标值"), QStringLiteral("0")},
             {QStringLiteral("lower"), QStringLiteral("等价下限"), QStringLiteral("例如 -1")},
             {QStringLiteral("upper"), QStringLiteral("等价上限"), QStringLiteral("例如 1")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                const auto lower = d.line_number("lower");
                const auto upper = d.line_number("upper");
                if (column < 0 || !lower.has_value() || !upper.has_value()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量列并输入等价下限与上限。"));
                }
                if (!(*lower < *upper)) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("等价下限必须小于上限。"));
                }
                c.analysis_name = "单样本等价性检验";
                c.chart_type = "one_sample_equivalence";
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                c.inference.hypothesis_mean = d.line_number("hypothesis_mean").value_or(0.0);
                c.inference.equivalence_lower = lower;
                c.inference.equivalence_upper = upper;
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                if (c.inference.confidence_level > 1.0) {
                    c.inference.confidence_level /= 100.0;
                }
                return {};
            },
            AnalysisService::one_sample_equivalence,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("two_sample_equivalence"),
            QStringLiteral("双样本等价性检验"),
            QStringLiteral("双样本等价性检验"),
            QStringLiteral("统计"),
            QStringLiteral("equivalence_t"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("两列独立样本"), true, false}},
            {{QStringLiteral("lower"), QStringLiteral("等价下限"), QStringLiteral("例如 -1")},
             {QStringLiteral("upper"), QStringLiteral("等价上限"), QStringLiteral("例如 1")},
             {QStringLiteral("variance"), QStringLiteral("方差方法"), QStringLiteral("welch 或 pooled")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const std::vector<int> columns = d.role_indices("variables");
                const auto lower = d.line_number("lower");
                const auto upper = d.line_number("upper");
                if (columns.size() != 2 || !lower.has_value() || !upper.has_value()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择两列独立样本并输入等价下限与上限。"));
                }
                if (!(*lower < *upper)) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("等价下限必须小于上限。"));
                }
                c.analysis_name = "双样本等价性检验";
                c.chart_type = "two_sample_equivalence";
                c.variable_columns = {
                    static_cast<std::size_t>(columns[0]), static_cast<std::size_t>(columns[1])};
                c.inference.equivalence_lower = lower;
                c.inference.equivalence_upper = upper;
                c.inference.variance_method = normalize(d.line_text("variance"));
                if (c.inference.variance_method != "pooled") {
                    c.inference.variance_method = "welch";
                }
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                if (c.inference.confidence_level > 1.0) {
                    c.inference.confidence_level /= 100.0;
                }
                return {};
            },
            AnalysisService::two_sample_equivalence,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("two_sample_equivalence_ratio"),
            QStringLiteral("双样本均值比等价性检验"),
            QStringLiteral("双样本均值比等价性检验"),
            QStringLiteral("统计"),
            QStringLiteral("equivalence_t"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("检验列 + 参考列"), true, false}},
            {{QStringLiteral("lower"), QStringLiteral("比值等价下限"), QStringLiteral("例如 0.8")},
             {QStringLiteral("upper"), QStringLiteral("比值等价上限"), QStringLiteral("例如 1.25")},
             {QStringLiteral("variance"), QStringLiteral("方差方法"), QStringLiteral("welch 或 pooled")},
             {QStringLiteral("transform"), QStringLiteral("变换"), QStringLiteral("none / log")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const std::vector<int> columns = d.role_indices("variables");
                const auto lower = d.line_number("lower");
                const auto upper = d.line_number("upper");
                if (columns.size() != 2 || !lower.has_value() || !upper.has_value()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择检验列与参考列，并输入比值等价下限与上限。"));
                }
                if (!(*lower > 0.0) || !(*lower < *upper)) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("比值等价下限必须大于 0 且小于上限。"));
                }
                c.analysis_name = "双样本均值比等价性检验";
                c.chart_type = "two_sample_equivalence_ratio";
                c.variable_columns = {
                    static_cast<std::size_t>(columns[0]), static_cast<std::size_t>(columns[1])};
                c.inference.equivalence_lower = lower;
                c.inference.equivalence_upper = upper;
                c.inference.variance_method = normalize(d.line_text("variance"));
                if (c.inference.variance_method != "pooled") {
                    c.inference.variance_method = "welch";
                }
                c.inference.equivalence_ratio_transform = normalize(d.line_text("transform"));
                if (c.inference.equivalence_ratio_transform != "log") {
                    c.inference.equivalence_ratio_transform = "none";
                }
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                if (c.inference.confidence_level > 1.0) {
                    c.inference.confidence_level /= 100.0;
                }
                return {};
            },
            AnalysisService::two_sample_equivalence_ratio,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("paired_equivalence"),
            QStringLiteral("配对等价性检验"),
            QStringLiteral("配对等价性检验"),
            QStringLiteral("统计"),
            QStringLiteral("equivalence_t"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("两列配对样本"), true, false}},
            {{QStringLiteral("lower"), QStringLiteral("等价下限"), QStringLiteral("例如 -1")},
             {QStringLiteral("upper"), QStringLiteral("等价上限"), QStringLiteral("例如 1")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const std::vector<int> columns = d.role_indices("variables");
                const auto lower = d.line_number("lower");
                const auto upper = d.line_number("upper");
                if (columns.size() != 2 || !lower.has_value() || !upper.has_value()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择两列配对样本并输入等价下限与上限。"));
                }
                if (!(*lower < *upper)) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("等价下限必须小于上限。"));
                }
                c.analysis_name = "配对等价性检验";
                c.chart_type = "paired_equivalence";
                c.variable_columns = {
                    static_cast<std::size_t>(columns[0]), static_cast<std::size_t>(columns[1])};
                c.inference.equivalence_lower = lower;
                c.inference.equivalence_upper = upper;
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                if (c.inference.confidence_level > 1.0) {
                    c.inference.confidence_level /= 100.0;
                }
                return {};
            },
            AnalysisService::paired_equivalence,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("one_proportion_equivalence"),
            QStringLiteral("单比例等价性检验"),
            QStringLiteral("单比例等价性检验"),
            QStringLiteral("统计"),
            QStringLiteral("equivalence_t"),
            false, true,
            {{QStringLiteral("events"), QStringLiteral("事件数"), false, false},
             {QStringLiteral("trials"), QStringLiteral("试验数"), false, false}},
            {{QStringLiteral("hypothesis_mean"), QStringLiteral("目标比例"), QStringLiteral("例如 0.5")},
             {QStringLiteral("lower"), QStringLiteral("等价下限"), QStringLiteral("例如 -0.05")},
             {QStringLiteral("upper"), QStringLiteral("等价上限"), QStringLiteral("例如 0.05")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int events = d.first_role_index("events");
                const int trials = d.first_role_index("trials");
                const auto hypothesis = d.line_number("hypothesis_mean");
                const auto lower = d.line_number("lower");
                const auto upper = d.line_number("upper");
                if (events < 0 || trials < 0 || !hypothesis.has_value()
                    || !lower.has_value() || !upper.has_value()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择事件/试验列，并输入目标比例与等价界限。"));
                }
                if (!(*lower < *upper)) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("等价下限必须小于上限。"));
                }
                if (!(*hypothesis >= 0.0 && *hypothesis <= 1.0)) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("目标比例必须在 0 与 1 之间。"));
                }
                c.analysis_name = "单比例等价性检验";
                c.chart_type = "one_proportion_equivalence";
                c.inference.first_events_column = static_cast<std::size_t>(events);
                c.inference.first_trials_column = static_cast<std::size_t>(trials);
                c.inference.hypothesis_mean = hypothesis;
                c.inference.equivalence_lower = lower;
                c.inference.equivalence_upper = upper;
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                if (c.inference.confidence_level > 1.0) {
                    c.inference.confidence_level /= 100.0;
                }
                return {};
            },
            AnalysisService::one_proportion_equivalence,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("two_proportion_equivalence"),
            QStringLiteral("两比例等价性检验"),
            QStringLiteral("两比例等价性检验"),
            QStringLiteral("统计"),
            QStringLiteral("equivalence_t"),
            false, true,
            {{QStringLiteral("first_events"), QStringLiteral("第一组事件数"), false, false},
             {QStringLiteral("first_trials"), QStringLiteral("第一组试验数"), false, false},
             {QStringLiteral("second_events"), QStringLiteral("第二组事件数"), false, false},
             {QStringLiteral("second_trials"), QStringLiteral("第二组试验数"), false, false}},
            {{QStringLiteral("lower"), QStringLiteral("等价下限"), QStringLiteral("例如 -0.05")},
             {QStringLiteral("upper"), QStringLiteral("等价上限"), QStringLiteral("例如 0.05")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int first_events = d.first_role_index("first_events");
                const int first_trials = d.first_role_index("first_trials");
                const int second_events = d.first_role_index("second_events");
                const int second_trials = d.first_role_index("second_trials");
                const auto lower = d.line_number("lower");
                const auto upper = d.line_number("upper");
                if (first_events < 0 || first_trials < 0 || second_events < 0
                    || second_trials < 0 || !lower.has_value() || !upper.has_value()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择两组事件/试验列并输入等价界限。"));
                }
                if (!(*lower < *upper)) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("等价下限必须小于上限。"));
                }
                c.analysis_name = "两比例等价性检验";
                c.chart_type = "two_proportion_equivalence";
                c.inference.first_events_column = static_cast<std::size_t>(first_events);
                c.inference.first_trials_column = static_cast<std::size_t>(first_trials);
                c.inference.second_events_column = static_cast<std::size_t>(second_events);
                c.inference.second_trials_column = static_cast<std::size_t>(second_trials);
                c.inference.equivalence_lower = lower;
                c.inference.equivalence_upper = upper;
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                if (c.inference.confidence_level > 1.0) {
                    c.inference.confidence_level /= 100.0;
                }
                return {};
            },
            AnalysisService::two_proportion_equivalence,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("one_way_anova"),
            QStringLiteral("单因素 ANOVA"),
            QStringLiteral("单因素 ANOVA"),
            QStringLiteral("统计"),
            QStringLiteral("one_way_anova"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应变量"), false, false},
             {QStringLiteral("factor"), QStringLiteral("因子/分组列"), false, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                const int factor = d.first_role_index("factor");
                if (response < 0 || factor < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择响应变量和因子/分组列。"));
                }
                c.analysis_name = "单因素 ANOVA";
                c.chart_type = "one_way_anova";
                c.variable_columns.push_back(static_cast<std::size_t>(response));
                c.selection.measurement_column = static_cast<std::size_t>(response);
                c.by_column = static_cast<std::size_t>(factor);
                return {};
            },
            AnalysisService::one_way_anova,
            QStringLiteral("ANOVA")},
        {
            QStringLiteral("paired_t"),
            QStringLiteral("配对 t 检验"),
            QStringLiteral("配对 t 检验"),
            QStringLiteral("统计"),
            QStringLiteral("paired_t"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("配对变量（两列）"), true, false}},
            {{QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const std::vector<int> columns = d.role_indices("variables");
                if (columns.size() != 2) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("配对 t 检验必须选择恰好两列。"));
                }
                c.analysis_name = "配对 t 检验";
                c.chart_type = "paired_t";
                c.variable_columns = {
                    static_cast<std::size_t>(columns[0]), static_cast<std::size_t>(columns[1])};
                const auto confidence = d.line_number("confidence");
                if (confidence.has_value()) {
                    c.inference.confidence_level = *confidence > 1.0 ? *confidence / 100.0 : *confidence;
                }
                c.inference.alternative = normalize(d.line_text("alternative"));
                return {};
            },
            AnalysisService::paired_t,
            QStringLiteral("基础统计")},
        {
            QStringLiteral("regression"),
            QStringLiteral("线性回归"),
            QStringLiteral("线性回归"),
            QStringLiteral("统计"),
            QStringLiteral("regression"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量（第一列响应，其余为预测变量）"),
              true, false}},
            {{QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const std::vector<int> columns = d.role_indices("variables");
                if (columns.size() < 2) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("回归至少需要一列响应变量和一列预测变量。"));
                }
                c.analysis_name = "线性回归";
                c.chart_type = "regression";
                for (const int column : columns) {
                    c.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                const auto confidence = d.line_number("confidence");
                if (confidence.has_value()) {
                    c.inference.confidence_level = *confidence > 1.0 ? *confidence / 100.0 : *confidence;
                }
                return {};
            },
            AnalysisService::regression,
            QStringLiteral("回归")},
        {
            QStringLiteral("two_proportions"),
            QStringLiteral("两比例检验"),
            QStringLiteral("两比例检验"),
            QStringLiteral("统计"),
            QStringLiteral("two_proportions"),
            false, true,
            {{QStringLiteral("first_events"), QStringLiteral("第一组事件数"), false, false},
             {QStringLiteral("first_trials"), QStringLiteral("第一组试验数"), false, false},
             {QStringLiteral("second_events"), QStringLiteral("第二组事件数"), false, false},
             {QStringLiteral("second_trials"), QStringLiteral("第二组试验数"), false, false}},
            {{QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")},
             {QStringLiteral("method"), QStringLiteral("方法"),
              QStringLiteral("normal / wilson / agresti_coull")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int first_events = d.first_role_index("first_events");
                const int first_trials = d.first_role_index("first_trials");
                const int second_events = d.first_role_index("second_events");
                const int second_trials = d.first_role_index("second_trials");
                if (first_events < 0 || first_trials < 0 || second_events < 0 || second_trials < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择四个计数列。"));
                }
                c.analysis_name = "两比例检验";
                c.chart_type = "two_proportions";
                c.inference.first_events_column = static_cast<std::size_t>(first_events);
                c.inference.first_trials_column = static_cast<std::size_t>(first_trials);
                c.inference.second_events_column = static_cast<std::size_t>(second_events);
                c.inference.second_trials_column = static_cast<std::size_t>(second_trials);
                const auto confidence = d.line_number("confidence");
                if (confidence.has_value()) {
                    c.inference.confidence_level = *confidence > 1.0 ? *confidence / 100.0 : *confidence;
                }
                c.inference.proportion_method = normalize(d.line_text("method"));
                if (c.inference.proportion_method != "wilson"
                    && c.inference.proportion_method != "agresti_coull") {
                    c.inference.proportion_method = "normal";
                }
                c.inference.alternative = normalize(d.line_text("alternative"));
                return {};
            },
            AnalysisService::two_proportions,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("chi_square"),
            QStringLiteral("列联表卡方"),
            QStringLiteral("列联表卡方"),
            QStringLiteral("统计"),
            QStringLiteral("chi_square"),
            false, true,
            {{QStringLiteral("row_category"), QStringLiteral("行分类列"), false, false},
             {QStringLiteral("column_category"), QStringLiteral("列分类列"), false, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int row = d.first_role_index("row_category");
                const int column = d.first_role_index("column_category");
                if (row < 0 || column < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择行分类列和列分类列。"));
                }
                c.analysis_name = "列联表卡方";
                c.chart_type = "chi_square";
                c.inference.row_category_column = static_cast<std::size_t>(row);
                c.inference.column_category_column = static_cast<std::size_t>(column);
                return {};
            },
            AnalysisService::chi_square,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("cross_tabulation"),
            QStringLiteral("交叉表"),
            QStringLiteral("交叉表 / Tally"),
            QStringLiteral("统计"),
            QStringLiteral("chi_square"),
            false, true,
            {{QStringLiteral("row_category"), QStringLiteral("行分类列"), false, false},
             {QStringLiteral("column_category"), QStringLiteral("列分类列"), false, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int row = d.first_role_index("row_category");
                const int column = d.first_role_index("column_category");
                if (row < 0 || column < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择行分类列和列分类列。"));
                }
                c.analysis_name = "交叉表";
                c.chart_type = "cross_tabulation";
                c.inference.row_category_column = static_cast<std::size_t>(row);
                c.inference.column_category_column = static_cast<std::size_t>(column);
                return {};
            },
            AnalysisService::cross_tabulation,
            QStringLiteral("表格")},
        {
            QStringLiteral("chi_square_gof"),
            QStringLiteral("卡方拟合优度"),
            QStringLiteral("卡方拟合优度"),
            QStringLiteral("统计"),
            QStringLiteral("chi_square"),
            false, true,
            {{QStringLiteral("category"), QStringLiteral("分类列"), false, false}},
            {{QStringLiteral("expected_proportions"), QStringLiteral("期望比例（逗号，可选）"),
              QStringLiteral("留空则等比例")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("category");
                if (column < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择分类列。"));
                }
                c.analysis_name = "卡方拟合优度";
                c.chart_type = "chi_square_gof";
                c.inference.gof_category_column = static_cast<std::size_t>(column);
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.inference.expected_proportions = normalize(d.line_text("expected_proportions"));
                return {};
            },
            AnalysisService::chi_square_gof,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("poisson_gof"),
            QStringLiteral("泊松拟合优度"),
            QStringLiteral("泊松拟合优度"),
            QStringLiteral("统计"),
            QStringLiteral("chi_square"),
            false, true,
            {{QStringLiteral("counts"), QStringLiteral("计数列（非负整数）"), false, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("counts");
                if (column < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择一列非负整数计数。"));
                }
                c.analysis_name = "泊松拟合优度";
                c.chart_type = "poisson_gof";
                c.variable_columns = {static_cast<std::size_t>(column)};
                return {};
            },
            AnalysisService::poisson_gof,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("anom"),
            QStringLiteral("均值分析 (ANOM)"),
            QStringLiteral("均值分析 (ANOM)"),
            QStringLiteral("统计"),
            QStringLiteral("one_way_anova"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应变量"), false, false},
             {QStringLiteral("factor"), QStringLiteral("因子/分组列"), false, false}},
            {{QStringLiteral("alpha"), QStringLiteral("族误差率 α"), QStringLiteral("0.05")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                const int factor = d.first_role_index("factor");
                if (response < 0 || factor < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择响应变量和因子/分组列。"));
                }
                c.analysis_name = "均值分析 (ANOM)";
                c.chart_type = "anom";
                c.variable_columns.push_back(static_cast<std::size_t>(response));
                c.selection.measurement_column = static_cast<std::size_t>(response);
                c.by_column = static_cast<std::size_t>(factor);
                c.inference.anom_alpha = d.line_number("alpha").value_or(0.05);
                if (c.inference.anom_alpha <= 0.0 || c.inference.anom_alpha >= 1.0) {
                    return apply_error(QStringLiteral("α 无效"),
                                       QStringLiteral("α 必须在 (0, 1) 内。"));
                }
                return {};
            },
            AnalysisService::anom,
            QStringLiteral("ANOVA")},
        {
            QStringLiteral("mann_whitney"),
            QStringLiteral("Mann-Whitney 检验"),
            QStringLiteral("Mann-Whitney 检验"),
            QStringLiteral("统计"),
            QStringLiteral("mann_whitney"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("两列独立样本"), true, false}},
            {{QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const std::vector<int> columns = d.role_indices("variables");
                if (columns.size() != 2) {
                    return apply_error(QStringLiteral("变量数量错误"),
                                       QStringLiteral("请选择正好两列独立样本。"));
                }
                c.analysis_name = "Mann-Whitney 检验";
                c.chart_type = "mann_whitney";
                c.variable_columns = {
                    static_cast<std::size_t>(columns[0]), static_cast<std::size_t>(columns[1])};
                c.inference.alternative = normalize(d.line_text("alternative"));
                return {};
            },
            AnalysisService::mann_whitney,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("wilcoxon_signed_rank"),
            QStringLiteral("Wilcoxon 符号秩检验"),
            QStringLiteral("Wilcoxon 符号秩检验"),
            QStringLiteral("统计"),
            QStringLiteral("wilcoxon_signed_rank"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("一列或两列配对"), true, false}},
            {{QStringLiteral("hypothesized_median"), QStringLiteral("假设中位数 η0"),
              QStringLiteral("单样本默认 0；配对忽略")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"),
              QStringLiteral("Walsh CI，默认 95")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const std::vector<int> columns = d.role_indices("variables");
                if (columns.empty() || columns.size() > 2) {
                    return apply_error(QStringLiteral("变量数量错误"),
                                       QStringLiteral("请选择一列（相对 η0）或两列配对样本。"));
                }
                c.analysis_name = "Wilcoxon 符号秩检验";
                c.chart_type = "wilcoxon_signed_rank";
                c.variable_columns.clear();
                for (const int column : columns) {
                    c.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                c.inference.alternative = normalize(d.line_text("alternative"));
                c.inference.hypothesis_mean =
                    d.line_number("hypothesized_median").value_or(0.0);
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                return {};
            },
            AnalysisService::wilcoxon_signed_rank,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("sign_test"),
            QStringLiteral("符号检验"),
            QStringLiteral("符号检验"),
            QStringLiteral("统计"),
            QStringLiteral("sign_test"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("一列或两列配对"), true, false}},
            {{QStringLiteral("hypothesized_median"), QStringLiteral("假设中位数 η0"),
              QStringLiteral("单样本默认 0；配对忽略")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"),
              QStringLiteral("中位数 CI，默认 95")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const std::vector<int> columns = d.role_indices("variables");
                if (columns.empty() || columns.size() > 2) {
                    return apply_error(QStringLiteral("变量数量错误"),
                                       QStringLiteral("请选择一列（单样本）或两列（配对）。"));
                }
                c.analysis_name = "符号检验";
                c.chart_type = "sign_test";
                c.variable_columns.clear();
                for (const int column : columns) {
                    c.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                c.inference.alternative = normalize(d.line_text("alternative"));
                c.inference.hypothesis_mean =
                    d.line_number("hypothesized_median").value_or(0.0);
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                return {};
            },
            AnalysisService::sign_test,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("runs_test"),
            QStringLiteral("游程检验"),
            QStringLiteral("游程检验"),
            QStringLiteral("统计"),
            QStringLiteral("wilcoxon_signed_rank"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("数值序列（一列）"), false, false}},
            {{QStringLiteral("runs_criterion"), QStringLiteral("比较准则"),
              QStringLiteral("mean / median / value（默认 mean）")},
             {QStringLiteral("criterion_value"), QStringLiteral("常数 K（criterion=value）"),
              QStringLiteral("仅 value 时使用")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择一列数值序列。"));
                }
                c.analysis_name = "游程检验";
                c.chart_type = "runs_test";
                c.variable_columns = {static_cast<std::size_t>(column)};
                std::string criterion = normalize(d.line_text("runs_criterion"));
                if (criterion != "median" && criterion != "value") {
                    criterion = "mean";
                }
                c.inference.runs_criterion = criterion;
                if (criterion == "value") {
                    const std::optional<double> value = d.line_number("criterion_value");
                    if (!value.has_value()) {
                        return apply_error(QStringLiteral("参数不足"),
                                           QStringLiteral("比较准则为 value 时请填写常数 K。"));
                    }
                    c.inference.hypothesis_mean = value;
                }
                return {};
            },
            AnalysisService::runs_test,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("mcnemar"),
            QStringLiteral("McNemar 检验"),
            QStringLiteral("McNemar 检验"),
            QStringLiteral("统计"),
            QStringLiteral("mcnemar"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("两列配对二元结果"), true, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const std::vector<int> columns = d.role_indices("variables");
                if (columns.size() != 2) {
                    return apply_error(QStringLiteral("变量数量错误"),
                                       QStringLiteral("请选择正好两列配对二元结果。"));
                }
                c.analysis_name = "McNemar 检验";
                c.chart_type = "mcnemar";
                c.variable_columns = {
                    static_cast<std::size_t>(columns[0]),
                    static_cast<std::size_t>(columns[1])};
                return {};
            },
            AnalysisService::mcnemar,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("fisher_exact"),
            QStringLiteral("Fisher 精确检验"),
            QStringLiteral("Fisher 精确检验"),
            QStringLiteral("统计"),
            QStringLiteral("chi_square"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("两列分类（2×2）"), true, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const std::vector<int> columns = d.role_indices("variables");
                if (columns.size() != 2) {
                    return apply_error(QStringLiteral("变量数量错误"),
                                       QStringLiteral("请选择正好两列分类变量。"));
                }
                c.analysis_name = "Fisher 精确检验";
                c.chart_type = "fisher_exact";
                c.variable_columns = {
                    static_cast<std::size_t>(columns[0]),
                    static_cast<std::size_t>(columns[1])};
                return {};
            },
            AnalysisService::fisher_exact,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("cochran_q"),
            QStringLiteral("Cochran Q 检验"),
            QStringLiteral("Cochran Q 检验"),
            QStringLiteral("统计"),
            QStringLiteral("cochran_q"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("≥3 列配对二元"), true, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const std::vector<int> columns = d.role_indices("variables");
                if (columns.size() < 2) {
                    return apply_error(QStringLiteral("变量数量错误"),
                                       QStringLiteral("请选择至少两列配对二元结果（计算需 k≥3）。"));
                }
                c.analysis_name = "Cochran Q 检验";
                c.chart_type = "cochran_q";
                c.variable_columns.clear();
                for (const int column : columns) {
                    c.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                return {};
            },
            AnalysisService::cochran_q,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("mood_median"),
            QStringLiteral("Mood 中位数检验"),
            QStringLiteral("Mood 中位数检验"),
            QStringLiteral("统计"),
            QStringLiteral("mood_median"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("测量值"), false, false},
             {QStringLiteral("factor"), QStringLiteral("分组列"), false, false}},
            {{QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"),
              QStringLiteral("各组 Sign CI，默认 95")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                const int factor = d.first_role_index("factor");
                if (response < 0 || factor < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量值和分组列。"));
                }
                c.analysis_name = "Mood 中位数检验";
                c.chart_type = "mood_median";
                c.variable_columns = {static_cast<std::size_t>(response)};
                c.by_column = static_cast<std::size_t>(factor);
                c.inference.confidence_level = d.line_number("confidence").value_or(95.0);
                return {};
            },
            AnalysisService::mood_median,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("kruskal_wallis"),
            QStringLiteral("Kruskal-Wallis 检验"),
            QStringLiteral("Kruskal-Wallis 检验"),
            QStringLiteral("统计"),
            QStringLiteral("kruskal_wallis"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("测量值"), false, false},
             {QStringLiteral("factor"), QStringLiteral("分组列"), false, false}},
            {{QStringLiteral("posthoc"), QStringLiteral("多重比较"),
              QStringLiteral("dunn / steel_dwass")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                const int factor = d.first_role_index("factor");
                if (response < 0 || factor < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量值和分组列。"));
                }
                c.analysis_name = "Kruskal-Wallis 检验";
                c.chart_type = "kruskal_wallis";
                c.variable_columns = {static_cast<std::size_t>(response)};
                c.by_column = static_cast<std::size_t>(factor);
                c.inference.nonparametric_posthoc = normalize(d.line_text("posthoc"));
                if (c.inference.nonparametric_posthoc != "steel_dwass") {
                    c.inference.nonparametric_posthoc = "dunn";
                }
                return {};
            },
            AnalysisService::kruskal_wallis,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("friedman"),
            QStringLiteral("Friedman 检验"),
            QStringLiteral("Friedman 检验"),
            QStringLiteral("统计"),
            QStringLiteral("friedman"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应"), false, false},
             {QStringLiteral("treatment"), QStringLiteral("处理"), false, false},
             {QStringLiteral("block"), QStringLiteral("区组"), false, false}},
            {{QStringLiteral("posthoc"), QStringLiteral("多重比较"),
              QStringLiteral("留空或 nemenyi")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                const int treatment = d.first_role_index("treatment");
                const int block = d.first_role_index("block");
                if (response < 0 || treatment < 0 || block < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择响应、处理与区组列。"));
                }
                c.analysis_name = "Friedman 检验";
                c.chart_type = "friedman";
                c.variable_columns = {static_cast<std::size_t>(response)};
                c.by_column = static_cast<std::size_t>(treatment);
                c.inference.anova_factor_b_column = static_cast<std::size_t>(block);
                c.inference.nonparametric_posthoc = normalize(d.line_text("posthoc"));
                if (c.inference.nonparametric_posthoc != "nemenyi") {
                    c.inference.nonparametric_posthoc.clear();
                }
                return {};
            },
            AnalysisService::friedman,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("time_series_smoothing"),
            QStringLiteral("时间序列平滑"),
            QStringLiteral("时间序列平滑"),
            QStringLiteral("统计"),
            QStringLiteral("time_series_smoothing"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("时间序列"), false, false}},
            {{QStringLiteral("method"), QStringLiteral("方法"), QStringLiteral("single 或 double")},
             {QStringLiteral("alpha"), QStringLiteral("Alpha"), QStringLiteral("0.2")},
             {QStringLiteral("gamma"), QStringLiteral("Gamma（双指数）"), QStringLiteral("0.2")},
             {QStringLiteral("periods"), QStringLiteral("预测期数"), QStringLiteral("1")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_silent();
                }
                c.analysis_name = "时间序列平滑";
                c.chart_type = "time_series_smoothing";
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.time_series.smoothing_method = normalize(d.line_text("method"));
                if (c.time_series.smoothing_method != "single") {
                    c.time_series.smoothing_method = "double";
                }
                c.time_series.smoothing_alpha = d.line_number("alpha").value_or(0.2);
                c.time_series.smoothing_gamma = d.line_number("gamma").value_or(0.2);
                c.time_series.forecast_periods = d.line_int("periods").value_or(1);
                return {};
            },
            AnalysisService::time_series_smoothing,
            QStringLiteral("时间序列")},
        {
            QStringLiteral("arima"),
            QStringLiteral("ARIMA 基础预测"),
            QStringLiteral("ARIMA 基础预测"),
            QStringLiteral("统计"),
            QStringLiteral("arima"),
            false, true,
            {{QStringLiteral("time"), QStringLiteral("时间列（可选）"), false, false},
             {QStringLiteral("value"), QStringLiteral("时间序列值"), false, false}},
            {{QStringLiteral("criterion"), QStringLiteral("选模准则"),
              QStringLiteral("aicc / aic / bic")},
             {QStringLiteral("periods"), QStringLiteral("预测期数"), QStringLiteral("3")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int value = d.first_role_index("value");
                if (value < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择时间序列值列。"));
                }
                c.analysis_name = "ARIMA 基础预测";
                c.chart_type = "arima";
                const int time = d.first_role_index("time");
                if (time >= 0) {
                    c.time_series.arima_time_column = static_cast<std::size_t>(time);
                }
                c.time_series.arima_value_column = static_cast<std::size_t>(value);
                c.time_series.arima_selection_criterion = normalize(d.line_text("criterion"));
                if (c.time_series.arima_selection_criterion != "aic"
                    && c.time_series.arima_selection_criterion != "bic") {
                    c.time_series.arima_selection_criterion = "aicc";
                }
                c.time_series.forecast_periods = d.line_int("periods").value_or(3);
                return {};
            },
            AnalysisService::arima,
            QStringLiteral("时间序列")},
        {
            QStringLiteral("two_factor_anova"),
            QStringLiteral("双因素 ANOVA"),
            QStringLiteral("双因素 ANOVA"),
            QStringLiteral("统计"),
            QStringLiteral("two_factor_anova"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应变量"), false, false},
             {QStringLiteral("factor_a"), QStringLiteral("因子 A"), false, false},
             {QStringLiteral("factor_b"), QStringLiteral("因子 B"), false, false}},
            {{QStringLiteral("encoding"), QStringLiteral("因子编码"),
              QStringLiteral("reference / effect")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                const int factor_a = d.first_role_index("factor_a");
                const int factor_b = d.first_role_index("factor_b");
                if (response < 0 || factor_a < 0 || factor_b < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择响应变量、因子 A 和因子 B。"));
                }
                c.analysis_name = "双因素 ANOVA";
                c.chart_type = "two_factor_anova";
                c.inference.anova_response_column = static_cast<std::size_t>(response);
                c.inference.anova_factor_a_column = static_cast<std::size_t>(factor_a);
                c.inference.anova_factor_b_column = static_cast<std::size_t>(factor_b);
                c.inference.anova_factor_encoding = normalize(d.line_text("encoding"));
                if (c.inference.anova_factor_encoding != "effect") {
                    c.inference.anova_factor_encoding = "reference";
                }
                return {};
            },
            AnalysisService::two_factor_anova,
            QStringLiteral("ANOVA")},
        {
            QStringLiteral("logistic_regression"),
            QStringLiteral("二元 Logistic 回归"),
            QStringLiteral("二元 Logistic 回归"),
            QStringLiteral("统计"),
            QStringLiteral("logistic_regression"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("二元响应"), false, false},
             {QStringLiteral("predictors"), QStringLiteral("数值预测变量"), true, false}},
            {{QStringLiteral("event"), QStringLiteral("事件水平"),
              QStringLiteral("0/1 数据可留空；文本数据输入事件标签")},
             {QStringLiteral("iterations"), QStringLiteral("最大迭代次数"), QStringLiteral("100")},
             {QStringLiteral("tolerance"), QStringLiteral("收敛阈值"), QStringLiteral("1e-8")},
             {QStringLiteral("stepwise"), QStringLiteral("逐步选择"), QStringLiteral("off/on")},
             {QStringLiteral("stepwise_method"), QStringLiteral("逐步方法"),
              QStringLiteral("forward_aicc / forward_bic / forward / backward / stepwise")},
             {QStringLiteral("stepwise_alpha_enter"), QStringLiteral("α 进入"), QStringLiteral("0.15")},
             {QStringLiteral("stepwise_alpha_remove"), QStringLiteral("α 移除"), QStringLiteral("0.15")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                const std::vector<int> predictors = d.role_indices("predictors");
                if (response < 0 || predictors.empty()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择一个二元响应列和至少一个预测变量。"));
                }
                c.analysis_name = "二元 Logistic 回归";
                c.chart_type = "logistic_regression";
                c.inference.logistic_response_column = static_cast<std::size_t>(response);
                for (const int predictor : predictors) {
                    c.inference.logistic_predictor_columns.push_back(
                        static_cast<std::size_t>(predictor));
                }
                c.inference.logistic_event_level = d.line_text("event");
                if (c.inference.logistic_event_level.empty()) {
                    c.inference.logistic_event_level = "1";
                }
                c.inference.logistic_max_iterations = d.line_int("iterations").value_or(100);
                c.inference.logistic_tolerance = d.line_number("tolerance").value_or(1.0e-8);
                const std::string stepwise = normalize(d.line_text("stepwise"));
                c.inference.logistic_stepwise_enabled =
                    stepwise == "on" || stepwise == "1" || stepwise == "true"
                    || stepwise == "yes";
                const std::string stepwise_method = normalize(d.line_text("stepwise_method"));
                if (!stepwise_method.empty()) {
                    c.inference.logistic_stepwise_method = stepwise_method;
                }
                c.inference.logistic_stepwise_alpha_enter =
                    d.line_number("stepwise_alpha_enter").value_or(0.15);
                c.inference.logistic_stepwise_alpha_remove =
                    d.line_number("stepwise_alpha_remove").value_or(0.15);
                return {};
            },
            AnalysisService::logistic_regression,
            QStringLiteral("回归")},
        {
            QStringLiteral("variance_test"),
            QStringLiteral("方差检验"),
            QStringLiteral("方差检验"),
            QStringLiteral("统计"),
            QStringLiteral("variance_test"),
            false, true,
            {{QStringLiteral("first"), QStringLiteral("第一样本 / 测量列"), false, false},
             {QStringLiteral("second"), QStringLiteral("第二样本（两方差）"), false, false},
             {QStringLiteral("group"), QStringLiteral("分组列（k 组等方差）"), false, false}},
            {{QStringLiteral("hypothesis"), QStringLiteral("假设方差（一方差）"),
              QStringLiteral("例如 1.0")},
             {QStringLiteral("method"), QStringLiteral("两方差 / 等方差方法"),
              QStringLiteral("f / levene / levene_mean / bonett / bartlett")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int first = d.first_role_index("first");
                const int second = d.first_role_index("second");
                const int group = d.first_role_index("group");
                if (first < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择第一样本列或测量列。"));
                }
                c.analysis_name = "方差检验";
                c.chart_type = "variance_test";
                c.inference.variance_first_column = static_cast<std::size_t>(first);
                if (second >= 0) {
                    c.inference.variance_second_column = static_cast<std::size_t>(second);
                }
                if (group >= 0) {
                    c.inference.variance_group_column = static_cast<std::size_t>(group);
                }
                c.inference.hypothesized_variance = d.line_number("hypothesis");
                c.inference.variance_test_method = normalize(d.line_text("method"));
                if (c.inference.variance_test_method == "levene_mean") {
                    c.inference.variance_test_method = "levene_mean";
                } else if (c.inference.variance_test_method == "levene"
                           || c.inference.variance_test_method == "brown_forsythe") {
                    c.inference.variance_test_method = "levene";
                } else if (c.inference.variance_test_method == "bonett") {
                    c.inference.variance_test_method = "bonett";
                } else if (c.inference.variance_test_method == "bartlett") {
                    c.inference.variance_test_method = "bartlett";
                } else {
                    c.inference.variance_test_method = "f";
                }
                c.inference.variance_alternative = normalize(d.line_text("alternative"));
                if (c.inference.variance_alternative != "less"
                    && c.inference.variance_alternative != "greater") {
                    c.inference.variance_alternative = "two_sided";
                }
                return {};
            },
            AnalysisService::variance_test,
            QStringLiteral("假设检验")},
        {
            QStringLiteral("time_series_decomposition"),
            QStringLiteral("时间序列分解"),
            QStringLiteral("时间序列分解"),
            QStringLiteral("统计"),
            QStringLiteral("time_series_decomposition"),
            false, true,
            {{QStringLiteral("time"), QStringLiteral("时间列（可选）"), false, false},
             {QStringLiteral("value"), QStringLiteral("时间序列值"), false, false}},
            {{QStringLiteral("period"), QStringLiteral("季节周期"), QStringLiteral("例如 4 或 12")},
             {QStringLiteral("model"), QStringLiteral("模型"),
              QStringLiteral("additive / multiplicative")},
             {QStringLiteral("periods"), QStringLiteral("预测期数"), QStringLiteral("4")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int value = d.first_role_index("value");
                if (value < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择时间序列值列。"));
                }
                c.analysis_name = "时间序列分解";
                c.chart_type = "time_series_decomposition";
                const int time = d.first_role_index("time");
                if (time >= 0) {
                    c.time_series.decomposition_time_column = static_cast<std::size_t>(time);
                }
                c.time_series.decomposition_value_column = static_cast<std::size_t>(value);
                c.time_series.decomposition_seasonal_period = d.line_int("period").value_or(1);
                c.time_series.decomposition_model = normalize(d.line_text("model"));
                if (c.time_series.decomposition_model != "multiplicative") {
                    c.time_series.decomposition_model = "additive";
                }
                c.time_series.forecast_periods = d.line_int("periods").value_or(4);
                return {};
            },
            AnalysisService::time_series_decomposition,
            QStringLiteral("时间序列")},
        {
            QStringLiteral("seasonal_forecasting"),
            QStringLiteral("季节性预测"),
            QStringLiteral("季节性预测"),
            QStringLiteral("统计"),
            QStringLiteral("seasonal_forecasting"),
            false, true,
            {{QStringLiteral("value"), QStringLiteral("时间序列值"), false, false}},
            {{QStringLiteral("period"), QStringLiteral("季节周期"), QStringLiteral("12")},
             {QStringLiteral("error"), QStringLiteral("误差模型"),
              QStringLiteral("additive / multiplicative")},
             {QStringLiteral("trend"), QStringLiteral("趋势模型"),
              QStringLiteral("additive / none / multiplicative")},
             {QStringLiteral("alpha"), QStringLiteral("Alpha"), QStringLiteral("0.2")},
             {QStringLiteral("beta"), QStringLiteral("Beta"), QStringLiteral("0.1")},
             {QStringLiteral("gamma"), QStringLiteral("Gamma"), QStringLiteral("0.2")},
             {QStringLiteral("forecast"), QStringLiteral("预测期数"), QStringLiteral("4")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int value = d.first_role_index("value");
                if (value < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择时间序列值列。"));
                }
                c.analysis_name = "季节性预测";
                c.chart_type = "seasonal_forecasting";
                c.time_series.decomposition_value_column = static_cast<std::size_t>(value);
                c.time_series.seasonal_period = static_cast<std::size_t>(
                    std::max(1, d.line_int("period").value_or(12)));
                c.time_series.seasonal_error_model = normalize(d.line_text("error"));
                if (c.time_series.seasonal_error_model != "multiplicative") {
                    c.time_series.seasonal_error_model = "additive";
                }
                c.time_series.seasonal_trend_model = normalize(d.line_text("trend"));
                if (c.time_series.seasonal_trend_model != "none"
                    && c.time_series.seasonal_trend_model != "multiplicative") {
                    c.time_series.seasonal_trend_model = "additive";
                }
                c.time_series.smoothing_alpha = d.line_number("alpha").value_or(0.2);
                c.time_series.seasonal_beta = d.line_number("beta").value_or(0.1);
                c.time_series.smoothing_gamma = d.line_number("gamma").value_or(0.2);
                c.time_series.forecast_periods = std::max(1, d.line_int("forecast").value_or(4));
                return {};
            },
            AnalysisService::seasonal_forecasting,
            QStringLiteral("时间序列")},
        {
            QStringLiteral("pca"),
            QStringLiteral("主成分分析"),
            QStringLiteral("主成分分析"),
            QStringLiteral("统计"),
            QStringLiteral("pca"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("数值变量（可多选）"), true, false}},
            {{QStringLiteral("mode"), QStringLiteral("矩阵模式"),
              QStringLiteral("covariance / standardized")},
             {QStringLiteral("components"), QStringLiteral("保留主成分数"),
              QStringLiteral("0 = 全部")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "主成分分析";
                c.chart_type = "pca";
                for (const int column : d.role_indices("variables")) {
                    if (column >= 0) {
                        c.pca.variable_columns.push_back(static_cast<std::size_t>(column));
                    }
                }
                c.pca.mode = normalize(d.line_text("mode"));
                c.pca.component_count = static_cast<std::size_t>(
                    std::max(0, d.line_int("components").value_or(0)));
                return {};
            },
            AnalysisService::pca,
            QStringLiteral("多变量")},
        {
            QStringLiteral("kmeans"),
            QStringLiteral("K-Means 聚类"),
            QStringLiteral("Cluster K-Means"),
            QStringLiteral("统计"),
            QStringLiteral("scatter"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("数值变量（可多选）"), true, false}},
            {{QStringLiteral("k"), QStringLiteral("簇数 k"), QStringLiteral("2")},
             {QStringLiteral("max_iterations"), QStringLiteral("最大迭代"), QStringLiteral("100")},
             {QStringLiteral("standardize"), QStringLiteral("标准化"), QStringLiteral("false")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "K-Means 聚类";
                c.chart_type = "kmeans";
                for (const int column : d.role_indices("variables")) {
                    if (column >= 0) {
                        c.kmeans.variable_columns.push_back(static_cast<std::size_t>(column));
                    }
                }
                if (c.kmeans.variable_columns.size() < 2) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请至少选择两个数值变量。"));
                }
                c.kmeans.cluster_count = static_cast<std::size_t>(
                    std::max(2, d.line_int("k").value_or(2)));
                c.kmeans.max_iterations = static_cast<std::size_t>(
                    std::max(1, d.line_int("max_iterations").value_or(100)));
                const std::string standardize = normalize(d.line_text("standardize"));
                c.kmeans.standardize =
                    standardize == "true" || standardize == "1" || standardize == "yes";
                return {};
            },
            AnalysisService::kmeans,
            QStringLiteral("多变量")},
        {
            QStringLiteral("cart_tree"),
            QStringLiteral("CART 单树"),
            QStringLiteral("CART Tree"),
            QStringLiteral("统计"),
            QStringLiteral("scatter"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应"), false, false},
             {QStringLiteral("predictors"), QStringLiteral("预测变量（数值）"), true, false}},
            {{QStringLiteral("task"), QStringLiteral("任务"),
              QStringLiteral("classification / regression")},
             {QStringLiteral("max_depth"), QStringLiteral("最大深度"), QStringLiteral("5")},
             {QStringLiteral("min_leaf"), QStringLiteral("最小叶样本"), QStringLiteral("5")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                if (response < 0) {
                    return apply_error(QStringLiteral("未选择响应"),
                                       QStringLiteral("请选择响应列。"));
                }
                c.analysis_name = "CART 单树";
                c.chart_type = "cart_tree";
                c.cart_tree.response_column = static_cast<std::size_t>(response);
                for (const int column : d.role_indices("predictors")) {
                    if (column >= 0) {
                        c.cart_tree.predictor_columns.push_back(static_cast<std::size_t>(column));
                    }
                }
                if (c.cart_tree.predictor_columns.empty()) {
                    return apply_error(QStringLiteral("未选择预测变量"),
                                       QStringLiteral("请至少选择一个数值预测列。"));
                }
                const std::string task = normalize(d.line_text("task"));
                c.cart_tree.task = (task == "regression") ? "regression" : "classification";
                c.cart_tree.max_depth = static_cast<std::size_t>(
                    std::max(1, d.line_int("max_depth").value_or(5)));
                c.cart_tree.min_leaf = static_cast<std::size_t>(
                    std::max(1, d.line_int("min_leaf").value_or(5)));
                return {};
            },
            AnalysisService::cart_tree,
            QStringLiteral("多变量")},
        {
            QStringLiteral("random_forest"),
            QStringLiteral("随机森林"),
            QStringLiteral("Random Forest"),
            QStringLiteral("统计"),
            QStringLiteral("scatter"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应"), false, false},
             {QStringLiteral("predictors"), QStringLiteral("预测变量（数值）"), true, false}},
            {{QStringLiteral("task"), QStringLiteral("任务"),
              QStringLiteral("classification / regression")},
             {QStringLiteral("n_trees"), QStringLiteral("树数"), QStringLiteral("50")},
             {QStringLiteral("max_depth"), QStringLiteral("最大深度"), QStringLiteral("5")},
             {QStringLiteral("min_leaf"), QStringLiteral("最小叶样本"), QStringLiteral("5")},
             {QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("1")},
             {QStringLiteral("oob"), QStringLiteral("计算 OOB"), QStringLiteral("true")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                if (response < 0) {
                    return apply_error(QStringLiteral("未选择响应"),
                                       QStringLiteral("请选择响应列。"));
                }
                c.analysis_name = "随机森林";
                c.chart_type = "random_forest";
                c.random_forest.response_column = static_cast<std::size_t>(response);
                for (const int column : d.role_indices("predictors")) {
                    if (column >= 0) {
                        c.random_forest.predictor_columns.push_back(
                            static_cast<std::size_t>(column));
                    }
                }
                if (c.random_forest.predictor_columns.empty()) {
                    return apply_error(QStringLiteral("未选择预测变量"),
                                       QStringLiteral("请至少选择一个数值预测列。"));
                }
                const std::string task = normalize(d.line_text("task"));
                c.random_forest.task =
                    (task == "regression") ? "regression" : "classification";
                c.random_forest.n_trees = static_cast<std::size_t>(
                    std::max(1, d.line_int("n_trees").value_or(50)));
                c.random_forest.max_depth = static_cast<std::size_t>(
                    std::max(1, d.line_int("max_depth").value_or(5)));
                c.random_forest.min_leaf = static_cast<std::size_t>(
                    std::max(1, d.line_int("min_leaf").value_or(5)));
                c.random_forest.seed = static_cast<std::uint32_t>(
                    std::max(0, d.line_int("seed").value_or(1)));
                const std::string oob = normalize(d.line_text("oob"));
                c.random_forest.compute_oob =
                    !(oob == "false" || oob == "0" || oob == "no");
                return {};
            },
            AnalysisService::random_forest,
            QStringLiteral("多变量")},
        {
            QStringLiteral("adf_test"),
            QStringLiteral("ADF 单位根检验"),
            QStringLiteral("Augmented Dickey-Fuller"),
            QStringLiteral("统计"),
            QStringLiteral("time_series_plot"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("序列"), false, false}},
            {{QStringLiteral("regression"), QStringLiteral("回归规格"),
              QStringLiteral("none / drift / trend")},
             {QStringLiteral("lags"), QStringLiteral("滞后（可空=自动）"), QStringLiteral("")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择数值序列列。"));
                }
                c.analysis_name = "ADF 单位根检验";
                c.chart_type = "adf_test";
                c.adf.series_column = static_cast<std::size_t>(column);
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                const std::string regression = normalize(d.line_text("regression"));
                if (regression == "none" || regression == "trend") {
                    c.adf.regression = regression;
                } else {
                    c.adf.regression = "drift";
                }
                c.adf.lags = static_cast<std::size_t>(
                    std::max(0, d.line_int("lags").value_or(0)));
                return {};
            },
            AnalysisService::adf_test,
            QStringLiteral("时间序列")},
        {
            QStringLiteral("poisson_regression"),
            QStringLiteral("Poisson 回归"),
            QStringLiteral("Poisson Regression"),
            QStringLiteral("统计"),
            QStringLiteral("scatter"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("计数响应"), false, false},
             {QStringLiteral("predictors"), QStringLiteral("数值预测变量"), true, false}},
            {{QStringLiteral("iterations"), QStringLiteral("最大迭代"), QStringLiteral("100")},
             {QStringLiteral("tolerance"), QStringLiteral("收敛阈值"), QStringLiteral("1e-8")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                if (response < 0) {
                    return apply_error(QStringLiteral("未选择响应"),
                                       QStringLiteral("请选择计数响应列。"));
                }
                c.analysis_name = "Poisson 回归";
                c.chart_type = "poisson_regression";
                c.poisson_regression.response_column = static_cast<std::size_t>(response);
                for (const int column : d.role_indices("predictors")) {
                    if (column >= 0) {
                        c.poisson_regression.predictor_columns.push_back(
                            static_cast<std::size_t>(column));
                    }
                }
                if (c.poisson_regression.predictor_columns.empty()) {
                    return apply_error(QStringLiteral("未选择预测变量"),
                                       QStringLiteral("请至少选择一个数值预测列。"));
                }
                c.poisson_regression.max_iterations = static_cast<std::size_t>(
                    std::max(1, d.line_int("iterations").value_or(100)));
                c.poisson_regression.tolerance = d.line_number("tolerance").value_or(1.0e-8);
                return {};
            },
            AnalysisService::poisson_regression,
            QStringLiteral("回归")},
        {
            QStringLiteral("isolation_forest"),
            QStringLiteral("Isolation Forest"),
            QStringLiteral("Isolation Forest"),
            QStringLiteral("统计"),
            QStringLiteral("time_series_plot"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("数值变量（可多选）"), true, false}},
            {{QStringLiteral("trees"), QStringLiteral("树数"), QStringLiteral("100")},
             {QStringLiteral("max_samples"), QStringLiteral("每树样本上限"), QStringLiteral("256")},
             {QStringLiteral("score_quantile"), QStringLiteral("分数分位阈值"), QStringLiteral("0.90")},
             {QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("1")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "Isolation Forest";
                c.chart_type = "isolation_forest";
                for (const int column : d.role_indices("variables")) {
                    if (column >= 0) {
                        c.isolation_forest.variable_columns.push_back(
                            static_cast<std::size_t>(column));
                    }
                }
                if (c.isolation_forest.variable_columns.size() < 2) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请至少选择两个数值变量。"));
                }
                c.isolation_forest.tree_count = static_cast<std::size_t>(
                    std::max(1, d.line_int("trees").value_or(100)));
                c.isolation_forest.max_samples = static_cast<std::size_t>(
                    std::max(2, d.line_int("max_samples").value_or(256)));
                c.isolation_forest.score_quantile =
                    d.line_number("score_quantile").value_or(0.90);
                c.isolation_forest.seed = static_cast<std::uint32_t>(
                    std::max(0, d.line_int("seed").value_or(1)));
                return {};
            },
            AnalysisService::isolation_forest,
            QStringLiteral("多变量")},
        {
            QStringLiteral("bootstrap_mean"),
            QStringLiteral("Bootstrap 均值 CI"),
            QStringLiteral("Bootstrap Mean CI"),
            QStringLiteral("统计"),
            QStringLiteral("histogram"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false}},
            {{QStringLiteral("replicates"), QStringLiteral("重抽样次数 B"), QStringLiteral("2000")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平"), QStringLiteral("0.95")},
             {QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("1")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择数值列。"));
                }
                c.analysis_name = "Bootstrap 均值 CI";
                c.chart_type = "bootstrap_mean";
                c.bootstrap_mean.series_column = static_cast<std::size_t>(column);
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.bootstrap_mean.replicates = static_cast<std::size_t>(
                    std::max(50, d.line_int("replicates").value_or(2000)));
                c.bootstrap_mean.confidence_level =
                    d.line_number("confidence").value_or(0.95);
                c.bootstrap_mean.seed = static_cast<std::uint32_t>(
                    std::max(0, d.line_int("seed").value_or(1)));
                return {};
            },
            AnalysisService::bootstrap_mean,
            QStringLiteral("推断 / 仿真")},
        {
            QStringLiteral("distribution_calculator"),
            QStringLiteral("分布计算器"),
            QStringLiteral("Distribution Calculator"),
            QStringLiteral("统计"),
            QStringLiteral("histogram"),
            false, false,
            {},
            {{QStringLiteral("distribution"), QStringLiteral("分布"),
              QStringLiteral("normal / t / chi_square / f / weibull")},
             {QStringLiteral("operation"), QStringLiteral("运算"),
              QStringLiteral("pdf / cdf / quantile")},
             {QStringLiteral("param1"), QStringLiteral("参数1"), QStringLiteral("0")},
             {QStringLiteral("param2"), QStringLiteral("参数2"), QStringLiteral("1")},
             {QStringLiteral("param3"), QStringLiteral("参数3（F 的 df2）"), QStringLiteral("1")},
             {QStringLiteral("value"), QStringLiteral("x 或概率 p"), QStringLiteral("0")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "分布计算器";
                c.chart_type = "distribution_calculator";
                const std::string dist = normalize(d.line_text("distribution"));
                c.distribution_calculator.distribution =
                    dist.empty() ? "normal" : dist;
                const std::string op = normalize(d.line_text("operation"));
                c.distribution_calculator.operation = op.empty() ? "cdf" : op;
                c.distribution_calculator.param1 =
                    d.line_number("param1").value_or(0.0);
                c.distribution_calculator.param2 =
                    d.line_number("param2").value_or(1.0);
                c.distribution_calculator.param3 =
                    d.line_number("param3").value_or(1.0);
                c.distribution_calculator.value =
                    d.line_number("value").value_or(0.0);
                return {};
            },
            AnalysisService::distribution_calculator,
            QStringLiteral("推断 / 仿真")},
        {
            QStringLiteral("bootstrap_two_sample"),
            QStringLiteral("Bootstrap 双样本均值差 CI"),
            QStringLiteral("Bootstrap Two-Sample Mean Difference CI"),
            QStringLiteral("统计"),
            QStringLiteral("histogram"),
            false, true,
            {{QStringLiteral("first"), QStringLiteral("样本 1"), false, false},
             {QStringLiteral("second"), QStringLiteral("样本 2"), false, false}},
            {{QStringLiteral("replicates"), QStringLiteral("重抽样次数 B"), QStringLiteral("2000")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平"), QStringLiteral("0.95")},
             {QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("1")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int first = d.first_role_index("first");
                const int second = d.first_role_index("second");
                if (first < 0 || second < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择两个数值列。"));
                }
                if (first == second) {
                    return apply_error(QStringLiteral("变量重复"),
                                       QStringLiteral("两个样本列必须不同。"));
                }
                c.analysis_name = "Bootstrap 双样本均值差 CI";
                c.chart_type = "bootstrap_two_sample";
                c.bootstrap_two_sample.first_column = static_cast<std::size_t>(first);
                c.bootstrap_two_sample.second_column = static_cast<std::size_t>(second);
                c.variable_columns.push_back(static_cast<std::size_t>(first));
                c.variable_columns.push_back(static_cast<std::size_t>(second));
                c.bootstrap_two_sample.replicates = static_cast<std::size_t>(
                    std::max(50, d.line_int("replicates").value_or(2000)));
                c.bootstrap_two_sample.confidence_level =
                    d.line_number("confidence").value_or(0.95);
                c.bootstrap_two_sample.seed = static_cast<std::uint32_t>(
                    std::max(0, d.line_int("seed").value_or(1)));
                return {};
            },
            AnalysisService::bootstrap_two_sample,
            QStringLiteral("推断 / 仿真")},
        {
            QStringLiteral("probit_reliability"),
            QStringLiteral("Probit 可靠性"),
            QStringLiteral("Probit Reliability"),
            QStringLiteral("统计"),
            QStringLiteral("scatter"),
            false, true,
            {{QStringLiteral("events"), QStringLiteral("事件数"), false, false},
             {QStringLiteral("trials"), QStringLiteral("试验数"), false, false},
             {QStringLiteral("stress"), QStringLiteral("应力/剂量"), false, false}},
            {{QStringLiteral("iterations"), QStringLiteral("最大迭代次数"), QStringLiteral("100")},
             {QStringLiteral("tolerance"), QStringLiteral("收敛阈值"), QStringLiteral("1e-8")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int events = d.first_role_index("events");
                const int trials = d.first_role_index("trials");
                const int stress = d.first_role_index("stress");
                if (events < 0 || trials < 0 || stress < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择事件数、试验数与应力/剂量列。"));
                }
                c.analysis_name = "Probit 可靠性";
                c.chart_type = "probit_reliability";
                c.probit_reliability.events_column = static_cast<std::size_t>(events);
                c.probit_reliability.trials_column = static_cast<std::size_t>(trials);
                c.probit_reliability.stress_column = static_cast<std::size_t>(stress);
                c.probit_reliability.max_iterations = static_cast<std::size_t>(
                    std::max(1, d.line_int("iterations").value_or(100)));
                c.probit_reliability.tolerance = d.line_number("tolerance").value_or(1.0e-8);
                return {};
            },
            AnalysisService::probit_reliability,
            QStringLiteral("可靠性")},
        {
            QStringLiteral("cluster_observations"),
            QStringLiteral("层次聚类（观测）"),
            QStringLiteral("Cluster Observations"),
            QStringLiteral("统计"),
            QStringLiteral("scatter"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("数值变量（可多选）"), true, false}},
            {{QStringLiteral("k"), QStringLiteral("切簇数 k"), QStringLiteral("2")},
             {QStringLiteral("standardize"), QStringLiteral("标准化"), QStringLiteral("false")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "层次聚类（观测）";
                c.chart_type = "cluster_observations";
                for (const int column : d.role_indices("variables")) {
                    if (column >= 0) {
                        c.hierarchical_cluster.variable_columns.push_back(
                            static_cast<std::size_t>(column));
                    }
                }
                if (c.hierarchical_cluster.variable_columns.size() < 2) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请至少选择两个数值变量。"));
                }
                c.hierarchical_cluster.cluster_count = static_cast<std::size_t>(
                    std::max(2, d.line_int("k").value_or(2)));
                const std::string standardize = normalize(d.line_text("standardize"));
                c.hierarchical_cluster.standardize =
                    standardize == "true" || standardize == "1" || standardize == "yes";
                return {};
            },
            AnalysisService::cluster_observations,
            QStringLiteral("多变量")},
        {
            QStringLiteral("ordinal_logistic"),
            QStringLiteral("有序 Logistic 回归"),
            QStringLiteral("Ordinal Logistic"),
            QStringLiteral("统计"),
            QStringLiteral("report"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("有序响应"), false, false},
             {QStringLiteral("predictors"), QStringLiteral("数值预测变量"), true, false}},
            {{QStringLiteral("iterations"), QStringLiteral("最大迭代"), QStringLiteral("50")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                if (response < 0) {
                    return apply_error(QStringLiteral("未选择响应"),
                                       QStringLiteral("请选择有序响应列。"));
                }
                c.analysis_name = "有序 Logistic 回归";
                c.chart_type = "ordinal_logistic";
                c.ordinal_logistic.response_column = static_cast<std::size_t>(response);
                for (const int column : d.role_indices("predictors")) {
                    if (column >= 0) {
                        c.ordinal_logistic.predictor_columns.push_back(
                            static_cast<std::size_t>(column));
                    }
                }
                if (c.ordinal_logistic.predictor_columns.empty()) {
                    return apply_error(QStringLiteral("未选择预测变量"),
                                       QStringLiteral("请至少选择一个数值预测列。"));
                }
                c.ordinal_logistic.max_iterations = static_cast<std::size_t>(
                    std::max(1, d.line_int("iterations").value_or(50)));
                return {};
            },
            AnalysisService::ordinal_logistic,
            QStringLiteral("回归")},
        {
            QStringLiteral("nominal_logistic"),
            QStringLiteral("名义 Logistic 回归"),
            QStringLiteral("Nominal Logistic"),
            QStringLiteral("统计"),
            QStringLiteral("report"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("名义响应"), false, false},
             {QStringLiteral("predictors"), QStringLiteral("数值预测变量"), true, false}},
            {{QStringLiteral("iterations"), QStringLiteral("最大迭代"), QStringLiteral("50")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                if (response < 0) {
                    return apply_error(QStringLiteral("未选择响应"),
                                       QStringLiteral("请选择名义响应列。"));
                }
                c.analysis_name = "名义 Logistic 回归";
                c.chart_type = "nominal_logistic";
                c.nominal_logistic.response_column = static_cast<std::size_t>(response);
                for (const int column : d.role_indices("predictors")) {
                    if (column >= 0) {
                        c.nominal_logistic.predictor_columns.push_back(
                            static_cast<std::size_t>(column));
                    }
                }
                if (c.nominal_logistic.predictor_columns.empty()) {
                    return apply_error(QStringLiteral("未选择预测变量"),
                                       QStringLiteral("请至少选择一个数值预测列。"));
                }
                c.nominal_logistic.max_iterations = static_cast<std::size_t>(
                    std::max(1, d.line_int("iterations").value_or(50)));
                return {};
            },
            AnalysisService::nominal_logistic,
            QStringLiteral("回归")},
        {
            QStringLiteral("discriminant"),
            QStringLiteral("线性判别分析"),
            QStringLiteral("Discriminant Analysis"),
            QStringLiteral("统计"),
            QStringLiteral("scatter"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("类别响应"), false, false},
             {QStringLiteral("predictors"), QStringLiteral("数值预测变量"), true, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                if (response < 0) {
                    return apply_error(QStringLiteral("未选择响应"),
                                       QStringLiteral("请选择类别响应列。"));
                }
                c.analysis_name = "线性判别分析";
                c.chart_type = "discriminant";
                c.discriminant.response_column = static_cast<std::size_t>(response);
                for (const int column : d.role_indices("predictors")) {
                    if (column >= 0) {
                        c.discriminant.predictor_columns.push_back(
                            static_cast<std::size_t>(column));
                    }
                }
                if (c.discriminant.predictor_columns.empty()) {
                    return apply_error(QStringLiteral("未选择预测变量"),
                                       QStringLiteral("请至少选择一个数值预测列。"));
                }
                return {};
            },
            AnalysisService::discriminant,
            QStringLiteral("多变量")},
        {
            QStringLiteral("ccf"),
            QStringLiteral("互相关（CCF）"),
            QStringLiteral("Cross Correlation"),
            QStringLiteral("统计"),
            QStringLiteral("time_series_plot"),
            false, true,
            {{QStringLiteral("x"), QStringLiteral("序列 X"), false, false},
             {QStringLiteral("y"), QStringLiteral("序列 Y"), false, false}},
            {{QStringLiteral("max_lag"), QStringLiteral("最大滞后（可空）"), QStringLiteral("")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平"), QStringLiteral("0.95")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int x = d.first_role_index("x");
                const int y = d.first_role_index("y");
                if (x < 0 || y < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择两个数值序列列。"));
                }
                c.analysis_name = "互相关（CCF）";
                c.chart_type = "ccf";
                c.ccf.x_column = static_cast<std::size_t>(x);
                c.ccf.y_column = static_cast<std::size_t>(y);
                c.ccf.max_lag = static_cast<std::size_t>(
                    std::max(0, d.line_int("max_lag").value_or(0)));
                c.inference.confidence_level =
                    d.line_number("confidence").value_or(0.95);
                return {};
            },
            AnalysisService::ccf,
            QStringLiteral("时间序列")},
        {
            QStringLiteral("correlogram"),
            QStringLiteral("Correlogram（相关热图）"),
            QStringLiteral("Correlogram"),
            QStringLiteral("图形"),
            QStringLiteral("heatmap"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("数值变量（可多选）"), true, false}},
            {{QStringLiteral("method"), QStringLiteral("方法"), QStringLiteral("pearson / spearman")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "Correlogram";
                c.chart_type = "correlogram";
                for (const int column : d.role_indices("variables")) {
                    if (column >= 0) {
                        c.correlogram.variable_columns.push_back(
                            static_cast<std::size_t>(column));
                    }
                }
                if (c.correlogram.variable_columns.size() < 2) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请至少选择两个数值变量。"));
                }
                const std::string method = normalize(d.line_text("method"));
                c.correlogram.method = (method == "spearman") ? "spearman" : "pearson";
                return {};
            },
            AnalysisService::correlogram,
            QStringLiteral("关系与多元")},
        {
            QStringLiteral("stepwise_regression"),
            QStringLiteral("逐步回归"),
            QStringLiteral("Stepwise Regression"),
            QStringLiteral("统计"),
            QStringLiteral("regression"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应"), false, false},
             {QStringLiteral("predictors"), QStringLiteral("候选预测变量"), true, false}},
            {{QStringLiteral("method"), QStringLiteral("方法"),
              QStringLiteral("stepwise / forward / backward / forward_aicc / forward_bic")},
             {QStringLiteral("alpha_enter"), QStringLiteral("α enter"), QStringLiteral("0.15")},
             {QStringLiteral("alpha_remove"), QStringLiteral("α remove"), QStringLiteral("0.15")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                if (response < 0) {
                    return apply_error(QStringLiteral("未选择响应"),
                                       QStringLiteral("请选择响应列。"));
                }
                c.analysis_name = "逐步回归";
                c.chart_type = "stepwise_regression";
                c.stepwise_regression.response_column = static_cast<std::size_t>(response);
                for (const int column : d.role_indices("predictors")) {
                    if (column >= 0) {
                        c.stepwise_regression.predictor_columns.push_back(
                            static_cast<std::size_t>(column));
                    }
                }
                if (c.stepwise_regression.predictor_columns.size() < 2) {
                    return apply_error(QStringLiteral("预测变量不足"),
                                       QStringLiteral("请至少选择两个候选预测列。"));
                }
                const std::string method = normalize(d.line_text("method"));
                if (method == "forward" || method == "backward" || method == "forward_aicc"
                    || method == "forward_bic") {
                    c.stepwise_regression.method = method;
                } else {
                    c.stepwise_regression.method = "stepwise";
                }
                c.stepwise_regression.alpha_enter =
                    d.line_number("alpha_enter").value_or(0.15);
                c.stepwise_regression.alpha_remove =
                    d.line_number("alpha_remove").value_or(0.15);
                return {};
            },
            AnalysisService::stepwise_regression,
            QStringLiteral("回归")},
        {
            QStringLiteral("best_subsets_regression"),
            QStringLiteral("Best Subsets 回归"),
            QStringLiteral("Best Subsets Regression"),
            QStringLiteral("统计"),
            QStringLiteral("regression"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应"), false, false},
             {QStringLiteral("predictors"), QStringLiteral("候选预测变量"), true, false}},
            {{QStringLiteral("min_predictors"), QStringLiteral("最小预测变量数"), QStringLiteral("1")},
             {QStringLiteral("max_predictors"), QStringLiteral("最大预测变量数（0=全部）"), QStringLiteral("0")},
             {QStringLiteral("models_per_size"), QStringLiteral("每规模模型数"), QStringLiteral("1")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int response = d.first_role_index("response");
                if (response < 0) {
                    return apply_error(QStringLiteral("未选择响应"),
                                       QStringLiteral("请选择响应列。"));
                }
                c.analysis_name = "Best Subsets 回归";
                c.chart_type = "best_subsets_regression";
                c.best_subsets_regression.response_column = static_cast<std::size_t>(response);
                for (const int column : d.role_indices("predictors")) {
                    if (column >= 0) {
                        c.best_subsets_regression.predictor_columns.push_back(
                            static_cast<std::size_t>(column));
                    }
                }
                if (c.best_subsets_regression.predictor_columns.empty()) {
                    return apply_error(QStringLiteral("预测变量不足"),
                                       QStringLiteral("请至少选择一个候选预测列。"));
                }
                c.best_subsets_regression.min_predictors = static_cast<std::size_t>(
                    std::max(1.0, d.line_number("min_predictors").value_or(1.0)));
                const double max_raw = d.line_number("max_predictors").value_or(0.0);
                c.best_subsets_regression.max_predictors =
                    max_raw <= 0.0 ? 0 : static_cast<std::size_t>(max_raw);
                c.best_subsets_regression.models_per_size = static_cast<std::size_t>(
                    std::clamp(d.line_number("models_per_size").value_or(1.0), 1.0, 5.0));
                return {};
            },
            AnalysisService::best_subsets_regression,
            QStringLiteral("回归")},
        {
            QStringLiteral("km_interval"),
            QStringLiteral("区间删失 Kaplan–Meier"),
            QStringLiteral("Interval-Censored KM"),
            QStringLiteral("统计"),
            QStringLiteral("report"),
            false, true,
            {{QStringLiteral("left"), QStringLiteral("区间左端 L"), false, false},
             {QStringLiteral("right"), QStringLiteral("区间右端 R（空/Inf=右删失）"), false, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int left = d.first_role_index("left");
                const int right = d.first_role_index("right");
                if (left < 0 || right < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择区间左右端列。"));
                }
                c.analysis_name = "区间删失 Kaplan–Meier";
                c.chart_type = "km_interval";
                c.km_interval.left_column = static_cast<std::size_t>(left);
                c.km_interval.right_column = static_cast<std::size_t>(right);
                return {};
            },
            AnalysisService::km_interval,
            QStringLiteral("可靠性")},
        {
            QStringLiteral("doe_plackett_burman"),
            QStringLiteral("Plackett–Burman 设计生成"),
            QStringLiteral("Plackett-Burman Design"),
            QStringLiteral("统计"),
            QStringLiteral("doe_factorial"),
            false, false,
            {},
            {{QStringLiteral("factors"), QStringLiteral("因子名（逗号分隔）"), QStringLiteral("A,B,C,D,E,F,G")},
             {QStringLiteral("low"), QStringLiteral("低水平（逗号，可空）"), QStringLiteral("")},
             {QStringLiteral("high"), QStringLiteral("高水平（逗号，可空）"), QStringLiteral("")},
             {QStringLiteral("centers"), QStringLiteral("中心点数"), QStringLiteral("0")},
             {QStringLiteral("randomize"), QStringLiteral("随机化"), QStringLiteral("false")},
             {QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("1")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "Plackett–Burman 设计";
                c.chart_type = "doe_plackett_burman";
                const QString factors_text =
                    QString::fromStdString(normalize(d.line_text("factors")));
                const QStringList names = factors_text.split(QLatin1Char(','), Qt::SkipEmptyParts);
                for (const QString& name : names) {
                    c.plackett_burman.factor_names.push_back(name.trimmed().toStdString());
                }
                if (c.plackett_burman.factor_names.empty()) {
                    return apply_error(QStringLiteral("因子不足"),
                                       QStringLiteral("请提供至少一个因子名称。"));
                }
                const QStringList lows =
                    QString::fromStdString(normalize(d.line_text("low")))
                        .split(QLatin1Char(','), Qt::KeepEmptyParts);
                const QStringList highs =
                    QString::fromStdString(normalize(d.line_text("high")))
                        .split(QLatin1Char(','), Qt::KeepEmptyParts);
                for (std::size_t i = 0; i < c.plackett_burman.factor_names.size(); ++i) {
                    c.plackett_burman.low_levels.push_back(
                        i < static_cast<std::size_t>(lows.size()) && !lows[static_cast<int>(i)].trimmed().isEmpty()
                            ? lows[static_cast<int>(i)].trimmed().toStdString() : "-1");
                    c.plackett_burman.high_levels.push_back(
                        i < static_cast<std::size_t>(highs.size()) && !highs[static_cast<int>(i)].trimmed().isEmpty()
                            ? highs[static_cast<int>(i)].trimmed().toStdString() : "+1");
                }
                c.plackett_burman.center_point_count = static_cast<std::size_t>(
                    std::max(0, d.line_int("centers").value_or(0)));
                const std::string randomize = normalize(d.line_text("randomize"));
                c.plackett_burman.randomize =
                    randomize == "true" || randomize == "1" || randomize == "yes";
                c.plackett_burman.random_seed = static_cast<std::uint64_t>(
                    std::max(0, d.line_int("seed").value_or(1)));
                return {};
            },
            AnalysisService::doe_plackett_burman,
            QStringLiteral("DOE")},
        {
            QStringLiteral("taguchi_orthogonal_design"),
            QStringLiteral("Taguchi 正交设计"),
            QStringLiteral("Taguchi Orthogonal Design"),
            QStringLiteral("统计"),
            QStringLiteral("doe_factorial"),
            false, false,
            {},
            {{QStringLiteral("factors"), QStringLiteral("因子名（逗号分隔）"), QStringLiteral("A,B,C")},
             {QStringLiteral("array"), QStringLiteral("阵列 L8/L9/L12"), QStringLiteral("L8")},
             {QStringLiteral("low"), QStringLiteral("低水平（逗号，可空）"), QStringLiteral("")},
             {QStringLiteral("high"), QStringLiteral("高水平（逗号，可空）"), QStringLiteral("")},
             {QStringLiteral("mid"), QStringLiteral("中水平（L9，逗号，可空）"), QStringLiteral("")},
             {QStringLiteral("randomize"), QStringLiteral("随机化"), QStringLiteral("false")},
             {QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("1")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "Taguchi 正交设计";
                c.chart_type = "taguchi_orthogonal_design";
                const QString factors_text =
                    QString::fromStdString(normalize(d.line_text("factors")));
                const QStringList names = factors_text.split(QLatin1Char(','), Qt::SkipEmptyParts);
                for (const QString& name : names) {
                    c.taguchi_orthogonal.factor_names.push_back(name.trimmed().toStdString());
                }
                if (c.taguchi_orthogonal.factor_names.empty()) {
                    return apply_error(QStringLiteral("因子不足"),
                                       QStringLiteral("请提供至少一个因子名称。"));
                }
                const std::string array = normalize(d.line_text("array"));
                if (array == "l9" || array == "9") {
                    c.taguchi_orthogonal.array = "L9";
                } else if (array == "l12" || array == "12") {
                    c.taguchi_orthogonal.array = "L12";
                } else {
                    c.taguchi_orthogonal.array = "L8";
                }
                const QStringList lows =
                    QString::fromStdString(normalize(d.line_text("low")))
                        .split(QLatin1Char(','), Qt::KeepEmptyParts);
                const QStringList highs =
                    QString::fromStdString(normalize(d.line_text("high")))
                        .split(QLatin1Char(','), Qt::KeepEmptyParts);
                const QStringList mids =
                    QString::fromStdString(normalize(d.line_text("mid")))
                        .split(QLatin1Char(','), Qt::KeepEmptyParts);
                for (std::size_t i = 0; i < c.taguchi_orthogonal.factor_names.size(); ++i) {
                    c.taguchi_orthogonal.low_levels.push_back(
                        i < static_cast<std::size_t>(lows.size())
                                && !lows[static_cast<int>(i)].trimmed().isEmpty()
                            ? lows[static_cast<int>(i)].trimmed().toStdString() : "-1");
                    c.taguchi_orthogonal.high_levels.push_back(
                        i < static_cast<std::size_t>(highs.size())
                                && !highs[static_cast<int>(i)].trimmed().isEmpty()
                            ? highs[static_cast<int>(i)].trimmed().toStdString() : "+1");
                    if (i < static_cast<std::size_t>(mids.size())
                        && !mids[static_cast<int>(i)].trimmed().isEmpty()) {
                        c.taguchi_orthogonal.mid_levels.push_back(
                            mids[static_cast<int>(i)].trimmed().toStdString());
                    } else {
                        c.taguchi_orthogonal.mid_levels.push_back("0");
                    }
                }
                const std::string randomize = normalize(d.line_text("randomize"));
                c.taguchi_orthogonal.randomize =
                    randomize == "true" || randomize == "1" || randomize == "yes";
                c.taguchi_orthogonal.random_seed = static_cast<std::uint64_t>(
                    std::max(0, d.line_int("seed").value_or(1)));
                return {};
            },
            AnalysisService::taguchi_orthogonal_design,
            QStringLiteral("DOE")},
        {
            QStringLiteral("taguchi_analyze"),
            QStringLiteral("Taguchi 分析"),
            QStringLiteral("Analyze Taguchi Design"),
            QStringLiteral("统计"),
            QStringLiteral("doe_factorial"),
            false, true,
            {{QStringLiteral("factors"), QStringLiteral("因子"), true, false},
             {QStringLiteral("responses"), QStringLiteral("响应"), true, false}},
            {{QStringLiteral("sn_type"), QStringLiteral("S/N 类型 larger/smaller/nominal"),
              QStringLiteral("larger")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                const auto factors = d.role_indices("factors");
                const auto responses = d.role_indices("responses");
                if (factors.empty() || responses.empty()) {
                    return apply_error(QStringLiteral("列不足"),
                                       QStringLiteral("请选择因子列与响应列。"));
                }
                c.analysis_name = "Taguchi 分析";
                c.chart_type = "taguchi_analyze";
                for (int col : factors) {
                    c.taguchi_analyze.factor_columns.push_back(
                        static_cast<std::size_t>(col));
                }
                for (int col : responses) {
                    c.taguchi_analyze.response_columns.push_back(
                        static_cast<std::size_t>(col));
                }
                const std::string sn = normalize(d.line_text("sn_type"));
                c.taguchi_analyze.sn_type = sn.empty() ? "larger" : sn;
                return {};
            },
            AnalysisService::taguchi_analyze,
            QStringLiteral("DOE")},
        {
            QStringLiteral("mixture_design"),
            QStringLiteral("Mixture 设计"),
            QStringLiteral("Mixture Design"),
            QStringLiteral("统计"),
            QStringLiteral("doe_factorial"),
            false, false,
            {},
            {{QStringLiteral("q"), QStringLiteral("分量数 q（3/4）"), QStringLiteral("3")},
             {QStringLiteral("names"), QStringLiteral("分量名（逗号）"), QStringLiteral("x1,x2,x3")},
             {QStringLiteral("randomize"), QStringLiteral("随机化"), QStringLiteral("false")},
             {QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("1")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                c.analysis_name = "Mixture 设计";
                c.chart_type = "mixture_design";
                const int q = d.line_int("q").value_or(3);
                if (q < 3 || q > 4) {
                    return apply_error(QStringLiteral("q 无效"),
                                       QStringLiteral("分量数 q 仅支持 3 或 4。"));
                }
                c.mixture_design.component_count = static_cast<std::size_t>(q);
                const QStringList names =
                    QString::fromStdString(normalize(d.line_text("names")))
                        .split(QLatin1Char(','), Qt::SkipEmptyParts);
                for (const QString& name : names) {
                    c.mixture_design.component_names.push_back(
                        name.trimmed().toStdString());
                }
                const std::string randomize = normalize(d.line_text("randomize"));
                c.mixture_design.randomize =
                    randomize == "true" || randomize == "1" || randomize == "yes";
                c.mixture_design.random_seed = static_cast<std::uint64_t>(
                    std::max(0, d.line_int("seed").value_or(1)));
                return {};
            },
            AnalysisService::mixture_design,
            QStringLiteral("DOE")},
        {
            QStringLiteral("mixture_analyze"),
            QStringLiteral("Mixture 分析"),
            QStringLiteral("Analyze Mixture Design"),
            QStringLiteral("统计"),
            QStringLiteral("doe_factorial"),
            false, true,
            {{QStringLiteral("components"), QStringLiteral("分量列"), true, false},
             {QStringLiteral("response"), QStringLiteral("响应"), true, false}},
            {{QStringLiteral("model_order"), QStringLiteral("linear/quadratic"),
              QStringLiteral("linear")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                const auto components = d.role_indices("components");
                const auto responses = d.role_indices("response");
                if (components.size() < 2 || responses.empty()) {
                    return apply_error(QStringLiteral("列不足"),
                                       QStringLiteral("需要至少两个分量列与一个响应列。"));
                }
                c.chart_type = "mixture_analyze";
                for (int col : components) {
                    c.mixture_analyze.component_columns.push_back(
                        static_cast<std::size_t>(col));
                }
                c.mixture_analyze.response_column =
                    static_cast<std::size_t>(responses.front());
                const std::string order = normalize(d.line_text("model_order"));
                c.mixture_analyze.model_order = order.empty() ? "linear" : order;
                return {};
            },
            AnalysisService::mixture_analyze,
            QStringLiteral("DOE")},
        {
            QStringLiteral("glm_two_way"),
            QStringLiteral("双因子 GLM"),
            QStringLiteral("GLM Two-Way"),
            QStringLiteral("统计"),
            QStringLiteral("anova_two_way"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应"), true, false},
             {QStringLiteral("factor_a"), QStringLiteral("因子 A"), true, false},
             {QStringLiteral("factor_b"), QStringLiteral("因子 B"), true, false}},
            {{QStringLiteral("interaction"), QStringLiteral("含交互"), QStringLiteral("true")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                const auto response = d.role_indices("response");
                const auto fa = d.role_indices("factor_a");
                const auto fb = d.role_indices("factor_b");
                if (response.empty() || fa.empty() || fb.empty()) {
                    return apply_error(QStringLiteral("列不足"),
                                       QStringLiteral("需要响应列与两个因子列。"));
                }
                c.chart_type = "glm_two_way";
                c.glm_two_way.response_column = static_cast<std::size_t>(response.front());
                c.glm_two_way.factor_a_column = static_cast<std::size_t>(fa.front());
                c.glm_two_way.factor_b_column = static_cast<std::size_t>(fb.front());
                const std::string inter = normalize(d.line_text("interaction"));
                c.glm_two_way.include_interaction =
                    inter.empty() || inter == "true" || inter == "1" || inter == "yes";
                return {};
            },
            AnalysisService::glm_two_way,
            QStringLiteral("ANOVA")},
        {
            QStringLiteral("analyze_variability"),
            QStringLiteral("Analyze Variability"),
            QStringLiteral("Analyze Variability"),
            QStringLiteral("统计"),
            QStringLiteral("doe_factorial"),
            false, true,
            {{QStringLiteral("factors"), QStringLiteral("因子列"), true, false},
             {QStringLiteral("replicates"), QStringLiteral("重复列"), true, false}},
            {{QStringLiteral("method"), QStringLiteral("估计方法"), QStringLiteral("lse")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                const auto factors = d.role_indices("factors");
                const auto replicates = d.role_indices("replicates");
                if (factors.empty() || replicates.size() < 2) {
                    return apply_error(QStringLiteral("列不足"),
                                       QStringLiteral("需要因子列与至少两个重复列。"));
                }
                c.chart_type = "analyze_variability";
                for (int col : factors) {
                    c.analyze_variability.factor_columns.push_back(
                        static_cast<std::size_t>(col));
                }
                for (int col : replicates) {
                    c.analyze_variability.replicate_columns.push_back(
                        static_cast<std::size_t>(col));
                }
                const std::string method = normalize(d.line_text("method"));
                c.analyze_variability.estimation_method =
                    method.empty() ? "lse" : method;
                return {};
            },
            AnalysisService::analyze_variability,
            QStringLiteral("DOE")},
        {
            QStringLiteral("factor_analysis"),
            QStringLiteral("因子分析"),
            QStringLiteral("Factor Analysis"),
            QStringLiteral("统计"),
            QStringLiteral("pca"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量列"), true, false}},
            {{QStringLiteral("factors"), QStringLiteral("因子数（0=Kaiser）"), QStringLiteral("0")},
             {QStringLiteral("varimax"), QStringLiteral("Varimax 旋转"), QStringLiteral("false")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                const auto variables = d.role_indices("variables");
                if (variables.size() < 3) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("至少需要三个数值变量。"));
                }
                c.chart_type = "factor_analysis";
                for (int col : variables) {
                    c.factor_analysis.variable_columns.push_back(
                        static_cast<std::size_t>(col));
                }
                c.factor_analysis.factor_count = static_cast<std::size_t>(
                    std::max(0, d.line_int("factors").value_or(0)));
                c.factor_analysis.use_kaiser_rule = true;
                const std::string varimax = normalize(d.line_text("varimax"));
                c.factor_analysis.varimax_rotation =
                    varimax == "true" || varimax == "1" || varimax == "yes";
                return {};
            },
            AnalysisService::factor_analysis,
            QStringLiteral("多变量")},
        {
            QStringLiteral("binary_response_doe"),
            QStringLiteral("二值响应 DOE"),
            QStringLiteral("Binary Response DOE"),
            QStringLiteral("统计"),
            QStringLiteral("doe_factorial"),
            false, true,
            {{QStringLiteral("factors"), QStringLiteral("因子列"), true, false},
             {QStringLiteral("events"), QStringLiteral("Events"), false, false},
             {QStringLiteral("trials"), QStringLiteral("Trials"), false, false},
             {QStringLiteral("binary"), QStringLiteral("Binary 0/1"), false, false}},
            {{QStringLiteral("interaction"), QStringLiteral("A×B 交互"), QStringLiteral("true")},
             {QStringLiteral("layout"), QStringLiteral("events_trials|binary"), QStringLiteral("events_trials")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                const auto factors = d.role_indices("factors");
                if (factors.empty()) {
                    return apply_error(QStringLiteral("列不足"),
                                       QStringLiteral("需要至少一个因子列。"));
                }
                c.chart_type = "binary_response_doe";
                for (int col : factors) {
                    c.binary_response_doe.factor_columns.push_back(
                        static_cast<std::size_t>(col));
                }
                const std::string layout = normalize(d.line_text("layout"));
                c.binary_response_doe.use_events_trials =
                    layout.empty() || layout == "events_trials";
                if (c.binary_response_doe.use_events_trials) {
                    const auto events = d.role_indices("events");
                    const auto trials = d.role_indices("trials");
                    if (events.empty() || trials.empty()) {
                        return apply_error(QStringLiteral("列不足"),
                                           QStringLiteral("events/trials 模式需要两列。"));
                    }
                    c.binary_response_doe.events_column =
                        static_cast<std::size_t>(events.front());
                    c.binary_response_doe.trials_column =
                        static_cast<std::size_t>(trials.front());
                } else {
                    const auto binary = d.role_indices("binary");
                    if (binary.empty()) {
                        return apply_error(QStringLiteral("列不足"),
                                           QStringLiteral("0/1 模式需要响应列。"));
                    }
                    c.binary_response_doe.binary_column =
                        static_cast<std::size_t>(binary.front());
                }
                const std::string inter = normalize(d.line_text("interaction"));
                c.binary_response_doe.include_ab_interaction =
                    inter.empty() || inter == "true" || inter == "1" || inter == "yes";
                return {};
            },
            AnalysisService::binary_response_doe,
            QStringLiteral("DOE")},
        {
            QStringLiteral("cluster_variables"),
            QStringLiteral("变量聚类"),
            QStringLiteral("Cluster Variables"),
            QStringLiteral("统计"),
            QStringLiteral("correlation"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量列"), true, false}},
            {{QStringLiteral("linkage"), QStringLiteral("连结"), QStringLiteral("complete")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                const auto variables = d.role_indices("variables");
                if (variables.size() < 3) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("至少需要三个数值变量。"));
                }
                c.chart_type = "cluster_variables";
                for (int col : variables) {
                    c.cluster_variables.variable_columns.push_back(
                        static_cast<std::size_t>(col));
                }
                const std::string linkage = normalize(d.line_text("linkage"));
                c.cluster_variables.linkage = linkage.empty() ? "complete" : linkage;
                return {};
            },
            AnalysisService::cluster_variables,
            QStringLiteral("多变量")},
        {
            QStringLiteral("glm_three_factor"),
            QStringLiteral("三因子 GLM"),
            QStringLiteral("GLM Three-Factor"),
            QStringLiteral("统计"),
            QStringLiteral("anova_two_way"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应"), true, false},
             {QStringLiteral("factor_a"), QStringLiteral("因子 A"), true, false},
             {QStringLiteral("factor_b"), QStringLiteral("因子 B"), true, false},
             {QStringLiteral("factor_c"), QStringLiteral("因子 C"), true, false}},
            {{QStringLiteral("ab"), QStringLiteral("A×B"), QStringLiteral("true")},
             {QStringLiteral("ac"), QStringLiteral("A×C"), QStringLiteral("true")},
             {QStringLiteral("bc"), QStringLiteral("B×C"), QStringLiteral("true")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                const auto response = d.role_indices("response");
                const auto fa = d.role_indices("factor_a");
                const auto fb = d.role_indices("factor_b");
                const auto fc = d.role_indices("factor_c");
                if (response.empty() || fa.empty() || fb.empty() || fc.empty()) {
                    return apply_error(QStringLiteral("列不足"),
                                       QStringLiteral("需要响应与三个因子列。"));
                }
                c.chart_type = "glm_three_factor";
                c.glm_three_factor.response_column =
                    static_cast<std::size_t>(response.front());
                c.glm_three_factor.factor_a_column =
                    static_cast<std::size_t>(fa.front());
                c.glm_three_factor.factor_b_column =
                    static_cast<std::size_t>(fb.front());
                c.glm_three_factor.factor_c_column =
                    static_cast<std::size_t>(fc.front());
                auto flag = [&](const char* key) {
                    const std::string value = normalize(d.line_text(key));
                    return value.empty() || value == "true" || value == "1"
                        || value == "yes";
                };
                c.glm_three_factor.include_ab_interaction = flag("ab");
                c.glm_three_factor.include_ac_interaction = flag("ac");
                c.glm_three_factor.include_bc_interaction = flag("bc");
                return {};
            },
            AnalysisService::glm_three_factor,
            QStringLiteral("ANOVA")},
        {
            QStringLiteral("life_data_regression"),
            QStringLiteral("寿命数据回归"),
            QStringLiteral("Life Data Regression"),
            QStringLiteral("统计"),
            QStringLiteral("reliability"),
            false, true,
            {{QStringLiteral("time"), QStringLiteral("时间"), true, false},
             {QStringLiteral("censor"), QStringLiteral("删失"), true, false},
             {QStringLiteral("covariates"), QStringLiteral("协变量"), true, false}},
            {{QStringLiteral("percentiles"), QStringLiteral("百分位"), QStringLiteral("1,5")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                const auto time = d.role_indices("time");
                const auto censor = d.role_indices("censor");
                const auto covariates = d.role_indices("covariates");
                if (time.empty() || censor.empty() || covariates.empty()) {
                    return apply_error(QStringLiteral("列不足"),
                                       QStringLiteral("需要时间、删失与协变量列。"));
                }
                if (covariates.size() > 2) {
                    return apply_error(QStringLiteral("协变量过多"),
                                       QStringLiteral("窄化实现最多 2 个协变量。"));
                }
                c.chart_type = "life_data_regression";
                c.life_data_regression.time_column =
                    static_cast<std::size_t>(time.front());
                c.life_data_regression.censor_column =
                    static_cast<std::size_t>(censor.front());
                for (int col : covariates) {
                    c.life_data_regression.covariate_columns.push_back(
                        static_cast<std::size_t>(col));
                }
                c.life_data_regression.distribution = "weibull";
                c.life_data_regression.percentile_levels = {1.0, 5.0};
                return {};
            },
            AnalysisService::life_data_regression,
            QStringLiteral("可靠性")},
        {
            QStringLiteral("doe_ccd"),
            QStringLiteral("中心复合设计 CCD"),
            QStringLiteral("Central Composite Design"),
            QStringLiteral("统计"),
            QStringLiteral("doe_factorial"),
            false, false,
            {},
            {{QStringLiteral("factor_ids"), QStringLiteral("因素 ID（逗号）"), QStringLiteral("A,B")},
             {QStringLiteral("factor_names"), QStringLiteral("因素名（逗号，可空）"), QStringLiteral("Temperature,Pressure")},
             {QStringLiteral("units"), QStringLiteral("单位（逗号，可空）"), QStringLiteral("C,kPa")},
             {QStringLiteral("low"), QStringLiteral("低水平（逗号数值）"), QStringLiteral("60,100")},
             {QStringLiteral("high"), QStringLiteral("高水平（逗号数值）"), QStringLiteral("80,140")},
             {QStringLiteral("centers"), QStringLiteral("中心（逗号，可空=中点）"), QStringLiteral("")},
             {QStringLiteral("variant"), QStringLiteral("变体 ccf/cci/ccc"), QStringLiteral("ccf")},
             {QStringLiteral("centers_n"), QStringLiteral("中心点数"), QStringLiteral("1")},
             {QStringLiteral("allow_beyond"), QStringLiteral("允许超范围星点(CCC)"), QStringLiteral("false")},
             {QStringLiteral("randomize"), QStringLiteral("随机化"), QStringLiteral("true")},
             {QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("42")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "中心复合设计";
                c.chart_type = "doe_ccd";
                auto& cfg = c.response_surface_design;
                cfg = {};
                cfg.design_kind = "ccd";
                const QString variant =
                    QString::fromStdString(normalize(d.line_text("variant"))).trimmed().toLower();
                cfg.ccd_variant = (variant == QLatin1String("ccc") || variant == QLatin1String("cci")
                                       || variant == QLatin1String("ccf"))
                    ? variant.toStdString()
                    : "ccf";
                const QStringList ids =
                    QString::fromStdString(normalize(d.line_text("factor_ids")))
                        .split(QLatin1Char(','), Qt::SkipEmptyParts);
                const QStringList names =
                    QString::fromStdString(normalize(d.line_text("factor_names")))
                        .split(QLatin1Char(','), Qt::KeepEmptyParts);
                const QStringList units =
                    QString::fromStdString(normalize(d.line_text("units")))
                        .split(QLatin1Char(','), Qt::KeepEmptyParts);
                const QStringList lows =
                    QString::fromStdString(normalize(d.line_text("low")))
                        .split(QLatin1Char(','), Qt::SkipEmptyParts);
                const QStringList highs =
                    QString::fromStdString(normalize(d.line_text("high")))
                        .split(QLatin1Char(','), Qt::SkipEmptyParts);
                const QString centers_text =
                    QString::fromStdString(normalize(d.line_text("centers"))).trimmed();
                const QStringList centers = centers_text.isEmpty()
                    ? QStringList{}
                    : centers_text.split(QLatin1Char(','), Qt::KeepEmptyParts);
                if (ids.isEmpty()) {
                    return apply_error(QStringLiteral("因素不足"),
                                       QStringLiteral("请提供至少两个因素 ID。"));
                }
                for (int i = 0; i < ids.size(); ++i) {
                    cfg.factor_ids.push_back(ids[i].trimmed().toStdString());
                    cfg.factor_names.push_back(
                        i < names.size() && !names[i].trimmed().isEmpty()
                            ? names[i].trimmed().toStdString()
                            : ids[i].trimmed().toStdString());
                    cfg.factor_units.push_back(
                        i < units.size() ? units[i].trimmed().toStdString() : std::string{});
                    bool ok_low = false;
                    bool ok_high = false;
                    const double low = i < lows.size() ? lows[i].trimmed().toDouble(&ok_low) : -1.0;
                    const double high =
                        i < highs.size() ? highs[i].trimmed().toDouble(&ok_high) : 1.0;
                    if (!ok_low || !ok_high) {
                        return apply_error(QStringLiteral("水平无效"),
                                           QStringLiteral("低/高水平必须为数值。"));
                    }
                    cfg.low_levels.push_back(low);
                    cfg.high_levels.push_back(high);
                    if (!centers_text.isEmpty()) {
                        if (i < centers.size() && !centers[i].trimmed().isEmpty()) {
                            bool ok_c = false;
                            const double center = centers[i].trimmed().toDouble(&ok_c);
                            if (!ok_c) {
                                return apply_error(QStringLiteral("中心无效"),
                                                   QStringLiteral("中心值必须为数值。"));
                            }
                            cfg.centers.push_back(center);
                        } else {
                            cfg.centers.push_back(0.5 * (low + high));
                        }
                    }
                }
                cfg.center_point_count = static_cast<std::size_t>(
                    std::max(0, d.line_int("centers_n").value_or(1)));
                const std::string allow = normalize(d.line_text("allow_beyond"));
                cfg.allow_beyond_range =
                    allow == "true" || allow == "1" || allow == "yes";
                const std::string randomize = normalize(d.line_text("randomize"));
                cfg.randomize =
                    randomize.empty() || randomize == "true" || randomize == "1"
                    || randomize == "yes";
                cfg.random_seed = static_cast<std::uint64_t>(
                    std::max(0, d.line_int("seed").value_or(42)));
                return {};
            },
            AnalysisService::doe_response_surface_design,
            QStringLiteral("DOE")},
        {
            QStringLiteral("doe_bbd"),
            QStringLiteral("Box–Behnken 设计"),
            QStringLiteral("Box-Behnken Design"),
            QStringLiteral("统计"),
            QStringLiteral("doe_factorial"),
            false, false,
            {},
            {{QStringLiteral("factor_ids"), QStringLiteral("因素 ID（逗号，≥3）"), QStringLiteral("X1,X2,X3")},
             {QStringLiteral("factor_names"), QStringLiteral("因素名（逗号，可空）"), QStringLiteral("Factor1,Factor2,Factor3")},
             {QStringLiteral("units"), QStringLiteral("单位（逗号，可空）"), QStringLiteral("1,1,1")},
             {QStringLiteral("low"), QStringLiteral("低水平（逗号数值）"), QStringLiteral("-1,-1,-1")},
             {QStringLiteral("high"), QStringLiteral("高水平（逗号数值）"), QStringLiteral("1,1,1")},
             {QStringLiteral("centers"), QStringLiteral("中心（逗号，可空=中点）"), QStringLiteral("")},
             {QStringLiteral("centers_n"), QStringLiteral("中心点数"), QStringLiteral("1")},
             {QStringLiteral("randomize"), QStringLiteral("随机化"), QStringLiteral("true")},
             {QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("7")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "Box–Behnken 设计";
                c.chart_type = "doe_bbd";
                auto& cfg = c.response_surface_design;
                cfg = {};
                cfg.design_kind = "bbd";
                cfg.ccd_variant.clear();
                const QStringList ids =
                    QString::fromStdString(normalize(d.line_text("factor_ids")))
                        .split(QLatin1Char(','), Qt::SkipEmptyParts);
                const QStringList names =
                    QString::fromStdString(normalize(d.line_text("factor_names")))
                        .split(QLatin1Char(','), Qt::KeepEmptyParts);
                const QStringList units =
                    QString::fromStdString(normalize(d.line_text("units")))
                        .split(QLatin1Char(','), Qt::KeepEmptyParts);
                const QStringList lows =
                    QString::fromStdString(normalize(d.line_text("low")))
                        .split(QLatin1Char(','), Qt::SkipEmptyParts);
                const QStringList highs =
                    QString::fromStdString(normalize(d.line_text("high")))
                        .split(QLatin1Char(','), Qt::SkipEmptyParts);
                const QString centers_text =
                    QString::fromStdString(normalize(d.line_text("centers"))).trimmed();
                const QStringList centers = centers_text.isEmpty()
                    ? QStringList{}
                    : centers_text.split(QLatin1Char(','), Qt::KeepEmptyParts);
                if (ids.size() < 3) {
                    return apply_error(QStringLiteral("因素不足"),
                                       QStringLiteral("BBD 需要至少 3 个连续因素。"));
                }
                for (int i = 0; i < ids.size(); ++i) {
                    cfg.factor_ids.push_back(ids[i].trimmed().toStdString());
                    cfg.factor_names.push_back(
                        i < names.size() && !names[i].trimmed().isEmpty()
                            ? names[i].trimmed().toStdString()
                            : ids[i].trimmed().toStdString());
                    cfg.factor_units.push_back(
                        i < units.size() ? units[i].trimmed().toStdString() : std::string{});
                    bool ok_low = false;
                    bool ok_high = false;
                    const double low = i < lows.size() ? lows[i].trimmed().toDouble(&ok_low) : -1.0;
                    const double high =
                        i < highs.size() ? highs[i].trimmed().toDouble(&ok_high) : 1.0;
                    if (!ok_low || !ok_high) {
                        return apply_error(QStringLiteral("水平无效"),
                                           QStringLiteral("低/高水平必须为数值。"));
                    }
                    cfg.low_levels.push_back(low);
                    cfg.high_levels.push_back(high);
                    if (!centers_text.isEmpty()) {
                        if (i < centers.size() && !centers[i].trimmed().isEmpty()) {
                            bool ok_c = false;
                            const double center = centers[i].trimmed().toDouble(&ok_c);
                            if (!ok_c) {
                                return apply_error(QStringLiteral("中心无效"),
                                                   QStringLiteral("中心值必须为数值。"));
                            }
                            cfg.centers.push_back(center);
                        } else {
                            cfg.centers.push_back(0.5 * (low + high));
                        }
                    }
                }
                cfg.center_point_count = static_cast<std::size_t>(
                    std::max(0, d.line_int("centers_n").value_or(1)));
                const std::string randomize = normalize(d.line_text("randomize"));
                cfg.randomize =
                    randomize.empty() || randomize == "true" || randomize == "1"
                    || randomize == "yes";
                cfg.random_seed = static_cast<std::uint64_t>(
                    std::max(0, d.line_int("seed").value_or(7)));
                return {};
            },
            AnalysisService::doe_response_surface_design,
            QStringLiteral("DOE")},
        {
            QStringLiteral("reliability"),
            QStringLiteral("可靠性分析（Kaplan-Meier / Weibull）"),
            QStringLiteral("Reliability Analysis"),
            QStringLiteral("统计"),
            QStringLiteral("report"),
            false, true,
            {{QStringLiteral("time"), QStringLiteral("寿命/时间"), false, false},
             {QStringLiteral("event"), QStringLiteral("失效指示（1=失效，0=删失；可与删失类型列二选一）"), false, true},
             {QStringLiteral("censor_type"), QStringLiteral("删失类型列（exact/right/left/interval，可选）"), false, true},
             {QStringLiteral("interval_left"), QStringLiteral("区间左界列（interval 行必填）"), false, true},
             {QStringLiteral("interval_right"), QStringLiteral("区间右界列（interval 行必填）"), false, true},
             {QStringLiteral("exposure"), QStringLiteral("暴露量列（可选，列求和）"), false, true},
             {QStringLiteral("failure_mode"), QStringLiteral("失效模式列（可选）"), false, true},
             {QStringLiteral("group"), QStringLiteral("分组列（Log-rank / Fine-Gray 二分类，可选）"), false, true},
             {QStringLiteral("covariate"), QStringLiteral("协变量列（Fine-Gray 连续/多列，可选；≥2 列走多协变量 IPCW，优先于二分类）"), true, true}},
            {{QStringLiteral("model"), QStringLiteral("模型"),
              QStringLiteral("kaplan_meier / weibull / weibull3 / exponential / exponential2 / lognormal / lognormal3")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int time = d.first_role_index("time");
                const int event = d.first_role_index("event");
                const int censor_type = d.first_role_index("censor_type");
                if (time < 0 || (event < 0 && censor_type < 0)) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择寿命列，以及失效指示列或删失类型列。"));
                }
                c.analysis_name = "Reliability Analysis";
                c.chart_type = "reliability";
                c.reliability.time_column = static_cast<std::size_t>(time);
                if (event >= 0) {
                    c.reliability.event_column = static_cast<std::size_t>(event);
                }
                if (censor_type >= 0) {
                    c.reliability.censoring_type_column = static_cast<std::size_t>(censor_type);
                }
                const int interval_left = d.first_role_index("interval_left");
                const int interval_right = d.first_role_index("interval_right");
                if ((interval_left >= 0) != (interval_right >= 0)) {
                    return apply_error(
                        QStringLiteral("区间界列不完整"),
                        QStringLiteral("区间左界与右界列必须同时指定。"));
                }
                if (interval_left >= 0) {
                    c.reliability.interval_left_column =
                        static_cast<std::size_t>(interval_left);
                    c.reliability.interval_right_column =
                        static_cast<std::size_t>(interval_right);
                }
                const int exposure = d.first_role_index("exposure");
                if (exposure >= 0) {
                    c.reliability.exposure_column = static_cast<std::size_t>(exposure);
                }
                const int failure_mode = d.first_role_index("failure_mode");
                if (failure_mode >= 0) {
                    c.reliability.failure_mode_column = static_cast<std::size_t>(failure_mode);
                }
                const int group = d.first_role_index("group");
                if (group >= 0) {
                    c.reliability.group_column = static_cast<std::size_t>(group);
                }
                c.reliability.covariate_columns.clear();
                c.reliability.covariate_column.reset();
                for (const int covariate : d.role_indices("covariate")) {
                    if (covariate < 0) {
                        continue;
                    }
                    c.reliability.covariate_columns.push_back(
                        static_cast<std::size_t>(covariate));
                }
                if (c.reliability.covariate_columns.size() == 1) {
                    c.reliability.covariate_column = c.reliability.covariate_columns.front();
                } else if (c.reliability.covariate_columns.size() > 5) {
                    return apply_error(
                        QStringLiteral("协变量过多"),
                        QStringLiteral("Fine-Gray 多协变量最多 5 列。"));
                }
                c.reliability.model = normalize(d.line_text("model"));
                if (c.reliability.model != "weibull"
                    && c.reliability.model != "weibull3"
                    && c.reliability.model != "exponential"
                    && c.reliability.model != "exponential2"
                    && c.reliability.model != "lognormal"
                    && c.reliability.model != "lognormal3") {
                    c.reliability.model = "kaplan_meier";
                }
                return {};
            },
            AnalysisService::reliability,
            QStringLiteral("可靠性")},
        {
            QStringLiteral("accelerated_life"),
            QStringLiteral("加速寿命分析"),
            QStringLiteral("Accelerated Life Testing"),
            QStringLiteral("统计"),
            QStringLiteral("report"),
            false, true,
            {{QStringLiteral("time"), QStringLiteral("寿命"), false, false},
             {QStringLiteral("event"), QStringLiteral("失效/删失"), false, false},
             {QStringLiteral("stress"), QStringLiteral("应力（摄氏温度）"), false, false}},
            {{QStringLiteral("use_stress"), QStringLiteral("使用应力（°C）"), QStringLiteral("25")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int time = d.first_role_index("time");
                const int event = d.first_role_index("event");
                const int stress = d.first_role_index("stress");
                if (time < 0 || event < 0 || stress < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择寿命、失效指示与应力列。"));
                }
                c.analysis_name = "加速寿命分析";
                c.chart_type = "accelerated_life";
                c.accelerated_life.time_column = static_cast<std::size_t>(time);
                c.accelerated_life.event_column = static_cast<std::size_t>(event);
                c.accelerated_life.stress_column = static_cast<std::size_t>(stress);
                c.accelerated_life.use_stress_celsius =
                    d.line_number("use_stress").value_or(25.0);
                return {};
            },
            AnalysisService::accelerated_life,
            QStringLiteral("可靠性")},
        {
            QStringLiteral("reliability_warranty"),
            QStringLiteral("保修摘要"),
            QStringLiteral("Warranty Summary"),
            QStringLiteral("统计"),
            QStringLiteral("report"),
            false, true,
            {{QStringLiteral("exposure_col"), QStringLiteral("暴露量列（可选，列求和优先于标量）"), false, true}},
            {{QStringLiteral("warranty_time"), QStringLiteral("保修窗口 T_w"), QStringLiteral("1000")},
             {QStringLiteral("time_unit"), QStringLiteral("时间单位"), QStringLiteral("hours")},
             {QStringLiteral("exposure"), QStringLiteral("暴露量标量（无列时使用）"), QStringLiteral("1000")},
             {QStringLiteral("reliability"), QStringLiteral("R(T_w)"), QStringLiteral("0.98")},
             {QStringLiteral("model"), QStringLiteral("模型标签"), QStringLiteral("weibull")},
             {QStringLiteral("observed_failures"), QStringLiteral("观察失效数"), QStringLiteral("0")},
             {QStringLiteral("censored"), QStringLiteral("删失数"), QStringLiteral("0")},
             {QStringLiteral("valid"), QStringLiteral("有效观测数"), QStringLiteral("0")},
             {QStringLiteral("prediction"), QStringLiteral("是否预测口径 true/false"), QStringLiteral("true")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "保修摘要";
                c.chart_type = "reliability_warranty";
                c.reliability.warranty_time = d.line_number("warranty_time").value_or(0.0);
                c.reliability.time_unit = normalize(d.line_text("time_unit"));
                c.reliability.exposure = d.line_number("exposure").value_or(0.0);
                c.reliability.reliability_at_warranty =
                    d.line_number("reliability").value_or(-1.0);
                c.reliability.model = normalize(d.line_text("model"));
                if (c.reliability.model.empty()) {
                    c.reliability.model = "external_reliability";
                }
                c.reliability.warranty_observed_failures = static_cast<std::size_t>(
                    std::max(0, d.line_int("observed_failures").value_or(0)));
                c.reliability.warranty_censored_count = static_cast<std::size_t>(
                    std::max(0, d.line_int("censored").value_or(0)));
                c.reliability.warranty_valid_count = static_cast<std::size_t>(
                    std::max(0, d.line_int("valid").value_or(0)));
                const std::string prediction = normalize(d.line_text("prediction"));
                c.reliability.reliability_is_prediction =
                    prediction.empty() || prediction == "true" || prediction == "1"
                    || prediction == "yes";
                const int exposure_col = d.first_role_index("exposure_col");
                if (exposure_col >= 0) {
                    c.reliability.exposure_column = static_cast<std::size_t>(exposure_col);
                }
                return {};
            },
            AnalysisService::reliability_warranty,
            QStringLiteral("可靠性")},
        {
            QStringLiteral("t_power"),
            QStringLiteral("t 功效与样本量"),
            QStringLiteral("t 功效与样本量"),
            QStringLiteral("统计"),
            QStringLiteral("one_sample_t"),
            false, false,
            {},
            {             {QStringLiteral("mode"), QStringLiteral("模式"),
              QStringLiteral("one_sample_sample_size / equivalence_one_sample_sample_size / "
                             "doe_factorial_power / tolerance_normal_sample_size")},
             {QStringLiteral("effect"), QStringLiteral("效应量 / 覆盖率 P / 等价半宽(σ单位)"), QStringLiteral("0.5")},
             {QStringLiteral("target"), QStringLiteral("目标功效或容差置信度"), QStringLiteral("0.8")},
             {QStringLiteral("alpha"), QStringLiteral("显著性水平"), QStringLiteral("0.05")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / greater / less")},
             {QStringLiteral("sample_size"), QStringLiteral("样本量（计算功效时，可逗号分隔）"), QString()},
             {QStringLiteral("effect_list"), QStringLiteral("效应量列表（可逗号分隔）"), QString()},
             {QStringLiteral("groups"), QStringLiteral("ANOVA 组数 / DOE 因子数 k"), QStringLiteral("3")},
             {QStringLiteral("null_proportion"), QStringLiteral("第一/假设比例或泊松率 / 容差 max k"),
              QStringLiteral("0.5")},
             {QStringLiteral("second_proportion"), QStringLiteral("第二/备择比例或比较率 λ1/λ2"),
              QStringLiteral("0.7")},
             {QStringLiteral("observation_length"), QStringLiteral("泊松观测长度 L"),
              QStringLiteral("1")},
             {QStringLiteral("doe_fraction_p"), QStringLiteral("DOE 部分析因 p（0=全因子）"),
              QStringLiteral("0")},
             {QStringLiteral("equivalence_difference"), QStringLiteral("等价真实差(σ单位)"),
              QStringLiteral("0")},
             {QStringLiteral("variance_method"), QStringLiteral("比例方差"),
              QStringLiteral("pooled / unpooled")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "t 功效与样本量";
                c.chart_type = "t_power";
                c.power.mode = normalize(d.line_text("mode"));
                if (c.power.mode.empty()) {
                    c.power.mode = "one_sample_sample_size";
                }
                c.power.effect_size = d.line_number("effect").value_or(0.5);
                c.power.target = d.line_number("target").value_or(0.8);
                c.power.alpha = d.line_number("alpha").value_or(0.05);
                c.power.sample_size = static_cast<std::size_t>(
                    std::max(0, d.line_int("sample_size").value_or(0)));
                c.power.sample_size_list = normalize(d.line_text("sample_size"));
                c.power.effect_size_list = normalize(d.line_text("effect_list"));
                if (c.power.effect_size_list.empty()) {
                    c.power.effect_size_list = normalize(d.line_text("effect"));
                }
                c.power.group_count = static_cast<std::size_t>(
                    std::max(0, d.line_int("groups").value_or(3)));
                c.power.null_proportion =
                    d.line_number("null_proportion").value_or(0.5);
                c.power.second_proportion =
                    d.line_number("second_proportion").value_or(0.7);
                c.power.observation_length =
                    d.line_number("observation_length").value_or(1.0);
                if (!(c.power.observation_length > 0.0)) {
                    c.power.observation_length = 1.0;
                }
                c.power.doe_fraction_p = static_cast<std::size_t>(
                    std::max(0, d.line_int("doe_fraction_p").value_or(0)));
                c.power.doe_replicates = std::max<std::size_t>(1, c.power.sample_size);
                c.power.equivalence_difference =
                    d.line_number("equivalence_difference").value_or(0.0);
                c.inference.alternative = normalize(d.line_text("alternative"));
                if (c.inference.alternative != "greater"
                    && c.inference.alternative != "less") {
                    c.inference.alternative = "two_sided";
                }
                c.power.variance_method = normalize(d.line_text("variance_method"));
                if (c.power.variance_method != "unpooled") {
                    c.power.variance_method = "pooled";
                }
                return {};
            },
            AnalysisService::t_power,
            QStringLiteral("功效与样本量")},
        {
            QStringLiteral("acf_pacf"),
            QStringLiteral("ACF/PACF"),
            QStringLiteral("自相关与偏自相关"),
            QStringLiteral("统计"),
            QStringLiteral("time_series_plot"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("序列"), false, false}},
            {{QStringLiteral("max_lag"), QStringLiteral("最大滞后（可空=自动）"), QStringLiteral("")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平（带宽）"), QStringLiteral("0.95")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "ACF/PACF";
                c.chart_type = "acf_pacf";
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择数值序列列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.graph.bin_count = d.line_int("max_lag").value_or(0);
                c.inference.confidence_level =
                    d.line_number("confidence").value_or(0.95);
                return {};
            },
            AnalysisService::acf_pacf,
            QStringLiteral("时间序列")},
        // ------------------------------------------------------------ 图形
        {
            QStringLiteral("histogram"),
            QStringLiteral("直方图"),
            QStringLiteral("直方图"),
            QStringLiteral("图形"),
            QStringLiteral("histogram"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false},
             {QStringLiteral("by"), QStringLiteral("分组变量"), false, true}},
            {{QStringLiteral("bins"), QStringLiteral("组数（可选）"), QStringLiteral("留空则自动")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.chart_type = "histogram";
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择连续变量列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.graph.bin_count = d.line_int("bins").value_or(0);
                if (d.first_role_index("by") >= 0) {
                    c.by_column = static_cast<std::size_t>(d.first_role_index("by"));
                    c.graph.by_column = c.by_column;
                }
                return {};
            },
            AnalysisService::histogram,
            QStringLiteral("分布与单变量")},
        {
            QStringLiteral("eda_4plot"),
            QStringLiteral("EDA 四图"),
            QStringLiteral("EDA 四图（NIST）"),
            QStringLiteral("图形"),
            QStringLiteral("histogram"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false}},
            {{QStringLiteral("bins"), QStringLiteral("直方图组数（可选）"), QStringLiteral("留空则自动")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "EDA 四图";
                c.chart_type = "eda_4plot";
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择连续变量列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.graph.bin_count = d.line_int("bins").value_or(0);
                return {};
            },
            AnalysisService::eda_4plot,
            QStringLiteral("时间 / 条形 / 其他")},
        {
            QStringLiteral("boxplot"),
            QStringLiteral("箱线图"),
            QStringLiteral("箱线图"),
            QStringLiteral("图形"),
            QStringLiteral("boxplot"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false},
             {QStringLiteral("by"), QStringLiteral("分类变量"), false, true}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.chart_type = "boxplot";
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择连续变量列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                if (d.first_role_index("by") >= 0) {
                    c.by_column = static_cast<std::size_t>(
                        d.first_role_index("by"));
                }
                return {};
            },
            AnalysisService::boxplot,
            QStringLiteral("分布与单变量")},
        {
            QStringLiteral("pareto"),
            QStringLiteral("柏拉图"),
            QStringLiteral("柏拉图"),
            QStringLiteral("质量工具"),
            QStringLiteral("pareto"),
            false, true,
            {{QStringLiteral("category"), QStringLiteral("缺陷类别"), false, false},
             {QStringLiteral("counts"), QStringLiteral("计数列"), false, true}},
            {{QStringLiteral("other_threshold"), QStringLiteral("Other 合并阈值（可选 %）"),
              QStringLiteral("例如 95；留空则不合并")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.chart_type = "pareto";
                const int column = d.first_role_index("category");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择缺陷类别列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                if (d.first_role_index("counts") >= 0) {
                    c.selection.defect_count_column = static_cast<std::size_t>(
                        d.first_role_index("counts"));
                }
                const std::optional<double> threshold =
                    d.line_number("other_threshold");
                if (threshold.has_value() && *threshold >= 0.0 && *threshold <= 100.0) {
                    c.pareto_other_threshold_percent = threshold;
                } else if (threshold.has_value()) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("Other 合并阈值必须在 0 到 100 之间。"));
                }
                return {};
            },
            AnalysisService::pareto,
            QStringLiteral("质量图 / 规划")},
        {
            QStringLiteral("run_chart"),
            QStringLiteral("运行图"),
            QStringLiteral("运行图"),
            QStringLiteral("质量工具"),
            QStringLiteral("time_series"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("数值观测（一列）"), false, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择一列数值观测。"));
                }
                c.analysis_name = "运行图";
                c.chart_type = "run_chart";
                c.variable_columns = {static_cast<std::size_t>(column)};
                return {};
            },
            AnalysisService::run_chart,
            QStringLiteral("质量图 / 规划")},
        {
            QStringLiteral("cause_and_effect"),
            QStringLiteral("因果图"),
            QStringLiteral("因果图（鱼骨）"),
            QStringLiteral("质量工具"),
            QStringLiteral("pareto"),
            false, true,
            {{QStringLiteral("category"), QStringLiteral("类别列"), false, false},
             {QStringLiteral("cause"), QStringLiteral("原因列"), false, false}},
            {{QStringLiteral("effect_title"), QStringLiteral("效应标题"),
              QStringLiteral("默认：效应")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int category = d.first_role_index("category");
                const int cause = d.first_role_index("cause");
                if (category < 0 || cause < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择类别列与原因列。"));
                }
                c.analysis_name = "因果图";
                c.chart_type = "cause_and_effect";
                c.variable_columns = {
                    static_cast<std::size_t>(category),
                    static_cast<std::size_t>(cause)};
                c.effect_title = d.line_text("effect_title");
                if (c.effect_title.empty()) {
                    c.effect_title = "效应";
                }
                return {};
            },
            AnalysisService::cause_and_effect,
            QStringLiteral("质量图 / 规划")},
        {
            QStringLiteral("density_plot"),
            QStringLiteral("密度图"),
            QStringLiteral("密度图"),
            QStringLiteral("图形"),
            QStringLiteral("histogram"),
            false, true,
            {{QStringLiteral("variable"), QStringLiteral("变量"), false, false},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "密度图";
                c.chart_type = "density_plot";
                c.graph.graph_kind = "density";
                const int column = d.first_role_index("variable");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择连续变量。"));
                }
                c.graph.x_column = static_cast<std::size_t>(column);
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("分布与单变量")},
        {
            QStringLiteral("hexbin_plot"),
            QStringLiteral("Hexbin"),
            QStringLiteral("Hexbin / 二维分箱"),
            QStringLiteral("图形"),
            QStringLiteral("scatter"),
            false, true,
            {{QStringLiteral("x_variable"), QStringLiteral("X 变量"), false, false},
             {QStringLiteral("y_variable"), QStringLiteral("Y 变量"), false, false},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("bins"), QStringLiteral("分箱数（0=自动）"), QStringLiteral("0")},
             {QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "Hexbin";
                c.chart_type = "hexbin_plot";
                c.graph.graph_kind = "hexbin";
                const int x = d.first_role_index("x_variable");
                const int y = d.first_role_index("y_variable");
                if (x < 0 || y < 0) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择 X 与 Y。"));
                }
                c.graph.x_column = static_cast<std::size_t>(x);
                c.graph.y_column = static_cast<std::size_t>(y);
                c.graph.bin_count = d.line_int("bins").value_or(0);
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("关系与多元")},
        {
            QStringLiteral("violin_plot"),
            QStringLiteral("小提琴图"),
            QStringLiteral("小提琴图"),
            QStringLiteral("图形"),
            QStringLiteral("boxplot"),
            false, true,
            {{QStringLiteral("variable"), QStringLiteral("响应"), false, false},
             {QStringLiteral("by"), QStringLiteral("分组"), false, true},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "小提琴图";
                c.chart_type = "violin_plot";
                c.graph.graph_kind = "violin";
                const int column = d.first_role_index("variable");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择响应变量。"));
                }
                c.graph.y_column = static_cast<std::size_t>(column);
                if (d.first_role_index("by") >= 0) {
                    c.graph.by_column =
                        static_cast<std::size_t>(d.first_role_index("by"));
                }
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("分布与单变量")},
        {
            QStringLiteral("bar_chart"),
            QStringLiteral("条形图"),
            QStringLiteral("条形图"),
            QStringLiteral("图形"),
            QStringLiteral("pareto"),
            false, true,
            {{QStringLiteral("category"), QStringLiteral("类别"), false, false},
             {QStringLiteral("weight"), QStringLiteral("权重/计数"), false, true},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "条形图";
                c.chart_type = "bar_chart";
                c.graph.graph_kind = "bar";
                const int category = d.first_role_index("category");
                if (category < 0) {
                    return apply_error(QStringLiteral("未选择类别"),
                                       QStringLiteral("请选择分类变量。"));
                }
                c.graph.x_column = static_cast<std::size_t>(category);
                if (d.first_role_index("weight") >= 0) {
                    c.graph.weight_column =
                        static_cast<std::size_t>(d.first_role_index("weight"));
                }
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("时间 / 条形 / 其他")},
        {
            QStringLiteral("scatter_plot"),
            QStringLiteral("散点图"),
            QStringLiteral("散点图"),
            QStringLiteral("图形"),
            QStringLiteral("scatter"),
            false, true,
            {{QStringLiteral("x_variable"), QStringLiteral("X 变量"), false, false},
             {QStringLiteral("y_variable"), QStringLiteral("Y 变量"), false, false},
             {QStringLiteral("by"), QStringLiteral("分组变量（图内着色）"), false, true},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true},
             {QStringLiteral("label"), QStringLiteral("标签变量"), false, true}},
            {{QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "散点图";
                c.chart_type = "scatter_plot";
                c.graph.graph_kind = "scatter";
                const int x = d.first_role_index("x_variable");
                const int y = d.first_role_index("y_variable");
                if (x < 0 || y < 0) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择 X 变量和 Y 变量。"));
                }
                c.graph.x_column = static_cast<std::size_t>(x);
                c.graph.y_column = static_cast<std::size_t>(y);
                if (d.first_role_index("by") >= 0) {
                    c.graph.by_column =
                        static_cast<std::size_t>(d.first_role_index("by"));
                }
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                if (d.first_role_index("label") >= 0) {
                    c.graph.label_column =
                        static_cast<std::size_t>(d.first_role_index("label"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("关系与多元")},
        {
            QStringLiteral("interval_plot"),
            QStringLiteral("区间散点图"),
            QStringLiteral("区间散点图"),
            QStringLiteral("图形"),
            QStringLiteral("interval"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应变量"), false, false},
             {QStringLiteral("category"), QStringLiteral("分类变量"), false, false},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("confidence"), QStringLiteral("置信水平"), QStringLiteral("0.95")},
             {QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "区间散点图";
                c.chart_type = "interval_plot";
                c.graph.graph_kind = "interval";
                const int response = d.first_role_index("response");
                const int category = d.first_role_index("category");
                if (response < 0 || category < 0) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择响应变量和分类变量。"));
                }
                c.graph.y_column = static_cast<std::size_t>(response);
                c.graph.by_column = static_cast<std::size_t>(category);
                c.graph.confidence_level = d.line_number("confidence").value_or(0.95);
                if (!(c.graph.confidence_level > 0.0 && c.graph.confidence_level < 1.0)) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("置信水平必须在 0 到 1 之间。"));
                }
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("时间 / 条形 / 其他")},
        {
            QStringLiteral("correlation_plot"),
            QStringLiteral("相关图"),
            QStringLiteral("相关图"),
            QStringLiteral("图形"),
            QStringLiteral("correlation"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), true, false},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("method"), QStringLiteral("方法"), QStringLiteral("pearson")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平"), QStringLiteral("0.95")},
             {QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "相关图";
                c.chart_type = "correlation_plot";
                c.graph.graph_kind = "correlation";
                c.graph.variable_columns.clear();
                for (const int column : d.role_indices("variables")) {
                    c.graph.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                if (c.graph.variable_columns.size() < 2) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("相关图至少需要两个连续变量。"));
                }
                c.graph.correlation_method = normalize(d.line_text("method"));
                if (c.graph.correlation_method != "spearman") {
                    c.graph.correlation_method = "pearson";
                }
                c.graph.confidence_level = d.line_number("confidence").value_or(0.95);
                if (!(c.graph.confidence_level > 0.0 && c.graph.confidence_level < 1.0)) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("置信水平必须在 0 到 1 之间。"));
                }
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("关系与多元")},
        {
            QStringLiteral("bubble_plot"),
            QStringLiteral("气泡图"),
            QStringLiteral("气泡图"),
            QStringLiteral("图形"),
            QStringLiteral("bubble"),
            false, true,
            {{QStringLiteral("x_variable"), QStringLiteral("X 变量"), false, false},
             {QStringLiteral("y_variable"), QStringLiteral("Y 变量"), false, false},
             {QStringLiteral("size_variable"), QStringLiteral("气泡大小变量"), false, false},
             {QStringLiteral("by"), QStringLiteral("分组变量"), false, true},
             {QStringLiteral("label"), QStringLiteral("标签变量"), false, true},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "气泡图";
                c.chart_type = "bubble_plot";
                c.graph.graph_kind = "bubble";
                const int x = d.first_role_index("x_variable");
                const int y = d.first_role_index("y_variable");
                const int size = d.first_role_index("size_variable");
                if (x < 0 || y < 0 || size < 0) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择 X、Y 和气泡大小变量。"));
                }
                c.graph.x_column = static_cast<std::size_t>(x);
                c.graph.y_column = static_cast<std::size_t>(y);
                c.graph.size_column = static_cast<std::size_t>(size);
                if (d.first_role_index("by") >= 0) {
                    c.graph.by_column =
                        static_cast<std::size_t>(d.first_role_index("by"));
                }
                if (d.first_role_index("label") >= 0) {
                    c.graph.label_column =
                        static_cast<std::size_t>(d.first_role_index("label"));
                }
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("关系与多元")},
        {
            QStringLiteral("probability_plot"),
            QStringLiteral("概率图"),
            QStringLiteral("正态概率图"),
            QStringLiteral("图形"),
            QStringLiteral("probability"),
            false, true,
            {{QStringLiteral("variable"), QStringLiteral("变量"), false, false},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "正态概率图";
                c.chart_type = "probability_plot";
                c.graph.graph_kind = "probability";
                const int column = d.first_role_index("variable");
                if (column < 0) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择连续变量。"));
                }
                c.graph.y_column = static_cast<std::size_t>(column);
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("分布与单变量")},
        {
            QStringLiteral("ecdf_plot"),
            QStringLiteral("经验累积分布图"),
            QStringLiteral("经验累积分布图"),
            QStringLiteral("图形"),
            QStringLiteral("ecdf"),
            false, true,
            {{QStringLiteral("variable"), QStringLiteral("变量"), false, false},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "经验累积分布图";
                c.chart_type = "ecdf_plot";
                c.graph.graph_kind = "ecdf";
                const int column = d.first_role_index("variable");
                if (column < 0) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择连续变量。"));
                }
                c.graph.y_column = static_cast<std::size_t>(column);
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("分布与单变量")},
        {
            QStringLiteral("matrix_plot"),
            QStringLiteral("矩阵图"),
            QStringLiteral("矩阵图"),
            QStringLiteral("图形"),
            QStringLiteral("matrix"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), true, false},
             {QStringLiteral("by"), QStringLiteral("分组变量"), false, true},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "矩阵图";
                c.chart_type = "matrix_plot";
                c.graph.graph_kind = "matrix";
                for (const int column : d.role_indices("variables")) {
                    c.graph.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                if (c.graph.variable_columns.size() < 2) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("矩阵图至少需要两个连续变量。"));
                }
                if (d.first_role_index("by") >= 0) {
                    c.graph.by_column = static_cast<std::size_t>(d.first_role_index("by"));
                }
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("关系与多元")},
        {
            QStringLiteral("marginal_plot"),
            QStringLiteral("边际图"),
            QStringLiteral("边际图"),
            QStringLiteral("图形"),
            QStringLiteral("marginal"),
            false, true,
            {{QStringLiteral("x_variable"), QStringLiteral("X 变量"), false, false},
             {QStringLiteral("y_variable"), QStringLiteral("Y 变量"), false, false},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "边际图";
                c.chart_type = "marginal_plot";
                c.graph.graph_kind = "marginal";
                const int x = d.first_role_index("x_variable");
                const int y = d.first_role_index("y_variable");
                if (x < 0 || y < 0) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择 X 变量和 Y 变量。"));
                }
                c.graph.x_column = static_cast<std::size_t>(x);
                c.graph.y_column = static_cast<std::size_t>(y);
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("时间 / 条形 / 其他")},
        {
            QStringLiteral("parallel_plot"),
            QStringLiteral("平行坐标图"),
            QStringLiteral("平行坐标图"),
            QStringLiteral("图形"),
            QStringLiteral("parallel"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), true, false},
             {QStringLiteral("by"), QStringLiteral("分组变量"), false, true},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "平行坐标图";
                c.chart_type = "parallel_plot";
                c.graph.graph_kind = "parallel";
                for (const int column : d.role_indices("variables")) {
                    c.graph.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                if (c.graph.variable_columns.size() < 2) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("平行坐标图至少需要两个连续变量。"));
                }
                if (d.first_role_index("by") >= 0) {
                    c.graph.by_column = static_cast<std::size_t>(d.first_role_index("by"));
                }
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("关系与多元")},
        {
            QStringLiteral("heatmap_plot"),
            QStringLiteral("热图"),
            QStringLiteral("热图"),
            QStringLiteral("图形"),
            QStringLiteral("heatmap"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("相关变量"), true, true},
             {QStringLiteral("x_variable"), QStringLiteral("列类别"), false, true},
             {QStringLiteral("y_variable"), QStringLiteral("行类别"), false, true},
             {QStringLiteral("z_variable"), QStringLiteral("数值"), false, true},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("method"), QStringLiteral("相关方法"), QStringLiteral("pearson")},
             {QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "热图";
                c.chart_type = "heatmap_plot";
                c.graph.graph_kind = "heatmap";
                for (const int column : d.role_indices("variables")) {
                    c.graph.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                if (d.first_role_index("x_variable") >= 0) {
                    c.graph.x_column = static_cast<std::size_t>(d.first_role_index("x_variable"));
                }
                if (d.first_role_index("y_variable") >= 0) {
                    c.graph.y_column = static_cast<std::size_t>(d.first_role_index("y_variable"));
                }
                if (d.first_role_index("z_variable") >= 0) {
                    c.graph.z_column = static_cast<std::size_t>(d.first_role_index("z_variable"));
                }
                c.graph.correlation_method = normalize(d.line_text("method"));
                if (c.graph.correlation_method != "spearman") {
                    c.graph.correlation_method = "pearson";
                }
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                if (c.graph.variable_columns.size() < 2
                    && !(c.graph.x_column.has_value() && c.graph.y_column.has_value()
                         && c.graph.z_column.has_value())) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择至少两个连续变量，或行/列类别和数值。"));
                }
                return {};
            },
            GraphService::run,
            QStringLiteral("关系与多元")},
        {
            QStringLiteral("time_series_plot"),
            QStringLiteral("时间序列图"),
            QStringLiteral("时间序列图"),
            QStringLiteral("图形"),
            QStringLiteral("time_series"),
            false, true,
            {{QStringLiteral("time"), QStringLiteral("时间变量"), false, false},
             {QStringLiteral("value"), QStringLiteral("数值变量"), false, false},
             {QStringLiteral("by"), QStringLiteral("分组变量"), false, true},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("connect_missing"), QStringLiteral("连接缺失间隔"),
              QStringLiteral("yes / no")},
             {QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "时间序列图";
                c.chart_type = "time_series_plot";
                c.graph.graph_kind = "time_series";
                const int time = d.first_role_index("time");
                const int value = d.first_role_index("value");
                if (time < 0 || value < 0) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择时间变量和数值变量。"));
                }
                c.graph.time_column = static_cast<std::size_t>(time);
                c.graph.x_column = static_cast<std::size_t>(time);
                c.graph.y_column = static_cast<std::size_t>(value);
                if (d.first_role_index("by") >= 0) {
                    c.graph.by_column = static_cast<std::size_t>(d.first_role_index("by"));
                }
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                const QString connect = QString::fromStdString(normalize(d.line_text("connect_missing")));
                c.graph.connect_missing = connect != QStringLiteral("no")
                    && connect != QStringLiteral("false")
                    && connect != QStringLiteral("0");
                return {};
            },
            GraphService::run,
            QStringLiteral("时间 / 条形 / 其他")},
        {
            QStringLiteral("area_plot"),
            QStringLiteral("区域图"),
            QStringLiteral("区域图"),
            QStringLiteral("图形"),
            QStringLiteral("area"),
            false, true,
            {{QStringLiteral("time"), QStringLiteral("顺序/时间变量"), false, false},
             {QStringLiteral("value"), QStringLiteral("数值变量"), false, false},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "区域图";
                c.chart_type = "area_plot";
                c.graph.graph_kind = "area";
                const int time = d.first_role_index("time");
                const int value = d.first_role_index("value");
                if (time < 0 || value < 0) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择顺序变量和数值变量。"));
                }
                c.graph.time_column = static_cast<std::size_t>(time);
                c.graph.x_column = static_cast<std::size_t>(time);
                c.graph.y_column = static_cast<std::size_t>(value);
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("时间 / 条形 / 其他")},
        {
            QStringLiteral("contour_plot"),
            QStringLiteral("等值线图"),
            QStringLiteral("等值线图"),
            QStringLiteral("图形"),
            QStringLiteral("contour"),
            false, true,
            {{QStringLiteral("x_variable"), QStringLiteral("X 变量"), false, false},
             {QStringLiteral("y_variable"), QStringLiteral("Y 变量"), false, false},
             {QStringLiteral("z_variable"), QStringLiteral("Z 变量"), false, false},
             {QStringLiteral("facet"), QStringLiteral("分面变量（多面板）"), false, true}},
            {{QStringLiteral("levels"), QStringLiteral("等值线层数"), QStringLiteral("8")},
             {QStringLiteral("facet_max_panels"), QStringLiteral("分面最大面板数"),
              QStringLiteral("6")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "等值线图";
                c.chart_type = "contour_plot";
                c.graph.graph_kind = "contour";
                const int x = d.first_role_index("x_variable");
                const int y = d.first_role_index("y_variable");
                const int z = d.first_role_index("z_variable");
                if (x < 0 || y < 0 || z < 0) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择 X、Y 和 Z 变量。"));
                }
                c.graph.x_column = static_cast<std::size_t>(x);
                c.graph.y_column = static_cast<std::size_t>(y);
                c.graph.z_column = static_cast<std::size_t>(z);
                c.graph.contour_levels = d.line_int("levels").value_or(8);
                if (c.graph.contour_levels < 2 || c.graph.contour_levels > 20) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("等值线层数必须在 2 到 20 之间。"));
                }
                if (d.first_role_index("facet") >= 0) {
                    c.graph.facet_column =
                        static_cast<std::size_t>(d.first_role_index("facet"));
                }
                c.graph.facet_max_panels =
                    std::clamp(d.line_int("facet_max_panels").value_or(6), 1, 12);
                return {};
            },
            GraphService::run,
            QStringLiteral("关系与多元")},
        {
            QStringLiteral("pie_plot"),
            QStringLiteral("饼图"),
            QStringLiteral("饼图"),
            QStringLiteral("图形"),
            QStringLiteral("pie"),
            false, true,
            {{QStringLiteral("category"), QStringLiteral("分类变量"), false, false},
             {QStringLiteral("weight"), QStringLiteral("权重/计数"), false, true}},
            {{QStringLiteral("other_threshold"), QStringLiteral("小类别合并阈值 %"),
              QStringLiteral("5")}},
            [](AnalysisConfiguration& c,
               const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "饼图";
                c.chart_type = "pie_plot";
                c.graph.graph_kind = "pie";
                const int category = d.first_role_index("category");
                if (category < 0) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择分类变量。"));
                }
                c.graph.x_column = static_cast<std::size_t>(category);
                if (d.first_role_index("weight") >= 0) {
                    c.graph.weight_column =
                        static_cast<std::size_t>(d.first_role_index("weight"));
                }
                c.graph.other_threshold_percent =
                    d.line_number("other_threshold").value_or(5.0);
                if (c.graph.other_threshold_percent < 0.0
                    || c.graph.other_threshold_percent > 100.0) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("小类别合并阈值必须在 0 到 100 之间。"));
                }
                return {};
            },
            GraphService::run,
            QStringLiteral("时间 / 条形 / 其他")},
        // ------------------------------------------------------------ 控制图
        {
            QStringLiteral("imr"),
            QStringLiteral("I-MR 控制图"),
            QStringLiteral("I-MR 控制图"),
            QStringLiteral("控制图"),
            QStringLiteral("imr"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false},
             {QStringLiteral("stage"), QStringLiteral("阶段列"), false, true}},
            {{QStringLiteral("mr_length"), QStringLiteral("移动极差长度"), QStringLiteral("2")},
             {QStringLiteral("sigma_method"), QStringLiteral("σ 方法"),
              QStringLiteral("average_moving_range")},
             {QStringLiteral("use_nelson_estimate"), QStringLiteral("Nelson estimate (0/1)"),
              QStringLiteral("0")},
             special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("individuals")),
             {QStringLiteral("historical_center"), QStringLiteral("历史均值"),
              QStringLiteral("可选")},
             {QStringLiteral("historical_sigma"), QStringLiteral("历史 Sigma"),
              QStringLiteral("可选")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "I-MR 控制图";
                c.chart_type = "imr";
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                if (d.first_role_index("stage") >= 0) {
                    c.control.stage_column = static_cast<std::size_t>(
                        d.first_role_index("stage"));
                }
                c.control.moving_range_length = d.line_int("mr_length").value_or(2);
                const std::string method = normalize(d.line_text("sigma_method"));
                if (method == "median_moving_range"
                    || method == "mssd"
                    || method == "average_moving_range") {
                    c.control.sigma_method = method;
                } else if (!method.empty()
                           && method != "average_moving_range") {
                    return apply_error(QStringLiteral("σ 方法无效"),
                                       QStringLiteral("请使用 average_moving_range / median_moving_range / mssd。"));
                }
                c.control.use_nelson_estimate =
                    d.line_int("use_nelson_estimate").value_or(0) != 0;
                apply_special_cause_selection(c, d);
                c.control.historical_center = d.line_number("historical_center");
                c.control.historical_sigma = d.line_number("historical_sigma");
                return {};
            },
            AnalysisService::individuals_moving_range,
            QStringLiteral("计量图")},
        {
            QStringLiteral("xbar_r"),
            QStringLiteral("Xbar-R 控制图"),
            QStringLiteral("Xbar-R 控制图"),
            QStringLiteral("控制图"),
            QStringLiteral("xbar_r"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false},
             {QStringLiteral("subgroup"), QStringLiteral("子组列"), false, true},
             {QStringLiteral("stage"), QStringLiteral("阶段列"), false, true}},
            {{QStringLiteral("subgroup_size"), QStringLiteral("子组大小"), QStringLiteral("5")},
             special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("xbar"))},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "Xbar-R 控制图";
                c.chart_type = "xbar_r";
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                if (d.first_role_index("subgroup") >= 0) {
                    c.selection.subgroup_column = static_cast<std::size_t>(
                        d.first_role_index("subgroup"));
                }
                if (d.first_role_index("stage") >= 0) {
                    c.control.stage_column = static_cast<std::size_t>(
                        d.first_role_index("stage"));
                }
                c.control.subgroup_size = d.line_int("subgroup_size").value_or(5);
                apply_special_cause_selection(c, d);
                return {};
            },
            AnalysisService::xbar_range,
            QStringLiteral("计量图")},
        {
            QStringLiteral("xbar_s"),
            QStringLiteral("Xbar-S 控制图"),
            QStringLiteral("Xbar-S 控制图"),
            QStringLiteral("控制图"),
            QStringLiteral("xbar_s"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false},
             {QStringLiteral("subgroup"), QStringLiteral("子组列"), false, true},
             {QStringLiteral("stage"), QStringLiteral("阶段列"), false, true}},
            {{QStringLiteral("subgroup_size"), QStringLiteral("子组大小"), QStringLiteral("5")},
             special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("xbar"))},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "Xbar-S 控制图";
                c.chart_type = "xbar_s";
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_silent();
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                if (d.first_role_index("subgroup") >= 0) {
                    c.selection.subgroup_column = static_cast<std::size_t>(
                        d.first_role_index("subgroup"));
                }
                if (d.first_role_index("stage") >= 0) {
                    c.control.stage_column = static_cast<std::size_t>(
                        d.first_role_index("stage"));
                }
                c.control.subgroup_size = d.line_int("subgroup_size").value_or(5);
                apply_special_cause_selection(c, d);
                return {};
            },
            AnalysisService::xbar_s,
            QStringLiteral("计量图")},
        {
            QStringLiteral("imr_rs"),
            QStringLiteral("I-MR-R/S 控制图"),
            QStringLiteral("I-MR-R/S 控制图"),
            QStringLiteral("控制图"),
            QStringLiteral("imr_rs"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false},
             {QStringLiteral("subgroup"), QStringLiteral("子组列"), false, true},
             {QStringLiteral("stage"), QStringLiteral("阶段列"), false, true}},
            {{QStringLiteral("subgroup_size"), QStringLiteral("子组大小"), QStringLiteral("5")},
             special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("individuals"))},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "I-MR-R/S 控制图";
                c.chart_type = "imr_rs";
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                if (d.first_role_index("subgroup") >= 0) {
                    c.selection.subgroup_column = static_cast<std::size_t>(
                        d.first_role_index("subgroup"));
                }
                if (d.first_role_index("stage") >= 0) {
                    c.control.stage_column = static_cast<std::size_t>(
                        d.first_role_index("stage"));
                }
                c.control.subgroup_size = d.line_int("subgroup_size").value_or(5);
                apply_special_cause_selection(c, d);
                return {};
            },
            AnalysisService::imr_rs,
            QStringLiteral("计量图")},
        {
            QStringLiteral("p_chart"),
            QStringLiteral("P 图"),
            QStringLiteral("P 图"),
            QStringLiteral("控制图"),
            QStringLiteral("p_chart"),
            true, true,
            {{QStringLiteral("defectives"), QStringLiteral("不合格品数"), false, false},
             {QStringLiteral("inspected"), QStringLiteral("检验数（列）"), false, true},
             {QStringLiteral("stage"), QStringLiteral("阶段列"), false, true}},
            {{QStringLiteral("inspected_constant"), QStringLiteral("检验数（常数）"), QString()},
             special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("attribute"))},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "P 图";
                c.chart_type = "p_chart";
                const int defectives = d.first_role_index("defectives");
                if (defectives < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择不合格品数列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defectives));
                c.selection.defect_count_column = static_cast<std::size_t>(defectives);
                if (d.first_role_index("inspected") >= 0) {
                    c.selection.inspected_count_column = static_cast<std::size_t>(
                        d.first_role_index("inspected"));
                }
                if (d.line_int("inspected_constant").has_value()) {
                    c.inspected_constant = static_cast<std::size_t>(
                        *d.line_int("inspected_constant"));
                }
                if (d.first_role_index("stage") >= 0) {
                    c.control.stage_column = static_cast<std::size_t>(
                        d.first_role_index("stage"));
                }
                apply_special_cause_selection(c, d);
                return {};
            },
            AnalysisService::p_chart,
            QStringLiteral("计数图")},
        {
            QStringLiteral("np_chart"),
            QStringLiteral("NP 图"),
            QStringLiteral("NP 图"),
            QStringLiteral("控制图"),
            QStringLiteral("np_chart"),
            false, true,
            {{QStringLiteral("defectives"), QStringLiteral("不合格品数"), false, false},
             {QStringLiteral("inspected"), QStringLiteral("检验数列"), false, true},
             {QStringLiteral("stage"), QStringLiteral("阶段列"), false, true}},
            {{QStringLiteral("inspected_constant"), QStringLiteral("检验数常数"),
              QStringLiteral("例如 100")},
             special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("attribute"))},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.chart_type = "np_chart";
                const int defect_column = d.first_role_index("defectives");
                if (defect_column < 0) {
                    return apply_silent();
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defect_column));
                c.selection.defect_count_column = static_cast<std::size_t>(defect_column);
                if (d.first_role_index("inspected") >= 0) {
                    c.selection.inspected_count_column = static_cast<std::size_t>(
                        d.first_role_index("inspected"));
                }
                if (d.line_int("inspected_constant").has_value()) {
                    c.inspected_constant = static_cast<std::size_t>(
                        *d.line_int("inspected_constant"));
                }
                if (d.first_role_index("stage") >= 0) {
                    c.control.stage_column = static_cast<std::size_t>(
                        d.first_role_index("stage"));
                }
                apply_special_cause_selection(c, d);
                return {};
            },
            AnalysisService::np_chart,
            QStringLiteral("计数图")},
        {
            QStringLiteral("c_chart"),
            QStringLiteral("C 图"),
            QStringLiteral("C 图"),
            QStringLiteral("控制图"),
            QStringLiteral("c_chart"),
            false, true,
            {{QStringLiteral("defects"), QStringLiteral("缺陷数"), false, false}},
            {{QStringLiteral("units"), QStringLiteral("每个子组单位数"), QStringLiteral("1")},
             {QStringLiteral("stage"), QStringLiteral("阶段列"), QString()},
             special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("attribute"))},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.chart_type = "c_chart";
                const int defect_column = d.first_role_index("defects");
                if (defect_column < 0) {
                    return apply_silent();
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defect_column));
                c.selection.defect_count_column = static_cast<std::size_t>(defect_column);
                c.inspected_constant = static_cast<std::size_t>(
                    d.line_int("units").value_or(1));
                if (d.first_role_index("stage") >= 0) {
                    c.control.stage_column = static_cast<std::size_t>(
                        d.first_role_index("stage"));
                }
                apply_special_cause_selection(c, d);
                return {};
            },
            AnalysisService::c_chart,
            QStringLiteral("计数图")},
        {
            QStringLiteral("u_chart"),
            QStringLiteral("U 图"),
            QStringLiteral("U 图"),
            QStringLiteral("控制图"),
            QStringLiteral("u_chart"),
            false, true,
            {{QStringLiteral("defects"), QStringLiteral("缺陷数"), false, false},
             {QStringLiteral("units"), QStringLiteral("单位数列"), false, false},
             {QStringLiteral("stage"), QStringLiteral("阶段列"), false, true}},
            {special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("attribute"))},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.chart_type = "u_chart";
                const int defect_column = d.first_role_index("defects");
                const int units_column = d.first_role_index("units");
                if (defect_column < 0 || units_column < 0) {
                    return apply_silent();
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defect_column));
                c.selection.defect_count_column = static_cast<std::size_t>(defect_column);
                c.selection.inspected_count_column = static_cast<std::size_t>(units_column);
                if (d.first_role_index("stage") >= 0) {
                    c.control.stage_column = static_cast<std::size_t>(
                        d.first_role_index("stage"));
                }
                apply_special_cause_selection(c, d);
                return {};
            },
            AnalysisService::u_chart,
            QStringLiteral("计数图")},
        {
            QStringLiteral("laney_p_chart"),
            QStringLiteral("Laney P' 图"),
            QStringLiteral("Laney P' 图"),
            QStringLiteral("控制图"),
            QStringLiteral("laney_p_chart"),
            true, true,
            {{QStringLiteral("defectives"), QStringLiteral("不合格品数"), false, false},
             {QStringLiteral("inspected"), QStringLiteral("检验数列"), false, true},
             {QStringLiteral("stage"), QStringLiteral("阶段列"), false, true}},
            {{QStringLiteral("inspected_constant"), QStringLiteral("检验数常数"),
              QStringLiteral("可选")},
             special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("laney")),
             {QStringLiteral("historical_center"), QStringLiteral("历史中心线"),
              QStringLiteral("可选")},
             {QStringLiteral("historical_sigma_z"), QStringLiteral("历史 Sigma Z"),
              QStringLiteral("可选")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "Laney P' 图";
                c.chart_type = "laney_p_chart";
                const int defect_column = d.first_role_index("defectives");
                if (defect_column < 0) {
                    return apply_silent();
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defect_column));
                c.selection.defect_count_column = static_cast<std::size_t>(defect_column);
                if (d.first_role_index("inspected") >= 0) {
                    c.selection.inspected_count_column = static_cast<std::size_t>(
                        d.first_role_index("inspected"));
                }
                if (d.first_role_index("stage") >= 0) {
                    c.control.stage_column = static_cast<std::size_t>(
                        d.first_role_index("stage"));
                }
                if (d.line_int("inspected_constant").has_value()) {
                    c.inspected_constant = static_cast<std::size_t>(
                        *d.line_int("inspected_constant"));
                }
                apply_special_cause_selection(c, d);
                c.control.historical_center = d.line_number("historical_center");
                c.control.historical_sigma_z = d.line_number("historical_sigma_z");
                return {};
            },
            AnalysisService::laney_p_chart,
            QStringLiteral("计数图")},
        {
            QStringLiteral("laney_u_chart"),
            QStringLiteral("Laney U' 图"),
            QStringLiteral("Laney U' 图"),
            QStringLiteral("控制图"),
            QStringLiteral("laney_u_chart"),
            false, true,
            {{QStringLiteral("defects"), QStringLiteral("缺陷数"), false, false},
             {QStringLiteral("units"), QStringLiteral("单位数列"), false, false},
             {QStringLiteral("stage"), QStringLiteral("阶段列"), false, true}},
            {special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("laney")),
             {QStringLiteral("historical_center"), QStringLiteral("历史中心线"),
              QStringLiteral("可选")},
             {QStringLiteral("historical_sigma_z"), QStringLiteral("历史 Sigma Z"),
              QStringLiteral("可选")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "Laney U' 图";
                c.chart_type = "laney_u_chart";
                const int defect_column = d.first_role_index("defects");
                const int units_column = d.first_role_index("units");
                if (defect_column < 0 || units_column < 0) {
                    return apply_silent();
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defect_column));
                c.selection.defect_count_column = static_cast<std::size_t>(defect_column);
                c.selection.inspected_count_column = static_cast<std::size_t>(units_column);
                if (d.first_role_index("stage") >= 0) {
                    c.control.stage_column = static_cast<std::size_t>(
                        d.first_role_index("stage"));
                }
                apply_special_cause_selection(c, d);
                c.control.historical_center = d.line_number("historical_center");
                c.control.historical_sigma_z = d.line_number("historical_sigma_z");
                return {};
            },
            AnalysisService::laney_u_chart,
            QStringLiteral("计数图")},
        {
            QStringLiteral("ewma"),
            QStringLiteral("EWMA 控制图"),
            QStringLiteral("EWMA 控制图"),
            QStringLiteral("控制图"),
            QStringLiteral("ewma"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("测量值"), false, false}},
            {{QStringLiteral("lambda"), QStringLiteral("Lambda"), QStringLiteral("0.2")},
             {QStringLiteral("limit"), QStringLiteral("控制限倍数"), QStringLiteral("3")},
             {QStringLiteral("historical_mean"), QStringLiteral("历史均值（可选）"),
              QStringLiteral("可选")},
             {QStringLiteral("historical_sigma"), QStringLiteral("历史 Sigma（可选）"),
              QStringLiteral("可选")},
             special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("ewma"))},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_silent();
                }
                c.analysis_name = "EWMA 控制图";
                c.chart_type = "ewma";
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.control.ewma_lambda = d.line_number("lambda").value_or(0.2);
                c.control.ewma_limit_sigma = d.line_number("limit").value_or(3.0);
                c.control.historical_center = d.line_number("historical_mean");
                c.control.historical_sigma = d.line_number("historical_sigma");
                apply_special_cause_selection(c, d);
                return {};
            },
            AnalysisService::ewma,
            QStringLiteral("时间加权")},
        {
            QStringLiteral("hotelling_t2"),
            QStringLiteral("Hotelling T²"),
            QStringLiteral("Hotelling T² 多元控制图"),
            QStringLiteral("控制图"),
            QStringLiteral("imr"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量（多列）"), true, false}},
            {{QStringLiteral("phase"), QStringLiteral("phase1 / phase2"), QStringLiteral("phase1")},
             {QStringLiteral("alpha"), QStringLiteral("α"), QStringLiteral("0.05")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const auto columns = d.role_indices("variables");
                if (columns.size() < 2) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请至少选择两个数值变量。"));
                }
                c.analysis_name = "Hotelling T²";
                c.chart_type = "hotelling_t2";
                c.variable_columns.clear();
                for (const int column : columns) {
                    c.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                const std::string phase = normalize(d.line_text("phase"));
                c.control.sigma_method = (phase == "phase2" ? "phase2" : "phase1");
                const double alpha = d.line_number("alpha").value_or(0.05);
                if (alpha > 0.0 && alpha < 1.0) {
                    c.inference.confidence_level = 1.0 - alpha;
                }
                return {};
            },
            AnalysisService::hotelling_t2,
            QStringLiteral("多元 / 稀有事件")},
        {
            QStringLiteral("mewma"),
            QStringLiteral("MEWMA"),
            QStringLiteral("多元 EWMA"),
            QStringLiteral("控制图"),
            QStringLiteral("ewma"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量（多列）"), true, false}},
            {{QStringLiteral("lambda"), QStringLiteral("Lambda"), QStringLiteral("0.1")},
             {QStringLiteral("ucl"), QStringLiteral("UCL（可选）"), QStringLiteral("可选")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const auto columns = d.role_indices("variables");
                if (columns.size() < 2) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请至少选择两个数值变量。"));
                }
                c.analysis_name = "MEWMA";
                c.chart_type = "mewma";
                c.variable_columns.clear();
                for (const int column : columns) {
                    c.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                c.control.ewma_lambda = d.line_number("lambda").value_or(0.1);
                c.control.historical_sigma = d.line_number("ucl");
                return {};
            },
            AnalysisService::mewma,
            QStringLiteral("多元 / 稀有事件")},
        {
            QStringLiteral("generalized_variance"),
            QStringLiteral("广义方差图"),
            QStringLiteral("广义方差 |S| 多元控制图"),
            QStringLiteral("控制图"),
            QStringLiteral("hotelling_t2"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量（多列）"), true, false}},
            {{QStringLiteral("subgroup_size"), QStringLiteral("子组大小 n"), QStringLiteral("5")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const auto columns = d.role_indices("variables");
                if (columns.size() < 2) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请至少选择两个数值变量。"));
                }
                const int n = static_cast<int>(d.line_number("subgroup_size").value_or(5.0));
                if (n <= static_cast<int>(columns.size())) {
                    return apply_error(QStringLiteral("子组过小"),
                                       QStringLiteral("子组大小 n 必须大于变量数 p。"));
                }
                c.analysis_name = "广义方差图";
                c.chart_type = "generalized_variance";
                c.variable_columns.clear();
                for (const int column : columns) {
                    c.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                c.control.subgroup_size = n;
                return {};
            },
            AnalysisService::generalized_variance,
            QStringLiteral("多元 / 稀有事件")},
        {
            QStringLiteral("cusum"),
            QStringLiteral("CUSUM 控制图"),
            QStringLiteral("CUSUM 控制图"),
            QStringLiteral("控制图"),
            QStringLiteral("cusum"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("测量值"), false, false}},
            {{QStringLiteral("target"), QStringLiteral("目标值"), QStringLiteral("0")},
             {QStringLiteral("sigma"), QStringLiteral("过程 Sigma"), QStringLiteral("1")},
             {QStringLiteral("k"), QStringLiteral("参考值 k"), QStringLiteral("0.5")},
             {QStringLiteral("h"), QStringLiteral("决策间隔 h"), QStringLiteral("4")},
             special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("cusum"))},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_silent();
                }
                c.analysis_name = "CUSUM 控制图";
                c.chart_type = "cusum";
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.control.cusum_target = d.line_number("target").value_or(0.0);
                c.control.cusum_sigma = d.line_number("sigma").value_or(1.0);
                c.control.cusum_k = d.line_number("k").value_or(0.5);
                c.control.cusum_h = d.line_number("h").value_or(4.0);
                apply_special_cause_selection(c, d);
                return {};
            },
            AnalysisService::cusum,
            QStringLiteral("时间加权")},
        {
            QStringLiteral("zone_chart"),
            QStringLiteral("区域图"),
            QStringLiteral("区域图"),
            QStringLiteral("控制图"),
            QStringLiteral("imr"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("测量值"), false, false}},
            {{QStringLiteral("mr_length"), QStringLiteral("移动极差长度"), QStringLiteral("2")},
             {QStringLiteral("historical_center"), QStringLiteral("历史均值（可选）"),
              QStringLiteral("可选")},
             {QStringLiteral("historical_sigma"), QStringLiteral("历史 Sigma（可选）"),
              QStringLiteral("可选")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_silent();
                }
                c.analysis_name = "区域图";
                c.chart_type = "zone_chart";
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.control.moving_range_length = d.line_int("mr_length").value_or(2);
                c.control.historical_center = d.line_number("historical_center");
                c.control.historical_sigma = d.line_number("historical_sigma");
                return {};
            },
            AnalysisService::zone_chart,
            QStringLiteral("计量图")},
        {
            QStringLiteral("z_mr"),
            QStringLiteral("Z-MR 控制图"),
            QStringLiteral("Z-MR 控制图"),
            QStringLiteral("控制图"),
            QStringLiteral("imr_rs"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("测量值"), false, false},
             {QStringLiteral("group"), QStringLiteral("分组列（可选）"), false, true}},
            {{QStringLiteral("mr_length"), QStringLiteral("移动极差长度"), QStringLiteral("2")},
             special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("z_mr"))},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_silent();
                }
                c.analysis_name = "Z-MR 控制图";
                c.chart_type = "z_mr";
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.control.moving_range_length = d.line_int("mr_length").value_or(2);
                if (d.first_role_index("group") >= 0) {
                    c.by_column = static_cast<std::size_t>(d.first_role_index("group"));
                }
                apply_special_cause_selection(c, d);
                return {};
            },
            AnalysisService::z_mr,
            QStringLiteral("计量图")},
        {
            QStringLiteral("moving_average"),
            QStringLiteral("移动平均控制图"),
            QStringLiteral("移动平均控制图"),
            QStringLiteral("控制图"),
            QStringLiteral("ewma"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("测量值"), false, false}},
            {{QStringLiteral("ma_window"), QStringLiteral("窗宽 w"), QStringLiteral("3")},
             {QStringLiteral("mr_length"), QStringLiteral("移动极差长度"), QStringLiteral("2")},
             {QStringLiteral("limit"), QStringLiteral("控制限倍数"), QStringLiteral("3")},
             {QStringLiteral("historical_center"), QStringLiteral("历史均值（可选）"),
              QStringLiteral("可选")},
             {QStringLiteral("historical_sigma"), QStringLiteral("历史 Sigma（可选）"),
              QStringLiteral("可选")},
             special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("moving_average"))},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_silent();
                }
                c.analysis_name = "移动平均控制图";
                c.chart_type = "moving_average";
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.control.ma_window = d.line_int("ma_window").value_or(3);
                c.control.moving_range_length = d.line_int("mr_length").value_or(2);
                c.control.ewma_limit_sigma = d.line_number("limit").value_or(3.0);
                c.control.historical_center = d.line_number("historical_center");
                c.control.historical_sigma = d.line_number("historical_sigma");
                apply_special_cause_selection(c, d);
                return {};
            },
            AnalysisService::moving_average,
            QStringLiteral("时间加权")},
        {
            QStringLiteral("g_chart"),
            QStringLiteral("G 图"),
            QStringLiteral("G 图（稀有事件间隔）"),
            QStringLiteral("控制图"),
            QStringLiteral("ewma"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("间隔列"), false, false}},
            {special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("g"))},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择数值间隔列。"));
                }
                c.analysis_name = "G 图";
                c.chart_type = "g_chart";
                c.variable_columns = {static_cast<std::size_t>(column)};
                apply_special_cause_selection(c, d);
                return {};
            },
            AnalysisService::g_chart,
            QStringLiteral("多元 / 稀有事件")},
        {
            QStringLiteral("t_chart"),
            QStringLiteral("T 图"),
            QStringLiteral("T 图（稀有事件间隔）"),
            QStringLiteral("控制图"),
            QStringLiteral("ewma"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("间隔列"), false, false}},
            {special_cause_policy_input(),
             special_cause_tests_input(QStringLiteral("t"))},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择数值间隔列。"));
                }
                c.analysis_name = "T 图";
                c.chart_type = "t_chart";
                c.variable_columns = {static_cast<std::size_t>(column)};
                apply_special_cause_selection(c, d);
                return {};
            },
            AnalysisService::t_chart,
            QStringLiteral("多元 / 稀有事件")},
        // ------------------------------------------------------------ 质量工具
        {
            QStringLiteral("capability"),
            QStringLiteral("正态过程能力"),
            QStringLiteral("正态过程能力分析"),
            QStringLiteral("质量工具"),
            QStringLiteral("capability"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false}},
            {{QStringLiteral("subgroup_size"), QStringLiteral("子组大小"), QStringLiteral("1")},
             {QStringLiteral("lsl"), QStringLiteral("LSL"), QStringLiteral("例如 73.95")},
             {QStringLiteral("usl"), QStringLiteral("USL"), QStringLiteral("例如 74.05")},
             {QStringLiteral("target"), QStringLiteral("Target"), QStringLiteral("可选")},
             {QStringLiteral("transform"), QStringLiteral("变换"),
              QStringLiteral("none / johnson")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "正态过程能力";
                c.chart_type = "capability";
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                c.control.subgroup_size = d.line_int("subgroup_size").value_or(1);
                c.specifications.lower = d.line_number("lsl");
                c.specifications.upper = d.line_number("usl");
                c.specifications.target = d.line_number("target");
                const std::string transform = normalize(d.line_text("transform"));
                c.capability_method = transform == "johnson" ? "johnson" : "normal";
                return {};
            },
            [](const DataTable& table, const AnalysisConfiguration& configuration) {
                return AnalysisService::capability(table, configuration);
            },
            QStringLiteral("过程能力")},
        {
            QStringLiteral("multi_vari"),
            QStringLiteral("Multi-Vari 图"),
            QStringLiteral("Multi-Vari 图"),
            QStringLiteral("质量工具"),
            QStringLiteral("interval"),
            false, true,
            {{QStringLiteral("measurement"), QStringLiteral("测量值"), false, false},
             {QStringLiteral("factors"), QStringLiteral("因子（2～4 列）"), true, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                c.analysis_name = "Multi-Vari 图";
                c.chart_type = "multi_vari";
                c.graph.graph_kind = "multi_vari";
                const int measurement = d.first_role_index("measurement");
                if (measurement < 0) {
                    return apply_error(QStringLiteral("未选择测量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.selection.measurement_column = static_cast<std::size_t>(measurement);
                c.graph.variable_columns.clear();
                for (const int column : d.role_indices("factors")) {
                    c.graph.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                if (c.graph.variable_columns.size() < 2
                    || c.graph.variable_columns.size() > 4) {
                    return apply_error(QStringLiteral("因子数量无效"),
                                       QStringLiteral("请选择 2～4 个因子列。"));
                }
                return {};
            },
            [](const DataTable& table, const AnalysisConfiguration& configuration) {
                return AnalysisService::multi_vari(table, configuration);
            },
            QStringLiteral("质量图 / 规划")},
        {
            QStringLiteral("variability_chart"),
            QStringLiteral("变异性图"),
            QStringLiteral("变异性图"),
            QStringLiteral("质量工具"),
            QStringLiteral("interval"),
            false, true,
            {{QStringLiteral("measurement"), QStringLiteral("测量值"), false, false},
             {QStringLiteral("factors"), QStringLiteral("因子（1～2 列）"), true, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                c.analysis_name = "变异性图";
                c.chart_type = "variability_chart";
                c.graph.graph_kind = "variability_chart";
                const int measurement = d.first_role_index("measurement");
                if (measurement < 0) {
                    return apply_error(QStringLiteral("未选择测量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.selection.measurement_column = static_cast<std::size_t>(measurement);
                c.graph.variable_columns.clear();
                for (const int column : d.role_indices("factors")) {
                    c.graph.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                if (c.graph.variable_columns.empty()
                    || c.graph.variable_columns.size() > 2) {
                    return apply_error(QStringLiteral("因子数量无效"),
                                       QStringLiteral("请选择 1～2 个因子列。"));
                }
                return {};
            },
            [](const DataTable& table, const AnalysisConfiguration& configuration) {
                return AnalysisService::variability_chart(table, configuration);
            },
            QStringLiteral("质量图 / 规划")},
        {
            QStringLiteral("acceptance_sampling"),
            QStringLiteral("属性一次抽样"),
            QStringLiteral("属性一次抽样"),
            QStringLiteral("质量工具"),
            QStringLiteral("interval"),
            false, false,
            {},
            {{QStringLiteral("sample_size"), QStringLiteral("样本量 n"), QStringLiteral("例如 20")},
             {QStringLiteral("acceptance_number"), QStringLiteral("接收数 c"), QStringLiteral("例如 1")},
             {QStringLiteral("aql"), QStringLiteral("AQL（可选）"), QString()},
             {QStringLiteral("rql"), QStringLiteral("RQL（可选）"), QString()},
             {QStringLiteral("lot_size"), QStringLiteral("批大小 N（可选，信息）"), QString()}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                c.analysis_name = "属性一次抽样";
                c.chart_type = "acceptance_sampling";
                const int sample_size = d.line_int("sample_size").value_or(0);
                const int acceptance_number = d.line_int("acceptance_number").value_or(0);
                if (sample_size <= 0) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("样本量 n 必须 ≥ 1。"));
                }
                if (acceptance_number < 0 || acceptance_number > sample_size) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("接收数 c 必须在 0…n 内。"));
                }
                c.inference.acceptance_sample_size = static_cast<std::size_t>(sample_size);
                c.inference.acceptance_number = static_cast<std::size_t>(acceptance_number);
                const std::optional<double> aql = d.line_number("aql");
                if (aql.has_value()) {
                    if (*aql < 0.0 || *aql > 1.0) {
                        return apply_error(QStringLiteral("AQL 无效"),
                                           QStringLiteral("AQL 必须在 [0, 1] 内。"));
                    }
                    c.inference.acceptance_aql = aql;
                }
                const std::optional<double> rql = d.line_number("rql");
                if (rql.has_value()) {
                    if (*rql < 0.0 || *rql > 1.0) {
                        return apply_error(QStringLiteral("RQL 无效"),
                                           QStringLiteral("RQL 必须在 [0, 1] 内。"));
                    }
                    c.inference.acceptance_rql = rql;
                }
                const int lot_size = d.line_int("lot_size").value_or(0);
                if (lot_size > 0) {
                    c.inference.acceptance_lot_size = static_cast<std::size_t>(lot_size);
                }
                return {};
            },
            AnalysisService::acceptance_sampling,
            QStringLiteral("容差 / 抽样")},
        {
            QStringLiteral("tolerance_intervals"),
            QStringLiteral("容差区间"),
            QStringLiteral("容差区间"),
            QStringLiteral("质量工具"),
            QStringLiteral("interval"),
            false, true,
            {{QStringLiteral("measurement"), QStringLiteral("测量值"), false, false}},
            {{QStringLiteral("method"), QStringLiteral("方法"),
              QStringLiteral("normal 或 nonparametric")},
             {QStringLiteral("coverage"), QStringLiteral("覆盖率 (%)"), QStringLiteral("95")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")},
             {QStringLiteral("alternative"), QStringLiteral("区间方向"),
              QStringLiteral("two_sided / lower / upper")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                const int column = d.first_role_index("measurement");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择测量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.inference.tolerance_method = normalize(d.line_text("method"));
                if (c.inference.tolerance_method != "nonparametric") {
                    c.inference.tolerance_method = "normal";
                }
                // Dual-write for older configs that only read variance_method.
                if (c.inference.tolerance_method == "nonparametric") {
                    c.inference.variance_method = "nonparametric";
                }
                c.analysis_name = c.inference.tolerance_method == "nonparametric"
                    ? "非参数容差区间" : "正态容差区间";
                c.chart_type = "tolerance_intervals";
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.selection.measurement_column = static_cast<std::size_t>(column);
                const double coverage = d.line_number("coverage").value_or(95.0);
                c.inference.coverage_proportion =
                    coverage > 1.0 ? coverage / 100.0 : coverage;
                const double confidence = d.line_number("confidence").value_or(95.0);
                c.inference.confidence_level =
                    confidence > 1.0 ? confidence / 100.0 : confidence;
                c.inference.alternative = normalize(d.line_text("alternative"));
                if (c.inference.alternative.empty()) {
                    c.inference.alternative = "two_sided";
                }
                return {};
            },
            [](const DataTable& table, const AnalysisConfiguration& configuration) {
                return AnalysisService::tolerance_intervals(table, configuration);
            },
            QStringLiteral("容差 / 抽样")},
        {
            QStringLiteral("distribution_identification"),
            QStringLiteral("个体分布识别"),
            QStringLiteral("个体分布识别"),
            QStringLiteral("质量工具"),
            QStringLiteral("normality_test"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("测量值"), false, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "个体分布识别";
                c.chart_type = "distribution_identification";
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.selection.measurement_column = static_cast<std::size_t>(column);
                return {};
            },
            [](const DataTable& table, const AnalysisConfiguration& configuration) {
                return AnalysisService::distribution_identification(table, configuration);
            },
            QStringLiteral("变换 / 分布")},
        {
            QStringLiteral("between_within_capability"),
            QStringLiteral("组间/组内过程能力"),
            QStringLiteral("组间/组内过程能力分析"),
            QStringLiteral("质量工具"),
            QStringLiteral("capability"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("测量值"), false, false},
             {QStringLiteral("subgroup"), QStringLiteral("子组"), false, false}},
            {{QStringLiteral("lsl"), QStringLiteral("LSL"), QStringLiteral("例如 73.95")},
             {QStringLiteral("usl"), QStringLiteral("USL"), QStringLiteral("例如 74.05")},
             {QStringLiteral("target"), QStringLiteral("Target"), QStringLiteral("可选")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "组间/组内过程能力";
                c.chart_type = "between_within_capability";
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                const int subgroup = d.first_role_index("subgroup");
                if (subgroup < 0) {
                    return apply_error(QStringLiteral("未选择子组"),
                                       QStringLiteral("组间/组内能力需要子组标识列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                c.selection.subgroup_column = static_cast<std::size_t>(subgroup);
                c.specifications.lower = d.line_number("lsl");
                c.specifications.upper = d.line_number("usl");
                c.specifications.target = d.line_number("target");
                c.capability_method = "between_within";
                return {};
            },
            [](const DataTable& table, const AnalysisConfiguration& configuration) {
                return AnalysisService::between_within_capability(table, configuration);
            },
            QStringLiteral("过程能力")},
        {
            QStringLiteral("batch_capability"),
            QStringLiteral("批次过程能力"),
            QStringLiteral("Batch Capability Analysis"),
            QStringLiteral("质量工具"),
            QStringLiteral("capability"),
            false, true,
            {{QStringLiteral("measurement"), QStringLiteral("测量值"), false, false},
             {QStringLiteral("batch"), QStringLiteral("批次"), false, false}},
            {{QStringLiteral("lsl"), QStringLiteral("LSL"), QStringLiteral("例如 73.95")},
             {QStringLiteral("usl"), QStringLiteral("USL"), QStringLiteral("例如 74.05")},
             {QStringLiteral("min_batch_size"), QStringLiteral("最小批次 N"), QStringLiteral("2")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int measurement = d.first_role_index("measurement");
                const int batch = d.first_role_index("batch");
                if (measurement < 0 || batch < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量列与批次列。"));
                }
                c.analysis_name = "批次过程能力";
                c.chart_type = "batch_capability";
                c.batch_capability.measurement_column = static_cast<std::size_t>(measurement);
                c.batch_capability.batch_column = static_cast<std::size_t>(batch);
                c.variable_columns = {static_cast<std::size_t>(measurement)};
                c.selection.measurement_column = static_cast<std::size_t>(measurement);
                c.specifications.lower = d.line_number("lsl");
                c.specifications.upper = d.line_number("usl");
                c.batch_capability.min_batch_size = static_cast<std::size_t>(
                    std::max(2.0, d.line_number("min_batch_size").value_or(2.0)));
                return {};
            },
            [](const DataTable& table, const AnalysisConfiguration& configuration) {
                return AnalysisService::batch_capability(table, configuration);
            },
            QStringLiteral("过程能力")},
        {
            QStringLiteral("nonparametric_capability"),
            QStringLiteral("非参数过程能力"),
            QStringLiteral("Nonparametric Capability Analysis"),
            QStringLiteral("质量工具"),
            QStringLiteral("capability"),
            false, true,
            {{QStringLiteral("measurement"), QStringLiteral("测量值"), false, false}},
            {{QStringLiteral("lsl"), QStringLiteral("LSL"), QStringLiteral("必填")},
             {QStringLiteral("usl"), QStringLiteral("USL"), QStringLiteral("必填")},
             {QStringLiteral("tolerance_k"), QStringLiteral("容差 K"), QStringLiteral("6")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int measurement = d.first_role_index("measurement");
                if (measurement < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量列。"));
                }
                c.analysis_name = "非参数过程能力";
                c.chart_type = "nonparametric_capability";
                c.nonparametric_capability.measurement_column = static_cast<std::size_t>(measurement);
                c.variable_columns = {static_cast<std::size_t>(measurement)};
                c.selection.measurement_column = static_cast<std::size_t>(measurement);
                c.specifications.lower = d.line_number("lsl");
                c.specifications.upper = d.line_number("usl");
                if (!c.specifications.lower.has_value() || !c.specifications.upper.has_value()) {
                    return apply_error(QStringLiteral("规格不足"),
                                       QStringLiteral("非参数 Cnp 需要 LSL 与 USL。"));
                }
                c.nonparametric_capability.tolerance_k =
                    d.line_number("tolerance_k").value_or(6.0);
                return {};
            },
            AnalysisService::nonparametric_capability,
            QStringLiteral("过程能力")},
        {
            QStringLiteral("cox_regression"),
            QStringLiteral("Cox 回归"),
            QStringLiteral("Cox Regression"),
            QStringLiteral("统计"),
            QStringLiteral("report"),
            false, true,
            {{QStringLiteral("time"), QStringLiteral("时间"), false, false},
             {QStringLiteral("event"), QStringLiteral("事件/删失"), false, false},
             {QStringLiteral("covariates"), QStringLiteral("协变量"), true, false}},
            {{QStringLiteral("confidence"), QStringLiteral("置信水平"), QStringLiteral("0.95")},
             {QStringLiteral("ties"), QStringLiteral("Ties 方法"), QStringLiteral("breslow")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int time = d.first_role_index("time");
                const int event = d.first_role_index("event");
                const std::vector<int> covariates = d.role_indices("covariates");
                if (time < 0 || event < 0 || covariates.empty()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择时间列、事件列与至少一个协变量。"));
                }
                c.analysis_name = "Cox 回归";
                c.chart_type = "cox_regression";
                c.cox_regression.time_column = static_cast<std::size_t>(time);
                c.cox_regression.event_column = static_cast<std::size_t>(event);
                for (const int column : covariates) {
                    c.cox_regression.covariate_columns.push_back(static_cast<std::size_t>(column));
                }
                c.cox_regression.confidence_level =
                    d.line_number("confidence").value_or(c.inference.confidence_level);
                const std::string ties = normalize(d.line_text("ties"));
                if (!ties.empty()) {
                    c.cox_regression.ties_method = ties;
                }
                return {};
            },
            AnalysisService::cox_regression,
            QStringLiteral("可靠性")},
        {
            QStringLiteral("weibayes"),
            QStringLiteral("Weibayes"),
            QStringLiteral("Weibayes"),
            QStringLiteral("统计"),
            QStringLiteral("report"),
            false, true,
            {{QStringLiteral("time"), QStringLiteral("时间"), false, false},
             {QStringLiteral("event"), QStringLiteral("事件/删失"), false, false}},
            {{QStringLiteral("shape_prior"), QStringLiteral("形状先验 β"), QStringLiteral("2")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int time = d.first_role_index("time");
                const int event = d.first_role_index("event");
                if (time < 0 || event < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择时间列与事件列。"));
                }
                c.analysis_name = "Weibayes";
                c.chart_type = "weibayes";
                c.weibayes.time_column = static_cast<std::size_t>(time);
                c.weibayes.event_column = static_cast<std::size_t>(event);
                c.weibayes.shape_prior =
                    d.line_number("shape_prior").value_or(2.0);
                return {};
            },
            AnalysisService::weibayes,
            QStringLiteral("可靠性")},
        {
            QStringLiteral("nhpp_repairable"),
            QStringLiteral("可修复系统 NHPP"),
            QStringLiteral("NHPP Crow-AMSAA"),
            QStringLiteral("统计"),
            QStringLiteral("report"),
            false, true,
            {{QStringLiteral("time"), QStringLiteral("累积失效时间"), false, false}},
            {{QStringLiteral("truncation"), QStringLiteral("截尾 T（可空=max ti）"), QStringLiteral("")},
             {QStringLiteral("duane"), QStringLiteral("Duane 图 true/false"), QStringLiteral("true")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                const int time = d.first_role_index("time");
                if (time < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择累积失效时间列。"));
                }
                c.analysis_name = "可修复系统 NHPP";
                c.chart_type = "nhpp_repairable";
                c.nhpp_repairable.time_column = static_cast<std::size_t>(time);
                const auto truncation = d.line_number("truncation");
                if (truncation.has_value() && *truncation > 0.0) {
                    c.nhpp_repairable.truncation_time = *truncation;
                }
                const std::string duane = normalize(d.line_text("duane"));
                c.nhpp_repairable.include_duane_plot =
                    duane.empty() || duane == "true" || duane == "1" || duane == "yes";
                return {};
            },
            AnalysisService::nhpp_repairable,
            QStringLiteral("可靠性")},
        {
            QStringLiteral("reliability_test_plan"),
            QStringLiteral("可靠性试验计划"),
            QStringLiteral("Reliability Demonstration Test Plan"),
            QStringLiteral("统计"),
            QStringLiteral("report"),
            false, false,
            {},
            {{QStringLiteral("beta"), QStringLiteral("形状 β"), QStringLiteral("1")},
             {QStringLiteral("R"), QStringLiteral("目标可靠度 R"), QStringLiteral("0.9")},
             {QStringLiteral("CL"), QStringLiteral("置信水平 CL"), QStringLiteral("0.9")},
             {QStringLiteral("T0"), QStringLiteral("试验时长 T0"), QStringLiteral("1")},
             {QStringLiteral("tm"), QStringLiteral("任务时长 tm"), QStringLiteral("1")},
             {QStringLiteral("r"), QStringLiteral("允许失效数"), QStringLiteral("0")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                c.analysis_name = "可靠性试验计划";
                c.chart_type = "reliability_test_plan";
                c.reliability_test_plan.shape_beta =
                    d.line_number("beta").value_or(1.0);
                c.reliability_test_plan.target_reliability =
                    d.line_number("R").value_or(0.9);
                c.reliability_test_plan.confidence_level =
                    d.line_number("CL").value_or(0.9);
                c.reliability_test_plan.test_time =
                    d.line_number("T0").value_or(1.0);
                c.reliability_test_plan.mission_time =
                    d.line_number("tm").value_or(1.0);
                c.reliability_test_plan.allowed_failures = static_cast<std::size_t>(
                    std::max(0, d.line_int("r").value_or(0)));
                return {};
            },
            AnalysisService::reliability_test_plan,
            QStringLiteral("功效与样本量")},
        {
            QStringLiteral("binomial_capability"),
            QStringLiteral("二项过程能力"),
            QStringLiteral("二项过程能力分析"),
            QStringLiteral("质量工具"),
            QStringLiteral("capability"),
            false, true,
            {{QStringLiteral("defectives"), QStringLiteral("不合格品数"), false, false},
             {QStringLiteral("inspected"), QStringLiteral("检验数（列）"), false, true}},
            {{QStringLiteral("inspected_constant"), QStringLiteral("检验数（常数）"), QString()},
             {QStringLiteral("target"), QStringLiteral("目标不合格品率"),
              QStringLiteral("0–1 或百分数")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                c.analysis_name = "二项过程能力";
                c.chart_type = "binomial_capability";
                c.capability_method = "binomial";
                const int defectives = d.first_role_index("defectives");
                if (defectives < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择不合格品数列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defectives));
                c.selection.defect_count_column = static_cast<std::size_t>(defectives);
                if (d.first_role_index("inspected") >= 0) {
                    c.selection.inspected_count_column = static_cast<std::size_t>(
                        d.first_role_index("inspected"));
                }
                if (d.line_int("inspected_constant").has_value()) {
                    c.inspected_constant = static_cast<std::size_t>(
                        *d.line_int("inspected_constant"));
                }
                if (!c.selection.inspected_count_column.has_value()
                    && !c.inspected_constant.has_value()) {
                    return apply_error(QStringLiteral("未指定检验数"),
                                       QStringLiteral("请选择检验数列或输入检验数常数。"));
                }
                c.specifications.target = d.line_number("target");
                return {};
            },
            [](const DataTable& table, const AnalysisConfiguration& configuration) {
                return AnalysisService::binomial_capability(table, configuration);
            },
            QStringLiteral("过程能力")},
        {
            QStringLiteral("poisson_capability"),
            QStringLiteral("泊松过程能力"),
            QStringLiteral("泊松过程能力分析"),
            QStringLiteral("质量工具"),
            QStringLiteral("capability"),
            false, true,
            {{QStringLiteral("defects"), QStringLiteral("缺陷数"), false, false},
             {QStringLiteral("units"), QStringLiteral("单位数（列）"), false, true}},
            {{QStringLiteral("inspected_constant"), QStringLiteral("单位数（常数）"), QString()},
             {QStringLiteral("target"), QStringLiteral("目标 DPU"), QStringLiteral("可选")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d)
                -> AnalysisApplyResult {
                c.analysis_name = "泊松过程能力";
                c.chart_type = "poisson_capability";
                c.capability_method = "poisson";
                const int defects = d.first_role_index("defects");
                if (defects < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择缺陷数列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defects));
                c.selection.defect_count_column = static_cast<std::size_t>(defects);
                if (d.first_role_index("units") >= 0) {
                    c.selection.inspected_count_column = static_cast<std::size_t>(
                        d.first_role_index("units"));
                }
                if (d.line_int("inspected_constant").has_value()) {
                    c.inspected_constant = static_cast<std::size_t>(
                        *d.line_int("inspected_constant"));
                }
                if (!c.selection.inspected_count_column.has_value()
                    && !c.inspected_constant.has_value()) {
                    return apply_error(QStringLiteral("未指定单位数"),
                                       QStringLiteral("请选择单位数列或输入单位数常数。"));
                }
                c.specifications.target = d.line_number("target");
                return {};
            },
            [](const DataTable& table, const AnalysisConfiguration& configuration) {
                return AnalysisService::poisson_capability(table, configuration);
            },
            QStringLiteral("过程能力")},
        {
            QStringLiteral("nonnormal_capability"),
            QStringLiteral("非正态过程能力"),
            QStringLiteral("非正态过程能力分析"),
            QStringLiteral("质量工具"),
            QStringLiteral("capability"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false}},
            {{QStringLiteral("lsl"), QStringLiteral("LSL"), QStringLiteral("例如 73.95")},
             {QStringLiteral("usl"), QStringLiteral("USL"), QStringLiteral("例如 74.05")},
             {QStringLiteral("target"), QStringLiteral("Target"), QStringLiteral("可选")},
             {QStringLiteral("distribution"), QStringLiteral("分布"),
              QStringLiteral("weibull / lognormal")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "非正态过程能力";
                c.chart_type = "capability";
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                c.specifications.lower = d.line_number("lsl");
                c.specifications.upper = d.line_number("usl");
                c.specifications.target = d.line_number("target");
                c.capability_method = "non_normal";
                const std::string distribution = normalize(d.line_text("distribution"));
                c.nonnormal_distribution =
                    distribution == "lognormal" ? "lognormal" : "weibull";
                return {};
            },
            [](const DataTable& table, const AnalysisConfiguration& configuration) {
                return AnalysisService::capability(table, configuration);
            },
            QStringLiteral("过程能力")},
        {
            QStringLiteral("capability_sixpack"),
            QStringLiteral("过程能力 Sixpack"),
            QStringLiteral("过程能力 Sixpack"),
            QStringLiteral("质量工具"),
            QStringLiteral("capability_sixpack"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false}},
            {{QStringLiteral("subgroup_size"), QStringLiteral("子组大小"), QStringLiteral("1")},
             {QStringLiteral("lsl"), QStringLiteral("LSL"), QStringLiteral("例如 73.95")},
             {QStringLiteral("usl"), QStringLiteral("USL"), QStringLiteral("例如 74.05")},
             {QStringLiteral("target"), QStringLiteral("Target"), QStringLiteral("可选")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "过程能力 Sixpack";
                c.chart_type = "capability_sixpack";
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                c.control.subgroup_size = d.line_int("subgroup_size").value_or(1);
                c.specifications.lower = d.line_number("lsl");
                c.specifications.upper = d.line_number("usl");
                c.specifications.target = d.line_number("target");
                return {};
            },
            AnalysisService::capability_sixpack,
            QStringLiteral("过程能力")},
        {
            QStringLiteral("box_cox"),
            QStringLiteral("Box-Cox 变换"),
            QStringLiteral("Box-Cox 变换"),
            QStringLiteral("质量工具"),
            QStringLiteral("box_cox"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("正值变量"), false, false}},
            {{QStringLiteral("lambda"), QStringLiteral("Lambda（可选）"),
              QStringLiteral("留空自动搜索 -5 到 5")},
             {QStringLiteral("lsl"), QStringLiteral("LSL（可选）"), QStringLiteral("规格下限")},
             {QStringLiteral("usl"), QStringLiteral("USL（可选）"), QStringLiteral("规格上限")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int column = d.first_role_index("variables");
                if (column < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择一个正值变量。"));
                }
                c.analysis_name = "Box-Cox 变换";
                c.chart_type = "box_cox";
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.inference.hypothesis_mean = d.line_number("lambda");
                c.specifications.lower = d.line_number("lsl");
                c.specifications.upper = d.line_number("usl");
                return {};
            },
            AnalysisService::box_cox,
            QStringLiteral("变换 / 分布")},
        {
            QStringLiteral("gage_rr"),
            QStringLiteral("Crossed Gage R&R"),
            QStringLiteral("Crossed Gage R&R"),
            QStringLiteral("质量工具"),
            QStringLiteral("gage_rr"),
            false, true,
            {{QStringLiteral("measurement"), QStringLiteral("测量值"), false, false},
             {QStringLiteral("part"), QStringLiteral("零件"), false, false},
             {QStringLiteral("operator"), QStringLiteral("操作员"), false, false}},
            {{QStringLiteral("lsl"), QStringLiteral("LSL（可选）"), QStringLiteral("规格下限")},
             {QStringLiteral("usl"), QStringLiteral("USL（可选）"), QStringLiteral("规格上限")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int measurement = d.first_role_index("measurement");
                const int part = d.first_role_index("part");
                const int oper = d.first_role_index("operator");
                if (measurement < 0 || part < 0 || oper < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量值、零件和操作员列。"));
                }
                c.analysis_name = "Crossed Gage R&R";
                c.chart_type = "gage_rr";
                c.msa.gage_measurement_column = static_cast<std::size_t>(measurement);
                c.msa.gage_part_column = static_cast<std::size_t>(part);
                c.msa.gage_operator_column = static_cast<std::size_t>(oper);
                c.specifications.lower = d.line_number("lsl");
                c.specifications.upper = d.line_number("usl");
                return {};
            },
            AnalysisService::gage_rr,
            QStringLiteral("MSA")},
        {
            QStringLiteral("emp_crossed"),
            QStringLiteral("EMP Crossed"),
            QStringLiteral("EMP Crossed（Wheeler）"),
            QStringLiteral("质量工具"),
            QStringLiteral("gage_rr"),
            false, true,
            {{QStringLiteral("measurement"), QStringLiteral("测量值"), false, false},
             {QStringLiteral("part"), QStringLiteral("零件"), false, false},
             {QStringLiteral("operator"), QStringLiteral("操作员"), false, false}},
            {},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int measurement = d.first_role_index("measurement");
                const int part = d.first_role_index("part");
                const int oper = d.first_role_index("operator");
                if (measurement < 0 || part < 0 || oper < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量值、零件和操作员列。"));
                }
                c.analysis_name = "EMP Crossed";
                c.chart_type = "emp_crossed";
                c.msa.gage_measurement_column = static_cast<std::size_t>(measurement);
                c.msa.gage_part_column = static_cast<std::size_t>(part);
                c.msa.gage_operator_column = static_cast<std::size_t>(oper);
                return {};
            },
            AnalysisService::emp_crossed,
            QStringLiteral("MSA")},
        {
            QStringLiteral("expanded_gage_rr"),
            QStringLiteral("Expanded Gage R&R"),
            QStringLiteral("Expanded Gage R&R（三因子平衡）"),
            QStringLiteral("质量工具"),
            QStringLiteral("gage_rr"),
            false, true,
            {{QStringLiteral("measurement"), QStringLiteral("测量值"), false, false},
             {QStringLiteral("part"), QStringLiteral("零件"), false, false},
             {QStringLiteral("operator"), QStringLiteral("操作员"), false, false},
             {QStringLiteral("additional"), QStringLiteral("附加因子"), false, false}},
            {{QStringLiteral("tolerance"), QStringLiteral("公差（可选）"), QString()}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int measurement = d.first_role_index("measurement");
                const int part = d.first_role_index("part");
                const int oper = d.first_role_index("operator");
                const int additional = d.first_role_index("additional");
                if (measurement < 0 || part < 0 || oper < 0 || additional < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量值、零件、操作员与附加因子列。"));
                }
                c.analysis_name = "Expanded Gage R&R";
                c.chart_type = "expanded_gage_rr";
                c.msa.gage_measurement_column = static_cast<std::size_t>(measurement);
                c.msa.gage_part_column = static_cast<std::size_t>(part);
                c.msa.gage_operator_column = static_cast<std::size_t>(oper);
                c.msa.gage_additional_column = static_cast<std::size_t>(additional);
                c.msa.gage_tolerance = d.line_number("tolerance").value_or(0.0);
                return {};
            },
            AnalysisService::expanded_gage_rr,
            QStringLiteral("MSA")},
        {
            QStringLiteral("msa_type1"),
            QStringLiteral("MSA Type 1 / Bias / Stability"),
            QStringLiteral("MSA Type 1 / Bias / Stability"),
            QStringLiteral("质量工具"),
            QStringLiteral("gage_rr"),
            false, true,
            {{QStringLiteral("measurement"), QStringLiteral("测量值"), false, false},
             {QStringLiteral("reference"), QStringLiteral("参考值列（Linearity）"), false, true}},
            {{QStringLiteral("mode"), QStringLiteral("模式"),
              QStringLiteral("type1 / bias_linearity / stability")},
             {QStringLiteral("reference_value"), QStringLiteral("参考值（Type 1）"), QString()},
             {QStringLiteral("tolerance"), QStringLiteral("公差宽度（可选）"), QString()},
             {QStringLiteral("lsl"), QStringLiteral("LSL（可选）"), QStringLiteral("规格下限")},
             {QStringLiteral("usl"), QStringLiteral("USL（可选）"), QStringLiteral("规格上限")},
             {QStringLiteral("process_variation"), QStringLiteral("过程变差 6σ（Linearity 可选）"),
              QStringLiteral("来自 Gage R&R Total Study Var 或历史 6σ")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                const int measurement = d.first_role_index("measurement");
                if (measurement < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.analysis_name = "MSA Type 1 / Bias / Stability";
                c.chart_type = "msa_type1";
                c.msa.gage_measurement_column = static_cast<std::size_t>(measurement);
                c.msa.mode = normalize(d.line_text("mode"));
                if (c.msa.mode != "bias_linearity" && c.msa.mode != "stability") {
                    c.msa.mode = "type1";
                }
                if (d.first_role_index("reference") >= 0) {
                    c.msa.reference_column = static_cast<std::size_t>(
                        d.first_role_index("reference"));
                }
                c.msa.reference_value = d.line_number("reference_value");
                c.msa.gage_tolerance = d.line_number("tolerance").value_or(0.0);
                c.msa.process_variation = d.line_number("process_variation");
                c.specifications.lower = d.line_number("lsl");
                c.specifications.upper = d.line_number("usl");
                return {};
            },
            AnalysisService::msa_type1,
            QStringLiteral("MSA")},
        {
            QStringLiteral("nested_gage_rr"),
            QStringLiteral("Nested Gage R&R"),
            QStringLiteral("Nested Gage R&R"),
            QStringLiteral("质量工具"),
            QStringLiteral("nested_gage_rr"),
            false, true,
            {{QStringLiteral("measurement"), QStringLiteral("测量值"), false, false},
             {QStringLiteral("part"), QStringLiteral("部件"), false, false},
             {QStringLiteral("operator"), QStringLiteral("操作者"), false, false}},
            {{QStringLiteral("tolerance"), QStringLiteral("公差"), QStringLiteral("可选")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "Nested Gage R&R";
                c.chart_type = "nested_gage_rr";
                c.msa.nested_measurement_column = static_cast<std::size_t>(
                    d.first_role_index("measurement"));
                c.msa.nested_part_column = static_cast<std::size_t>(
                    d.first_role_index("part"));
                c.msa.nested_operator_column = static_cast<std::size_t>(
                    d.first_role_index("operator"));
                c.msa.gage_tolerance = d.line_number("tolerance").value_or(0.0);
                return {};
            },
            AnalysisService::nested_gage_rr,
            QStringLiteral("MSA")},
        {
            QStringLiteral("attribute_agreement"),
            QStringLiteral("属性一致性分析"),
            QStringLiteral("属性一致性分析"),
            QStringLiteral("质量工具"),
            QStringLiteral("attribute_agreement"),
            false, true,
            {{QStringLiteral("rating"), QStringLiteral("评级"), false, false},
             {QStringLiteral("part"), QStringLiteral("部件"), false, false},
             {QStringLiteral("appraiser"), QStringLiteral("评估者"), false, false},
             {QStringLiteral("standard"), QStringLiteral("标准（可选）"), false, true}},
            {{QStringLiteral("ordinal"), QStringLiteral("有序评级（true 时计算 Kendall W/τ）"),
              QStringLiteral("false")},
             {QStringLiteral("kappa_weight"),
              QStringLiteral("Kappa 权重"),
              QStringLiteral("none / linear / quadratic")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "属性一致性分析";
                c.chart_type = "attribute_agreement";
                c.msa.attribute_rating_column = static_cast<std::size_t>(
                    d.first_role_index("rating"));
                c.msa.attribute_part_column = static_cast<std::size_t>(
                    d.first_role_index("part"));
                c.msa.attribute_appraiser_column = static_cast<std::size_t>(
                    d.first_role_index("appraiser"));
                const int standard = d.first_role_index("standard");
                if (standard >= 0) {
                    c.msa.attribute_standard_column = static_cast<std::size_t>(standard);
                }
                const std::string ordinal = normalize(d.line_text("ordinal"));
                c.msa.ratings_are_ordinal = ordinal == "true" || ordinal == "1"
                    || ordinal == "yes";
                const std::string weight = normalize(d.line_text("kappa_weight"));
                if (weight == "linear" || weight == "quadratic") {
                    c.msa.kappa_weight_scheme = weight;
                } else {
                    c.msa.kappa_weight_scheme = "none";
                }
                return {};
            },
            AnalysisService::attribute_agreement,
            QStringLiteral("MSA")},
        {
            QStringLiteral("doe_factorial"),
            QStringLiteral("2 水平析因设计生成"),
            QStringLiteral("2 水平全因子 / 部分析因设计"),
            QStringLiteral("统计"),
            QStringLiteral("doe_factorial"),
            false, false,
            {{QStringLiteral("response"), QStringLiteral("响应列（可选，选择后进行响应分析）"),
              false, true},
             {QStringLiteral("factor_columns"), QStringLiteral("已导入因子列（可选，多选）"),
              true, true}},
            {{QStringLiteral("factors"), QStringLiteral("因子名（逗号分隔）"),
              QStringLiteral("A,B,C,D")},
             {QStringLiteral("low"), QStringLiteral("低水平（逗号分隔）"), QStringLiteral("-1,-1,-1,-1")},
             {QStringLiteral("high"), QStringLiteral("高水平（逗号分隔）"), QStringLiteral("1,1,1,1")},
             {QStringLiteral("fraction_p"), QStringLiteral("部分析因 p（0=全因子）"),
              QStringLiteral("0")},
             {QStringLiteral("generators"), QStringLiteral("生成器（可空=默认；如 D=ABC）"),
              QStringLiteral("")},
             {QStringLiteral("centers"), QStringLiteral("中心点数"), QStringLiteral("0")},
             {QStringLiteral("blocks"), QStringLiteral("区组数"), QStringLiteral("1")},
             {QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("0")},
             {QStringLiteral("x_factor"), QStringLiteral("等值线 X 因子（可空=第1个）"),
              QStringLiteral("")},
             {QStringLiteral("y_factor"), QStringLiteral("等值线 Y 因子（可空=第2个）"),
              QStringLiteral("")},
             {QStringLiteral("hold"), QStringLiteral("非轴因子实际 hold（名=值;…，可空=编码0）"),
              QStringLiteral("")}},
            doe_apply,
            doe_run,
            QStringLiteral("DOE")},
        {
            QStringLiteral("doe_response"),
            QStringLiteral("DOE 响应分析"),
            QStringLiteral("2 水平析因响应分析"),
            QStringLiteral("统计"),
            QStringLiteral("doe_factorial"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应列（可选，选择后进行响应分析）"),
              false, true},
             {QStringLiteral("factor_columns"), QStringLiteral("已导入因子列（可选，多选）"),
              true, true}},
            {{QStringLiteral("factors"), QStringLiteral("因子名（逗号分隔）"),
              QStringLiteral("Temperature,Pressure")},
             {QStringLiteral("low"), QStringLiteral("低水平（逗号分隔）"), QStringLiteral("-1,-1")},
             {QStringLiteral("high"), QStringLiteral("高水平（逗号分隔）"), QStringLiteral("1,1")},
             {QStringLiteral("fraction_p"), QStringLiteral("部分析因 p（设计生成用）"),
              QStringLiteral("0")},
             {QStringLiteral("generators"), QStringLiteral("生成器（设计生成用）"),
              QStringLiteral("")},
             {QStringLiteral("centers"), QStringLiteral("中心点数"), QStringLiteral("0")},
             {QStringLiteral("blocks"), QStringLiteral("区组数"), QStringLiteral("1")},
             {QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("0")},
             {QStringLiteral("x_factor"), QStringLiteral("等值线 X 因子（可空=第1个）"),
              QStringLiteral("")},
             {QStringLiteral("y_factor"), QStringLiteral("等值线 Y 因子（可空=第2个）"),
              QStringLiteral("")},
             {QStringLiteral("hold"), QStringLiteral("非轴因子实际 hold（名=值;…，可空=编码0）"),
              QStringLiteral("")}},
            doe_apply,
            doe_run,
            QStringLiteral("DOE")},
        {
            QStringLiteral("rsm_response"),
            QStringLiteral("响应曲面分析"),
            QStringLiteral("响应曲面分析（RSM）"),
            QStringLiteral("统计"),
            QStringLiteral("doe_factorial"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应"), false, false},
             {QStringLiteral("factors"), QStringLiteral("因子（≥2）"), true, false}},
            {{QStringLiteral("design_source_id"), QStringLiteral("设计来源 ID（可选）"), QStringLiteral("")},
             {QStringLiteral("design_kind"), QStringLiteral("设计族 ccd/bbd（可选）"), QStringLiteral("")},
             {QStringLiteral("coding_low"), QStringLiteral("设计低水平（逗号，可选）"), QStringLiteral("")},
             {QStringLiteral("coding_high"), QStringLiteral("设计高水平（逗号，可选）"), QStringLiteral("")},
             {QStringLiteral("coding_center"), QStringLiteral("设计中心（逗号，可选）"), QStringLiteral("")}},
            [](AnalysisConfiguration& c, const datalab::application::AnalysisIntent& d) -> AnalysisApplyResult {
                c.analysis_name = "响应曲面分析";
                c.chart_type = "rsm_response";
                const int response = d.first_role_index("response");
                if (response < 0) {
                    return apply_error(QStringLiteral("未选择响应"),
                                       QStringLiteral("请选择响应列。"));
                }
                c.variable_columns.clear();
                c.variable_columns.push_back(static_cast<std::size_t>(response));
                for (const int index : d.role_indices("factors")) {
                    c.variable_columns.push_back(static_cast<std::size_t>(index));
                }
                if (c.variable_columns.size() < 3) {
                    return apply_error(QStringLiteral("因子不足"),
                                       QStringLiteral("RSM 至少需要两个连续因子。"));
                }
                auto& design = c.response_surface_design;
                design.design_source_id = normalize(d.line_text("design_source_id"));
                const std::string kind = normalize(d.line_text("design_kind"));
                if (kind == "ccd" || kind == "bbd") {
                    design.design_kind = kind;
                }
                const QString lows_text =
                    QString::fromStdString(normalize(d.line_text("coding_low"))).trimmed();
                const QString highs_text =
                    QString::fromStdString(normalize(d.line_text("coding_high"))).trimmed();
                const QString centers_text =
                    QString::fromStdString(normalize(d.line_text("coding_center"))).trimmed();
                design.low_levels.clear();
                design.high_levels.clear();
                design.centers.clear();
                if (!lows_text.isEmpty() && !highs_text.isEmpty()) {
                    const QStringList lows = lows_text.split(QLatin1Char(','), Qt::SkipEmptyParts);
                    const QStringList highs = highs_text.split(QLatin1Char(','), Qt::SkipEmptyParts);
                    const QStringList centers = centers_text.isEmpty()
                        ? QStringList{}
                        : centers_text.split(QLatin1Char(','), Qt::KeepEmptyParts);
                    if (lows.size() != highs.size()) {
                        return apply_error(QStringLiteral("编码边界不匹配"),
                                           QStringLiteral("coding_low 与 coding_high 数量须一致。"));
                    }
                    for (int i = 0; i < lows.size(); ++i) {
                        bool ok_l = false;
                        bool ok_h = false;
                        design.low_levels.push_back(lows[i].trimmed().toDouble(&ok_l));
                        design.high_levels.push_back(highs[i].trimmed().toDouble(&ok_h));
                        if (!ok_l || !ok_h) {
                            return apply_error(QStringLiteral("编码边界无效"),
                                               QStringLiteral("设计低/高水平必须为数值。"));
                        }
                        if (!centers_text.isEmpty()) {
                            if (i < centers.size() && !centers[i].trimmed().isEmpty()) {
                                bool ok_c = false;
                                design.centers.push_back(centers[i].trimmed().toDouble(&ok_c));
                                if (!ok_c) {
                                    return apply_error(QStringLiteral("编码中心无效"),
                                                       QStringLiteral("设计中心必须为数值。"));
                                }
                            } else {
                                design.centers.push_back(
                                    0.5 * (design.low_levels.back() + design.high_levels.back()));
                            }
                        }
                    }
                }
                return {};
            },
            AnalysisService::rsm_response,
            QStringLiteral("DOE")},
        {
            QStringLiteral("response_optimization"),
            QStringLiteral("DOE 响应优化"),
            QStringLiteral("DOE 响应优化"),
            QStringLiteral("统计"),
            QStringLiteral("doe_factorial"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("响应列（可多选）"), true, false},
             {QStringLiteral("factor_columns"), QStringLiteral("已导入因子列（多选）"),
              true, false}},
            {{QStringLiteral("goal"), QStringLiteral("优化目标"), QStringLiteral("maximize")},
             {QStringLiteral("lower"), QStringLiteral("下限（可空，默认观测最小）"), QStringLiteral("")},
             {QStringLiteral("upper"), QStringLiteral("上限（可空，默认观测最大）"), QStringLiteral("")},
             {QStringLiteral("target"), QStringLiteral("目标值（目标优化时使用）"), QStringLiteral("")},
             {QStringLiteral("weight"), QStringLiteral("权重"), QStringLiteral("1")},
             {QStringLiteral("objectives"), QStringLiteral("各响应独立目标"), QStringLiteral("")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平"), QStringLiteral("0.95")},
             {QStringLiteral("low"), QStringLiteral("低水平（逗号分隔，可选）"), QStringLiteral("-1,-1")},
             {QStringLiteral("high"), QStringLiteral("高水平（逗号分隔，可选）"), QStringLiteral("1,1")}},
            response_optimization_apply,
            AnalysisService::response_optimization,
            QStringLiteral("DOE")},
    };
    return commands;
}

const AnalysisCommand* find(const QString& id)
{
    const auto& commands = all();
    for (const auto& command : commands) {
        if (command.id == id) {
            return &command;
        }
    }
    return nullptr;
}

}  // namespace analysis_commands
