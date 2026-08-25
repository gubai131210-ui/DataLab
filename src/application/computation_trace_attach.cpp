#include "application/computation_trace_attach.h"
#include "application/computation_trace_attach_deep.h"
#include "application/computation_trace_helpers.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace datalab::application {
namespace {

using trace_helpers::make_step;
using trace_helpers::opt_fmt;
using trace_helpers::push_step;

using datalab::domain::ComputationStep;
using datalab::domain::ComputationTrace;
using datalab::domain::FormulaBinding;
using datalab::domain::OutputPage;
using datalab::domain::StatisticTable;

// Compile-time / grep catalog of every non-E G9 command id (141).
constexpr const char* k_g9_covered_commands[] = {
    "descriptive",
    "normality_test",
    "outlier_test",
    "correlation",
    "one_sample_t",
    "one_sample_z",
    "one_proportion",
    "one_poisson_rate",
    "two_poisson_rate",
    "two_sample_t",
    "one_sample_equivalence",
    "two_sample_equivalence",
    "two_sample_equivalence_ratio",
    "paired_equivalence",
    "one_proportion_equivalence",
    "two_proportion_equivalence",
    "one_way_anova",
    "paired_t",
    "regression",
    "two_proportions",
    "chi_square",
    "cross_tabulation",
    "chi_square_gof",
    "poisson_gof",
    "anom",
    "mann_whitney",
    "wilcoxon_signed_rank",
    "sign_test",
    "runs_test",
    "mcnemar",
    "fisher_exact",
    "cochran_q",
    "mood_median",
    "kruskal_wallis",
    "friedman",
    "time_series_smoothing",
    "arima",
    "two_factor_anova",
    "logistic_regression",
    "variance_test",
    "time_series_decomposition",
    "seasonal_forecasting",
    "pca",
    "kmeans",
    "cart_tree",
    "random_forest",
    "adf_test",
    "poisson_regression",
    "isolation_forest",
    "bootstrap_mean",
    "distribution_calculator",
    "bootstrap_two_sample",
    "probit_reliability",
    "cluster_observations",
    "ordinal_logistic",
    "nominal_logistic",
    "discriminant",
    "ccf",
    "correlogram",
    "stepwise_regression",
    "best_subsets_regression",
    "km_interval",
    "doe_plackett_burman",
    "taguchi_orthogonal_design",
    "doe_ccd",
    "doe_bbd",
    "reliability",
    "accelerated_life",
    "reliability_warranty",
    "t_power",
    "acf_pacf",
    "histogram",
    "eda_4plot",
    "boxplot",
    "pareto",
    "run_chart",
    "cause_and_effect",
    "density_plot",
    "hexbin_plot",
    "violin_plot",
    "bar_chart",
    "scatter_plot",
    "interval_plot",
    "correlation_plot",
    "bubble_plot",
    "probability_plot",
    "ecdf_plot",
    "matrix_plot",
    "marginal_plot",
    "parallel_plot",
    "heatmap_plot",
    "time_series_plot",
    "area_plot",
    "contour_plot",
    "pie_plot",
    "imr",
    "xbar_r",
    "xbar_s",
    "imr_rs",
    "p_chart",
    "np_chart",
    "c_chart",
    "u_chart",
    "laney_p_chart",
    "laney_u_chart",
    "ewma",
    "hotelling_t2",
    "mewma",
    "generalized_variance",
    "cusum",
    "zone_chart",
    "z_mr",
    "moving_average",
    "g_chart",
    "t_chart",
    "capability",
    "multi_vari",
    "variability_chart",
    "acceptance_sampling",
    "tolerance_intervals",
    "distribution_identification",
    "between_within_capability",
    "batch_capability",
    "nonparametric_capability",
    "cox_regression",
    "weibayes",
    "binomial_capability",
    "poisson_capability",
    "nonnormal_capability",
    "capability_sixpack",
    "box_cox",
    "gage_rr",
    "emp_crossed",
    "expanded_gage_rr",
    "msa_type1",
    "nested_gage_rr",
    "attribute_agreement",
    "doe_factorial",
    "doe_response",
    "rsm_response",
    "response_optimization"
};

std::string fmt_num(double value)
{
    std::ostringstream oss;
    oss << std::setprecision(10) << value;
    return oss.str();
}

std::string opt_size(const std::optional<std::size_t>& value)
{
    if (!value.has_value()) {
        return {};
    }
    return std::to_string(*value);
}

FormulaBinding bind(
    const std::string& symbol,
    const std::string& label,
    const std::string& value,
    const std::string& role)
{
    FormulaBinding b;
    b.symbol = symbol;
    b.label = label;
    b.value = value;
    b.role = role;
    return b;
}

std::string table_value_impl(
    const OutputPage& page, const std::vector<std::string>& wanted)
{
    for (const StatisticTable& table : page.tables) {
        for (const auto& row : table.rows) {
            if (row.empty()) {
                continue;
            }
            for (const std::string& label : wanted) {
                if (row.front() == label && row.size() >= 2) {
                    return row[1];
                }
                for (std::size_t i = 0; i + 1 < row.size(); ++i) {
                    if (row[i] == label) {
                        return row[i + 1];
                    }
                }
            }
        }
    }
    return {};
}

std::string table_value(const OutputPage& page, const std::string& a)
{
    return table_value_impl(page, {a});
}
std::string table_value(
    const OutputPage& page, const std::string& a, const std::string& b)
{
    return table_value_impl(page, {a, b});
}
std::string table_value(
    const OutputPage& page,
    const std::string& a,
    const std::string& b,
    const std::string& c)
{
    return table_value_impl(page, {a, b, c});
}
std::string table_value(
    const OutputPage& page,
    const std::string& a,
    const std::string& b,
    const std::string& c,
    const std::string& d)
{
    return table_value_impl(page, {a, b, c, d});
}
std::string table_value(
    const OutputPage& page,
    const std::string& a,
    const std::string& b,
    const std::string& c,
    const std::string& d,
    const std::string& e)
{
    return table_value_impl(page, {a, b, c, d, e});
}
std::string table_value(
    const OutputPage& page,
    const std::string& a,
    const std::string& b,
    const std::string& c,
    const std::string& d,
    const std::string& e,
    const std::string& f)
{
    return table_value_impl(page, {a, b, c, d, e, f});
}

std::string first_nonempty(std::initializer_list<std::string> values)
{
    for (const std::string& value : values) {
        if (!value.empty()) {
            return value;
        }
    }
    return {};
}

const std::unordered_map<std::string, std::string>& prefix_map()
{
    static const std::unordered_map<std::string, std::string> map = {
        {"desc", "descriptive"},
        {"normality", "normality_test"},
        {"outlier_test", "outlier_test"},
        {"correlation", "correlation"},
        {"one_sample_t", "one_sample_t"},
        {"one_sample_z", "one_sample_z"},
        {"two_sample_t", "two_sample_t"},
        {"paired_t", "paired_t"},
        {"anova", "one_way_anova"},
        {"two_factor_anova", "two_factor_anova"},
        {"regression", "regression"},
        {"logistic", "logistic_regression"},
        {"two_proportions", "two_proportions"},
        {"one_proportion", "one_proportion"},
        {"one_poisson_rate", "one_poisson_rate"},
        {"two_poisson_rate", "two_poisson_rate"},
        {"chi_square", "chi_square"},
        {"cross_tab", "cross_tabulation"},
        {"chi_square_gof", "chi_square_gof"},
        {"poisson_gof", "poisson_gof"},
        {"box_cox", "box_cox"},
        {"gage_rr", "gage_rr"},
        {"emp", "emp_crossed"},
        {"expanded_gage", "expanded_gage_rr"},
        {"msa_type1", "msa_type1"},
        {"nested_gage", "nested_gage_rr"},
        {"attribute_agreement", "attribute_agreement"},
        {"mann_whitney", "mann_whitney"},
        {"wilcoxon", "wilcoxon_signed_rank"},
        {"sign_test", "sign_test"},
        {"runs_test", "runs_test"},
        {"fisher_exact", "fisher_exact"},
        {"mcnemar", "mcnemar"},
        {"cochran_q", "cochran_q"},
        {"mood_median", "mood_median"},
        {"kruskal_wallis", "kruskal_wallis"},
        {"friedman", "friedman"},
        {"cap", "capability"},
        {"binomial_cap", "binomial_capability"},
        {"poisson_cap", "poisson_capability"},
        {"sixpack", "capability_sixpack"},
        {"batch_capability", "batch_capability"},
        {"nonparametric_capability", "nonparametric_capability"},
        {"distribution_id", "distribution_identification"},
        {"tolerance", "tolerance_intervals"},
        {"acceptance_sampling", "acceptance_sampling"},
        {"multi_vari", "multi_vari"},
        {"variability_chart", "variability_chart"},
        {"imr", "imr"},
        {"imrrs", "imr_rs"},
        {"ewma", "ewma"},
        {"cusum", "cusum"},
        {"zone_chart", "zone_chart"},
        {"z_mr", "z_mr"},
        {"moving_average", "moving_average"},
        {"t2", "hotelling_t2"},
        {"mewma", "mewma"},
        {"gv", "generalized_variance"},
        {"hist", "histogram"},
        {"eda4", "eda_4plot"},
        {"box", "boxplot"},
        {"pareto", "pareto"},
        {"run_chart", "run_chart"},
        {"cause_and_effect", "cause_and_effect"},
        {"weibayes", "weibayes"},
        {"reliability", "reliability"},
        {"accelerated_life", "accelerated_life"},
        {"reliability_warranty", "reliability_warranty"},
        {"cox_regression", "cox_regression"},
        {"probit_reliability", "probit_reliability"},
        {"km_interval", "km_interval"},
        {"random_forest", "random_forest"},
        {"distribution_calculator", "distribution_calculator"},
        {"t_power", "t_power"},
        {"pca", "pca"},
        {"kmeans", "kmeans"},
        {"cart_tree", "cart_tree"},
        {"isolation_forest", "isolation_forest"},
        {"bootstrap_mean", "bootstrap_mean"},
        {"bootstrap_two_sample", "bootstrap_two_sample"},
        {"adf_test", "adf_test"},
        {"poisson_regression", "poisson_regression"},
        {"ordinal_logistic", "ordinal_logistic"},
        {"nominal_logistic", "nominal_logistic"},
        {"discriminant", "discriminant"},
        {"cluster_observations", "cluster_observations"},
        {"ccf", "ccf"},
        {"correlogram", "correlogram"},
        {"stepwise_regression", "stepwise_regression"},
        {"best_subsets_regression", "best_subsets_regression"},
        {"acf_pacf", "acf_pacf"},
        {"arima", "arima"},
        {"decomposition", "time_series_decomposition"},
        {"seasonal_forecast", "seasonal_forecasting"},
        {"time_series", "time_series_smoothing"},
        {"anom", "anom"},
        {"variance", "variance_test"},
        {"response_optimization", "response_optimization"},
        {"rsm_response", "rsm_response"},
    };
    return map;
}

void append_design_rule_steps(std::vector<ComputationStep>& steps)
{
    push_step(steps, make_step(0, "选择阵列/设计类型", "design", "design", ""));
    push_step(steps, make_step(0, "按水平与因子生成运行表", "levels × factors", "run table", ""));
    push_step(steps, make_step(0, "导出工作表或分析模型", "runs", "worksheet/model", ""));
}

void attach_impl(OutputPage& page, const std::string& command_id)
{
    if (command_id.empty() || command_id == "tests" || command_id == "rule_policy") {
        return;
    }
    // Touch catalog so the array is not optimized away and remains greppable.
    static_cast<void>(k_g9_covered_commands);

    if (attach_deep_trace(page, command_id)) {
        return;
    }

    if (command_id == "capability") {
        ComputationTrace tr;
        tr.command_id = "capability";
        tr.formula_id = "cpk_min_usl_lsl";
        tr.title = "过程能力 Cpk";
        tr.plain_formula = "Cpk = min( (USL-μ)/(3σ), (μ-LSL)/(3σ) )";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Process_capability_index";
        tr.result_symbol = "Cpk";
        const auto& cfg = page.configuration.specifications;
        const std::string usl = cfg.upper.has_value() ? fmt_num(*cfg.upper)
            : first_nonempty({table_value(page, "USL"), "见结果表"});
        const std::string lsl = cfg.lower.has_value() ? fmt_num(*cfg.lower)
            : first_nonempty({table_value(page, "LSL"), "见结果表"});
        const std::string mu = first_nonempty({
            table_value(page, "Mean", "均值", "Process Mean"),
            "见结果表"});
        const std::string sigma = first_nonempty({
            table_value(page, "Within", "StDev", "σ", "Sigma", "标准差"),
            "见结果表"});
        std::string cpk = "见结果表";
        if (page.facts.capability.has_value() && page.facts.capability->cpk.has_value()) {
            cpk = fmt_num(*page.facts.capability->cpk);
        } else {
            cpk = first_nonempty({table_value(page, "Cpk"), "见结果表"});
        }
        tr.bindings.push_back(bind("USL", "规格上限", usl, "input"));
        tr.bindings.push_back(bind("LSL", "规格下限", lsl, "input"));
        tr.bindings.push_back(bind("μ", "过程均值", mu, "input"));
        tr.bindings.push_back(bind("σ", "组内标准差", sigma, "input"));
        tr.bindings.push_back(bind("Cpk", "过程能力指数", cpk, "result"));
        tr.result_value = cpk;

        push_step(tr.steps, make_step(1, "上规格距离", "Cpu = (USL−μ)/(3σ)",
            "(" + usl + "−" + mu + ")/(3·" + sigma + ")", usl));
        push_step(tr.steps, make_step(2, "下规格距离", "Cpl = (μ−LSL)/(3σ)",
            "(" + mu + "−" + lsl + ")/(3·" + sigma + ")", lsl));
        tr.substituted_text = "Cpk = min( (" + usl + "-" + mu + ")/(3·" + sigma
            + "), (" + mu + "-" + lsl + ")/(3·" + sigma + ") ) = " + cpk;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "one_sample_t") {
        ComputationTrace tr;
        tr.command_id = "one_sample_t";
        tr.formula_id = "one_sample_t_statistic";
        tr.title = "单样本 t 检验";
        tr.plain_formula = "t = (x̄ - μ₀) / (s / √n)";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Student%27s_t-test";
        tr.result_symbol = "t";
        std::string n = "见结果表";
        std::string xbar = "见结果表";
        std::string s = first_nonempty({table_value(page, "StDev", "标准差", "s", "SE Mean"), "见结果表"});
        std::string mu0 = page.configuration.inference.hypothesis_mean.has_value()
            ? fmt_num(*page.configuration.inference.hypothesis_mean)
            : first_nonempty({table_value(page, "假设均值", "Hypothesized Mean", "μ0"), "见结果表"});
        std::string tstat = first_nonempty({table_value(page, "T", "t", "Statistic"), "见结果表"});
        if (page.facts.t_test.has_value()) {
            n = std::to_string(page.facts.t_test->n);
            if (page.facts.t_test->mean.has_value()) {
                xbar = fmt_num(*page.facts.t_test->mean);
            }
            if (page.facts.t_test->sample_standard_deviation.has_value()) {
                s = fmt_num(*page.facts.t_test->sample_standard_deviation);
            }
        } else {
            n = first_nonempty({table_value(page, "N", "n"), n});
            xbar = first_nonempty({table_value(page, "Mean", "均值"), xbar});
        }
        tr.bindings.push_back(bind("n", "样本量", n, "input"));
        tr.bindings.push_back(bind("x̄", "样本均值", xbar, "input"));
        tr.bindings.push_back(bind("s", "样本标准差", s, "input"));
        tr.bindings.push_back(bind("μ₀", "假设均值", mu0, "input"));
        tr.bindings.push_back(bind("t", "t 统计量", tstat, "result"));
        tr.result_value = tstat;

        push_step(tr.steps, make_step(1, "样本均值", "x̄ = Σx/n", "x̄ = " + xbar, xbar));
        push_step(tr.steps, make_step(2, "标准误", "SE = s/√n", "SE = " + s + "/√" + n, s));
        push_step(tr.steps, make_step(3, "t 统计量", "t = (x̄−μ₀)/SE",
            "t = " + tstat, tstat));
        tr.substituted_text = "t = (" + xbar + " - " + mu0 + ") / (" + s + " / √" + n
            + ") = " + tstat;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "weibayes") {
        ComputationTrace tr;
        tr.command_id = "weibayes";
        tr.formula_id = "weibayes_scale";
        tr.title = "Weibayes 尺度 η";
        tr.plain_formula = "η = ( Σ t_i^β / r )^(1/β)";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Weibull_distribution";
        tr.result_symbol = "η";
        std::string beta = fmt_num(page.configuration.weibayes.shape_prior);
        std::string r = "见结果表";
        std::string eta = "见结果表";
        if (page.facts.weibayes.has_value()) {
            beta = fmt_num(page.facts.weibayes->shape_prior);
            r = std::to_string(page.facts.weibayes->failure_count);
            if (page.facts.weibayes->scale.has_value()) {
                eta = fmt_num(*page.facts.weibayes->scale);
            }
        } else {
            r = first_nonempty({table_value(page, "Failures (r)", "Failures", "r"), r});
            eta = first_nonempty({table_value(page, "Scale η", "Scale", "η"), eta});
            beta = first_nonempty({table_value(page, "Shape β (prior)", "Shape", "β"), beta});
        }
        tr.bindings.push_back(bind("β", "形状先验", beta, "input"));
        tr.bindings.push_back(bind("r", "失效数", r, "input"));
        tr.bindings.push_back(bind("η", "尺度参数", eta, "result"));
        tr.result_value = eta;

        push_step(tr.steps, make_step(1, "形状先验 β", "β fixed", "β = " + beta, beta));
        push_step(tr.steps, make_step(2, "失效数 r", "r = failures", "r = " + r, r));
        push_step(tr.steps, make_step(3, "尺度 η", "η = (Σ t_i^β / r)^(1/β)",
            "η = " + eta, eta));
        tr.substituted_text = "η = ( Σ t_i^" + beta + " / " + r + " )^(1/" + beta
            + ") = " + eta;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "descriptive") {
        ComputationTrace tr;
        tr.command_id = "descriptive";
        tr.formula_id = "descriptive_mean";
        tr.title = "描述性统计均值";
        tr.plain_formula = "x̄ = Σx_i / n";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "x̄";
        const std::string xbar = first_nonempty({
            page.facts.descriptive.has_value()
                ? opt_fmt(page.facts.descriptive->mean) : std::string{},
            table_value(page, "Mean", "均值", "Average")});
        const std::string n_desc = first_nonempty({
            page.facts.descriptive.has_value()
                ? std::to_string(page.facts.descriptive->n) : std::string{},
            table_value(page, "N", "n", "有效 N")});
        tr.bindings.push_back(bind("n", "样本量 N", n_desc, "input"));
        tr.bindings.push_back(bind("x̄", "均值", xbar, "result"));
        tr.result_value = xbar;
        push_step(tr.steps, make_step(1, "求和", "Σx_i", "n = " + n_desc, n_desc));
        push_step(tr.steps, make_step(2, "均值", "x̄ = Σx_i/n", "x̄ = " + xbar, xbar));
        tr.substituted_text = "x̄ = Σx_i / " + n_desc + " = " + xbar;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "anom") {
        ComputationTrace tr;
        tr.command_id = "anom";
        tr.formula_id = "anom_main";
        tr.title = "anom 生成规则";
        tr.plain_formula = "anom 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "anom 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "time_series_smoothing") {
        ComputationTrace tr;
        tr.command_id = "time_series_smoothing";
        tr.formula_id = "time_series_smoothing_main";
        tr.title = "time series smoothing 生成规则";
        tr.plain_formula = "time_series_smoothing 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "time_series_smoothing 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "arima") {
        ComputationTrace tr;
        tr.command_id = "arima";
        tr.formula_id = "arima_main";
        tr.title = "arima 生成规则";
        tr.plain_formula = "arima 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "arima 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "time_series_decomposition") {
        ComputationTrace tr;
        tr.command_id = "time_series_decomposition";
        tr.formula_id = "time_series_decomposition_main";
        tr.title = "time series decomposition 生成规则";
        tr.plain_formula = "time_series_decomposition 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "time_series_decomposition 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "seasonal_forecasting") {
        ComputationTrace tr;
        tr.command_id = "seasonal_forecasting";
        tr.formula_id = "seasonal_forecasting_main";
        tr.title = "seasonal forecasting 生成规则";
        tr.plain_formula = "seasonal_forecasting 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "seasonal_forecasting 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "pca") {
        ComputationTrace tr;
        tr.command_id = "pca";
        tr.formula_id = "pca_main";
        tr.title = "pca 关键方程";
        tr.plain_formula = "pca 关键方程（维数摘要）";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "result";
        tr.bindings.push_back(bind("dim", "维数/规模", first_nonempty({table_value(page, "N", "p", "k", "Components", "Trees"), "见结果表"}), "input"));
        tr.bindings.push_back(bind("key", "关键量", first_nonempty({table_value(page, "Result", "Statistic", "R²", "Inertia"), "见结果表"}), "result"));
        tr.result_value = first_nonempty({table_value(page, "Result", "Statistic"), "见结果表"});
        tr.substituted_text = "pca 关键方程（维数摘要）（维数摘要，非全矩阵） → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "kmeans") {
        ComputationTrace tr;
        tr.command_id = "kmeans";
        tr.formula_id = "kmeans_main";
        tr.title = "kmeans 关键方程";
        tr.plain_formula = "kmeans 关键方程（维数摘要）";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "result";
        tr.bindings.push_back(bind("dim", "维数/规模", first_nonempty({table_value(page, "N", "p", "k", "Components", "Trees"), "见结果表"}), "input"));
        tr.bindings.push_back(bind("key", "关键量", first_nonempty({table_value(page, "Result", "Statistic", "R²", "Inertia"), "见结果表"}), "result"));
        tr.result_value = first_nonempty({table_value(page, "Result", "Statistic"), "见结果表"});
        tr.substituted_text = "kmeans 关键方程（维数摘要）（维数摘要，非全矩阵） → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "cart_tree") {
        ComputationTrace tr;
        tr.command_id = "cart_tree";
        tr.formula_id = "cart_tree_main";
        tr.title = "cart tree 关键方程";
        tr.plain_formula = "cart_tree 关键方程（维数摘要）";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "result";
        tr.bindings.push_back(bind("dim", "维数/规模", first_nonempty({table_value(page, "N", "p", "k", "Components", "Trees"), "见结果表"}), "input"));
        tr.bindings.push_back(bind("key", "关键量", first_nonempty({table_value(page, "Result", "Statistic", "R²", "Inertia"), "见结果表"}), "result"));
        tr.result_value = first_nonempty({table_value(page, "Result", "Statistic"), "见结果表"});
        tr.substituted_text = "cart_tree 关键方程（维数摘要）（维数摘要，非全矩阵） → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "random_forest") {
        ComputationTrace tr;
        tr.command_id = "random_forest";
        tr.formula_id = "random_forest_main";
        tr.title = "random forest 关键方程";
        tr.plain_formula = "random_forest 关键方程（维数摘要）";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "result";
        tr.bindings.push_back(bind("dim", "维数/规模", first_nonempty({table_value(page, "N", "p", "k", "Components", "Trees"), "见结果表"}), "input"));
        tr.bindings.push_back(bind("key", "关键量", first_nonempty({table_value(page, "Result", "Statistic", "R²", "Inertia"), "见结果表"}), "result"));
        tr.result_value = first_nonempty({table_value(page, "Result", "Statistic"), "见结果表"});
        tr.substituted_text = "random_forest 关键方程（维数摘要）（维数摘要，非全矩阵） → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "isolation_forest") {
        ComputationTrace tr;
        tr.command_id = "isolation_forest";
        tr.formula_id = "isolation_forest_main";
        tr.title = "isolation forest 关键方程";
        tr.plain_formula = "isolation_forest 关键方程（维数摘要）";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "result";
        tr.bindings.push_back(bind("dim", "维数/规模", first_nonempty({table_value(page, "N", "p", "k", "Components", "Trees"), "见结果表"}), "input"));
        tr.bindings.push_back(bind("key", "关键量", first_nonempty({table_value(page, "Result", "Statistic", "R²", "Inertia"), "见结果表"}), "result"));
        tr.result_value = first_nonempty({table_value(page, "Result", "Statistic"), "见结果表"});
        tr.substituted_text = "isolation_forest 关键方程（维数摘要）（维数摘要，非全矩阵） → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "bootstrap_mean") {
        ComputationTrace tr;
        tr.command_id = "bootstrap_mean";
        tr.formula_id = "bootstrap_mean_main";
        tr.title = "bootstrap mean 生成规则";
        tr.plain_formula = "bootstrap_mean 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "bootstrap_mean 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "distribution_calculator") {
        ComputationTrace tr;
        tr.command_id = "distribution_calculator";
        tr.formula_id = "distribution_calculator_main";
        tr.title = "distribution calculator 关键方程";
        tr.plain_formula = "distribution_calculator 关键方程（维数摘要）";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "result";
        tr.bindings.push_back(bind("dim", "维数/规模", first_nonempty({table_value(page, "N", "p", "k", "Components", "Trees"), "见结果表"}), "input"));
        tr.bindings.push_back(bind("key", "关键量", first_nonempty({table_value(page, "Result", "Statistic", "R²", "Inertia"), "见结果表"}), "result"));
        tr.result_value = first_nonempty({table_value(page, "Result", "Statistic"), "见结果表"});
        tr.substituted_text = "distribution_calculator 关键方程（维数摘要）（维数摘要，非全矩阵） → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "bootstrap_two_sample") {
        ComputationTrace tr;
        tr.command_id = "bootstrap_two_sample";
        tr.formula_id = "bootstrap_two_sample_main";
        tr.title = "bootstrap two sample 生成规则";
        tr.plain_formula = "bootstrap_two_sample 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "bootstrap_two_sample 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "cluster_observations") {
        ComputationTrace tr;
        tr.command_id = "cluster_observations";
        tr.formula_id = "cluster_observations_main";
        tr.title = "cluster observations 生成规则";
        tr.plain_formula = "cluster_observations 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "cluster_observations 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "discriminant") {
        ComputationTrace tr;
        tr.command_id = "discriminant";
        tr.formula_id = "discriminant_main";
        tr.title = "discriminant 关键方程";
        tr.plain_formula = "discriminant 关键方程（维数摘要）";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "result";
        tr.bindings.push_back(bind("dim", "维数/规模", first_nonempty({table_value(page, "N", "p", "k", "Components", "Trees"), "见结果表"}), "input"));
        tr.bindings.push_back(bind("key", "关键量", first_nonempty({table_value(page, "Result", "Statistic", "R²", "Inertia"), "见结果表"}), "result"));
        tr.result_value = first_nonempty({table_value(page, "Result", "Statistic"), "见结果表"});
        tr.substituted_text = "discriminant 关键方程（维数摘要）（维数摘要，非全矩阵） → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "ccf") {
        ComputationTrace tr;
        tr.command_id = "ccf";
        tr.formula_id = "ccf_main";
        tr.title = "ccf 生成规则";
        tr.plain_formula = "ccf 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "ccf 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "correlogram") {
        ComputationTrace tr;
        tr.command_id = "correlogram";
        tr.formula_id = "correlogram_main";
        tr.title = "correlogram 生成规则";
        tr.plain_formula = "correlogram 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "correlogram 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "km_interval") {
        ComputationTrace tr;
        tr.command_id = "km_interval";
        tr.formula_id = "km_interval_main";
        tr.title = "km interval 生成规则";
        tr.plain_formula = "km_interval 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "km_interval 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "doe_plackett_burman") {
        ComputationTrace tr;
        tr.command_id = "doe_plackett_burman";
        tr.formula_id = "doe_plackett_burman_main";
        tr.title = "doe plackett burman 生成规则";
        tr.plain_formula = "doe_plackett_burman 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "doe_plackett_burman 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "taguchi_orthogonal_design") {
        ComputationTrace tr;
        tr.command_id = "taguchi_orthogonal_design";
        tr.formula_id = "taguchi_orthogonal_design_main";
        tr.title = "taguchi orthogonal design 生成规则";
        tr.plain_formula = "taguchi_orthogonal_design 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "taguchi_orthogonal_design 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "doe_ccd") {
        ComputationTrace tr;
        tr.command_id = "doe_ccd";
        tr.formula_id = "doe_ccd_main";
        tr.title = "doe ccd 生成规则";
        tr.plain_formula = "doe_ccd 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "doe_ccd 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "doe_bbd") {
        ComputationTrace tr;
        tr.command_id = "doe_bbd";
        tr.formula_id = "doe_bbd_main";
        tr.title = "doe bbd 生成规则";
        tr.plain_formula = "doe_bbd 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "doe_bbd 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "t_power") {
        ComputationTrace tr;
        tr.command_id = "t_power";
        tr.formula_id = "t_power_main";
        tr.title = "t power 关键方程";
        tr.plain_formula = "t_power 关键方程（维数摘要）";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "result";
        tr.bindings.push_back(bind("dim", "维数/规模", first_nonempty({table_value(page, "N", "p", "k", "Components", "Trees"), "见结果表"}), "input"));
        tr.bindings.push_back(bind("key", "关键量", first_nonempty({table_value(page, "Result", "Statistic", "R²", "Inertia"), "见结果表"}), "result"));
        tr.result_value = first_nonempty({table_value(page, "Result", "Statistic"), "见结果表"});
        tr.substituted_text = "t_power 关键方程（维数摘要）（维数摘要，非全矩阵） → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "acf_pacf") {
        ComputationTrace tr;
        tr.command_id = "acf_pacf";
        tr.formula_id = "acf_pacf_main";
        tr.title = "acf pacf 生成规则";
        tr.plain_formula = "acf_pacf 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "acf_pacf 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "histogram") {
        ComputationTrace tr;
        tr.command_id = "histogram";
        tr.formula_id = "histogram_display";
        tr.title = "直方图显示摘要";
        tr.plain_formula = "频数分箱展示（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "eda_4plot") {
        ComputationTrace tr;
        tr.command_id = "eda_4plot";
        tr.formula_id = "eda_4plot_main";
        tr.title = "eda 4plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "boxplot") {
        ComputationTrace tr;
        tr.command_id = "boxplot";
        tr.formula_id = "boxplot_main";
        tr.title = "boxplot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "pareto") {
        ComputationTrace tr;
        tr.command_id = "pareto";
        tr.formula_id = "pareto_main";
        tr.title = "pareto 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "run_chart") {
        ComputationTrace tr;
        tr.command_id = "run_chart";
        tr.formula_id = "run_chart_main";
        tr.title = "run chart 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "cause_and_effect") {
        ComputationTrace tr;
        tr.command_id = "cause_and_effect";
        tr.formula_id = "cause_and_effect_main";
        tr.title = "cause and effect 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "density_plot") {
        ComputationTrace tr;
        tr.command_id = "density_plot";
        tr.formula_id = "density_plot_main";
        tr.title = "density plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "hexbin_plot") {
        ComputationTrace tr;
        tr.command_id = "hexbin_plot";
        tr.formula_id = "hexbin_plot_main";
        tr.title = "hexbin plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "violin_plot") {
        ComputationTrace tr;
        tr.command_id = "violin_plot";
        tr.formula_id = "violin_plot_main";
        tr.title = "violin plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "bar_chart") {
        ComputationTrace tr;
        tr.command_id = "bar_chart";
        tr.formula_id = "bar_chart_main";
        tr.title = "bar chart 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "scatter_plot") {
        ComputationTrace tr;
        tr.command_id = "scatter_plot";
        tr.formula_id = "scatter_plot_main";
        tr.title = "scatter plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "interval_plot") {
        ComputationTrace tr;
        tr.command_id = "interval_plot";
        tr.formula_id = "interval_plot_main";
        tr.title = "interval plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "correlation_plot") {
        ComputationTrace tr;
        tr.command_id = "correlation_plot";
        tr.formula_id = "correlation_plot_main";
        tr.title = "correlation plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "bubble_plot") {
        ComputationTrace tr;
        tr.command_id = "bubble_plot";
        tr.formula_id = "bubble_plot_main";
        tr.title = "bubble plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "probability_plot") {
        ComputationTrace tr;
        tr.command_id = "probability_plot";
        tr.formula_id = "probability_plot_main";
        tr.title = "probability plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "ecdf_plot") {
        ComputationTrace tr;
        tr.command_id = "ecdf_plot";
        tr.formula_id = "ecdf_plot_main";
        tr.title = "ecdf plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "matrix_plot") {
        ComputationTrace tr;
        tr.command_id = "matrix_plot";
        tr.formula_id = "matrix_plot_main";
        tr.title = "matrix plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "marginal_plot") {
        ComputationTrace tr;
        tr.command_id = "marginal_plot";
        tr.formula_id = "marginal_plot_main";
        tr.title = "marginal plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "parallel_plot") {
        ComputationTrace tr;
        tr.command_id = "parallel_plot";
        tr.formula_id = "parallel_plot_main";
        tr.title = "parallel plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "heatmap_plot") {
        ComputationTrace tr;
        tr.command_id = "heatmap_plot";
        tr.formula_id = "heatmap_plot_main";
        tr.title = "heatmap plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "time_series_plot") {
        ComputationTrace tr;
        tr.command_id = "time_series_plot";
        tr.formula_id = "time_series_plot_main";
        tr.title = "time series plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "area_plot") {
        ComputationTrace tr;
        tr.command_id = "area_plot";
        tr.formula_id = "area_plot_main";
        tr.title = "area plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "contour_plot") {
        ComputationTrace tr;
        tr.command_id = "contour_plot";
        tr.formula_id = "contour_plot_main";
        tr.title = "contour plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "pie_plot") {
        ComputationTrace tr;
        tr.command_id = "pie_plot";
        tr.formula_id = "pie_plot_main";
        tr.title = "pie plot 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "hotelling_t2") {
        ComputationTrace tr;
        tr.command_id = "hotelling_t2";
        tr.formula_id = "hotelling_t2_main";
        tr.title = "hotelling t2 关键方程";
        tr.plain_formula = "hotelling_t2 关键方程（维数摘要）";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "result";
        tr.bindings.push_back(bind("dim", "维数/规模", first_nonempty({table_value(page, "N", "p", "k", "Components", "Trees"), "见结果表"}), "input"));
        tr.bindings.push_back(bind("key", "关键量", first_nonempty({table_value(page, "Result", "Statistic", "R²", "Inertia"), "见结果表"}), "result"));
        tr.result_value = first_nonempty({table_value(page, "Result", "Statistic"), "见结果表"});
        tr.substituted_text = "hotelling_t2 关键方程（维数摘要）（维数摘要，非全矩阵） → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "multi_vari") {
        ComputationTrace tr;
        tr.command_id = "multi_vari";
        tr.formula_id = "multi_vari_main";
        tr.title = "multi vari 生成规则";
        tr.plain_formula = "multi_vari 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "multi_vari 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "variability_chart") {
        ComputationTrace tr;
        tr.command_id = "variability_chart";
        tr.formula_id = "variability_chart_main";
        tr.title = "variability chart 显示摘要";
        tr.plain_formula = "显示摘要：有效 N（complete-case）";
        tr.evidence_type = "display_summary";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "N";
        const std::string n = first_nonempty({
            opt_size(page.facts.eda.has_value() ? std::optional<std::size_t>(page.facts.eda->n) : std::nullopt),
            table_value(page, "N"),
            table_value(page, "有效 N"),
            "见结果表"});
        tr.bindings.push_back(bind("N", "有效样本量", n, "input"));
        tr.bindings.push_back(bind("complete_case", "缺失策略", "complete-case；hidden≠excluded", "intermediate"));
        tr.result_value = n;
        tr.substituted_text = "显示摘要：N = " + n + "（complete-case）";
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "acceptance_sampling") {
        ComputationTrace tr;
        tr.command_id = "acceptance_sampling";
        tr.formula_id = "acceptance_sampling_main";
        tr.title = "acceptance sampling 生成规则";
        tr.plain_formula = "acceptance_sampling 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "acceptance_sampling 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "distribution_identification") {
        ComputationTrace tr;
        tr.command_id = "distribution_identification";
        tr.formula_id = "distribution_identification_main";
        tr.title = "distribution identification 生成规则";
        tr.plain_formula = "distribution_identification 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "distribution_identification 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "box_cox") {
        ComputationTrace tr;
        tr.command_id = "box_cox";
        tr.formula_id = "box_cox_main";
        tr.title = "box cox 生成规则";
        tr.plain_formula = "box_cox 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "box_cox 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "doe_factorial") {
        ComputationTrace tr;
        tr.command_id = "doe_factorial";
        tr.formula_id = "doe_factorial_main";
        tr.title = "doe factorial 生成规则";
        tr.plain_formula = "doe_factorial 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "doe_factorial 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "doe_response") {
        ComputationTrace tr;
        tr.command_id = "doe_response";
        tr.formula_id = "doe_response_main";
        tr.title = "doe response 生成规则";
        tr.plain_formula = "doe_response 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "doe_response 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "rsm_response") {
        ComputationTrace tr;
        tr.command_id = "rsm_response";
        tr.formula_id = "rsm_response_main";
        tr.title = "rsm response 生成规则";
        tr.plain_formula = "rsm_response 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "rsm_response 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }
    if (command_id == "response_optimization") {
        ComputationTrace tr;
        tr.command_id = "response_optimization";
        tr.formula_id = "response_optimization_main";
        tr.title = "response optimization 生成规则";
        tr.plain_formula = "response_optimization 设计/生成主规则";
        tr.evidence_type = "formula_reference";
        tr.primary_url = "https://en.wikipedia.org/wiki/Statistics";
        tr.result_symbol = "runs";
        tr.bindings.push_back(bind("design", "设计/生成", table_value(page, "Design", "设计", "Array", "Runs"), "input"));
        tr.bindings.push_back(bind("runs", "运行数/点数", first_nonempty({table_value(page, "Runs", "运行数", "N", "点数"), "见结果表"}), "result"));
        append_design_rule_steps(tr.steps);
        tr.result_value = first_nonempty({table_value(page, "Runs", "运行数", "N"), "见结果表"});
        tr.substituted_text = "response_optimization 设计/生成主规则 → " + tr.result_value;
        page.computation_traces.push_back(std::move(tr));
        return;
    }

}

}  // namespace

void attach_computation_traces(OutputPage& page, const std::string& command_id)
{
    attach_impl(page, command_id);
}

std::string resolve_command_id_from_page(const OutputPage& page)
{
    if (!page.analysis_command_id.empty()) {
        return page.analysis_command_id;
    }
    const std::string& id = page.id;
    if (id.empty()) {
        return {};
    }
    // Exact command id prefix (id may be "capability_12").
    for (const char* covered : k_g9_covered_commands) {
        const std::string cid(covered);
        if (id == cid || id.rfind(cid + "_", 0) == 0) {
            return cid;
        }
    }
    const auto underscore = id.find('_');
    const std::string prefix = underscore == std::string::npos ? id : id.substr(0, underscore);
    const auto& map = prefix_map();
    const auto it = map.find(prefix);
    if (it != map.end()) {
        return it->second;
    }
    // Multi-segment prefixes like "one_sample_t_3".
    for (const auto& [key, value] : map) {
        if (id.rfind(key + "_", 0) == 0 || id == key) {
            return value;
        }
    }
    return {};
}

}  // namespace datalab::application
