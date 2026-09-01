#include "domain/report_text_catalog_parts.h"

namespace datalab::domain {
namespace {

const ReportTextEntry kReportTextCatalogPart20[] = {
{"interp.expanded_gage_unbalanced_summary",
 "不平衡 Gage：N = %1，Part = %2，Operator = %3，平衡 = %4。",
 "Unbalanced Gage: N = %1, parts = %2, operators = %3, balanced = %4."},
{"interp.expanded_gage_unbalanced_varcomp",
 "VarComp 由 MS 估计；%Contribution = VarComp/Total；NDC 读 Facts。",
 "VarComp from MS; %Contribution = VarComp/Total; read NDC in Facts."},
{"interp.expanded_gage_unbalanced_scope",
 "不平衡 GLM 窄化；非 expanded_gage_rr 对话框。",
 "Unbalanced GLM narrow scope; not expanded_gage_rr dialog."},
{"interp.split_plot_analyze_summary",
 "裂区：N = %1，WP = %2，WP R² = %3。",
 "Split-plot: N = %1, WP = %2, WP R² = %3."},
{"interp.split_plot_analyze_errors",
 "F_HTC 分母 MS_WP_Error；F_ETC 分母 MS_SP_Error。",
 "F_HTC uses MS_WP_Error; F_ETC uses MS_SP_Error."},
{"interp.split_plot_analyze_scope",
 "裂区双误差窄化；非 doe_factorial 对话框。",
 "Split-plot dual-error narrow scope; not doe_factorial dialog."},
{"interp.mixture_process_variable_summary",
 "Mixture+过程：N = %1，q = %2，R² = %3。",
 "Mixture+process: N = %1, q = %2, R² = %3."},
{"interp.mixture_process_variable_scheffe",
 "Scheffé 无截距 OLS；Σ x_i ≈ 1 容差检查。",
 "Scheffé no-intercept OLS; sum-to-one tolerance check."},
{"interp.mixture_process_variable_scope",
 "1 过程变量窄化；非 mixture_analyze 对话框。",
 "One process variable narrow scope; not mixture_analyze dialog."},
{"interp.manova_one_way_summary",
 "MANOVA：N = %1，响应 = %2，组 = %3。",
 "MANOVA: N = %1, responses = %2, groups = %3."},
{"interp.manova_one_way_tests",
 "Wilks/Pillai/LH/Roy；λ_i 为 E^{-1}H 特征值。",
 "Wilks/Pillai/LH/Roy; lambda_i are eigenvalues of E^{-1}H."},
{"interp.manova_one_way_scope",
 "单因子 MANOVA 窄化；非 General MANOVA。",
 "One-way MANOVA narrow scope; not General MANOVA."},
};

}  // namespace

void append_report_text_catalog_part20(std::vector<ReportTextEntry>& out)
{
    out.insert(
        out.end(),
        std::begin(kReportTextCatalogPart20),
        std::end(kReportTextCatalogPart20));
}

}  // namespace datalab::domain
