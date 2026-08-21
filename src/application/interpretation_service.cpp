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
    if (page.facts.doe.has_value()
        && !page.facts.doe->largest_standardized_effect_term.empty()) {
        const auto& doe = *page.facts.doe;
        conclusion.bullets.push_back(
            "效应 Pareto 最大项为 " + doe.largest_standardized_effect_term
            + "；参考线方法为 "
            + (doe.pareto_method.empty() ? "未指定" : doe.pareto_method)
            + "。条越过参考线只提供统计证据，不表示过程合格。");
    }
    if (page.facts.doe.has_value() && page.facts.doe->factor_count >= 4
        && !page.facts.doe->cube_plot_available) {
        limitations.bullets.push_back(
            "未生成立方图（仅支持 2–3 个因子）；高维设计请结合 Pareto 与主效应/交互图。");
        raise_severity(limitations.severity, Severity::warning);
    }
    if (page.facts.doe.has_value() && page.facts.doe->contour_plot_available
        && !page.facts.doe->contour_x_factor.empty()
        && !page.facts.doe->contour_y_factor.empty()) {
        std::string contour_text = "等值线/曲面轴为 "
            + page.facts.doe->contour_x_factor + " 与 "
            + page.facts.doe->contour_y_factor;
        if (!page.facts.doe->held_factor_names.empty()) {
            bool any_actual = false;
            for (const auto& actual : page.facts.doe->held_actual_values) {
                if (!actual.empty()) {
                    any_actual = true;
                    break;
                }
            }
            if (any_actual) {
                contour_text += "；其余因子实际单位 hold：";
                for (std::size_t i = 0; i < page.facts.doe->held_factor_names.size(); ++i) {
                    if (i > 0) {
                        contour_text += "、";
                    }
                    contour_text += page.facts.doe->held_factor_names[i];
                    if (i < page.facts.doe->held_actual_values.size()
                        && !page.facts.doe->held_actual_values[i].empty()) {
                        contour_text += "=" + page.facts.doe->held_actual_values[i];
                    }
                    if (i < page.facts.doe->held_coded_values.size()) {
                        contour_text += "（编码 "
                            + std::to_string(page.facts.doe->held_coded_values[i]) + "）";
                    }
                }
            } else {
                contour_text += "；其余因子编码 hold=0：";
                for (std::size_t i = 0; i < page.facts.doe->held_factor_names.size(); ++i) {
                    if (i > 0) {
                        contour_text += "、";
                    }
                    contour_text += page.facts.doe->held_factor_names[i];
                }
            }
        }
        contour_text += "。";
        conclusion.bullets.push_back(std::move(contour_text));
    }
    if (page.facts.doe.has_value() && page.facts.doe->residual_count > 0) {
        limitations.bullets.push_back(
            "残差图供调查残差形态；直方图不用于证明正态。");
        raise_severity(limitations.severity, Severity::warning);
    }
    if (page.method_name == "2-Level Factorial Response Analysis") {
        limitations.bullets.push_back(
            "等值线/曲面在编码 [-1,1] 空间求值；非轴因子默认 hold 编码 0，"
            "也可按实际单位 hold；二水平模型无平方项，不能把曲面曲率当成已估计。");
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
    if (page.method_name == "Response Optimization" && page.facts.doe.has_value()) {
        const auto& doe = *page.facts.doe;
        std::ostringstream response_text;
        for (std::size_t index = 0; index < doe.response_names.size(); ++index) {
            if (index != 0) {
                response_text << "、";
            }
            response_text << doe.response_names[index];
        }
        if (doe.multi_response) {
            conclusion.bullets.push_back(
                "多响应 Desirability 优化覆盖 "
                + std::to_string(doe.response_count) + " 个响应（"
                + response_text.str() + "）；总体 D 为几何平均。");
        }
        if (doe.best_overall_desirability.has_value()) {
            conclusion.bullets.push_back(
                "最佳候选总体 Desirability = "
                + std::to_string(*doe.best_overall_desirability)
                + "；这是在编码 ±1 设计空间内枚举得到的结果。");
        } else {
            conclusion.bullets.push_back(
                "响应优化在编码 ±1 空间枚举候选组合；结果不能外推到设计空间之外。");
        }
        if (!doe.prediction_interval_available) {
            limitations.bullets.push_back(
                "缺协方差或残差自由度不足，置信/预测区间不可用；"
                "不要仅凭点预测确定最优设置。");
            raise_severity(limitations.severity, Severity::warning);
        }
        advice.bullets.push_back(
            "多响应冲突时优先核对各响应 Desirability 与权重；"
            "确认性试验应覆盖总体 D 靠前且工程上可实施的组合。");
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
        if (facts.by_part_plot_available) {
            conclusion.bullets.push_back(
                "按零件图展示各零件重复测量的离散程度；同一零件上点越集中表示重复性越小。"
                "该图不单独判定量具是否合格。");
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
            ? page.facts.msa->slope : std::nullopt;
        const auto bias_low = page.facts.msa.has_value()
            ? page.facts.msa->bias_low : std::nullopt;
        const auto bias_high = page.facts.msa.has_value()
            ? page.facts.msa->bias_high : std::nullopt;
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
            if (page.facts.msa.has_value()) {
                const auto& facts = *page.facts.msa;
                if (facts.average_bias.has_value()) {
                    conclusion.bullets.push_back(
                        "Average Bias = " + std::to_string(*facts.average_bias) + "。");
                }
                if (facts.average_bias_p.has_value()) {
                    conclusion.bullets.push_back(
                        "Average Bias p = " + std::to_string(*facts.average_bias_p) + "。");
                }
                if (facts.intercept_p_value.has_value()) {
                    conclusion.bullets.push_back(
                        "Constant p = " + std::to_string(*facts.intercept_p_value) + "。");
                }
                if (facts.percent_linearity.has_value()) {
                    conclusion.bullets.push_back(
                        "%Linearity = " + std::to_string(*facts.percent_linearity)
                        + "；基于用户提供的过程变差（6σ）。");
                }
                if (facts.slope_p_value.has_value() && *facts.slope_p_value <= 0.05) {
                    limitations.bullets.push_back(
                        "斜率检验 p≤0.05，提示线性显著；应分别解读各参考水平的偏倚，"
                        "不宜只用平均偏倚。");
                    raise_severity(limitations.severity, Severity::warning);
                }
            }
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
        ? (page.facts.power->actual_power.has_value()
               ? page.facts.power->actual_power : page.facts.power->power)
        : number_in_column(page.tables.front(), "Actual Power");
    const auto effect = page.facts.power.has_value()
        ? page.facts.power->effect_size
        : number_in_column(page.tables.front(), "Effect Size");
    const auto target = page.facts.power.has_value()
        ? page.facts.power->target
        : std::optional<double>{page.configuration.power.target};
    if (!power.has_value() || !effect.has_value()) {
        limitations.bullets.push_back("缺少 Actual Power 或 Effect Size；结果不能用于样本量决策。");
        raise_severity(limitations.severity, Severity::warning);
        return;
    }
    if (page.method_name == "T Test Power") {
        conclusion.bullets.push_back("在效应量 = " + std::to_string(*effect)
            + "、α = " + std::to_string(page.configuration.power.alpha)
            + " 下，估计实际功效 = " + std::to_string(*power) + "。");
        advice.bullets.push_back("将效应量预先定义为具有工程意义的最小差异，并同时考虑实际脱落、分组和方差不确定性。");
    } else {
        conclusion.bullets.push_back(
            "在效应量 = " + std::to_string(*effect)
            + (target.has_value()
                   ? "、目标功效 = " + std::to_string(*target) : "")
            + " 下，估计实际功效 = " + std::to_string(*power)
            + (page.facts.power.has_value() && page.facts.power->sample_size.has_value()
                   ? "，对应样本量 n = " + std::to_string(*page.facts.power->sample_size)
                   : "")
            + "。数值是假设条件下的计算值。");
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
    } else if (page.method_name == "Time Series Decomposition") {
        if (page.configuration.time_series.decomposition_seasonal_period < 2) {
            limitations.bullets.push_back("季节周期小于 2，不能支持有意义的季节指数解释。");
            raise_severity(limitations.severity, Severity::warning);
        }
        if (page.configuration.time_series.decomposition_model == "multiplicative") {
            advice.bullets.push_back("乘法分解要求观测值为正；季节指数与趋势外推对结构变化敏感，应核对残差与移动平均边界。");
        } else {
            advice.bullets.push_back("加法分解的季节指数与线性趋势外推对结构变化敏感；应核对残差与非等间隔时间诊断。");
        }
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
    if (regression.influential_count > 0
        || regression.outlier_count > 0
        || regression.high_leverage_count > 0) {
        limitations.bullets.push_back(
            "存在打标观测，请结合异常观测表调查；解释层不会自动删除这些观测。");
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
        "结合残差对拟合值图、残差顺序图、拟合线图置信/预测带、杠杆值和 Cook's D 后再解释模型。");
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
            + "；区间含 0 不显著。同字母仅表示在本产品 Tukey 近似规则下未显著不同，"
              "不能写成已证明相同。");
        advice.bullets.push_back("不要把逐比较 alpha 当成家族错误率。");
        advice.bullets.push_back(
            "区间图是各组均值的个体置信区间（pooled MSE），不是 Tukey 同时比较。");
        if (anova.tukey_grouping_available) {
            conclusion.bullets.push_back(
                "Grouping Information 使用 "
                + std::to_string(anova.grouping_letter_count)
                + " 个字母；字母来自成对显著矩阵，不改 Studentized Range 近似。");
        }
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
        || page.method_name.find("Factorial") != std::string::npos
        || page.method_name == "Response Optimization") {
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
            advice.bullets.push_back(
                "评估者×零件图是观察一致率，不是量具通过或已证明一致。");
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
               || page.method_name == "Time Series Decomposition"
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
            + "。描述统计不检验分布假设，也不能写成过程合格。");
    }
    if (page.facts.chi_square.has_value()) {
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
        if (facts.plot_available) {
            limitations.bullets.push_back(
                "观察频数热图只展示列联表中的计数分布，不能证明因果关系。");
        }
    }
    if (page.facts.chi_square_gof.has_value()) {
        const auto& facts = *page.facts.chi_square_gof;
        conclusion.bullets.push_back(
            "拟合优度 Pearson χ²"
            + (facts.statistic.has_value() ? " = " + std::to_string(*facts.statistic) : "")
            + (facts.p_value.has_value()
                   ? "，P = " + std::to_string(*facts.p_value) : "")
            + "，DF = "
            + (facts.degrees_of_freedom.has_value()
                   ? std::to_string(*facts.degrees_of_freedom) : "*")
            + "。P 值只描述观察频数与指定比例的一致程度，不能证明总体比例等于假设。");
        if (facts.expected_count_warning) {
            raise_severity(conclusion.severity, Severity::warning);
            limitations.bullets.push_back("存在期望频数过小的类别，卡方近似可能不可靠。");
        }
        if (!facts.validity_status.empty() && facts.validity_status != "ok") {
            raise_severity(limitations.severity, Severity::warning);
            limitations.bullets.push_back(
                "GOF 有效性状态为 " + facts.validity_status
                + (facts.minimum_expected_count.has_value()
                       ? "（最小期望频数 = "
                             + std::to_string(*facts.minimum_expected_count) + "）"
                       : "")
                + "。");
        }
        if (!facts.recommendation.empty()) {
            advice.bullets.push_back(facts.recommendation);
        }
        if (facts.missing_count > 0) {
            limitations.bullets.push_back(
                "缺失 N* = " + std::to_string(facts.missing_count) + "，未进入类别计数。");
        }
    }
    if (page.facts.mcnemar.has_value() && conclusion.bullets.empty()) {
        const auto& facts = *page.facts.mcnemar;
        if (!facts.computable) {
            raise_severity(conclusion.severity, Severity::warning);
            conclusion.bullets.push_back(
                "McNemar 未计算出统计量（非二元、无不一致对或输入不足）。"
                "这只说明当前配对表不支持该检验，不能写成前后比例已证明相同或不同。");
        } else {
            conclusion.bullets.push_back(
                "McNemar（Edwards 连续性校正）χ² = "
                + (facts.chi_square.has_value()
                       ? std::to_string(*facts.chi_square) : "*")
                + "，配对有效 N = " + std::to_string(facts.pair_count)
                + "，不一致对数 b+c = " + std::to_string(facts.discordant)
                + (facts.p_value.has_value()
                       ? "，P = " + std::to_string(*facts.p_value) : "")
                + "。结果只陈述边际比例差异证据，不能写成已证明相同或不同。");
        }
    }
    if (page.facts.cochran_q.has_value() && conclusion.bullets.empty()) {
        const auto& facts = *page.facts.cochran_q;
        if (!facts.computable) {
            conclusion.bullets.push_back(
                "Cochran Q 未计算出统计量（例如列数不足、编码失败或分母退化）。"
                "两列配对请用 McNemar；不得写成已证明各处理阳性率相同或不同。");
        } else {
            conclusion.bullets.push_back(
                "Cochran Q = "
                + (facts.q_statistic.has_value() ? std::to_string(*facts.q_statistic) : "*")
                + "，DF = " + std::to_string(facts.degrees_of_freedom)
                + (facts.p_value.has_value()
                       ? "，P = " + std::to_string(*facts.p_value) : "")
                + "。结果只陈述配对二元处理间差异证据，不能写成已证明阳性率相同或不同。");
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
        if (facts.location_estimate.has_value()
            && facts.ci_lower.has_value() && facts.ci_upper.has_value()) {
            if (facts.method == "wilcoxon_one_sample") {
                conclusion.bullets.push_back(
                    "Walsh 估计中位数为 " + std::to_string(*facts.location_estimate)
                    + "，置信区间 [" + std::to_string(*facts.ci_lower) + ", "
                    + std::to_string(*facts.ci_upper)
                    + "]。区间只描述中位数的可能范围，不能写成已证明等于假设值。");
            } else {
                conclusion.bullets.push_back(
                    "位置差异估计为 " + std::to_string(*facts.location_estimate)
                    + "，置信区间 [" + std::to_string(*facts.ci_lower) + ", "
                    + std::to_string(*facts.ci_upper) + "]。区间只描述位置差的可能范围。");
            }
        } else if (facts.location_estimate.has_value()
                   && facts.ci_lower.has_value()) {
            if (facts.method == "wilcoxon_one_sample") {
                conclusion.bullets.push_back(
                    "Walsh 估计中位数为 " + std::to_string(*facts.location_estimate)
                    + "，置信下界 " + std::to_string(*facts.ci_lower) + "。");
            } else {
                conclusion.bullets.push_back(
                    "位置差异估计为 " + std::to_string(*facts.location_estimate)
                    + "，置信下界 " + std::to_string(*facts.ci_lower) + "。");
            }
        } else if (facts.location_estimate.has_value()
                   && facts.ci_upper.has_value()) {
            if (facts.method == "wilcoxon_one_sample") {
                conclusion.bullets.push_back(
                    "Walsh 估计中位数为 " + std::to_string(*facts.location_estimate)
                    + "，置信上界 " + std::to_string(*facts.ci_upper) + "。");
            } else {
                conclusion.bullets.push_back(
                    "位置差异估计为 " + std::to_string(*facts.location_estimate)
                    + "，置信上界 " + std::to_string(*facts.ci_upper) + "。");
            }
        } else if (facts.location_estimate.has_value()
                   && facts.method == "wilcoxon_one_sample") {
            conclusion.bullets.push_back(
                "Walsh 估计中位数为 " + std::to_string(*facts.location_estimate)
                + "。点估计不能写成已证明等于假设值。");
        }
        if (facts.plot_point_count > 0) {
            limitations.bullets.push_back(
                "箱线图与个体值图基于 " + std::to_string(facts.plot_point_count)
                + " 个有效观测，缺失单元格未进入图形。");
        }
        if (facts.dunn_available) {
            conclusion.bullets.push_back(
                "Dunn–Bonferroni 成对比较共 "
                + std::to_string(facts.posthoc_pair_count)
                + " 对；Grouping 字母只反映该规则下的显著矩阵，不能写成组间已证明相同或不同。");
        }
        if (facts.steel_dwass_available) {
            conclusion.bullets.push_back(
                "Steel–Dwass（近似）成对比较共 "
                + std::to_string(facts.posthoc_pair_count)
                + " 对；Grouping 字母只反映该规则下的显著矩阵，不能写成组间已证明相同或不同。");
        }
        if (facts.nemenyi_available) {
            conclusion.bullets.push_back(
                "Nemenyi（近似）成对比较共 "
                + std::to_string(facts.posthoc_pair_count)
                + " 对；Grouping 字母只反映该规则下的显著矩阵，不能写成处理间已证明相同或不同。");
        }
        if (facts.method == "friedman" && facts.statistic.has_value()
            && facts.p_value.has_value()) {
            conclusion.bullets.push_back(
                "Friedman S（调整后）= " + std::to_string(*facts.statistic)
                + "，P = " + std::to_string(*facts.p_value)
                + "。这只陈述区组设计下处理间秩差异证据，不能写成已证明相同或不同。");
        }
        if ((facts.method == "sign_test" || facts.method == "sign_test_paired")
            && facts.p_value.has_value()) {
            conclusion.bullets.push_back(
                "符号检验（二项精确）P = " + std::to_string(*facts.p_value)
                + "。未拒绝原假设不能证明中位数等于假设值。");
        }
        if (facts.method == "wilcoxon_one_sample" && facts.p_value.has_value()) {
            conclusion.bullets.push_back(
                "单样本 Wilcoxon 符号秩 P = " + std::to_string(*facts.p_value)
                + "。未拒绝原假设不能证明中位数等于假设值；Walsh 估计与区间只描述位置。");
        }
        if (facts.method == "mood_median" && facts.p_value.has_value()) {
            conclusion.bullets.push_back(
                "Mood 中位数检验 χ² = "
                + (facts.statistic.has_value() ? std::to_string(*facts.statistic) : "*")
                + "，P = " + std::to_string(*facts.p_value)
                + "。未拒绝原假设不能证明各组中位数相同。");
        }
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
                + (facts.maximum_vif.has_value()
                       ? "；最大 VIF = " + std::to_string(*facts.maximum_vif)
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
    if (page.facts.multi_vari.has_value()) {
        const auto& facts = *page.facts.multi_vari;
        conclusion.bullets.push_back(
            "Multi-Vari 图用 " + std::to_string(facts.factor_count)
            + " 个因子分层显示 " + std::to_string(facts.valid_count)
            + " 个有效测量的均值"
            + (facts.factor_names.size() >= 2
                   ? "（" + facts.factor_names[0] + "、" + facts.factor_names[1]
                         + (facts.factor_names.size() > 2
                                ? "、" + facts.factor_names[2] : "")
                         + "）"
                   : "")
            + "。");
        advice.bullets.push_back(
            "结合因子均值表回查占主导的因子水平；需要检验显著性时使用 ANOVA 或回归。");
        limitations.bullets.push_back(
            "分层均值图只用于探索变异来源，不能把结果写成过程判定。");
        if (facts.missing_count > 0) {
            limitations.bullets.push_back(
                "已跳过 " + std::to_string(facts.missing_count)
                + " 个缺失或不完整行。");
        }
    }
    if (page.facts.tolerance.has_value()) {
        const auto& facts = *page.facts.tolerance;
        std::string interval_text = "容差区间方法为 "
            + (facts.method.empty() ? std::string("未计算") : facts.method);
        if (!facts.method_family.empty()) {
            interval_text += "（" + facts.method_family + "）";
        }
        if (facts.coverage.has_value()) {
            interval_text += "，覆盖率 " + std::to_string(*facts.coverage);
        }
        if (facts.confidence_level.has_value()) {
            interval_text += "，置信水平 " + std::to_string(*facts.confidence_level);
        }
        if (facts.achieved_confidence.has_value()) {
            interval_text += "，achieved confidence "
                + std::to_string(*facts.achieved_confidence);
        }
        if (facts.lower.has_value()) {
            interval_text += "，下限 " + std::to_string(*facts.lower);
        }
        if (facts.upper.has_value()) {
            interval_text += "，上限 " + std::to_string(*facts.upper);
        }
        interval_text += "。正态假设状态为 " + facts.assumption_status
            + "，区间只描述当前样本覆盖，不是规格覆盖或过程判定。";
        conclusion.bullets.push_back(interval_text);
        advice.bullets.push_back(
            "结合直方图与正态性诊断调查覆盖区间；需要对照规格时使用过程能力分析。");
        limitations.bullets.push_back(
            "本方法假设测量近似正态且未在本页验证；单侧使用 Natrella 近似，双侧使用 Howe 近似。");
        if (facts.missing_count > 0) {
            limitations.bullets.push_back(
                "已跳过 " + std::to_string(facts.missing_count)
                + " 个缺失或非法数值。");
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
        std::string note = "。未拒绝原假设不能证明方差相等。";
        if (facts.method == "Bonett") {
            note += "Bonett 为连续分布下的标准差比路径，与中位数 Levene 不同；"
                    "极偏/重尾小样本时宜对照 Levene。";
        } else if (facts.method == "Bartlett") {
            note += "Bartlett 对正态偏离敏感；稳健场景宜对照中位数 Levene。"
                    "正态假设状态未在本页验证。";
        } else if (facts.method.find("F") != std::string::npos
                   || facts.method == "F-test") {
            note += "F 检验依赖正态假设。";
        } else {
            note += "Levene（中位数）为稳健默认路径。";
        }
        conclusion.bullets.push_back(
            "等方差检验方法为 " + facts.method
            + (facts.p_value.has_value()
                   ? "，P = " + std::to_string(*facts.p_value) : "")
            + note);
    }
    if (page.facts.proportion.has_value()) {
        const auto& facts = *page.facts.proportion;
        std::ostringstream text;
        if (facts.kind == "two_sample") {
            text << "两比例检验方法为 " << (facts.method.empty() ? "normal" : facts.method)
                 << "，p̂1 = "
                 << (facts.proportion.has_value() ? std::to_string(*facts.proportion) : "*")
                 << "，p̂2 = "
                 << (facts.second_proportion.has_value()
                         ? std::to_string(*facts.second_proportion) : "*");
            if (facts.difference.has_value()) {
                text << "，差值 = " << *facts.difference;
            }
            if (facts.p_value.has_value()) {
                text << "，Wald P = " << *facts.p_value;
            }
            if (facts.fisher_p_value.has_value()) {
                text << "，Fisher P = " << *facts.fisher_p_value;
            }
            text << "。P 值只描述两组比例差异的证据强度，不是规格判定。";
        } else {
            text << "单比例检验方法为 " << (facts.method.empty() ? "exact" : facts.method);
            if (!facts.ci_method.empty()) {
                text << "（CI=" << facts.ci_method << "）";
            }
            text << "，p̂ = "
                 << (facts.proportion.has_value() ? std::to_string(*facts.proportion) : "*");
            if (facts.hypothesized.has_value()) {
                text << "，H0: p = " << *facts.hypothesized;
            }
            if (facts.p_value.has_value()) {
                text << "，P = " << *facts.p_value;
            }
            text << "。P 值只描述与假设比例的证据强度，不是规格判定。";
        }
        conclusion.bullets.push_back(text.str());
        limitations.bullets.push_back(
            "比例假设未验证独立性与恒定 p（assumption_status="
            + facts.assumption_status + "）。");
        raise_severity(limitations.severity, Severity::warning);
    }
    if (page.facts.box_cox.has_value()) {
        const auto& facts = *page.facts.box_cox;
        std::ostringstream text;
        text << "Box-Cox 选定 λ = " << facts.lambda
             << "，有效观测 N = " << facts.n;
        if (facts.transformed_standard_deviation.has_value()) {
            text << "，变换后标准化 SD = "
                 << *facts.transformed_standard_deviation;
        }
        text << "。概率图只是诊断，不能写成数据已正态。";
        conclusion.bullets.push_back(text.str());
        limitations.bullets.push_back(
            "变换后能力指数（若出现）不是过程合格判定（assumption_status="
            + facts.assumption_status + "）。");
        raise_severity(limitations.severity, Severity::warning);
    }
    if (page.facts.poisson_rate.has_value()) {
        const auto& facts = *page.facts.poisson_rate;
        std::ostringstream text;
        if (facts.kind == "two_sample") {
            text << "双样本泊松率方法为 " << (facts.method.empty() ? "exact" : facts.method);
            if (facts.comparison == "ratio") {
                text << "，比较量=率比 ρ=λ1/λ2"
                     << "，ρ̂ = "
                     << (facts.ratio.has_value() ? std::to_string(*facts.ratio) : "*");
            } else {
                text << "，λ̂1 = "
                     << (facts.rate.has_value() ? std::to_string(*facts.rate) : "*")
                     << "，λ̂2 = "
                     << (facts.second_rate.has_value()
                             ? std::to_string(*facts.second_rate) : "*");
            }
        } else {
            text << "单样本泊松率方法为 " << (facts.method.empty() ? "exact" : facts.method)
                 << "，λ̂ = "
                 << (facts.rate.has_value() ? std::to_string(*facts.rate) : "*");
            if (facts.hypothesized.has_value()) {
                text << "，H0: λ = " << *facts.hypothesized;
            }
        }
        if (facts.p_value.has_value()) {
            text << "，P = " << *facts.p_value;
        }
        text << "。P 值只描述与假设发生率的证据强度，不是规格判定。";
        conclusion.bullets.push_back(text.str());
        limitations.bullets.push_back(
            "泊松率假设未验证独立同质发生率（assumption_status="
            + facts.assumption_status + "）。");
        raise_severity(limitations.severity, Severity::warning);
    }
    if (page.facts.equivalence.has_value()) {
        const auto& facts = *page.facts.equivalence;
        std::ostringstream text;
        text << "等价性检验比较"
             << (facts.kind == "one_proportion" || facts.kind == "two_proportion"
                     ? "比例差"
                     : (facts.kind == "two_sample_ratio" ? "均值比" : "差值"))
             << "与界限 ["
             << (facts.lower.has_value() ? std::to_string(*facts.lower) : "*")
             << ", "
             << (facts.upper.has_value() ? std::to_string(*facts.upper) : "*")
             << "]";
        if (facts.ci_lower.has_value() && facts.ci_upper.has_value()) {
            text << "；CI(" << facts.ci_method << ") 为 ["
                 << *facts.ci_lower << ", " << *facts.ci_upper << "]";
        }
        text << (facts.within_limits
                     ? "，区间落在等价界限内。"
                     : "，区间未落入等价界限内。");
        if (facts.p_lower.has_value() && facts.p_upper.has_value()) {
            text << " 双单侧 P 值分别为 " << *facts.p_lower
                 << " 与 " << *facts.p_upper << "。";
        }
        text << "这只陈述界限关系，不能写成已证明等价。";
        conclusion.bullets.push_back(text.str());
        std::string assumption_text = "正态与独立样本";
        if (facts.kind == "paired") {
            assumption_text =
                "配对差值近似正态，且每对观测来自同一对象/批次的匹配测量";
        } else if (facts.kind == "one_proportion" || facts.kind == "two_proportion") {
            assumption_text = "独立二项试验与大样本正态近似（Wald z-TOST）";
        } else if (facts.kind == "two_sample_ratio") {
            if (facts.ci_method == "tost_ratio_log_1_minus_alpha") {
                assumption_text =
                    "独立对数正态样本与全正观测（均值比 TOST，对数变换 / 几何均值比）";
            } else {
                assumption_text = "独立正态样本与正参考均值（均值比 TOST，非对数）";
            }
        }
        limitations.bullets.push_back(
            "TOST 依赖" + assumption_text + "假设（assumption_status="
            + facts.assumption_status + "）。");
        raise_severity(limitations.severity, Severity::warning);
    }
    if (page.method_name == "Response Optimization" && conclusion.bullets.empty()
        && !page.facts.doe.has_value()) {
        conclusion.bullets.push_back(
            "响应优化在编码 ±1 空间枚举候选组合；缺协方差时置信/预测区间不可用，"
            "最佳组合不能外推到设计空间之外。");
    }

    std::size_t out_of_control = 0;
    std::optional<double> cpk;
    std::optional<double> ppk;
    const bool attribute_capability = page.facts.capability.has_value()
        && (page.facts.capability->method == "binomial"
            || page.facts.capability->method == "poisson");
    if (page.facts.capability.has_value() && !attribute_capability) {
        cpk = page.facts.capability->cpk;
        ppk = page.facts.capability->ppk;
    }
    if (attribute_capability) {
        const auto& capability = *page.facts.capability;
        if (capability.method == "binomial") {
            if (capability.percent_defective.has_value()) {
                conclusion.bullets.push_back(
                    "%Defective = " + std::to_string(*capability.percent_defective)
                    + "（Average P = "
                    + (capability.average_p.has_value()
                           ? std::to_string(*capability.average_p) : "*")
                    + "）。这是当前样本的不合格品率估计，不是过程合格判定。");
            }
            if (capability.process_z.has_value()) {
                conclusion.bullets.push_back(
                    "Process Z = " + std::to_string(*capability.process_z)
                    + "，由 Average P 的标准正态右尾得到。");
            }
        } else if (capability.mean_dpu.has_value()) {
            conclusion.bullets.push_back(
                "Mean DPU = " + std::to_string(*capability.mean_dpu)
                + "。这是当前样本的单位缺陷率估计，不是过程合格判定。");
        }
        limitations.bullets.push_back(
            capability.method == "binomial"
                ? "二项过程能力未验证独立性、恒定 p 与稳定性（assumption_status="
                    + capability.assumption_status + "），不能写成过程合格。"
                : "泊松过程能力未验证独立性、恒定 DPU 与稳定性（assumption_status="
                    + capability.assumption_status + "），不能写成过程合格。");
        raise_severity(limitations.severity, Severity::warning);
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
        if (page.facts.capability.has_value()
            && page.facts.capability->cpk_lower.has_value()
            && page.facts.capability->cpk_upper.has_value()) {
            conclusion.bullets.push_back(
                "Cpk 置信区间 ["
                + std::to_string(*page.facts.capability->cpk_lower) + ", "
                + std::to_string(*page.facts.capability->cpk_upper)
                + "]（Bissell 近似，公式参考）。区间描述抽样不确定性，不是合格判定。");
        }
        if (assumption != "verified") {
            limitations.bullets.push_back(
                "过程能力未验证稳定性与正态性（assumption_status=" + assumption
                + "），不能把 Cpk/Ppk 直接写成合格判定。");
        }
        if (page.facts.capability->normality_p_value.has_value()) {
            limitations.bullets.push_back(
                "Anderson-Darling P = "
                + std::to_string(*page.facts.capability->normality_p_value)
                + "；未拒绝正态假设不等于已证明正态分布。");
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
        if (page.facts.capability->method == "johnson"
            && page.facts.capability->transform_p_value.has_value()) {
            limitations.bullets.push_back(
                "Johnson 变换 AD P = "
                + std::to_string(*page.facts.capability->transform_p_value)
                + "；这是变换尺度上的拟合证据，不是原始数据正态性证明。");
        }
        raise_severity(limitations.severity, Severity::warning);
        }
    }
    if (out_of_control > 0) {
        raise_severity(conclusion.severity, Severity::warning);
        conclusion.bullets.push_back(
            "发现 " + std::to_string(out_of_control) + " 个控制图超限点。");
        advice.bullets.push_back("建议回查对应原始行并确认特殊原因。");
    }
    if (page.facts.spc.has_value() && page.facts.spc->sigma_within.has_value()) {
        const auto& spc = *page.facts.spc;
        std::ostringstream text;
        text << "组内 σ = " << *spc.sigma_within;
        if (spc.sigma_between.has_value()) {
            text << "，组间 σ = " << *spc.sigma_between;
        }
        if (spc.sigma_between_within.has_value()) {
            text << "，σ_BW = " << *spc.sigma_between_within;
        }
        if (!spc.between_within_method.empty()) {
            text << "（" << spc.between_within_method << "）";
        }
        text << "。这些标准差只描述当前子组分解，不是规格判定。";
        conclusion.bullets.push_back(text.str());
    }
    if (page.method_name == "EWMA Chart") {
        limitations.bullets.push_back(
            "EWMA 仅启用 Test 1；Tests 2–8 不适用，超限点需结合原始观测调查。");
        raise_severity(limitations.severity, Severity::info);
    }
    if (page.method_name == "CUSUM Chart") {
        limitations.bullets.push_back(
            "CUSUM 使用累计和决策间隔信号，不套用 Shewhart Tests 1–8；"
            "信号点需结合原始行调查，不是删点指令。");
        raise_severity(limitations.severity, Severity::info);
    }
    if (page.facts.t_test.has_value()) {
        const auto& facts = *page.facts.t_test;
        std::ostringstream text;
        if (facts.kind == "two_sample") {
            text << "双样本 t 检验（"
                 << (facts.variance_method.empty() ? "welch" : facts.variance_method)
                 << "）";
        } else if (facts.kind == "paired") {
            text << "配对 t 检验";
        } else {
            text << "单样本 t 检验";
        }
        if (facts.p_value.has_value()) {
            text << "，P = " << *facts.p_value;
        }
        if (facts.difference.has_value()) {
            text << "，差值 = " << *facts.difference;
        }
        text << "。P 值与置信区间只描述当前样本证据，统计显著不等于工程差异具有实际重要性，也不是过程合格判定。";
        conclusion.bullets.push_back(text.str());
        limitations.bullets.push_back(
            "t 检验正态与独立假设未验证（assumption_status="
            + facts.assumption_status + "）。");
        raise_severity(limitations.severity, Severity::warning);
    }
    if (page.facts.normality.has_value()) {
        const auto& facts = *page.facts.normality;
        std::ostringstream text;
        text << "Anderson-Darling 正态性检验判定为 " << facts.decision;
        if (facts.p_value.has_value()) {
            text << "，P = " << *facts.p_value;
        }
        if (facts.decision == "fail_to_reject") {
            text << "。在 alpha 下未拒绝正态假设，不能写成数据已正态。";
        } else if (facts.decision == "reject") {
            text << "。证据反对正态假设，这不是规格判定。";
        } else {
            text << "。统计量未计算，不能写成数据已正态。";
        }
        conclusion.bullets.push_back(text.str());
        limitations.bullets.push_back(
            "正态假设状态为 " + facts.assumption_status + "。");
        raise_severity(limitations.severity, Severity::warning);
    }
    if (page.facts.outlier_test.has_value()) {
        const auto& facts = *page.facts.outlier_test;
        std::ostringstream text;
        text << "Grubbs 异常值检验";
        if (facts.g_statistic.has_value()) {
            text << " G = " << *facts.g_statistic;
        }
        if (facts.p_value.has_value()) {
            text << "，P = " << *facts.p_value;
        }
        if (facts.p_value.has_value() && *facts.p_value <= facts.alpha) {
            text << "。在 α 下拒绝“无异常值”假设，嫌疑观测需工程调查，"
                    "P 值只描述与正态假设下的一致性，不能写成必须删除或已确认异常。";
        } else {
            text << "。未拒绝“无异常值”假设，不能证明数据中没有异常值。";
        }
        conclusion.bullets.push_back(text.str());
        limitations.bullets.push_back(
            "Grubbs 要求近似正态且至多一个异常值（assumption_status="
            + facts.assumption_status + "）。");
        raise_severity(limitations.severity, Severity::warning);
    }
    if (page.facts.correlation.has_value()) {
        const auto& facts = *page.facts.correlation;
        conclusion.bullets.push_back(
            std::string(facts.method == "spearman" ? "Spearman" : "Pearson")
            + " 相关系数表示变量关联方向与强度；P-Value 反映在零相关假设下的证据强度，"
              "不能单独证明因果关系。未拒绝零相关不能写成已证明无关。");
        limitations.bullets.push_back(
            "相关分析使用 complete-case 对齐（N = " + std::to_string(facts.n)
            + "），assumption_status=" + facts.assumption_status + "。");
    }
    if (page.method_name == "Correlation" && !page.facts.correlation.has_value()) {
        conclusion.bullets.push_back("相关系数表示变量关联方向与强度；P-Value 反映在零相关假设下观察到当前系数的证据强度，"
                                 "不能单独证明因果关系。");
    } else if ((page.method_name == "One-Sample T"
                || page.method_name == "Two-Sample T")
               && !page.facts.t_test.has_value()) {
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
        if (regression.durbin_watson.has_value()) {
            conclusion.bullets.push_back(
                "Durbin-Watson = " + std::to_string(*regression.durbin_watson)
                + "，判定区 = " + regression.durbin_watson_decision
                + "（按输入顺序与 α=0.05 近似 dL/dU）。"
                "不能写成已证明无自相关或存在自相关。");
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
