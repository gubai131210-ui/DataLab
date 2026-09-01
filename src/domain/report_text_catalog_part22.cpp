#include "domain/report_text_catalog_parts.h"

namespace datalab::domain {
namespace {

const ReportTextEntry kReportTextCatalogPart22[] = {
{"interp.simple_correspondence_summary",
 "简单对应：N = %1，惯性 = %2，χ² = %3。",
 "Simple CA: N = %1, inertia = %2, chi-square = %3."},
{"interp.simple_correspondence_method",
 "I = χ²/n；SVD 主坐标；行/列贡献。",
 "I = chi2/n; SVD principal coordinates; row/column contributions."},
{"interp.simple_correspondence_scope",
 "2 列分类窄化；非 multiple_correspondence 对话框。",
 "Two-column CA narrow scope; not multiple_correspondence dialog."},
{"interp.multiple_correspondence_summary",
 "多重对应：N = %1，变量 = %2，惯性 = %3。",
 "MCA: N = %1, variables = %2, inertia = %3."},
{"interp.multiple_correspondence_method",
 "指示矩阵 Burt/MCA；Column Contributions。",
 "Indicator matrix MCA; column contributions."},
{"interp.multiple_correspondence_scope",
 "3～6 列分类；非 simple_correspondence 对话框。",
 "3-6 categorical columns; not simple_correspondence dialog."},
{"interp.nonlinear_regression_summary",
 "非线性回归：N = %1，模型 = %2，收敛 = %3。",
 "Nonlinear regression: N = %1, model = %2, converged = %3."},
{"interp.nonlinear_regression_method",
 "GN/LM 迭代；相对 offset 收敛判据。",
 "GN/LM iteration; relative offset convergence."},
{"interp.nonlinear_regression_scope",
 "内置模型窄化；非 linear_regression 对话框。",
 "Built-in models narrow scope; not linear_regression dialog."},
{"interp.split_plot_design_summary",
 "裂区设计：因子 = %1，Whole plots = %2，Runs = %3。",
 "Split-plot design: factors = %1, whole plots = %2, runs = %3."},
{"interp.split_plot_design_method",
 "2 水平 HTC/ETC；Whole plot 列；可接 split_plot_analyze。",
 "2-level HTC/ETC; whole plot column; feeds split_plot_analyze."},
{"interp.split_plot_design_scope",
 "设计生成窄化；非 split_plot_analyze 对话框。",
 "Design generation narrow scope; not split_plot_analyze dialog."},
};

}  // namespace

void append_report_text_catalog_part22(std::vector<ReportTextEntry>& out)
{
    out.insert(
        out.end(),
        std::begin(kReportTextCatalogPart22),
        std::end(kReportTextCatalogPart22));
}

}  // namespace datalab::domain
