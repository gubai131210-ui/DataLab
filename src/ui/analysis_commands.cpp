#include "ui/analysis_commands.h"

#include "application/analysis_service.h"
#include "ui/analysis_setup_dialog.h"

#include <QList>
#include <QRegularExpression>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace analysis_commands {

namespace {

using datalab::application::AnalysisService;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::OutputPage;

using ApplyFn = std::function<AnalysisApplyResult(
    AnalysisConfiguration&, const AnalysisSetupDialog&)>;
using RunFn = std::function<OutputPage(
    const DataTable&, const AnalysisConfiguration&)>;

// 校验失败：弹出提示后中止（对应原 run_* 的 QMessageBox::information）。
AnalysisApplyResult apply_error(const QString& title, const QString& message)
{
    return {false, title, message};
}

// 校验失败：静默中止（对应原 run_* 中无提示的裸 return）。
AnalysisApplyResult apply_silent()
{
    return {false, {}, {}};
}

// DOE 2 水平全因子：doe_factorial 与 doe_response 共用配置构建与执行。
const ApplyFn doe_apply = [](AnalysisConfiguration& c,
                             const AnalysisSetupDialog& d) -> AnalysisApplyResult {
    auto split = [](const QString& text) {
        std::vector<std::string> values;
        for (const QString& value : text.split(',', Qt::SkipEmptyParts)) {
            values.push_back(value.trimmed().toStdString());
        }
        return values;
    };
    c.analysis_name = "2 水平全因子设计";
    c.chart_type = "doe_factorial";
    c.doe_factor_names = split(d.line_text(QStringLiteral("factors")));
    c.doe_low_levels = split(d.line_text(QStringLiteral("low")));
    c.doe_high_levels = split(d.line_text(QStringLiteral("high")));
    c.doe_center_point_count = static_cast<std::size_t>(
        d.line_int(QStringLiteral("centers")).value_or(0));
    c.doe_block_count = static_cast<std::size_t>(
        std::max(1, d.line_int(QStringLiteral("blocks")).value_or(1)));
    c.doe_randomize = true;
    c.doe_random_seed = static_cast<std::uint64_t>(
        std::max(0, d.line_int(QStringLiteral("seed")).value_or(0)));
    const int response_column = d.first_role_index(QStringLiteral("response"));
    if (response_column >= 0) {
        c.doe_response_column = static_cast<std::size_t>(response_column);
        const QList<int> factor_columns =
            d.role_indices(QStringLiteral("factor_columns"));
        for (const int column : factor_columns) {
            c.doe_factor_columns.push_back(static_cast<std::size_t>(column));
        }
        if (c.doe_factor_columns.empty()) {
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

}  // namespace

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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "显示描述性统计";
                c.chart_type = "descriptive";
                for (const int index : d.role_indices(QStringLiteral("variables"))) {
                    c.variable_columns.push_back(static_cast<std::size_t>(index));
                }
                if (d.first_role_index(QStringLiteral("by")) >= 0) {
                    c.by_column = static_cast<std::size_t>(
                        d.first_role_index(QStringLiteral("by")));
                }
                if (c.variable_columns.empty()) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请从左侧选择至少一列到“变量”。"));
                }
                return {};
            },
            AnalysisService::descriptive},
        {
            QStringLiteral("normality_test"),
            QStringLiteral("正态性检验"),
            QStringLiteral("正态性检验"),
            QStringLiteral("统计"),
            QStringLiteral("normality_test"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false}},
            {},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int column = d.first_role_index(QStringLiteral("variables"));
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.analysis_name = "正态性检验";
                c.chart_type = "normality_test";
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                return {};
            },
            AnalysisService::normality_test},
        {
            QStringLiteral("correlation"),
            QStringLiteral("相关分析"),
            QStringLiteral("相关分析"),
            QStringLiteral("统计"),
            QStringLiteral("correlation"),
            true, true,
            {{QStringLiteral("variables"), QStringLiteral("变量（至少两列）"), true, false}},
            {{QStringLiteral("method"), QStringLiteral("方法"), QStringLiteral("pearson 或 spearman")},
             {QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"), QStringLiteral("95")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const QList<int> columns = d.role_indices(QStringLiteral("variables"));
                if (columns.size() < 2) {
                    return apply_error(QStringLiteral("变量不足"),
                                       QStringLiteral("请选择至少两列数值变量。"));
                }
                c.analysis_name = "相关分析";
                c.chart_type = "correlation";
                for (const int column : columns) {
                    c.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                c.correlation_method = d.line_text(QStringLiteral("method")).trimmed()
                    .toLower().toStdString();
                if (c.correlation_method != "spearman") {
                    c.correlation_method = "pearson";
                }
                const double confidence = d.line_number(QStringLiteral("confidence")).value_or(95.0);
                c.confidence_level = confidence > 1.0 ? confidence / 100.0 : confidence;
                return {};
            },
            AnalysisService::correlation},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int column = d.first_role_index(QStringLiteral("variables"));
                const auto hypothesis = d.line_number(QStringLiteral("hypothesis_mean"));
                if (column < 0 || !hypothesis.has_value()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量列并输入假设均值。"));
                }
                c.analysis_name = "单样本 t 检验";
                c.chart_type = "one_sample_t";
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                c.hypothesis_mean = hypothesis;
                c.confidence_level = d.line_number(QStringLiteral("confidence")).value_or(95.0);
                if (c.confidence_level > 1.0) {
                    c.confidence_level /= 100.0;
                }
                c.alternative = d.line_text(QStringLiteral("alternative")).trimmed()
                    .toLower().toStdString();
                return {};
            },
            AnalysisService::one_sample_t},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const QList<int> columns = d.role_indices(QStringLiteral("variables"));
                if (columns.size() != 2) {
                    return apply_error(QStringLiteral("变量数量错误"),
                                       QStringLiteral("请选择正好两列独立样本变量。"));
                }
                c.analysis_name = "双样本 t 检验";
                c.chart_type = "two_sample_t";
                c.variable_columns = {
                    static_cast<std::size_t>(columns[0]), static_cast<std::size_t>(columns[1])};
                c.variance_method = d.line_text(QStringLiteral("variance")).trimmed()
                    .toLower().toStdString();
                if (c.variance_method != "pooled") {
                    c.variance_method = "welch";
                }
                c.confidence_level = d.line_number(QStringLiteral("confidence")).value_or(95.0);
                if (c.confidence_level > 1.0) {
                    c.confidence_level /= 100.0;
                }
                c.alternative = d.line_text(QStringLiteral("alternative")).trimmed()
                    .toLower().toStdString();
                return {};
            },
            AnalysisService::two_sample_t},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int response = d.first_role_index(QStringLiteral("response"));
                const int factor = d.first_role_index(QStringLiteral("factor"));
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
            AnalysisService::one_way_anova},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const QList<int> columns = d.role_indices(QStringLiteral("variables"));
                if (columns.size() != 2) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("配对 t 检验必须选择恰好两列。"));
                }
                c.analysis_name = "配对 t 检验";
                c.chart_type = "paired_t";
                c.variable_columns = {
                    static_cast<std::size_t>(columns[0]), static_cast<std::size_t>(columns[1])};
                const auto confidence = d.line_number(QStringLiteral("confidence"));
                if (confidence.has_value()) {
                    c.confidence_level = *confidence > 1.0 ? *confidence / 100.0 : *confidence;
                }
                c.alternative = d.line_text(QStringLiteral("alternative"))
                    .trimmed().toLower().toStdString();
                return {};
            },
            AnalysisService::paired_t},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const QList<int> columns = d.role_indices(QStringLiteral("variables"));
                if (columns.size() < 2) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("回归至少需要一列响应变量和一列预测变量。"));
                }
                c.analysis_name = "线性回归";
                c.chart_type = "regression";
                for (const int column : columns) {
                    c.variable_columns.push_back(static_cast<std::size_t>(column));
                }
                const auto confidence = d.line_number(QStringLiteral("confidence"));
                if (confidence.has_value()) {
                    c.confidence_level = *confidence > 1.0 ? *confidence / 100.0 : *confidence;
                }
                return {};
            },
            AnalysisService::regression},
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
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int first_events = d.first_role_index(QStringLiteral("first_events"));
                const int first_trials = d.first_role_index(QStringLiteral("first_trials"));
                const int second_events = d.first_role_index(QStringLiteral("second_events"));
                const int second_trials = d.first_role_index(QStringLiteral("second_trials"));
                if (first_events < 0 || first_trials < 0 || second_events < 0 || second_trials < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择四个计数列。"));
                }
                c.analysis_name = "两比例检验";
                c.chart_type = "two_proportions";
                c.first_events_column = static_cast<std::size_t>(first_events);
                c.first_trials_column = static_cast<std::size_t>(first_trials);
                c.second_events_column = static_cast<std::size_t>(second_events);
                c.second_trials_column = static_cast<std::size_t>(second_trials);
                const auto confidence = d.line_number(QStringLiteral("confidence"));
                if (confidence.has_value()) {
                    c.confidence_level = *confidence > 1.0 ? *confidence / 100.0 : *confidence;
                }
                c.alternative = d.line_text(QStringLiteral("alternative"))
                    .trimmed().toLower().toStdString();
                return {};
            },
            AnalysisService::two_proportions},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int row = d.first_role_index(QStringLiteral("row_category"));
                const int column = d.first_role_index(QStringLiteral("column_category"));
                if (row < 0 || column < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择行分类列和列分类列。"));
                }
                c.analysis_name = "列联表卡方";
                c.chart_type = "chi_square";
                c.row_category_column = static_cast<std::size_t>(row);
                c.column_category_column = static_cast<std::size_t>(column);
                return {};
            },
            AnalysisService::chi_square},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const QList<int> columns = d.role_indices(QStringLiteral("variables"));
                if (columns.size() != 2) {
                    return apply_error(QStringLiteral("变量数量错误"),
                                       QStringLiteral("请选择正好两列独立样本。"));
                }
                c.analysis_name = "Mann-Whitney 检验";
                c.chart_type = "mann_whitney";
                c.variable_columns = {
                    static_cast<std::size_t>(columns[0]), static_cast<std::size_t>(columns[1])};
                c.alternative = d.line_text(QStringLiteral("alternative"))
                    .trimmed().toLower().toStdString();
                return {};
            },
            AnalysisService::mann_whitney},
        {
            QStringLiteral("wilcoxon_signed_rank"),
            QStringLiteral("Wilcoxon 符号秩检验"),
            QStringLiteral("Wilcoxon 符号秩检验"),
            QStringLiteral("统计"),
            QStringLiteral("wilcoxon_signed_rank"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("两列配对样本"), true, false}},
            {{QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const QList<int> columns = d.role_indices(QStringLiteral("variables"));
                if (columns.size() != 2) {
                    return apply_error(QStringLiteral("变量数量错误"),
                                       QStringLiteral("请选择正好两列配对样本。"));
                }
                c.analysis_name = "Wilcoxon 符号秩检验";
                c.chart_type = "wilcoxon_signed_rank";
                c.variable_columns = {
                    static_cast<std::size_t>(columns[0]), static_cast<std::size_t>(columns[1])};
                c.alternative = d.line_text(QStringLiteral("alternative"))
                    .trimmed().toLower().toStdString();
                return {};
            },
            AnalysisService::wilcoxon_signed_rank},
        {
            QStringLiteral("kruskal_wallis"),
            QStringLiteral("Kruskal-Wallis 检验"),
            QStringLiteral("Kruskal-Wallis 检验"),
            QStringLiteral("统计"),
            QStringLiteral("kruskal_wallis"),
            false, true,
            {{QStringLiteral("response"), QStringLiteral("测量值"), false, false},
             {QStringLiteral("factor"), QStringLiteral("分组列"), false, false}},
            {},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int response = d.first_role_index(QStringLiteral("response"));
                const int factor = d.first_role_index(QStringLiteral("factor"));
                if (response < 0 || factor < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量值和分组列。"));
                }
                c.analysis_name = "Kruskal-Wallis 检验";
                c.chart_type = "kruskal_wallis";
                c.variable_columns = {static_cast<std::size_t>(response)};
                c.by_column = static_cast<std::size_t>(factor);
                return {};
            },
            AnalysisService::kruskal_wallis},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int column = d.first_role_index(QStringLiteral("variables"));
                if (column < 0) {
                    return apply_silent();
                }
                c.analysis_name = "时间序列平滑";
                c.chart_type = "time_series_smoothing";
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.smoothing_method = d.line_text(QStringLiteral("method")).trimmed()
                    .toLower().toStdString();
                if (c.smoothing_method != "single") {
                    c.smoothing_method = "double";
                }
                c.smoothing_alpha = d.line_number(QStringLiteral("alpha")).value_or(0.2);
                c.smoothing_gamma = d.line_number(QStringLiteral("gamma")).value_or(0.2);
                c.forecast_periods = d.line_int(QStringLiteral("periods")).value_or(1);
                return {};
            },
            AnalysisService::time_series_smoothing},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int value = d.first_role_index(QStringLiteral("value"));
                if (value < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择时间序列值列。"));
                }
                c.analysis_name = "ARIMA 基础预测";
                c.chart_type = "arima";
                const int time = d.first_role_index(QStringLiteral("time"));
                if (time >= 0) {
                    c.arima_time_column = static_cast<std::size_t>(time);
                }
                c.arima_value_column = static_cast<std::size_t>(value);
                c.arima_selection_criterion = d.line_text(QStringLiteral("criterion")).trimmed()
                    .toLower().toStdString();
                if (c.arima_selection_criterion != "aic"
                    && c.arima_selection_criterion != "bic") {
                    c.arima_selection_criterion = "aicc";
                }
                c.forecast_periods = d.line_int(QStringLiteral("periods")).value_or(3);
                return {};
            },
            AnalysisService::arima},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int response = d.first_role_index(QStringLiteral("response"));
                const int factor_a = d.first_role_index(QStringLiteral("factor_a"));
                const int factor_b = d.first_role_index(QStringLiteral("factor_b"));
                if (response < 0 || factor_a < 0 || factor_b < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择响应变量、因子 A 和因子 B。"));
                }
                c.analysis_name = "双因素 ANOVA";
                c.chart_type = "two_factor_anova";
                c.anova_response_column = static_cast<std::size_t>(response);
                c.anova_factor_a_column = static_cast<std::size_t>(factor_a);
                c.anova_factor_b_column = static_cast<std::size_t>(factor_b);
                c.anova_factor_encoding = d.line_text(QStringLiteral("encoding")).trimmed()
                    .toLower().toStdString();
                if (c.anova_factor_encoding != "effect") {
                    c.anova_factor_encoding = "reference";
                }
                return {};
            },
            AnalysisService::two_factor_anova},
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
             {QStringLiteral("tolerance"), QStringLiteral("收敛阈值"), QStringLiteral("1e-8")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int response = d.first_role_index(QStringLiteral("response"));
                const QList<int> predictors = d.role_indices(QStringLiteral("predictors"));
                if (response < 0 || predictors.isEmpty()) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择一个二元响应列和至少一个预测变量。"));
                }
                c.analysis_name = "二元 Logistic 回归";
                c.chart_type = "logistic_regression";
                c.logistic_response_column = static_cast<std::size_t>(response);
                for (const int predictor : predictors) {
                    c.logistic_predictor_columns.push_back(
                        static_cast<std::size_t>(predictor));
                }
                c.logistic_event_level = d.line_text(QStringLiteral("event")).trimmed()
                    .toStdString();
                if (c.logistic_event_level.empty()) {
                    c.logistic_event_level = "1";
                }
                c.logistic_max_iterations = d.line_int(QStringLiteral("iterations")).value_or(100);
                c.logistic_tolerance = d.line_number(QStringLiteral("tolerance")).value_or(1.0e-8);
                return {};
            },
            AnalysisService::logistic_regression},
        {
            QStringLiteral("variance_test"),
            QStringLiteral("方差检验"),
            QStringLiteral("方差检验"),
            QStringLiteral("统计"),
            QStringLiteral("variance_test"),
            false, true,
            {{QStringLiteral("first"), QStringLiteral("第一样本"), false, false},
             {QStringLiteral("second"), QStringLiteral("第二样本（两方差）"), false, false}},
            {{QStringLiteral("hypothesis"), QStringLiteral("假设方差（一方差）"),
              QStringLiteral("例如 1.0")},
             {QStringLiteral("method"), QStringLiteral("两方差方法"),
              QStringLiteral("f / levene / brown_forsythe")},
             {QStringLiteral("alternative"), QStringLiteral("备择方向"),
              QStringLiteral("two_sided / less / greater")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int first = d.first_role_index(QStringLiteral("first"));
                const int second = d.first_role_index(QStringLiteral("second"));
                if (first < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择第一样本列。"));
                }
                c.analysis_name = "方差检验";
                c.chart_type = "variance_test";
                c.variance_first_column = static_cast<std::size_t>(first);
                if (second >= 0) {
                    c.variance_second_column = static_cast<std::size_t>(second);
                }
                c.hypothesized_variance = d.line_number(QStringLiteral("hypothesis"));
                c.variance_test_method = d.line_text(QStringLiteral("method")).trimmed()
                    .toLower().toStdString();
                if (c.variance_test_method != "levene"
                    && c.variance_test_method != "brown_forsythe") {
                    c.variance_test_method = "f";
                }
                c.variance_alternative = d.line_text(QStringLiteral("alternative")).trimmed()
                    .toLower().toStdString();
                if (c.variance_alternative != "less"
                    && c.variance_alternative != "greater") {
                    c.variance_alternative = "two_sided";
                }
                return {};
            },
            AnalysisService::variance_test},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int value = d.first_role_index(QStringLiteral("value"));
                if (value < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择时间序列值列。"));
                }
                c.analysis_name = "时间序列分解";
                c.chart_type = "time_series_decomposition";
                const int time = d.first_role_index(QStringLiteral("time"));
                if (time >= 0) {
                    c.decomposition_time_column = static_cast<std::size_t>(time);
                }
                c.decomposition_value_column = static_cast<std::size_t>(value);
                c.decomposition_seasonal_period = d.line_int(QStringLiteral("period")).value_or(1);
                c.decomposition_model = d.line_text(QStringLiteral("model")).trimmed()
                    .toLower().toStdString();
                if (c.decomposition_model != "multiplicative") {
                    c.decomposition_model = "additive";
                }
                c.forecast_periods = d.line_int(QStringLiteral("periods")).value_or(4);
                return {};
            },
            AnalysisService::time_series_decomposition},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "季节性预测";
                c.chart_type = "seasonal_forecasting";
                c.decomposition_value_column = static_cast<std::size_t>(
                    d.first_role_index(QStringLiteral("value")));
                c.seasonal_period = static_cast<std::size_t>(
                    std::max(1, d.line_int(QStringLiteral("period")).value_or(12)));
                c.seasonal_error_model = d.line_text(QStringLiteral("error")).trimmed()
                    .toLower().toStdString();
                c.seasonal_trend_model = d.line_text(QStringLiteral("trend")).trimmed()
                    .toLower().toStdString();
                c.smoothing_alpha = d.line_number(QStringLiteral("alpha")).value_or(0.2);
                c.seasonal_beta = d.line_number(QStringLiteral("beta")).value_or(0.1);
                c.smoothing_gamma = d.line_number(QStringLiteral("gamma")).value_or(0.2);
                c.forecast_periods = std::max(1, d.line_int(
                    QStringLiteral("forecast")).value_or(4));
                return {};
            },
            AnalysisService::seasonal_forecasting},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "主成分分析";
                c.chart_type = "pca";
                for (const int column : d.role_indices(QStringLiteral("variables"))) {
                    if (column >= 0) {
                        c.pca_variable_columns.push_back(static_cast<std::size_t>(column));
                    }
                }
                c.pca_mode = d.line_text(QStringLiteral("mode")).trimmed().toLower().toStdString();
                c.pca_component_count = static_cast<std::size_t>(
                    std::max(0, d.line_int(QStringLiteral("components")).value_or(0)));
                return {};
            },
            AnalysisService::pca},
        {
            QStringLiteral("reliability"),
            QStringLiteral("可靠性分析（Kaplan-Meier / Weibull）"),
            QStringLiteral("Reliability Analysis"),
            QStringLiteral("统计"),
            QStringLiteral("report"),
            false, true,
            {{QStringLiteral("time"), QStringLiteral("寿命/时间"), false, false},
             {QStringLiteral("event"), QStringLiteral("失效指示（1=失效，0=删失）"), false, false},
             {QStringLiteral("group"), QStringLiteral("分组列（Log-rank，可选）"), false, true}},
            {{QStringLiteral("model"), QStringLiteral("模型"),
              QStringLiteral("kaplan_meier / weibull / exponential")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int time = d.first_role_index(QStringLiteral("time"));
                const int event = d.first_role_index(QStringLiteral("event"));
                if (time < 0 || event < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择寿命列和失效指示列。"));
                }
                c.analysis_name = "Reliability Analysis";
                c.chart_type = "reliability";
                c.reliability_time_column = static_cast<std::size_t>(time);
                c.reliability_event_column = static_cast<std::size_t>(event);
                const int group = d.first_role_index(QStringLiteral("group"));
                if (group >= 0) {
                    c.reliability_group_column = static_cast<std::size_t>(group);
                }
                c.reliability_model = d.line_text(QStringLiteral("model")).trimmed()
                    .toLower().toStdString();
                if (c.reliability_model != "weibull"
                    && c.reliability_model != "exponential") {
                    c.reliability_model = "kaplan_meier";
                }
                return {};
            },
            AnalysisService::reliability},
        {
            QStringLiteral("t_power"),
            QStringLiteral("t 功效与样本量"),
            QStringLiteral("t 功效与样本量"),
            QStringLiteral("统计"),
            QStringLiteral("one_sample_t"),
            false, false,
            {},
            {{QStringLiteral("mode"), QStringLiteral("模式"),
              QStringLiteral("one_sample_sample_size")},
             {QStringLiteral("effect"), QStringLiteral("效应量 d"), QStringLiteral("0.5")},
             {QStringLiteral("target"), QStringLiteral("目标功效"), QStringLiteral("0.8")},
             {QStringLiteral("alpha"), QStringLiteral("显著性水平"), QStringLiteral("0.05")},
             {QStringLiteral("sample_size"), QStringLiteral("样本量（计算功效时）"), QString()},
             {QStringLiteral("groups"), QStringLiteral("ANOVA 组数"), QStringLiteral("3")},
             {QStringLiteral("null_proportion"), QStringLiteral("第一/假设比例"),
              QStringLiteral("0.5")},
             {QStringLiteral("second_proportion"), QStringLiteral("第二/备择比例"),
              QStringLiteral("0.7")},
             {QStringLiteral("variance_method"), QStringLiteral("比例方差"),
              QStringLiteral("pooled / unpooled")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "t 功效与样本量";
                c.chart_type = "t_power";
                c.power_mode = d.line_text(QStringLiteral("mode")).trimmed().toLower().toStdString();
                if (c.power_mode.empty()) {
                    c.power_mode = "one_sample_sample_size";
                }
                c.power_effect_size = d.line_number(QStringLiteral("effect")).value_or(0.5);
                c.power_target = d.line_number(QStringLiteral("target")).value_or(0.8);
                c.power_alpha = d.line_number(QStringLiteral("alpha")).value_or(0.05);
                c.power_sample_size = static_cast<std::size_t>(
                    std::max(0, d.line_int(QStringLiteral("sample_size")).value_or(0)));
                c.power_group_count = static_cast<std::size_t>(
                    std::max(0, d.line_int(QStringLiteral("groups")).value_or(3)));
                c.power_null_proportion =
                    d.line_number(QStringLiteral("null_proportion")).value_or(0.5);
                c.power_second_proportion =
                    d.line_number(QStringLiteral("second_proportion")).value_or(0.7);
                c.power_variance_method = d.line_text(QStringLiteral("variance_method")).trimmed()
                    .toLower().toStdString();
                if (c.power_variance_method != "unpooled") {
                    c.power_variance_method = "pooled";
                }
                return {};
            },
            AnalysisService::t_power},
        // ------------------------------------------------------------ 图形
        {
            QStringLiteral("histogram"),
            QStringLiteral("直方图"),
            QStringLiteral("直方图"),
            QStringLiteral("图形"),
            QStringLiteral("histogram"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false}},
            {},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.chart_type = "histogram";
                const int column = d.first_role_index(QStringLiteral("variables"));
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择连续变量列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                return {};
            },
            AnalysisService::histogram},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.chart_type = "boxplot";
                const int column = d.first_role_index(QStringLiteral("variables"));
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择连续变量列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                if (d.first_role_index(QStringLiteral("by")) >= 0) {
                    c.by_column = static_cast<std::size_t>(
                        d.first_role_index(QStringLiteral("by")));
                }
                return {};
            },
            AnalysisService::boxplot},
        {
            QStringLiteral("pareto"),
            QStringLiteral("柏拉图"),
            QStringLiteral("柏拉图"),
            QStringLiteral("图形"),
            QStringLiteral("pareto"),
            false, true,
            {{QStringLiteral("category"), QStringLiteral("缺陷类别"), false, false},
             {QStringLiteral("counts"), QStringLiteral("计数列"), false, true}},
            {{QStringLiteral("other_threshold"), QStringLiteral("Other 合并阈值（可选 %）"),
              QStringLiteral("例如 95；留空则不合并")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.chart_type = "pareto";
                const int column = d.first_role_index(QStringLiteral("category"));
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择缺陷类别列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                if (d.first_role_index(QStringLiteral("counts")) >= 0) {
                    c.selection.defect_count_column = static_cast<std::size_t>(
                        d.first_role_index(QStringLiteral("counts")));
                }
                const std::optional<double> threshold =
                    d.line_number(QStringLiteral("other_threshold"));
                if (threshold.has_value() && *threshold >= 0.0 && *threshold <= 100.0) {
                    c.pareto_other_threshold_percent = threshold;
                } else if (threshold.has_value()) {
                    return apply_error(QStringLiteral("参数无效"),
                                       QStringLiteral("Other 合并阈值必须在 0 到 100 之间。"));
                }
                return {};
            },
            AnalysisService::pareto},
        // ------------------------------------------------------------ 控制图
        {
            QStringLiteral("imr"),
            QStringLiteral("I-MR 控制图"),
            QStringLiteral("I-MR 控制图"),
            QStringLiteral("控制图"),
            QStringLiteral("imr"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false}},
            {{QStringLiteral("mr_length"), QStringLiteral("移动极差长度"), QStringLiteral("2")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "I-MR 控制图";
                c.chart_type = "imr";
                const int column = d.first_role_index(QStringLiteral("variables"));
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                c.moving_range_length = d.line_int(QStringLiteral("mr_length")).value_or(2);
                return {};
            },
            AnalysisService::individuals_moving_range},
        {
            QStringLiteral("xbar_r"),
            QStringLiteral("Xbar-R 控制图"),
            QStringLiteral("Xbar-R 控制图"),
            QStringLiteral("控制图"),
            QStringLiteral("xbar_r"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false},
             {QStringLiteral("subgroup"), QStringLiteral("子组列"), false, true}},
            {{QStringLiteral("subgroup_size"), QStringLiteral("子组大小"), QStringLiteral("5")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "Xbar-R 控制图";
                c.chart_type = "xbar_r";
                const int column = d.first_role_index(QStringLiteral("variables"));
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                if (d.first_role_index(QStringLiteral("subgroup")) >= 0) {
                    c.selection.subgroup_column = static_cast<std::size_t>(
                        d.first_role_index(QStringLiteral("subgroup")));
                }
                c.subgroup_size = d.line_int(QStringLiteral("subgroup_size")).value_or(5);
                return {};
            },
            AnalysisService::xbar_range},
        {
            QStringLiteral("xbar_s"),
            QStringLiteral("Xbar-S 控制图"),
            QStringLiteral("Xbar-S 控制图"),
            QStringLiteral("控制图"),
            QStringLiteral("xbar_s"),
            false, true,
            {{QStringLiteral("variables"), QStringLiteral("变量"), false, false},
             {QStringLiteral("subgroup"), QStringLiteral("子组列"), false, true}},
            {{QStringLiteral("subgroup_size"), QStringLiteral("子组大小"), QStringLiteral("5")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "Xbar-S 控制图";
                c.chart_type = "xbar_s";
                const int column = d.first_role_index(QStringLiteral("variables"));
                if (column < 0) {
                    return apply_silent();
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                if (d.first_role_index(QStringLiteral("subgroup")) >= 0) {
                    c.selection.subgroup_column = static_cast<std::size_t>(
                        d.first_role_index(QStringLiteral("subgroup")));
                }
                c.subgroup_size = d.line_int(QStringLiteral("subgroup_size")).value_or(5);
                return {};
            },
            AnalysisService::xbar_s},
        {
            QStringLiteral("p_chart"),
            QStringLiteral("P 图"),
            QStringLiteral("P 图"),
            QStringLiteral("控制图"),
            QStringLiteral("p_chart"),
            true, true,
            {{QStringLiteral("defectives"), QStringLiteral("不合格品数"), false, false},
             {QStringLiteral("inspected"), QStringLiteral("检验数（列）"), false, true}},
            {{QStringLiteral("inspected_constant"), QStringLiteral("检验数（常数）"), QString()}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "P 图";
                c.chart_type = "p_chart";
                const int defectives = d.first_role_index(QStringLiteral("defectives"));
                if (defectives < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择不合格品数列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defectives));
                c.selection.defect_count_column = static_cast<std::size_t>(defectives);
                if (d.first_role_index(QStringLiteral("inspected")) >= 0) {
                    c.selection.inspected_count_column = static_cast<std::size_t>(
                        d.first_role_index(QStringLiteral("inspected")));
                }
                if (d.line_int(QStringLiteral("inspected_constant")).has_value()) {
                    c.inspected_constant = static_cast<std::size_t>(
                        *d.line_int(QStringLiteral("inspected_constant")));
                }
                return {};
            },
            AnalysisService::p_chart},
        {
            QStringLiteral("np_chart"),
            QStringLiteral("NP 图"),
            QStringLiteral("NP 图"),
            QStringLiteral("控制图"),
            QStringLiteral("np_chart"),
            false, true,
            {{QStringLiteral("defectives"), QStringLiteral("不合格品数"), false, false},
             {QStringLiteral("inspected"), QStringLiteral("检验数列"), false, true}},
            {{QStringLiteral("inspected_constant"), QStringLiteral("检验数常数"),
              QStringLiteral("例如 100")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.chart_type = "np_chart";
                const int defect_column = d.first_role_index(QStringLiteral("defectives"));
                if (defect_column < 0) {
                    return apply_silent();
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defect_column));
                c.selection.defect_count_column = static_cast<std::size_t>(defect_column);
                if (d.first_role_index(QStringLiteral("inspected")) >= 0) {
                    c.selection.inspected_count_column = static_cast<std::size_t>(
                        d.first_role_index(QStringLiteral("inspected")));
                }
                if (d.line_int(QStringLiteral("inspected_constant")).has_value()) {
                    c.inspected_constant = static_cast<std::size_t>(
                        *d.line_int(QStringLiteral("inspected_constant")));
                }
                return {};
            },
            AnalysisService::np_chart},
        {
            QStringLiteral("c_chart"),
            QStringLiteral("C 图"),
            QStringLiteral("C 图"),
            QStringLiteral("控制图"),
            QStringLiteral("c_chart"),
            false, true,
            {{QStringLiteral("defects"), QStringLiteral("缺陷数"), false, false}},
            {{QStringLiteral("units"), QStringLiteral("每个子组单位数"), QStringLiteral("1")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.chart_type = "c_chart";
                const int defect_column = d.first_role_index(QStringLiteral("defects"));
                if (defect_column < 0) {
                    return apply_silent();
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defect_column));
                c.selection.defect_count_column = static_cast<std::size_t>(defect_column);
                c.inspected_constant = static_cast<std::size_t>(
                    d.line_int(QStringLiteral("units")).value_or(1));
                return {};
            },
            AnalysisService::c_chart},
        {
            QStringLiteral("u_chart"),
            QStringLiteral("U 图"),
            QStringLiteral("U 图"),
            QStringLiteral("控制图"),
            QStringLiteral("u_chart"),
            false, true,
            {{QStringLiteral("defects"), QStringLiteral("缺陷数"), false, false},
             {QStringLiteral("units"), QStringLiteral("单位数列"), false, false}},
            {},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.chart_type = "u_chart";
                const int defect_column = d.first_role_index(QStringLiteral("defects"));
                const int units_column = d.first_role_index(QStringLiteral("units"));
                if (defect_column < 0 || units_column < 0) {
                    return apply_silent();
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defect_column));
                c.selection.defect_count_column = static_cast<std::size_t>(defect_column);
                c.selection.inspected_count_column = static_cast<std::size_t>(units_column);
                return {};
            },
            AnalysisService::u_chart},
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
             {QStringLiteral("tests"), QStringLiteral("特殊原因测试"),
              QStringLiteral("例如 1 2 3 4")},
             {QStringLiteral("historical_center"), QStringLiteral("历史中心线"),
              QStringLiteral("可选")},
             {QStringLiteral("historical_sigma_z"), QStringLiteral("历史 Sigma Z"),
              QStringLiteral("可选")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "Laney P' 图";
                c.chart_type = "laney_p_chart";
                const int defect_column = d.first_role_index(QStringLiteral("defectives"));
                if (defect_column < 0) {
                    return apply_silent();
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defect_column));
                c.selection.defect_count_column = static_cast<std::size_t>(defect_column);
                if (d.first_role_index(QStringLiteral("inspected")) >= 0) {
                    c.selection.inspected_count_column = static_cast<std::size_t>(
                        d.first_role_index(QStringLiteral("inspected")));
                }
                if (d.first_role_index(QStringLiteral("stage")) >= 0) {
                    c.stage_column = static_cast<std::size_t>(
                        d.first_role_index(QStringLiteral("stage")));
                }
                if (d.line_int(QStringLiteral("inspected_constant")).has_value()) {
                    c.inspected_constant = static_cast<std::size_t>(
                        *d.line_int(QStringLiteral("inspected_constant")));
                }
                c.enabled_special_cause_tests.clear();
                for (const QString& token : d.line_text(QStringLiteral("tests")).split(
                         QRegularExpression(QStringLiteral("[,;\\s]+")), Qt::SkipEmptyParts)) {
                    bool ok = false;
                    const int test = token.toInt(&ok);
                    if (ok && test >= 1 && test <= 4) {
                        c.enabled_special_cause_tests.push_back(test);
                    }
                }
                if (c.enabled_special_cause_tests.empty()) {
                    c.enabled_special_cause_tests = {1};
                }
                c.historical_center = d.line_number(QStringLiteral("historical_center"));
                c.historical_sigma_z = d.line_number(QStringLiteral("historical_sigma_z"));
                return {};
            },
            AnalysisService::laney_p_chart},
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
            {{QStringLiteral("tests"), QStringLiteral("特殊原因测试"),
              QStringLiteral("例如 1 2 3 4")},
             {QStringLiteral("historical_center"), QStringLiteral("历史中心线"),
              QStringLiteral("可选")},
             {QStringLiteral("historical_sigma_z"), QStringLiteral("历史 Sigma Z"),
              QStringLiteral("可选")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "Laney U' 图";
                c.chart_type = "laney_u_chart";
                const int defect_column = d.first_role_index(QStringLiteral("defects"));
                const int units_column = d.first_role_index(QStringLiteral("units"));
                if (defect_column < 0 || units_column < 0) {
                    return apply_silent();
                }
                c.variable_columns.push_back(static_cast<std::size_t>(defect_column));
                c.selection.defect_count_column = static_cast<std::size_t>(defect_column);
                c.selection.inspected_count_column = static_cast<std::size_t>(units_column);
                if (d.first_role_index(QStringLiteral("stage")) >= 0) {
                    c.stage_column = static_cast<std::size_t>(
                        d.first_role_index(QStringLiteral("stage")));
                }
                c.enabled_special_cause_tests.clear();
                for (const QString& token : d.line_text(QStringLiteral("tests")).split(
                         QRegularExpression(QStringLiteral("[,;\\s]+")), Qt::SkipEmptyParts)) {
                    bool ok = false;
                    const int test = token.toInt(&ok);
                    if (ok && test >= 1 && test <= 4) {
                        c.enabled_special_cause_tests.push_back(test);
                    }
                }
                if (c.enabled_special_cause_tests.empty()) {
                    c.enabled_special_cause_tests = {1};
                }
                c.historical_center = d.line_number(QStringLiteral("historical_center"));
                c.historical_sigma_z = d.line_number(QStringLiteral("historical_sigma_z"));
                return {};
            },
            AnalysisService::laney_u_chart},
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
              QStringLiteral("可选")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int column = d.first_role_index(QStringLiteral("variables"));
                if (column < 0) {
                    return apply_silent();
                }
                c.analysis_name = "EWMA 控制图";
                c.chart_type = "ewma";
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.ewma_lambda = d.line_number(QStringLiteral("lambda")).value_or(0.2);
                c.ewma_limit_sigma = d.line_number(QStringLiteral("limit")).value_or(3.0);
                c.historical_center = d.line_number(QStringLiteral("historical_mean"));
                return {};
            },
            AnalysisService::ewma},
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
             {QStringLiteral("h"), QStringLiteral("决策间隔 h"), QStringLiteral("4")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int column = d.first_role_index(QStringLiteral("variables"));
                if (column < 0) {
                    return apply_silent();
                }
                c.analysis_name = "CUSUM 控制图";
                c.chart_type = "cusum";
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.cusum_target = d.line_number(QStringLiteral("target")).value_or(0.0);
                c.cusum_sigma = d.line_number(QStringLiteral("sigma")).value_or(1.0);
                c.cusum_k = d.line_number(QStringLiteral("k")).value_or(0.5);
                c.cusum_h = d.line_number(QStringLiteral("h")).value_or(4.0);
                return {};
            },
            AnalysisService::cusum},
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
             {QStringLiteral("target"), QStringLiteral("Target"), QStringLiteral("可选")}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "正态过程能力";
                c.chart_type = "capability";
                const int column = d.first_role_index(QStringLiteral("variables"));
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                c.subgroup_size = d.line_int(QStringLiteral("subgroup_size")).value_or(1);
                c.specifications.lower = d.line_number(QStringLiteral("lsl"));
                c.specifications.upper = d.line_number(QStringLiteral("usl"));
                c.specifications.target = d.line_number(QStringLiteral("target"));
                return {};
            },
            [](const DataTable& table, const AnalysisConfiguration& configuration) {
                return AnalysisService::capability(table, configuration);
            }},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "过程能力 Sixpack";
                c.chart_type = "capability_sixpack";
                const int column = d.first_role_index(QStringLiteral("variables"));
                if (column < 0) {
                    return apply_error(QStringLiteral("未选择变量"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.variable_columns.push_back(static_cast<std::size_t>(column));
                c.selection.measurement_column = static_cast<std::size_t>(column);
                c.subgroup_size = d.line_int(QStringLiteral("subgroup_size")).value_or(1);
                c.specifications.lower = d.line_number(QStringLiteral("lsl"));
                c.specifications.upper = d.line_number(QStringLiteral("usl"));
                c.specifications.target = d.line_number(QStringLiteral("target"));
                return {};
            },
            AnalysisService::capability_sixpack},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int column = d.first_role_index(QStringLiteral("variables"));
                if (column < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择一个正值变量。"));
                }
                c.analysis_name = "Box-Cox 变换";
                c.chart_type = "box_cox";
                c.variable_columns = {static_cast<std::size_t>(column)};
                c.hypothesis_mean = d.line_number(QStringLiteral("lambda"));
                c.specifications.lower = d.line_number(QStringLiteral("lsl"));
                c.specifications.upper = d.line_number(QStringLiteral("usl"));
                return {};
            },
            AnalysisService::box_cox},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int measurement = d.first_role_index(QStringLiteral("measurement"));
                const int part = d.first_role_index(QStringLiteral("part"));
                const int oper = d.first_role_index(QStringLiteral("operator"));
                if (measurement < 0 || part < 0 || oper < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量值、零件和操作员列。"));
                }
                c.analysis_name = "Crossed Gage R&R";
                c.chart_type = "gage_rr";
                c.gage_measurement_column = static_cast<std::size_t>(measurement);
                c.gage_part_column = static_cast<std::size_t>(part);
                c.gage_operator_column = static_cast<std::size_t>(oper);
                c.specifications.lower = d.line_number(QStringLiteral("lsl"));
                c.specifications.upper = d.line_number(QStringLiteral("usl"));
                return {};
            },
            AnalysisService::gage_rr},
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
             {QStringLiteral("tolerance"), QStringLiteral("公差宽度（可选）"), QString()}},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                const int measurement = d.first_role_index(QStringLiteral("measurement"));
                if (measurement < 0) {
                    return apply_error(QStringLiteral("参数不足"),
                                       QStringLiteral("请选择测量值列。"));
                }
                c.analysis_name = "MSA Type 1 / Bias / Stability";
                c.chart_type = "msa_type1";
                c.gage_measurement_column = static_cast<std::size_t>(measurement);
                c.msa_mode = d.line_text(QStringLiteral("mode")).trimmed().toLower().toStdString();
                if (c.msa_mode != "bias_linearity" && c.msa_mode != "stability") {
                    c.msa_mode = "type1";
                }
                if (d.first_role_index(QStringLiteral("reference")) >= 0) {
                    c.msa_reference_column = static_cast<std::size_t>(
                        d.first_role_index(QStringLiteral("reference")));
                }
                c.msa_reference_value = d.line_number(QStringLiteral("reference_value"));
                c.gage_tolerance = d.line_number(QStringLiteral("tolerance")).value_or(0.0);
                return {};
            },
            AnalysisService::msa_type1},
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
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "Nested Gage R&R";
                c.chart_type = "nested_gage_rr";
                c.nested_gage_measurement_column = static_cast<std::size_t>(
                    d.first_role_index(QStringLiteral("measurement")));
                c.nested_gage_part_column = static_cast<std::size_t>(
                    d.first_role_index(QStringLiteral("part")));
                c.nested_gage_operator_column = static_cast<std::size_t>(
                    d.first_role_index(QStringLiteral("operator")));
                c.gage_tolerance = d.line_number(QStringLiteral("tolerance")).value_or(0.0);
                return {};
            },
            AnalysisService::nested_gage_rr},
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
            {},
            [](AnalysisConfiguration& c, const AnalysisSetupDialog& d) -> AnalysisApplyResult {
                c.analysis_name = "属性一致性分析";
                c.chart_type = "attribute_agreement";
                c.attribute_rating_column = static_cast<std::size_t>(
                    d.first_role_index(QStringLiteral("rating")));
                c.attribute_part_column = static_cast<std::size_t>(
                    d.first_role_index(QStringLiteral("part")));
                c.attribute_appraiser_column = static_cast<std::size_t>(
                    d.first_role_index(QStringLiteral("appraiser")));
                const int standard = d.first_role_index(QStringLiteral("standard"));
                if (standard >= 0) {
                    c.attribute_standard_column = static_cast<std::size_t>(standard);
                }
                return {};
            },
            AnalysisService::attribute_agreement},
        {
            QStringLiteral("doe_factorial"),
            QStringLiteral("2 水平全因子设计"),
            QStringLiteral("2 水平全因子设计"),
            QStringLiteral("质量工具"),
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
             {QStringLiteral("centers"), QStringLiteral("中心点数"), QStringLiteral("0")},
             {QStringLiteral("blocks"), QStringLiteral("区组数"), QStringLiteral("1")},
             {QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("0")}},
            doe_apply,
            doe_run},
        {
            QStringLiteral("doe_response"),
            QStringLiteral("DOE 响应分析"),
            QStringLiteral("2 水平全因子设计"),
            QStringLiteral("质量工具"),
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
             {QStringLiteral("centers"), QStringLiteral("中心点数"), QStringLiteral("0")},
             {QStringLiteral("blocks"), QStringLiteral("区组数"), QStringLiteral("1")},
             {QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("0")}},
            doe_apply,
            doe_run},
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
