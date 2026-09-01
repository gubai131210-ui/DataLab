#include "domain/report_text_catalog_parts.h"

namespace datalab::domain {
namespace {

const ReportTextEntry kReportTextCatalogPart21[] = {
{"interp.general_manova_summary",
 "General MANOVA：N = %1，响应 = %2，效应 = %3。",
 "General MANOVA: N = %1, responses = %2, effects = %3."},
{"interp.general_manova_tests",
 "Type III SSCP；λ_i 为 E^{-1}H 特征值；四检验 F 近似。",
 "Type III SSCP; lambda_i eigenvalues of E^{-1}H; four-test F approx."},
{"interp.general_manova_scope",
 "多因子/协变量 General MANOVA；非 manova_one_way 对话框。",
 "Multi-factor General MANOVA; not manova_one_way dialog."},
{"interp.mixed_effects_reml_summary",
 "混合 REML：N = %1，随机水平 = %2，σ² = %3，σ_u² = %4。",
 "Mixed REML: N = %1, random levels = %2, sigma2 = %3, sigma_u2 = %4."},
{"interp.mixed_effects_reml_method",
 "y = Xβ + Zμ + ε；REML 方差分量 + GLS 固定效应。",
 "y = Xb + Zmu + eps; REML VarComp + GLS fixed effects."},
{"interp.mixed_effects_reml_scope",
 "单随机项 REML 窄化；非单页 GLM 壳。",
 "Single random-term REML narrow scope; not one-page GLM shell."},
{"interp.binary_doe_probit_summary",
 "二值 DOE Probit：N = %1，link = %2，收敛 = %3。",
 "Binary DOE probit: N = %1, link = %2, converged = %3."},
{"interp.binary_doe_probit_irwls",
 "Probit Φ⁻¹(μ) 或 Gompit log(-log(1-μ))；IRWLS。",
 "Probit Phi^-1(mu) or Gompit log(-log(1-mu)); IRWLS."},
{"interp.binary_doe_probit_scope",
 "Probit/Gompit 窄化；非 binary_response_doe logit。",
 "Probit/Gompit narrow scope; not binary_response_doe logit."},
{"interp.life_data_lognormal_summary",
 "Lognormal 寿命：N = %1，失败 = %2，删失 = %3。",
 "Lognormal life: N = %1, failures = %2, censored = %3."},
{"interp.life_data_lognormal_mle",
 "log(T) ~ Normal；右删失 MLE；百分位 exp(μ + σΦ⁻¹(p))。",
 "log(T) ~ Normal; right-censoring MLE; percentiles exp(mu + sigma Phi^-1(p))."},
{"interp.life_data_lognormal_scope",
 "Lognormal 窄化；非 life_data_regression Weibull。",
 "Lognormal narrow scope; not life_data_regression Weibull."},
};

}  // namespace

void append_report_text_catalog_part21(std::vector<ReportTextEntry>& out)
{
    out.insert(
        out.end(),
        std::begin(kReportTextCatalogPart21),
        std::end(kReportTextCatalogPart21));
}

}  // namespace datalab::domain
