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
    const double alpha = 1.0 - page.configuration.inference.confidence_level;
    bool has_p_value = page.facts.doe.has_value() && page.facts.doe->has_p_value;
    std::vector<std::string> significant_terms =
        page.facts.doe.has_value() ? page.facts.doe->significant_terms
                                   : std::vector<std::string>{};
    if (!page.facts.doe.has_value()) {
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
            "设计包含 " + std::to_string(page.configuration.doe.factor_names.size())
            + " 个因子、" + std::to_string(page.configuration.doe.center_point_count)
            + " 个中心点；实施时保持随机化/区组记录，避免把运行顺序效应误认为因子效应。");
    }
    if (page.configuration.doe.center_point_count == 0
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
    if (page.facts.msa.has_value()
        && (page.method_name.find("Gage R&R") != std::string::npos
            || page.method_name.find("Nested Gage") != std::string::npos)) {
        const auto& facts = *page.facts.msa;
        if (facts.ndc_available && facts.ndc.has_value()) {
            conclusion.bullets.push_back(
                "ndc = " + std::to_string(*facts.ndc)
                + (*facts.ndc < 5.0
                       ? "，小于 5，提示测量系统分辨力需要调查。"
                       : "；ndc 只描述当前研究中零件间变异相对 Gage 变异的分辨力。"));
            if (*facts.ndc < 5.0) {
                raise_severity(limitations.severity, Severity::warning);
                limitations.bullets.push_back(
                    "ndc<5 不是量具不合格的绝对结论，需要结合 %Study Var 和公差风险。");
            }
        } else {
            limitations.bullets.push_back("ndc 不可估计，不能据此评价测量系统分辨力。");
            raise_severity(limitations.severity, Severity::warning);
        }
        if (facts.gage_percent_study_variation.has_value()) {
            conclusion.bullets.push_back(
                "Total Gage R&R %Study Var = "
                + std::to_string(*facts.gage_percent_study_variation)
                + "；%Contribution 与 %Study Var 口径不同，不能混用。");
        }
        if (facts.negative_variance_truncated) {
            limitations.bullets.push_back(
                "存在负方差分量并已截断为 0；解释时应同时查看原始估计。");
            raise_severity(limitations.severity, Severity::warning);
        }
        if (facts.interaction_reduction_recommended) {
            advice.bullets.push_back(
                "Part×Operator 交互 p>0.25，传统流程可考虑缩减模型；当前结果仍保留完整交互。");
        }
        advice.bullets.push_back(
            "将 %Study Var、%Tolerance 和现场公差一起看，不要把单个百分比写成量具合格。");
        return;
    }
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
        const auto slope = page.facts.msa.has_value()
            ? page.facts.msa->slope : number_in_column(*summary, "Slope");
        const auto bias_low = page.facts.msa.has_value()
            ? page.facts.msa->bias_low : number_in_column(*summary, "Bias Low");
        const auto bias_high = page.facts.msa.has_value()
            ? page.facts.msa->bias_high : number_in_column(*summary, "Bias High");
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
        const auto p_value = page.facts.msa.has_value()
            ? page.facts.msa->p_value : number_in_column(*summary, "P");
        const auto cgk = page.facts.msa.has_value()
            ? page.facts.msa->cgk : number_in_column(*summary, "Cgk");
        const auto tolerance = page.facts.msa.has_value()
            ? page.facts.msa->tolerance_percent
            : number_in_column(*summary, "%Tolerance");
        if (!p_value.has_value() || !cgk.has_value()) {
            limitations.bullets.push_back("Type 1 表缺少 P 或 Cgk，不能完整评价偏倚与重复性。");
            raise_severity(limitations.severity, Severity::warning);
        } else {
            const double alpha = 1.0 - page.configuration.inference.confidence_level;
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
    if (page.facts.reliability.has_value()) {
        const auto& facts = *page.facts.reliability;
        if (!facts.identifiable) {
            limitations.bullets.push_back(
                facts.not_computed_reason.empty()
                    ? "可靠性结果当前不可识别，不能估计寿命分位数。"
                    : "可靠性结果不可识别（" + facts.not_computed_reason
                          + "），不能估计寿命分位数。");
            raise_severity(limitations.severity, Severity::warning);
            return;
        }
        if (page.method_name == "Kaplan-Meier") {
            conclusion.bullets.push_back(
                "Kaplan-Meier 有效观测 "
                + std::to_string(facts.valid_count.value_or(0))
                + "，失效 "
                + std::to_string(facts.failure_count.value_or(0))
                + "，删失 "
                + std::to_string(facts.censored_count.value_or(0)) + "。");
            if (facts.censored_count.has_value() && *facts.censored_count > 0) {
                limitations.bullets.push_back(
                    "存在右删失；尾部生存率由较少的风险集支持，不能把删失时间当作失效时间。");
                raise_severity(limitations.severity, Severity::warning);
            }
            if (!facts.median_life.has_value()) {
                limitations.bullets.push_back(
                    "生存曲线未下降到 0.5，中位寿命不可估计。");
            }
            advice.bullets.push_back(
                "按目标任务时间读取生存概率和置信区间；比较方案时使用分层或 Log-rank。");
            return;
        }
        if (facts.shape.has_value()
            && (page.method_name == "Weibull Lifetime"
                || page.method_name == "3-Parameter Weibull Lifetime")) {
            conclusion.bullets.push_back(
                "Weibull 形状参数 β = " + std::to_string(*facts.shape)
                + (*facts.shape > 1.0 ? "，提示失效率随时间上升。"
                   : *facts.shape < 1.0 ? "，提示早期失效型失效率随时间下降。"
                                        : "，接近恒定失效率。"));
            if (page.method_name == "3-Parameter Weibull Lifetime"
                && facts.threshold.has_value()) {
                conclusion.bullets.push_back(
                    "阈值 λ = " + std::to_string(*facts.threshold)
                    + "；百分位寿命按 t_p = λ + α[-ln(1-p)]^(1/β) 计算。");
                limitations.bullets.push_back(
                    "三参数拟合未拒绝假设不等于已证明寿命服从三参数 Weibull。");
            }
            if (!facts.converged) {
                limitations.bullets.push_back("Weibull 参数未收敛，分位寿命不能作为决策依据。");
                raise_severity(limitations.severity, Severity::warning);
            }
        } else if (page.method_name == "Exponential Lifetime"
                   || page.method_name == "2-Parameter Exponential Lifetime") {
            conclusion.bullets.push_back(
                page.method_name == "2-Parameter Exponential Lifetime"
                    && facts.threshold.has_value()
                    ? "两参数指数估计了阈值 λ = " + std::to_string(*facts.threshold)
                          + " 后的恒定失效率。"
                    : "指数模型估计的是恒定失效率假设下的寿命分布。");
            limitations.bullets.push_back(
                "指数拟合未拒绝假设不等于已证明寿命服从指数分布。");
            raise_severity(limitations.severity, Severity::warning);
        } else if (page.method_name == "Lognormal Lifetime"
                   || page.method_name == "3-Parameter Lognormal Lifetime") {
            conclusion.bullets.push_back(
                page.method_name == "3-Parameter Lognormal Lifetime"
                    && facts.threshold.has_value()
                    ? "三参数对数正态估计了阈值 λ = " + std::to_string(*facts.threshold)
                          + "；分位寿命按 λ + exp(μ + σ Φ⁻¹(p)) 计算。"
                    : "对数正态模型估计了右删失下的位置/尺度参数；分位寿命由 "
                          "exp(μ + σ Φ⁻¹(p)) 给出。");
            if (!facts.converged) {
                limitations.bullets.push_back(
                    "对数正态参数未收敛，分位寿命不能作为决策依据。");
                raise_severity(limitations.severity, Severity::warning);
            }
            limitations.bullets.push_back(
                "对数正态拟合未拒绝假设并不等于已证明寿命服从对数正态。");
        }
        advice.bullets.push_back(
            "报告模型参数和删失处理，并用现场失效机理验证模型假设；不要把分位寿命当成单件保证寿命。");
        return;
    }
    if (page.method_name == "Kaplan-Meier") {
        if (!page.facts.reliability.has_value()
            && (page.tables.empty() || page.tables.front().rows.empty())) {
            limitations.bullets.push_back("Kaplan-Meier 生存表为空，不能估计生存曲线或中位寿命。");
            raise_severity(limitations.severity, Severity::warning);
            return;
        }
        const std::size_t row_count = page.tables.empty()
            ? 0 : page.tables.front().rows.size();
        const std::size_t censor_count = page.facts.reliability.has_value()
            && page.facts.reliability->censored_count.has_value()
            ? *page.facts.reliability->censored_count : [&] {
            const auto& table = page.tables.front();
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
        conclusion.bullets.push_back("Kaplan-Meier 曲线基于 " + std::to_string(row_count)
                                     + " 个时间点；删失数合计 " + std::to_string(censor_count) + "。");
        if (censor_count > 0) {
            limitations.bullets.push_back("存在右删失；尾部生存率由较少的风险集支持，不能把删失时间当作失效时间。");
            raise_severity(limitations.severity, Severity::warning);
        }
        advice.bullets.push_back("按目标任务时间读取生存概率，并报告置信区间；需要比较产品/方案时使用分层或组间检验。");
    } else {
        const auto shape = page.facts.reliability.has_value()
            ? page.facts.reliability->shape
            : (page.method_name == "Weibull Lifetime" && !page.tables.empty()
                   ? number_after(page.tables.front(), "Shape") : std::nullopt);
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
    if (!page.facts.power.has_value()
        && (page.tables.empty() || page.tables.front().rows.empty())) {
        limitations.bullets.push_back("功效/样本量结果表为空，不能给出设计建议。");
        raise_severity(limitations.severity, Severity::warning);
        return;
    }
    const auto power = page.facts.power.has_value()
        ? page.facts.power->power
        : number_in_column(page.tables.front(), "Power");
    const auto effect = page.facts.power.has_value()
        ? page.facts.power->effect_size
        : number_in_column(page.tables.front(), "Effect Size");
    if (!power.has_value() || !effect.has_value()) {
        limitations.bullets.push_back("缺少 Power 或 Effect Size；结果不能用于样本量决策。");
        raise_severity(limitations.severity, Severity::warning);
        return;
    }
    if (page.method_name == "T Test Power") {
        conclusion.bullets.push_back("在效应量 = " + std::to_string(*effect)
            + "、α = " + std::to_string(page.configuration.power.alpha)
            + " 下，估计功效 = " + std::to_string(*power) + "。");
        advice.bullets.push_back("将效应量预先定义为具有工程意义的最小差异，并同时考虑实际脱落、分组和方差不确定性。");
    } else {
        conclusion.bullets.push_back("按目标功效 " + std::to_string(page.configuration.power.target)
            + " 估计所需样本量；效应量假设为 " + std::to_string(*effect) + "。");
        advice.bullets.push_back("样本量结果是模型和效应量假设的条件结果；试验前应进行敏感性分析并向上取整到可执行的分组方案。");
    }
    if (page.configuration.power.effect_size <= 0.0 || page.configuration.power.alpha <= 0.0
        || page.configuration.power.alpha >= 1.0) {
        limitations.bullets.push_back("效应量或 α 配置不在有效范围，不能把数值当作正式设计依据。");
        raise_severity(limitations.severity, Severity::error);
    }
}

void add_forecast_rules(const domain::OutputPage& page,
                        domain::InterpretationSection& conclusion,
                        domain::InterpretationSection& advice,
                        domain::InterpretationSection& limitations)
{
    if (!page.facts.forecast.has_value() && page.tables.empty()) {
        limitations.bullets.push_back("预测结果表为空，不能评价预测或未来期。");
        raise_severity(limitations.severity, Severity::warning);
        return;
    }
    if (page.facts.forecast.has_value()) {
        const auto& facts = *page.facts.forecast;
        conclusion.bullets.push_back("预测误差指标已计算；"
            + (facts.mape.has_value()
                   ? "MAPE = " + std::to_string(*facts.mape) + "。" : ""));
        if (facts.mase.has_value() && *facts.mase > 1.0) {
            advice.bullets.push_back("MASE > 1，当前模型不优于朴素基准；上线前应比较替代模型或扩大验证窗口。");
        }
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
        if (page.configuration.time_series.seasonal_period < 2) {
            limitations.bullets.push_back("季节周期小于 2，不能支持有意义的季节性解释。");
            raise_severity(limitations.severity, Severity::warning);
        }
        advice.bullets.push_back("预测区间反映模型不确定性，不是规格上下限；应通过滚动验证检查不同预测期的稳定性。");
    } else {
        advice.bullets.push_back("未来预测依赖平滑/ARIMA 模型假设；应检查残差、自相关和结构变化，避免外推超出历史范围。");
    }
}

void add_regression_rules(const domain::OutputPage& page,
                          domain::InterpretationSection& conclusion,
                          domain::InterpretationSection& advice,
                          domain::InterpretationSection& limitations)
{
    if (!page.facts.regression.has_value()) {
        return;
    }
    const auto& regression = *page.facts.regression;
    if (regression.rank_deficient) {
        limitations.bullets.push_back("设计矩阵秩亏，回归系数不可解释。");
        raise_severity(limitations.severity, Severity::error);
        return;
    }
    if (regression.r_squared.has_value()) {
        conclusion.bullets.push_back(
            "R² = " + std::to_string(*regression.r_squared)
            + " 只描述当前样本拟合程度，不能单独判定模型合格。");
    }
    if (regression.error_degrees_of_freedom.has_value()
        && *regression.error_degrees_of_freedom <= 0.0) {
        limitations.bullets.push_back("误差自由度不足，不输出 t、F 与 P。");
        raise_severity(limitations.severity, Severity::warning);
    }
    if (regression.influential_count > 0) {
        limitations.bullets.push_back(
            "存在 " + std::to_string(regression.influential_count)
            + " 个影响点提示，解释层不会自动删除这些观测。");
        raise_severity(limitations.severity, Severity::warning);
    }
    if (regression.max_vif.has_value() && *regression.max_vif > 5.0) {
        advice.bullets.push_back(
            "最大 VIF = " + std::to_string(*regression.max_vif)
            + "，提示共线性调查，不会自动删除预测变量。");
    }
    if (regression.assumption_status == "evidence_against") {
        limitations.bullets.push_back(
            "残差假设检查提供了需要调查的证据；不能把系数显著性直接写成因果关系。");
        raise_severity(limitations.severity, Severity::warning);
    }
    advice.bullets.push_back(
        "结合残差对拟合值图、残差顺序图、杠杆值和 Cook's D 后再解释模型。");
}

void add_anova_rules(const domain::OutputPage& page,
                     domain::InterpretationSection& conclusion,
                     domain::InterpretationSection& advice,
                     domain::InterpretationSection& limitations)
{
    if (!page.facts.anova.has_value()) {
        return;
    }
    const auto& anova = *page.facts.anova;
    if (!anova.estimable || anova.not_estimable_term_count > 0) {
        limitations.bullets.push_back(
            "存在不可估计项或误差自由度不足，这些项不输出伪造 F/P。");
        raise_severity(limitations.severity, Severity::warning);
    }
    if (anova.significant_terms.empty()) {
        conclusion.bullets.push_back(
            "在当前显著性水平下，没有可报告为显著的 ANOVA 项；这不等于各组完全相同。");
    } else {
        std::ostringstream text;
        for (std::size_t i = 0; i < anova.significant_terms.size(); ++i) {
            if (i != 0) {
                text << "、";
            }
            text << anova.significant_terms[i];
        }
        conclusion.bullets.push_back("当前有统计证据的项：" + text.str() + "。");
    }
    if (anova.family_confidence_level.has_value()) {
        conclusion.bullets.push_back(
            "Tukey 同时置信水平 = " + std::to_string(*anova.family_confidence_level)
            + "，显著比较对数 = " + std::to_string(anova.tukey_significant_pairs)
            + "；区间含 0 不显著。");
        advice.bullets.push_back("不要把逐比较 alpha 当成家族错误率。");
    } else {
        advice.bullets.push_back("总体 F 显著只说明至少一组不同，需要多重比较确定具体组对。");
    }
    if (anova.assumption_status == "evidence_against") {
        limitations.bullets.push_back(
            "残差正态或方差齐性检查提供了需要调查的证据，不能把 p 值直接写成工程差异。");
        raise_severity(limitations.severity, Severity::warning);
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
               || page.method_name.find("Stability") != std::string::npos
               || page.method_name.find("Gage R&R") != std::string::npos) {
        add_msa_rules(page, conclusion, advice, limitations);
    } else if (page.method_name == "Attribute Agreement Analysis") {
        if (page.facts.msa.has_value()) {
            const auto& facts = *page.facts.msa;
            conclusion.bullets.push_back(
                "属性一致性报告观察一致率与 Kappa；≥3 名评估者使用 Fleiss，两两比较仍用 Cohen。");
            limitations.bullets.push_back(
                "拒绝 Kappa=0 不等于已证明评估者一致；Kappa 只描述超出偶然的绝对一致率。");
            raise_severity(limitations.severity, Severity::info);
            if (facts.ratings_are_ordinal && facts.kendall_available) {
                conclusion.bullets.push_back(
                    "有序评级已计算 Kendall 系数；拒绝 W=0 或 τ=0 不等于已证明有序一致。");
            } else if (facts.ratings_are_ordinal) {
                limitations.bullets.push_back(
                    "已请求有序评级，但 Kendall 不可识别或等级不足；未伪造 W=1。");
            } else {
                advice.bullets.push_back(
                    "名义评级看 Kappa；有序评级请设置 ordinal=true 以计算 Kendall W/τ。");
            }
        } else {
            limitations.bullets.push_back(
                "属性一致性缺少 Facts，不能从表头反解析 Kappa 或 Kendall。");
            raise_severity(limitations.severity, Severity::warning);
        }
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
    add_regression_rules(page, conclusion, advice, limitations);
    add_anova_rules(page, conclusion, advice, limitations);
    if (page.facts.descriptive.has_value() && conclusion.bullets.empty()) {
        const auto& facts = *page.facts.descriptive;
        conclusion.bullets.push_back(
            "有效观测 N = " + std::to_string(facts.n)
            + "，缺失 N* = " + std::to_string(facts.missing_count)
            + (facts.mean.has_value()
                   ? "，均值为 " + std::to_string(*facts.mean) : "")
            + "。描述统计不检验分布假设。");
    }
    if (page.facts.chi_square.has_value() && conclusion.bullets.empty()) {
        const auto& facts = *page.facts.chi_square;
        conclusion.bullets.push_back(
            "Pearson χ²"
            + (facts.statistic.has_value() ? " = " + std::to_string(*facts.statistic) : "")
            + (facts.p_value.has_value()
                   ? "，P = " + std::to_string(*facts.p_value) : "")
            + "。P 值只描述当前列联表与独立性假设的一致程度，不能证明因果关系。");
        if (facts.expected_count_warning) {
            raise_severity(conclusion.severity, Severity::warning);
            limitations.bullets.push_back("存在期望频数过小的单元格，卡方近似可能不可靠。");
        }
    }
    if (page.facts.nonparametric.has_value() && conclusion.bullets.empty()) {
        const auto& facts = *page.facts.nonparametric;
        conclusion.bullets.push_back(
            facts.method + " 使用 " + facts.approximation + " 近似"
            + (facts.p_value.has_value()
                   ? "，P = " + std::to_string(*facts.p_value) : "")
            + (facts.tie_correction ? "，已做 ties 修正" : "")
            + "。未拒绝原假设不能证明两组或各组分布相同。");
        if (facts.small_sample_warning) {
            raise_severity(limitations.severity, Severity::warning);
            limitations.bullets.push_back("存在小样本组，近似 P 值只作提示。");
        }
    }
    if (page.facts.logistic.has_value() && conclusion.bullets.empty()) {
        const auto& facts = *page.facts.logistic;
        if (!facts.converged || facts.complete_separation) {
            raise_severity(conclusion.severity, Severity::warning);
            conclusion.bullets.push_back(
                facts.complete_separation
                    ? "模型出现完全分离，极大似然估计可能不存在，不对系数给出稳定解释。"
                    : "IRLS 未收敛，不对发散系数给出稳定解释。");
        } else {
            std::string hl_text = "，Hosmer–Lemeshow 未计算";
            if (facts.hosmer_lemeshow_status == "computed"
                && facts.hosmer_lemeshow_p.has_value()) {
                const bool reject = *facts.hosmer_lemeshow_p < 0.05;
                hl_text = reject
                    ? "，在 α = 0.05 下拒绝拟合不足（Hosmer–Lemeshow P = "
                        + std::to_string(*facts.hosmer_lemeshow_p) + "）"
                    : "，在 α = 0.05 下未拒绝拟合不足（Hosmer–Lemeshow P = "
                        + std::to_string(*facts.hosmer_lemeshow_p) + "）";
            }
            conclusion.bullets.push_back(
                "二元 Logistic 已收敛" + hl_text
                + "。未拒绝拟合不足不能说明模型已充分"
                + (facts.high_leverage_count > 0
                       ? "；检测到 " + std::to_string(facts.high_leverage_count)
                             + " 个高杠杆观测"
                       : "")
                + "。系数解释依赖事件编码和 complete-case 样本。");
        }
    }
    if (page.facts.pca.has_value() && conclusion.bullets.empty()) {
        const auto& facts = *page.facts.pca;
        conclusion.bullets.push_back(
            "PCA 模式为 " + facts.mode + "，保留 "
            + std::to_string(facts.retained_component_count) + " 个主成分"
            + (facts.observation_count > 0
                   ? "，有效观测 " + std::to_string(facts.observation_count) : "")
            + "，检测到 " + std::to_string(facts.anomaly_count)
            + " 个 T²/Q 异常观测。异常阈值只作诊断，T²/Q 超限不是过程合格或失控判定。");
        if (!facts.converged) {
            raise_severity(conclusion.severity, Severity::warning);
            limitations.bullets.push_back("特征分解未收敛时不对主成分或异常点给出稳定解释。");
        }
    }
    if (page.facts.distribution_identification.has_value() && conclusion.bullets.empty()) {
        const auto& facts = *page.facts.distribution_identification;
        conclusion.bullets.push_back(
            "个体分布识别按 Anderson-Darling 升序比较四族二参数分布；当前表内最优为 "
            + facts.best_distribution
            + (facts.best_anderson_darling.has_value()
                   ? "（AD = " + std::to_string(*facts.best_anderson_darling) + "）"
                   : "")
            + "。排序结果不证明数据服从该分布，也不自动改写过程能力默认方法。");
    }
    if (page.facts.variance.has_value() && conclusion.bullets.empty()) {
        const auto& facts = *page.facts.variance;
        conclusion.bullets.push_back(
            "等方差检验方法为 " + facts.method
            + (facts.p_value.has_value()
                   ? "，P = " + std::to_string(*facts.p_value) : "")
            + "。未拒绝原假设不能证明方差相等；F 检验依赖正态假设。");
    }
    if (page.method_name == "Response Optimization" && conclusion.bullets.empty()) {
        conclusion.bullets.push_back(
            "响应优化在编码 ±1 空间枚举候选组合；缺协方差时置信/预测区间不可用，"
            "最佳组合不能外推到设计空间之外。");
    }

    std::size_t out_of_control = 0;
    std::optional<double> cpk;
    std::optional<double> ppk;
    if (page.facts.capability.has_value()) {
        cpk = page.facts.capability->cpk;
        ppk = page.facts.capability->ppk;
    }
    if (page.facts.spc.has_value()
        && page.facts.spc->out_of_control_count.has_value()) {
        out_of_control = *page.facts.spc->out_of_control_count;
    }
    const bool need_legacy_summary_scan =
        !page.facts.capability.has_value()
        || !page.facts.spc.has_value()
        || !page.facts.spc->out_of_control_count.has_value();
    for (const auto& table : page.tables) {
        if (!need_legacy_summary_scan) {
            break;
        }
        for (const auto& row : table.rows) {
            for (std::size_t index = 0; index < row.size(); ++index) {
                if ((!page.facts.spc.has_value()
                     || !page.facts.spc->out_of_control_count.has_value())
                    && row[index].find("超限点数") != std::string::npos
                    && index + 1 < row.size()) {
                    try {
                        out_of_control += static_cast<std::size_t>(std::stoul(row[index + 1]));
                    } catch (...) {
                    }
                }
                if (!page.facts.capability.has_value()
                    && index + 1 < row.size() && row[index] == "Cpk") {
                    try { cpk = std::stod(row[index + 1]); } catch (...) {}
                }
                if (!page.facts.capability.has_value()
                    && index + 1 < row.size() && row[index] == "Ppk") {
                    try { ppk = std::stod(row[index + 1]); } catch (...) {}
                }
            }
        }
    }
    if (cpk.has_value()) {
        raise_severity(conclusion.severity, *cpk < 1.33 ? Severity::warning : Severity::info);
        const std::string assumption = page.facts.capability.has_value()
            ? page.facts.capability->assumption_status : "not_verified";
        conclusion.bullets.push_back("Cpk = " + std::to_string(*cpk)
                                 + (*cpk < 1.33
                                        ? "，低于项目提示基准 1.33，需要调查过程能力。"
                                        : "，达到项目提示基准 1.33；这不是已验证的过程合格结论。"));
        if (assumption != "verified") {
            limitations.bullets.push_back(
                "过程能力未验证稳定性与正态性（assumption_status=" + assumption
                + "），不能把 Cpk/Ppk 直接写成合格判定。");
        }
    }
    if (ppk.has_value()) {
        conclusion.bullets.push_back("Ppk = " + std::to_string(*ppk)
                                 + (*ppk < 1.33
                                        ? "，整体过程表现低于 1.33 提示基准。"
                                        : "，整体过程表现达到 1.33 提示基准。"));
    }
    if (page.facts.capability.has_value()
        && (page.facts.capability->method == "johnson"
            || page.facts.capability->method == "non_normal"
            || page.facts.capability->method == "between_within")) {
        if (page.facts.capability->method == "between_within") {
            limitations.bullets.push_back(
                "组间/组内能力使用 σ_BW 计算 Cp/Cpk、样本标准差计算 Pp/Ppk；"
                "未验证稳定性不等于过程合格。");
            raise_severity(limitations.severity, Severity::warning);
        } else {
        limitations.bullets.push_back(
            page.facts.capability->method == "johnson"
                ? "Johnson 变换后的 Pp/Ppk 是变换尺度上的 overall 指数；"
                  "未拒绝变换后正态假设不等于原始数据服从正态分布，也不能写成过程合格。"
                : "非正态 Z-score Pp/Ppk 依赖所选分布的 CDF；拟合未拒绝假设不等于已证明"
                  "过程服从该分布，也不能写成过程合格。");
        raise_severity(limitations.severity, Severity::warning);
        }
    }
    if (out_of_control > 0) {
        raise_severity(conclusion.severity, Severity::warning);
        conclusion.bullets.push_back(
            "发现 " + std::to_string(out_of_control) + " 个控制图超限点。");
        advice.bullets.push_back("建议回查对应原始行并确认特殊原因。");
    }
    if (page.method_name == "Correlation") {
        conclusion.bullets.push_back("相关系数表示变量关联方向与强度；P-Value 反映在零相关假设下观察到当前系数的证据强度，"
                                 "不能单独证明因果关系。");
    } else if (page.method_name == "One-Sample T"
               || page.method_name == "Two-Sample T") {
        conclusion.bullets.push_back("请结合 P-Value 与均值差置信区间判断差异；统计显著不等于工程差异具有实际重要性。");
    } else if (page.method_name == "One-Way ANOVA"
               && !page.facts.anova.has_value()) {
        conclusion.bullets.push_back("ANOVA 的总体 F 检验显著时，只能说明至少存在一组均值差异；"
                                 "需要后续多重比较确定具体组对。");
    }
    if (page.facts.regression.has_value() && conclusion.bullets.empty()) {
        const auto& regression = *page.facts.regression;
        if (regression.r_squared.has_value()) {
            conclusion.bullets.push_back(
                "R² = " + std::to_string(*regression.r_squared)
                + " 只描述当前样本拟合程度，不能单独判定模型合格。");
        }
    }
    std::optional<double> sigma_z;
    if (page.facts.spc.has_value()) {
        sigma_z = page.facts.spc->sigma_z;
    }
    if (sigma_z.has_value() && *sigma_z > 0.0) {
        const std::string chart_name = page.method_name.find("Laney") != std::string::npos
            ? page.method_name : "当前";
        conclusion.bullets.push_back(chart_name + " Sigma Z = "
            + std::to_string(*sigma_z)
            + (*sigma_z > 1.0
                   ? "，存在过度离散，传统控制限可能过窄。"
                   : "，控制限已按离散程度进行调整。"));
    } else {
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
    }
    if (page.method_name == "Pareto Chart"
        && (page.facts.pareto.has_value()
            || (!page.tables.empty() && !page.tables.front().rows.empty()))) {
        if (page.facts.pareto.has_value()) {
            const auto& facts = *page.facts.pareto;
            conclusion.bullets.push_back(
                "最大类别为“" + facts.largest_category + "”，计数 "
                + (facts.largest_count.has_value() ? std::to_string(*facts.largest_count) : "未知")
                + "，单项占比 "
                + (facts.largest_percent.has_value()
                       ? std::to_string(*facts.largest_percent) : "未知") + "%。");
            advice.bullets.push_back(
                "前 " + std::to_string(std::min<std::size_t>(
                    2, page.tables.empty() ? 0 : page.tables.front().rows.size()))
                + " 个类别累计占比 "
                + (facts.top_categories_percent.has_value()
                       ? std::to_string(*facts.top_categories_percent) : "未知") + "%；"
                "应优先结合现场原因验证，而不是直接假设存在 80/20 规律。");
        } else {
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
