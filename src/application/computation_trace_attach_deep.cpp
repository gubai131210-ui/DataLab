#include "application/computation_trace_attach_deep.h"

#include "application/computation_trace_helpers.h"

#include <string>
#include <utility>
#include <vector>

namespace datalab::application {
namespace {

using datalab::domain::ComputationStep;
using datalab::domain::ComputationTrace;
using datalab::domain::OutputPage;

using namespace datalab::application::trace_helpers;


// ---------- Shared finish ----------
bool finish_l3(OutputPage& page, ComputationTrace tr)
{
    if (tr.plain_formula.empty()) {
        return false;
    }
    attach_trace(page, std::move(tr));
    return true;
}

// ---------- SPC / control chart family ----------
bool attach_spc_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol,
    const char* sigma_step_desc,
    const char* sigma_step_before)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Control_chart";
    tr.result_symbol = result_symbol;

    std::string cl = require_value(
        table_value(page, "CL", "Center", "中心线", "X-bar", "X̄"),
        page.facts.spc.has_value() ? opt_fmt(page.facts.spc->estimated_sigma) : std::string{});
    std::string ucl = require_value(table_value(page, "UCL", "UCL (I)", "上控制限"));
    std::string lcl = require_value(table_value(page, "LCL", "LCL (I)", "下控制限"));
    std::string sigma = require_value(
        page.facts.spc.has_value() ? opt_fmt(page.facts.spc->sigma_within) : std::string{},
        page.facts.spc.has_value() ? opt_fmt(page.facts.spc->estimated_sigma) : std::string{},
        table_value(page, "StDev", "Sigma", "σ", "Std Dev"));
    std::string mr_bar = table_value(page, "MR-bar", "MR Bar", "R-bar", "R̄", "MR̄");

    tr.bindings.push_back(bind("CL", "中心线", cl, "input"));
    tr.bindings.push_back(bind("UCL", "上控制限", ucl, "result"));
    tr.bindings.push_back(bind("LCL", "下控制限", lcl, "intermediate"));
    tr.bindings.push_back(bind("σ", "过程 σ", sigma, "input"));
    if (!mr_bar.empty()) {
        tr.bindings.push_back(bind("MR̄", "平均移动极差", mr_bar, "intermediate"));
    }

    push_step(tr.steps, make_step(1, "估计中心线", "CL = x̄", "CL = " + cl, cl));
    if (!mr_bar.empty() && !sigma.empty()) {
        push_step(tr.steps, make_step(
            2, sigma_step_desc, sigma_step_before,
            "σ = " + sigma + " (MR̄=" + mr_bar + ")", sigma));
    } else if (!sigma.empty()) {
        push_step(tr.steps, make_step(2, "估计 σ", "σ = MR̄/d₂", "σ = " + sigma, sigma));
    }
    push_step(tr.steps, make_step(
        3, "构造控制限", "UCL = CL + 3σ", "UCL = " + ucl + ", LCL = " + lcl, ucl));

    tr.result_value = require_value(ucl, cl);
    tr.substituted_text = std::string(plain_formula) + " → UCL=" + ucl
        + ", CL=" + cl + ", LCL=" + lcl;
    return finish_l3(page, std::move(tr));
}

// ---------- t-test family ----------
bool attach_t_test_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Student%27s_t-test";
    tr.result_symbol = result_symbol;

    std::string n = require_value(
        page.facts.t_test.has_value() ? std::to_string(page.facts.t_test->n) : std::string{},
        table_value(page, "N", "n"));
    std::string xbar = require_value(
        page.facts.t_test.has_value() ? opt_fmt(page.facts.t_test->mean) : std::string{},
        table_value(page, "Mean", "均值", "Sample Mean"));
    std::string s = require_value(
        page.facts.t_test.has_value()
            ? opt_fmt(page.facts.t_test->sample_standard_deviation) : std::string{},
        table_value(page, "StDev", "标准差", "s", "SE Mean"));
    std::string stat = require_value(
        page.facts.t_test.has_value() ? opt_fmt(page.facts.t_test->z_statistic) : std::string{},
        table_value(page, "T", "t", "Z", "Statistic", "统计量"));
    std::string p = require_value(
        page.facts.t_test.has_value() ? opt_fmt(page.facts.t_test->p_value) : std::string{},
        table_value(page, "P", "P-Value", "P 值"));

    tr.bindings.push_back(bind("n", "样本量", n, "input"));
    tr.bindings.push_back(bind("x̄", "样本均值", xbar, "input"));
    tr.bindings.push_back(bind("s", "标准差", s, "input"));
    tr.bindings.push_back(bind(result_symbol, "检验统计量", stat, "intermediate"));
    tr.bindings.push_back(bind("P", "P 值", p, "result"));

    push_step(tr.steps, make_step(1, "样本均值", "x̄ = Σx/n", "x̄ = " + xbar, xbar));
    push_step(tr.steps, make_step(
        2, "标准误", "SE = s/√n", "SE = " + s + "/√" + n, s));
    push_step(tr.steps, make_step(
        3, "检验统计量", plain_formula, std::string(result_symbol) + " = " + stat, stat));

    tr.result_value = require_value(stat, p);
    tr.substituted_text = std::string(result_symbol) + " = (" + xbar + ") / (" + s
        + " / √" + n + ") = " + stat;
    return finish_l3(page, std::move(tr));
}

// ---------- proportion family ----------
bool attach_proportion_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Binomial_test";
    tr.result_symbol = result_symbol;

    std::string events = require_value(
        page.facts.proportion.has_value()
            ? std::to_string(page.facts.proportion->events) : std::string{},
        table_value(page, "Events", "Events (x)", "x"));
    std::string trials = require_value(
        page.facts.proportion.has_value()
            ? std::to_string(page.facts.proportion->trials) : std::string{},
        table_value(page, "Trials", "N", "n"));
    std::string phat = require_value(
        page.facts.proportion.has_value()
            ? opt_fmt(page.facts.proportion->proportion) : std::string{},
        table_value(page, "Sample p", "p̂", "Proportion"));
    std::string stat = require_value(
        table_value(page, "Z", "Statistic", "统计量"));
    std::string p = require_value(
        page.facts.proportion.has_value()
            ? opt_fmt(page.facts.proportion->p_value) : std::string{},
        table_value(page, "P", "P-Value", "P 值"));

    tr.bindings.push_back(bind("x", "事件数", events, "input"));
    tr.bindings.push_back(bind("n", "试验数", trials, "input"));
    tr.bindings.push_back(bind("p̂", "样本比例", phat, "intermediate"));
    tr.bindings.push_back(bind("Z", "Z 统计量", stat, "intermediate"));
    tr.bindings.push_back(bind("P", "P 值", p, "result"));

    push_step(tr.steps, make_step(
        1, "样本比例", "p̂ = x/n", "p̂ = " + events + "/" + trials + " = " + phat, phat));
    push_step(tr.steps, make_step(
        2, "标准误", "SE = √[p̂(1−p̂)/n]", "SE from p̂=" + phat, phat));
    push_step(tr.steps, make_step(
        3, "Z 统计量", plain_formula, "Z = " + stat, stat));

    tr.result_value = require_value(stat, p);
    tr.substituted_text = "p̂ = " + events + "/" + trials + " = " + phat + " → Z = " + stat;
    return finish_l3(page, std::move(tr));
}

// ---------- regression family ----------
bool attach_regression_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Linear_regression";
    tr.result_symbol = result_symbol;

    std::string r2 = require_value(
        page.facts.regression.has_value()
            ? opt_fmt(page.facts.regression->r_squared) : std::string{},
        table_value(page, "R-Sq", "R²", "R-Square"));
    std::string coef = require_value(
        table_value(page, "Constant", "Intercept", "截距", "Coef", "Coefficient"));
    std::string se = require_value(table_value(page, "SE Coef", "SE", "标准误"));
    std::string t = require_value(table_value(page, "T", "t", "T-Value"));

    tr.bindings.push_back(bind("R²", "决定系数", r2, "result"));
    tr.bindings.push_back(bind("β₀", "截距/系数", coef, "input"));
    tr.bindings.push_back(bind("SE", "标准误", se, "intermediate"));
    tr.bindings.push_back(bind("t", "t 值", t, "intermediate"));

    push_step(tr.steps, make_step(
        1, "拟合模型", plain_formula, "R² = " + r2, r2));
    push_step(tr.steps, make_step(
        2, "系数估计", "β̂ = (X'X)⁻¹X'Y", "β₀ = " + coef + " (SE=" + se + ")", coef));
    push_step(tr.steps, make_step(
        3, "系数检验", "t = β/SE", "t = " + t, t));

    tr.result_value = require_value(r2, coef);
    tr.substituted_text = std::string(plain_formula) + " → R² = " + r2 + ", β₀ = " + coef;
    return finish_l3(page, std::move(tr));
}

// ---------- MSA / gage R&R family ----------
bool attach_msa_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Measurement_system_analysis";
    tr.result_symbol = result_symbol;

    std::string grr = require_value(
        page.facts.msa.has_value()
            ? opt_fmt(page.facts.msa->gage_percent_study_variation) : std::string{},
        table_value(page, "%Study Var", "%GRR", "Gage R&R", "Study Var"));
    std::string ndc = require_value(
        page.facts.msa.has_value() ? opt_fmt(page.facts.msa->ndc) : std::string{},
        table_value(page, "ndc", "NDC", "Distinct Categories"));
    std::string repeat = require_value(
        page.facts.msa.has_value()
            ? opt_fmt(page.facts.msa->gage_percent_contribution) : std::string{},
        table_value(page, "%Contribution", "Repeatability"));

    tr.bindings.push_back(bind("%GRR", "Gage R&R %", grr, "result"));
    tr.bindings.push_back(bind("ndc", "可区分类别数", ndc, "intermediate"));
    tr.bindings.push_back(bind("%Repeat", "重复性 %", repeat, "input"));

    push_step(tr.steps, make_step(
        1, "方差分量", "ANOVA → σ²_part, σ²_gage", "%GRR = " + grr, grr));
    push_step(tr.steps, make_step(
        2, "ndc 计算", "ndc = floor(1.41·σ_part/σ_gage)", "ndc = " + ndc, ndc));
    push_step(tr.steps, make_step(
        3, "研究变异", "%Study Var = 6σ_gage/TV", "%GRR = " + grr, grr));

    tr.result_value = require_value(grr, ndc);
    tr.substituted_text = std::string(plain_formula) + " → %GRR = " + grr + ", ndc = " + ndc;
    return finish_l3(page, std::move(tr));
}

// ---------- generic hypothesis test (chi-square, anova, nonparametric) ----------
bool attach_generic_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
    tr.result_symbol = result_symbol;

    std::string n = require_value(
        page.facts.chi_square.has_value()
            ? std::to_string(page.facts.chi_square->total_count) : std::string{},
        page.facts.nonparametric.has_value()
            ? std::to_string(page.facts.nonparametric->group_count) : std::string{},
        table_value(page, "N", "n", "Total", "有效 N"));
    std::string stat = require_value(
        page.facts.chi_square.has_value()
            ? opt_fmt(page.facts.chi_square->statistic) : std::string{},
        page.facts.nonparametric.has_value()
            ? opt_fmt(page.facts.nonparametric->statistic) : std::string{},
        page.facts.variance.has_value()
            ? opt_fmt(page.facts.variance->statistic) : std::string{},
        table_value(page, "Statistic", "Chi-Sq", "F", "H", "U", "Z", "统计量", "T"));
    std::string p = require_value(
        page.facts.chi_square.has_value()
            ? opt_fmt(page.facts.chi_square->p_value)
            : page.facts.anova.has_value()
                ? opt_fmt(page.facts.anova->p_value)
                : page.facts.nonparametric.has_value()
                    ? opt_fmt(page.facts.nonparametric->p_value)
                    : page.facts.variance.has_value()
                        ? opt_fmt(page.facts.variance->p_value)
                        : std::string{},
        table_value(page, "P", "P-Value", "P 值"));

    tr.bindings.push_back(bind("N", "样本量/规模", n, "input"));
    tr.bindings.push_back(bind(result_symbol, "检验统计量", stat, "intermediate"));
    tr.bindings.push_back(bind("P", "P 值", p, "result"));

    push_step(tr.steps, make_step(1, "汇总数据", "N = " + n, "N = " + n, n));
    push_step(tr.steps, make_step(
        2, "计算统计量", plain_formula, std::string(result_symbol) + " = " + stat, stat));
    push_step(tr.steps, make_step(
        3, "显著性", "P = P(" + std::string(result_symbol) + " ≥ obs)", "P = " + p, p));

    tr.result_value = require_value(stat, p);
    tr.substituted_text = std::string(plain_formula) + " → " + result_symbol + " = " + stat + ", P = " + p;
    return finish_l3(page, std::move(tr));
}

// ---------- capability variant ----------
bool attach_capability_l3(
    OutputPage& page,
    const char* command_id,
    const char* formula_id,
    const char* title,
    const char* plain_formula,
    const char* result_symbol)
{
    ComputationTrace tr;
    tr.command_id = command_id;
    tr.formula_id = formula_id;
    tr.title = title;
    tr.plain_formula = plain_formula;
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Process_capability_index";
    tr.result_symbol = result_symbol;

    const auto& cfg = page.configuration.specifications;
    std::string usl = require_value(
        cfg.upper.has_value() ? fmt_num(*cfg.upper) : std::string{},
        table_value(page, "USL"));
    std::string lsl = require_value(
        cfg.lower.has_value() ? fmt_num(*cfg.lower) : std::string{},
        table_value(page, "LSL"));
    std::string cpk = require_value(
        page.facts.capability.has_value()
            ? opt_fmt(page.facts.capability->cpk) : std::string{},
        page.facts.nonparametric_capability.has_value()
            ? opt_fmt(page.facts.nonparametric_capability->cnpk) : std::string{},
        table_value(page, "Cpk", "Ppk", "Cp"));
    std::string sigma = require_value(
        page.facts.tolerance.has_value()
            ? opt_fmt(page.facts.tolerance->standard_deviation) : std::string{},
        table_value(page, "StDev", "Within", "σ"));

    tr.bindings.push_back(bind("USL", "规格上限", usl, "input"));
    tr.bindings.push_back(bind("LSL", "规格下限", lsl, "input"));
    tr.bindings.push_back(bind("σ", "标准差", sigma, "input"));
    tr.bindings.push_back(bind(result_symbol, "能力指数", cpk, "result"));

    push_step(tr.steps, make_step(
        1, "规格宽度", "USL−LSL", "(" + usl + "−" + lsl + ")", usl));
    push_step(tr.steps, make_step(
        2, "过程 σ", "σ from within/between", "σ = " + sigma, sigma));
    push_step(tr.steps, make_step(
        3, "能力指数", plain_formula, std::string(result_symbol) + " = " + cpk, cpk));

    tr.result_value = cpk;
    tr.substituted_text = std::string(plain_formula) + " → " + result_symbol + " = " + cpk;
    return finish_l3(page, std::move(tr));
}


bool try_attach_normality_test(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "normality_test";
    tr.formula_id = "anderson_darling";
    tr.title = "正态性检验 Anderson-Darling";
    tr.plain_formula = "A² = −n − (1/n)Σ[(2i−1)(ln F(x_(i))+ln(1−F(x_(n+1−i))))]";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Normality_test";
    tr.result_symbol = "P";
    std::string n = require_value(
        page.facts.normality.has_value()
            ? std::to_string(page.facts.normality->n) : std::string{},
        table_value(page, "N", "n"));
    std::string ad = require_value(
        page.facts.normality.has_value()
            ? opt_fmt(page.facts.normality->anderson_darling) : std::string{},
        table_value(page, "AD", "A-Squared", "Anderson-Darling"));
    std::string p = require_value(
        page.facts.normality.has_value()
            ? opt_fmt(page.facts.normality->p_value) : std::string{},
        table_value(page, "P", "P-Value", "P 值"));
    tr.bindings.push_back(bind("N", "样本量", n, "input"));
    tr.bindings.push_back(bind("A²", "Anderson-Darling", ad, "intermediate"));
    tr.bindings.push_back(bind("P", "P 值", p, "result"));
    push_step(tr.steps, make_step(1, "排序标准化", "F(x_(i)) from sorted x", "N = " + n, n));
    push_step(tr.steps, make_step(2, "A² 统计量", "A² = −n − (1/n)Σ[(2i−1)(ln F(x_(i))+ln(1−F(x_(n+1−i))))]", "A² = " + ad, ad));
    push_step(tr.steps, make_step(3, "P 值", "Stephens 修正", "P = " + p, p));
    tr.result_value = require_value(p, ad);
    tr.substituted_text = "A² = −n − (1/n)Σ[(2i−1)(ln F(x_(i))+ln(1−F(x_(n+1−i))))] → A² = " + ad + ", P = " + p;
    return finish_l3(page, std::move(tr));
}

bool try_attach_outlier_test(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "outlier_test";
    tr.formula_id = "grubbs_g";
    tr.title = "Grubbs 离群值检验";
    tr.plain_formula = "G = max|x_i − x̄| / s";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Grubbs%27_test_for_outliers";
    tr.result_symbol = "G";
    std::string n = require_value(
        page.facts.outlier_test.has_value()
            ? std::to_string(page.facts.outlier_test->n) : std::string{},
        table_value(page, "N", "n"));
    std::string xbar = require_value(
        page.facts.outlier_test.has_value()
            ? opt_fmt(page.facts.outlier_test->mean) : std::string{},
        table_value(page, "Mean", "均值"));
    std::string s = require_value(
        page.facts.outlier_test.has_value()
            ? opt_fmt(page.facts.outlier_test->standard_deviation) : std::string{},
        table_value(page, "StDev", "StDev(Overall)"));
    std::string g = require_value(
        page.facts.outlier_test.has_value()
            ? opt_fmt(page.facts.outlier_test->g_statistic) : std::string{},
        table_value(page, "G", "Statistic", "统计量"));
    std::string p = require_value(
        page.facts.outlier_test.has_value()
            ? opt_fmt(page.facts.outlier_test->p_value) : std::string{},
        table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("n", "样本量", n, "input"));
    tr.bindings.push_back(bind("x̄", "均值", xbar, "input"));
    tr.bindings.push_back(bind("s", "标准差", s, "input"));
    tr.bindings.push_back(bind("G", "Grubbs G", g, "result"));
    tr.bindings.push_back(bind("P", "P 值", p, "intermediate"));
    push_step(tr.steps, make_step(1, "样本均值", "x̄ = Σx/n", "x̄ = " + xbar, xbar));
    push_step(tr.steps, make_step(2, "Grubbs G", "G = max|x_i − x̄| / s", "G = " + g, g));
    push_step(tr.steps, make_step(3, "P 值", "G critical table", "P = " + p, p));
    tr.result_value = require_value(g, p);
    tr.substituted_text = "G = max|x_i − " + xbar + "| / " + s + " = " + g;
    return finish_l3(page, std::move(tr));
}

bool try_attach_correlation(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "correlation";
    tr.formula_id = "pearson_r";
    tr.title = "Pearson 相关系数";
    tr.plain_formula = "r = Σ[(x−x̄)(y−ȳ)] / √[Σ(x−x̄)² Σ(y−ȳ)²]";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Pearson_correlation_coefficient";
    tr.result_symbol = "r";
    std::string n = require_value(
        page.facts.correlation.has_value()
            ? std::to_string(page.facts.correlation->n) : std::string{},
        table_value(page, "N", "n"));
    std::string r = require_value(table_value(page, "r", "R", "Correlation", "Pearson"));
    std::string p = require_value(table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("n", "样本量", n, "input"));
    tr.bindings.push_back(bind("r", "相关系数", r, "result"));
    tr.bindings.push_back(bind("P", "P 值", p, "intermediate"));
    push_step(tr.steps, make_step(1, "离差乘积和", "Σ(x−x̄)(y−ȳ)", "n = " + n, n));
    push_step(tr.steps, make_step(2, "Pearson r", "r = Σ[(x−x̄)(y−ȳ)] / √[Σ(x−x̄)² Σ(y−ȳ)²]", "r = " + r, r));
    push_step(tr.steps, make_step(3, "显著性", "t = r√[(n−2)/(1−r²)]", "P = " + p, p));
    tr.result_value = require_value(r, p);
    tr.substituted_text = "r = Σ[(x−x̄)(y−ȳ)] / √[Σ(x−x̄)² Σ(y−ȳ)²] → r = " + r;
    return finish_l3(page, std::move(tr));
}

bool try_attach_one_sample_z(OutputPage& page)
{
    return attach_t_test_l3(page, "one_sample_z", "one_sample_z", "单样本 Z 检验", "Z = (x̄ − μ₀) / (σ / √n)", "Z");
}

bool try_attach_one_proportion(OutputPage& page)
{
    return attach_proportion_l3(page, "one_proportion", "one_proportion", "单样本比例检验", "p̂ = x/n；Z = (p̂ − p₀) / √[p₀(1−p₀)/n]", "Z");
}

bool try_attach_one_poisson_rate(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "one_poisson_rate";
    tr.formula_id = "one_poisson_rate";
    tr.title = "单样本 Poisson 率";
    tr.plain_formula = "λ̂ = events/exposure；Z = (λ̂ − λ₀)/√(λ₀/exposure)";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Poisson_distribution";
    tr.result_symbol = "Z";
    std::string events = require_value(
        page.facts.poisson_rate.has_value()
            ? std::to_string(page.facts.poisson_rate->events) : std::string{},
        table_value(page, "Events", "Events (x)"));
    std::string exposure = require_value(
        page.facts.poisson_rate.has_value()
            ? fmt_num(page.facts.poisson_rate->exposure) : std::string{},
        table_value(page, "Exposure", "Length of exposure"));
    std::string rate = require_value(
        page.facts.poisson_rate.has_value()
            ? opt_fmt(page.facts.poisson_rate->rate) : std::string{},
        table_value(page, "Sample Rate", "Rate"));
    std::string z = require_value(
        page.facts.poisson_rate.has_value()
            ? opt_fmt(page.facts.poisson_rate->z_statistic) : std::string{},
        table_value(page, "Z", "Statistic"));
    tr.bindings.push_back(bind("events", "事件数", events, "input"));
    tr.bindings.push_back(bind("T", "暴露量", exposure, "input"));
    tr.bindings.push_back(bind("λ̂", "样本率", rate, "intermediate"));
    tr.bindings.push_back(bind("Z", "Z", z, "result"));
    push_step(tr.steps, make_step(1, "样本率", "λ̂ = events/T", "λ̂ = " + rate, rate));
    push_step(tr.steps, make_step(2, "标准误", "SE = √(λ/T)", "T = " + exposure, exposure));
    push_step(tr.steps, make_step(3, "Z 统计量", "λ̂ = events/exposure；Z = (λ̂ − λ₀)/√(λ₀/exposure)", "Z = " + z, z));
    tr.result_value = require_value(z, rate);
    tr.substituted_text = "λ̂ = " + events + "/" + exposure + " = " + rate;
    return finish_l3(page, std::move(tr));
}

bool try_attach_two_poisson_rate(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "two_poisson_rate";
    tr.formula_id = "two_poisson_rate";
    tr.title = "双样本 Poisson 率";
    tr.plain_formula = "Z = (λ̂₁ − λ̂₂) / √[λ̂(1/n₁ + 1/n₂)]";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Poisson_distribution";
    tr.result_symbol = "Z";
    std::string events = require_value(
        page.facts.poisson_rate.has_value()
            ? std::to_string(page.facts.poisson_rate->events) : std::string{},
        table_value(page, "Events", "Events (x)"));
    std::string exposure = require_value(
        page.facts.poisson_rate.has_value()
            ? fmt_num(page.facts.poisson_rate->exposure) : std::string{},
        table_value(page, "Exposure", "Length of exposure"));
    std::string rate = require_value(
        page.facts.poisson_rate.has_value()
            ? opt_fmt(page.facts.poisson_rate->rate) : std::string{},
        table_value(page, "Sample Rate", "Rate"));
    std::string z = require_value(
        page.facts.poisson_rate.has_value()
            ? opt_fmt(page.facts.poisson_rate->z_statistic) : std::string{},
        table_value(page, "Z", "Statistic"));
    tr.bindings.push_back(bind("events", "事件数", events, "input"));
    tr.bindings.push_back(bind("T", "暴露量", exposure, "input"));
    tr.bindings.push_back(bind("λ̂", "样本率", rate, "intermediate"));
    tr.bindings.push_back(bind("Z", "Z", z, "result"));
    push_step(tr.steps, make_step(1, "样本率", "λ̂ = events/T", "λ̂ = " + rate, rate));
    push_step(tr.steps, make_step(2, "标准误", "SE = √(λ/T)", "T = " + exposure, exposure));
    push_step(tr.steps, make_step(3, "Z 统计量", "Z = (λ̂₁ − λ̂₂) / √[λ̂(1/n₁ + 1/n₂)]", "Z = " + z, z));
    tr.result_value = require_value(z, rate);
    tr.substituted_text = "λ̂ = " + events + "/" + exposure + " = " + rate;
    return finish_l3(page, std::move(tr));
}

bool try_attach_two_sample_t(OutputPage& page)
{
    return attach_t_test_l3(page, "two_sample_t", "welch_t", "双样本 t 检验 (Welch)", "t = (x̄₁ − x̄₂) / SE；SE = √(s₁²/n₁ + s₂²/n₂)", "t");
}

bool try_attach_one_sample_equivalence(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "one_sample_equivalence";
    tr.formula_id = "tost_one_sample";
    tr.title = "单样本等价性 (TOST)";
    tr.plain_formula = "H₀: |μ − μ₀| ≥ δ；TOST 用两侧 1−α 置信区间";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/equivalence_test";
    tr.result_symbol = "P";
    std::string diff = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->difference) : std::string{},
        table_value(page, "Difference", "差值", "Mean"));
    std::string lo = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_lower) : std::string{},
        table_value(page, "Lower", "CI Lower", "下限"));
    std::string hi = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_upper) : std::string{},
        table_value(page, "Upper", "CI Upper", "上限"));
    std::string p = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->p_lower) : std::string{},
        table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("Δ", "点估计", diff, "input"));
    tr.bindings.push_back(bind("CI", "1−2α 置信区间", lo + " ~ " + hi, "intermediate"));
    tr.bindings.push_back(bind("P", "TOST P", p, "result"));
    push_step(tr.steps, make_step(1, "点估计", "Δ = estimate", "Δ = " + diff, diff));
    push_step(tr.steps, make_step(2, "TOST 区间", "H₀: |μ − μ₀| ≥ δ；TOST 用两侧 1−α 置信区间", lo + " ≤ Δ ≤ " + hi, hi));
    push_step(tr.steps, make_step(3, "等价判定", "P(Δ ≤ −δ) & P(Δ ≥ δ)", "P = " + p, p));
    tr.result_value = require_value(p, diff);
    tr.substituted_text = "H₀: |μ − μ₀| ≥ δ；TOST 用两侧 1−α 置信区间 → Δ = " + diff + ", CI=[" + lo + "," + hi + "]";
    return finish_l3(page, std::move(tr));
}

bool try_attach_two_sample_equivalence(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "two_sample_equivalence";
    tr.formula_id = "tost_two_sample";
    tr.title = "双样本等价性 (TOST)";
    tr.plain_formula = "H₀: |μ₁ − μ₂| ≥ δ；TOST 用差值 1−α 置信区间";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/equivalence_test";
    tr.result_symbol = "P";
    std::string diff = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->difference) : std::string{},
        table_value(page, "Difference", "差值", "Mean"));
    std::string lo = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_lower) : std::string{},
        table_value(page, "Lower", "CI Lower", "下限"));
    std::string hi = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_upper) : std::string{},
        table_value(page, "Upper", "CI Upper", "上限"));
    std::string p = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->p_lower) : std::string{},
        table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("Δ", "点估计", diff, "input"));
    tr.bindings.push_back(bind("CI", "1−2α 置信区间", lo + " ~ " + hi, "intermediate"));
    tr.bindings.push_back(bind("P", "TOST P", p, "result"));
    push_step(tr.steps, make_step(1, "点估计", "Δ = estimate", "Δ = " + diff, diff));
    push_step(tr.steps, make_step(2, "TOST 区间", "H₀: |μ₁ − μ₂| ≥ δ；TOST 用差值 1−α 置信区间", lo + " ≤ Δ ≤ " + hi, hi));
    push_step(tr.steps, make_step(3, "等价判定", "P(Δ ≤ −δ) & P(Δ ≥ δ)", "P = " + p, p));
    tr.result_value = require_value(p, diff);
    tr.substituted_text = "H₀: |μ₁ − μ₂| ≥ δ；TOST 用差值 1−α 置信区间 → Δ = " + diff + ", CI=[" + lo + "," + hi + "]";
    return finish_l3(page, std::move(tr));
}

bool try_attach_two_sample_equivalence_ratio(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "two_sample_equivalence_ratio";
    tr.formula_id = "tost_ratio";
    tr.title = "双样本等价性 (比率)";
    tr.plain_formula = "H₀: ratio 超出 [θ_L, θ_U]；TOST 对 ln(ratio)";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/equivalence_test";
    tr.result_symbol = "P";
    std::string diff = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->difference) : std::string{},
        table_value(page, "Difference", "差值", "Mean"));
    std::string lo = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_lower) : std::string{},
        table_value(page, "Lower", "CI Lower", "下限"));
    std::string hi = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_upper) : std::string{},
        table_value(page, "Upper", "CI Upper", "上限"));
    std::string p = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->p_lower) : std::string{},
        table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("Δ", "点估计", diff, "input"));
    tr.bindings.push_back(bind("CI", "1−2α 置信区间", lo + " ~ " + hi, "intermediate"));
    tr.bindings.push_back(bind("P", "TOST P", p, "result"));
    push_step(tr.steps, make_step(1, "点估计", "Δ = estimate", "Δ = " + diff, diff));
    push_step(tr.steps, make_step(2, "TOST 区间", "H₀: ratio 超出 [θ_L, θ_U]；TOST 对 ln(ratio)", lo + " ≤ Δ ≤ " + hi, hi));
    push_step(tr.steps, make_step(3, "等价判定", "P(Δ ≤ −δ) & P(Δ ≥ δ)", "P = " + p, p));
    tr.result_value = require_value(p, diff);
    tr.substituted_text = "H₀: ratio 超出 [θ_L, θ_U]；TOST 对 ln(ratio) → Δ = " + diff + ", CI=[" + lo + "," + hi + "]";
    return finish_l3(page, std::move(tr));
}

bool try_attach_paired_equivalence(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "paired_equivalence";
    tr.formula_id = "tost_paired";
    tr.title = "配对等价性 (TOST)";
    tr.plain_formula = "H₀: |d̄| ≥ δ；d_i = x_i − y_i";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/equivalence_test";
    tr.result_symbol = "P";
    std::string diff = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->difference) : std::string{},
        table_value(page, "Difference", "差值", "Mean"));
    std::string lo = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_lower) : std::string{},
        table_value(page, "Lower", "CI Lower", "下限"));
    std::string hi = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_upper) : std::string{},
        table_value(page, "Upper", "CI Upper", "上限"));
    std::string p = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->p_lower) : std::string{},
        table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("Δ", "点估计", diff, "input"));
    tr.bindings.push_back(bind("CI", "1−2α 置信区间", lo + " ~ " + hi, "intermediate"));
    tr.bindings.push_back(bind("P", "TOST P", p, "result"));
    push_step(tr.steps, make_step(1, "点估计", "Δ = estimate", "Δ = " + diff, diff));
    push_step(tr.steps, make_step(2, "TOST 区间", "H₀: |d̄| ≥ δ；d_i = x_i − y_i", lo + " ≤ Δ ≤ " + hi, hi));
    push_step(tr.steps, make_step(3, "等价判定", "P(Δ ≤ −δ) & P(Δ ≥ δ)", "P = " + p, p));
    tr.result_value = require_value(p, diff);
    tr.substituted_text = "H₀: |d̄| ≥ δ；d_i = x_i − y_i → Δ = " + diff + ", CI=[" + lo + "," + hi + "]";
    return finish_l3(page, std::move(tr));
}

bool try_attach_one_proportion_equivalence(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "one_proportion_equivalence";
    tr.formula_id = "tost_one_proportion";
    tr.title = "单比例等价性";
    tr.plain_formula = "H₀: |p̂ − p₀| ≥ δ";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/equivalence_test";
    tr.result_symbol = "P";
    std::string diff = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->difference) : std::string{},
        table_value(page, "Difference", "差值", "Mean"));
    std::string lo = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_lower) : std::string{},
        table_value(page, "Lower", "CI Lower", "下限"));
    std::string hi = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_upper) : std::string{},
        table_value(page, "Upper", "CI Upper", "上限"));
    std::string p = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->p_lower) : std::string{},
        table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("Δ", "点估计", diff, "input"));
    tr.bindings.push_back(bind("CI", "1−2α 置信区间", lo + " ~ " + hi, "intermediate"));
    tr.bindings.push_back(bind("P", "TOST P", p, "result"));
    push_step(tr.steps, make_step(1, "点估计", "Δ = estimate", "Δ = " + diff, diff));
    push_step(tr.steps, make_step(2, "TOST 区间", "H₀: |p̂ − p₀| ≥ δ", lo + " ≤ Δ ≤ " + hi, hi));
    push_step(tr.steps, make_step(3, "等价判定", "P(Δ ≤ −δ) & P(Δ ≥ δ)", "P = " + p, p));
    tr.result_value = require_value(p, diff);
    tr.substituted_text = "H₀: |p̂ − p₀| ≥ δ → Δ = " + diff + ", CI=[" + lo + "," + hi + "]";
    return finish_l3(page, std::move(tr));
}

bool try_attach_two_proportion_equivalence(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "two_proportion_equivalence";
    tr.formula_id = "tost_two_proportion";
    tr.title = "双比例等价性";
    tr.plain_formula = "H₀: |p̂₁ − p̂₂| ≥ δ";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/equivalence_test";
    tr.result_symbol = "P";
    std::string diff = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->difference) : std::string{},
        table_value(page, "Difference", "差值", "Mean"));
    std::string lo = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_lower) : std::string{},
        table_value(page, "Lower", "CI Lower", "下限"));
    std::string hi = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->ci_upper) : std::string{},
        table_value(page, "Upper", "CI Upper", "上限"));
    std::string p = require_value(
        page.facts.equivalence.has_value()
            ? opt_fmt(page.facts.equivalence->p_lower) : std::string{},
        table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("Δ", "点估计", diff, "input"));
    tr.bindings.push_back(bind("CI", "1−2α 置信区间", lo + " ~ " + hi, "intermediate"));
    tr.bindings.push_back(bind("P", "TOST P", p, "result"));
    push_step(tr.steps, make_step(1, "点估计", "Δ = estimate", "Δ = " + diff, diff));
    push_step(tr.steps, make_step(2, "TOST 区间", "H₀: |p̂₁ − p̂₂| ≥ δ", lo + " ≤ Δ ≤ " + hi, hi));
    push_step(tr.steps, make_step(3, "等价判定", "P(Δ ≤ −δ) & P(Δ ≥ δ)", "P = " + p, p));
    tr.result_value = require_value(p, diff);
    tr.substituted_text = "H₀: |p̂₁ − p̂₂| ≥ δ → Δ = " + diff + ", CI=[" + lo + "," + hi + "]";
    return finish_l3(page, std::move(tr));
}

bool try_attach_one_way_anova(OutputPage& page)
{
    return attach_generic_l3(page, "one_way_anova", "one_way_anova_f", "单因子 ANOVA", "F = MSB/MSW = [SSB/(k−1)] / [SSW/(N−k)]", "F");
}

bool try_attach_paired_t(OutputPage& page)
{
    return attach_t_test_l3(page, "paired_t", "paired_t", "配对 t 检验", "t = d̄ / (s_d / √n)；d_i = x_i − y_i", "t");
}

bool try_attach_regression(OutputPage& page)
{
    return attach_regression_l3(page, "regression", "ols_regression", "最小二乘线性回归", "Y = β₀ + Σ β_j X_j + ε；β̂ = (X'X)⁻¹X'Y", "R²");
}

bool try_attach_two_proportions(OutputPage& page)
{
    return attach_proportion_l3(page, "two_proportions", "two_proportions", "双样本比例检验", "Z = (p̂₁ − p̂₂) / √[p̂(1−p̂)(1/n₁ + 1/n₂)]", "Z");
}

bool try_attach_chi_square(OutputPage& page)
{
    return attach_generic_l3(page, "chi_square", "chi_square", "卡方独立性检验", "χ² = Σ (O − E)² / E", "χ²");
}

bool try_attach_cross_tabulation(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "cross_tabulation";
    tr.formula_id = "cross_tabulation";
    tr.title = "列联表比例";
    tr.plain_formula = "cell% = count / row(or col) total";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Contingency_table";
    tr.result_symbol = "N";
    std::string n = require_value(
        page.facts.cross_tab.has_value()
            ? std::to_string(page.facts.cross_tab->total_count) : std::string{},
        table_value(page, "Total", "N"));
    tr.bindings.push_back(bind("N", "合计", n, "result"));
    push_step(tr.steps, make_step(1, "行合计", "row sums", "N = " + n, n));
    push_step(tr.steps, make_step(2, "列合计", "col sums", n, n));
    push_step(tr.steps, make_step(3, "cell %", "cell% = count / row(or col) total", n, n));
    tr.result_value = n;
    tr.substituted_text = "cell% = count / row(or col) total → N = " + n;
    return finish_l3(page, std::move(tr));
}

bool try_attach_chi_square_gof(OutputPage& page)
{
    return attach_generic_l3(page, "chi_square_gof", "chi_square_gof", "卡方拟合优度", "χ² = Σ (O − E)² / E，E = p·N", "χ²");
}

bool try_attach_poisson_gof(OutputPage& page)
{
    return attach_generic_l3(page, "poisson_gof", "poisson_gof", "Poisson 拟合优度", "χ² = Σ (O − E)² / E，E = λ̂·exposure", "χ²");
}

bool try_attach_mann_whitney(OutputPage& page)
{
    return attach_generic_l3(page, "mann_whitney", "mann_whitney_u", "Mann-Whitney U 检验", "U = n₁n₂ + n₁(n₁+1)/2 − R₁", "U");
}

bool try_attach_wilcoxon_signed_rank(OutputPage& page)
{
    return attach_generic_l3(page, "wilcoxon_signed_rank", "wilcoxon_signed_rank", "Wilcoxon 符号秩检验", "W⁺ = Σ R_i (d_i > 0)", "W");
}

bool try_attach_sign_test(OutputPage& page)
{
    return attach_generic_l3(page, "sign_test", "sign_test", "符号检验", "S = #{d_i > 0} ~ Binomial(n, 0.5)", "S");
}

bool try_attach_runs_test(OutputPage& page)
{
    return attach_generic_l3(page, "runs_test", "runs_test", "游程检验", "Z = (R − μ_R) / σ_R", "Z");
}

bool try_attach_mcnemar(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "mcnemar";
    tr.formula_id = "mcnemar";
    tr.title = "McNemar 检验";
    tr.plain_formula = "χ² = (b − c)² / (b + c)";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/McNemar%27s_test";
    tr.result_symbol = "χ²";
    std::string b = require_value(
        page.facts.mcnemar.has_value()
            ? std::to_string(page.facts.mcnemar->b) : std::string{},
        table_value(page, "b", "B"));
    std::string c = require_value(
        page.facts.mcnemar.has_value()
            ? std::to_string(page.facts.mcnemar->c) : std::string{},
        table_value(page, "c", "C"));
    std::string chi = require_value(
        page.facts.mcnemar.has_value()
            ? opt_fmt(page.facts.mcnemar->chi_square) : std::string{},
        table_value(page, "Chi-Sq", "Statistic"));
    tr.bindings.push_back(bind("b", "discordant b", b, "input"));
    tr.bindings.push_back(bind("c", "discordant c", c, "input"));
    tr.bindings.push_back(bind("χ²", "McNemar χ²", chi, "result"));
    push_step(tr.steps, make_step(1, "discordant", "b, c counts", "b=" + b + ", c=" + c, b));
    push_step(tr.steps, make_step(2, "χ²", "χ² = (b − c)² / (b + c)", "χ² = " + chi, chi));
    push_step(tr.steps, make_step(3, "df=1", "χ²_1", chi, chi));
    tr.result_value = chi;
    tr.substituted_text = "χ² = (" + b + "−" + c + ")²/(" + b + "+" + c + ") = " + chi;
    return finish_l3(page, std::move(tr));
}

bool try_attach_fisher_exact(OutputPage& page)
{
    return attach_generic_l3(page, "fisher_exact", "fisher_exact", "Fisher 精确检验", "P = Σ hypergeom(a|n,m,k)", "P");
}

bool try_attach_cochran_q(OutputPage& page)
{
    return attach_generic_l3(page, "cochran_q", "cochran_q", "Cochran Q 检验", "Q = (k−1)(kΣC_j² − T²) / (kT − ΣR_i²)", "Q");
}

bool try_attach_mood_median(OutputPage& page)
{
    return attach_generic_l3(page, "mood_median", "mood_median", "Mood 中位数检验", "χ² = Σ (O − E)² / E (above/below grand median)", "χ²");
}

bool try_attach_kruskal_wallis(OutputPage& page)
{
    return attach_generic_l3(page, "kruskal_wallis", "kruskal_wallis", "Kruskal-Wallis 检验", "H = [12/(N(N+1))] Σ (R_j²/n_j) − 3(N+1)", "H");
}

bool try_attach_friedman(OutputPage& page)
{
    return attach_generic_l3(page, "friedman", "friedman", "Friedman 检验", "χ²_r = [12/(bk(k+1))] Σ R_j² − 3b(k+1)", "χ²");
}

bool try_attach_two_factor_anova(OutputPage& page)
{
    return attach_generic_l3(page, "two_factor_anova", "two_factor_anova", "双因子 ANOVA", "F = MS_effect / MS_error", "F");
}

bool try_attach_logistic_regression(OutputPage& page)
{
    return attach_regression_l3(page, "logistic_regression", "logistic_regression", "Logistic 回归", "logit(π) = ln(π/(1−π)) = x'β", "Deviance");
}

bool try_attach_variance_test(OutputPage& page)
{
    return attach_generic_l3(page, "variance_test", "levene_f", "方差齐性检验", "F = s_max² / s_min² (或 Levene W)", "F");
}

bool try_attach_adf_test(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "adf_test";
    tr.formula_id = "adf_test";
    tr.title = "ADF 单位根检验";
    tr.plain_formula = "Δy_t = α + γ y_{t−1} + Σ δ_i Δy_{t−i} + ε_t";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Augmented_Dickey%E2%80%93Fuller_test";
    tr.result_symbol = "t";
    std::string gamma = require_value(
        page.facts.adf.has_value()
            ? opt_fmt(page.facts.adf->tau) : std::string{},
        table_value(page, "Gamma", "t", "Statistic"));
    std::string p = require_value(table_value(page, "P", "P-Value", "MacKinnon P"));
    tr.bindings.push_back(bind("γ", "γ (unit root)", gamma, "result"));
    tr.bindings.push_back(bind("P", "P 值", p, "intermediate"));
    push_step(tr.steps, make_step(1, "差分回归", "Δy_t = α + γ y_{t−1} + Σ δ_i Δy_{t−i} + ε_t", "γ = " + gamma, gamma));
    push_step(tr.steps, make_step(2, "ADF t", "t on γ", gamma, gamma));
    push_step(tr.steps, make_step(3, "MacKinnon P", "critical values", "P = " + p, p));
    tr.result_value = require_value(gamma, p);
    tr.substituted_text = "Δy_t = α + γ y_{t−1} + Σ δ_i Δy_{t−i} + ε_t → γ = " + gamma;
    return finish_l3(page, std::move(tr));
}

bool try_attach_poisson_regression(OutputPage& page)
{
    return attach_regression_l3(page, "poisson_regression", "poisson_regression", "Poisson 回归", "ln(E[Y]) = x'β；Y ~ Poisson(μ)", "Deviance");
}

bool try_attach_probit_reliability(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "probit_reliability";
    tr.formula_id = "probit_reliability";
    tr.title = "Probit 可靠性";
    tr.plain_formula = "logit(p) = β₀ + β₁·stress；LD50 = −β₀/β₁";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Survival_analysis";
    tr.result_symbol = "LD50";
    std::string n = require_value(
        page.facts.reliability.has_value() && page.facts.reliability->valid_count.has_value()
            ? std::to_string(*page.facts.reliability->valid_count) : std::string{},
        page.facts.warranty.has_value()
            ? std::to_string(page.facts.warranty->valid_count) : std::string{},
        table_value(page, "N", "Valid"));
    std::string param = require_value(
        page.facts.reliability.has_value()
            ? opt_fmt(page.facts.reliability->scale) : std::string{},
        page.facts.warranty.has_value()
            ? fmt_num(page.facts.warranty->reliability_at_warranty) : std::string{},
        table_value(page, "Scale", "η", "Reliability"));
    std::string p = require_value(table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("N", "有效观测", n, "input"));
    tr.bindings.push_back(bind("LD50", "关键参数", param, "result"));
    push_step(tr.steps, make_step(1, "数据汇总", "failures/censoring", "N = " + n, n));
    push_step(tr.steps, make_step(2, "模型拟合", "logit(p) = β₀ + β₁·stress；LD50 = −β₀/β₁", "LD50 = " + param, param));
    push_step(tr.steps, make_step(3, "推断", "SE/CI", param, param));
    tr.result_value = param;
    tr.substituted_text = "logit(p) = β₀ + β₁·stress；LD50 = −β₀/β₁ → " + param;
    return finish_l3(page, std::move(tr));
}

bool try_attach_ordinal_logistic(OutputPage& page)
{
    return attach_regression_l3(page, "ordinal_logistic", "ordinal_logistic", "有序 Logistic", "logit P(Y≤k) = θ_k + x'β", "Deviance");
}

bool try_attach_nominal_logistic(OutputPage& page)
{
    return attach_regression_l3(page, "nominal_logistic", "nominal_logistic", "名义 Logistic", "log(P(Y=k)/P(Y=ref)) = η_k", "Deviance");
}

bool try_attach_stepwise_regression(OutputPage& page)
{
    return attach_regression_l3(page, "stepwise_regression", "stepwise_regression", "逐步回归", "进入/剔除按 F 或 AIC 准则", "R²");
}

bool try_attach_best_subsets_regression(OutputPage& page)
{
    return attach_regression_l3(page, "best_subsets_regression", "best_subsets_cp", "最优子集回归", "Cp = SSE_p / s²_full − (n − 2(p+1))", "Cp");
}

bool try_attach_reliability(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "reliability";
    tr.formula_id = "kaplan_meier";
    tr.title = "Kaplan-Meier 生存";
    tr.plain_formula = "Ŝ(t) = Π (1 − d_i/n_i) at death times";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Survival_analysis";
    tr.result_symbol = "Ŝ";
    std::string n = require_value(
        page.facts.reliability.has_value() && page.facts.reliability->valid_count.has_value()
            ? std::to_string(*page.facts.reliability->valid_count) : std::string{},
        page.facts.warranty.has_value()
            ? std::to_string(page.facts.warranty->valid_count) : std::string{},
        table_value(page, "N", "Valid"));
    std::string param = require_value(
        page.facts.reliability.has_value()
            ? opt_fmt(page.facts.reliability->scale) : std::string{},
        page.facts.warranty.has_value()
            ? fmt_num(page.facts.warranty->reliability_at_warranty) : std::string{},
        table_value(page, "Scale", "η", "Reliability"));
    std::string p = require_value(table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("N", "有效观测", n, "input"));
    tr.bindings.push_back(bind("Ŝ", "关键参数", param, "result"));
    push_step(tr.steps, make_step(1, "数据汇总", "failures/censoring", "N = " + n, n));
    push_step(tr.steps, make_step(2, "模型拟合", "Ŝ(t) = Π (1 − d_i/n_i) at death times", "Ŝ = " + param, param));
    push_step(tr.steps, make_step(3, "推断", "SE/CI", param, param));
    tr.result_value = param;
    tr.substituted_text = "Ŝ(t) = Π (1 − d_i/n_i) at death times → " + param;
    return finish_l3(page, std::move(tr));
}

bool try_attach_accelerated_life(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "accelerated_life";
    tr.formula_id = "accelerated_life";
    tr.title = "加速寿命模型";
    tr.plain_formula = "log(Y) = β₀ + β₁·x + (1/shape)·Φ⁻¹(p)";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Survival_analysis";
    tr.result_symbol = "β₁";
    std::string n = require_value(
        page.facts.reliability.has_value() && page.facts.reliability->valid_count.has_value()
            ? std::to_string(*page.facts.reliability->valid_count) : std::string{},
        page.facts.warranty.has_value()
            ? std::to_string(page.facts.warranty->valid_count) : std::string{},
        table_value(page, "N", "Valid"));
    std::string param = require_value(
        page.facts.reliability.has_value()
            ? opt_fmt(page.facts.reliability->scale) : std::string{},
        page.facts.warranty.has_value()
            ? fmt_num(page.facts.warranty->reliability_at_warranty) : std::string{},
        table_value(page, "Scale", "η", "Reliability"));
    std::string p = require_value(table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("N", "有效观测", n, "input"));
    tr.bindings.push_back(bind("β₁", "关键参数", param, "result"));
    push_step(tr.steps, make_step(1, "数据汇总", "failures/censoring", "N = " + n, n));
    push_step(tr.steps, make_step(2, "模型拟合", "log(Y) = β₀ + β₁·x + (1/shape)·Φ⁻¹(p)", "β₁ = " + param, param));
    push_step(tr.steps, make_step(3, "推断", "SE/CI", param, param));
    tr.result_value = param;
    tr.substituted_text = "log(Y) = β₀ + β₁·x + (1/shape)·Φ⁻¹(p) → " + param;
    return finish_l3(page, std::move(tr));
}

bool try_attach_reliability_warranty(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "reliability_warranty";
    tr.formula_id = "reliability_warranty";
    tr.title = "保修可靠性预测";
    tr.plain_formula = "E[claims] = exposure × (1 − R(t_w))";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Survival_analysis";
    tr.result_symbol = "Claims";
    std::string n = require_value(
        page.facts.reliability.has_value() && page.facts.reliability->valid_count.has_value()
            ? std::to_string(*page.facts.reliability->valid_count) : std::string{},
        page.facts.warranty.has_value()
            ? std::to_string(page.facts.warranty->valid_count) : std::string{},
        table_value(page, "N", "Valid"));
    std::string param = require_value(
        page.facts.reliability.has_value()
            ? opt_fmt(page.facts.reliability->scale) : std::string{},
        page.facts.warranty.has_value()
            ? fmt_num(page.facts.warranty->reliability_at_warranty) : std::string{},
        table_value(page, "Scale", "η", "Reliability"));
    std::string p = require_value(table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("N", "有效观测", n, "input"));
    tr.bindings.push_back(bind("Claims", "关键参数", param, "result"));
    push_step(tr.steps, make_step(1, "数据汇总", "failures/censoring", "N = " + n, n));
    push_step(tr.steps, make_step(2, "模型拟合", "E[claims] = exposure × (1 − R(t_w))", "Claims = " + param, param));
    push_step(tr.steps, make_step(3, "推断", "SE/CI", param, param));
    tr.result_value = param;
    tr.substituted_text = "E[claims] = exposure × (1 − R(t_w)) → " + param;
    return finish_l3(page, std::move(tr));
}

bool try_attach_imr(OutputPage& page)
{
    return attach_spc_l3(page, "imr", "imr_limits", "I-MR 控制限", "X̄ = mean(x_i)；σ = MR̄/d₂；UCL_I = X̄ + 3σ", "UCL",
        "移动极差 σ", "σ = MR̄/d₂");
}

bool try_attach_xbar_r(OutputPage& page)
{
    return attach_spc_l3(page, "xbar_r", "xbar_r_limits", "X̄-R 控制限", "X̄̄ = mean(X̄_i)；R̄ = mean(R_i)；UCL = X̄̄ + A₂R̄", "UCL",
        "估计 σ", "σ from data");
}

bool try_attach_xbar_s(OutputPage& page)
{
    return attach_spc_l3(page, "xbar_s", "xbar_s_limits", "X̄-S 控制限", "X̄̄ = mean(X̄_i)；S̄ = mean(S_i)；UCL = X̄̄ + A₃S̄", "UCL",
        "估计 σ", "σ from data");
}

bool try_attach_imr_rs(OutputPage& page)
{
    return attach_spc_l3(page, "imr_rs", "imr_rs_limits", "I-MR-R/S 控制限", "I 图用 X̄±3σ；MR/R 图用 MR̄/d₂ 或 R̄/d₂", "UCL",
        "移动极差 σ", "σ = MR̄/d₂");
}

bool try_attach_p_chart(OutputPage& page)
{
    return attach_spc_l3(page, "p_chart", "p_chart_limits", "P 控制图", "p̄ = Σd_i/Σn_i；UCL = p̄ + 3√[p̄(1−p̄)/n_i]", "UCL",
        "比例/计数 σ", "σ = √[p̄(1−p̄)/n]");
}

bool try_attach_np_chart(OutputPage& page)
{
    return attach_spc_l3(page, "np_chart", "np_chart_limits", "NP 控制图", "n̄p̄ = mean(np_i)；UCL = n̄p̄ + 3√[n̄p̄(1−p̄)]", "UCL",
        "比例/计数 σ", "σ = √[p̄(1−p̄)/n]");
}

bool try_attach_c_chart(OutputPage& page)
{
    return attach_spc_l3(page, "c_chart", "c_chart_limits", "C 控制图", "c̄ = mean(c_i)；UCL = c̄ + 3√c̄", "UCL",
        "比例/计数 σ", "σ = √[p̄(1−p̄)/n]");
}

bool try_attach_u_chart(OutputPage& page)
{
    return attach_spc_l3(page, "u_chart", "u_chart_limits", "U 控制图", "ū = Σc_i/Σn_i；UCL = ū + 3√(ū/n_i)", "UCL",
        "比例/计数 σ", "σ = √[p̄(1−p̄)/n]");
}

bool try_attach_laney_p_chart(OutputPage& page)
{
    return attach_spc_l3(page, "laney_p_chart", "laney_p_limits", "Laney P 控制图", "UCL = p̄ + 3σ_Z √[p̄(1−p̄)/n]", "UCL",
        "比例/计数 σ", "σ = √[p̄(1−p̄)/n]");
}

bool try_attach_laney_u_chart(OutputPage& page)
{
    return attach_spc_l3(page, "laney_u_chart", "laney_u_limits", "Laney U 控制图", "UCL = ū + 3σ_Z √(ū/n)", "UCL",
        "比例/计数 σ", "σ = √[p̄(1−p̄)/n]");
}

bool try_attach_ewma(OutputPage& page)
{
    return attach_spc_l3(page, "ewma", "ewma_limits", "EWMA 控制图", "z_t = λ x_t + (1−λ)z_{t−1}；限 = μ ± Lσ√[λ/(2−λ)]", "UCL",
        "EWMA 渐近 σ", "σ_Z = σ√[λ/(2−λ)]");
}

bool try_attach_mewma(OutputPage& page)
{
    return attach_spc_l3(page, "mewma", "mewma_limits", "MEWMA 控制图", "T² = λ y_t' Σ⁻¹ y_t + (1−λ)T²_{t−1}", "UCL",
        "估计 σ", "σ from data");
}

bool try_attach_generalized_variance(OutputPage& page)
{
    return attach_spc_l3(page, "generalized_variance", "generalized_variance", "广义方差控制图", "UCL = |Σ̂|(b₁ + 3√b₂)", "UCL",
        "估计 σ", "σ from data");
}

bool try_attach_cusum(OutputPage& page)
{
    return attach_spc_l3(page, "cusum", "cusum", "CUSUM 控制图", "C⁺_t = max(0, C⁺_{t−1} + (x_t−μ₀)/σ − k)", "C",
        "估计 σ", "σ from data");
}

bool try_attach_zone_chart(OutputPage& page)
{
    return attach_spc_l3(page, "zone_chart", "zone_chart", "Zone 控制图", "Z = (x − μ)/σ；分区 ±1σ, ±2σ, ±3σ", "Z",
        "估计 σ", "σ from data");
}

bool try_attach_z_mr(OutputPage& page)
{
    return attach_spc_l3(page, "z_mr", "z_mr", "Z-MR 控制图", "Z_i = (x_i − μ)/σ；MR_i = |Z_i − Z_{i−1}|", "UCL",
        "移动极差 σ", "σ = MR̄/d₂");
}

bool try_attach_moving_average(OutputPage& page)
{
    return attach_spc_l3(page, "moving_average", "moving_average", "移动平均控制图", "MA_t = (1/w) Σ_{i=0}^{w−1} x_{t−i}", "UCL",
        "估计 σ", "σ from data");
}

bool try_attach_g_chart(OutputPage& page)
{
    return attach_spc_l3(page, "g_chart", "g_chart", "G 控制图", "p̂ = 1/(x̄+1)；CL/LCL/UCL 为几何分位", "UCL",
        "估计 σ", "σ from data");
}

bool try_attach_t_chart(OutputPage& page)
{
    return attach_spc_l3(page, "t_chart", "t_chart", "T 控制图", "基于 Weibull/Gamma 分位构造 CL/UCL/LCL", "UCL",
        "估计 σ", "σ from data");
}

bool try_attach_tolerance_intervals(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "tolerance_intervals";
    tr.formula_id = "tolerance_interval";
    tr.title = "公差区间";
    tr.plain_formula = "TI = x̄ ± k·s (正态) 或非参数次序统计量";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Tolerance_interval";
    tr.result_symbol = "k";
    std::string n = require_value(
        page.facts.tolerance.has_value()
            ? std::to_string(page.facts.tolerance->valid_count) : std::string{},
        table_value(page, "N", "n"));
    std::string mean = require_value(
        page.facts.tolerance.has_value()
            ? opt_fmt(page.facts.tolerance->mean) : std::string{},
        table_value(page, "Mean", "均值"));
    std::string s = require_value(
        page.facts.tolerance.has_value()
            ? opt_fmt(page.facts.tolerance->standard_deviation) : std::string{},
        table_value(page, "StDev", "StDev"));
    std::string k = require_value(
        page.facts.tolerance.has_value()
            ? opt_fmt(page.facts.tolerance->k_factor) : std::string{},
        table_value(page, "K", "k", "Factor"));
    tr.bindings.push_back(bind("n", "N", n, "input"));
    tr.bindings.push_back(bind("x̄", "均值", mean, "input"));
    tr.bindings.push_back(bind("s", "标准差", s, "input"));
    tr.bindings.push_back(bind("k", "k 因子", k, "result"));
    push_step(tr.steps, make_step(1, "x̄, s", "descriptive", "x̄ = " + mean + ", s = " + s, mean));
    push_step(tr.steps, make_step(2, "k 因子", "from (p, γ, n)", "k = " + k, k));
    push_step(tr.steps, make_step(3, "区间", "x̄ ± k·s", "TI = x̄ ± k·s (正态) 或非参数次序统计量", k));
    tr.result_value = k;
    tr.substituted_text = "TI = x̄ ± k·s (正态) 或非参数次序统计量 → k = " + k + ", x̄ = " + mean;
    return finish_l3(page, std::move(tr));
}

bool try_attach_between_within_capability(OutputPage& page)
{
    return attach_capability_l3(page, "between_within_capability", "between_within_cp", "组间组内能力", "σ_BW = √(σ_B² + σ_w²)；Cp = (USL−LSL)/(6σ_BW)", "Cp");
}

bool try_attach_batch_capability(OutputPage& page)
{
    return attach_capability_l3(page, "batch_capability", "batch_capability", "批次能力", "各批次分别估计 Cpk 后汇总", "Cpk");
}

bool try_attach_nonparametric_capability(OutputPage& page)
{
    return attach_capability_l3(page, "nonparametric_capability", "nonparametric_capability", "非参数能力", "Ppk 由经验分位数与规格限计算", "Ppk");
}

bool try_attach_cox_regression(OutputPage& page)
{
    ComputationTrace tr;
    tr.command_id = "cox_regression";
    tr.formula_id = "cox_regression";
    tr.title = "Cox 比例风险";
    tr.plain_formula = "λ(t|x) = λ₀(t) exp(x'β)";
    tr.evidence_type = "formula_reference";
    tr.primary_url = "https://en.wikipedia.org/wiki/Survival_analysis";
    tr.result_symbol = "HR";
    std::string n = require_value(
        page.facts.reliability.has_value() && page.facts.reliability->valid_count.has_value()
            ? std::to_string(*page.facts.reliability->valid_count) : std::string{},
        page.facts.warranty.has_value()
            ? std::to_string(page.facts.warranty->valid_count) : std::string{},
        table_value(page, "N", "Valid"));
    std::string param = require_value(
        page.facts.reliability.has_value()
            ? opt_fmt(page.facts.reliability->scale) : std::string{},
        page.facts.warranty.has_value()
            ? fmt_num(page.facts.warranty->reliability_at_warranty) : std::string{},
        table_value(page, "Scale", "η", "Reliability"));
    std::string p = require_value(table_value(page, "P", "P-Value"));
    tr.bindings.push_back(bind("N", "有效观测", n, "input"));
    tr.bindings.push_back(bind("HR", "关键参数", param, "result"));
    push_step(tr.steps, make_step(1, "数据汇总", "failures/censoring", "N = " + n, n));
    push_step(tr.steps, make_step(2, "模型拟合", "λ(t|x) = λ₀(t) exp(x'β)", "HR = " + param, param));
    push_step(tr.steps, make_step(3, "推断", "SE/CI", param, param));
    tr.result_value = param;
    tr.substituted_text = "λ(t|x) = λ₀(t) exp(x'β) → " + param;
    return finish_l3(page, std::move(tr));
}

bool try_attach_binomial_capability(OutputPage& page)
{
    return attach_capability_l3(page, "binomial_capability", "binomial_capability", "二项能力", "p̂ = Σd_i/Σn_i；PPM = 10⁶·p̂", "Z");
}

bool try_attach_poisson_capability(OutputPage& page)
{
    return attach_capability_l3(page, "poisson_capability", "poisson_capability", "Poisson 能力", "DPU = defects/units；Z 由 Poisson 分位", "Z");
}

bool try_attach_nonnormal_capability(OutputPage& page)
{
    return attach_capability_l3(page, "nonnormal_capability", "nonnormal_capability", "非正态能力", "Ppk = min(PPL, PPU)；PPL = −Z.LSL/3", "Ppk");
}

bool try_attach_capability_sixpack(OutputPage& page)
{
    return attach_capability_l3(page, "capability_sixpack", "capability_sixpack", "能力六合一", "p_i = (i+0.625)/(n+0.25)；z_i = Φ⁻¹(p_i)", "Cpk");
}

bool try_attach_gage_rr(OutputPage& page)
{
    return attach_msa_l3(page, "gage_rr", "gage_rr_ndc", "Gage R&R", "ndc = floor(1.41 × σ_part / σ_gage)", "%GRR");
}

bool try_attach_emp_crossed(OutputPage& page)
{
    return attach_msa_l3(page, "emp_crossed", "emp_crossed", "EMP Crossed", "Part / (Part + Repeat + Operator + Part×Operator)", "%Repeat");
}

bool try_attach_expanded_gage_rr(OutputPage& page)
{
    return attach_msa_l3(page, "expanded_gage_rr", "expanded_gage_rr", "Expanded Gage R&R", "三因子 ANOVA 方差分量", "%GRR");
}

bool try_attach_msa_type1(OutputPage& page)
{
    return attach_msa_l3(page, "msa_type1", "msa_type1", "MSA Type 1", "Cg = Tol/(6s)；Cgk 扣偏倚", "Cg");
}

bool try_attach_nested_gage_rr(OutputPage& page)
{
    return attach_msa_l3(page, "nested_gage_rr", "nested_gage_rr", "Nested Gage R&R", "嵌套 ANOVA 方差分量", "%GRR");
}

bool try_attach_attribute_agreement(OutputPage& page)
{
    return attach_msa_l3(page, "attribute_agreement", "attribute_agreement", "属性一致性", "κ = (P_o − P_e)/(1 − P_e)", "κ");
}


}  // namespace

bool attach_deep_trace(OutputPage& page, const std::string& command_id)
{
    if (command_id == "normality_test") {
        return try_attach_normality_test(page);
    }
    if (command_id == "outlier_test") {
        return try_attach_outlier_test(page);
    }
    if (command_id == "correlation") {
        return try_attach_correlation(page);
    }
    if (command_id == "one_sample_z") {
        return try_attach_one_sample_z(page);
    }
    if (command_id == "one_proportion") {
        return try_attach_one_proportion(page);
    }
    if (command_id == "one_poisson_rate") {
        return try_attach_one_poisson_rate(page);
    }
    if (command_id == "two_poisson_rate") {
        return try_attach_two_poisson_rate(page);
    }
    if (command_id == "two_sample_t") {
        return try_attach_two_sample_t(page);
    }
    if (command_id == "one_sample_equivalence") {
        return try_attach_one_sample_equivalence(page);
    }
    if (command_id == "two_sample_equivalence") {
        return try_attach_two_sample_equivalence(page);
    }
    if (command_id == "two_sample_equivalence_ratio") {
        return try_attach_two_sample_equivalence_ratio(page);
    }
    if (command_id == "paired_equivalence") {
        return try_attach_paired_equivalence(page);
    }
    if (command_id == "one_proportion_equivalence") {
        return try_attach_one_proportion_equivalence(page);
    }
    if (command_id == "two_proportion_equivalence") {
        return try_attach_two_proportion_equivalence(page);
    }
    if (command_id == "one_way_anova") {
        return try_attach_one_way_anova(page);
    }
    if (command_id == "paired_t") {
        return try_attach_paired_t(page);
    }
    if (command_id == "regression") {
        return try_attach_regression(page);
    }
    if (command_id == "two_proportions") {
        return try_attach_two_proportions(page);
    }
    if (command_id == "chi_square") {
        return try_attach_chi_square(page);
    }
    if (command_id == "cross_tabulation") {
        return try_attach_cross_tabulation(page);
    }
    if (command_id == "chi_square_gof") {
        return try_attach_chi_square_gof(page);
    }
    if (command_id == "poisson_gof") {
        return try_attach_poisson_gof(page);
    }
    if (command_id == "mann_whitney") {
        return try_attach_mann_whitney(page);
    }
    if (command_id == "wilcoxon_signed_rank") {
        return try_attach_wilcoxon_signed_rank(page);
    }
    if (command_id == "sign_test") {
        return try_attach_sign_test(page);
    }
    if (command_id == "runs_test") {
        return try_attach_runs_test(page);
    }
    if (command_id == "mcnemar") {
        return try_attach_mcnemar(page);
    }
    if (command_id == "fisher_exact") {
        return try_attach_fisher_exact(page);
    }
    if (command_id == "cochran_q") {
        return try_attach_cochran_q(page);
    }
    if (command_id == "mood_median") {
        return try_attach_mood_median(page);
    }
    if (command_id == "kruskal_wallis") {
        return try_attach_kruskal_wallis(page);
    }
    if (command_id == "friedman") {
        return try_attach_friedman(page);
    }
    if (command_id == "two_factor_anova") {
        return try_attach_two_factor_anova(page);
    }
    if (command_id == "logistic_regression") {
        return try_attach_logistic_regression(page);
    }
    if (command_id == "variance_test") {
        return try_attach_variance_test(page);
    }
    if (command_id == "adf_test") {
        return try_attach_adf_test(page);
    }
    if (command_id == "poisson_regression") {
        return try_attach_poisson_regression(page);
    }
    if (command_id == "probit_reliability") {
        return try_attach_probit_reliability(page);
    }
    if (command_id == "ordinal_logistic") {
        return try_attach_ordinal_logistic(page);
    }
    if (command_id == "nominal_logistic") {
        return try_attach_nominal_logistic(page);
    }
    if (command_id == "stepwise_regression") {
        return try_attach_stepwise_regression(page);
    }
    if (command_id == "best_subsets_regression") {
        return try_attach_best_subsets_regression(page);
    }
    if (command_id == "reliability") {
        return try_attach_reliability(page);
    }
    if (command_id == "accelerated_life") {
        return try_attach_accelerated_life(page);
    }
    if (command_id == "reliability_warranty") {
        return try_attach_reliability_warranty(page);
    }
    if (command_id == "imr") {
        return try_attach_imr(page);
    }
    if (command_id == "xbar_r") {
        return try_attach_xbar_r(page);
    }
    if (command_id == "xbar_s") {
        return try_attach_xbar_s(page);
    }
    if (command_id == "imr_rs") {
        return try_attach_imr_rs(page);
    }
    if (command_id == "p_chart") {
        return try_attach_p_chart(page);
    }
    if (command_id == "np_chart") {
        return try_attach_np_chart(page);
    }
    if (command_id == "c_chart") {
        return try_attach_c_chart(page);
    }
    if (command_id == "u_chart") {
        return try_attach_u_chart(page);
    }
    if (command_id == "laney_p_chart") {
        return try_attach_laney_p_chart(page);
    }
    if (command_id == "laney_u_chart") {
        return try_attach_laney_u_chart(page);
    }
    if (command_id == "ewma") {
        return try_attach_ewma(page);
    }
    if (command_id == "mewma") {
        return try_attach_mewma(page);
    }
    if (command_id == "generalized_variance") {
        return try_attach_generalized_variance(page);
    }
    if (command_id == "cusum") {
        return try_attach_cusum(page);
    }
    if (command_id == "zone_chart") {
        return try_attach_zone_chart(page);
    }
    if (command_id == "z_mr") {
        return try_attach_z_mr(page);
    }
    if (command_id == "moving_average") {
        return try_attach_moving_average(page);
    }
    if (command_id == "g_chart") {
        return try_attach_g_chart(page);
    }
    if (command_id == "t_chart") {
        return try_attach_t_chart(page);
    }
    if (command_id == "tolerance_intervals") {
        return try_attach_tolerance_intervals(page);
    }
    if (command_id == "between_within_capability") {
        return try_attach_between_within_capability(page);
    }
    if (command_id == "batch_capability") {
        return try_attach_batch_capability(page);
    }
    if (command_id == "nonparametric_capability") {
        return try_attach_nonparametric_capability(page);
    }
    if (command_id == "cox_regression") {
        return try_attach_cox_regression(page);
    }
    if (command_id == "binomial_capability") {
        return try_attach_binomial_capability(page);
    }
    if (command_id == "poisson_capability") {
        return try_attach_poisson_capability(page);
    }
    if (command_id == "nonnormal_capability") {
        return try_attach_nonnormal_capability(page);
    }
    if (command_id == "capability_sixpack") {
        return try_attach_capability_sixpack(page);
    }
    if (command_id == "gage_rr") {
        return try_attach_gage_rr(page);
    }
    if (command_id == "emp_crossed") {
        return try_attach_emp_crossed(page);
    }
    if (command_id == "expanded_gage_rr") {
        return try_attach_expanded_gage_rr(page);
    }
    if (command_id == "msa_type1") {
        return try_attach_msa_type1(page);
    }
    if (command_id == "nested_gage_rr") {
        return try_attach_nested_gage_rr(page);
    }
    if (command_id == "attribute_agreement") {
        return try_attach_attribute_agreement(page);
    }
    return false;
}

}  // namespace datalab::application
