#include "application/interpretation_service.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <sstream>

namespace datalab::application {

namespace {

using Severity = domain::DiagnosticMessage::Severity;

void raise_severity(Severity& current, Severity candidate)
{
    if (candidate == Severity::error
        || (candidate == Severity::warning && current == Severity::info)) {
        current = candidate;
    }
}

std::optional<double> number_after(const domain::StatisticTable& table,
                                   const std::string& label)
{
    for (const auto& row : table.rows) {
        for (std::size_t index = 0; index + 1 < row.size(); ++index) {
            if (row[index] == label) {
                try {
                    const double value = std::stod(row[index + 1]);
                    if (std::isfinite(value)) {
                        return value;
                    }
                } catch (...) {
                }
            }
        }
    }
    return std::nullopt;
}

std::optional<double> number_in_column(const domain::StatisticTable& table,
                                       const std::string& header,
                                       std::size_t row_index = 0)
{
    const auto column = std::find(table.headers.cbegin(), table.headers.cend(), header);
    if (column == table.headers.cend() || row_index >= table.rows.size()) {
        return std::nullopt;
    }
    const std::size_t index = static_cast<std::size_t>(
        std::distance(table.headers.cbegin(), column));
    if (index >= table.rows[row_index].size()) {
        return std::nullopt;
    }
    try {
        const double value = std::stod(table.rows[row_index][index]);
        return std::isfinite(value) ? std::optional<double>(value) : std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

std::size_t count_column_values(const domain::StatisticTable& table,
                                const std::string& header)
{
    const auto column = std::find(table.headers.cbegin(), table.headers.cend(), header);
    if (column == table.headers.cend()) {
        return 0;
    }
    const std::size_t index = static_cast<std::size_t>(
        std::distance(table.headers.cbegin(), column));
    std::size_t count = 0;
    for (const auto& row : table.rows) {
        if (index < row.size() && !row[index].empty() && row[index] != "*") {
            ++count;
        }
    }
    return count;
}

void add_limitations(const domain::OutputPage& page,
                     domain::InterpretationSection& limitations)
{
    for (const auto& diagnostic : page.diagnostics) {
        if (diagnostic.severity == Severity::error) {
            raise_severity(limitations.severity, Severity::error);
            limitations.bullets.push_back("分析错误，以下结论不可用于决策：" + diagnostic.message);
        } else if (diagnostic.severity == Severity::warning) {
            raise_severity(limitations.severity, Severity::warning);
            limitations.bullets.push_back("分析限制：" + diagnostic.message);
        }
    }
    if (!page.configuration.excluded_rows.empty()) {
        limitations.bullets.push_back(
            "配置排除了 " + std::to_string(page.configuration.excluded_rows.size())
            + " 行；结论只适用于纳入分析的数据。");
    }
}

void add_doe_rules(const domain::OutputPage& page,
                   domain::InterpretationSection& conclusion,
                   domain::InterpretationSection& advice,
                   domain::InterpretationSection& limitations)
{
    const double alpha = 1.0 - page.configuration.confidence_level;
    bool has_p_value = false;
    std::vector<std::string> significant_terms;
    for (const auto& table : page.tables) {
        if (table.title.find("ANOVA") == std::string::npos) {
            continue;
        }
        for (const auto& row : table.rows) {
            if (row.empty() || row.size() < 6 || row.back() == "*") {
                continue;
            }
            try {
                const double p_value = std::stod(row.back());
                has_p_value = true;
                if (p_value < alpha) {
                    significant_terms.push_back(row.front());
                }
            } catch (...) {
            }
        }
    }
    if (!has_p_value) {
        limitations.bullets.push_back(
            "DOE 表中没有可用的 P-Value；不能据此宣称因子或交互作用显著。");
        raise_severity(limitations.severity, Severity::warning);
    } else if (significant_terms.empty()) {
        conclusion.bullets.push_back("在当前显著性水平下，没有发现可报告为显著的 DOE 项。");
    } else {
        conclusion.bullets.push_back(
            "在 α = " + std::to_string(alpha) + " 下显著的项："
            + [&] {
                std::ostringstream text;
                for (std::size_t i = 0; i < significant_terms.size(); ++i) {
                    if (i != 0) text << "、";
                    text << significant_terms[i];
                }
                return text.str();
            }() + "。");
        advice.bullets.push_back("优先在确认性试验中复核显著项，并结合效应方向选择可操作的因子水平；"
                                "显著性本身不代表工程影响足够大。");
    }
    if (page.method_name == "2-Level Factorial Design") {
        advice.bullets.push_back(
            "设计包含 " + std::to_string(page.configuration.doe_factor_names.size())
            + " 个因子、" + std::to_string(page.configuration.doe_center_point_count)
            + " 个中心点；实施时保持随机化/区组记录，避免把运行顺序效应误认为因子效应。");
    }
    if (page.configuration.doe_center_point_count == 0
        && page.method_name.find("Response") != std::string::npos) {
        limitations.bullets.push_back("未配置中心点，无法用中心点直接检验曲率；二水平线性模型的适用范围有限。");
        raise_severity(limitations.severity, Severity::warning);
    }
    advice.bullets.push_back("检查残差随机性、异常值和重复/纯误差后再确定最优设置；"
                             "不要仅凭主效应图下结论。");
}

void add_msa_rules(const domain::OutputPage& page,
                   domain::InterpretationSection& conclusion,
                   domain::InterpretationSection& advice,
                   domain::InterpretationSection& limitations)
{
    if (page.method_name == "Stability / Gage Run Chart") {
        const auto out = number_after(page.tables.front(), "Out of Control");
        if (!out.has_value()) {
            limitations.bullets.push_back("没有可用的超限点统计，不能判定量具稳定性。");
            raise_severity(limitations.severity, Severity::warning);
        } else if (*out > 0.0) {
            conclusion.bullets.push_back("量具稳定性图发现 " + std::to_string(*out)
                                         + " 个超限点，统计上存在失控信号。");
            advice.bullets.push_back("按时间顺序回查校准、环境、操作者和量具维护记录；"
                                     "不要在特殊原因未排除前用该量具放行产品。");
        } else {
            conclusion.bullets.push_back("当前观测未发现稳定性图超限点；这不等同于量具满足能力要求。");
        }
        return;
    }
    const domain::StatisticTable* summary = page.tables.empty() ? nullptr : &page.tables.front();
    if (summary == nullptr || summary->rows.empty()) {
        limitations.bullets.push_back("MSA 结果表为空，不能判定偏倚、线性或 Type 1 能力。");
        raise_severity(limitations.severity, Severity::warning);
        return;
    }
    if (page.method_name == "MSA Bias and Linearity") {
        const auto slope = number_in_column(*summary, "Slope");
        const auto bias_low = number_in_column(*summary, "Bias Low");
        const auto bias_high = number_in_column(*summary, "Bias High");
        if (!slope.has_value() || !bias_low.has_value() || !bias_high.has_value()) {
            limitations.bullets.push_back("偏倚/线性表缺少斜率或端点偏倚，不能作完整判定。");
            raise_severity(limitations.severity, Severity::warning);
        } else {
            conclusion.bullets.push_back("偏倚回归斜率 = " + std::to_string(*slope)
                                         + "，低/高参考点偏倚 = "
                                         + std::to_string(*bias_low) + " / "
                                         + std::to_string(*bias_high) + "。");
            advice.bullets.push_back("将端点偏倚与产品公差和测量系统可接受界限比较；"
                                     "回归关系不能替代跨操作者、跨部件的完整 Gage R&R。");
        }
    } else {
        const auto p_value = number_in_column(*summary, "P");
        const auto cgk = number_in_column(*summary, "Cgk");
        const auto tolerance = number_in_column(*summary, "%Tolerance");
        if (!p_value.has_value() || !cgk.has_value()) {
            limitations.bullets.push_back("Type 1 表缺少 P 或 Cgk，不能完整评价偏倚与重复性。");
            raise_severity(limitations.severity, Severity::warning);
        } else {
            const double alpha = 1.0 - page.configuration.confidence_level;
            conclusion.bullets.push_back("偏倚检验 P = " + std::to_string(*p_value)
                                         + (*p_value < alpha ? "，与参考值差异具有统计证据。" :
                                            "，未发现与参考值差异的统计证据。"));
            conclusion.bullets.push_back("Cgk = " + std::to_string(*cgk)
                                         + "；应结合组织规定的能力门槛解释，而非套用单一通用阈值。");
            if (tolerance.has_value()) {
                advice.bullets.push_back("%Tolerance = " + std::to_string(*tolerance)
                                         + "；请与产品公差及风险等级核对后决定校准、维修或放行策略。");
            }
        }
    }
}

void add_reliability_rules(const domain::OutputPage& page,
                           domain::InterpretationSection& conclusion,
                           domain::InterpretationSection& advice,
                           domain::InterpretationSection& limitations)
{
    if (page.method_name == "Kaplan-Meier") {
        if (page.tables.empty() || page.tables.front().rows.empty()) {
            limitations.bullets.push_back("Kaplan-Meier 生存表为空，不能估计生存曲线或中位寿命。");
            raise_severity(limitations.severity, Severity::warning);
            return;
        }
        const auto& table = page.tables.front();
        const std::size_t censor_count = [&] {
            std::size_t value = 0;
            const auto column = std::find(table.headers.cbegin(), table.headers.cend(), "Censored");
            if (column == table.headers.cend()) return value;
            const std::size_t index = static_cast<std::size_t>(
                std::distance(table.headers.cbegin(), column));
            for (const auto& row : table.rows) {
                if (index < row.size()) {
                    try { value += static_cast<std::size_t>(std::stoul(row[index])); } catch (...) {}
                }
            }
            return value;
        }();
        conclusion.bullets.push_back("Kaplan-Meier 曲线基于 " + std::to_string(table.rows.size())
                                     + " 个时间点；删失数合计 " + std::to_string(censor_count) + "。");
        if (censor_count > 0) {
            limitations.bullets.push_back("存在右删失；尾部生存率由较少的风险集支持，不能把删失时间当作失效时间。");
            raise_severity(limitations.severity, Severity::warning);
        }
        advice.bullets.push_back("按目标任务时间读取生存概率，并报告置信区间；需要比较产品/方案时使用分层或组间检验。");
    } else {
        const auto shape = page.method_name == "Weibull Lifetime"
            && !page.tables.empty()
            ? number_after(page.tables.front(), "Shape") : std::nullopt;
        if (page.method_name == "Weibull Lifetime" && !shape.has_value()) {
            limitations.bullets.push_back("Weibull 参数缺失，不能解释失效率随时间的变化。");
            raise_severity(limitations.severity, Severity::warning);
        } else if (shape.has_value()) {
            conclusion.bullets.push_back("Weibull 形状参数 β = " + std::to_string(*shape)
                + (*shape > 1.0 ? "，提示失效率随时间上升。" :
                   *shape < 1.0 ? "，提示早期失效型失效率随时间下降。" :
                                  "，接近恒定失效率。"));
        } else {
            conclusion.bullets.push_back("指数模型估计的是恒定失效率假设下的寿命分布。");
            limitations.bullets.push_back("指数模型不能描述随时间上升或下降的失效率；应与 Weibull 等模型比较适配性。");
            raise_severity(limitations.severity, Severity::warning);
        }
        advice.bullets.push_back("报告模型参数和删失处理，并用现场失效机理验证模型假设；不要把分位寿命当成单件保证寿命。");
    }
}

void add_power_rules(const domain::OutputPage& page,
                     domain::InterpretationSection& conclusion,
                     domain::InterpretationSection& advice,
                     domain::InterpretationSection& limitations)
{
    if (page.tables.empty() || page.tables.front().rows.empty()) {
        limitations.bullets.push_back("功效/样本量结果表为空，不能给出设计建议。");
        raise_severity(limitations.severity, Severity::warning);
        return;
    }
    const auto& table = page.tables.front();
    const auto power = number_in_column(table, "Power");
    const auto effect = number_in_column(table, "Effect Size");
    if (!power.has_value() || !effect.has_value()) {
        limitations.bullets.push_back("缺少 Power 或 Effect Size；结果不能用于样本量决策。");
        raise_severity(limitations.severity, Severity::warning);
        return;
    }
    if (page.method_name == "T Test Power") {
        conclusion.bullets.push_back("在效应量 = " + std::to_string(*effect)
            + "、α = " + std::to_string(page.configuration.power_alpha)
            + " 下，估计功效 = " + std::to_string(*power) + "。");
        advice.bullets.push_back("将效应量预先定义为具有工程意义的最小差异，并同时考虑实际脱落、分组和方差不确定性。");
    } else {
        conclusion.bullets.push_back("按目标功效 " + std::to_string(page.configuration.power_target)
            + " 估计所需样本量；效应量假设为 " + std::to_string(*effect) + "。");
        advice.bullets.push_back("样本量结果是模型和效应量假设的条件结果；试验前应进行敏感性分析并向上取整到可执行的分组方案。");
    }
    if (page.configuration.power_effect_size <= 0.0 || page.configuration.power_alpha <= 0.0
        || page.configuration.power_alpha >= 1.0) {
        limitations.bullets.push_back("效应量或 α 配置不在有效范围，不能把数值当作正式设计依据。");
        raise_severity(limitations.severity, Severity::error);
    }
}

void add_forecast_rules(const domain::OutputPage& page,
                        domain::InterpretationSection& conclusion,
                        domain::InterpretationSection& advice,
                        domain::InterpretationSection& limitations)
{
    if (page.tables.empty()) {
        limitations.bullets.push_back("预测结果表为空，不能评价预测或未来期。");
        raise_severity(limitations.severity, Severity::warning);
        return;
    }
    for (const auto& table : page.tables) {
        if (table.title.find("误差") != std::string::npos
            || table.title.find("准确度") != std::string::npos) {
            const auto mape = number_in_column(table, "MAPE (%)");
            const auto mase = number_in_column(table, "MASE");
            conclusion.bullets.push_back("预测误差指标已计算；"
                + (mape.has_value() ? "MAPE = " + std::to_string(*mape) + "。" : ""));
            if (mase.has_value() && *mase > 1.0) {
                advice.bullets.push_back("MASE > 1，当前模型不优于朴素基准；上线前应比较替代模型或扩大验证窗口。");
            }
        }
    }
    if (page.method_name == "Holt-Winters Seasonal Forecasting") {
        if (page.configuration.seasonal_period < 2) {
            limitations.bullets.push_back("季节周期小于 2，不能支持有意义的季节性解释。");
            raise_severity(limitations.severity, Severity::warning);
        }
        advice.bullets.push_back("预测区间反映模型不确定性，不是规格上下限；应通过滚动验证检查不同预测期的稳定性。");
    } else {
        advice.bullets.push_back("未来预测依赖平滑/ARIMA 模型假设；应检查残差、自相关和结构变化，避免外推超出历史范围。");
    }
}

}  // namespace

void InterpretationService::enrich(domain::OutputPage& page)
{
    page.interpretation.clear();
    domain::InterpretationSection conclusion{"统计结论", {}, Severity::info};
    domain::InterpretationSection advice{"工程建议", {}, Severity::info};
    domain::InterpretationSection limitations{"限制与数据质量", {}, Severity::info};
    add_limitations(page, limitations);
    if (page.method_name.find("DOE") != std::string::npos
        || page.method_name.find("Factorial") != std::string::npos) {
        add_doe_rules(page, conclusion, advice, limitations);
    } else if (page.method_name.find("MSA") != std::string::npos
               || page.method_name.find("Stability") != std::string::npos) {
        add_msa_rules(page, conclusion, advice, limitations);
    } else if (page.method_name.find("Lifetime") != std::string::npos
               || page.method_name == "Kaplan-Meier") {
        add_reliability_rules(page, conclusion, advice, limitations);
    } else if (page.method_name == "T Test Power"
               || page.method_name == "T Test Sample Size") {
        add_power_rules(page, conclusion, advice, limitations);
    } else if (page.method_name.find("Smoothing") != std::string::npos
               || page.method_name == "ARIMA"
               || page.method_name.find("Forecasting") != std::string::npos) {
        add_forecast_rules(page, conclusion, advice, limitations);
    }

    std::size_t out_of_control = 0;
    std::optional<double> cpk;
    std::optional<double> ppk;
    for (const auto& table : page.tables) {
        for (const auto& row : table.rows) {
            for (std::size_t index = 0; index < row.size(); ++index) {
                if (row[index].find("超限点数") != std::string::npos && index + 1 < row.size()) {
                    try {
                        out_of_control += static_cast<std::size_t>(std::stoul(row[index + 1]));
                    } catch (...) {
                    }
                }
                if (index + 1 < row.size() && row[index] == "Cpk") {
                    try { cpk = std::stod(row[index + 1]); } catch (...) {}
                }
                if (index + 1 < row.size() && row[index] == "Ppk") {
                    try { ppk = std::stod(row[index + 1]); } catch (...) {}
                }
            }
        }
    }
    if (cpk.has_value()) {
        raise_severity(conclusion.severity, *cpk < 1.33 ? Severity::warning : Severity::info);
        conclusion.bullets.push_back("Cpk = " + std::to_string(*cpk)
                                 + (*cpk < 1.33
                                        ? "，低于 1.33，过程能力不足。"
                                        : "，达到 1.33 的基本能力门槛。"));
    }
    if (ppk.has_value()) {
        conclusion.bullets.push_back("Ppk = " + std::to_string(*ppk)
                                 + (*ppk < 1.33
                                        ? "，整体过程表现低于 1.33。"
                                        : "，整体过程表现达到 1.33。"));
    }
    if (out_of_control > 0) {
        raise_severity(conclusion.severity, Severity::warning);
        advice.bullets.push_back("发现 " + std::to_string(out_of_control)
                                 + " 个控制图超限点，建议回查对应原始行并确认特殊原因。");
    }
    if (page.method_name == "Correlation") {
        conclusion.bullets.push_back("相关系数表示变量关联方向与强度；P-Value 反映在零相关假设下观察到当前系数的证据强度，"
                                 "不能单独证明因果关系。");
    } else if (page.method_name == "One-Sample T"
               || page.method_name == "Two-Sample T") {
        conclusion.bullets.push_back("请结合 P-Value 与均值差置信区间判断差异；统计显著不等于工程差异具有实际重要性。");
    } else if (page.method_name == "One-Way ANOVA") {
        conclusion.bullets.push_back("ANOVA 的总体 F 检验显著时，只能说明至少存在一组均值差异；"
                                 "需要后续多重比较确定具体组对。");
    }
    for (const auto& plot : page.plots) {
        if (plot.sigma_z > 0.0) {
            const std::string chart_name = page.method_name.find("Laney") != std::string::npos
                ? page.method_name : "当前";
            conclusion.bullets.push_back(chart_name + " Sigma Z = "
                + std::to_string(plot.sigma_z)
                + (plot.sigma_z > 1.0
                       ? "，存在过度离散，传统控制限可能过窄。"
                       : "，控制限已按离散程度进行调整。"));
        }
    }
    if (page.method_name == "Pareto Chart" && !page.tables.empty()
        && !page.tables.front().rows.empty()) {
        const auto& rows = page.tables.front().rows;
        const auto& largest = rows.front();
        if (largest.size() >= 4) {
            conclusion.bullets.push_back(
                "最大类别为“" + largest[0] + "”，计数 " + largest[1]
                + "，单项占比 " + largest[2] + "%。");
            advice.bullets.push_back(
                "前 " + std::to_string(std::min<std::size_t>(2, rows.size()))
                + " 个类别累计占比 "
                + rows[std::min<std::size_t>(1, rows.size() - 1)][3] + "%；"
                "应优先结合现场原因验证，而不是直接假设存在 80/20 规律。");
        }
    }
    if (conclusion.bullets.empty()) {
        conclusion.bullets.push_back("未发现当前规则能够自动判定的异常；请结合现场工艺、规格要求和图表进行确认。");
    }
    if (advice.bullets.empty()) {
        advice.bullets.push_back("将统计结果与规格、风险等级、历史基线和现场特殊原因结合后再采取措施。");
    }
    if (!limitations.bullets.empty()) {
        page.interpretation.push_back(std::move(limitations));
    }
    page.interpretation.push_back(std::move(conclusion));
    page.interpretation.push_back(std::move(advice));
}

}  // namespace datalab::application
