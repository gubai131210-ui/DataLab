#include "domain/report_text_catalog_parts.h"

namespace datalab::domain {
namespace {

const ReportTextEntry kReportTextCatalogPart18[] = {
{"interp.mixture_analyze_summary",
 "Mixture 分析：q = %1，模型 = %2，N = %3。",
 "Mixture analyze: q = %1, model = %2, N = %3."},
{"interp.mixture_analyze_coef",
 "Scheffé OLS 系数与 ANOVA；无常数项混料模型。",
 "Scheffé OLS coefficients and ANOVA; no-intercept mixture model."},
{"interp.mixture_analyze_scope",
 "独立于 mixture_design；禁止「配方已优化」。",
 "Independent of mixture_design; forbid formulation-optimized claims."},
{"interp.glm_two_way_summary",
 "双因子 GLM：N = %1，交互 = %2，平衡 = %3。",
 "Two-way GLM: N = %1, interaction = %2, balanced = %3."},
{"interp.glm_two_way_fitted",
 "Fitted Means 为回归预测按水平平均；非原始单元均值。",
 "Fitted means average regression predictions by level; not raw cell means."},
{"interp.glm_two_way_scope",
 "Type III 窄化；禁止「过程已合格」。",
 "Type III narrow scope; forbid process-qualified claims."},
{"interp.analyze_variability_summary",
 "Analyze Variability：运行 = %1，因子 = %2，重复 = %3。",
 "Analyze variability: runs = %1, factors = %2, replicates = %3."},
{"interp.analyze_variability_effects",
 "ln(s) 分散模型；2 水平效应 = 2×系数。",
 "ln(s) dispersion model; two-level effect = 2×coefficient."},
{"interp.analyze_variability_scope",
 "2 水平窄化；非 Taguchi 分析。",
 "Two-level narrow scope; not Taguchi analyze."},
{"interp.factor_analysis_summary",
 "因子分析：N = %1，变量 = %2，因子 = %3。",
 "Factor analysis: N = %1, variables = %2, factors = %3."},
{"interp.factor_analysis_loadings",
 "主成分提取载荷与 % Var；含 Scree 图。",
 "PCA-extraction loadings and % variance; includes scree plot."},
{"interp.factor_analysis_scope",
 "无 Hotelling T²；与 pca 命令区分。",
 "No Hotelling T²; distinct from pca command."},
};

}  // namespace

void append_report_text_catalog_part18(std::vector<ReportTextEntry>& out)
{
    out.insert(
        out.end(),
        std::begin(kReportTextCatalogPart18),
        std::end(kReportTextCatalogPart18));
}

}  // namespace datalab::domain
