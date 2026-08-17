#include "application/analysis_catalog.h"

namespace datalab::application {

const std::vector<AnalysisDescriptor>& AnalysisCatalog::all()
{
    static const std::vector<AnalysisDescriptor> descriptors = {
        {"descriptive", "显示描述性统计", "统计"},
        {"normality_test", "正态性检验", "统计"},
        {"correlation", "相关分析", "统计"},
        {"one_sample_t", "单样本 t 检验", "统计"},
        {"two_sample_t", "双样本 t 检验", "统计"},
        {"one_way_anova", "单因素 ANOVA", "统计"},
        {"paired_t", "配对 t 检验", "统计"},
        {"regression", "线性回归", "统计"},
        {"two_proportions", "两比例检验", "统计"},
        {"chi_square", "列联表卡方", "统计"},
        {"mann_whitney", "Mann-Whitney 检验", "统计"},
        {"wilcoxon_signed_rank", "Wilcoxon 符号秩检验", "统计"},
        {"kruskal_wallis", "Kruskal-Wallis 检验", "统计"},
        {"box_cox", "Box-Cox 变换", "质量工具"},
        {"gage_rr", "Crossed Gage R&R", "质量工具"},
        {"msa_type1", "MSA Type 1 / Bias / Stability", "质量工具"},
        {"imr", "I-MR 控制图", "控制图"},
        {"xbar_r", "Xbar-R 控制图", "控制图"},
        {"xbar_s", "Xbar-S 控制图", "控制图"},
        {"p_chart", "P 图", "控制图"},
        {"np_chart", "NP 图", "控制图"},
        {"c_chart", "C 图", "控制图"},
        {"u_chart", "U 图", "控制图"},
        {"laney_p_chart", "Laney P' 图", "控制图"},
        {"laney_u_chart", "Laney U' 图", "控制图"},
        {"ewma", "EWMA 控制图", "控制图"},
        {"cusum", "CUSUM 控制图", "控制图"},
        {"time_series_smoothing", "时间序列平滑", "统计"},
        {"arima", "ARIMA 基础预测", "统计"},
        {"two_factor_anova", "双因素 ANOVA", "统计"},
        {"logistic_regression", "二元 Logistic 回归", "统计"},
        {"variance_test", "方差检验", "统计"},
        {"time_series_decomposition", "时间序列分解", "统计"},
        {"seasonal_forecasting", "季节性预测", "统计"},
        {"pca", "主成分分析", "统计"},
        {"doe_factorial", "2 水平全因子设计", "质量"},
        {"doe_response", "DOE 响应分析", "质量"},
        {"nested_gage_rr", "Nested Gage R&R", "质量"},
        {"attribute_agreement", "属性一致性分析", "质量"},
        {"reliability", "可靠性分析（Kaplan-Meier / Weibull）", "统计"},
        {"t_power", "t 功效与样本量", "统计"},
        {"capability", "正态过程能力", "质量工具"},
        {"capability_sixpack", "过程能力 Sixpack", "质量工具"},
        {"histogram", "直方图", "图形"},
        {"boxplot", "箱线图", "图形"},
        {"pareto", "柏拉图", "图形"}};
    return descriptors;
}

}  // namespace datalab::application
