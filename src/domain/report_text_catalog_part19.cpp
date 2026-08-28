#include "domain/report_text_catalog_parts.h"

namespace datalab::domain {
namespace {

const ReportTextEntry kReportTextCatalogPart19[] = {
{"interp.binary_response_doe_summary",
 "二值响应 DOE：设计行 = %1，展开 N = %2，事件 = %3，试验 = %4。",
 "Binary response DOE: design rows = %1, expanded N = %2, events = %3, trials = %4."},
{"interp.binary_response_doe_or",
 "Logit 系数与 OR = exp(β)；读 Coefficients 与 Odds Ratio 表。",
 "Logit coefficients and OR = exp(beta); read coefficient and odds ratio tables."},
{"interp.binary_response_doe_scope",
 "Logit IRWLS 窄化；禁止「过程已合格」。",
 "Logit IRWLS narrow scope; forbid process-qualified claims."},
{"interp.cluster_variables_summary",
 "变量聚类：N = %1，变量 = %2，合并 = %3，连结 = %4。",
 "Cluster variables: N = %1, variables = %2, merges = %3, linkage = %4."},
{"interp.cluster_variables_dendrogram",
 "d_ij = 1 − |ρ_ij|；amalgamation 步数 = p − 1。",
 "d_ij = 1 - |rho_ij|; amalgamation steps = p - 1."},
{"interp.cluster_variables_scope",
 "变量层次聚类；非 cluster_observations。",
 "Variable hierarchical clustering; not cluster_observations."},
{"interp.glm_three_factor_summary",
 "三因子 GLM：N = %1，平衡 = %2，AB/AC/BC 交互开关见 Facts。",
 "Three-factor GLM: N = %1, balanced = %2; see Facts for pairwise interactions."},
{"interp.glm_three_factor_fitted",
 "Fitted Means 为回归预测按水平平均；无 ABC 三阶交互。",
 "Fitted means average regression predictions; no ABC third-order interaction."},
{"interp.glm_three_factor_scope",
 "Type III 窄化；禁止「过程已合格」。",
 "Type III narrow scope; forbid process-qualified claims."},
{"interp.life_data_regression_summary",
 "寿命回归：N = %1，失败 = %2，删失 = %3，Shape = %4。",
 "Life regression: N = %1, failures = %2, censored = %3, shape = %4."},
{"interp.life_data_regression_coef",
 "Weibull MLE 回归表；log Y_p = β₀ + Σβ_k x_k + σΦ⁻¹(p)。",
 "Weibull MLE regression table; log Y_p = beta0 + sum beta_k x_k + sigma Phi^-1(p)."},
{"interp.life_data_regression_scope",
 "1～2 协变量窄化；非 accelerated_life。",
 "One-to-two covariate narrow scope; not accelerated_life."},
};

}  // namespace

void append_report_text_catalog_part19(std::vector<ReportTextEntry>& out)
{
    out.insert(
        out.end(),
        std::begin(kReportTextCatalogPart19),
        std::end(kReportTextCatalogPart19));
}

}  // namespace datalab::domain
