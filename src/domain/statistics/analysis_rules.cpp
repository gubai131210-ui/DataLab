#include "domain/statistics/analysis_rules.h"

#include "domain/quality_diagnostics.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace datalab::domain::statistics {

using datalab::domain::has_diagnostic_code;

namespace {

std::optional<double> component_percent(
    const std::vector<GageVarianceComponent>& components,
    const std::string& source,
    bool study_variation)
{
    for (const auto& component : components) {
        if (component.source == source) {
            return study_variation
                ? component.percent_study_variation
                : component.percent_contribution;
        }
    }
    return std::nullopt;
}

std::optional<double> nested_component_percent(
    const std::vector<NestedGageVarianceComponent>& components,
    const std::string& source,
    bool study_variation)
{
    for (const auto& component : components) {
        if (component.source == source) {
            return study_variation
                ? component.percent_study_variation
                : component.percent_contribution;
        }
    }
    return std::nullopt;
}

void append_ndc_rule(MsaFacts& facts)
{
    if (!facts.ndc_available) {
        facts.rules.push_back(make_rule_evidence(
            "ndc_not_computed",
            "not_computed",
            "ndc 不可估计；不能据此评价测量系统分辨力。",
            {},
            "先检查零总变异、零 Gage 标准差或无效容差后再解释 ndc。"));
        return;
    }
    if (facts.ndc.has_value() && *facts.ndc < 5.0) {
        facts.rules.push_back(make_rule_evidence(
            "ndc_investigation",
            "triggered",
            "ndc < 5，提示测量系统对零件间差异的分辨力需要调查。",
            {},
            "ndc 小于 5 只是调查提示，不是量具不合格的绝对结论。"));
    } else {
        facts.rules.push_back(make_rule_evidence(
            "ndc_investigation",
            "not_triggered",
            "ndc ≥ 5；这只说明当前研究中零件间变异相对 Gage 变异较大。",
            {},
            "仍需结合 %Study Var、%Tolerance 和现场公差风险解释。"));
    }
}

}  // namespace

AssumptionCheck make_assumption_check(
    const std::string& name,
    const std::string& status,
    std::optional<double> statistic,
    std::optional<double> p_value,
    const std::string& evidence_summary)
{
    return {name, status, statistic, p_value, evidence_summary};
}

RuleEvidence make_rule_evidence(
    const std::string& id,
    const std::string& status,
    const std::string& message,
    const std::vector<RowId>& related_rows,
    const std::string& suggested_action)
{
    return {id, status, message, related_rows, suggested_action};
}

std::string combine_assumption_status(const std::vector<AssumptionCheck>& checks)
{
    if (checks.empty()) {
        return "not_verified";
    }
    bool any_against = false;
    bool any_not_verified = false;
    bool any_computed = false;
    for (const auto& check : checks) {
        if (check.status == "evidence_against") {
            any_against = true;
        } else if (check.status == "no_evidence_against") {
            any_computed = true;
        } else if (check.status != "not_applicable") {
            any_not_verified = true;
        }
    }
    if (any_against) {
        return "evidence_against";
    }
    if (any_not_verified || !any_computed) {
        return "not_verified";
    }
    return "no_evidence_against";
}

std::vector<AnalysisRuleSpec> regression_rule_catalog()
{
    return {
        {"error_df", "误差自由度", "N-p-1 必须为正才能输出 t、F 与 P。"},
        {"rank_deficiency", "秩亏/共线", "设计矩阵秩亏时拒绝拟合，不输出伪造推断。"},
        {"residual_normality", "残差正态性", "Anderson-Darling 只能拒绝或未拒绝正态假设。"},
        {"residual_independence", "残差独立性",
         "Durbin-Watson 对照 α=0.05 近似 dL/dU 判定区；不能写成已证明无自相关。"},
        {"homoscedasticity", "方差齐性", "残差对拟合值图是主要证据，不单独宣称已验证。"},
        {"leverage", "高杠杆", "杠杆值 > 2p/n 时标记为需要调查的高杠杆点。"},
        {"outlier", "异常残差", "|删除学生化残差| > 3 时标记为异常点调查。"},
        {"influence", "影响点", "Cook's D > 4/n 或 |DFITS| 超过阈值时标记影响点。"},
        {"collinearity", "共线性", "VIF > 5 提示共线性调查，不自动删除变量。"},
    };
}

std::vector<AnalysisRuleSpec> anova_rule_catalog()
{
    return {
        {"estimability", "可估计性", "秩亏或无误差自由度时不输出伪造 F/P。"},
        {"residual_normality", "残差正态性", "用残差 Anderson-Darling 作为假设证据。"},
        {"homogeneity", "方差齐性", "Levene/组内标准差比较只作为调查证据。"},
        {"family_error_rate", "家族错误率", "Tukey 必须回显同时置信水平和调整后 p 值。"},
        {"unbalanced_design", "不平衡设计", "不平衡时 Sequential SS 与 Adjusted SS 可能不同。"},
    };
}

std::vector<AnalysisRuleSpec> msa_rule_catalog()
{
    return {
        {"design_balance", "设计平衡", "零件×操作员单元重复次数必须一致。"},
        {"negative_variance", "负方差分量", "保留截断前后方差分量，截断后用于 %Contribution。"},
        {"interaction_model", "交互项模型", "报告 Part×Operator 显著性，不自动缩减模型。"},
        {"ndc_investigation", "ndc 调查", "ndc = truncate(1.41×PartStDev/GageStDev)，<5 只作调查提示。"},
        {"percent_metrics", "百分比口径", "%Contribution 与 %Study Var 不可混用。"},
        {"invalid_tolerance", "无效容差", "无有效公差时不计算 %Tolerance。"},
        {"zero_repeatability", "零重复性", "Type 1 零重复性不输出伪造 p=0 推断。"},
    };
}

std::vector<AnalysisRuleSpec> reliability_rule_catalog()
{
    return {
        {"event_encoding", "事件编码", "只接受明确的失效/删失语义，拒绝未知编码。"},
        {"time_validity", "时间合法性", "寿命必须为有限正数。"},
        {"risk_set", "风险集", "同一时刻多个失效应合并，并报告 at-risk/failures/censored。"},
        {"identifiability", "可识别性", "全删失或最大观测删失时尾部指标可能不可估计。"},
        {"convergence", "参数收敛", "Weibull 必须回显收敛、边界命中和估计方法。"},
    };
}

RegressionFacts regression_facts_from(const RegressionResult& result)
{
    RegressionFacts facts;
    facts.r_squared = result.r_squared;
    facts.residual_normality_p =
        result.diagnostics_summary.residual_normality.has_value()
            ? result.diagnostics_summary.residual_normality->p_value
            : std::nullopt;
    if (result.diagnostics_summary.residual_normality.has_value()) {
        facts.residual_anderson_darling =
            result.diagnostics_summary.residual_normality->anderson_darling;
    }
    facts.influential_count = result.diagnostics_summary.influential_count;
    facts.outlier_count = result.diagnostics_summary.outlier_count;
    facts.high_leverage_count = result.diagnostics_summary.high_leverage_count;
    facts.durbin_watson = result.durbin_watson;
    facts.durbin_watson_dl = result.diagnostics_summary.durbin_watson_dl;
    facts.durbin_watson_du = result.diagnostics_summary.durbin_watson_du;
    facts.durbin_watson_decision = result.diagnostics_summary.durbin_watson_decision;
    facts.error_degrees_of_freedom = result.evidence.degrees_of_freedom;
    facts.rank_deficient = has_diagnostic_code(
        result.diagnostics, "rank_deficient_design");
    facts.assumptions = result.diagnostics_summary.assumptions;
    facts.rules = result.diagnostics_summary.rules;
    for (const auto& coefficient : result.coefficients) {
        if (coefficient.vif.has_value()) {
            facts.max_vif = facts.max_vif.has_value()
                ? std::max(*facts.max_vif, *coefficient.vif)
                : coefficient.vif;
        }
    }
    facts.assumption_status = result.evidence.assumption_status.empty()
        ? combine_assumption_status(facts.assumptions)
        : result.evidence.assumption_status;
    return facts;
}

AnovaFacts one_way_anova_facts_from(
    const AnovaResult& result,
    const TukeyResult& tukey,
    const bool tukey_grouping_available,
    const std::size_t grouping_letter_count)
{
    AnovaFacts facts;
    facts.p_value = result.p_value;
    facts.error_degrees_of_freedom = result.error_degrees_of_freedom;
    facts.estimable = result.p_value.has_value();
    facts.assumption_status = result.evidence.assumption_status;
    facts.assumptions = result.assumptions;
    facts.rules = result.rules;
    if (result.p_value.has_value() && *result.p_value < 0.05) {
        facts.significant_terms.push_back("组间");
    }
    facts.family_confidence_level = tukey.family_confidence_level;
    facts.tukey_method = tukey.method;
    if (!tukey.comparisons.empty()) {
        facts.tukey_interval_columns = "lower_upper";
    }
    for (const auto& comparison : tukey.comparisons) {
        if (comparison.significant) {
            ++facts.tukey_significant_pairs;
        }
    }
    facts.tukey_grouping_available = tukey_grouping_available;
    facts.grouping_letter_count = grouping_letter_count;
    facts.rules.insert(facts.rules.end(), tukey.rules.cbegin(), tukey.rules.cend());
    return facts;
}

AnovaFacts two_factor_anova_facts_from(const TwoFactorAnovaResult& result)
{
    AnovaFacts facts;
    facts.error_degrees_of_freedom = result.error_degrees_of_freedom;
    facts.assumption_status = result.evidence.assumption_status;
    facts.assumptions = result.assumptions;
    facts.rules = result.rules;
    facts.estimable = true;
    for (const auto& effect : result.effects) {
        if (!effect.estimable || !effect.p_value.has_value()) {
            facts.estimable = facts.estimable && effect.estimable;
            if (!effect.estimable) {
                ++facts.not_estimable_term_count;
            }
        } else if (*effect.p_value < 0.05) {
            facts.significant_terms.push_back(effect.term);
            if (!facts.p_value.has_value() || *effect.p_value < *facts.p_value) {
                facts.p_value = effect.p_value;
            }
        }
    }
    return facts;
}

MsaFacts gage_rr_facts_from(const GageRrResult& result)
{
    MsaFacts facts;
    facts.ndc = result.ndc_available
        ? std::optional<double>(result.ndc) : std::nullopt;
    facts.ndc_available = result.ndc_available;
    facts.design_balanced = result.design_balanced;
    facts.interaction_retained = result.interaction_retained;
    facts.interaction_p_value = result.interaction_p_value;
    facts.interaction_reduction_recommended = result.interaction_reduction_recommended;
    facts.negative_variance_truncated = result.negative_variance_truncated;
    facts.gage_percent_study_variation = component_percent(
        result.variance_components, "Total Gage R&R", true);
    facts.gage_percent_contribution = component_percent(
        result.variance_components, "Total Gage R&R", false);
    facts.tolerance_percent = std::nullopt;
    for (const auto& component : result.variance_components) {
        if (component.source == "Total Gage R&R" && component.percent_tolerance_available) {
            facts.tolerance_percent = component.percent_tolerance;
        }
    }
    facts.rules = result.rules;
    facts.assumption_status = result.evidence.assumption_status;
    append_ndc_rule(facts);
    return facts;
}

MsaFacts nested_gage_facts_from(const NestedGageRrResult& result)
{
    MsaFacts facts;
    facts.ndc = result.ndc_available
        ? std::optional<double>(result.ndc) : std::nullopt;
    facts.ndc_available = result.ndc_available;
    facts.design_balanced = result.design_balanced;
    facts.interaction_retained = false;
    facts.interaction_plot_available = false;
    facts.negative_variance_truncated = result.negative_variance_truncated;
    facts.gage_percent_study_variation = nested_component_percent(
        result.variance_components, "Total Gage R&R", true);
    facts.gage_percent_contribution = nested_component_percent(
        result.variance_components, "Total Gage R&R", false);
    for (const auto& component : result.variance_components) {
        if (component.source == "Total Gage R&R" && component.percent_tolerance_available) {
            facts.tolerance_percent = component.percent_tolerance;
        }
    }
    facts.rules = result.rules;
    facts.assumption_status = result.evidence.assumption_status;
    append_ndc_rule(facts);
    return facts;
}

MsaFacts type1_facts_from(const MsaType1Result& result)
{
    MsaFacts facts;
    facts.p_value = result.inference_available
        ? std::optional<double>(result.p_value) : std::nullopt;
    facts.cgk = result.cgk;
    facts.tolerance_percent = result.percent_tolerance;
    facts.rules = result.rules;
    facts.assumption_status = result.inference_available
        ? "not_verified" : "not_computed";
    return facts;
}

MsaFacts bias_linearity_facts_from(const BiasLinearityResult& result)
{
    MsaFacts facts;
    facts.slope = result.slope;
    facts.bias_low = result.bias_at_low;
    facts.bias_high = result.bias_at_high;
    facts.linearity = result.linearity;
    facts.percent_linearity = result.percent_linearity;
    facts.slope_p_value = result.slope_p_value;
    facts.intercept_p_value = result.intercept_p_value;
    facts.residual_s = result.residual_s;
    facts.average_bias = result.average_bias;
    facts.average_bias_p = result.average_bias_p;
    facts.process_variation_used = result.process_variation_used;
    facts.p_value = result.slope_p_value;
    facts.rules = result.rules;
    facts.assumption_status = "not_verified";
    return facts;
}

MsaFacts stability_facts_from(const StabilityResult& result)
{
    MsaFacts facts;
    facts.p_value = static_cast<double>(result.out_of_control.size());
    facts.rules = result.rules;
    facts.assumption_status = "not_verified";
    return facts;
}

MsaFacts attribute_agreement_facts_from(const AttributeAgreementResult& result)
{
    MsaFacts facts;
    facts.ratings_are_ordinal = result.ratings_are_ordinal;
    facts.p_value = result.overall_available && result.overall.identifiable
        ? std::optional<double>(result.overall.kappa) : std::nullopt;
    facts.weighted_kappa_available = false;
    for (const auto& row : result.between_evaluator) {
        if (row.estimate.method == "cohen_linear"
            || row.estimate.method == "cohen_quadratic") {
            facts.weighted_kappa_available = true;
            facts.kappa_weight_scheme = row.estimate.method == "cohen_quadratic"
                ? "quadratic" : "linear";
            break;
        }
    }
    if (!facts.weighted_kappa_available) {
        for (const auto& row : result.against_standard) {
            if (row.estimate.method == "cohen_linear"
                || row.estimate.method == "cohen_quadratic") {
                facts.weighted_kappa_available = true;
                facts.kappa_weight_scheme = row.estimate.method == "cohen_quadratic"
                    ? "quadratic" : "linear";
                break;
            }
        }
    }
    if (result.between_kendall.has_value() && result.between_kendall->identifiable) {
        facts.kendall_available = true;
        facts.kendall_w = result.between_kendall->coefficient;
        facts.kendall_w_p = result.between_kendall->p_value;
    }
    if (result.overall_kendall.has_value() && result.overall_kendall->identifiable) {
        facts.kendall_available = true;
        facts.kendall_tau = result.overall_kendall->tau;
        facts.kendall_tau_p = result.overall_kendall->p_value;
    } else if (!result.against_standard_kendall.empty()
               && result.against_standard_kendall.front().estimate.identifiable) {
        facts.kendall_available = true;
        facts.kendall_tau = result.against_standard_kendall.front().estimate.tau;
        facts.kendall_tau_p = result.against_standard_kendall.front().estimate.p_value;
    }
    facts.rules.push_back(make_rule_evidence(
        "kappa_interpretation",
        "not_triggered",
        "拒绝 Kappa=0 不等于已证明评估者一致。",
        {},
        "Kappa 只描述超出偶然的绝对一致率，不能写成测量系统合格。"));
    if (facts.weighted_kappa_available) {
        facts.rules.push_back(make_rule_evidence(
            "weighted_kappa_not_minitab_aaa",
            "not_triggered",
            "Weighted Kappa 是 DataLab 可选 Cohen 加权，不是 Minitab AAA 默认输出。",
            {},
            "Minitab 有序评级路径使用 Kendall；不要把加权 κ 写成 Minitab AAA 结果。"));
    }
    if (result.ratings_are_ordinal) {
        facts.rules.push_back(make_rule_evidence(
            "kendall_interpretation",
            facts.kendall_available ? "not_triggered" : "triggered",
            facts.kendall_available
                ? "有序评级已计算 Kendall W/τ；拒绝系数为 0 不等于已证明有序一致。"
                : "已请求有序评级，但 Kendall 不可识别或等级不足。",
            {},
            "不要把 Kendall 写成加权 Kappa，也不要把未拒绝原假设写成已证明一致。"));
    }
    facts.assumption_status = "not_verified";
    return facts;
}

ReliabilityFacts kaplan_meier_facts_from(const KaplanMeierResult& result)
{
    ReliabilityFacts facts;
    facts.censored_count = result.censored_count;
    facts.failure_count = result.failure_count;
    facts.valid_count = result.valid_count;
    facts.median_life = result.median_life;
    facts.identifiable = result.survival_identifiable;
    facts.not_computed_reason = result.not_computed_reason;
    facts.rules = result.rules;
    return facts;
}

ReliabilityFacts weibull_facts_from(const WeibullResult& result)
{
    ReliabilityFacts facts;
    facts.shape = result.identifiable
        ? std::optional<double>(result.shape) : std::nullopt;
    facts.scale = result.identifiable
        ? std::optional<double>(result.scale) : std::nullopt;
    facts.threshold = result.identifiable ? result.threshold : std::nullopt;
    facts.distribution = result.threshold.has_value() ? "weibull3" : "weibull";
    facts.censored_count = result.observations >= result.failures
        ? result.observations - result.failures : 0;
    facts.failure_count = result.failures;
    facts.valid_count = result.observations;
    facts.median_life = result.median_life;
    facts.identifiable = result.identifiable;
    facts.converged = result.converged;
    facts.not_computed_reason = result.not_computed_reason;
    facts.rules = result.rules;
    return facts;
}

ReliabilityFacts exponential_facts_from(const ExponentialResult& result)
{
    ReliabilityFacts facts;
    facts.shape = result.identifiable ? std::optional<double>(1.0) : std::nullopt;
    facts.scale = result.identifiable
        ? std::optional<double>(1.0 / result.rate) : std::nullopt;
    facts.threshold = result.identifiable ? result.threshold : std::nullopt;
    facts.distribution = result.threshold.has_value() ? "exponential2" : "exponential";
    facts.censored_count = result.observations >= result.failures
        ? result.observations - result.failures : 0;
    facts.failure_count = result.failures;
    facts.valid_count = result.observations;
    facts.median_life = result.b50;
    facts.identifiable = result.identifiable;
    facts.converged = result.converged || result.identifiable;
    facts.not_computed_reason = result.not_computed_reason;
    facts.rules = result.rules;
    return facts;
}

ReliabilityFacts lognormal_facts_from(const LognormalResult& result)
{
    ReliabilityFacts facts;
    facts.location = result.identifiable
        ? std::optional<double>(result.location) : std::nullopt;
    facts.scale = result.identifiable
        ? std::optional<double>(result.scale) : std::nullopt;
    facts.threshold = result.identifiable ? result.threshold : std::nullopt;
    facts.distribution = result.threshold.has_value() ? "lognormal3" : "lognormal";
    facts.censored_count = result.observations >= result.failures
        ? result.observations - result.failures : 0;
    facts.failure_count = result.failures;
    facts.valid_count = result.observations;
    facts.median_life = result.median_life;
    facts.identifiable = result.identifiable;
    facts.converged = result.converged;
    facts.not_computed_reason = result.not_computed_reason;
    facts.rules = result.rules;
    return facts;
}

}  // namespace datalab::domain::statistics
