#include "application/report_localization.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace datalab::application {
namespace {

std::string replace_token(std::string text, const std::string& token, const std::string& value)
{
    const auto pos = text.find(token);
    if (pos != std::string::npos) {
        text.replace(pos, token.size(), value);
    }
    return text;
}

std::string replace_all_tokens(
    std::string text, const std::vector<std::pair<std::string, std::string>>& tokens)
{
    // Longer tokens first so "%10" is not partially matched by "%1".
    std::vector<std::pair<std::string, std::string>> ordered = tokens;
    std::sort(
        ordered.begin(),
        ordered.end(),
        [](const auto& a, const auto& b) { return a.first.size() > b.first.size(); });
    for (const auto& [token, value] : ordered) {
        text = replace_token(std::move(text), token, value);
    }
    return text;
}

bool starts_with(const std::string& text, const std::string& prefix)
{
    return text.size() >= prefix.size()
        && text.compare(0, prefix.size(), prefix) == 0;
}

bool ends_with(const std::string& text, const std::string& suffix)
{
    return text.size() >= suffix.size()
        && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool report_language_is_english(const std::string& language)
{
    return language == "en" || language == "en-US" || language == "en_US"
        || language.rfind("en-", 0) == 0;
}

void replace_all_inplace(std::string& haystack, const std::string& from, const std::string& to)
{
    if (from.empty()) {
        return;
    }
    std::size_t pos = 0;
    while ((pos = haystack.find(from, pos)) != std::string::npos) {
        haystack.replace(pos, from.size(), to);
        pos += to.size();
    }
}

// en-US payload cleanup for Chinese list separators and DOE coded annotations.
std::string localize_zh_payload_punctuation(std::string text, const std::string& language)
{
    if (!report_language_is_english(language)) {
        return text;
    }
    replace_all_inplace(text, "、", ", ");
    const std::string coded_open = "（编码 ";
    const std::string coded_close = "）";
    std::size_t pos = 0;
    while ((pos = text.find(coded_open, pos)) != std::string::npos) {
        const auto end = text.find(coded_close, pos + coded_open.size());
        if (end == std::string::npos) {
            break;
        }
        const std::string value =
            text.substr(pos + coded_open.size(), end - (pos + coded_open.size()));
        const std::string replacement = "(coded " + value + ")";
        text.replace(pos, end + coded_close.size() - pos, replacement);
        pos += replacement.size();
    }
    return text;
}

std::string localize_known_plain_message(
    const std::string& message,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    static const std::pair<const char*, const char*> exact_ids[] = {
        {"建议合并相邻类别后复算；当前 P 值可作为探索性证据。",
         "interp.gof_rec_merge_adjacent"},
        {"期望频数过低，建议先调整分组（合并类别）再进行拟合优度检验。",
         "interp.gof_rec_regroup_low_expected"},
        {"存在期望频数小于 5 的类别，卡方近似可能不可靠。",
         "interp.gof_diag_expected_lt5_cats"},
        {"低期望频数比例偏高，建议合并类别并谨慎解释 P 值。",
         "interp.gof_diag_high_low_expected_share"},
        {"期望频数过低，卡方近似可靠性较差。",
         "interp.gof_diag_expected_too_low"},
        {"存在期望频数小于 1 的单元格，卡方近似 P 值不显示。",
         "interp.chi2_diag_expected_lt1_cells"},
        {"拟合优度至少需要两个类别，且类别与计数长度相同。",
         "interp.gof_err_need_two_categories"},
        {"各类观察计数必须为非负有限数，类别名不能为空。",
         "interp.gof_err_nonneg_counts_names"},
        {"期望比例个数必须与类别个数相同。",
         "interp.gof_err_proportion_length"},
        {"观察总计数必须大于 0。",
         "interp.gof_err_total_count_positive"},
        {"期望比例必须为正有限数。",
         "interp.gof_err_proportion_positive"},
        {"期望比例之和必须为 1。",
         "interp.gof_err_proportions_sum_one"},
        {"列联表至少需要两行两列，且标签数量必须匹配。",
         "interp.chi2_err_table_shape"},
        {"列联表每一行必须具有相同列数。",
         "interp.chi2_err_ragged_rows"},
        {"列联表单元格必须是非负有限计数。",
         "interp.chi2_err_nonneg_cells"},
        {"列联表总计数必须大于 0。",
         "interp.chi2_err_total_positive"},
        {"McNemar 要求两列配对标签等长且非空。",
         "interp.mcnemar_err_input_mismatch"},
        {"存在无法识别为二元水平的标签。",
         "interp.mcnemar_err_not_binary"},
        {"无不一致对（b+c=0），McNemar 统计量不可计算。",
         "interp.mcnemar_err_no_discordant"},
        {"二元编码至少需要两列配对标签。",
         "interp.binary_encode_err_need_columns"},
        {"配对二元列必须等长。",
         "interp.binary_encode_err_length_mismatch"},
        {"配对二元输入为空。",
         "interp.binary_encode_err_empty"},
        {"配对二元要求标签非空。",
         "interp.binary_encode_err_empty_label"},
        {"要求合计恰为两个水平，或可识别的二元编码（0/1、pass/fail 等）。",
         "interp.binary_encode_err_not_binary"},
        {"Cochran Q 输入为空。",
         "interp.cochran_err_empty"},
        {"Cochran Q 要求至少 3 个处理列；两列配对请用 McNemar。",
         "interp.cochran_err_use_mcnemar"},
        {"处理标签个数必须与列数一致。",
         "interp.cochran_err_label_mismatch"},
        {"Cochran Q 宽表每行处理数必须一致。",
         "interp.cochran_err_ragged"},
        {"Cochran Q 仅接受已编码的 0/1 值。",
         "interp.cochran_err_not_binary"},
        {"存在全失败或全成功的处理列，解读需谨慎。",
         "interp.cochran_warn_sparse_column"},
        {"受试者数 < 4，χ² 近似只作提示。",
         "interp.cochran_warn_small_n"},
        {"Cochran Q 分母为 0（行和无变异），统计量不可计算。",
         "interp.cochran_err_degenerate"},
        {"2×2 表总计数必须大于 0。",
         "interp.fisher_err_empty_table"},
        {"存在零单元格，优势比未定义（输出 *）。",
         "interp.fisher_warn_or_undefined"},
        {"事件数或非事件数小于 5，正态近似可能不准确；请参考 Fisher 精确检验。",
         "interp.two_prop_warn_small_count"},
        {"两比例检验要求非零分母，事件数不能超过试验数，且置信水平有效。",
         "interp.two_prop_err_counts"},
        {"比例标准误为 0，无法计算正态近似检验。",
         "interp.two_prop_err_zero_se"},
        {"假设比例标准误为 0，无法计算正态近似检验。",
         "interp.one_prop_err_zero_se"},
        {"单比例检验要求试验数大于 0，且事件数不能超过试验数。",
         "interp.one_prop_err_counts"},
        {"假设比例必须位于 (0, 1) 内。",
         "interp.one_prop_err_hypothesized"},
        {"假设比例必须在 0 与 1 之间。",
         "interp.one_prop_err_hypothesized_alt"},
        {"已将多行事件数/试验数求和后再做单比例检验。",
         "interp.one_prop_warn_summarized_rows"},
        {"n p0 或 n(1-p0) 小于 5，正态近似可能不准确。",
         "interp.one_prop_warn_small_np0"},
        {"已将多行缺陷数/观测长度求和后再做单样本泊松率检验。",
         "interp.poisson_warn_summarized_rows"},
        {"总发生数不大于 10，正态近似可能不准确。",
         "interp.poisson_warn_small_count"},
        {"两组总发生数不大于 10，正态近似可能不准确。",
         "interp.poisson_warn_small_count_two"},
        {"样本量过小，无法计算 McKean–Ryan 置信区间。",
         "interp.mann_whitney_warn_ci_small_n"},
        {"Mann–Whitney 检验要求两组均有有效观测。",
         "interp.mann_whitney_err_need_two_groups"},
        {"秩方差为 0，无法计算 Mann–Whitney P 值。",
         "interp.mann_whitney_err_zero_rank_variance"},
        {"存在样本量小于 10 的组，Mann–Whitney 正态近似只作提示。",
         "interp.mann_whitney_warn_small_sample"},
        {"Wilcoxon signed-rank 检验至少需要两个非零差值。",
         "interp.wilcoxon_err_need_two_nonzero"},
        {"非零差值少于 10，Wilcoxon 正态近似只作提示。",
         "interp.wilcoxon_warn_small_sample"},
        {"符号秩方差为 0，无法计算 Wilcoxon P 值。",
         "interp.wilcoxon_err_zero_rank_variance"},
        {"Wilcoxon signed-rank 检验要求两列行数相同。",
         "interp.wilcoxon_err_unequal_pairs"},
        {"Kruskal–Wallis 检验至少需要两个且标签匹配的组。",
         "interp.kruskal_err_invalid_groups"},
        {"Kruskal–Wallis 检验需要每个组至少一个有效观测。",
         "interp.kruskal_err_insufficient_obs"},
        {"存在少于 5 个观测的组，卡方近似可能不可靠。",
         "interp.kruskal_warn_small_group"},
        {"Friedman 检验要求响应、处理与区组等长且非空。",
         "interp.friedman_err_input_mismatch"},
        {"Friedman 检验要求数值响应与非空处理/区组标签。",
         "interp.friedman_err_invalid_row"},
        {"Friedman 检验至少需要 2 个处理与 2 个区组。",
         "interp.friedman_err_insufficient_levels"},
        {"同一区组与处理出现重复观测，Friedman 要求每格恰 1 个观测。",
         "interp.friedman_err_duplicate_cell"},
        {"存在缺失的区组×处理组合，Friedman 要求平衡设计。",
         "interp.friedman_err_unbalanced"},
        {"符号检验在丢弃等于假设中位数的观测后无有效符号。",
         "interp.sign_err_no_nonzero"},
        {"有效符号数 ≥ 25；主 P 仍为二项精确，正态近似仅作参考。",
         "interp.sign_warn_large_n_note"},
        {"中位数置信区间需要至少一个有限观测。",
         "interp.sign_ci_err_no_data"},
        {"n<2 时无法计算符号检验中位数置信区间。",
         "interp.sign_ci_warn_insufficient_n"},
        {"无法构造合法的符号检验中位数置信区间。",
         "interp.sign_ci_warn_unavailable"},
        {"配对符号检验要求两列等长。",
         "interp.sign_err_pair_mismatch"},
        {"Mood 中位数检验至少需要两个且标签匹配的组。",
         "interp.mood_err_invalid_groups"},
        {"观测数少于 2 的组已从 Mood 检验中排除。",
         "interp.mood_warn_group_dropped"},
        {"排除小组后不足两个组，无法计算 Mood 中位数检验。",
         "interp.mood_err_insufficient_groups"},
        {"Mood 中位数检验有效观测不足。",
         "interp.mood_err_insufficient_observations"},
        {"无法计算总体中位数。",
         "interp.mood_err_median_undefined"},
        {"N≤ 或 N> 一侧为 0，Mood χ² 不可计算。",
         "interp.mood_err_degenerate_table"},
        {"存在期望频数 < 5 的单元格，χ² 近似需谨慎解读。",
         "interp.mood_warn_expected_count"},
        {"存在样本量较小的组，Mood χ² 近似只作提示。",
         "interp.mood_warn_small_sample"},
        {"游程检验至少需要 2 个有限观测。",
         "interp.runs_err_insufficient_n"},
        {"指定比较准则时必须提供有限的 K。",
         "interp.runs_err_missing_criterion"},
        {"所有观测都在比较准则的同一侧，无法进行游程检验。",
         "interp.runs_err_one_sided_empty"},
        {"两侧观测少于 10，正态近似可能不稳定。",
         "interp.runs_warn_normal_thin"},
        {"游程方差不可计算。",
         "interp.runs_err_variance_invalid"},
        {"Run Chart 需要至少一个有限观测。",
         "interp.run_chart_err_empty"},
        {"N < 3，仅输出图形与描述，不出随机性近似 P。",
         "interp.run_chart_warn_small_n"},
        {"全部点落在中位数同一侧，关于中位数的随机性 P 不可用。",
         "interp.run_chart_warn_median_degenerate"},
        {"存在相邻相等点：按产品锁定计入下行游程（平坦差分归下行）。",
         "interp.run_chart_warn_flat_as_down"},
        {"方差分量原始估计为负，已截断为 0。",
         "interp.gage_warn_neg_var_truncated"},
        {"公差必须为有限非负数；NaN、无穷或负数不可用。",
         "interp.gage_err_invalid_tolerance"},
        {"Gage R&R 要求测量值、零件和操作员列长度一致且至少有四条记录。",
         "interp.gage_err_shape"},
        {"测量值、零件和操作员标签必须有效。",
         "interp.gage_err_invalid_row"},
        {"Crossed Gage R&R 至少需要两个零件和两个操作员。",
         "interp.gage_err_insufficient_levels"},
        {"每个零件与操作员组合至少需要两次重复测量。",
         "interp.gage_err_insufficient_replicates"},
        {"每个零件×操作员组合必须具有相同的重复次数。",
         "interp.gage_err_unbalanced_replicates"},
        {"Part×Operator 交互 p>0.25，传统 AIAG 流程可考虑将交互并入重复性；当前结果保留完整交互模型，不自动缩减。",
         "interp.gage_warn_interaction_pooling"},
        {"ndc<5 只作为调查提示，不是量具不合格的绝对结论。",
         "interp.gage_warn_ndc_lt5"},
        {"Gage 标准差为 0，ndc 不可估计。",
         "interp.gage_err_ndc_not_estimable"},
        {"所有测量值相同，无法估计 Gage R&R 方差分量。",
         "interp.gage_err_zero_total_variation"},
        {"公差必须为有限非负数。",
         "interp.nested_gage_err_invalid_tolerance"},
        {"Nested Gage R&R 要求长度一致、至少四条记录。",
         "interp.nested_gage_err_shape"},
        {"Nested Gage R&R 中每个零件必须只属于一个操作员。",
         "interp.nested_gage_err_part_multi_operator"},
        {"Nested Gage R&R 至少需要两个操作员和两个零件。",
         "interp.nested_gage_err_insufficient_levels"},
        {"每个零件至少需要两次重复测量。",
         "interp.nested_gage_err_insufficient_replicates"},
        {"每个零件必须具有相同的重复次数。",
         "interp.nested_gage_err_unbalanced_replicates"},
        {"每个操作员必须至少有一个零件。",
         "interp.nested_gage_err_empty_operator"},
        {"每个操作员必须分配相同数量的零件。",
         "interp.nested_gage_err_unbalanced_operator"},
        {"ndc<5 只作为调查提示。",
         "interp.nested_gage_warn_ndc_lt5"},
        {"所有测量值相同，无法估计 Nested Gage R&R 方差分量。",
         "interp.nested_gage_err_zero_total_variation"},
        {"测量值与因子列长度必须一致。",
         "interp.expanded_gage_err_length_mismatch"},
        {"三因子 Expanded Gage 需要足够观测。",
         "interp.expanded_gage_err_insufficient_data"},
        {"测量值必须有限。",
         "interp.expanded_gage_err_non_finite"},
        {"Part、Operator 与附加因子各自至少需要 2 个水平。",
         "interp.expanded_gage_err_insufficient_levels"},
        {"本轮 Expanded 仅支持完整平衡交叉；缺失格子请用交叉 Gage 或后续 GLM。",
         "interp.expanded_gage_err_incomplete_design"},
        {"各格子重复次数必须相同。",
         "interp.expanded_gage_err_unbalanced_replicates"},
        {"缺少重复。",
         "interp.expanded_gage_err_no_replicates"},
        {"重复次数=1：三阶交互用作重复性误差估计。",
         "interp.expanded_gage_warn_three_way_as_error"},
        {"本命令为平衡三因子随机 Expanded；不平衡/固定效应/嵌套 GLM 仍延后。",
         "interp.expanded_gage_info_scoped"},
        {"Type 1 Gage 至少需要两条有限测量值，参考值和公差必须有效。",
         "interp.msa_type1_err_input"},
        {"测量值必须为有限数。",
         "interp.msa_type1_err_non_finite"},
        {"重复性为零时 t、p 和置信区间不可用，不输出 p=0。",
         "interp.msa_type1_err_zero_repeatability"},
        {"Bias/Linearity 要求至少三组且参考值与测量值长度一致。",
         "interp.bias_lin_err_shape"},
        {"参考值和测量值必须为有限数。",
         "interp.bias_lin_err_non_finite"},
        {"参考值必须包含至少两个不同水平。",
         "interp.bias_lin_err_zero_ref_range"},
        {"过程变差必须为有限正数（6×过程标准差）。",
         "interp.bias_lin_err_process_variation"},
        {"Stability 至少需要三条测量值。",
         "interp.gage_stab_err_input"},
        {"请选择测量值列，并在配置中提供参考值。",
         "interp.msa_type1_err_need_measurement_col"},
        {"请选择参考值列。",
         "interp.bias_lin_err_need_reference_col"},
        {"没有可用于 Bias/Linearity 的 complete-case 行。",
         "interp.bias_lin_err_no_complete_rows"},
        {"未提供过程变差（6×过程标准差），Linearity / %Linearity / %Bias 未计算。",
         "interp.bias_lin_info_no_process_variation"},
        {"请输入参考值。",
         "interp.msa_type1_err_need_reference_value"},
        {"请选择测量值列。",
         "interp.msa_err_need_measurement_col"},
        {"请选择评级、部件和评估者列。",
         "interp.attr_agree_err_need_columns"},
        {"加权 Kappa 需要可排序的数值评级；已回退未加权 Cohen。",
         "interp.attr_agree_warn_weighted_unranked"},
        {"有序评级必须全部为数值，才能计算 Kendall；未使用字典序。",
         "interp.attr_agree_warn_kendall_need_numeric"},
        {"Kendall 系数需要至少三个有序等级；当前只输出 Kappa。",
         "interp.attr_agree_warn_kendall_need_levels"},
        {"评估者间 Kendall W 不可识别；不伪造 W=1。",
         "interp.attr_agree_warn_kendall_w_unidentifiable"},
        {"部分零件评级众数平票，对应评估者×零件一致率未计算。",
         "interp.attr_agree_warn_mode_tie"},
        {"未知的 kappa_weight_scheme，已回退为 none。",
         "interp.attr_agree_warn_unknown_weight_scheme"},
        {"属性一致性要求列长度一致，标准列为空或与记录数一致，置信度必须在 0 与 1 之间。",
         "interp.attr_agree_err_shape"},
        {"项目和评估者标签不能为空。",
         "interp.attr_agree_err_empty_labels"},
        {"属性一致性至少需要两个项目和两个评估者。",
         "interp.attr_agree_err_insufficient_levels"},
        {"存在缺失评级；相关配对只使用双方均有评级的项目。",
         "interp.attr_agree_warn_missing_ratings"},
        {"部分项目未被某些评估者评级，评估者间比较将排除这些项目。",
         "interp.attr_agree_warn_incomplete_items"},
        {"部分项目缺少标准评级，相关评估者-标准比较将排除这些项目。",
         "interp.attr_agree_warn_missing_standards"},
        {"评估者重复次数不一致；不等长配对已排除，不会静默截断。",
         "interp.attr_agree_warn_unequal_replicates"},
        {"Fleiss overall Kappa 保持未加权；linear/quadratic 仅用于两两 Cohen。",
         "interp.attr_agree_info_fleiss_unweighted"},
        {"期望一致率 P_expected=1，Kappa 不可识别，不计算无限标准误。",
         "interp.attr_agree_warn_kappa_unidentifiable"},
        {"回归至少需要两个观测、一个响应列和一个预测列。",
         "interp.reg_err_shape"},
        {"置信水平必须为 (0,1) 内的有限数。",
         "interp.reg_err_confidence"},
        {"预测变量数量和标签数量必须有效且一致。",
         "interp.reg_err_predictor_labels"},
        {"每行预测变量必须具有相同列数。",
         "interp.reg_err_ragged_predictors"},
        {"回归观测数必须不少于参数个数。",
         "interp.reg_err_insufficient_n"},
        {"误差自由度 N-p-1 ≤ 0，不输出 t、F 与 P。",
         "interp.reg_warn_no_error_df"},
        {"响应变量必须全部为有限数值。",
         "interp.reg_err_invalid_response"},
        {"预测变量必须全部为有限数值。",
         "interp.reg_err_invalid_predictor"},
        {"设计矩阵存在完全共线或常量预测变量。",
         "interp.reg_err_collinear_design"},
        {"设计矩阵秩亏，已拒绝拟合。",
         "interp.reg_err_rank_deficient"},
        {"秩亏时不要解释系数；先检查常量列或完全共线预测变量。",
         "interp.reg_rec_rank_deficient"},
        {"无法计算回归系数协方差矩阵。",
         "interp.reg_err_singular_cov"},
        {"VIF>5 提示共线性调查，不会自动删除预测变量。",
         "interp.reg_warn_vif_gt5"},
        {"多预测变量回归应同时查看 Seq SS 与 Adj SS；两者不一致常见于不平衡或共线设计。",
         "interp.reg_warn_seq_adj_ss"},
        {"Durbin-Watson 临界界不可用（需要 15≤n≤100 且 1≤k'≤5）。",
         "interp.reg_warn_dw_bounds"},
        {"Durbin-Watson 按输入顺序计算；n 或回归元数超出 α=0.05 界表范围，未给 dL/dU。",
         "interp.reg_assume_dw_out_of_table"},
        {"Durbin-Watson 按输入顺序；判定区对照 α=0.05 近似 dL/dU，不能写成已证明无自相关。",
         "interp.reg_assume_dw_approx"},
        {"Anderson-Darling 只能在 alpha 下拒绝或未拒绝残差正态假设。",
         "interp.reg_assume_ad_residual"},
        {"残差正态性未计算。",
         "interp.reg_assume_normality_missing"},
        {"方差齐性主要依据残差对拟合值图，当前不单独给出数值判定。",
         "interp.reg_assume_homoscedasticity"},
        {"增加独立观测或减少模型项后再解释系数显著性。",
         "interp.reg_rec_need_error_df"},
        {"误差自由度为正，系数推断可用。",
         "interp.reg_rule_error_df_ok"},
        {"仍需结合残差图和影响点解释模型。",
         "interp.reg_rec_use_residual_plots"},
        {"当前设计矩阵可估计。",
         "interp.reg_rule_design_estimable"},
        {"存在删除学生化残差绝对值大于 3 的观测。",
         "interp.reg_rule_outlier_triggered"},
        {"没有 |删除学生化残差| > 3 的观测。",
         "interp.reg_rule_outlier_clear"},
        {"存在杠杆值超过 2p/n 的高杠杆点。",
         "interp.reg_rule_leverage_triggered"},
        {"Cook's D 或 DFITS 超过常用调查阈值。",
         "interp.reg_rule_influence_triggered"},
        {"回查原始行；解释层不会自动删除这些观测。",
         "interp.reg_rec_review_outliers"},
        {"仍应检查残差图，不能据此宣称没有异常。",
         "interp.reg_rec_check_residuals"},
        {"高杠杆点需要调查，不自动删除。",
         "interp.reg_rec_investigate_leverage"},
        {"影响点提示需要调查，解释层不会自动删除这些观测。",
         "interp.reg_rec_investigate_influence"},
        {"至少一个预测变量 VIF>5。",
         "interp.reg_rule_vif_triggered"},
        {"未发现 VIF>5 的预测变量。",
         "interp.reg_rule_vif_clear"},
        {"VIF>5 只提示共线性调查，不会自动删除变量。",
         "interp.reg_rec_vif_investigate"},
        {"单因素 ANOVA 至少需要两个有效组。",
         "interp.anova_err_need_two_groups"},
        {"组标签数量必须与有效组数量一致。",
         "interp.anova_err_label_count"},
        {"ANOVA 不允许存在没有有效观测的组。",
         "interp.anova_err_empty_group"},
        {"组内误差平方和为 0，无法计算有限 F 统计量。",
         "interp.anova_err_zero_error_ss"},
        {"误差自由度或组内平方和为 0，不输出伪造 F/P。",
         "interp.anova_rule_estimability_zero_error"},
        {"增加组内重复或检查是否所有观测完全相同。",
         "interp.anova_rec_add_replicates"},
        {"ANOVA 残差正态性只能拒绝或未拒绝假设，不能证明组内误差正态。",
         "interp.anova_assume_residual_normality"},
        {"Levene 检验只作为方差齐性调查证据，不能单独决定模型是否可用。",
         "interp.anova_assume_levene_homogeneity"},
        {"误差自由度大于 0，F/P 可计算。",
         "interp.anova_rule_error_df_ok"},
        {"总体 F 显著只说明至少一组均值不同，需要多重比较。",
         "interp.anova_rec_need_posthoc"},
        {"Levene 检验提供了组方差不同的证据。",
         "interp.anova_rule_levene_against"},
        {"当前 Levene 检验未提供组方差不同的证据。",
         "interp.anova_rule_levene_clear"},
        {"方差不齐时应结合图形和非参数替代方法，而不是直接宣称 ANOVA 无效。",
         "interp.anova_rec_heteroscedasticity"},
        {"两个因子和响应变量必须具有相同且非零的行数。",
         "interp.twoway_anova_err_shape"},
        {"含缺失因子或响应值的观测已从分析中排除。",
         "interp.twoway_anova_warn_omitted"},
        {"双因素 ANOVA 需要两个因子各至少两个水平及至少三个观测。",
         "interp.twoway_anova_err_levels"},
        {"观测数较少；部分效应可能不可估计或误差自由度不足。",
         "interp.twoway_anova_warn_small_n"},
        {"因子组合存在空单元，交互项可能秩亏。",
         "interp.twoway_anova_warn_empty_cells"},
        {"因子组合的重复数不平衡；Seq SS 与 Adj SS 可能不同。",
         "interp.twoway_anova_warn_unbalanced"},
        {"设计矩阵秩亏；不可估计的项不输出伪造 F/P。",
         "interp.twoway_anova_warn_rank_deficient"},
        {"误差自由度为 0，无法计算 F 与 P。",
         "interp.twoway_anova_warn_no_error_df"},
        {"双因素 ANOVA 残差来自单元均值；拒绝正态假设只是调查证据。",
         "interp.twoway_anova_assume_residual_normality"},
        {"单元重复数足够时用 Levene 检验作为方差齐性证据。",
         "interp.twoway_anova_assume_levene"},
        {"单元重复不足，无法计算方差齐性检验。",
         "interp.twoway_anova_assume_homogeneity_missing"},
        {"不平衡设计应同时报告 Seq SS 与 Adj SS，不要只看其中一个。",
         "interp.twoway_anova_rec_report_seq_adj"},
        {"误差自由度或 MSE 不可用，未输出组均值区间。",
         "interp.anova_info_interval_unavailable"},
        {"请选择因子/分组列。",
         "interp.anova_err_need_factor_col"},
        {"请选择响应变量、因子 A 和因子 B。",
         "interp.twoway_anova_err_need_columns"},
        {"因子组合重复数平衡。",
         "interp.twoway_anova_rule_balanced"},
        {"因子组合不平衡，Sequential SS 与 Adjusted SS 可能不同。",
         "interp.twoway_anova_rule_unbalanced"},
        {"存在不可估计项或无误差自由度，不输出伪造 F/P。",
         "interp.twoway_anova_rule_estimability_triggered"},
        {"当前效应可估计且误差自由度为正。",
         "interp.twoway_anova_rule_estimability_clear"},
        {"不可估计项必须显示原因，不能填入默认 F/P。",
         "interp.twoway_anova_rec_show_reason"},
        {"Tukey 比较至少需要两个且标签数量必须匹配的有效组。",
         "interp.tukey_err_invalid_groups"},
        {"置信水平必须大于 0 且小于 1。",
         "interp.tukey_err_confidence"},
        {"Tukey 比较不允许空组。",
         "interp.tukey_err_empty_group"},
        {"Tukey 比较不允许无穷或非数值观测。",
         "interp.tukey_err_non_finite"},
        {"Tukey 比较需要正的组内误差自由度。",
         "interp.tukey_err_error_df"},
        {"当前 Tukey 调整使用 Studentized range 的保守 t 分布近似。",
         "interp.tukey_warn_approx"},
        {"不要把逐比较 alpha 当成家族错误率。",
         "interp.tukey_rec_family_error"},
        {"生成器格式应为 D=ABC;E=ABD。",
         "interp.doe_err_generator_format"},
        {"生成器左侧应为单个因子字母。",
         "interp.doe_err_generator_left"},
        {"生成器引用了不存在的因子字母。",
         "interp.doe_err_generator_unknown_letter"},
        {"生成器右侧乘积无法解析。",
         "interp.doe_err_generator_product"},
        {"生成器右侧不能包含被生成因子。",
         "interp.doe_err_generator_self_ref"},
        {"生成器应定义最后 p 个附加因子。",
         "interp.doe_err_generator_not_extra"},
        {"生成器数量必须等于部分析因 p。",
         "interp.doe_err_generator_count"},
        {"每个附加因子都需要生成器。",
         "interp.doe_err_generator_incomplete"},
        {"DOE 至少需要一个因子。",
         "interp.doe_err_empty_factors"},
        {"区组数必须大于零。",
         "interp.doe_err_block_count"},
        {"部分析因 p 必须小于因子数 k。",
         "interp.doe_err_fraction_p"},
        {"每个 DOE 因子必须有名称、低水平和高水平。",
         "interp.doe_err_incomplete_factor"},
        {"DOE 因子名称必须唯一。",
         "interp.doe_err_duplicate_factor_name"},
        {"该 (k,p) 无内置默认生成器，请在 generators 中手写。",
         "interp.doe_err_no_default_generators"},
        {"至少需要一个连续因素。",
         "interp.rsd_err_no_factors"},
        {"因素 ID 必须唯一。",
         "interp.rsd_err_duplicate_factor_id"},
        {"第一阶段只接受连续因素；分类因素不得静默编码。",
         "interp.rsd_err_categorical_blocked"},
        {"因素低水平必须严格小于高水平。",
         "interp.rsd_err_invalid_bounds"},
        {"CCD 至少需要 2 个连续因素。",
         "interp.ccd_err_min_factors"},
        {"CCC 星点超出原始因素范围；请允许超范围星点，或改用 CCI/CCF。",
         "interp.ccd_err_ccc_beyond_range"},
        {"CCC 星点超出原始 low/high；已按允许超范围策略生成，实验可行性需人工确认。",
         "interp.ccd_info_ccc_beyond_allowed"},
        {"BBD 第一阶段支持 3–7 个连续因素；2 因素不接受。",
         "interp.bbd_err_factor_count"},
        {"BBD 不包含所有因素同时处于极端水平的角点；这是设计空间边界，不是实现缺陷。",
         "interp.bbd_info_no_corners"},
        {"没有重复运行，无法估计纯误差和失拟。",
         "interp.doe_warn_no_pure_error"},
        {"纯误差占用了全部残差自由度，无法检验失拟。",
         "interp.doe_warn_lof_df_zero"},
        {"中心点与角点均存在，但没有可用纯误差无法检验曲率。",
         "interp.doe_warn_curvature_no_pe"},
        {"设计没有中心点，未执行曲率检验。",
         "interp.doe_info_no_curvature_test"},
        {"请选择响应列和至少一个设计因子列。",
         "interp.doe_err_need_response_factors"},
        {"没有可用于响应分析的有效 DOE 运行。",
         "interp.doe_err_no_valid_runs"},
        {"请提供至少一个连续因素。",
         "interp.rsd_err_need_continuous_factor"},
        {"存在缺少有效因子水平的运行，已跳过。",
         "interp.doe_warn_missing_run"},
        {"已按 PointType/中心水平导入中心点运行，可用于曲率与纯误差。",
         "interp.doe_info_center_imported"},
        {"因子值均在 [-1,1]，按已编码单位拟合 RSM。",
         "interp.rsm_factors_already_coded"},
        {"至少一个因子无变异，无法编码。",
         "interp.rsm_factor_no_variation"},
        {"因子已按列内 min/max 线性编码到 [-1,1]。",
         "interp.rsm_factors_minmax_coded"},
        {"设计编码边界与因子数不匹配。",
         "interp.rsm_design_bounds_incomplete"},
        {"设计低/高水平必须有限且 low < high。",
         "interp.rsm_design_bounds_invalid"},
        {"设计中心必须落在 [low, high] 内。",
         "interp.rsm_design_center_invalid"},
        {"因子已按设计 low/high/center 编码（与 CCD/BBD 一致）。",
         "interp.rsm_factors_design_bounds_coded"},
        {"RSM 需要响应与因子行数一致，且至少两个因子。",
         "interp.rsm_err_shape"},
        {"每行因子列数必须一致。",
         "interp.rsm_err_ragged_factors"},
        {"检测到重复编码点，已用于纯误差 / 失拟估计。",
         "interp.rsm_replicated_coded_points"},
        {"没有重复编码点，无法估计纯误差和失拟；不得用残差 MS 冒充纯误差。",
         "interp.rsm_insufficient_pure_error"},
        {"失拟 ANOVA 证据类型 formula_reference；不是 vendor_oracle。",
         "interp.rsm_lof_formula_reference_only"},
        {"纯误差自由度大于残差自由度，跳过失拟分解（可能模型过参或重复点过多）。",
         "interp.rsm_pure_error_exceeds_residual_df"},
        {"等值线需要两个不同因子且模型已拟合。",
         "interp.rsm_err_invalid_grid"},
        {"寿命和删失指示列长度必须一致且至少包含两条记录。",
         "interp.rel_err_shape"},
        {"寿命必须为有限正数。",
         "interp.rel_err_time_positive"},
        {"全为删失，无法识别 Weibull 参数。",
         "interp.weibull_err_all_censored"},
        {"只有一条失效记录时 Weibull 形状参数通常不可稳定识别。",
         "interp.weibull_err_one_failure"},
        {"失效时间没有足够变化，无法有限地识别 Weibull 形状。",
         "interp.weibull_err_no_variation"},
        {"置信水平必须在 0 和 1 之间。",
         "interp.rel_err_confidence"},
        {"全为删失，无法识别失效分布或中位寿命。",
         "interp.km_err_all_censored"},
        {"最大观测为删失，尾部生存函数不可估计到 0。",
         "interp.km_warn_tail_not_to_zero"},
        {"Log-rank 检验要求寿命、事件和分组列长度一致。",
         "interp.logrank_err_shape"},
        {"Log-rank 时间必须为正数，分组必须编码为 0 或 1。",
         "interp.logrank_err_values"},
        {"Log-rank 检验至少需要两个非空分组。",
         "interp.logrank_err_two_groups"},
        {"分组失效模式没有可估计的 Log-rank 方差。",
         "interp.logrank_err_zero_variance"},
        {"三参数 Weibull 至少需要三次失效才能识别阈值。",
         "interp.weibull3_err_few_failures"},
        {"失效时间没有足够变化，无法识别三参数 Weibull。",
         "interp.weibull3_err_no_variation"},
        {"三参数 Weibull 似然无界（常见于形状 ≤ 1）；不估计阈值，也不伪造参数。",
         "interp.weibull3_err_unbounded"},
        {"未能找到形状大于 1 的有限三参数 Weibull 估计。",
         "interp.weibull3_err_no_interior"},
        {"指数模型至少需要一条失效记录。",
         "interp.exp_err_need_failure"},
        {"两参数指数至少需要一条失效记录。",
         "interp.exp2_err_need_failure"},
        {"失效时间没有足够变化，两参数指数似然无界；不估计阈值，也不伪造参数。",
         "interp.exp2_err_unbounded_variation"},
        {"两参数指数未能找到有限阈值估计；不伪造参数。",
         "interp.exp2_err_no_threshold"},
        {"全为删失，无法识别对数正态参数。",
         "interp.lognormal_err_all_censored"},
        {"只有一条失效记录时对数正态尺度通常不可稳定识别。",
         "interp.lognormal_err_one_failure"},
        {"三参数对数正态至少需要两次失效才能识别阈值。",
         "interp.lognormal3_err_few_failures"},
        {"失效时间没有足够变化，三参数对数正态似然无界；不估计阈值，也不伪造参数。",
         "interp.lognormal3_err_unbounded_variation"},
        {"三参数对数正态似然无界；不估计阈值，也不伪造参数。",
         "interp.lognormal3_err_unbounded"},
        {"未能找到有限的三参数对数正态估计。",
         "interp.lognormal3_err_no_interior"},
        {"Hosmer–Lemeshow 在样本量不足、未收敛或完全分离时不计算。",
         "interp.logistic_hl_not_computed"},
        {"Hosmer–Lemeshow 分组期望方差过小，检验不可用。",
         "interp.logistic_hl_low_group_variance"},
        {"Hosmer–Lemeshow 有效组数不足，检验不可用。",
         "interp.logistic_hl_too_few_groups"},
        {"二元 Logistic 回归至少需要三个观测和一个预测变量。",
         "interp.logistic_err_need_obs_predictor"},
        {"置信水平、收敛容差和最大迭代次数必须有效。",
         "interp.logistic_err_invalid_options"},
        {"预测变量标签数量必须与预测变量列数一致。",
         "interp.logistic_err_label_count"},
        {"响应变量必须全部为 0 或 1。",
         "interp.logistic_err_binary_response"},
        {"Logistic 回归需要多于参数数量的观测。",
         "interp.logistic_err_need_more_obs_than_params"},
        {"Logistic 信息矩阵秩亏，无法进行 IRLS 更新。",
         "interp.logistic_err_rank_deficient_info"},
        {"线性预测量达到数值稳定性边界，概率已进行安全截断。",
         "interp.logistic_warn_eta_clipped"},
        {"IRLS 在最大迭代次数内未达到收敛容差。",
         "interp.logistic_warn_irls_not_converged"},
        {"预测变量完全分离了 0/1 响应，极大似然估计可能不存在。",
         "interp.logistic_warn_complete_separation"},
        {"预测概率达到边界，模型可能存在准分离。",
         "interp.logistic_warn_quasi_separation"},
        {"无法计算 Logistic 系数协方差矩阵。",
         "interp.logistic_err_cov_failed"},
        {"Poisson 回归至少需要三个观测和一个预测变量。",
         "interp.poisson_reg_err_need_obs_predictor"},
        {"响应必须为非负有限值，且预测矩阵规整。",
         "interp.poisson_reg_err_invalid_response"},
        {"预测变量含非有限值。",
         "interp.poisson_reg_err_nonfinite_predictor"},
        {"IRLS 信息矩阵奇异。",
         "interp.poisson_reg_err_singular_info"},
        {"已达最大迭代次数。",
         "interp.glm_warn_max_iterations"},
        {"无法计算系数协方差。",
         "interp.poisson_reg_warn_cov_failed"},
        {"逐步回归需要 ≥4 观测与 ≥2 候选预测变量。",
         "interp.stepwise_err_need_obs_candidates"},
        {"α_enter / α_remove 必须在 (0,1)。",
         "interp.stepwise_err_alpha_range"},
        {"α_remove < α_enter 可能导致振荡；仍继续。",
         "interp.stepwise_warn_alpha_order"},
        {"未选入任何预测变量（仅截距）。",
         "interp.stepwise_warn_intercept_only"},
        {"有序 Logistic 需要 ≥10 观测、≥3 有序水平与 ≥1 预测变量。",
         "interp.ordinal_err_need_obs_levels"},
        {"响应类别编码越界。",
         "interp.ordinal_err_bad_category_code"},
        {"有序 Logistic 信息矩阵奇异。",
         "interp.ordinal_err_singular_info"},
        {"检测到 1 点超出 3σ 控制限，建议复核该观测、测量和记录过程。",
         "interp.spc_rule1_beyond_3sigma"},
        {"检测到连续 9 点位于中心线同侧，建议复核阶段、设备或批次因素。",
         "interp.spc_rule2_nine_same_side"},
        {"检测到连续 6 点单调上升或下降，建议复核趋势、刀具磨损或过程漂移。",
         "interp.spc_rule3_six_trend"},
        {"检测到连续 14 点上下交替，建议复核系统性周期或两台设备交替影响。",
         "interp.spc_rule4_fourteen_alternating"},
        {"检测到 3 点中有 2 点同侧超过 2σ，提示可能存在较小的过程偏移。",
         "interp.spc_rule5_two_of_three_2sigma"},
        {"检测到 5 点中有 4 点同侧超过 1σ，提示可能存在较小的过程偏移。",
         "interp.spc_rule6_four_of_five_1sigma"},
        {"检测到连续 15 点落在 1σ 以内，控制限可能过宽或数据存在分层。",
         "interp.spc_rule7_fifteen_within_1sigma"},
        {"检测到连续 8 点落在 1σ 以外，提示可能存在混合总体或双群模式。",
         "interp.spc_rule8_eight_beyond_1sigma"},
        {"I-MR 不允许 NaN 或无穷观测。",
         "interp.imr_err_non_finite"},
        {"MSSD 需要至少一对相继差分。",
         "interp.imr_err_mssd_need_pair"},
        {"MSSD 无偏常数 c4' 使用近似式，不是完整 Minitab 表查表。",
         "interp.imr_warn_mssd_c4_approx"},
        {"Nelson estimate 仅适用于平均移动极差法；当前 MSSD 方法已忽略该选项。",
         "interp.imr_warn_nelson_ignored_mssd"},
        {"Nelson estimate 剔除后无剩余移动极差，回退到未剔除估计。",
         "interp.imr_warn_nelson_fallback"},
        {"Nelson estimate 仅适用于平均移动极差法；当前方法已忽略该选项。",
         "interp.imr_warn_nelson_ignored_method"},
        {"Xbar-R 要求各组样本量相等，不能用最后一组的 d2/A2 代替。",
         "interp.xbar_r_err_unequal_n"},
        {"Xbar-R 子组不允许 NaN 或无穷观测。",
         "interp.xbar_r_err_non_finite"},
        {"I-MR-R/S 至少需要两个子组。",
         "interp.imr_rs_err_need_two_subgroups"},
        {"各子组必须至少包含两个观测。",
         "interp.spc_err_subgroup_min_two"},
        {"I-MR-R/S 子组不允许 NaN 或无穷观测。",
         "interp.imr_rs_err_non_finite"},
        {"无法计算 I-MR-R/S 控制图。",
         "diag.imr_rs.cannot_compute_chart"},
        {"估计的组间方差为负，已截断为 0；σ_B 可能低估。",
         "interp.spc_warn_neg_between_var"},
        {"EWMA 要求非空数据、lambda 位于 (0,1] 且控制限倍数大于 0。",
         "interp.ewma_err_options"},
        {"EWMA 的过程标准差必须大于 0。",
         "interp.ewma_err_sigma"},
        {"CUSUM 要求非空数据、sigma/h 大于 0 且 k 不小于 0。",
         "interp.cusum_err_options"},
        {"区域图至少需要 2 个观测。",
         "interp.zone_err_need_two"},
        {"区域图不允许非有限观测。",
         "interp.zone_err_non_finite"},
        {"区域得分采用 Jaehn 1/2/4 权重、累计阈值 8（formula_reference），不是 Minitab 自定义权重 golden。",
         "interp.zone_warn_jaehn_scoring"},
        {"未提供完整历史 μ/σ；本轮用各组样本均值与全序列 MR/d2 估计 σ。",
         "interp.zmr_warn_estimated_limits"},
        {"CUSUM 不使用 Shewhart 特殊原因规则（beyond_control_limit 等），改用上/下侧累计和首次信号。",
         "interp.cusum_info_not_shewhart_rules"},
        {"删失工作表为空。",
         "interp.censoring_worksheet_empty"},
        {"删失工作表需要 censoring_type 与 time 列。",
         "interp.censoring_worksheet_missing_columns"},
        {"interval 行需要 interval_left / interval_right 列。",
         "interp.censoring_worksheet_missing_interval_bounds"},
        {"删失契约至少需要一条观测。",
         "interp.censoring_empty"},
        {"时间必须为有限非负数；负时间阻止分析。",
         "interp.censoring_negative_or_nonfinite_time"},
        {"出现时间为 0 的失效；请确认单位与记录口径。",
         "interp.censoring_zero_failure_time"},
        {"区间删失边界必须为有限非负数。",
         "interp.censoring_interval_nonfinite"},
        {"区间删失要求左端严格小于右端；反向区间阻止。",
         "interp.censoring_interval_reversed"},
        {"暴露量必须为有限非负数。",
         "interp.censoring_invalid_exposure"},
        {"观测时间单位不一致，阻止合并分析。",
         "interp.censoring_time_unit_conflict"},
        {"零失效（全删失或无 exact 事件）；生存/参数估计可能不可识别。",
         "interp.censoring_zero_failures"},
        {"全部为失效事件，无删失。",
         "interp.censoring_all_failures"},
        {"全部删失；中位寿命等点估计通常不可得。",
         "interp.censoring_all_censored"},
        {"删失契约证据类型 formula_reference；右删失不得当作失效。",
         "interp.censoring_formula_reference"},
        {"保修窗口 T_w 必须为正有限数。",
         "interp.warranty_invalid_window"},
        {"保修摘要需要明确时间单位。",
         "interp.warranty_missing_time_unit"},
        {"R(T_w) 必须落在 [0,1]。",
         "interp.warranty_invalid_reliability"},
        {"输入为全删失；预测摘要仍可计算，但不得宣称为观察失效率。",
         "interp.warranty_all_censored_input"},
        {"claims/1000 = 1000*(1-R(T_w))；证据类型 formula_reference，非 vendor_oracle。",
         "interp.warranty_formula_reference"},
        {"当前摘要标记为 prediction，不得与观察失效计数混读为同一口径。",
         "interp.warranty_prediction_label"},
        {"部分 failure_mode 层使用 cause-specific R(T_w)，其余仍用池化 R；证据类型 formula_reference，不是 vendor_oracle。",
         "interp.warranty_strata_mixed_reliability"},
        {"分层 expected_failures = 层暴露量 * F_mode(T_w)，F_mode 来自 cause-specific 分模式拟合（竞争失效作右删失）；formula_reference，不是 vendor_oracle。",
         "interp.warranty_strata_mode_specific_reliability"},
        {"分模式拟合当前仅支持二参数 Weibull / Lognormal / Exponential / KM；阈值模型请用总体拟合。",
         "interp.mode_fit_threshold_model_unsupported"},
        {"无带 failure_mode 标签的 exact 失效，跳过分模式拟合。",
         "interp.mode_fit_no_labeled_failures"},
        {"分模式可靠度 = cause-specific：目标模式 exact 为失效，其他已标注模式的 exact 作为右删失，原始 right 仍为右删失；evidence_type=formula_reference，algorithm_id=cause_specific_right_censored_competing；不是 vendor_oracle。",
         "interp.mode_fit_cause_specific_scope"},
        {"事件列只接受明确的失效/删失编码；未知值不会被静默当作删失。",
         "interp.rel_invalid_event_value"},
        {"删失类型列无法解析，或与事件列冲突；未知值不会被静默改写。left/interval 若出现在数据中，将由删失契约拒绝经典 KM 路径。",
         "interp.rel_invalid_censoring_type_value"},
        {"删失契约校验失败。",
         "interp.censoring_contract_failed"},
        {"需要至少一行多元观测。",
         "interp.hotelling_err_empty_matrix"},
        {"Hotelling T² 至少需要两个变量列。",
         "interp.hotelling_err_need_multivariate"},
        {"个体 T² Phase I 需要 m > p+1。",
         "interp.hotelling_err_phase1_m_gt_p_plus_1"},
        {"source_rows 长度必须匹配观测数。",
         "interp.hotelling_err_source_row_mismatch"},
        {"无法估计均值/协方差（非有限或不矩形）。",
         "interp.hotelling_err_invalid_matrix"},
        {"样本协方差奇异，无法计算 T²。",
         "interp.hotelling_err_singular_covariance"},
        {"本命令是正式多元 Hotelling T² 控制图，不是 PCA 经验分位 T²。",
         "interp.hotelling_info_not_pca_empirical_t2"},
        {"MEWMA 至少需要两个变量列。",
         "interp.mewma_err_need_multivariate"},
        {"MEWMA 至少需要 3 个观测。",
         "interp.mewma_err_need_three_obs"},
        {"无法估计均值/协方差。",
         "interp.mewma_err_invalid_matrix"},
        {"观测矩阵必须矩形。",
         "interp.mewma_err_ragged_matrix"},
        {"MEWMA 协方差在当前步奇异。",
         "interp.mewma_err_singular_step_cov"},
        {"默认 UCL 使用渐近 χ² 近似，不是仿真 ARL 校准常数；可手工指定 ucl。",
         "interp.mewma_warn_ucl_not_arl_calibrated"},
        {"MEWMA 是多元向量平滑；不要与单变量 EWMA 图混淆。",
         "interp.mewma_info_not_univariate_ewma"},
        {"需要至少一个子组。",
         "interp.gv_err_empty_subgroups"},
        {"子组不能为空。",
         "interp.gv_err_empty_subgroup"},
        {"GV 图至少需要两个变量。",
         "interp.gv_err_need_multivariate"},
        {"广义方差图要求每个子组大小 n > 变量数 p。",
         "interp.gv_err_subgroup_too_small"},
        {"广义方差图要求等量子组。",
         "interp.gv_err_unequal_subgroups"},
        {"子组观测必须具有相同变量数。",
         "interp.gv_err_ragged_subgroup"},
        {"b1/b2 常数无效。",
         "interp.gv_err_invalid_b_constants"},
        {"子组含非有限值。",
         "interp.gv_err_non_finite"},
        {"广义方差图按 Montgomery |S| 子组公式；个体观测路径不做假 |S|。",
         "interp.gv_info_montgomery_subgroup"},
        {"NIST 指出多元变差图存在争议；本输出仅作 |S| 探索信号，不是唯一变差判定。",
         "interp.gv_warn_variability_chart_caveat"},
        {"重复性方差为 0，Probable Error 为 0；请检查设计。",
         "interp.emp_warn_zero_repeatability"},
        {"EMP 分级基于 Wheeler ICC，不是 AIAG %Study Var 合格判定。",
         "interp.emp_info_not_aiag_pass_fail"},
        {"无带 failure_mode 标签的 exact 失效，跳过 Aalen–Johansen CIF。",
         "interp.cif_info_no_labeled_failures"},
        {"累计发生函数 CIF = Aalen–Johansen（formula_reference / aalen_johansen_cif）：总体生存把任一标注失效当作事件；CIF_k 为原因 k 的累计发生概率。不是 Fine-Gray 多协变量回归，不是 cause-specific（竞争删失）可靠度，不是 vendor_oracle。二分类 group 的 Fine-Gray 另有门禁路径。",
         "interp.cif_info_aalen_johansen_scope"},
        {"Aalen–Johansen CIF 本身不是 Fine-Gray 回归；不得把 CIF 表写成 Fine-Gray 或商业软件对齐。",
         "interp.cif_warn_not_fine_gray"},
        {"不得把本 Fine-Gray 结果写成商业软件对齐或 pinned R survival::finegray 黄金标准。",
         "interp.fine_gray_warn_not_commercial_alignment"},
        {"Fine-Gray 信息量退化，未能估计 β。",
         "interp.fine_gray_err_singular_information_beta"},
        {"Fine-Gray β 迭代发散。",
         "interp.fine_gray_err_beta_diverged"},
        {"Fine-Gray 未在迭代上限内收敛。",
         "interp.fine_gray_err_not_converged"},
        {"Fine-Gray 信息量不可逆，未能估计 SE。",
         "interp.fine_gray_err_singular_information_se"},
        {"Fine-Gray 协变量矩阵行数与观测不一致。",
         "interp.fine_gray_err_covariate_row_mismatch"},
        {"Fine-Gray 需要带 failure_mode 的 exact 失效。",
         "interp.fine_gray_err_need_labeled_failures"},
        {"Fine-Gray 协变量矩阵行长度不一致。",
         "interp.fine_gray_err_ragged_covariate_matrix"},
        {"fine_gray_multi 需要至少两个协变量；单列请用 fine_gray_continuous。",
         "interp.fine_gray_err_need_two_covariates"},
        {"Fine-Gray 连续协变量长度与观测不一致。",
         "interp.fine_gray_err_continuous_length_mismatch"},
        {"目标原因失效只出现在一个分组中；估计仍可运行但对比识别较弱。",
         "interp.fine_gray_warn_target_one_group_only"},
        {"已指定多协变量列：Fine-Gray 使用 multi IPCW，不与二分类 group Fine-Gray 同时运行。",
         "interp.fine_gray_info_multi_priority"},
        {"已指定连续协变量列：Fine-Gray 使用 continuous IPCW，不与二分类 group Fine-Gray 同时运行。",
         "interp.fine_gray_info_continuous_priority"},
        {"Isolation Forest 需要非空矩阵与 tree_count ≥ 1。",
         "interp.iforest_err_empty"},
        {"至少需要 2 个观测与 2 个变量。",
         "interp.iforest_err_need_2x2"},
        {"自研 Isolation Forest；多元异常辅助；非单变量 Grubbs/Dixon；非 TreeNet。",
         "interp.iforest_scope"},
        {"K-Means 需要 k ≥ 2。",
         "interp.kmeans_err_k_lt2"},
        {"K-Means 需要至少一行有效数值观测。",
         "interp.kmeans_err_empty"},
        {"有效观测数必须 ≥ k。",
         "interp.cluster_err_n_lt_k"},
        {"初始质心取前 k 个有效观测（分析尺度）；Lloyd 迭代（分配→更新质心）。",
         "interp.kmeans_init_lloyd"},
        {"已达最大迭代次数，分配可能尚未完全稳定。",
         "interp.kmeans_warn_max_iter"},
        {"CART 需要预测矩阵与响应对齐且非空。",
         "interp.cart_err_empty"},
        {"至少需要一个数值预测变量。",
         "interp.cart_err_no_predictors"},
        {"有效观测过少，无法按 min_leaf 分裂。",
         "interp.cart_err_insufficient_for_min_leaf"},
        {"自研 CART 单树；非 Minitab TreeNet/Random Forests 数值对齐；本轮无成本复杂度剪枝。",
         "interp.cart_scope_producer"},
        {"线性判别需要 ≥2 类、≥1 预测变量与足够观测。",
         "interp.lda_err_invalid"},
        {"类别编码或预测行列不一致。",
         "interp.lda_err_bad_row"},
        {"每个类至少需要 2 个观测。",
         "interp.lda_err_small_class"},
        {"合并协方差自由度不足。",
         "interp.lda_err_pooled_df"},
        {"合并协方差奇异。",
         "interp.lda_err_singular_pooled"},
        {"线性判别（等协方差）；非 Minitab golden；不做 QDA。",
         "interp.lda_scope_producer"},
        {"层次聚类需要非空矩阵且 k≥2。",
         "interp.hclust_err_invalid"},
        {"Complete linkage + 欧氏距离；非 Minitab golden。",
         "interp.hclust_linkage_scope"},
        {"观测不足（n<30），无法做二维高斯混合 EM；不得把不足样本写成已排除混合。",
         "interp.mixture2_err_insufficient_n"},
        {"二维高斯混合 EM 密度退化，未能收敛；不得伪造混合拟合。",
         "interp.mixture2_err_density_degenerate"},
        {"二维高斯混合 EM 未在迭代上限内收敛；不得把未收敛结果写成已确认混合。",
         "interp.mixture2_err_not_converged"},
        {"观测不足（n<30），无法做多 k 高斯混合 BIC 搜索；不得把不足样本写成已排除混合。",
         "interp.mixture_k_err_insufficient_n"},
        {"多 k 高斯混合 BIC 搜索未得到可用拟合；不得伪造混合结论。",
         "interp.mixture_k_err_search_failed"},
        {"能力指标未验证过程稳定性和正态性；数值仅供调查，不能单独作为过程合格结论。",
         "interp.cap_assumption_stability_normality"},
        {"能力指标未验证过程稳定性；数值仅供调查，不能单独作为过程合格结论。",
         "interp.cap_assumption_stability_only"},
        {"观测不足，无法做能力分析前的 I-MR 稳定性初筛。",
         "interp.cap_stability_insufficient_n"},
        {"I-MR Rule-1 初筛未检出超限点；这不是完整控制图验收，也不等于已验证稳定性/正态性，不得自动开放合格判定。",
         "interp.capability_stability_clear_not_verified"},
        {"能力分析稳定性前置：当前仅做公式参考级 I-MR Rule-1 初筛；合格判定保持关闭，直至独立稳定性与正态性验收工作流落地。",
         "interp.capability_stability_prerequisite"},
        {"观测不足（n<30），无法做能力分析前的双峰直方图初筛；不得把不足样本写成已排除双峰。",
         "interp.cap_bimodality_insufficient_n"},
        {"直方图双峰初筛未检出可分离峰；这不是单峰证明，不得据此开放过程合格判定。",
         "interp.cap_bimodality_clear_not_proof"},
        {"能力分析双峰前置：当前为公式参考级直方图峰谷初筛；Hartigan dip 与二维高斯混合另有门禁筛查；合格判定保持关闭。",
         "interp.cap_bimodality_screen_note"},
        {"观测不足（n<8），无法做 Hartigan dip 单峰门禁筛查；不得把不足样本写成已证明单峰。",
         "interp.cap_hartigan_insufficient_n"},
        {"Hartigan dip 门禁筛查未拒绝 Uniform 零假设下的单峰（formula_reference）；这不是过程单峰证明，也不得开放合格判定。",
         "interp.cap_hartigan_consistent_not_proof"},
        {"能力分析 Hartigan dip 前置：公式参考级研究筛查（Uniform 零假设 MC）；不是商业软件对齐；二维高斯混合另有门禁；合格判定保持关闭。",
         "interp.cap_hartigan_screen_note"},
        {"观测不足（n<30），无法做多 k 高斯混合门禁；不得把不足样本写成已排除混合。",
         "interp.cap_mixture_insufficient_n"},
        {"多 k 高斯混合门禁未得到可用拟合；不得伪造混合结论或开放合格判定。",
         "interp.cap_mixture_failed"},
        {"多 k 高斯混合门禁未优选多成分（formula_reference / gaussian_mixture_k_bic）；这不是单峰/单成分证明，也不得开放合格判定。",
         "interp.cap_mixture_not_preferred"},
        {"能力分析混合模型前置：高斯 EM + BIC 搜索 k=1..k_max（formula_reference / gaussian_mixture_k_bic）；不是非高斯混合，不是商业软件对齐，合格判定保持关闭。",
         "interp.cap_mixture_screen_note"},
        {"Johnson 能力为研究/预览结果：可显示拟合与变换尺度指数，但不得输出过程合格判定（门禁：缺 golden/尾部/人工解释验收）。",
         "interp.johnson_capability_gated"},
        {"正态能力未满足稳定性/正态性验收前置：禁止过程合格判定（pass_fail_judgment_allowed=false）。",
         "interp.cap_pass_fail_blocked_stability"},
        {"规格限落在 Johnson 变换定义域外，无法计算 Pp/Ppk。",
         "interp.johnson_spec_outside_support"},
        {"至少一个规格限落在变换定义域外；越界侧的百分位回推未实现，仅输出仍可变换规格的 overall 指数。",
         "interp.johnson_spec_partial_outside"},
        {"Johnson 变换路径只报告 overall Pp/Ppk，不报告 within Cp/Cpk。",
         "interp.johnson_within_not_applicable"},
        {"Johnson 变换至少需要 8 个有限观测。",
         "interp.johnson_err_need_eight"},
        {"样本量不足以估计 Johnson 分位匹配所需的尾部分位数。",
         "interp.johnson_warn_tail_quantile"},
        {"Johnson 变换按 Chou et al. (1998) 分位匹配与 AD p 值选择；数值是公式参考，不是 Minitab Individual Distribution Identification 导出。",
         "interp.johnson_formula_reference"},
        {"非正态能力使用拟合分布 CDF 的 Z-score 法计算 Pp/Ppk；不报告 Cp/Cpk。数值是公式参考，不是 Minitab 导出。",
         "interp.cap_nonnormal_zscore_scope"},
        {"Box-Cox 变换至少需要两个有效观测。",
         "interp.box_cox_err_need_two"},
        {"Box-Cox 变换要求所有观测严格大于 0。",
         "interp.box_cox_err_positive"},
        {"Box-Cox lambda 必须位于 [-5, 5]。",
         "interp.box_cox_err_lambda_range"},
        {"n<8 时正态性近似较粗糙，未拒绝正态假设不能当作已验证正态。",
         "interp.normality_warn_n_lt8"},
        {"二项过程能力未验证独立性、恒定 p 与稳定性，不能写成过程合格。",
         "interp.attr_binomial_assumption_static"},
        {"泊松过程能力未验证独立性、恒定 DPU 与稳定性，不能写成过程合格。",
         "interp.attr_poisson_assumption_static"},
        {"组间/组内能力至少需要两个子组。",
         "interp.cap_between_within_need_two_subgroups"},
        {"无法计算组间/组内能力指标。",
         "diag.capability.between_within_cannot_compute"},
        {"散点图至少需要两个有效的完整观测行。",
         "diag.scatter_need_two_complete"},
        {"区间图存在有效观测数少于 2 的分组，无法估计标准误。",
         "diag.interval_group_lt2_se"},
        {"相关图至少需要选择两个连续变量。",
         "diag.correlogram_need_two_vars"},
        {"正态概率图至少需要三个有效观测。",
         "diag.normal_prob_need_three"},
        {"矩阵图至少需要两个连续变量。",
         "diag.matrix_need_two_vars"},
        {"平行坐标图至少需要两个连续变量。",
         "diag.parallel_need_two_vars"},
        {"密度估计至少需要两个有限观测。",
         "diag.density_need_two_finite"},
        {"二维分箱至少需要两个 complete-case 点。",
         "diag.hexbin_need_two_complete"},
        {"ndc 不可估计；不能据此评价测量系统分辨力。",
         "rule.msa.ndc_not_computed.message"},
        {"先检查零总变异、零 Gage 标准差或无效容差后再解释 ndc。",
         "rule.msa.ndc_not_computed.action"},
        {"ndc < 5，提示测量系统对零件间差异的分辨力需要调查。",
         "rule.msa.ndc_lt5.message"},
        {"ndc 小于 5 只是调查提示，不是量具不合格的绝对结论。",
         "rule.msa.ndc_lt5.action"},
        {"ndc ≥ 5；这只说明当前研究中零件间变异相对 Gage 变异较大。",
         "rule.msa.ndc_ge5.message"},
        {"仍需结合 %Study Var、%Tolerance 和现场公差风险解释。",
         "rule.msa.ndc_ge5.action"},
        {"交叉设计单元重复次数平衡。",
         "rule.msa.design_balance.message"},
        {"平衡设计是 ANOVA 方差分量解释的前提。",
         "rule.msa.design_balance.action"},
        {"零件×操作员单元重复次数不一致。",
         "rule.msa.design_balance_unbalanced.message"},
        {"交叉设计需要平衡重复后才能解释方差分量。",
         "rule.msa.design_balance_unbalanced.action"},
        {"嵌套设计零件重复次数不一致。",
         "rule.msa.nested_design_balance_unbalanced.message"},
        {"先补齐平衡重复后再解释方差分量。",
         "rule.msa.nested_design_balance_unbalanced.action"},
        {"嵌套设计在操作员和零件重复上平衡。",
         "rule.msa.nested_design_balance_ok.message"},
        {"嵌套设计不平衡。",
         "rule.msa.nested_design_balance_triggered.message"},
        {"每个零件只属于一个操作员，且重复次数应一致。",
         "rule.msa.nested_design_balance.action"},
        {"存在负方差分量，已截断为 0。",
         "rule.msa.nested_negative_variance_triggered.message"},
        {"解释 %Contribution 时同时查看截断前的原始方差分量。",
         "rule.msa.nested_negative_variance.action"},
        {"交互项 p>0.25，可考虑缩减，但当前保留完整模型。",
         "rule.msa.interaction_triggered.message"},
        {"当前保留 Part×Operator 交互模型。",
         "rule.msa.interaction_ok.message"},
        {"交互是否缩减必须回显；本实现不自动并入重复性。",
         "rule.msa.interaction.action"},
        {"存在负方差分量，已截断为 0，并保留原始估计。",
         "rule.msa.negative_variance_triggered.message"},
        {"方差分量原始估计均非负。",
         "rule.msa.negative_variance_ok.message"},
        {"截断后的分量用于 %Contribution；解释时同时查看原始值。",
         "rule.msa.negative_variance.action"},
        {"%Contribution 基于方差，%Study Var 基于标准差，口径不同。",
         "rule.msa.percent_metrics.message"},
        {"不要把 %Contribution 与 %Study Var 当成同一个百分比。",
         "rule.msa.percent_metrics.action"},
        {"未提供有效公差，%Tolerance 不可用。",
         "rule.msa.invalid_tolerance.message"},
        {"只有有限正公差才能计算 %Tolerance。",
         "rule.msa.invalid_tolerance.action"},
        {"拒绝 Kappa=0 不等于已证明评估者一致。",
         "rule.aa.kappa_interpretation.message"},
        {"Kappa 只描述超出偶然的绝对一致率，不能写成测量系统合格。",
         "rule.aa.kappa_interpretation.action"},
        {"Weighted Kappa 是 DataLab 可选 Cohen 加权，不是 Minitab AAA 默认输出。",
         "rule.aa.weighted_kappa.message"},
        {"Minitab 有序评级路径使用 Kendall；不要把加权 κ 写成 Minitab AAA 结果。",
         "rule.aa.weighted_kappa.action"},
        {"有序评级已计算 Kendall W/τ；拒绝系数为 0 不等于已证明有序一致。",
         "rule.aa.kendall_ok.message"},
        {"已请求有序评级，但 Kendall 不可识别或等级不足。",
         "rule.aa.kendall_unavailable.message"},
        {"不要把 Kendall 写成加权 Kappa，也不要把未拒绝原假设写成已证明一致。",
         "rule.aa.kendall.action"},
        {"设计平衡", "rule.msa.catalog.design_balance.name"},
        {"ndc 调查", "rule.msa.catalog.ndc_investigation.name"},
        {"ndc = truncate(1.41×PartStDev/GageStDev)，<5 只作调查提示。",
         "rule.msa.catalog.ndc_investigation.message"},
        {"误差自由度", "rule.regression.catalog.error_df.name"},
        {"秩亏/共线", "rule.regression.catalog.rank_deficiency.name"},
        {"N-p-1 必须为正才能输出 t、F 与 P。",
         "rule.regression.catalog.error_df.message"},
        {"设计矩阵秩亏时拒绝拟合，不输出伪造推断。",
         "rule.regression.catalog.rank_deficiency.message"},
        {"残差正态性", "rule.regression.catalog.residual_normality.name"},
        {"Anderson-Darling 只能拒绝或未拒绝正态假设。",
         "rule.regression.catalog.residual_normality.message"},
        {"残差独立性", "rule.regression.catalog.residual_independence.name"},
        {"Durbin-Watson 对照 α=0.05 近似 dL/dU 判定区；不能写成已证明无自相关。",
         "rule.regression.catalog.residual_independence.message"},
        {"方差齐性", "rule.regression.catalog.homoscedasticity.name"},
        {"残差对拟合值图是主要证据，不单独宣称已验证。",
         "rule.regression.catalog.homoscedasticity.message"},
        {"高杠杆", "rule.regression.catalog.leverage.name"},
        {"杠杆值 > 2p/n 时标记为需要调查的高杠杆点。",
         "rule.regression.catalog.leverage.message"},
        {"异常残差", "rule.regression.catalog.outlier.name"},
        {"|删除学生化残差| > 3 时标记为异常点调查。",
         "rule.regression.catalog.outlier.message"},
        {"影响点", "rule.regression.catalog.influence.name"},
        {"Cook's D > 4/n 或 |DFITS| 超过阈值时标记影响点。",
         "rule.regression.catalog.influence.message"},
        {"共线性", "rule.regression.catalog.collinearity.name"},
        {"VIF > 5 提示共线性调查，不自动删除变量。",
         "rule.regression.catalog.collinearity.message"},
        {"可估计性", "rule.anova.catalog.estimability.name"},
        {"秩亏或无误差自由度时不输出伪造 F/P。",
         "rule.anova.catalog.estimability.message"},
        {"用残差 Anderson-Darling 作为假设证据。",
         "rule.anova.catalog.residual_normality.message"},
        {"Levene/组内标准差比较只作为调查证据。",
         "rule.anova.catalog.homogeneity.message"},
        {"家族错误率", "rule.anova.catalog.family_error_rate.name"},
        {"Tukey 必须回显同时置信水平和调整后 p 值。",
         "rule.anova.catalog.family_error_rate.message"},
        {"不平衡设计", "rule.anova.catalog.unbalanced_design.name"},
        {"不平衡时 Sequential SS 与 Adjusted SS 可能不同。",
         "rule.anova.catalog.unbalanced_design.message"},
        {"零件×操作员单元重复次数必须一致。",
         "rule.msa.catalog.design_balance.message"},
        {"负方差分量", "rule.msa.catalog.negative_variance.name"},
        {"保留截断前后方差分量，截断后用于 %Contribution。",
         "rule.msa.catalog.negative_variance.message"},
        {"交互项模型", "rule.msa.catalog.interaction_model.name"},
        {"报告 Part×Operator 显著性，不自动缩减模型。",
         "rule.msa.catalog.interaction_model.message"},
        {"百分比口径", "rule.msa.catalog.percent_metrics.name"},
        {"%Contribution 与 %Study Var 不可混用。",
         "rule.msa.catalog.percent_metrics.message"},
        {"无效容差", "rule.msa.catalog.invalid_tolerance.name"},
        {"无有效公差时不计算 %Tolerance。",
         "rule.msa.catalog.invalid_tolerance.message"},
        {"零重复性", "rule.msa.catalog.zero_repeatability.name"},
        {"Type 1 零重复性不输出伪造 p=0 推断。",
         "rule.msa.catalog.zero_repeatability.message"},
        {"事件编码", "rule.reliability.catalog.event_encoding.name"},
        {"只接受明确的失效/删失语义，拒绝未知编码。",
         "rule.reliability.catalog.event_encoding.message"},
        {"时间合法性", "rule.reliability.catalog.time_validity.name"},
        {"寿命必须为有限正数。", "rule.reliability.catalog.time_validity.message"},
        {"风险集", "rule.reliability.catalog.risk_set.name"},
        {"同一时刻多个失效应合并，并报告 at-risk/failures/censored。",
         "rule.reliability.catalog.risk_set.message"},
        {"可识别性", "rule.reliability.catalog.identifiability.name"},
        {"全删失或最大观测删失时尾部指标可能不可估计。",
         "rule.reliability.catalog.identifiability.message"},
        {"参数收敛", "rule.reliability.catalog.convergence.name"},
        {"Weibull 必须回显收敛、边界命中和估计方法。",
         "rule.reliability.catalog.convergence.message"},
        {"双样本等价性检验跳过缺失或非法单元格。",
         "interp.eq_two_sample_skip_missing"},
        {"均值比等价性检验跳过缺失或非法单元格。",
         "interp.eq_ratio_skip_missing"},
        {"配对等价性检验按 complete-case 对齐，已跳过不成对的缺失或非法单元格。",
         "interp.eq_paired_cc_skip_unpaired"},
        {"两比例等价性检验跳过了缺失或非法单元格。",
         "interp.eq_two_prop_skip_missing"},
        {"回归使用 complete-case，跳过响应缺失、预测变量缺失或无效的行。",
         "interp.reg_cc_skip_incomplete_rows"},
        {"两比例检验跳过了缺失或非法单元格。",
         "interp.two_prop_skip_missing"},
        {"序列中间存在缺失或非法值，游程检验要求完整连续观测，未计算检验。",
         "interp.runs_err_interior_missing"},
        {"平滑跳过缺失或无效的数值单元格。",
         "interp.smooth_skip_missing_cells"},
        {"Logistic 回归使用 complete-case，缺失或非法行已排除。",
         "interp.logistic_cc_excluded_rows"},
        {"缺失、* 或非法数值未进入容差区间计算。",
         "interp.tol_skip_missing_star_invalid"},
        {"时间列与值列未配对的行已跳过，分解仅使用可对齐的完整观测。",
         "interp.decomp_skip_unpaired_time_value"},
        {"分解跳过缺失或无效的时间/数值单元格。",
         "interp.decomp_skip_missing_time_value"},
        {"Nested Gage R&R 跳过缺失或非法的测量/零件/操作者单元格。",
         "interp.nested_gage_skip_missing_cells"},
        {"序列中间存在缺失或非法值，运行图随机性检验要求完整连续观测，未计算四模式 P。",
         "interp.run_chart_err_interior_missing"},
        {"跳过了缺失或非法单元格。",
         "interp.skip_missing_invalid_cells"},
        {"包含缺失或非有限值的观测行已被排除。",
         "interp.pca_warn_missing_row_excluded"},
        {"缺失或非有限响应未参与 DOE 效应摘要。",
         "interp.doe_warn_missing_response_effects"},
        {"缺失响应运行未参与拟合。",
         "interp.doe_warn_missing_response_fit"},
        {"某分组观测少于 2，已跳过密度形状。",
         "interp.violin_skip_small_group_density"},
        {"没有可用于 Multi-Vari 图的 complete-case 观测。",
         "interp.multi_vari_err_no_cc"},
        {"Target 必须为有限数，不能静默跳过。",
         "interp.cap_err_target_finite_no_silent_skip"},
        {"无法估计过程 σ（移动极差）。", "interp.zone_err_sigma_mr"},
        {"过程 σ 必须大于 0。", "interp.zone_err_sigma_positive"},
        {"无法估计 σ。", "interp.zmr_err_sigma"},
        {"Z-MR 至少需要 2 个观测。", "interp.zmr_err_insufficient_n"},
        {"Z-MR 不允许非有限观测。", "interp.zmr_err_non_finite"},
        {"存在组 σ ≤ 0。", "interp.zmr_err_group_sigma"},
        {"移动平均图不允许非有限观测。", "interp.ma_err_non_finite"},
        {"移动平均图需要至少 w 个观测。", "interp.ma_err_insufficient_n"},
        {"无法估计过程 σ。", "interp.ma_err_sigma"},
        {"σ 与控制限倍数必须大于 0。", "interp.ma_err_sigma_limit"},
        {"G 图至少需要两个有限间隔。", "interp.g_err_insufficient_points"},
        {"G 图间隔必须为非负有限数。", "interp.g_err_invalid_interval"},
        {"无法从间隔均值估计几何分布参数 p。", "interp.g_err_invalid_probability"},
        {"T 图间隔必须为非负有限数。", "interp.t_err_invalid_interval"},
        {"T 图至少需要两个正间隔。", "interp.t_err_insufficient_points"},
        {"Weibull 参数无法识别，T 图控制限未计算。", "interp.t_err_weibull_failed"},
        {"存在 0 间隔，已排除后用 log-log 回归估计 Weibull 参数。",
         "interp.t_warn_zero_interval_regression"},
        {"0 间隔回归无法估计 Weibull 参数。", "interp.t_err_regression_failed"},
        {"0 间隔回归得到非正形状参数。", "interp.t_err_regression_nonpositive_shape"},
        {"区间图没有可用于计算的有效分组。", "diag.graph.interval_no_valid_groups"},
        {"气泡大小不能为负数。", "diag.graph.negative_bubble_size"},
        {"气泡图没有可用于绘制的完整观测行。", "diag.graph.no_valid_bubbles"},
        {"经验累积分布图至少需要一个有效观测。", "diag.graph.ecdf_need_one"},
        {"热图没有可用于聚合的完整观测。", "diag.graph.heatmap_no_valid_cells"},
        {"存在重复时间点；图形按时间排序后保留全部观测。", "diag.graph.duplicate_time"},
        {"时间间隔不规则；图形按时间排序，但不把间隔当作等距。", "diag.graph.irregular_interval"},
        {"时间序列图至少需要两个有效观测。", "diag.graph.time_series_need_two"},
        {"等值线图需要完整的规则 X/Y/Z 网格。", "diag.graph.contour_irregular_grid"},
        {"饼图权重不能为负数。", "diag.graph.negative_pie_weight"},
        {"饼图各类别合计必须大于 0。", "diag.graph.pie_zero_total"},
        {"未找到有效的类别-原因行。请提供非空类别列与原因列。", "diag.cause_effect.empty"},
        {"测量列与因子 A 长度不一致。", "diag.variability.length_mismatch_a"},
        {"测量列与因子 B 长度不一致。", "diag.variability.length_mismatch_b"},
        {"没有有效的测量值可用于变异性图。", "diag.variability.empty"},
        {"效果（未命名）", "label.cause_effect.unnamed"},
        {"导入结果的列类型数量与列名不一致。", "diag.import.contract.column_types_mismatch"},
        {"导入结果的 RowId 数量与数据行不一致。", "diag.import.contract.row_ids_mismatch"},
        {"导入结果的单元格状态行数与数据行不一致。",
         "diag.import.contract.cell_states_rows_mismatch"},
        {"导入元数据中的原始行数与数据行不一致。",
         "diag.import.contract.metadata_row_count_mismatch"},
        {"导入元数据中的列数与列名不一致。",
         "diag.import.contract.metadata_column_count_mismatch"},
        {"导入结果包含重复的 RowId。", "diag.import.contract.duplicate_row_id"},
        {"滞后过大，有效回归行不足。", "diag.adf.lags_too_large"},
        {"临界值为大样本 MacKinnon 风格常数表；非 Minitab 导出 golden。",
         "diag.adf.critical_source"},
        {"默认带宽为 Silverman：h=0.9·min(s,IQR/1.34)·n^(-1/5)；高斯核。",
         "diag.eda.kde_silverman_bandwidth"},
        {"ImportPlan 缺少 provider_id。", "diag.import.plan.missing_provider_id"},
        {"ImportPlan 缺少对象名称。", "diag.import.plan.missing_object_name"},
        {"请至少选择一列导入。", "diag.import.plan.need_selected_columns"},
        {"column_order 长度必须与 selected_columns 一致。",
         "diag.import.plan.column_order_length"},
        {"column_order 必须与 selected_columns 是同一集合。",
         "diag.import.plan.column_order_mismatch"},
        {"aliases 长度必须与 selected_columns 一致。", "diag.import.plan.aliases_length"},
        {"keyset_after.columns 不能为空。", "diag.import.plan.keyset_columns_empty"},
        {"keyset_after.columns 与 after_values 长度必须一致。",
         "diag.import.plan.keyset_columns_values_length"},
        {"单列 keyset 必须与 order_key 一致。", "diag.import.plan.keyset_single_order_key"},
        {"复合 keyset 必须包含 order_key。", "diag.import.plan.keyset_must_include_order_key"},
        {"不能同时使用 keyset_after 与 row_offset；优先 keyset。",
         "diag.import.plan.keyset_row_offset_conflict"},
        {"keyset 分页需要设置 row_limit/page_size。", "diag.import.plan.keyset_needs_row_limit"},
        {"当前观测未发现稳定性图超限点。", "rule.msa.type1.stability_clear.message"},
        {"量具稳定性图存在超限点，需要调查特殊原因。",
         "rule.msa.type1.stability_triggered.message"},
        {"超限只是调查提示，不能直接判定量具合格或不合格。",
         "rule.msa.type1.stability.action"},
        {"组内 σ 至少需要两个子组。", "interp.spc_err_within_need_two_subgroups"},
        {"各子组必须具有相同观测数。", "interp.spc_err_equal_subgroup_size"},
        {"子组不允许 NaN 或无穷观测。", "interp.spc_err_subgroup_non_finite"},
        {"子组大小超出无偏常数表范围。", "interp.spc_err_subgroup_size_table"},
        {"无法计算该子组大小的 c4。", "interp.spc_err_c4"},
        {"LSL 必须为有限数。", "interp.capability_err_lsl_finite"},
        {"USL 必须为有限数。", "interp.capability_err_usl_finite"},
        {"过程均值必须为有限数。", "interp.capability_err_mean_finite"},
        {"组间/组内能力需要有效数值观测。", "interp.capability_err_bw_need_valid"},
        {"ANOM 至少需要两个组。", "interp.anom_err_need_two_groups"},
        {"每组至少需要 2 个观测。", "interp.anom_err_group_min_two"},
        {"无法估计组内方差。", "interp.anom_err_within_var"},
        {"组内标准差为 0。", "interp.anom_err_zero_within_sd"},
        {"因素 ID 不能为空。", "interp.rsd_err_empty_factor_id"},
        {"因素低/高水平必须有限。", "interp.rsd_err_nonfinite_bounds"},
        {"中心值必须有限且落在 [low, high] 内。", "interp.rsd_err_invalid_center"},
        {"alpha 必须为正有限数。", "interp.ccd_err_invalid_alpha"},
        {"样本量 n 必须 ≥ 1。", "interp.acceptance_err_n_ge_1"},
        {"接收数 c 不能大于样本量 n。", "interp.acceptance_err_c_gt_n"},
        {"批大小 N < n；本轮仍用二项 OC（无限批近似）。",
         "interp.acceptance_warn_lot_lt_n"},
        {"各组样本量不等；决策限仍用总 N 近似，解读需谨慎。",
         "interp.anom_warn_unequal_n"},
        {"决策限采用正态/多重比较近似（formula_reference），不是 Minitab exact h 表 golden。",
         "interp.anom_warn_limits_approx"},
        {"泊松拟合优度要求非负整数计数。", "interp.poisson_gof_err_nonneg_int"},
        {"至少需要 2 个有效计数。", "interp.poisson_gof_err_need_two"},
        {"合并后类别不足，无法计算拟合优度。", "interp.poisson_gof_err_too_few_bins"},
        {"自由度 < 1（类别过少）。", "interp.poisson_gof_err_df_lt_1"},
        {"存在期望频数 < 5，卡方近似需谨慎。", "interp.poisson_gof_warn_expected_lt5"},
        {"设计生成失败。", "interp.design_gen_failed"},
        {"有效样本不足以进行 AICc 小样本修正，已返回未修正的 AIC。",
         "interp.arima_warn_aicc_unavailable"},
        {"残差平方和接近零，信息准则使用数值下限稳定计算。",
         "interp.arima_warn_near_zero_sse"},
        {"AR(1) 拟合失败。", "interp.arima_err_ar1_fit_failed"},
        {"MA(1) 拟合失败。", "interp.arima_err_ma1_fit_failed"},
        {"差分后样本不足以拟合 ARIMA 阶数。",
         "interp.arima_err_insufficient_differenced"},
        {"AR 拟合失败或设计矩阵秩亏。", "interp.arima_err_ar_fit_rank_deficient"},
        {"MA 拟合失败。", "interp.arima_err_ma_fit_failed"},
        {"当前仅支持 AR(p,d,0) 与 MA(0,d,q) 候选，不含混合 ARMA 阶。",
         "interp.arima_err_no_mixed_arma_candidates"},
        {"ARIMA 候选要求更多的有限观测值才能拟合。",
         "interp.arima_err_insufficient_samples"},
        {"预测期数必须大于零。", "interp.arima_err_forecast_horizon_gt0"},
        {"观测序列必须全部为有限数值。", "interp.arima_err_nonfinite_observations"},
        {"模型计算产生了非有限结果，请检查数据尺度后重试。",
         "interp.arima_err_nonfinite_result"},
        {"仅支持 p/q≤3、d≤2 的有限阶 ARIMA。", "interp.arima_err_order_bounds"},
        {"ARIMA(0,0,0) 不在候选网格内。", "interp.arima_err_degenerate_000"},
        {"混合 ARMA 阶尚未纳入 Best ARIMA 网格。",
         "interp.arima_err_mixed_arma_not_in_grid"},
        {"Best ARIMA 候选基于条件最小二乘与差分尺度 AICc；与 Minitab 迭代最小二乘（Meeker TSERIES）的最优阶可能不同。",
         "interp.arima_warn_css_aicc_not_minitab"},
        {"请指定等价下限和上限。", "diag.equiv_need_bounds"},
        {"请指定比值等价下限和上限。", "diag.equiv_need_ratio_bounds"},
        {"请指定目标比例。", "diag.equiv_need_target_proportion"},
        {"请选择不合格品/缺陷数列。", "diag.attr.need_defect_column"},
        {"请指定检验数/单位数列或常数。", "diag.attr.need_inspected_or_constant"},
        {"不合格品数必须是非负整数。", "diag.attr.defectives_nonneg_int"},
        {"不合格品数和检验数必须是非负整数。",
         "diag.attr.defectives_inspected_nonneg_int"},
        {"请指定不合格品数列和检验数（常数或列）。",
         "diag.attr.need_defectives_and_inspected"},
        {"NP 图需要固定检验数或检验数列。", "diag.attr.np_need_inspected"},
        {"缺陷数必须是非负整数。", "diag.attr.defects_nonneg_int"},
        {"U 图需要单位数列。", "diag.attr.u_need_units"},
        {"Laney U' 图需要单位数列。", "diag.attr.laney_u_need_units"},
        {"阶段列存在缺失标签，请补齐原始数据。", "diag.spc.stage_column_missing_labels"},
        {"重复次数小于 2，无法绘制按零件 R 图。", "diag.msa.insufficient_replicates_part_r"},
        {"重复次数小于 2，无法绘制按操作者 R 图。", "diag.msa.insufficient_replicates_operator_r"},
        {"当前为二参数 Weibull。含阈值的三参数模型请选择 model=weibull3。",
         "diag.reliability.two_param_weibull_hint"},
        {"当前为一参数指数。含阈值的两参数模型请选择 model=exponential2。",
         "diag.reliability.one_param_exponential_hint"},
        {"当前为二参数对数正态。含阈值的三参数模型请选择 model=lognormal3。",
         "diag.reliability.two_param_lognormal_hint"},
        {"目标功效必须位于 0 和 1 之间。", "diag.power.target_not_in_01"},
        {"在允许的最大样本量内无法达到目标功效。", "diag.power.max_sample_cannot_reach_target"},
        {"在允许的最大重复数内无法达到目标功效。", "diag.power.max_replicate_cannot_reach_target"},
        {"在允许的最大 n 内 Howe k 仍大于目标上限。", "diag.power.howe_k_exceeds_limit_at_max_n"},
        {"时间序列没有有效数值观测。", "diag.time_series.no_valid_observations"},
        {"单指数平滑要求至少两个观测、alpha 位于 (0,1] 且预测期数大于 0。",
         "diag.time_series.single_exp_invalid"},
        {"双指数平滑要求至少三个观测，alpha/gamma 位于 (0,1] 且预测期数大于 0。",
         "diag.time_series.double_exp_invalid"},
        {"正态性检验需要至少一个有效数值观测。", "diag.normality.need_one_valid"},
        {"正态性检验至少需要 2 个观测；当前仅返回概率图点。",
         "diag.normality.need_two_obs_prob_only"},
        {"常量样本无法计算正态性检验统计量。", "diag.normality.zero_variance"},
        {"正态性检验至少需要 3 个有效观测；n<3 时不计算统计量与 p 值。",
         "diag.normality.need_three_valid"},
        {"Ryan–Joiner 统计量无法计算（方差或正态得分为零）。",
         "diag.normality.ryan_joiner_not_computed"},
        {"Ryan–Joiner：R 高于 α=0.10 临界，报告 p>0.10（存储 0.10）。",
         "diag.normality.ryan_joiner_p_gt_010"},
        {"Ryan–Joiner：R 不高于 α=0.01 临界，报告 p<0.01（存储 0.01）。",
         "diag.normality.ryan_joiner_p_lt_001"},
        {"个体分布识别至少需要 3 个有效数值观测。",
         "diag.distribution_id.insufficient_data"},
        {"存在非正值；Weibull、Lognormal 与 Exponential 仅对全部正值计算 AD。",
         "diag.distribution_id.non_positive_values"},
        {"因子数量过大，无法验证全因子运行数。", "diag.doe.factor_count_overflow"},
        {"每个 DOE 运行必须包含一个对应每个因子的编码。",
         "diag.doe.invalid_run_shape"},
        {"中心点的所有编码必须为零。", "diag.doe.invalid_center_point"},
        {"全因子运行的编码只能为 -1 或 +1。", "diag.doe.invalid_factor_level"},
        {"设计未完整覆盖每个 2 水平因子组合，或存在重复运行。",
         "diag.doe.incomplete_factorial_coverage"},
        {"hold 实际值超出高低水平，已 clamp 到编码 [-1,1]。",
         "diag.doe.hold_out_of_range"},
        {"因子高低水平相等，hold 回退编码 0。", "diag.doe.invalid_hold_levels"},
        {"hold 实际值无法匹配高低水平，该因子回退编码 0。",
         "diag.doe.invalid_hold_value"},
        {"未作图的因子按指定实际单位 hold（已转换为编码）求值。",
         "diag.doe.contour_factors_held_at_actual"},
        {"未作图的因子在编码 0 处保持不变。", "diag.doe.contour_factors_held_at_zero"},
        {"基设计因子过多，无法生成运行数。", "diag.doe.base_design_factor_overflow"},
        {"因子数量过大，无法生成运行数。", "diag.doe.generate_run_count_overflow"},
        {"中心点数量过大，无法生成 DOE 运行列表。",
         "diag.doe.center_point_count_overflow"},
        {"DOE 运行数与设计内容不一致。", "diag.doe.invalid_run_count"},
        {"DOE 区组编号必须从 1 开始。", "diag.doe.invalid_block"},
        {"响应值数量必须与 DOE 运行数一致。", "diag.doe.invalid_response_shape"},
        {"计算 DOE 效应时正负对比组必须都有有效响应。",
         "diag.doe.insufficient_effect_data"},
        {"因子数量过大，无法诊断完整 2 水平组合。",
         "diag.doe.diagnose_factor_count_overflow"},
        {"设计缺少一个或多个 2 水平因子组合。", "diag.doe.missing_runs"},
        {"设计包含重复的因子组合。", "diag.doe.duplicate_runs"},
        {"区组项已作为模型中的分类项纳入拟合。", "diag.doe.block_terms_included"},
        {"检测到重复因子组合，已用于纯误差估计。", "diag.doe.replicated_runs"},
        {"误差自由度为 0，标准化效应 Pareto 使用 Lenth PSE 参考线。",
         "diag.doe.lenth_pse_unreplicated"},
        {"等值线/曲面图需要至少两个连续因子。", "diag.contour_requires_two_factors"},
        {"等值线因子索引无效。", "diag.doe.invalid_contour_factor_index"},
        {"没有可用于求值的析因系数。", "diag.doe.missing_factorial_coefficients"},
        {"二水平模型无平方项，等值线为双线性面，不能表示曲率。",
         "diag.doe.factorial_contour_no_quadratic"},
        {"轴因子的 hold 条目已忽略。", "diag.doe.hold_ignored_axis_factor"},
        {"hold 中出现未知因子名，已忽略。", "diag.doe.unknown_hold_factor"},
        {"二维高斯混合 = formula_reference / gaussian_mixture_2_em（EM + BIC vs 单正态）；"
         "多 k 搜索见 gaussian_mixture_k_bic；不是 vendor_oracle，不得写成过程合格判定。",
         "diag.gaussian_mixture2.scope"},
        {"区间删失 KM 至少需要约 3 条观测。", "diag.km_interval.need_three_obs"},
        {"存在 R < L 的无效区间。", "diag.km_interval.invalid_interval_order"},
        {"有效区间过少。", "diag.km_interval.too_few_valid"},
        {"无法构造 Turnbull 时间网格。", "diag.km_interval.grid_failed"},
        {"无候选质量点。", "diag.km_interval.no_mass_points"},
        {"Turnbull 质量塌缩。", "diag.km_interval.mass_collapsed"},
        {"Turnbull 已达最大迭代。", "diag.km_interval.max_iter"},
        {"Turnbull NPMLE（简化网格）；非右删失 product-limit；非 Minitab golden。",
         "diag.km_interval.scope"},
        {"证据类型 = formula_reference（algorithm_id=turnbull_npmle_simplified_grid）；"
         "不得写成 vendor_oracle / golden / 商业软件对齐。",
         "diag.km_interval.evidence"},
        {"估计的组间方差为负，已截断为 0；σ_B 可能低估。",
         "diag.cap.between_variance_truncated"},
        {"组间/组内能力区间自由度使用 N−1，不是 Minitab Rbar/Sbar 调整 ν。",
         "diag.cap.ci_df_used_sample_n"},
        {"样本量不足或置信水平非法，未计算能力指数区间。", "diag.cap.ci_not_computed"},
        {"至少一个规格限落在变换定义域外；越界侧的百分位回推未实现，"
         "仅输出仍可变换规格的 overall 指数。",
         "interp.johnson_spec_partial_outside"},
        {"经典 Kaplan–Meier 路径不接受左删失/区间删失；请改用区间删失 KM，"
         "或先过滤为 exact/right。",
         "diag.censoring.classic_km_rejects_left_interval"},
        {"未提供分层输入；仅输出总体保修摘要。", "diag.warranty.no_strata_input"},
        {"分层 expected_failures = 层暴露量 * F(T_w)，F 来自总体池化 R(T_w)；"
         "未使用分模式可靠度，不是 vendor_oracle。",
         "diag.warranty.strata_pooled_reliability"},
        {"分层 kind 必须为 failure_mode 或 group。", "diag.warranty.stratum_kind_missing"},
        {"同一次分层摘要不得混用 failure_mode 与 group。",
         "diag.warranty.stratum_kind_mixed"},
        {"分层暴露量必须为有限非负数。", "diag.warranty.stratum_invalid_exposure"},
        {"分层无测量暴露量且 valid_count 全为 0，无法追溯分母。",
         "diag.warranty.stratum_no_denominator"},
        {"分层暴露量按 valid_count 比例分摊标量总暴露量；"
         "这不是实测分母，报告须标注 exposure_attribution=proportional_scalar。",
         "diag.warranty.stratum_exposure_proportional"},
        {"分层暴露量之和与总体暴露量不一致；分层 expected_failures 使用各层自身暴露量，"
         "不得静默重标度到总体。",
         "diag.warranty.stratum_exposure_sum_mismatch"},
        {"未知特殊原因规则", "status.unknown_special_cause_rule"},
        {"缺陷数和单位数必须是非负整数。", "diag.attr.defects_units_nonneg_int"},
        {"缺陷数和单位数必须是有效的非负整数列。",
         "diag.attr.defects_units_valid_cols"},
        {"没有可用于柏拉图的类别数据。", "diag.pareto.no_category_data"},
        {"已忽略负数或非整数缺陷计数。", "diag.pareto.ignored_bad_counts"},
        {"缺陷数必须为非负整数。", "diag.attr.defects_nonneg_int"},
        {"请指定检验数列或检验数常数。", "diag.attr.need_inspected_col_or_constant"},
        {"检验数必须是非负整数。", "diag.attr.inspected_nonneg_int"},
        {"请选择类别列与原因列。", "diag.cause_effect.need_category_and_cause"},
        {"未检测到中心点（编码全 0）；纯误差/失拟诊断受限。",
         "diag.rsm.no_center_points_coded"},
        {"缺少回归系数协方差矩阵，响应优化表中的置信区间与预测区间显示为 *。",
         "diag.doe.opt_missing_cov_ci_star"},
        {"CCD 点集按 NIST Response Surface / CCD 定义生成；证据类型 formula_reference，非 vendor_oracle，未冻结为商业软件对齐 golden。",
         "diag.ccd.nist_formula_reference"},
        {"BBD 点集按 NIST Box–Behnken 定义生成；证据类型 formula_reference，非 vendor_oracle。",
         "diag.bbd.nist_formula_reference"},
        {"complete-case 有效行不足，无法拟合二次模型。",
         "diag.rsm.insufficient_complete_case"},
        {"因子编码失败。", "diag.rsm.factor_coding_failed"},
        {"请选择区间左端与右端列（右删失可用空/Inf）。",
         "diag.km_interval.need_left_right_columns"},
        {"EDA 四图用于探索位置/散布/随机性/分布形态假设，不能写成过程受控或分布已正态。",
         "diag.eda.four_plot_exploratory"},
        {"所选列没有数值观测。", "diag.column_no_numeric_observations"},
        {"请选择多个连续变量，或行类别、列类别和数值。",
         "diag.graph.need_multi_or_heatmap_layout"},
        {"请选择 X、Y 和气泡大小变量。", "diag.graph.need_xy_size"},
        {"请选择 X、Y 和 Z 变量。", "diag.graph.need_xyz"},
        {"请选择 X 变量和 Y 变量。", "diag.graph.need_xy"},
        {"请选择响应变量和分类变量。", "diag.graph.need_response_and_category"},
        {"请至少选择两个连续变量。", "diag.graph.need_two_continuous"},
        {"请选择连续变量。", "diag.graph.need_continuous"},
        {"请选择数值变量。", "diag.graph.need_numeric"},
        {"请选择分类变量。", "diag.graph.need_category"},
        {"请选择响应变量。", "diag.graph.need_response"},
        {"条形图显示计数省略 hidden；分析口径计数保留 hidden、仅省略 excluded。两者不得合并为一个 bool。",
         "diag.graph.bar_hidden_excluded_distinct"},
        {"请选择至少两列数值变量。", "diag.corr.need_two_numeric"},
        {"请指定假设均值。", "diag.ttest.need_hypothesized_mean"},
        {"请指定已知总体标准差 σ（必须为正）。", "diag.ztest.need_known_sigma"},
        {"请选择两列独立样本变量。", "diag.two_sample_t.need_two_columns"},
        {"请选择正好两列独立样本。", "diag.mann_whitney.need_exactly_two"},
        {"请选择两列独立样本。", "diag.tost.need_two_independent"},
        {"请选择检验列与参考列。", "diag.equiv.need_test_and_reference"},
        {"请选择两列配对测量值。", "diag.paired.need_two_columns"},
        {"请选择两组事件数和试验数列。", "diag.two_prop.need_two_event_trial_pairs"},
        {"请选择事件数列和试验数列。", "diag.one_prop.need_events_trials"},
        {"请选择一个响应变量和至少一个预测变量。",
         "diag.regression.need_response_and_predictor"},
        {"请指定假设比例。", "diag.one_prop.need_hypothesized_proportion"},
        {"请选择缺陷计数列。", "diag.poisson.need_count_column"},
        {"请选择观测长度列或输入观测长度常数。", "diag.poisson.need_length_or_const"},
        {"请指定假设发生率。", "diag.poisson.need_hypothesized_rate"},
        {"请选择两组缺陷计数和观测长度列。", "diag.two_poisson.need_count_length_pairs"},
        {"请选择行分类列和列分类列。", "diag.chi2.need_row_and_column"},
        {"请选择分类列。", "diag.gof.need_category_column"},
        {"请选择第一样本列或测量列。", "diag.variance.need_first_or_measure"},
        {"所选列没有足够的数值观测。", "diag.column_insufficient_numeric"},
        {"没有可用于比例检验的 complete-case 行。", "diag.prop.no_complete_case_rows"},
        {"事件数和试验数必须为非负整数，且事件数不能超过试验数。",
         "diag.prop.events_trials_nonneg_bounded"},
        {"请选择一列（相对 η0）或两列配对样本。", "diag.wilcoxon.need_one_or_paired"},
        {"请选择一列（单样本）或两列（配对）。", "diag.sign.need_one_or_paired"},
        {"请选择一列数值序列（按行序）。", "diag.runs.need_numeric_sequence"},
        {"请选择正好两列分类变量以构建 2×2 表。",
         "diag.fisher.need_exactly_two_categorical"},
        {"请选择正好两列配对二元结果。", "diag.mcnemar.need_exactly_two_paired_binary"},
        {"请选择至少两列配对二元结果（k≥3 才计算；k=2 请用 McNemar）。",
         "diag.cochran.need_at_least_two_paired_binary"},
        {"请选择测量列和分组列。", "diag.grouped.need_measure_and_group"},
        {"请选择测量值、零件、操作员与附加因子列；无附加因子时请用交叉 Gage R&R。",
         "diag.msa.need_expanded_factors"},
        {"请选择测量值、零件和操作员列。", "diag.msa.need_measure_part_operator"},
        {"请选择测量值、部件和操作者列。", "diag.msa.need_measure_part_operator_alt"},
        {"请选择响应、处理与区组列。", "diag.friedman.need_response_treatment_block"},
        {"请至少选择两个数值变量列。", "diag.multivariate.need_two_numeric"},
        {"请选择时间序列数值列。", "diag.ts.need_numeric_series"},
        {"请选择二元响应列和至少一个预测变量。",
         "diag.logistic.need_binary_response_and_predictor"},
        {"请选择时间序列值列并输入正整数季节周期。",
         "diag.seasonal.need_series_and_period"},
        {"请选择响应列与至少两个连续因子列。",
         "diag.rsm.need_response_and_two_factors"},
        {"请选择至少一个响应列和已导入因子列。",
         "diag.doe_opt.need_response_and_imported_factors"},
        {"请选择寿命列。", "diag.reliability.need_lifetime_column"},
        {"请选择失效/删失指示列，或逐行删失类型列（exact/right/left/interval）。",
         "diag.reliability.need_event_or_censor_type"},
        {"请选择一个数值序列列。", "diag.ts.need_one_numeric_series"},
        {"请选择计数响应列与至少一个数值预测列。",
         "diag.poisson_reg.need_count_response_and_predictor"},
        {"请选择响应列与至少两个候选预测列。",
         "diag.stepwise.need_response_and_two_candidates"},
        {"请选择响应列与至少一个数值预测列。",
         "diag.cart.need_response_and_numeric_predictor"},
        {"请选择一个数值列。", "diag.univariate.need_one_numeric"},
        {"请选择有序响应列与至少一个数值预测列。",
         "diag.ordinal.need_ordered_response_and_predictor"},
        {"请选择类别响应与至少一个数值预测列。",
         "diag.discriminant.need_class_response_and_predictor"},
        {"请选择两个数值序列列。", "diag.ts.need_two_numeric_series"},
        {"请提供至少一个因子名称。", "diag.pb.need_factor_name"},
        {"请输入 LSL 或 USL。", "diag.capability.need_lsl_or_usl"},
        {"请选择一列数值观测（子组大小=1）。", "diag.run_chart.need_individuals"},
        {"请指定样本量 n ≥ 1。", "diag.oc.need_sample_size_n"},
        {"请选择一列非负整数计数。", "diag.gof.need_nonneg_integer_counts"},
    };
    for (const auto& [zh, id] : exact_ids) {
        if (message == zh) {
            return domain::resolve_report_text(id, language, missing_out).text;
        }
    }
    // Longer diagnostic prefixes first so "响应优化预测：" wins over shorter tokens.
    static const std::pair<const char*, const char*> diag_prefix_ids[] = {
        {"响应优化预测：", "diag.prefix.response_opt_prediction"},
        {"相关分析：", "diag.prefix.correlation"},
        {"单样本 t：", "diag.prefix.one_sample_t"},
        {"单样本 Z：", "diag.prefix.one_sample_z"},
        {"双样本 t：", "diag.prefix.two_sample_t"},
        {"第一组：", "diag.prefix.first_group"},
        {"第二组：", "diag.prefix.second_group"},
        {"能力分析: ", "diag.prefix.capability"},
    };
    for (const auto& [zh_prefix, prefix_id] : diag_prefix_ids) {
        const std::string prefix = zh_prefix;
        if (starts_with(message, prefix) && message.size() > prefix.size()) {
            const std::string raw = message.substr(prefix.size());
            const std::string detail =
                localize_known_plain_message(raw, language, missing_out);
            return domain::resolve_report_text(prefix_id, language, missing_out).text
                + detail;
        }
    }
    {
        const std::string chart_suffix_zh = "没有可显示的数据。";
        if (ends_with(message, chart_suffix_zh)
            && message.size() > chart_suffix_zh.size()) {
            return message.substr(0, message.size() - chart_suffix_zh.size())
                + domain::resolve_report_text(
                      "interp.chart_no_displayable_data_suffix",
                      language,
                      missing_out)
                      .text;
        }
    }
    {
        const std::string units_suffix_zh = "需要单位数列。";
        if (ends_with(message, units_suffix_zh)
            && message.size() > units_suffix_zh.size()) {
            return message.substr(0, message.size() - units_suffix_zh.size())
                + domain::resolve_report_text(
                      "diag.attr.need_units_column_suffix",
                      language,
                      missing_out)
                      .text;
        }
    }
    {
        const std::string doe_prefix = "部分析因分辨度 ";
        const std::string doe_suffix = "（字长最短非 I 词）。";
        if (starts_with(message, doe_prefix) && ends_with(message, doe_suffix)
            && message.size() > doe_prefix.size() + doe_suffix.size()) {
            const std::string roman = message.substr(
                doe_prefix.size(),
                message.size() - doe_prefix.size() - doe_suffix.size());
            return replace_token(
                domain::resolve_report_text(
                    "interp.doe_fractional_resolution", language, missing_out)
                    .text,
                "%1",
                roman);
        }
    }
    {
        const std::string tukey_prefix = "Tukey 同时置信水平 = ";
        const std::string tukey_suffix =
            "；显著性由同时置信区间是否包含 0 决定。";
        if (starts_with(message, tukey_prefix) && ends_with(message, tukey_suffix)
            && message.size() > tukey_prefix.size() + tukey_suffix.size()) {
            const std::string level = message.substr(
                tukey_prefix.size(),
                message.size() - tukey_prefix.size() - tukey_suffix.size());
            return replace_token(
                domain::resolve_report_text(
                    "rule.anova.catalog.family_error_rate.computed",
                    language,
                    missing_out)
                    .text,
                "%1",
                level);
        }
    }
    {
        const std::string fisher_prefix =
            "Fisher 精确检验要求恰好 2×2 水平；当前行水平数 = ";
        const std::string fisher_mid = "，列水平数 = ";
        const std::string fisher_suffix = "。";
        if (starts_with(message, fisher_prefix) && ends_with(message, fisher_suffix)) {
            const auto mid_pos = message.find(fisher_mid, fisher_prefix.size());
            if (mid_pos != std::string::npos) {
                const std::string n1 = message.substr(
                    fisher_prefix.size(), mid_pos - fisher_prefix.size());
                const std::string n2 = message.substr(
                    mid_pos + fisher_mid.size(),
                    message.size() - (mid_pos + fisher_mid.size())
                        - fisher_suffix.size());
                return replace_token(
                    replace_token(
                        domain::resolve_report_text(
                            "interp.fisher_need_2x2_levels", language, missing_out)
                            .text,
                        "%1",
                        n1),
                    "%2",
                    n2);
            }
        }
    }
    if (starts_with(message, "描述统计跳过 ")
        && ends_with(message, " 个缺失或非法单元格（含 *）。")) {
        const std::string n = message.substr(
            std::string("描述统计跳过 ").size(),
            message.size() - std::string("描述统计跳过 ").size()
                - std::string(" 个缺失或非法单元格（含 *）。").size());
        return replace_token(
            domain::resolve_report_text(
                "interp.desc_skip_n_missing_star", language, missing_out)
                .text,
            "%1",
            n);
    }
    if (starts_with(message, "泊松拟合优度跳过 ")
        && ends_with(message, " 个缺失或非法单元格。")) {
        const std::string n = message.substr(
            std::string("泊松拟合优度跳过 ").size(),
            message.size() - std::string("泊松拟合优度跳过 ").size()
                - std::string(" 个缺失或非法单元格。").size());
        return replace_token(
            domain::resolve_report_text(
                "interp.poisson_gof_skip_n_missing", language, missing_out)
                .text,
            "%1",
            n);
    }
    {
        const std::string t_prefix = "双样本 t 检验跳过缺失或非法单元格；组 1 = ";
        const std::string t_mid = "，组 2 = ";
        const std::string t_suffix = "。";
        if (starts_with(message, t_prefix) && ends_with(message, t_suffix)) {
            const auto mid_pos = message.find(t_mid, t_prefix.size());
            if (mid_pos != std::string::npos) {
                const std::string n1 = message.substr(
                    t_prefix.size(), mid_pos - t_prefix.size());
                const std::string n2 = message.substr(
                    mid_pos + t_mid.size(),
                    message.size() - (mid_pos + t_mid.size()) - t_suffix.size());
                return replace_token(
                    replace_token(
                        domain::resolve_report_text(
                            "interp.two_sample_t_skip_groups", language, missing_out)
                            .text,
                        "%1",
                        n1),
                    "%2",
                    n2);
            }
        }
    }
    {
        static const std::pair<const char*, const char*> skip_prefix_ids[] = {
            {"相关分析 complete-case 跳过 ", "interp.corr_skip_complete_case_rows"},
            {"ANOVA 跳过 ", "interp.anova_skip_n_response"},
            {"ANOM 跳过 ", "interp.anom_skip_n_response"},
            {"Box-Cox 跳过 ", "interp.boxcox_skip_n_obs"},
            {"Type 1 Gage 跳过 ", "interp.type1_gage_skip_n_obs"},
            {"Bias/Linearity 跳过 ", "interp.bias_linearity_skip_n_obs"},
            {"Mann–Whitney 跳过 ", "interp.mann_whitney_skip_n_star"},
            {"Wilcoxon 跳过 ", "interp.wilcoxon_skip_n_star"},
            {"符号检验跳过 ", "interp.sign_skip_n_star"},
            {"McNemar 按 complete-case 跳过 ", "interp.mcnemar_skip_n_star"},
            {"Mood 跳过 ", "interp.mood_skip_n_star"},
            {"Kruskal–Wallis 跳过 ", "interp.kruskal_skip_n_star"},
            {"Friedman 按 complete-case 跳过 ", "interp.friedman_skip_n_star"},
            {"Fisher 精确检验按 complete-case 跳过 ",
             "interp.fisher_skip_n_missing_cells"},
            {"Cochran Q 按 complete-case 跳过 ", "interp.cochran_skip_n_rows_star"},
            {"测量或因子缺失、* 或非法单元格已跳过 ", "interp.multivari_skip_n_rows"},
            {"因果图跳过 ", "interp.cause_effect_skip_n_empty"},
            {"已跳过 ", "diag.cause_effect.skipped_empty_cells"},
            {"游程检验跳过两端缺失或非法单元格 N* = ", "interp.runs_skip_nstar_ends"},
            {"运行图跳过两端缺失或非法单元格 N* = ", "interp.run_chart_skip_nstar_ends"},
        };
        static const std::pair<const char*, const char*> skip_suffix_by_id[] = {
            {"interp.corr_skip_complete_case_rows", " 个含缺失或非法单元格的行。"},
            {"interp.anova_skip_n_response", " 个缺失或非法响应值。"},
            {"interp.anom_skip_n_response", " 个缺失或非法响应值。"},
            {"interp.boxcox_skip_n_obs", " 个缺失或非法观测。"},
            {"interp.type1_gage_skip_n_obs", " 个缺失或非法观测。"},
            {"interp.bias_linearity_skip_n_obs", " 个缺失或不完整观测。"},
            {"interp.mann_whitney_skip_n_star", " 个缺失或非法单元格（含 *）。"},
            {"interp.wilcoxon_skip_n_star", " 个缺失或非法单元格（含 *）。"},
            {"interp.sign_skip_n_star", " 个缺失或非法单元格（含 *）。"},
            {"interp.mcnemar_skip_n_star", " 个缺失或非法单元格（含 *）。"},
            {"interp.mood_skip_n_star", " 个缺失或非法单元格（含 *）。"},
            {"interp.kruskal_skip_n_star", " 个缺失或非法单元格（含 *）。"},
            {"interp.friedman_skip_n_star", " 个缺失或非法单元格（含 *）。"},
            {"interp.fisher_skip_n_missing_cells", " 个缺失单元格。"},
            {"interp.cochran_skip_n_rows_star", " 行缺失（含 *）。"},
            {"interp.multivari_skip_n_rows", " 行。"},
            {"interp.cause_effect_skip_n_empty", " 个空类别或空原因单元格。"},
            {"diag.cause_effect.skipped_empty_cells", " 个空类别或空原因单元格。"},
            {"interp.runs_skip_nstar_ends", "。"},
            {"interp.run_chart_skip_nstar_ends", "。"},
        };
        for (const auto& [prefix_zh, id] : skip_prefix_ids) {
            const std::string prefix = prefix_zh;
            if (!starts_with(message, prefix)) {
                continue;
            }
            const char* suffix_zh = nullptr;
            for (const auto& [sid, sfx] : skip_suffix_by_id) {
                if (std::string(sid) == id) {
                    suffix_zh = sfx;
                    break;
                }
            }
            if (suffix_zh == nullptr) {
                continue;
            }
            const std::string suffix = suffix_zh;
            if (!ends_with(message, suffix)
                || message.size() <= prefix.size() + suffix.size()) {
                continue;
            }
            const std::string n = message.substr(
                prefix.size(), message.size() - prefix.size() - suffix.size());
            return replace_token(
                domain::resolve_report_text(id, language, missing_out).text,
                "%1",
                n);
        }
    }
    {
        const std::string mid = " 跳过 ";
        const std::string suffix = " 个缺失或非法单元格。";
        const auto mid_pos = message.find(mid);
        if (mid_pos != std::string::npos && mid_pos > 0 && ends_with(message, suffix)
            && !starts_with(message, "跳过 ")
            && !starts_with(message, "泊松拟合优度跳过 ")
            && !starts_with(message, "描述统计跳过 ")) {
            const std::string name = message.substr(0, mid_pos);
            const std::string n = message.substr(
                mid_pos + mid.size(),
                message.size() - (mid_pos + mid.size()) - suffix.size());
            return replace_token(
                replace_token(
                    domain::resolve_report_text(
                        "interp.named_skip_n_missing_cells", language, missing_out)
                        .text,
                    "%1",
                    name),
                "%2",
                n);
        }
    }
    if (starts_with(message, "跳过 ")
        && ends_with(message, " 个缺失或非法单元格（含 *）。")) {
        const std::string n = message.substr(
            std::string("跳过 ").size(),
            message.size() - std::string("跳过 ").size()
                - std::string(" 个缺失或非法单元格（含 *）。").size());
        return replace_token(
            domain::resolve_report_text(
                "interp.skip_n_missing_invalid_cells_star", language, missing_out)
                .text,
            "%1",
            n);
    }
    if (starts_with(message, "跳过 ")
        && ends_with(message, " 个缺失或非法单元格（计入 N*）。")) {
        const std::string n = message.substr(
            std::string("跳过 ").size(),
            message.size() - std::string("跳过 ").size()
                - std::string(" 个缺失或非法单元格（计入 N*）。").size());
        return replace_token(
            domain::resolve_report_text(
                "interp.skip_n_missing_counted_nstar", language, missing_out)
                .text,
            "%1",
            n);
    }
    if (starts_with(message, "跳过 ")
        && ends_with(message, " 个缺失或非法单元格。")) {
        const std::string n = message.substr(
            std::string("跳过 ").size(),
            message.size() - std::string("跳过 ").size()
                - std::string(" 个缺失或非法单元格。").size());
        return replace_token(
            domain::resolve_report_text(
                "interp.skip_n_missing_invalid_cells", language, missing_out)
                .text,
            "%1",
            n);
    }
    {
        static const std::pair<const char*, const char*> prefix_ids[] = {
            {"所选列不存在：", "diag.import.plan.column_not_found"},
            {"所选列重复：", "diag.import.plan.duplicate_column"},
            {"排序键不存在：", "diag.import.plan.order_key_not_found"},
            {"keyset 列不存在：", "diag.import.plan.keyset_column_not_found"},
            {"过滤列不存在：", "diag.import.plan.filter_column_not_found"},
            {"不支持的过滤运算符：", "diag.import.plan.filter_op_unsupported"},
            {"过滤运算符 ", "diag.import.plan.filter_needs_value"},
        };
        for (const auto& [prefix_zh, id] : prefix_ids) {
            const std::string prefix = prefix_zh;
            if (!starts_with(message, prefix)) {
                continue;
            }
            const std::string tail = message.substr(prefix.size());
            if (id == std::string("diag.import.plan.filter_needs_value")) {
                const std::string suffix = " 需要绑定值。";
                if (!ends_with(tail, suffix) || tail.size() <= suffix.size()) {
                    continue;
                }
                const std::string op = tail.substr(0, tail.size() - suffix.size());
                return replace_token(
                    domain::resolve_report_text(id, language, missing_out).text,
                    "%1",
                    op);
            }
            return replace_token(
                domain::resolve_report_text(id, language, missing_out).text,
                "%1",
                tail);
        }
        for (const std::string& op : {"IS NULL", "IS NOT NULL"}) {
            const std::string zh = op + " 不得带绑定值。";
            if (message == zh) {
                return replace_token(
                    domain::resolve_report_text(
                        "diag.import.plan.filter_null_no_value", language, missing_out)
                        .text,
                    "%1",
                    op);
            }
        }
    }
    return message;
}

std::string localize_spc_rule_name(
    const std::string& name,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    static const std::pair<const char*, const char*> name_ids[] = {
        {"单点超出 3σ 控制限", "spc.rule.beyond_control_limit.name"},
        {"连续 9 点位于中心线同侧", "spc.rule.nine_same_side.name"},
        {"连续 6 点持续单调趋势", "spc.rule.six_point_trend.name"},
        {"连续 14 点上下交替", "spc.rule.fourteen_alternating.name"},
        {"3 点中至少 2 点同侧超过 2σ", "spc.rule.two_of_three_beyond_2sigma.name"},
        {"5 点中至少 4 点同侧超过 1σ", "spc.rule.four_of_five_beyond_1sigma.name"},
        {"连续 15 点全部落在 1σ 内", "spc.rule.fifteen_within_1sigma.name"},
        {"连续 8 点全部位于 1σ 外且同侧", "spc.rule.eight_beyond_1sigma.name"},
    };
    for (const auto& [zh, id] : name_ids) {
        if (name == zh
            || name == domain::resolve_report_text(id, "en-US", nullptr).text) {
            return domain::resolve_report_text(id, language, missing_out).text;
        }
    }
    return name;
}

std::string localize_special_cause_rule_name_list(
    const std::string& list,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    if (list.empty()) {
        return list;
    }
    std::string result;
    std::size_t start = 0;
    while (start < list.size()) {
        const auto comma = list.find(", ", start);
        const std::size_t length =
            comma == std::string::npos ? list.size() - start : comma - start;
        const std::string token = list.substr(start, length);
        if (!result.empty()) {
            result += ", ";
        }
        result += localize_spc_rule_name(token, language, missing_out);
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 2;
    }
    return result;
}

std::string localize_spc_rule_action(
    const std::string& action,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    static const std::pair<const char*, const char*> action_ids[] = {
        {"复核测量、批次、设备或取样条件，并关联原始观测行。",
         "spc.rule.beyond_control_limit.action"},
        {"调查均值偏移、分层或阶段变化，检查窗口首尾相关行。",
         "spc.rule.nine_same_side.action"},
        {"调查磨损、漂移或时间相关结构，核对趋势窗口点。",
         "spc.rule.six_point_trend.action"},
        {"调查周期、交替作业或过度调整，核对交替窗口。",
         "spc.rule.fourteen_alternating.action"},
        {"调查较小系统性偏移，核对命中点标准化距离。",
         "spc.rule.two_of_three_beyond_2sigma.action"},
        {"调查过程均值小幅移动，核对命中点标准化距离。",
         "spc.rule.four_of_five_beyond_1sigma.action"},
        {"调查控制限是否过宽或数据分层，核对 σ 来源。",
         "spc.rule.fifteen_within_1sigma.action"},
        {"调查混合总体或双群模式，按分组/批次复核。",
         "spc.rule.eight_beyond_1sigma.action"},
    };
    // Strip trailing period variants for matching if needed.
    std::string trimmed = action;
    if (!trimmed.empty() && (trimmed.back() == '。' || trimmed.back() == '.')) {
        // Keep as catalog includes Chinese period.
    }
    for (const auto& [zh, id] : action_ids) {
        if (trimmed == zh
            || trimmed == domain::resolve_report_text(id, "en-US", nullptr).text) {
            return domain::resolve_report_text(id, language, missing_out).text;
        }
    }
    return localize_zh_payload_punctuation(std::move(trimmed), language);
}

std::string localize_spc_rule_exact_map(
    const std::string& text,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out,
    const std::pair<const char*, const char*>* begin,
    const std::pair<const char*, const char*>* end)
{
    for (const auto* it = begin; it != end; ++it) {
        if (text == it->first
            || text == domain::resolve_report_text(it->second, "en-US", nullptr).text) {
            return domain::resolve_report_text(it->second, language, missing_out).text;
        }
    }
    return text;
}

std::string localize_spc_rule_window(
    const std::string& text,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    static const std::pair<const char*, const char*> ids[] = {
        {"单点", "spc.rule.beyond_control_limit.window"},
        {"连续 9 点", "spc.rule.nine_same_side.window"},
        {"连续 6 点", "spc.rule.six_point_trend.window"},
        {"连续 14 点", "spc.rule.fourteen_alternating.window"},
        {"连续 3 点窗口", "spc.rule.two_of_three_beyond_2sigma.window"},
        {"连续 5 点窗口", "spc.rule.four_of_five_beyond_1sigma.window"},
        {"连续 15 点", "spc.rule.fifteen_within_1sigma.window"},
        {"连续 8 点", "spc.rule.eight_beyond_1sigma.window"},
    };
    return localize_spc_rule_exact_map(
        text, language, missing_out, ids, ids + std::size(ids));
}

std::string localize_spc_rule_threshold(
    const std::string& text,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    static const std::pair<const char*, const char*> ids[] = {
        {"LCL / UCL（约 3σ）", "spc.rule.beyond_control_limit.threshold"},
        {"同侧 9 点", "spc.rule.nine_same_side.threshold"},
        {"严格单调 6 点", "spc.rule.six_point_trend.threshold"},
        {"相邻符号严格交替", "spc.rule.fourteen_alternating.threshold"},
        {"同侧且 |y−CL| > 2σ，至少 2 点",
         "spc.rule.two_of_three_beyond_2sigma.threshold"},
        {"同侧且 |y−CL| > 1σ，至少 4 点",
         "spc.rule.four_of_five_beyond_1sigma.threshold"},
        {"|y−CL| < σ", "spc.rule.fifteen_within_1sigma.threshold"},
        {"同侧且 |y−CL| > σ", "spc.rule.eight_beyond_1sigma.threshold"},
    };
    return localize_spc_rule_exact_map(
        text, language, missing_out, ids, ids + std::size(ids));
}

std::string localize_spc_rule_comparison(
    const std::string& text,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    static const std::pair<const char*, const char*> ids[] = {
        {"y < LCL 或 y > UCL（严格）", "spc.rule.beyond_control_limit.comparison"},
        {"全部位于中心线同一侧（中心线上点打断）",
         "spc.rule.nine_same_side.comparison"},
        {"严格递增或严格递减（相等打断）", "spc.rule.six_point_trend.comparison"},
        {"相邻差分符号交替（零差分打断）",
         "spc.rule.fourteen_alternating.comparison"},
        {"同侧且严格大于 2σ", "spc.rule.two_of_three_beyond_2sigma.comparison"},
        {"同侧且严格大于 1σ", "spc.rule.four_of_five_beyond_1sigma.comparison"},
        {"严格落在 ±1σ 以内", "spc.rule.fifteen_within_1sigma.comparison"},
    };
    return localize_spc_rule_exact_map(
        text, language, missing_out, ids, ids + std::size(ids));
}

std::string localize_spc_rule_explanation(
    const std::string& text,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    static const std::pair<const char*, const char*> ids[] = {
        {"任一点低于 LCL 或高于 UCL；有限控制限使用严格越界比较。表示该点与当前控制模型不一致，不等于根因已确认。",
         "spc.rule.beyond_control_limit.explanation"},
        {"连续 9 个可用点全部在中心线同一侧；跨缺失或阶段断点不形成窗口。提示均值偏移、分层或阶段变化。",
         "spc.rule.nine_same_side.explanation"},
        {"连续 6 点严格递增或严格递减；相等值不构成趋势。提示磨损、漂移或时间相关结构。",
         "spc.rule.six_point_trend.explanation"},
        {"连续 14 点相邻差值符号严格交替；零差值打断规则。提示周期、人机/设备交替或过度调整。",
         "spc.rule.fourteen_alternating.explanation"},
        {"3 点窗口内至少 2 点位于中心线同侧且距离严格大于 2σ；提示较小但系统性的偏移。",
         "spc.rule.two_of_three_beyond_2sigma.explanation"},
        {"5 点窗口内至少 4 点位于同侧且距离严格大于 1σ；提示过程均值发生小幅移动。",
         "spc.rule.four_of_five_beyond_1sigma.explanation"},
        {"连续 15 个点的绝对中心线距离严格小于 σ；提示控制限可能过宽或数据存在分层。",
         "spc.rule.fifteen_within_1sigma.explanation"},
        {"连续 8 点均在中心线同一侧且绝对距离严格大于 σ；提示混合总体或双群模式。",
         "spc.rule.eight_beyond_1sigma.explanation"},
    };
    return localize_spc_rule_exact_map(
        text, language, missing_out, ids, ids + std::size(ids));
}

std::string localize_spc_rule_reason(
    const std::string& text,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    static const std::pair<const char*, const char*> ids[] = {
        {"规则未启用，本次分析未验证该特殊原因模式。",
         "spc.rule.reason.not_verified"},
        {"没有可绘制的控制图点，无法判定规则。",
         "spc.rule.reason.calculation_failed"},
    };
    const std::string exact =
        localize_spc_rule_exact_map(
            text, language, missing_out, ids, ids + std::size(ids));
    if (exact != text) {
        return exact;
    }
    const std::string zh_prefix = "此规则不适用于控制图类型 ";
    const std::string en_prefix =
        domain::resolve_report_text(
            "spc.rule.reason.not_applicable_prefix", "en-US", nullptr)
            .text;
    const std::string localized_prefix =
        domain::resolve_report_text(
            "spc.rule.reason.not_applicable_prefix", language, missing_out)
            .text;
    if (starts_with(text, zh_prefix) && ends_with(text, "。")) {
        const std::string kind =
            text.substr(zh_prefix.size(), text.size() - zh_prefix.size() - 3);
        return localized_prefix + kind + ".";
    }
    if (starts_with(text, en_prefix) && ends_with(text, ".")) {
        const std::string kind =
            text.substr(en_prefix.size(), text.size() - en_prefix.size() - 1);
        return localized_prefix + kind + ".";
    }
    return text;
}

std::string localize_spc_rule_message(
    const std::string& text,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    const std::string zh_not_triggered = " 当前未触发。";
    const std::string en_not_triggered =
        domain::resolve_report_text(
            "spc.rule.message.not_triggered_suffix", "en-US", nullptr)
            .text;
    const std::string localized_not_triggered =
        domain::resolve_report_text(
            "spc.rule.message.not_triggered_suffix", language, missing_out)
            .text;
    if (ends_with(text, zh_not_triggered) || ends_with(text, en_not_triggered)) {
        const std::string suffix =
            ends_with(text, zh_not_triggered) ? zh_not_triggered : en_not_triggered;
        const std::string base = text.substr(0, text.size() - suffix.size());
        const std::string localized_base =
            localize_spc_rule_explanation(base, language, missing_out);
        if (localized_base != base) {
            return localized_base + localized_not_triggered;
        }
        return text;
    }

    const std::string zh_points_prefix = " 触发图点序号（1-based）: ";
    const std::string en_points_prefix =
        domain::resolve_report_text(
            "spc.rule.message.triggered_points_prefix", "en-US", nullptr)
            .text;
    const std::string localized_points_prefix =
        domain::resolve_report_text(
            "spc.rule.message.triggered_points_prefix", language, missing_out)
            .text;
    auto try_points = [&](const std::string& prefix) -> std::optional<std::string> {
        const auto pos = text.find(prefix);
        if (pos == std::string::npos) {
            return std::nullopt;
        }
        const std::string base = text.substr(0, pos);
        const std::string rest = text.substr(pos + prefix.size());
        const std::string localized_base =
            localize_spc_rule_explanation(base, language, missing_out);
        if (localized_base == base) {
            return std::nullopt;
        }
        return localized_base + localized_points_prefix + rest;
    };
    if (auto localized = try_points(zh_points_prefix)) {
        return *localized;
    }
    if (auto localized = try_points(en_points_prefix)) {
        return *localized;
    }
    return localize_spc_rule_explanation(text, language, missing_out);
}

std::optional<std::size_t> parse_leading_count_after_prefix(
    const std::string& text, const std::string& prefix)
{
    if (!starts_with(text, prefix)) {
        return std::nullopt;
    }
    std::size_t index = prefix.size();
    while (index < text.size() && std::isspace(static_cast<unsigned char>(text[index]))) {
        ++index;
    }
    if (index >= text.size() || !std::isdigit(static_cast<unsigned char>(text[index]))) {
        return std::nullopt;
    }
    std::size_t value = 0;
    while (index < text.size() && std::isdigit(static_cast<unsigned char>(text[index]))) {
        value = value * 10 + static_cast<std::size_t>(text[index] - '0');
        ++index;
    }
    return value;
}

void localize_visibility_limitation_bullets(
    std::vector<domain::InterpretationSection>& sections,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    // Map known Chinese production headings from InterpretationService (and short aliases).
    for (domain::InterpretationSection& section : sections) {
        if (section.heading == "统计结论" || section.heading == "结论") {
            section.heading =
                domain::resolve_report_text("interp.section.conclusion", language, missing_out).text;
        } else if (section.heading == "工程建议" || section.heading == "建议") {
            section.heading =
                domain::resolve_report_text("interp.section.advice", language, missing_out).text;
        } else if (section.heading == "限制与数据质量" || section.heading == "限制") {
            section.heading =
                domain::resolve_report_text(
                    "interp.section.limitations", language, missing_out).text;
        } else if (section.heading == "关键风险与限制") {
            section.heading =
                domain::resolve_report_text("interp.section.key_risks", language, missing_out).text;
        } else if (section.heading == "方法说明") {
            section.heading =
                domain::resolve_report_text("interp.section.method", language, missing_out).text;
        } else if (section.heading == "残差正态性") {
            section.heading =
                domain::resolve_report_text(
                    "interp.section.residual_normality", language, missing_out)
                    .text;
        }
        for (std::string& bullet : section.bullets) {
            // Exact diagnostic / recommendation bodies (also used under 分析限制：).
            {
                const std::string known =
                    localize_known_plain_message(bullet, language, missing_out);
                if (known != bullet) {
                    bullet = known;
                    continue;
                }
            }
            if (const auto excluded = parse_leading_count_after_prefix(bullet, "配置排除了")) {
                bullet = replace_token(
                    domain::resolve_report_text("interp.excluded_rows", language, missing_out).text,
                    "%1",
                    std::to_string(*excluded));
            } else if (const auto hidden = parse_leading_count_after_prefix(bullet, "配置隐藏了")) {
                bullet = replace_token(
                    domain::resolve_report_text("interp.hidden_rows", language, missing_out).text,
                    "%1",
                    std::to_string(*hidden));
            } else if (bullet.find("稳定性前置未验收；禁止写成过程合格判定") != std::string::npos) {
                // Keep any leading index text; replace the fixed honesty clause.
                const auto clause = domain::resolve_report_text(
                    "interp.capability_stability_blocked", language, missing_out).text;
                const auto pos = bullet.find("（稳定性前置未验收");
                if (pos != std::string::npos) {
                    bullet = bullet.substr(0, pos) + clause;
                } else {
                    bullet = clause;
                }
            } else if (bullet.find("研究/预览指数；门禁禁止写成过程合格判定") != std::string::npos) {
                const auto clause = domain::resolve_report_text(
                    "interp.johnson_gate_clause", language, missing_out).text;
                const auto pos = bullet.find("（研究/预览指数");
                if (pos != std::string::npos) {
                    bullet = bullet.substr(0, pos) + clause;
                } else {
                    bullet = clause;
                }
            } else if (bullet.find("非正态/变换路径；禁止写成过程合格判定") != std::string::npos) {
                const auto clause = domain::resolve_report_text(
                    "interp.nonnormal_gate_clause", language, missing_out).text;
                const auto pos = bullet.find("（非正态/变换路径");
                if (pos != std::string::npos) {
                    bullet = bullet.substr(0, pos) + clause;
                } else {
                    bullet = clause;
                }
            } else if (starts_with(bullet, "Cpk = ")
                       && ends_with(
                           bullet, "，低于项目提示基准 1.33，需要调查过程能力。")) {
                const std::string v = bullet.substr(
                    std::string("Cpk = ").size(),
                    bullet.size() - std::string("Cpk = ").size()
                        - std::string("，低于项目提示基准 1.33，需要调查过程能力。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.cpk_below_1_33", language, missing_out)
                        .text,
                    "%1",
                    v);
            } else if (starts_with(bullet, "Cpk = ")
                       && ends_with(
                           bullet,
                           "，达到项目提示基准 1.33；这不是已验证的过程合格结论。")) {
                const std::string v = bullet.substr(
                    std::string("Cpk = ").size(),
                    bullet.size() - std::string("Cpk = ").size()
                        - std::string(
                              "，达到项目提示基准 1.33；这不是已验证的过程合格结论。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.cpk_meets_1_33", language, missing_out)
                        .text,
                    "%1",
                    v);
            } else if (starts_with(bullet, "Ppk = ")
                       && ends_with(
                           bullet, "，整体过程表现低于 1.33 提示基准。")) {
                const std::string v = bullet.substr(
                    std::string("Ppk = ").size(),
                    bullet.size() - std::string("Ppk = ").size()
                        - std::string("，整体过程表现低于 1.33 提示基准。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.ppk_below_1_33", language, missing_out)
                        .text,
                    "%1",
                    v);
            } else if (starts_with(bullet, "Ppk = ")
                       && ends_with(
                           bullet, "，整体过程表现达到 1.33 提示基准。")) {
                const std::string v = bullet.substr(
                    std::string("Ppk = ").size(),
                    bullet.size() - std::string("Ppk = ").size()
                        - std::string("，整体过程表现达到 1.33 提示基准。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.ppk_meets_1_33", language, missing_out)
                        .text,
                    "%1",
                    v);
            } else if (starts_with(bullet, "Cpk 置信区间 [")
                       && ends_with(
                           bullet,
                           "]（Bissell 近似，公式参考）。区间描述抽样不确定性，不是合格判定。")) {
                const std::string body = bullet.substr(
                    std::string("Cpk 置信区间 [").size(),
                    bullet.size() - std::string("Cpk 置信区间 [").size()
                        - std::string(
                              "]（Bissell 近似，公式参考）。区间描述抽样不确定性，不是合格判定。")
                              .size());
                const auto comma = body.find(", ");
                if (comma != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.cpk_ci_bissell", language, missing_out)
                            .text,
                        {{"%1", body.substr(0, comma)},
                         {"%2", body.substr(comma + 2)}});
                }
            } else if (starts_with(
                           bullet, "过程能力未验证稳定性与正态性（assumption_status=")
                       && ends_with(
                           bullet, "），不能把 Cpk/Ppk 直接写成合格判定。")) {
                const std::string status = bullet.substr(
                    std::string(
                        "过程能力未验证稳定性与正态性（assumption_status=")
                        .size(),
                    bullet.size()
                        - std::string(
                              "过程能力未验证稳定性与正态性（assumption_status=")
                              .size()
                        - std::string("），不能把 Cpk/Ppk 直接写成合格判定。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.capability_assumption_not_verified", language,
                        missing_out)
                        .text,
                    "%1",
                    status);
            } else if (starts_with(bullet, "Anderson-Darling P = ")
                       && ends_with(
                           bullet, "；未拒绝正态假设不等于已证明正态分布。")) {
                const std::string p = bullet.substr(
                    std::string("Anderson-Darling P = ").size(),
                    bullet.size() - std::string("Anderson-Darling P = ").size()
                        - std::string("；未拒绝正态假设不等于已证明正态分布。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.anderson_darling_p_not_proof", language,
                        missing_out)
                        .text,
                    "%1",
                    p);
            } else if (starts_with(bullet, "Anderson-Darling = ")
                       && ends_with(
                           bullet,
                           "，在 alpha 下未拒绝残差正态假设，不能据此宣称模型合格。")) {
                const std::string ad = bullet.substr(
                    std::string("Anderson-Darling = ").size(),
                    bullet.size() - std::string("Anderson-Darling = ").size()
                        - std::string(
                              "，在 alpha 下未拒绝残差正态假设，不能据此宣称模型合格。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.regression_residual_ad_fail_to_reject", language,
                        missing_out)
                        .text,
                    "%1",
                    ad);
            } else if (starts_with(bullet, "Anderson-Darling = ")
                       && ends_with(
                           bullet,
                           "，请结合正态概率图和 P 值判断残差正态性，不能只看 R²。")) {
                const std::string ad = bullet.substr(
                    std::string("Anderson-Darling = ").size(),
                    bullet.size() - std::string("Anderson-Darling = ").size()
                        - std::string(
                              "，请结合正态概率图和 P 值判断残差正态性，不能只看 R²。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.regression_residual_ad_use_plot_p", language,
                        missing_out)
                        .text,
                    "%1",
                    ad);
            } else if (starts_with(bullet, "Johnson 变换 AD P = ")
                       && ends_with(
                           bullet,
                           "；这是变换尺度上的拟合证据，不是原始数据正态性证明。")) {
                const std::string p = bullet.substr(
                    std::string("Johnson 变换 AD P = ").size(),
                    bullet.size() - std::string("Johnson 变换 AD P = ").size()
                        - std::string(
                              "；这是变换尺度上的拟合证据，不是原始数据正态性证明。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.johnson_transform_ad_p", language, missing_out)
                        .text,
                    "%1",
                    p);
            } else if (starts_with(bullet, "gate_status=")
                       && bullet.find("：当前为研究/预览，证据类型 ")
                           != std::string::npos
                       && ends_with(
                           bullet,
                           "；未满足 golden/尾部验收前不得开放合格判定。")) {
                const auto mid = bullet.find("：当前为研究/预览，证据类型 ");
                const auto end = bullet.find(
                    "；未满足 golden/尾部验收前不得开放合格判定。");
                if (mid != std::string::npos && end != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.capability_research_preview_gate", language,
                            missing_out)
                            .text,
                        {{"%1",
                          bullet.substr(
                              std::string("gate_status=").size(),
                              mid - std::string("gate_status=").size())},
                         {"%2",
                          bullet.substr(
                              mid
                                  + std::string("：当前为研究/预览，证据类型 ")
                                        .size(),
                              end
                                  - (mid
                                     + std::string(
                                           "：当前为研究/预览，证据类型 ")
                                           .size()))}});
                }
            } else if (starts_with(bullet, "gate_status=")
                       && bullet.find("；stability_screen=") != std::string::npos
                       && ends_with(
                           bullet,
                           "）：I-MR Rule-1 / 直方图双峰 / Hartigan dip / "
                           "高斯混合门禁筛查不等于完整稳定性或单峰验收。")) {
                const auto s1 = bullet.find("；stability_screen=");
                const auto s2 = bullet.find("（OOC=");
                const auto s3 = bullet.find("）；bimodality_screen=");
                const auto s4 = bullet.find("（peaks=");
                const auto s5 = bullet.find("）；hartigan_dip=");
                const auto s6 = bullet.find("（D=");
                const auto s7 = bullet.find("）；mixture=");
                const auto s8 = bullet.find("（k=");
                const auto s9 = bullet.find("，ΔBIC=");
                const auto s10 = bullet.find(
                    "）：I-MR Rule-1 / 直方图双峰 / Hartigan dip / "
                    "高斯混合门禁筛查不等于完整稳定性或单峰验收。");
                if (s1 != std::string::npos && s2 != std::string::npos
                    && s3 != std::string::npos && s4 != std::string::npos
                    && s5 != std::string::npos && s6 != std::string::npos
                    && s7 != std::string::npos && s8 != std::string::npos
                    && s9 != std::string::npos && s10 != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.capability_multi_screen_gate", language,
                            missing_out)
                            .text,
                        {{"%10",
                          bullet.substr(
                              s9 + std::string("，ΔBIC=").size(),
                              s10
                                  - (s9 + std::string("，ΔBIC=").size()))},
                         {"%9",
                          bullet.substr(
                              s8 + std::string("（k=").size(),
                              s9 - (s8 + std::string("（k=").size()))},
                         {"%8",
                          bullet.substr(
                              s7 + std::string("）；mixture=").size(),
                              s8
                                  - (s7
                                     + std::string("）；mixture=").size()))},
                         {"%7",
                          bullet.substr(
                              s6 + std::string("（D=").size(),
                              s7 - (s6 + std::string("（D=").size()))},
                         {"%6",
                          bullet.substr(
                              s5 + std::string("）；hartigan_dip=").size(),
                              s6
                                  - (s5
                                     + std::string("）；hartigan_dip=")
                                           .size()))},
                         {"%5",
                          bullet.substr(
                              s4 + std::string("（peaks=").size(),
                              s5 - (s4 + std::string("（peaks=").size()))},
                         {"%4",
                          bullet.substr(
                              s3 + std::string("）；bimodality_screen=").size(),
                              s4
                                  - (s3
                                     + std::string("）；bimodality_screen=")
                                           .size()))},
                         {"%3",
                          bullet.substr(
                              s2 + std::string("（OOC=").size(),
                              s3 - (s2 + std::string("（OOC=").size()))},
                         {"%2",
                          bullet.substr(
                              s1 + std::string("；stability_screen=").size(),
                              s2
                                  - (s1
                                     + std::string("；stability_screen=")
                                           .size()))},
                         {"%1",
                          bullet.substr(
                              std::string("gate_status=").size(),
                              s1 - std::string("gate_status=").size())}});
                }
            } else if (starts_with(bullet, "最大类别为“")
                       && bullet.find("”，计数 ") != std::string::npos
                       && ends_with(bullet, "%。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const auto mid = bullet.find("”，计数 ");
                const auto pct = bullet.find("，单项占比 ");
                if (mid != std::string::npos && pct != std::string::npos) {
                    const std::string cat = bullet.substr(
                        std::string("最大类别为“").size(),
                        mid - std::string("最大类别为“").size());
                    std::string count = bullet.substr(
                        mid + std::string("”，计数 ").size(),
                        pct - (mid + std::string("”，计数 ").size()));
                    std::string share = bullet.substr(
                        pct + std::string("，单项占比 ").size());
                    if (ends_with(share, "%。")) {
                        share = share.substr(0, share.size() - 4);
                    }
                    if (en) {
                        if (count == "未知") {
                            count = "unknown";
                        }
                        if (share == "未知") {
                            share = "unknown";
                        }
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.pareto_largest_category", language,
                            missing_out)
                            .text,
                        {{"%1", cat}, {"%2", count}, {"%3", share}});
                }
            } else if (starts_with(bullet, "Grouping Information 使用 ")
                       && ends_with(
                           bullet,
                           " 个字母；字母来自成对显著矩阵，不改 Studentized Range 近似。")) {
                const std::string n = bullet.substr(
                    std::string("Grouping Information 使用 ").size(),
                    bullet.size()
                        - std::string("Grouping Information 使用 ").size()
                        - std::string(
                              " 个字母；字母来自成对显著矩阵，不改 Studentized Range 近似。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.anova_grouping_letters", language, missing_out)
                        .text,
                    "%1",
                    n);
            } else if (starts_with(bullet, "Durbin-Watson = ")
                       && ends_with(
                           bullet,
                           "。不能写成已证明无自相关或存在自相关。")) {
                const std::string body = bullet.substr(
                    std::string("Durbin-Watson = ").size(),
                    bullet.size() - std::string("Durbin-Watson = ").size()
                        - std::string("。不能写成已证明无自相关或存在自相关。")
                              .size());
                const auto zone = body.find("，判定区 = ");
                const auto paren = body.find("（按输入顺序与 α=0.05 近似 dL/dU）");
                if (zone != std::string::npos && paren != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.durbin_watson_summary", language, missing_out)
                            .text,
                        {{"%1", body.substr(0, zone)},
                         {"%2",
                          body.substr(
                              zone + std::string("，判定区 = ").size(),
                              paren
                                  - (zone
                                     + std::string("，判定区 = ").size()))}});
                }
            } else if (starts_with(bullet, "I-MR σ 估计方法 = ")
                       && ends_with(
                           bullet,
                           "。σ 估计变化只影响控制限宽度，不能单独写成稳定结论。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("I-MR σ 估计方法 = ").size(),
                    bullet.size() - std::string("I-MR σ 估计方法 = ").size()
                        - std::string(
                              "。σ 估计变化只影响控制限宽度，不能单独写成稳定结论。")
                              .size());
                std::string method = body;
                std::string nelson;
                const auto n_mark = body.find("；Nelson estimate 剔除过大 MR ");
                if (n_mark != std::string::npos) {
                    method = body.substr(0, n_mark);
                    std::string rest = body.substr(
                        n_mark
                        + std::string("；Nelson estimate 剔除过大 MR ").size());
                    if (ends_with(rest, " 个")) {
                        rest = rest.substr(0, rest.size() - 4);
                    }
                    nelson = en
                        ? ("; Nelson estimate excluded " + rest + " large MR(s)")
                        : ("；Nelson estimate 剔除过大 MR " + rest + " 个");
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.imr_sigma_method", language, missing_out)
                        .text,
                    {{"%1", method}, {"%2", nelson}});
            } else if (starts_with(bullet, "已按阶段列汇总估计（阶段数 = ")
                       && ends_with(
                           bullet,
                           "）；当前输出仍使用全样本估计限，不是逐阶段独立控制图。")) {
                const std::string n = bullet.substr(
                    std::string("已按阶段列汇总估计（阶段数 = ").size(),
                    bullet.size()
                        - std::string("已按阶段列汇总估计（阶段数 = ").size()
                        - std::string(
                              "）；当前输出仍使用全样本估计限，不是逐阶段独立控制图。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.imr_stage_estimates", language, missing_out)
                        .text,
                    "%1",
                    n);
            } else if ((starts_with(bullet, "Hotelling T²：")
                        || starts_with(bullet, "MEWMA：")
                        || starts_with(bullet, "广义方差 |S|："))
                       && bullet.find("，超限 = ") != std::string::npos
                       && bullet.find(
                              "。超限是统计信号提示，需要结合工艺调查，不能单独写成失控或稳定结论。")
                           != std::string::npos) {
                const bool en = language != "zh-CN" && language != "zh";
                std::string label;
                std::string rest;
                if (starts_with(bullet, "Hotelling T²：")) {
                    label = "Hotelling T²";
                    rest = bullet.substr(std::string("Hotelling T²：").size());
                } else if (starts_with(bullet, "MEWMA：")) {
                    label = "MEWMA";
                    rest = bullet.substr(std::string("MEWMA：").size());
                } else {
                    label = en ? "Generalized variance |S|" : "广义方差 |S|";
                    rest = bullet.substr(std::string("广义方差 |S|：").size());
                }
                const auto core_end = rest.find(
                    "。超限是统计信号提示，需要结合工艺调查，不能单独写成失控或稳定结论。");
                std::string core = rest.substr(0, core_end);
                std::string note_zh = rest.substr(
                    core_end
                    + std::string(
                          "。超限是统计信号提示，需要结合工艺调查，不能单独写成失控或稳定结论。")
                          .size());
                const auto m_mark = core.find("m = ");
                const auto p_mark = core.find("，p = ");
                const auto ooc_mark = core.find("，超限 = ");
                const auto sub_mark = core.find("，子组数 = ");
                if (m_mark != std::string::npos && p_mark != std::string::npos
                    && ooc_mark != std::string::npos) {
                    const std::string m = core.substr(
                        m_mark + std::string("m = ").size(),
                        p_mark - (m_mark + std::string("m = ").size()));
                    std::string p;
                    std::string sub_part;
                    if (sub_mark != std::string::npos && sub_mark < ooc_mark) {
                        p = core.substr(
                            p_mark + std::string("，p = ").size(),
                            sub_mark - (p_mark + std::string("，p = ").size()));
                        const std::string sub = core.substr(
                            sub_mark + std::string("，子组数 = ").size(),
                            ooc_mark
                                - (sub_mark + std::string("，子组数 = ").size()));
                        sub_part = en ? (", subgroups = " + sub)
                                      : ("，子组数 = " + sub);
                    } else {
                        p = core.substr(
                            p_mark + std::string("，p = ").size(),
                            ooc_mark - (p_mark + std::string("，p = ").size()));
                    }
                    const std::string ooc =
                        core.substr(ooc_mark + std::string("，超限 = ").size());
                    std::string note_id;
                    if (note_zh.find("PCA 经验分位") != std::string::npos) {
                        note_id = "interp.multivariate_spc_hotelling_note";
                    } else if (note_zh.find("ARL 仿真") != std::string::npos) {
                        note_id = "interp.multivariate_spc_mewma_note";
                    } else if (note_zh.find("Montgomery") != std::string::npos) {
                        note_id = "interp.multivariate_spc_gs_note";
                    }
                    const std::string note =
                        note_id.empty()
                            ? note_zh
                            : domain::resolve_report_text(
                                  note_id, language, missing_out)
                                  .text;
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.multivariate_spc_summary", language,
                            missing_out)
                            .text,
                        {{"%1", label},
                         {"%2", m},
                         {"%3", p},
                         {"%4", sub_part},
                         {"%5", ooc},
                         {"%6", note}});
                }
            } else if (bullet.find(" Sigma Z = ") != std::string::npos
                       && (ends_with(
                               bullet, "，存在过度离散，传统控制限可能过窄。")
                           || ends_with(
                               bullet, "，控制限已按离散程度进行调整。"))) {
                const bool en = language != "zh-CN" && language != "zh";
                const auto mid = bullet.find(" Sigma Z = ");
                std::string chart = bullet.substr(0, mid);
                if (en && chart == "当前") {
                    chart = "Current";
                }
                const bool overdisp =
                    ends_with(bullet, "，存在过度离散，传统控制限可能过窄。");
                const std::string suffix =
                    overdisp ? "，存在过度离散，传统控制限可能过窄。"
                             : "，控制限已按离散程度进行调整。";
                const std::string z = bullet.substr(
                    mid + std::string(" Sigma Z = ").size(),
                    bullet.size() - (mid + std::string(" Sigma Z = ").size())
                        - suffix.size());
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        overdisp ? "interp.laney_sigma_z_overdisp"
                                 : "interp.laney_sigma_z_adjusted",
                        language,
                        missing_out)
                        .text,
                    {{"%1", chart}, {"%2", z}});
            } else if (starts_with(bullet, "前 ")
                       && bullet.find(" 个类别累计占比 ") != std::string::npos
                       && ends_with(
                           bullet,
                           "%；应优先结合现场原因验证，而不是直接假设存在 80/20 规律。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const auto mid = bullet.find(" 个类别累计占比 ");
                const auto end = bullet.find(
                    "%；应优先结合现场原因验证，而不是直接假设存在 80/20 规律。");
                if (mid != std::string::npos && end != std::string::npos) {
                    const std::string n = bullet.substr(
                        std::string("前 ").size(),
                        mid - std::string("前 ").size());
                    std::string pct = bullet.substr(
                        mid + std::string(" 个类别累计占比 ").size(),
                        end - (mid + std::string(" 个类别累计占比 ").size()));
                    if (en && pct == "未知") {
                        pct = "unknown";
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.pareto_top_cumulative", language, missing_out)
                            .text,
                        {{"%1", n}, {"%2", pct}});
                }
            } else if (starts_with(bullet, "分析错误，以下结论不可用于决策：")) {
                const std::string raw =
                    bullet.substr(std::string("分析错误，以下结论不可用于决策：").size());
                const std::string detail =
                    localize_known_plain_message(raw, language, missing_out);
                // Only rewrite when the detail is known — avoid EN prefix + ZH body.
                if (detail != raw) {
                    bullet = replace_token(
                        domain::resolve_report_text(
                            "interp.analysis_error_prefix", language, missing_out)
                            .text,
                        "%1",
                        detail);
                }
            } else if (starts_with(bullet, "分析限制：")) {
                const std::string raw =
                    bullet.substr(std::string("分析限制：").size());
                const std::string detail =
                    localize_known_plain_message(raw, language, missing_out);
                if (detail != raw) {
                    bullet = replace_token(
                        domain::resolve_report_text(
                            "interp.analysis_limitation_prefix", language, missing_out)
                            .text,
                        "%1",
                        detail);
                }
            } else if (starts_with(bullet, "Weibull 形状参数 β = ")
                       && ends_with(bullet, "。")) {
                const std::string body = bullet.substr(
                    std::string("Weibull 形状参数 β = ").size());
                std::string value = body;
                std::string tip_id;
                if (ends_with(body, "，提示失效率随时间上升。")) {
                    tip_id = "interp.weibull_shape_tip_rising";
                    value = body.substr(
                        0,
                        body.size()
                            - std::string("，提示失效率随时间上升。").size());
                } else if (ends_with(
                               body, "，提示早期失效型失效率随时间下降。")) {
                    tip_id = "interp.weibull_shape_tip_falling";
                    value = body.substr(
                        0,
                        body.size()
                            - std::string("，提示早期失效型失效率随时间下降。")
                                  .size());
                } else if (ends_with(body, "，接近恒定失效率。")) {
                    tip_id = "interp.weibull_shape_tip_constant";
                    value = body.substr(
                        0, body.size() - std::string("，接近恒定失效率。").size());
                }
                const bool bare_beta =
                    tip_id.empty() && ends_with(body, "。")
                    && body.find("，") == std::string::npos;
                if (tip_id.empty() && !bare_beta) {
                    // Unknown tip suffix: leave ZH unchanged rather than
                    // half-translating "Weibull shape parameter β = …，提示…".
                } else {
                    if (bare_beta) {
                        value = body.substr(
                            0, body.size() - std::string("。").size());
                    }
                    std::string tip;
                    if (!tip_id.empty()) {
                        tip = domain::resolve_report_text(
                                  tip_id, language, missing_out)
                                  .text;
                    } else {
                        tip = (language == "zh-CN" || language == "zh") ? "。"
                                                                       : ".";
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.weibull_shape_line", language, missing_out)
                            .text,
                        {{"%1", value}, {"%2", tip}});
                }
            } else if (starts_with(bullet, "阈值 λ = ")
                       && ends_with(
                           bullet,
                           "；百分位寿命按 t_p = λ + α[-ln(1-p)]^(1/β) 计算。")) {
                const std::string v = bullet.substr(
                    std::string("阈值 λ = ").size(),
                    bullet.size() - std::string("阈值 λ = ").size()
                        - std::string(
                              "；百分位寿命按 t_p = λ + α[-ln(1-p)]^(1/β) 计算。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.weibull_threshold_percentile", language,
                        missing_out)
                        .text,
                    "%1",
                    v);
            } else if (starts_with(bullet, "两参数指数估计了阈值 λ = ")
                       && ends_with(bullet, " 后的恒定失效率。")) {
                const std::string v = bullet.substr(
                    std::string("两参数指数估计了阈值 λ = ").size(),
                    bullet.size()
                        - std::string("两参数指数估计了阈值 λ = ").size()
                        - std::string(" 后的恒定失效率。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.exp_two_param_threshold", language, missing_out)
                        .text,
                    "%1",
                    v);
            } else if (starts_with(bullet, "三参数对数正态估计了阈值 λ = ")
                       && ends_with(
                           bullet,
                           "；分位寿命按 λ + exp(μ + σ Φ⁻¹(p)) 计算。")) {
                const std::string v = bullet.substr(
                    std::string("三参数对数正态估计了阈值 λ = ").size(),
                    bullet.size()
                        - std::string("三参数对数正态估计了阈值 λ = ").size()
                        - std::string(
                              "；分位寿命按 λ + exp(μ + σ Φ⁻¹(p)) 计算。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.lognormal_three_param_threshold", language,
                        missing_out)
                        .text,
                    "%1",
                    v);
            } else if (starts_with(bullet, "证据类型 ")
                       && ends_with(bullet, "；非 vendor_oracle。")) {
                const std::string evidence =
                    bullet.substr(std::string("证据类型 ").size(),
                                  bullet.size() - std::string("证据类型 ").size()
                                      - std::string("；非 vendor_oracle。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.evidence_type_not_vendor", language, missing_out)
                        .text,
                    "%1",
                    evidence);
            } else if (starts_with(bullet, "Kaplan-Meier 曲线基于 ")) {
                const std::string rest =
                    bullet.substr(std::string("Kaplan-Meier 曲线基于 ").size());
                const auto time_end = rest.find(" 个时间点；删失数合计 ");
                const auto censor_end = rest.rfind("。");
                if (time_end != std::string::npos && censor_end != std::string::npos
                    && censor_end > time_end) {
                    const std::string n1 = rest.substr(0, time_end);
                    const std::string n2 = rest.substr(
                        time_end + std::string(" 个时间点；删失数合计 ").size(),
                        censor_end
                            - (time_end + std::string(" 个时间点；删失数合计 ").size()));
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.km_curve_summary", language, missing_out)
                            .text,
                        {{"%1", n1}, {"%2", n2}});
                }
            } else if (starts_with(bullet, "保修摘要：T_w = ")) {
                const std::string body =
                    bullet.substr(std::string("保修摘要：T_w = ").size());
                const auto unit_sep = body.find(' ');
                const auto exp_mark = body.find("，暴露量 = ");
                const auto claims_mark = body.find("，claims/1000 = ");
                const auto qty_mark = body.find("（口径 = ");
                const auto qty_end = body.rfind("）。");
                if (unit_sep != std::string::npos && exp_mark != std::string::npos
                    && claims_mark != std::string::npos && qty_mark != std::string::npos
                    && qty_end != std::string::npos && unit_sep < exp_mark
                    && exp_mark < claims_mark && claims_mark < qty_mark
                    && qty_mark < qty_end) {
                    const std::string tw = body.substr(0, unit_sep);
                    const std::string unit =
                        body.substr(unit_sep + 1, exp_mark - (unit_sep + 1));
                    const std::string exposure = body.substr(
                        exp_mark + std::string("，暴露量 = ").size(),
                        claims_mark - (exp_mark + std::string("，暴露量 = ").size()));
                    const std::string claims = body.substr(
                        claims_mark + std::string("，claims/1000 = ").size(),
                        qty_mark - (claims_mark + std::string("，claims/1000 = ").size()));
                    const std::string quantity = body.substr(
                        qty_mark + std::string("（口径 = ").size(),
                        qty_end - (qty_mark + std::string("（口径 = ").size()));
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.warranty_summary_line", language, missing_out)
                            .text,
                        {{"%1", tw},
                         {"%2", unit},
                         {"%3", exposure},
                         {"%4", claims},
                         {"%5", quantity}});
                }

            } else if (starts_with(bullet, "在 α = ")
                       && bullet.find(" 下显著的项：") != std::string::npos
                       && ends_with(bullet, "。")) {
                const auto mid = bullet.find(" 下显著的项：");
                const std::string alpha = bullet.substr(
                    std::string("在 α = ").size(),
                    mid - std::string("在 α = ").size());
                const std::string terms = localize_zh_payload_punctuation(
                    bullet.substr(
                        mid + std::string(" 下显著的项：").size(),
                        bullet.size() - (mid + std::string(" 下显著的项：").size())
                            - std::string("。").size()),
                    language);
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.doe_significant_terms", language, missing_out)
                        .text,
                    {{"%1", alpha}, {"%2", terms}});
            } else if (starts_with(bullet, "设计包含 ")
                       && bullet.find(" 个因子、") != std::string::npos
                       && bullet.find(" 个中心点；") != std::string::npos) {
                const auto fmark = bullet.find(" 个因子、");
                const auto cmark = bullet.find(" 个中心点；");
                if (fmark != std::string::npos && cmark != std::string::npos
                    && fmark < cmark) {
                    const std::string n_factors = bullet.substr(
                        std::string("设计包含 ").size(),
                        fmark - std::string("设计包含 ").size());
                    const std::string n_centers = bullet.substr(
                        fmark + std::string(" 个因子、").size(),
                        cmark - (fmark + std::string(" 个因子、").size()));
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.doe_design_factors_centers", language, missing_out)
                            .text,
                        {{"%1", n_factors}, {"%2", n_centers}});
                }
            } else if (starts_with(bullet, "设计类型 = ")
                       && bullet.find("；运行数 = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。可将设计矩阵写入工作表后用 DOE 响应分析衔接。")) {
                const std::string body = bullet.substr(
                    std::string("设计类型 = ").size(),
                    bullet.size() - std::string("设计类型 = ").size()
                        - std::string(
                              "。可将设计矩阵写入工作表后用 DOE 响应分析衔接。")
                              .size());
                const auto run_mark = body.find("；运行数 = ");
                if (run_mark != std::string::npos) {
                    const bool en = language != "zh-CN" && language != "zh";
                    const std::string kind = body.substr(0, run_mark);
                    std::string runs = body.substr(
                        run_mark + std::string("；运行数 = ").size());
                    std::string resolution_part;
                    const auto res_mark = runs.find("；分辨度 = ");
                    if (res_mark != std::string::npos) {
                        const std::string res_val = runs.substr(
                            res_mark + std::string("；分辨度 = ").size());
                        runs = runs.substr(0, res_mark);
                        resolution_part = en ? ("; resolution = " + res_val)
                                             : ("；分辨度 = " + res_val);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.doe_design_type_runs", language, missing_out)
                            .text,
                        {{"%1", kind}, {"%2", runs}, {"%3", resolution_part}});
                }
            } else if (starts_with(bullet, "生成器：")
                       && ends_with(
                           bullet, "。别名结构仅说明混淆模式，不是显著性结论。")) {
                const std::string generators = bullet.substr(
                    std::string("生成器：").size(),
                    bullet.size() - std::string("生成器：").size()
                        - std::string(
                              "。别名结构仅说明混淆模式，不是显著性结论。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.doe_generator_alias", language, missing_out)
                        .text,
                    "%1",
                    generators);
            } else if (starts_with(bullet, "多响应 Desirability 优化覆盖 ")
                       && bullet.find(" 个响应（") != std::string::npos
                       && ends_with(bullet, "）；总体 D 为几何平均。")) {
                const auto mid = bullet.find(" 个响应（");
                if (mid != std::string::npos) {
                    const std::string count = bullet.substr(
                        std::string("多响应 Desirability 优化覆盖 ").size(),
                        mid - std::string("多响应 Desirability 优化覆盖 ").size());
                    const std::string names = localize_zh_payload_punctuation(
                        bullet.substr(
                            mid + std::string(" 个响应（").size(),
                            bullet.size() - (mid + std::string(" 个响应（").size())
                                - std::string("）；总体 D 为几何平均。").size()),
                        language);
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.doe_desirability_multi", language, missing_out)
                            .text,
                        {{"%1", count}, {"%2", names}});
                }
            } else if (starts_with(bullet, "最佳候选总体 Desirability = ")
                       && ends_with(
                           bullet, "；这是在编码 ±1 设计空间内枚举得到的结果。")) {
                const std::string value = bullet.substr(
                    std::string("最佳候选总体 Desirability = ").size(),
                    bullet.size()
                        - std::string("最佳候选总体 Desirability = ").size()
                        - std::string(
                              "；这是在编码 ±1 设计空间内枚举得到的结果。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.doe_desirability_best", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "RSM 二次模型（编码单位）：因子数 = ")
                       && bullet.find("；项数 = ") != std::string::npos
                       && ends_with(bullet, "。")) {
                const std::string body = bullet.substr(
                    std::string("RSM 二次模型（编码单位）：因子数 = ").size(),
                    bullet.size()
                        - std::string("RSM 二次模型（编码单位）：因子数 = ").size()
                        - std::string("。").size());
                const auto term_mark = body.find("；项数 = ");
                if (term_mark != std::string::npos) {
                    const bool en = language != "zh-CN" && language != "zh";
                    const std::string factors = body.substr(0, term_mark);
                    std::string terms = body.substr(
                        term_mark + std::string("；项数 = ").size());
                    std::string r2_part;
                    const auto r2_mark = terms.find("；R² = ");
                    if (r2_mark != std::string::npos) {
                        const std::string r2_val = terms.substr(
                            r2_mark + std::string("；R² = ").size());
                        terms = terms.substr(0, r2_mark);
                        r2_part = en ? ("; R² = " + r2_val) : ("；R² = " + r2_val);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.rsm_quadratic_summary", language, missing_out)
                            .text,
                        {{"%1", factors}, {"%2", terms}, {"%3", r2_part}});
                }
            } else if (starts_with(bullet, "当前 |t| 最大项 = ")
                       && ends_with(
                           bullet, "（仅描述相对大小，不是工程最优）。")) {
                const std::string term = bullet.substr(
                    std::string("当前 |t| 最大项 = ").size(),
                    bullet.size() - std::string("当前 |t| 最大项 = ").size()
                        - std::string("（仅描述相对大小，不是工程最优）。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.rsm_largest_abs_t", language, missing_out)
                        .text,
                    "%1",
                    term);
            } else if (starts_with(bullet, "设计来源 ID = ")
                       && bullet.find("；编码模式 = ") != std::string::npos
                       && ends_with(bullet, "。")) {
                const std::string body = bullet.substr(
                    std::string("设计来源 ID = ").size(),
                    bullet.size() - std::string("设计来源 ID = ").size()
                        - std::string("。").size());
                const auto mode_mark = body.find("；编码模式 = ");
                if (mode_mark != std::string::npos) {
                    const bool en = language != "zh-CN" && language != "zh";
                    std::string source_id = body.substr(0, mode_mark);
                    const std::string mode = body.substr(
                        mode_mark + std::string("；编码模式 = ").size());
                    std::string kind_part;
                    const std::string open_paren = "（";
                    const std::string close_paren = "）";
                    if (source_id.size() >= open_paren.size() + close_paren.size()
                        && source_id.compare(
                               source_id.size() - close_paren.size(),
                               close_paren.size(),
                               close_paren)
                            == 0) {
                        const auto open = source_id.rfind(open_paren);
                        if (open != std::string::npos) {
                            const std::string kind = source_id.substr(
                                open + open_paren.size(),
                                source_id.size() - open - open_paren.size()
                                    - close_paren.size());
                            source_id = source_id.substr(0, open);
                            kind_part = en ? (" (" + kind + ")")
                                           : ("（" + kind + "）");
                        }
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.rsm_design_source", language, missing_out)
                            .text,
                        {{"%1", source_id}, {"%2", kind_part}, {"%3", mode}});
                }
            } else if (starts_with(bullet, "失拟检验可用（纯误差 DF = ")
                       && bullet.find("；失拟 DF = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "）；证据类型 formula_reference，不是 vendor_oracle。")) {
                const std::string body = bullet.substr(
                    std::string("失拟检验可用（纯误差 DF = ").size(),
                    bullet.size()
                        - std::string("失拟检验可用（纯误差 DF = ").size()
                        - std::string(
                              "）；证据类型 formula_reference，不是 vendor_oracle。")
                              .size());
                const auto lof_mark = body.find("；失拟 DF = ");
                if (lof_mark != std::string::npos) {
                    const bool en = language != "zh-CN" && language != "zh";
                    const std::string pe_df = body.substr(0, lof_mark);
                    std::string lof_df = body.substr(
                        lof_mark + std::string("；失拟 DF = ").size());
                    std::string p_part;
                    const auto p_mark = lof_df.find("；P = ");
                    if (p_mark != std::string::npos) {
                        const std::string p_val = lof_df.substr(
                            p_mark + std::string("；P = ").size());
                        lof_df = lof_df.substr(0, p_mark);
                        p_part = en ? ("; P = " + p_val) : ("；P = " + p_val);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.rsm_lof_available", language, missing_out)
                            .text,
                        {{"%1", pe_df}, {"%2", lof_df}, {"%3", p_part}});
                }
            } else if (starts_with(bullet, "量具稳定性图发现 ")
                       && ends_with(bullet, " 个超限点，统计上存在失控信号。")) {
                const std::string count = bullet.substr(
                    std::string("量具稳定性图发现 ").size(),
                    bullet.size() - std::string("量具稳定性图发现 ").size()
                        - std::string(" 个超限点，统计上存在失控信号。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.gage_stability_ooc", language, missing_out)
                        .text,
                    "%1",
                    count);
            } else if (starts_with(bullet, "偏倚回归斜率 = ")
                       && bullet.find("，低/高参考点偏倚 = ") != std::string::npos
                       && ends_with(bullet, "。")) {
                const auto mid = bullet.find("，低/高参考点偏倚 = ");
                const auto slash = bullet.find(" / ", mid);
                if (mid != std::string::npos && slash != std::string::npos) {
                    const std::string slope = bullet.substr(
                        std::string("偏倚回归斜率 = ").size(),
                        mid - std::string("偏倚回归斜率 = ").size());
                    const std::string low = bullet.substr(
                        mid + std::string("，低/高参考点偏倚 = ").size(),
                        slash - (mid + std::string("，低/高参考点偏倚 = ").size()));
                    const std::string high = bullet.substr(
                        slash + std::string(" / ").size(),
                        bullet.size() - (slash + std::string(" / ").size())
                            - std::string("。").size());
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.msa_bias_slope_endpoints", language, missing_out)
                            .text,
                        {{"%1", slope}, {"%2", low}, {"%3", high}});
                }
            } else if (starts_with(bullet, "EMP Crossed：ICC(with interaction) = ")
                       && bullet.find("，分级 = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。这是 Wheeler 监控能力分级，不是 AIAG 合格判定，不能写成量具合格。")) {
                const std::string body = bullet.substr(
                    std::string("EMP Crossed：ICC(with interaction) = ").size(),
                    bullet.size()
                        - std::string("EMP Crossed：ICC(with interaction) = ").size()
                        - std::string(
                              "。这是 Wheeler 监控能力分级，不是 AIAG 合格判定，不能写成量具合格。")
                              .size());
                const auto mid = body.find("，分级 = ");
                if (mid != std::string::npos) {
                    const std::string icc = body.substr(0, mid);
                    const std::string grade = body.substr(
                        mid + std::string("，分级 = ").size());
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.msa_emp_icc", language, missing_out)
                            .text,
                        {{"%1", icc}, {"%2", grade}});
                }
            } else if (starts_with(bullet, "Probable Error = ")
                       && ends_with(
                           bullet,
                           "；用于对照测量增量是否物理合理，不是公差合格证明。")) {
                const std::string value = bullet.substr(
                    std::string("Probable Error = ").size(),
                    bullet.size() - std::string("Probable Error = ").size()
                        - std::string(
                              "；用于对照测量增量是否物理合理，不是公差合格证明。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.msa_probable_error", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "ndc = ")
                       && ends_with(
                           bullet, "，小于 5，提示测量系统分辨力需要调查。")) {
                const std::string value = bullet.substr(
                    std::string("ndc = ").size(),
                    bullet.size() - std::string("ndc = ").size()
                        - std::string("，小于 5，提示测量系统分辨力需要调查。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.msa_ndc_low", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "ndc = ")
                       && ends_with(
                           bullet,
                           "；ndc 只描述当前研究中零件间变异相对 Gage 变异的分辨力。")) {
                const std::string value = bullet.substr(
                    std::string("ndc = ").size(),
                    bullet.size() - std::string("ndc = ").size()
                        - std::string(
                              "；ndc 只描述当前研究中零件间变异相对 Gage 变异的分辨力。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.msa_ndc_ok", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "Total Gage R&R %Study Var = ")
                       && ends_with(
                           bullet,
                           "；%Contribution 与 %Study Var 口径不同，不能混用。")) {
                const std::string value = bullet.substr(
                    std::string("Total Gage R&R %Study Var = ").size(),
                    bullet.size()
                        - std::string("Total Gage R&R %Study Var = ").size()
                        - std::string(
                              "；%Contribution 与 %Study Var 口径不同，不能混用。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.msa_gage_study_var", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "Average Bias = ")
                       && ends_with(bullet, "。")
                       && bullet.find(" p = ") == std::string::npos) {
                const std::string value = bullet.substr(
                    std::string("Average Bias = ").size(),
                    bullet.size() - std::string("Average Bias = ").size()
                        - std::string("。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.msa_average_bias", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "Average Bias p = ")
                       && ends_with(bullet, "。")) {
                const std::string value = bullet.substr(
                    std::string("Average Bias p = ").size(),
                    bullet.size() - std::string("Average Bias p = ").size()
                        - std::string("。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.msa_average_bias_p", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "Constant p = ")
                       && ends_with(bullet, "。")) {
                const std::string value = bullet.substr(
                    std::string("Constant p = ").size(),
                    bullet.size() - std::string("Constant p = ").size()
                        - std::string("。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.msa_constant_p", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "%Linearity = ")
                       && ends_with(
                           bullet, "；基于用户提供的过程变差（6σ）。")) {
                const std::string value = bullet.substr(
                    std::string("%Linearity = ").size(),
                    bullet.size() - std::string("%Linearity = ").size()
                        - std::string("；基于用户提供的过程变差（6σ）。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.msa_percent_linearity", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "偏倚检验 P = ")
                       && ends_with(
                           bullet, "，与参考值差异具有统计证据。")) {
                const std::string value = bullet.substr(
                    std::string("偏倚检验 P = ").size(),
                    bullet.size() - std::string("偏倚检验 P = ").size()
                        - std::string("，与参考值差异具有统计证据。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.msa_bias_p_evidence", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "偏倚检验 P = ")
                       && ends_with(
                           bullet, "，未发现与参考值差异的统计证据。")) {
                const std::string value = bullet.substr(
                    std::string("偏倚检验 P = ").size(),
                    bullet.size() - std::string("偏倚检验 P = ").size()
                        - std::string("，未发现与参考值差异的统计证据。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.msa_bias_p_no_evidence", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "Cgk = ")
                       && ends_with(
                           bullet,
                           "；应结合组织规定的能力门槛解释，而非套用单一通用阈值。")) {
                const std::string value = bullet.substr(
                    std::string("Cgk = ").size(),
                    bullet.size() - std::string("Cgk = ").size()
                        - std::string(
                              "；应结合组织规定的能力门槛解释，而非套用单一通用阈值。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.msa_cgk", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "%Tolerance = ")
                       && ends_with(
                           bullet,
                           "；请与产品公差及风险等级核对后决定校准、维修或放行策略。")) {
                const std::string value = bullet.substr(
                    std::string("%Tolerance = ").size(),
                    bullet.size() - std::string("%Tolerance = ").size()
                        - std::string(
                              "；请与产品公差及风险等级核对后决定校准、维修或放行策略。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.msa_tolerance_percent", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (bullet.find(" 使用 ") != std::string::npos
                       && bullet.find(" 近似") != std::string::npos
                       && ends_with(
                           bullet,
                           "。未拒绝原假设不能证明两组或各组分布相同。")) {
                const std::string body = bullet.substr(
                    0,
                    bullet.size()
                        - std::string(
                              "。未拒绝原假设不能证明两组或各组分布相同。")
                              .size());
                const auto use_mark = body.find(" 使用 ");
                const auto approx_mark = body.find(" 近似", use_mark);
                if (use_mark != std::string::npos
                    && approx_mark != std::string::npos
                    && approx_mark > use_mark) {
                    const bool en = language != "zh-CN" && language != "zh";
                    const std::string method = body.substr(0, use_mark);
                    const std::string approx = body.substr(
                        use_mark + std::string(" 使用 ").size(),
                        approx_mark - (use_mark + std::string(" 使用 ").size()));
                    std::string rest = body.substr(
                        approx_mark + std::string(" 近似").size());
                    std::string p_part;
                    std::string ties_part;
                    const auto p_mark = rest.find("，P = ");
                    const auto ties_mark = rest.find("，已做 ties 修正");
                    if (p_mark != std::string::npos) {
                        std::string p_val;
                        if (ties_mark != std::string::npos && ties_mark > p_mark) {
                            p_val = rest.substr(
                                p_mark + std::string("，P = ").size(),
                                ties_mark - (p_mark + std::string("，P = ").size()));
                        } else {
                            p_val = rest.substr(
                                p_mark + std::string("，P = ").size());
                        }
                        p_part = en ? (", P = " + p_val) : ("，P = " + p_val);
                    }
                    if (ties_mark != std::string::npos) {
                        ties_part = en ? (", with ties correction")
                                       : ("，已做 ties 修正");
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.nonparam_method_approx", language, missing_out)
                            .text,
                        {{"%1", method},
                         {"%2", approx},
                         {"%3", p_part},
                         {"%4", ties_part}});
                }
            } else if (starts_with(bullet, "Walsh 估计中位数为 ")
                       && bullet.find("，置信区间 [") != std::string::npos
                       && ends_with(
                           bullet,
                           "]。区间只描述中位数的可能范围，不能写成已证明等于假设值。")) {
                const std::string body = bullet.substr(
                    std::string("Walsh 估计中位数为 ").size(),
                    bullet.size() - std::string("Walsh 估计中位数为 ").size()
                        - std::string(
                              "]。区间只描述中位数的可能范围，不能写成已证明等于假设值。")
                              .size());
                const auto ci_mark = body.find("，置信区间 [");
                const auto comma = body.find(", ", ci_mark);
                if (ci_mark != std::string::npos && comma != std::string::npos) {
                    const std::string est = body.substr(0, ci_mark);
                    const std::string lower = body.substr(
                        ci_mark + std::string("，置信区间 [").size(),
                        comma - (ci_mark + std::string("，置信区间 [").size()));
                    const std::string upper = body.substr(comma + 2);
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.nonparam_walsh_ci", language, missing_out)
                            .text,
                        {{"%1", est}, {"%2", lower}, {"%3", upper}});
                }
            } else if (starts_with(bullet, "Walsh 估计中位数为 ")
                       && bullet.find("，置信下界 ") != std::string::npos
                       && ends_with(bullet, "。")) {
                const auto mid = bullet.find("，置信下界 ");
                const std::string est = bullet.substr(
                    std::string("Walsh 估计中位数为 ").size(),
                    mid - std::string("Walsh 估计中位数为 ").size());
                const std::string lower = bullet.substr(
                    mid + std::string("，置信下界 ").size(),
                    bullet.size() - (mid + std::string("，置信下界 ").size())
                        - std::string("。").size());
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.nonparam_walsh_lower", language, missing_out)
                        .text,
                    {{"%1", est}, {"%2", lower}});
            } else if (starts_with(bullet, "Walsh 估计中位数为 ")
                       && bullet.find("，置信上界 ") != std::string::npos
                       && ends_with(bullet, "。")) {
                const auto mid = bullet.find("，置信上界 ");
                const std::string est = bullet.substr(
                    std::string("Walsh 估计中位数为 ").size(),
                    mid - std::string("Walsh 估计中位数为 ").size());
                const std::string upper = bullet.substr(
                    mid + std::string("，置信上界 ").size(),
                    bullet.size() - (mid + std::string("，置信上界 ").size())
                        - std::string("。").size());
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.nonparam_walsh_upper", language, missing_out)
                        .text,
                    {{"%1", est}, {"%2", upper}});
            } else if (starts_with(bullet, "Walsh 估计中位数为 ")
                       && ends_with(
                           bullet, "。点估计不能写成已证明等于假设值。")) {
                const std::string est = bullet.substr(
                    std::string("Walsh 估计中位数为 ").size(),
                    bullet.size() - std::string("Walsh 估计中位数为 ").size()
                        - std::string("。点估计不能写成已证明等于假设值。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.nonparam_walsh_point", language, missing_out)
                        .text,
                    "%1",
                    est);
            } else if (starts_with(bullet, "位置差异估计为 ")
                       && bullet.find("，置信区间 [") != std::string::npos
                       && ends_with(
                           bullet, "]。区间只描述位置差的可能范围。")) {
                const std::string body = bullet.substr(
                    std::string("位置差异估计为 ").size(),
                    bullet.size() - std::string("位置差异估计为 ").size()
                        - std::string("]。区间只描述位置差的可能范围。").size());
                const auto ci_mark = body.find("，置信区间 [");
                const auto comma = body.find(", ", ci_mark);
                if (ci_mark != std::string::npos && comma != std::string::npos) {
                    const std::string est = body.substr(0, ci_mark);
                    const std::string lower = body.substr(
                        ci_mark + std::string("，置信区间 [").size(),
                        comma - (ci_mark + std::string("，置信区间 [").size()));
                    const std::string upper = body.substr(comma + 2);
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.nonparam_location_diff_ci", language,
                            missing_out)
                            .text,
                        {{"%1", est}, {"%2", lower}, {"%3", upper}});
                }
            } else if (starts_with(bullet, "位置差异估计为 ")
                       && bullet.find("，置信下界 ") != std::string::npos
                       && ends_with(bullet, "。")) {
                const auto mid = bullet.find("，置信下界 ");
                const std::string est = bullet.substr(
                    std::string("位置差异估计为 ").size(),
                    mid - std::string("位置差异估计为 ").size());
                const std::string lower = bullet.substr(
                    mid + std::string("，置信下界 ").size(),
                    bullet.size() - (mid + std::string("，置信下界 ").size())
                        - std::string("。").size());
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.nonparam_location_diff_lower", language,
                        missing_out)
                        .text,
                    {{"%1", est}, {"%2", lower}});
            } else if (starts_with(bullet, "位置差异估计为 ")
                       && bullet.find("，置信上界 ") != std::string::npos
                       && ends_with(bullet, "。")) {
                const auto mid = bullet.find("，置信上界 ");
                const std::string est = bullet.substr(
                    std::string("位置差异估计为 ").size(),
                    mid - std::string("位置差异估计为 ").size());
                const std::string upper = bullet.substr(
                    mid + std::string("，置信上界 ").size(),
                    bullet.size() - (mid + std::string("，置信上界 ").size())
                        - std::string("。").size());
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.nonparam_location_diff_upper", language,
                        missing_out)
                        .text,
                    {{"%1", est}, {"%2", upper}});
            } else if (starts_with(bullet, "箱线图与个体值图基于 ")
                       && ends_with(
                           bullet, " 个有效观测，缺失单元格未进入图形。")) {
                const std::string count = bullet.substr(
                    std::string("箱线图与个体值图基于 ").size(),
                    bullet.size() - std::string("箱线图与个体值图基于 ").size()
                        - std::string(" 个有效观测，缺失单元格未进入图形。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.nonparam_boxplot_n", language, missing_out)
                        .text,
                    "%1",
                    count);
            } else if (starts_with(bullet, "Dunn–Bonferroni 成对比较共 ")
                       && ends_with(
                           bullet,
                           " 对；Grouping 字母只反映该规则下的显著矩阵，不能写成组间已证明相同或不同。")) {
                const std::string count = bullet.substr(
                    std::string("Dunn–Bonferroni 成对比较共 ").size(),
                    bullet.size()
                        - std::string("Dunn–Bonferroni 成对比较共 ").size()
                        - std::string(
                              " 对；Grouping 字母只反映该规则下的显著矩阵，不能写成组间已证明相同或不同。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.nonparam_dunn_pairs", language, missing_out)
                        .text,
                    "%1",
                    count);
            } else if (starts_with(bullet, "Steel–Dwass（近似）成对比较共 ")
                       && ends_with(
                           bullet,
                           " 对；Grouping 字母只反映该规则下的显著矩阵，不能写成组间已证明相同或不同。")) {
                const std::string count = bullet.substr(
                    std::string("Steel–Dwass（近似）成对比较共 ").size(),
                    bullet.size()
                        - std::string("Steel–Dwass（近似）成对比较共 ").size()
                        - std::string(
                              " 对；Grouping 字母只反映该规则下的显著矩阵，不能写成组间已证明相同或不同。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.nonparam_steel_dwass_pairs", language, missing_out)
                        .text,
                    "%1",
                    count);
            } else if (starts_with(bullet, "Nemenyi（近似）成对比较共 ")
                       && ends_with(
                           bullet,
                           " 对；Grouping 字母只反映该规则下的显著矩阵，不能写成处理间已证明相同或不同。")) {
                const std::string count = bullet.substr(
                    std::string("Nemenyi（近似）成对比较共 ").size(),
                    bullet.size()
                        - std::string("Nemenyi（近似）成对比较共 ").size()
                        - std::string(
                              " 对；Grouping 字母只反映该规则下的显著矩阵，不能写成处理间已证明相同或不同。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.nonparam_nemenyi_pairs", language, missing_out)
                        .text,
                    "%1",
                    count);
            } else if (starts_with(bullet, "Friedman S（调整后）= ")
                       && bullet.find("，P = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。这只陈述区组设计下处理间秩差异证据，不能写成已证明相同或不同。")) {
                const std::string body = bullet.substr(
                    std::string("Friedman S（调整后）= ").size(),
                    bullet.size() - std::string("Friedman S（调整后）= ").size()
                        - std::string(
                              "。这只陈述区组设计下处理间秩差异证据，不能写成已证明相同或不同。")
                              .size());
                const auto p_mark = body.find("，P = ");
                if (p_mark != std::string::npos) {
                    const std::string s = body.substr(0, p_mark);
                    const std::string p = body.substr(
                        p_mark + std::string("，P = ").size());
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.nonparam_friedman_s", language, missing_out)
                            .text,
                        {{"%1", s}, {"%2", p}});
                }
            } else if (starts_with(bullet, "符号检验（二项精确）P = ")
                       && ends_with(
                           bullet,
                           "。未拒绝原假设不能证明中位数等于假设值。")) {
                const std::string value = bullet.substr(
                    std::string("符号检验（二项精确）P = ").size(),
                    bullet.size()
                        - std::string("符号检验（二项精确）P = ").size()
                        - std::string(
                              "。未拒绝原假设不能证明中位数等于假设值。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.nonparam_sign_p", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "Sign 中位数置信区间为 [")
                       && ends_with(
                           bullet,
                           "]，只描述位置不确定性，不能写成已证明等于假设值。")) {
                const std::string body = bullet.substr(
                    std::string("Sign 中位数置信区间为 [").size(),
                    bullet.size()
                        - std::string("Sign 中位数置信区间为 [").size()
                        - std::string(
                              "]，只描述位置不确定性，不能写成已证明等于假设值。")
                              .size());
                const auto comma = body.find(", ");
                if (comma != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.nonparam_sign_ci", language, missing_out)
                            .text,
                        {{"%1", body.substr(0, comma)},
                         {"%2", body.substr(comma + 2)}});
                }
            } else if (starts_with(bullet, "游程检验")
                       && ends_with(
                           bullet,
                           "。结果只陈述相对比较准则 K 的顺序随机性证据，不能写成已证明过程受控或失控。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string mid = bullet.substr(
                    std::string("游程检验").size(),
                    bullet.size() - std::string("游程检验").size()
                        - std::string(
                              "。结果只陈述相对比较准则 K 的顺序随机性证据，不能写成已证明过程受控或失控。")
                              .size());
                std::string mid_en = mid;
                if (en) {
                    if (starts_with(mid, " P = ")) {
                        mid_en = " P = " + mid.substr(std::string(" P = ").size());
                    } else if (mid == " 未计算出 P") {
                        mid_en = " did not compute P";
                    }
                }
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.nonparam_runs", language, missing_out)
                        .text,
                    "%1",
                    en ? mid_en : mid);
            } else if (starts_with(bullet, "比较准则 K = ")
                       && ends_with(bullet, "；等号归入 ≤K 侧。")) {
                const std::string value = bullet.substr(
                    std::string("比较准则 K = ").size(),
                    bullet.size() - std::string("比较准则 K = ").size()
                        - std::string("；等号归入 ≤K 侧。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.nonparam_runs_k", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "单样本 Wilcoxon 符号秩 P = ")
                       && ends_with(
                           bullet,
                           "。未拒绝原假设不能证明中位数等于假设值；Walsh 估计与区间只描述位置。")) {
                const std::string value = bullet.substr(
                    std::string("单样本 Wilcoxon 符号秩 P = ").size(),
                    bullet.size()
                        - std::string("单样本 Wilcoxon 符号秩 P = ").size()
                        - std::string(
                              "。未拒绝原假设不能证明中位数等于假设值；Walsh 估计与区间只描述位置。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.nonparam_wilcoxon_one_sample_p", language,
                        missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "配对 Wilcoxon 符号秩 P = ")
                       && ends_with(
                           bullet,
                           "。未拒绝原假设不能证明配对差分中位数为 0。")) {
                const std::string value = bullet.substr(
                    std::string("配对 Wilcoxon 符号秩 P = ").size(),
                    bullet.size()
                        - std::string("配对 Wilcoxon 符号秩 P = ").size()
                        - std::string(
                              "。未拒绝原假设不能证明配对差分中位数为 0。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.nonparam_wilcoxon_paired_p", language,
                        missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "Mood 中位数检验 χ² = ")
                       && bullet.find("，P = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。未拒绝原假设不能证明各组中位数相同；各组 Sign CI 只描述组内位置。")) {
                const std::string body = bullet.substr(
                    std::string("Mood 中位数检验 χ² = ").size(),
                    bullet.size()
                        - std::string("Mood 中位数检验 χ² = ").size()
                        - std::string(
                              "。未拒绝原假设不能证明各组中位数相同；各组 Sign CI 只描述组内位置。")
                              .size());
                const auto p_mark = body.find("，P = ");
                if (p_mark != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.nonparam_mood", language, missing_out)
                            .text,
                        {{"%1", body.substr(0, p_mark)},
                         {"%2",
                          body.substr(p_mark + std::string("，P = ").size())}});
                }
            } else if (starts_with(bullet, "Fisher 精确检验")
                       && ends_with(
                           bullet,
                           "。P 值只描述当前 2×2 表与独立性假设的一致程度，不能写成已证明存在或不存在关联。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string mid = bullet.substr(
                    std::string("Fisher 精确检验").size(),
                    bullet.size() - std::string("Fisher 精确检验").size()
                        - std::string(
                              "。P 值只描述当前 2×2 表与独立性假设的一致程度，不能写成已证明存在或不存在关联。")
                              .size());
                std::string p_part;
                std::string or_part;
                const auto or_mark = mid.find("，优势比 OR = ");
                if (starts_with(mid, " P = ")) {
                    const std::string p_val = or_mark == std::string::npos
                        ? mid.substr(std::string(" P = ").size())
                        : mid.substr(
                              std::string(" P = ").size(),
                              or_mark - std::string(" P = ").size());
                    p_part = en ? (" P = " + p_val) : (" P = " + p_val);
                }
                if (or_mark != std::string::npos) {
                    const std::string or_val =
                        mid.substr(or_mark + std::string("，优势比 OR = ").size());
                    or_part = en ? (", odds ratio OR = " + or_val)
                                 : ("，优势比 OR = " + or_val);
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.fisher_exact_summary", language, missing_out)
                        .text,
                    {{"%1", p_part}, {"%2", or_part}});
            } else if (starts_with(bullet, "Pearson χ²")
                       && ends_with(
                           bullet,
                           "。P 值只描述当前列联表与独立性假设的一致程度，不能证明因果关系。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string mid = bullet.substr(
                    std::string("Pearson χ²").size(),
                    bullet.size() - std::string("Pearson χ²").size()
                        - std::string(
                              "。P 值只描述当前列联表与独立性假设的一致程度，不能证明因果关系。")
                              .size());
                std::string stat_part;
                std::string p_part;
                const auto p_mark = mid.find("，P = ");
                if (starts_with(mid, " = ")) {
                    const std::string stat = p_mark == std::string::npos
                        ? mid.substr(std::string(" = ").size())
                        : mid.substr(
                              std::string(" = ").size(),
                              p_mark - std::string(" = ").size());
                    stat_part = en ? (" = " + stat) : (" = " + stat);
                }
                if (p_mark != std::string::npos) {
                    const std::string p_val =
                        mid.substr(p_mark + std::string("，P = ").size());
                    p_part = en ? (", P = " + p_val) : ("，P = " + p_val);
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.pearson_chi2_assoc", language, missing_out)
                        .text,
                    {{"%1", stat_part}, {"%2", p_part}});
            } else if (starts_with(bullet, "最大 |调整残差| = ")
                       && ends_with(
                           bullet,
                           "。这些量只帮助定位偏离，不能写成已证明关联或无关联。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string mid = bullet.substr(
                    std::string("最大 |调整残差| = ").size(),
                    bullet.size() - std::string("最大 |调整残差| = ").size()
                        - std::string(
                              "。这些量只帮助定位偏离，不能写成已证明关联或无关联。")
                              .size());
                std::string value = mid;
                std::string cell_part;
                const auto cell_mark = mid.find("，贡献最大单元格: ");
                if (cell_mark != std::string::npos) {
                    value = mid.substr(0, cell_mark);
                    const std::string cell = mid.substr(
                        cell_mark + std::string("，贡献最大单元格: ").size());
                    cell_part = en ? (", largest contribution cell: " + cell)
                                   : ("，贡献最大单元格: " + cell);
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.chi2_max_adj_residual", language, missing_out)
                        .text,
                    {{"%1", value}, {"%2", cell_part}});
            } else if (starts_with(bullet, "交叉表汇总 ")
                       && bullet.find(" × ") != std::string::npos
                       && bullet.find(" 分类，N = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。本输出只提供频数与百分比，不做独立性检验；检验请使用列联表卡方。")) {
                const std::string body = bullet.substr(
                    std::string("交叉表汇总 ").size(),
                    bullet.size() - std::string("交叉表汇总 ").size()
                        - std::string(
                              "。本输出只提供频数与百分比，不做独立性检验；检验请使用列联表卡方。")
                              .size());
                const auto x_mark = body.find(" × ");
                const auto n_mark = body.find(" 分类，N = ");
                if (x_mark != std::string::npos && n_mark != std::string::npos
                    && n_mark > x_mark) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.cross_tab_summary", language, missing_out)
                            .text,
                        {{"%1", body.substr(0, x_mark)},
                         {"%2",
                          body.substr(
                              x_mark + std::string(" × ").size(),
                              n_mark - (x_mark + std::string(" × ").size()))},
                         {"%3",
                          body.substr(n_mark + std::string(" 分类，N = ").size())}});
                }
            } else if (starts_with(bullet, "泊松拟合优度 Pearson χ²")
                       && bullet.find("，DF = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。P 值只描述观察计数与泊松模型的偏离程度，不能证明总体服从泊松分布。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("泊松拟合优度 Pearson χ²").size(),
                    bullet.size()
                        - std::string("泊松拟合优度 Pearson χ²").size()
                        - std::string(
                              "。P 值只描述观察计数与泊松模型的偏离程度，不能证明总体服从泊松分布。")
                              .size());
                const auto df_mark = body.find("，DF = ");
                std::string left = body.substr(0, df_mark);
                std::string right = body.substr(
                    df_mark + std::string("，DF = ").size());
                std::string stat_part;
                std::string p_part;
                std::string lambda_part;
                const auto p_mark = left.find("，P = ");
                if (starts_with(left, " = ")) {
                    const std::string stat = p_mark == std::string::npos
                        ? left.substr(std::string(" = ").size())
                        : left.substr(
                              std::string(" = ").size(),
                              p_mark - std::string(" = ").size());
                    stat_part = " = " + stat;
                }
                if (p_mark != std::string::npos) {
                    p_part = en
                        ? (", P = "
                           + left.substr(p_mark + std::string("，P = ").size()))
                        : ("，P = "
                           + left.substr(p_mark + std::string("，P = ").size()));
                }
                const auto lam_mark = right.find("，λ̂ = ");
                std::string df = right;
                if (lam_mark != std::string::npos) {
                    df = right.substr(0, lam_mark);
                    const std::string lam = right.substr(
                        lam_mark + std::string("，λ̂ = ").size());
                    lambda_part = en ? (", λ̂ = " + lam) : ("，λ̂ = " + lam);
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.gof_poisson", language, missing_out)
                        .text,
                    {{"%1", stat_part},
                     {"%2", p_part},
                     {"%3", df},
                     {"%4", lambda_part}});
            } else if (starts_with(bullet, "拟合优度 Pearson χ²")
                       && bullet.find("，DF = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。P 值只描述观察频数与指定比例的一致程度，不能证明总体比例等于假设。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("拟合优度 Pearson χ²").size(),
                    bullet.size() - std::string("拟合优度 Pearson χ²").size()
                        - std::string(
                              "。P 值只描述观察频数与指定比例的一致程度，不能证明总体比例等于假设。")
                              .size());
                const auto df_mark = body.find("，DF = ");
                std::string left = body.substr(0, df_mark);
                const std::string df = body.substr(
                    df_mark + std::string("，DF = ").size());
                std::string stat_part;
                std::string p_part;
                const auto p_mark = left.find("，P = ");
                if (starts_with(left, " = ")) {
                    const std::string stat = p_mark == std::string::npos
                        ? left.substr(std::string(" = ").size())
                        : left.substr(
                              std::string(" = ").size(),
                              p_mark - std::string(" = ").size());
                    stat_part = " = " + stat;
                }
                if (p_mark != std::string::npos) {
                    p_part = en
                        ? (", P = "
                           + left.substr(p_mark + std::string("，P = ").size()))
                        : ("，P = "
                           + left.substr(p_mark + std::string("，P = ").size()));
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.gof_pearson", language, missing_out)
                        .text,
                    {{"%1", stat_part}, {"%2", p_part}, {"%3", df}});
            } else if (starts_with(bullet, "GOF 有效性状态为 ")
                       && ends_with(bullet, "。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string mid = bullet.substr(
                    std::string("GOF 有效性状态为 ").size(),
                    bullet.size() - std::string("GOF 有效性状态为 ").size()
                        - std::string("。").size());
                std::string status = mid;
                std::string min_part;
                const std::string open = "（最小期望频数 = ";
                const std::string close = "）";
                if (mid.size() >= open.size() + close.size()
                    && mid.compare(
                           mid.size() - close.size(), close.size(), close)
                        == 0) {
                    const auto open_pos = mid.rfind(open);
                    if (open_pos != std::string::npos) {
                        status = mid.substr(0, open_pos);
                        const std::string min_val = mid.substr(
                            open_pos + open.size(),
                            mid.size() - open_pos - open.size() - close.size());
                        min_part = en
                            ? (" (minimum expected count = " + min_val + ")")
                            : ("（最小期望频数 = " + min_val + "）");
                    }
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.gof_validity_status", language, missing_out)
                        .text,
                    {{"%1", status}, {"%2", min_part}});
            } else if (starts_with(bullet, "缺失 N* = ")
                       && ends_with(bullet, "，未进入类别计数。")) {
                const std::string value = bullet.substr(
                    std::string("缺失 N* = ").size(),
                    bullet.size() - std::string("缺失 N* = ").size()
                        - std::string("，未进入类别计数。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.gof_missing_n", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(
                           bullet, "McNemar（Edwards 连续性校正）χ² = ")
                       && bullet.find("，配对有效 N = ") != std::string::npos
                       && bullet.find("，不一致对数 b+c = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。结果只陈述边际比例差异证据，不能写成已证明相同或不同。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("McNemar（Edwards 连续性校正）χ² = ").size(),
                    bullet.size()
                        - std::string("McNemar（Edwards 连续性校正）χ² = ").size()
                        - std::string(
                              "。结果只陈述边际比例差异证据，不能写成已证明相同或不同。")
                              .size());
                const auto n_mark = body.find("，配对有效 N = ");
                const auto d_mark = body.find("，不一致对数 b+c = ");
                if (n_mark != std::string::npos && d_mark != std::string::npos
                    && d_mark > n_mark) {
                    const std::string chi = body.substr(0, n_mark);
                    const std::string n = body.substr(
                        n_mark + std::string("，配对有效 N = ").size(),
                        d_mark - (n_mark + std::string("，配对有效 N = ").size()));
                    std::string disc = body.substr(
                        d_mark + std::string("，不一致对数 b+c = ").size());
                    std::string p_part;
                    const auto p_mark = disc.find("，P = ");
                    if (p_mark != std::string::npos) {
                        const std::string p_val = disc.substr(
                            p_mark + std::string("，P = ").size());
                        disc = disc.substr(0, p_mark);
                        p_part = en ? (", P = " + p_val) : ("，P = " + p_val);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.mcnemar_summary", language, missing_out)
                            .text,
                        {{"%1", chi},
                         {"%2", n},
                         {"%3", disc},
                         {"%4", p_part}});
                }
            } else if (starts_with(bullet, "Cochran Q = ")
                       && bullet.find("，DF = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。结果只陈述配对二元处理间差异证据，不能写成已证明阳性率相同或不同。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("Cochran Q = ").size(),
                    bullet.size() - std::string("Cochran Q = ").size()
                        - std::string(
                              "。结果只陈述配对二元处理间差异证据，不能写成已证明阳性率相同或不同。")
                              .size());
                const auto df_mark = body.find("，DF = ");
                const std::string q = body.substr(0, df_mark);
                std::string df = body.substr(
                    df_mark + std::string("，DF = ").size());
                std::string p_part;
                const auto p_mark = df.find("，P = ");
                if (p_mark != std::string::npos) {
                    const std::string p_val =
                        df.substr(p_mark + std::string("，P = ").size());
                    df = df.substr(0, p_mark);
                    p_part = en ? (", P = " + p_val) : ("，P = " + p_val);
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.cochran_q_summary", language, missing_out)
                        .text,
                    {{"%1", q}, {"%2", df}, {"%3", p_part}});
            } else if (starts_with(bullet, "二元 Logistic 已收敛")
                       && bullet.find("。未拒绝拟合不足不能说明模型已充分")
                           != std::string::npos
                       && ends_with(
                           bullet, "。系数解释依赖事件编码和 complete-case 样本。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const auto mid_mark =
                    bullet.find("。未拒绝拟合不足不能说明模型已充分");
                const std::string hl = bullet.substr(
                    std::string("二元 Logistic 已收敛").size(),
                    mid_mark - std::string("二元 Logistic 已收敛").size());
                std::string rest = bullet.substr(
                    mid_mark
                        + std::string("。未拒绝拟合不足不能说明模型已充分").size(),
                    bullet.size()
                        - (mid_mark
                           + std::string("。未拒绝拟合不足不能说明模型已充分").size())
                        - std::string(
                              "。系数解释依赖事件编码和 complete-case 样本。")
                              .size());
                std::string hl_en = hl;
                if (en) {
                    if (hl == "，Hosmer–Lemeshow 未计算") {
                        hl_en = ", Hosmer–Lemeshow not computed";
                    } else if (starts_with(
                                   hl,
                                   "，在 α = 0.05 下拒绝拟合不足（Hosmer–Lemeshow P = ")
                               && ends_with(hl, "）")) {
                        const std::string p = hl.substr(
                            std::string(
                                "，在 α = 0.05 下拒绝拟合不足（Hosmer–Lemeshow P = ")
                                .size(),
                            hl.size()
                                - std::string(
                                      "，在 α = 0.05 下拒绝拟合不足（Hosmer–Lemeshow P = ")
                                      .size()
                                - std::string("）").size());
                        hl_en =
                            ", rejects lack of fit at α = 0.05 (Hosmer–Lemeshow P = "
                            + p + ")";
                    } else if (starts_with(
                                   hl,
                                   "，在 α = 0.05 下未拒绝拟合不足（Hosmer–Lemeshow P = ")
                               && ends_with(hl, "）")) {
                        const std::string p = hl.substr(
                            std::string(
                                "，在 α = 0.05 下未拒绝拟合不足（Hosmer–Lemeshow P = ")
                                .size(),
                            hl.size()
                                - std::string(
                                      "，在 α = 0.05 下未拒绝拟合不足（Hosmer–Lemeshow P = ")
                                      .size()
                                - std::string("）").size());
                        hl_en =
                            ", does not reject lack of fit at α = 0.05 (Hosmer–Lemeshow P = "
                            + p + ")";
                    }
                }
                std::string lev_part;
                std::string vif_part;
                const auto lev_mark = rest.find("；检测到 ");
                const auto vif_mark = rest.find("；最大 VIF = ");
                if (lev_mark != std::string::npos) {
                    std::string lev_body = rest.substr(
                        lev_mark + std::string("；检测到 ").size());
                    if (vif_mark != std::string::npos && vif_mark > lev_mark) {
                        lev_body = rest.substr(
                            lev_mark + std::string("；检测到 ").size(),
                            vif_mark
                                - (lev_mark + std::string("；检测到 ").size()));
                    }
                    if (ends_with(lev_body, " 个高杠杆观测")) {
                        const std::string n = lev_body.substr(
                            0,
                            lev_body.size()
                                - std::string(" 个高杠杆观测").size());
                        lev_part = en
                            ? ("; detected " + n + " high-leverage observation(s)")
                            : ("；检测到 " + n + " 个高杠杆观测");
                    }
                }
                if (vif_mark != std::string::npos) {
                    const std::string vif =
                        rest.substr(vif_mark + std::string("；最大 VIF = ").size());
                    vif_part = en ? ("; maximum VIF = " + vif)
                                  : ("；最大 VIF = " + vif);
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.logistic_converged", language, missing_out)
                        .text,
                    {{"%1", en ? hl_en : hl},
                     {"%2", lev_part},
                     {"%3", vif_part}});
            } else if (starts_with(bullet, "PCA 模式为 ")
                       && bullet.find("，保留 ") != std::string::npos
                       && bullet.find(" 个主成分") != std::string::npos
                       && ends_with(
                           bullet,
                           "。异常阈值只作诊断，T²/Q 超限不是过程合格或失控判定。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("PCA 模式为 ").size(),
                    bullet.size() - std::string("PCA 模式为 ").size()
                        - std::string(
                              "。异常阈值只作诊断，T²/Q 超限不是过程合格或失控判定。")
                              .size());
                const auto ret_mark = body.find("，保留 ");
                const auto pc_mark = body.find(" 个主成分", ret_mark);
                const auto anom_mark = body.find("，检测到 ");
                if (ret_mark != std::string::npos && pc_mark != std::string::npos
                    && anom_mark != std::string::npos) {
                    const std::string mode = body.substr(0, ret_mark);
                    const std::string retained = body.substr(
                        ret_mark + std::string("，保留 ").size(),
                        pc_mark - (ret_mark + std::string("，保留 ").size()));
                    std::string between = body.substr(
                        pc_mark + std::string(" 个主成分").size(),
                        anom_mark - (pc_mark + std::string(" 个主成分").size()));
                    std::string n_part;
                    if (starts_with(between, "，有效观测 ")) {
                        n_part = en
                            ? (", valid observations "
                               + between.substr(std::string("，有效观测 ").size()))
                            : between;
                    }
                    std::string anom = body.substr(
                        anom_mark + std::string("，检测到 ").size());
                    if (ends_with(anom, " 个 T²/Q 异常观测")) {
                        anom = anom.substr(
                            0, anom.size() - std::string(" 个 T²/Q 异常观测").size());
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.pca_summary", language, missing_out)
                            .text,
                        {{"%1", mode},
                         {"%2", retained},
                         {"%3", n_part},
                         {"%4", anom}});
                }
            } else if (starts_with(bullet, "K-Means：k = ")
                       && ends_with(
                           bullet,
                           "。簇标签只描述相对邻近结构，不能写成过程或批次判定。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("K-Means：k = ").size(),
                    bullet.size() - std::string("K-Means：k = ").size()
                        - std::string(
                              "。簇标签只描述相对邻近结构，不能写成过程或批次判定。")
                              .size());
                const auto n_mark = body.find("，N = ");
                const auto v_mark = body.find("，变量数 = ");
                const auto it_mark = body.find("，迭代 = ");
                if (n_mark != std::string::npos && v_mark != std::string::npos
                    && it_mark != std::string::npos) {
                    const std::string k = body.substr(0, n_mark);
                    const std::string n = body.substr(
                        n_mark + std::string("，N = ").size(),
                        v_mark - (n_mark + std::string("，N = ").size()));
                    const std::string vars = body.substr(
                        v_mark + std::string("，变量数 = ").size(),
                        it_mark - (v_mark + std::string("，变量数 = ").size()));
                    std::string it_rest =
                        body.substr(it_mark + std::string("，迭代 = ").size());
                    std::string iterations;
                    std::string conv_part;
                    std::string ss_part;
                    std::size_t cut = it_rest.size();
                    const auto p1 = it_rest.find("（");
                    const auto p2 = it_rest.find("；");
                    if (p1 != std::string::npos) {
                        cut = std::min(cut, p1);
                    }
                    if (p2 != std::string::npos) {
                        cut = std::min(cut, p2);
                    }
                    iterations = it_rest.substr(0, cut);
                    std::string rest = it_rest.substr(cut);
                    if (starts_with(rest, "（已收敛）")
                        || starts_with(rest, "（未完全收敛）")) {
                        const std::string close_paren = "）";
                        const auto close = rest.find(close_paren);
                        if (close != std::string::npos) {
                            const std::string tag =
                                rest.substr(0, close + close_paren.size());
                            conv_part = en
                                ? (tag == "（已收敛）" ? " (converged)"
                                                       : " (not fully converged)")
                                : tag;
                            rest = rest.substr(tag.size());
                        }
                    }
                    if (starts_with(rest, "；总簇内平方和 ≈ ")) {
                        ss_part = en
                            ? ("; total within-cluster SS ≈ "
                               + rest.substr(
                                     std::string("；总簇内平方和 ≈ ").size()))
                            : rest;
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.kmeans_summary", language, missing_out)
                            .text,
                        {{"%1", k},
                         {"%2", n},
                         {"%3", vars},
                         {"%4", iterations},
                         {"%5", conv_part},
                         {"%6", ss_part}});
                }
            } else if (starts_with(bullet, "CART 单树任务 = ")
                       && ends_with(
                           bullet, "。训练集指标不能外推为过程合格结论。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("CART 单树任务 = ").size(),
                    bullet.size() - std::string("CART 单树任务 = ").size()
                        - std::string("。训练集指标不能外推为过程合格结论。")
                              .size());
                const auto n_mark = body.find("，N = ");
                const auto p_mark = body.find("，预测变量 = ");
                const auto node_mark = body.find("，结点 = ");
                const auto leaf_mark = body.find("，叶 = ");
                if (n_mark != std::string::npos && p_mark != std::string::npos
                    && node_mark != std::string::npos
                    && leaf_mark != std::string::npos) {
                    const std::string task = body.substr(0, n_mark);
                    const std::string n = body.substr(
                        n_mark + std::string("，N = ").size(),
                        p_mark - (n_mark + std::string("，N = ").size()));
                    const std::string preds = body.substr(
                        p_mark + std::string("，预测变量 = ").size(),
                        node_mark - (p_mark + std::string("，预测变量 = ").size()));
                    const std::string nodes = body.substr(
                        node_mark + std::string("，结点 = ").size(),
                        leaf_mark - (node_mark + std::string("，结点 = ").size()));
                    std::string rest =
                        body.substr(leaf_mark + std::string("，叶 = ").size());
                    std::string leaves = rest;
                    std::string top_part;
                    std::string metric_part;
                    const auto top_mark = rest.find("；主导分裂变量 = ");
                    const auto rmse_mark = rest.find("；训练集 RMSE ≈ ");
                    const auto acc_mark = rest.find("；训练集准确率 ≈ ");
                    std::size_t first_opt = rest.size();
                    if (top_mark != std::string::npos) {
                        first_opt = std::min(first_opt, top_mark);
                    }
                    if (rmse_mark != std::string::npos) {
                        first_opt = std::min(first_opt, rmse_mark);
                    }
                    if (acc_mark != std::string::npos) {
                        first_opt = std::min(first_opt, acc_mark);
                    }
                    leaves = rest.substr(0, first_opt);
                    rest = rest.substr(first_opt);
                    if (starts_with(rest, "；主导分裂变量 = ")) {
                        std::size_t end = rest.size();
                        const auto r2 = rest.find("；训练集 RMSE ≈ ");
                        const auto a2 = rest.find("；训练集准确率 ≈ ");
                        if (r2 != std::string::npos) {
                            end = std::min(end, r2);
                        }
                        if (a2 != std::string::npos) {
                            end = std::min(end, a2);
                        }
                        const std::string top = rest.substr(
                            std::string("；主导分裂变量 = ").size(),
                            end - std::string("；主导分裂变量 = ").size());
                        top_part = en ? ("; top split variable = " + top)
                                      : ("；主导分裂变量 = " + top);
                        rest = rest.substr(end);
                    }
                    if (starts_with(rest, "；训练集 RMSE ≈ ")) {
                        metric_part = en
                            ? ("; training RMSE ≈ "
                               + rest.substr(std::string("；训练集 RMSE ≈ ").size()))
                            : rest;
                    } else if (starts_with(rest, "；训练集准确率 ≈ ")) {
                        metric_part = en
                            ? ("; training accuracy ≈ "
                               + rest.substr(
                                     std::string("；训练集准确率 ≈ ").size()))
                            : rest;
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.cart_summary", language, missing_out)
                            .text,
                        {{"%1", task},
                         {"%2", n},
                         {"%3", preds},
                         {"%4", nodes},
                         {"%5", leaves},
                         {"%6", top_part},
                         {"%7", metric_part}});
                }
            } else if (starts_with(bullet, "ADF（")
                       && bullet.find("）：N = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。单位根结论只描述差分平稳性证据，不能外推为工艺判定。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("ADF（").size(),
                    bullet.size() - std::string("ADF（").size()
                        - std::string(
                              "。单位根结论只描述差分平稳性证据，不能外推为工艺判定。")
                              .size());
                const auto reg_end = body.find("）：N = ");
                const auto lag_mark = body.find("，滞后 = ");
                const auto used_mark = body.find("，有效回归行 = ");
                if (reg_end != std::string::npos && lag_mark != std::string::npos
                    && used_mark != std::string::npos) {
                    const std::string reg = body.substr(0, reg_end);
                    const std::string n = body.substr(
                        reg_end + std::string("）：N = ").size(),
                        lag_mark - (reg_end + std::string("）：N = ").size()));
                    const std::string lags = body.substr(
                        lag_mark + std::string("，滞后 = ").size(),
                        used_mark - (lag_mark + std::string("，滞后 = ").size()));
                    std::string rest = body.substr(
                        used_mark + std::string("，有效回归行 = ").size());
                    std::string used = rest;
                    std::string tau_part;
                    std::string crit_part;
                    std::string reject_part;
                    const auto tau_mark = rest.find("；τ ≈ ");
                    const auto crit_mark = rest.find("；5% 临界值 ≈ ");
                    const auto rej_mark = rest.find("；相对 5% 临界值");
                    std::size_t first = rest.size();
                    if (tau_mark != std::string::npos) {
                        first = std::min(first, tau_mark);
                    }
                    if (crit_mark != std::string::npos) {
                        first = std::min(first, crit_mark);
                    }
                    if (rej_mark != std::string::npos) {
                        first = std::min(first, rej_mark);
                    }
                    used = rest.substr(0, first);
                    rest = rest.substr(first);
                    if (starts_with(rest, "；τ ≈ ")) {
                        std::size_t end = rest.size();
                        const auto c2 = rest.find("；5% 临界值 ≈ ");
                        const auto r2 = rest.find("；相对 5% 临界值");
                        if (c2 != std::string::npos) {
                            end = std::min(end, c2);
                        }
                        if (r2 != std::string::npos) {
                            end = std::min(end, r2);
                        }
                        tau_part = en
                            ? ("; τ ≈ "
                               + rest.substr(
                                     std::string("；τ ≈ ").size(),
                                     end - std::string("；τ ≈ ").size()))
                            : rest.substr(0, end);
                        rest = rest.substr(end);
                    }
                    if (starts_with(rest, "；5% 临界值 ≈ ")) {
                        std::size_t end = rest.size();
                        const auto r2 = rest.find("；相对 5% 临界值");
                        if (r2 != std::string::npos) {
                            end = r2;
                        }
                        crit_part = en
                            ? ("; 5% critical ≈ "
                               + rest.substr(
                                     std::string("；5% 临界值 ≈ ").size(),
                                     end - std::string("；5% 临界值 ≈ ").size()))
                            : rest.substr(0, end);
                        rest = rest.substr(end);
                    }
                    if (rest == "；相对 5% 临界值有拒绝单位根的证据") {
                        reject_part = en
                            ? "; evidence to reject unit root vs 5% critical value"
                            : rest;
                    } else if (rest == "；相对 5% 临界值未拒绝单位根") {
                        reject_part = en
                            ? "; unit root not rejected vs 5% critical value"
                            : rest;
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.adf_summary", language, missing_out)
                            .text,
                        {{"%1", reg},
                         {"%2", n},
                         {"%3", lags},
                         {"%4", used},
                         {"%5", tau_part},
                         {"%6", crit_part},
                         {"%7", reject_part}});
                }
            } else if (starts_with(bullet, "Poisson 回归（log 链）：N = ")
                       && ends_with(
                           bullet,
                           "。系数显著性只描述计数均值与预测变量的关联证据。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("Poisson 回归（log 链）：N = ").size(),
                    bullet.size()
                        - std::string("Poisson 回归（log 链）：N = ").size()
                        - std::string(
                              "。系数显著性只描述计数均值与预测变量的关联证据。")
                              .size());
                const auto p_mark = body.find("，预测变量 = ");
                const auto it_mark = body.find("，迭代 = ");
                if (p_mark != std::string::npos && it_mark != std::string::npos) {
                    const std::string n = body.substr(0, p_mark);
                    const std::string preds = body.substr(
                        p_mark + std::string("，预测变量 = ").size(),
                        it_mark - (p_mark + std::string("，预测变量 = ").size()));
                    std::string rest =
                        body.substr(it_mark + std::string("，迭代 = ").size());
                    std::string iterations = rest;
                    std::string conv_part;
                    std::string dev_part;
                    std::string aic_part;
                    std::size_t cut = rest.size();
                    const auto c1 = rest.find("（");
                    const auto d1 = rest.find("；偏差 ≈ ");
                    const auto a1 = rest.find("；AIC ≈ ");
                    if (c1 != std::string::npos) {
                        cut = std::min(cut, c1);
                    }
                    if (d1 != std::string::npos) {
                        cut = std::min(cut, d1);
                    }
                    if (a1 != std::string::npos) {
                        cut = std::min(cut, a1);
                    }
                    iterations = rest.substr(0, cut);
                    rest = rest.substr(cut);
                    if (starts_with(rest, "（已收敛）")
                        || starts_with(rest, "（未收敛）")) {
                        const std::string close_paren = "）";
                        const auto close = rest.find(close_paren);
                        if (close != std::string::npos) {
                            const std::string tag =
                                rest.substr(0, close + close_paren.size());
                            conv_part = en
                                ? (tag == "（已收敛）" ? " (converged)"
                                                       : " (not converged)")
                                : tag;
                            rest = rest.substr(tag.size());
                        }
                    }
                    if (starts_with(rest, "；偏差 ≈ ")) {
                        std::size_t end = rest.size();
                        const auto a2 = rest.find("；AIC ≈ ");
                        if (a2 != std::string::npos) {
                            end = a2;
                        }
                        dev_part = en
                            ? ("; deviance ≈ "
                               + rest.substr(
                                     std::string("；偏差 ≈ ").size(),
                                     end - std::string("；偏差 ≈ ").size()))
                            : rest.substr(0, end);
                        rest = rest.substr(end);
                    }
                    if (starts_with(rest, "；AIC ≈ ")) {
                        aic_part = en
                            ? ("; AIC ≈ "
                               + rest.substr(std::string("；AIC ≈ ").size()))
                            : rest;
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.poisson_reg_summary", language, missing_out)
                            .text,
                        {{"%1", n},
                         {"%2", preds},
                         {"%3", iterations},
                         {"%4", conv_part},
                         {"%5", dev_part},
                         {"%6", aic_part}});
                }
            } else if (starts_with(bullet, "Isolation Forest：N = ")
                       && ends_with(
                           bullet,
                           "。分数高只提示相对孤立，与单变量 outlier_test 分流，不能写成工艺判定。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("Isolation Forest：N = ").size(),
                    bullet.size()
                        - std::string("Isolation Forest：N = ").size()
                        - std::string(
                              "。分数高只提示相对孤立，与单变量 outlier_test 分流，不能写成工艺判定。")
                              .size());
                const auto v_mark = body.find("，变量 = ");
                const auto t_mark = body.find("，树数 = ");
                const auto a_mark = body.find("，标记异常 = ");
                if (v_mark != std::string::npos && t_mark != std::string::npos
                    && a_mark != std::string::npos) {
                    const std::string n = body.substr(0, v_mark);
                    const std::string vars = body.substr(
                        v_mark + std::string("，变量 = ").size(),
                        t_mark - (v_mark + std::string("，变量 = ").size()));
                    const std::string trees = body.substr(
                        t_mark + std::string("，树数 = ").size(),
                        a_mark - (t_mark + std::string("，树数 = ").size()));
                    std::string rest =
                        body.substr(a_mark + std::string("，标记异常 = ").size());
                    std::string anom = rest;
                    std::string thr_part;
                    const auto thr_mark = rest.find("；分数阈值 ≈ ");
                    if (thr_mark != std::string::npos) {
                        anom = rest.substr(0, thr_mark);
                        thr_part = en
                            ? ("; score threshold ≈ "
                               + rest.substr(
                                     thr_mark + std::string("；分数阈值 ≈ ").size()))
                            : rest.substr(thr_mark);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.isolation_forest_summary", language,
                            missing_out)
                            .text,
                        {{"%1", n},
                         {"%2", vars},
                         {"%3", trees},
                         {"%4", anom},
                         {"%5", thr_part}});
                }
            } else if (starts_with(bullet, "Bootstrap 均值（")
                       && ends_with(bullet, "。区间只描述重抽样不确定性。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("Bootstrap 均值（").size(),
                    bullet.size() - std::string("Bootstrap 均值（").size()
                        - std::string("。区间只描述重抽样不确定性。").size());
                const auto method_end = body.find("）：N = ");
                const auto b_mark = body.find("，B = ");
                if (method_end != std::string::npos
                    && b_mark != std::string::npos) {
                    const std::string method = body.substr(0, method_end);
                    const std::string n = body.substr(
                        method_end + std::string("）：N = ").size(),
                        b_mark
                            - (method_end + std::string("）：N = ").size()));
                    std::string rest =
                        body.substr(b_mark + std::string("，B = ").size());
                    std::string b = rest;
                    std::string mean_part;
                    std::string ci_part;
                    const auto mean_mark = rest.find("；均值 ≈ ");
                    const auto ci_mark = rest.find("；CI ≈ [");
                    std::size_t first = rest.size();
                    if (mean_mark != std::string::npos) {
                        first = std::min(first, mean_mark);
                    }
                    if (ci_mark != std::string::npos) {
                        first = std::min(first, ci_mark);
                    }
                    b = rest.substr(0, first);
                    rest = rest.substr(first);
                    if (starts_with(rest, "；均值 ≈ ")) {
                        std::size_t end = rest.size();
                        const auto c2 = rest.find("；CI ≈ [");
                        if (c2 != std::string::npos) {
                            end = c2;
                        }
                        mean_part = en
                            ? ("; mean ≈ "
                               + rest.substr(
                                     std::string("；均值 ≈ ").size(),
                                     end - std::string("；均值 ≈ ").size()))
                            : rest.substr(0, end);
                        rest = rest.substr(end);
                    }
                    if (starts_with(rest, "；CI ≈ [") && ends_with(rest, "]")) {
                        const std::string inside = rest.substr(
                            std::string("；CI ≈ [").size(),
                            rest.size() - std::string("；CI ≈ [").size()
                                - std::string("]").size());
                        ci_part = en ? ("; CI ≈ [" + inside + "]")
                                     : ("；CI ≈ [" + inside + "]");
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.bootstrap_mean_summary", language,
                            missing_out)
                            .text,
                        {{"%1", method},
                         {"%2", n},
                         {"%3", b},
                         {"%4", mean_part},
                         {"%5", ci_part}});
                }
            } else if (starts_with(bullet, "层次聚类（")
                       && ends_with(
                           bullet, "。簇标签只描述相对邻近结构。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("层次聚类（").size(),
                    bullet.size() - std::string("层次聚类（").size()
                        - std::string("。簇标签只描述相对邻近结构。").size());
                const auto link_end = body.find("）：N = ");
                const auto k_mark = body.find("，k = ");
                const auto m_mark = body.find("，合并步 = ");
                if (link_end != std::string::npos && k_mark != std::string::npos
                    && m_mark != std::string::npos) {
                    const std::string link = body.substr(0, link_end);
                    const std::string n = body.substr(
                        link_end + std::string("）：N = ").size(),
                        k_mark - (link_end + std::string("）：N = ").size()));
                    const std::string k = body.substr(
                        k_mark + std::string("，k = ").size(),
                        m_mark - (k_mark + std::string("，k = ").size()));
                    std::string rest =
                        body.substr(m_mark + std::string("，合并步 = ").size());
                    std::string merges = rest;
                    std::string scale_part;
                    if (ends_with(rest, "；已标准化")
                        || ends_with(rest, "；未标准化")) {
                        const auto semi = rest.rfind("；");
                        merges = rest.substr(0, semi);
                        const std::string tag = rest.substr(semi);
                        scale_part = en
                            ? (tag == "；已标准化" ? "; standardized"
                                                   : "; not standardized")
                            : tag;
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.hier_cluster_summary", language, missing_out)
                            .text,
                        {{"%1", link},
                         {"%2", n},
                         {"%3", k},
                         {"%4", merges},
                         {"%5", scale_part}});
                }
            } else if (starts_with(
                           bullet, "有序 Logistic（比例优势 logit）：N = ")
                       && ends_with(
                           bullet,
                           "。系数描述有序累积对数优势的关联证据。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("有序 Logistic（比例优势 logit）：N = ").size(),
                    bullet.size()
                        - std::string("有序 Logistic（比例优势 logit）：N = ")
                              .size()
                        - std::string(
                              "。系数描述有序累积对数优势的关联证据。")
                              .size());
                const auto cat_mark = body.find("，水平 = ");
                const auto pred_mark = body.find("，预测变量 = ");
                if (cat_mark != std::string::npos
                    && pred_mark != std::string::npos) {
                    const std::string n = body.substr(0, cat_mark);
                    const std::string cats = body.substr(
                        cat_mark + std::string("，水平 = ").size(),
                        pred_mark - (cat_mark + std::string("，水平 = ").size()));
                    std::string rest = body.substr(
                        pred_mark + std::string("，预测变量 = ").size());
                    std::string preds = rest;
                    std::string conv_part;
                    std::string aic_part;
                    std::size_t cut = rest.size();
                    const auto c1 = rest.find("；已收敛");
                    const auto c2 = rest.find("；未收敛");
                    const auto a1 = rest.find("；AIC ≈ ");
                    if (c1 != std::string::npos) {
                        cut = std::min(cut, c1);
                    }
                    if (c2 != std::string::npos) {
                        cut = std::min(cut, c2);
                    }
                    if (a1 != std::string::npos) {
                        cut = std::min(cut, a1);
                    }
                    preds = rest.substr(0, cut);
                    rest = rest.substr(cut);
                    if (starts_with(rest, "；已收敛")
                        || starts_with(rest, "；未收敛")) {
                        std::size_t end = rest.size();
                        const auto a2 = rest.find("；AIC ≈ ");
                        if (a2 != std::string::npos) {
                            end = a2;
                        }
                        const std::string tag = rest.substr(0, end);
                        conv_part = en
                            ? (tag == "；已收敛" ? "; converged" : "; not converged")
                            : tag;
                        rest = rest.substr(end);
                    }
                    if (starts_with(rest, "；AIC ≈ ")) {
                        aic_part = en
                            ? ("; AIC ≈ "
                               + rest.substr(std::string("；AIC ≈ ").size()))
                            : rest;
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.ordinal_logistic_summary", language,
                            missing_out)
                            .text,
                        {{"%1", n},
                         {"%2", cats},
                         {"%3", preds},
                         {"%4", conv_part},
                         {"%5", aic_part}});
                }
            } else if (starts_with(bullet, "线性判别：N = ")
                       && ends_with(
                           bullet, "。训练集准确率不能外推为工艺判定。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("线性判别：N = ").size(),
                    bullet.size() - std::string("线性判别：N = ").size()
                        - std::string("。训练集准确率不能外推为工艺判定。")
                              .size());
                const auto c_mark = body.find("，类数 = ");
                const auto p_mark = body.find("，预测变量 = ");
                if (c_mark != std::string::npos && p_mark != std::string::npos) {
                    const std::string n = body.substr(0, c_mark);
                    const std::string classes = body.substr(
                        c_mark + std::string("，类数 = ").size(),
                        p_mark - (c_mark + std::string("，类数 = ").size()));
                    std::string rest =
                        body.substr(p_mark + std::string("，预测变量 = ").size());
                    std::string preds = rest;
                    std::string acc_part;
                    const auto acc_mark = rest.find("；训练准确率 ≈ ");
                    if (acc_mark != std::string::npos) {
                        preds = rest.substr(0, acc_mark);
                        acc_part = en
                            ? ("; training accuracy ≈ "
                               + rest.substr(
                                     acc_mark
                                     + std::string("；训练准确率 ≈ ").size()))
                            : rest.substr(acc_mark);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.lda_summary", language, missing_out)
                            .text,
                        {{"%1", n},
                         {"%2", classes},
                         {"%3", preds},
                         {"%4", acc_part}});
                }
            } else if (starts_with(bullet, "有效观测 N = ")
                       && bullet.find("，缺失 N* = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。描述统计不检验分布假设，也不能写成过程合格。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("有效观测 N = ").size(),
                    bullet.size() - std::string("有效观测 N = ").size()
                        - std::string(
                              "。描述统计不检验分布假设，也不能写成过程合格。")
                              .size());
                const auto miss_mark = body.find("，缺失 N* = ");
                if (miss_mark != std::string::npos) {
                    const std::string n = body.substr(0, miss_mark);
                    std::string rest = body.substr(
                        miss_mark + std::string("，缺失 N* = ").size());
                    std::string missing = rest;
                    std::string mean_part;
                    const auto mean_mark = rest.find("，均值为 ");
                    if (mean_mark != std::string::npos) {
                        missing = rest.substr(0, mean_mark);
                        mean_part = en
                            ? (", mean = "
                               + rest.substr(
                                     mean_mark + std::string("，均值为 ").size()))
                            : rest.substr(mean_mark);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.descriptive_n_summary", language, missing_out)
                            .text,
                        {{"%1", n}, {"%2", missing}, {"%3", mean_part}});
                }
            } else if (starts_with(bullet, "初始质心取前 k 个观测；结果依赖尺度")
                       && ends_with(bullet, "；非 Minitab golden。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string mid = bullet.substr(
                    std::string("初始质心取前 k 个观测；结果依赖尺度").size(),
                    bullet.size()
                        - std::string("初始质心取前 k 个观测；结果依赖尺度").size()
                        - std::string("；非 Minitab golden。").size());
                std::string scale_part = mid;
                if (en) {
                    if (mid == "（已标准化）") {
                        scale_part = " (standardized)";
                    } else if (mid == "（未标准化）") {
                        scale_part = " (not standardized)";
                    }
                }
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.kmeans_init_honesty", language, missing_out)
                        .text,
                    "%1",
                    scale_part);
            } else if (starts_with(bullet, "密度图：N = ")
                       && ends_with(
                           bullet, "。曲线是平滑估计，不能据此写成正态性结论。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string mid = bullet.substr(
                    std::string("密度图：N = ").size(),
                    bullet.size() - std::string("密度图：N = ").size()
                        - std::string(
                              "。曲线是平滑估计，不能据此写成正态性结论。")
                              .size());
                std::string n = mid;
                std::string h_part;
                const auto h_mark = mid.find("；Silverman 带宽 h = ");
                if (h_mark != std::string::npos) {
                    n = mid.substr(0, h_mark);
                    const std::string h = mid.substr(
                        h_mark + std::string("；Silverman 带宽 h = ").size());
                    h_part = en ? ("; Silverman bandwidth h = " + h)
                                : ("；Silverman 带宽 h = " + h);
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.eda_density", language, missing_out)
                        .text,
                    {{"%1", n}, {"%2", h_part}});
            } else if (starts_with(bullet, "二维分箱：N = ")
                       && bullet.find("；格 = ") != std::string::npos
                       && ends_with(
                           bullet, "（矩形格，产品名 Hexbin）。")) {
                const std::string body = bullet.substr(
                    std::string("二维分箱：N = ").size(),
                    bullet.size() - std::string("二维分箱：N = ").size()
                        - std::string("（矩形格，产品名 Hexbin）。").size());
                const auto g_mark = body.find("；格 = ");
                const auto x_mark = body.find("×", g_mark);
                if (g_mark != std::string::npos && x_mark != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.eda_hexbin", language, missing_out)
                            .text,
                        {{"%1", body.substr(0, g_mark)},
                         {"%2",
                          body.substr(
                              g_mark + std::string("；格 = ").size(),
                              x_mark - (g_mark + std::string("；格 = ").size()))},
                         {"%3", body.substr(x_mark + std::string("×").size())}});
                }
            } else if (starts_with(bullet, "小提琴图：组数 = ")
                       && ends_with(
                           bullet, "；形状来自分组 KDE，箱线为五数摘要。")) {
                const std::string n = bullet.substr(
                    std::string("小提琴图：组数 = ").size(),
                    bullet.size() - std::string("小提琴图：组数 = ").size()
                        - std::string("；形状来自分组 KDE，箱线为五数摘要。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.eda_violin", language, missing_out)
                        .text,
                    "%1",
                    n);
            } else if (starts_with(bullet, "通用条形图：类别数 = ")
                       && ends_with(
                           bullet,
                           "；未按计数排序，无累积百分比（与柏拉图分流）。")) {
                const std::string n = bullet.substr(
                    std::string("通用条形图：类别数 = ").size(),
                    bullet.size() - std::string("通用条形图：类别数 = ").size()
                        - std::string(
                              "；未按计数排序，无累积百分比（与柏拉图分流）。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.eda_bar", language, missing_out)
                        .text,
                    "%1",
                    n);
            } else if (starts_with(bullet, "NIST EDA 四图：N = ")
                       && ends_with(
                           bullet,
                           "。四图用于检查位置/散布/随机性/分布形态假设，不能写成受控结论或正态已成立。")) {
                const std::string body = bullet.substr(
                    std::string("NIST EDA 四图：N = ").size(),
                    bullet.size() - std::string("NIST EDA 四图：N = ").size()
                        - std::string(
                              "。四图用于检查位置/散布/随机性/分布形态假设，不能写成受控结论或正态已成立。")
                              .size());
                const auto semi = body.find("；同页包含");
                const std::string n =
                    semi == std::string::npos ? body : body.substr(0, semi);
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.eda_4plot", language, missing_out)
                        .text,
                    "%1",
                    n);
            } else if (starts_with(bullet, "受控分面：面板 = ")
                       && bullet.find("/") != std::string::npos
                       && bullet.find("（max=") != std::string::npos
                       && ends_with(
                           bullet,
                           "。分面是多面板编排，不是自由拼版；by/分组仍是图内着色。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("受控分面：面板 = ").size(),
                    bullet.size() - std::string("受控分面：面板 = ").size()
                        - std::string(
                              "。分面是多面板编排，不是自由拼版；by/分组仍是图内着色。")
                              .size());
                const auto slash = body.find("/");
                const auto max_open = body.find("（max=");
                const auto max_close = body.find("）", max_open);
                if (slash != std::string::npos && max_open != std::string::npos
                    && max_close != std::string::npos && max_open > slash) {
                    const std::string panels = body.substr(0, slash);
                    const std::string levels = body.substr(
                        slash + 1, max_open - (slash + 1));
                    const std::string maxv = body.substr(
                        max_open + std::string("（max=").size(),
                        max_close
                            - (max_open + std::string("（max=").size()));
                    std::string trunc_part;
                    std::string rest = body.substr(max_close + std::string("）").size());
                    if (starts_with(rest, "；已截断 ")
                        && ends_with(rest, " 个水平")) {
                        const std::string t = rest.substr(
                            std::string("；已截断 ").size(),
                            rest.size() - std::string("；已截断 ").size()
                                - std::string(" 个水平").size());
                        trunc_part = en ? ("; truncated " + t + " level(s)")
                                        : ("；已截断 " + t + " 个水平");
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.eda_facet", language, missing_out)
                            .text,
                        {{"%1", panels},
                         {"%2", levels},
                         {"%3", maxv},
                         {"%4", trunc_part}});
                }
            } else if (starts_with(bullet, "因果图效应为“")
                       && bullet.find("”，共 ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。这是结构化头脑风暴摘要，不是统计检验，不能写成已证明根因。")) {
                const std::string body = bullet.substr(
                    std::string("因果图效应为“").size(),
                    bullet.size() - std::string("因果图效应为“").size()
                        - std::string(
                              "。这是结构化头脑风暴摘要，不是统计检验，不能写成已证明根因。")
                              .size());
                const auto q_end = body.find("”，共 ");
                const auto cat_mark = body.find(" 个类别、");
                const auto cause_mark = body.find(" 条原因");
                if (q_end != std::string::npos && cat_mark != std::string::npos
                    && cause_mark != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.cause_effect_summary", language, missing_out)
                            .text,
                        {{"%1", body.substr(0, q_end)},
                         {"%2",
                          body.substr(
                              q_end + std::string("”，共 ").size(),
                              cat_mark
                                  - (q_end + std::string("”，共 ").size()))},
                         {"%3",
                          body.substr(
                              cat_mark + std::string(" 个类别、").size(),
                              cause_mark
                                  - (cat_mark + std::string(" 个类别、").size()))}});
                }
            } else if (starts_with(bullet, "区间删失 KM（Turnbull）：N = ")
                       && bullet.find("，左/区间/右 = ") != std::string::npos
                       && ends_with(bullet, "。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("区间删失 KM（Turnbull）：N = ").size(),
                    bullet.size()
                        - std::string("区间删失 KM（Turnbull）：N = ").size()
                        - std::string("。").size());
                const auto c_mark = body.find("，左/区间/右 = ");
                if (c_mark != std::string::npos) {
                    const std::string n = body.substr(0, c_mark);
                    std::string rest = body.substr(
                        c_mark + std::string("，左/区间/右 = ").size());
                    std::string median_part;
                    const auto med_mark = rest.find("；中位寿命 ≈ ");
                    const auto miss_mark = rest.find("；中位寿命未估出");
                    if (med_mark != std::string::npos) {
                        median_part = en
                            ? ("; median life ≈ "
                               + rest.substr(
                                     med_mark
                                     + std::string("；中位寿命 ≈ ").size()))
                            : rest.substr(med_mark);
                        rest = rest.substr(0, med_mark);
                    } else if (miss_mark != std::string::npos) {
                        median_part = en ? "; median life not estimated"
                                         : "；中位寿命未估出";
                        rest = rest.substr(0, miss_mark);
                    }
                    const auto s1 = rest.find('/');
                    const auto s2 = rest.find('/', s1 == std::string::npos
                                                     ? 0
                                                     : s1 + 1);
                    if (s1 != std::string::npos && s2 != std::string::npos) {
                        bullet = replace_all_tokens(
                            domain::resolve_report_text(
                                "interp.km_interval_turnbull", language,
                                missing_out)
                                .text,
                            {{"%1", n},
                             {"%2", rest.substr(0, s1)},
                             {"%3", rest.substr(s1 + 1, s2 - (s1 + 1))},
                             {"%4", rest.substr(s2 + 1)},
                             {"%5", median_part}});
                    }
                }
            } else if (starts_with(bullet, "Turnbull NPMLE（简化网格）；evidence_type=")
                       && bullet.find("；algorithm_id=") != std::string::npos
                       && ends_with(
                           bullet,
                           "；非右删失 product-limit；非参数寿命模型；不得写成 vendor_oracle/golden。")) {
                const std::string body = bullet.substr(
                    std::string("Turnbull NPMLE（简化网格）；evidence_type=")
                        .size(),
                    bullet.size()
                        - std::string(
                              "Turnbull NPMLE（简化网格）；evidence_type=")
                              .size()
                        - std::string(
                              "；非右删失 product-limit；非参数寿命模型；不得写成 vendor_oracle/golden。")
                              .size());
                const auto aid = body.find("；algorithm_id=");
                if (aid != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.km_interval_honesty", language, missing_out)
                            .text,
                        {{"%1", body.substr(0, aid)},
                         {"%2",
                          body.substr(
                              aid + std::string("；algorithm_id=").size())}});
                }
            } else if (starts_with(bullet, "ACF/PACF 基于 N = ")
                       && ends_with(
                           bullet,
                           "。越过带宽只提示相对白噪声零假设的相关证据，不能写成过程失控判定。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("ACF/PACF 基于 N = ").size(),
                    bullet.size() - std::string("ACF/PACF 基于 N = ").size()
                        - std::string(
                              "。越过带宽只提示相对白噪声零假设的相关证据，不能写成过程失控判定。")
                              .size());
                const auto lag_mark = body.find("，最大滞后 = ");
                const auto method_mark = body.find("；默认置信带方法 = ");
                if (lag_mark != std::string::npos
                    && method_mark != std::string::npos) {
                    const std::string n = body.substr(0, lag_mark);
                    const std::string lag = body.substr(
                        lag_mark + std::string("，最大滞后 = ").size(),
                        method_mark
                            - (lag_mark + std::string("，最大滞后 = ").size()));
                    std::string rest = body.substr(
                        method_mark + std::string("；默认置信带方法 = ").size());
                    std::string method = rest;
                    std::string half_part;
                    const auto half_mark = rest.find("，半宽 ≈ ");
                    if (half_mark != std::string::npos) {
                        method = rest.substr(0, half_mark);
                        half_part = en
                            ? (", half-width ≈ "
                               + rest.substr(
                                     half_mark + std::string("，半宽 ≈ ").size()))
                            : rest.substr(half_mark);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.acf_pacf_summary", language, missing_out)
                            .text,
                        {{"%1", n},
                         {"%2", lag},
                         {"%3", method},
                         {"%4", half_part}});
                }
            } else if (starts_with(bullet, "Ljung–Box P ≈ ")
                       && ends_with(
                           bullet,
                           "；小 P 提示序列存在总体自相关结构，应结合 ACF/PACF 图与工艺时序解释。")) {
                const std::string p = bullet.substr(
                    std::string("Ljung–Box P ≈ ").size(),
                    bullet.size() - std::string("Ljung–Box P ≈ ").size()
                        - std::string(
                              "；小 P 提示序列存在总体自相关结构，应结合 ACF/PACF 图与工艺时序解释。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.ljung_box_p", language, missing_out)
                        .text,
                    "%1",
                    p);
            } else if (starts_with(bullet, "Z-MR 图 N = ")
                       && bullet.find("，组数 = ") != std::string::npos
                       && bullet.find("，Z 图超限 = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "，超限点需要调查，不能直接当作过程失控或稳定结论。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("Z-MR 图 N = ").size(),
                    bullet.size() - std::string("Z-MR 图 N = ").size()
                        - std::string(
                              "，超限点需要调查，不能直接当作过程失控或稳定结论。")
                              .size());
                const auto g_mark = body.find("，组数 = ");
                const auto z_mark = body.find("，Z 图超限 = ");
                const auto dep_mark = body.find("；短流程标准化依赖 ");
                if (g_mark != std::string::npos && z_mark != std::string::npos
                    && dep_mark != std::string::npos) {
                    const std::string n = body.substr(0, g_mark);
                    const std::string groups = body.substr(
                        g_mark + std::string("，组数 = ").size(),
                        z_mark - (g_mark + std::string("，组数 = ").size()));
                    const std::string ooc = body.substr(
                        z_mark + std::string("，Z 图超限 = ").size(),
                        dep_mark - (z_mark + std::string("，Z 图超限 = ").size()));
                    std::string dep = body.substr(
                        dep_mark + std::string("；短流程标准化依赖 ").size());
                    if (en) {
                        if (dep == "样本估计 μ/σ") {
                            dep = "sample-estimated μ/σ";
                        } else if (dep == "历史 μ/σ") {
                            dep = "historical μ/σ";
                        }
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.z_mr_summary", language, missing_out)
                            .text,
                        {{"%1", n},
                         {"%2", groups},
                         {"%3", ooc},
                         {"%4", dep}});
                }
            } else if (starts_with(bullet, "移动平均图窗宽 w = ")
                       && bullet.find("，「单点超出 3σ 控制限」触发 = ")
                           != std::string::npos
                       && ends_with(
                           bullet,
                           "，不能将统计信号直接等同于过程失控判定或稳定结论。")) {
                const std::string body = bullet.substr(
                    std::string("移动平均图窗宽 w = ").size(),
                    bullet.size() - std::string("移动平均图窗宽 w = ").size()
                        - std::string(
                              "，不能将统计信号直接等同于过程失控判定或稳定结论。")
                              .size());
                const auto t_mark =
                    body.find("，「单点超出 3σ 控制限」触发 = ");
                const auto semi = body.find("；MA 平滑后的出限是信号提示");
                if (t_mark != std::string::npos) {
                    const std::string w = body.substr(0, t_mark);
                    std::string triggers = body.substr(
                        t_mark + std::string("，「单点超出 3σ 控制限」触发 = ").size());
                    if (semi != std::string::npos && semi > t_mark) {
                        triggers = body.substr(
                            t_mark
                                + std::string("，「单点超出 3σ 控制限」触发 = ")
                                      .size(),
                            semi
                                - (t_mark
                                   + std::string(
                                         "，「单点超出 3σ 控制限」触发 = ")
                                         .size()));
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.ma_chart_summary", language, missing_out)
                            .text,
                        {{"%1", w}, {"%2", triggers}});
                }
            } else if (starts_with(bullet, "运行图 N = ")
                       && bullet.find("；关于中位数游程 = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。四模式 P 只提示相对随机性的偏离方向，不能写成已证明过程受控或失控。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("运行图 N = ").size(),
                    bullet.size() - std::string("运行图 N = ").size()
                        - std::string(
                              "。四模式 P 只提示相对随机性的偏离方向，不能写成已证明过程受控或失控。")
                              .size());
                const auto run_mark = body.find("；关于中位数游程 = ");
                const auto up_mark = body.find("，上升/下降游程 = ");
                if (run_mark != std::string::npos
                    && up_mark != std::string::npos) {
                    std::string left = body.substr(0, run_mark);
                    std::string n = left;
                    std::string med_part;
                    const auto med_mark = left.find("，中位数 = ");
                    if (med_mark != std::string::npos) {
                        n = left.substr(0, med_mark);
                        med_part = en
                            ? (", median = "
                               + left.substr(
                                     med_mark + std::string("，中位数 = ").size()))
                            : left.substr(med_mark);
                    }
                    const std::string about = body.substr(
                        run_mark + std::string("；关于中位数游程 = ").size(),
                        up_mark
                            - (run_mark
                               + std::string("；关于中位数游程 = ").size()));
                    const std::string updown = body.substr(
                        up_mark + std::string("，上升/下降游程 = ").size());
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.run_chart_summary", language, missing_out)
                            .text,
                        {{"%1", n},
                         {"%2", med_part},
                         {"%3", about},
                         {"%4", updown}});
                }
            } else if (starts_with(bullet, "区域图 Jaehn 累计得分达到阈值 ")
                       && bullet.find(" 的点有 ") != std::string::npos
                       && ends_with(
                           bullet,
                           "，不能将统计信号直接等同于过程失控判定或稳定结论。")) {
                const auto mid = bullet.find(" 的点有 ");
                const auto semi = bullet.find("；这是统计信号提示");
                if (mid != std::string::npos) {
                    const std::string thr = bullet.substr(
                        std::string("区域图 Jaehn 累计得分达到阈值 ").size(),
                        mid
                            - std::string("区域图 Jaehn 累计得分达到阈值 ")
                                  .size());
                    std::string count = bullet.substr(
                        mid + std::string(" 的点有 ").size());
                    if (semi != std::string::npos && semi > mid) {
                        count = bullet.substr(
                            mid + std::string(" 的点有 ").size(),
                            semi - (mid + std::string(" 的点有 ").size()));
                    } else if (ends_with(count, " 个")) {
                        // strip trailing before english template uses %2 as count
                        const auto end = count.find(" 个");
                        if (end != std::string::npos) {
                            count = count.substr(0, end);
                        }
                    }
                    // Prefer count up to " 个"
                    const auto ge = count.find(" 个");
                    if (ge != std::string::npos) {
                        count = count.substr(0, ge);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.zone_chart_summary", language, missing_out)
                            .text,
                        {{"%1", thr}, {"%2", count}});
                }
            } else if (starts_with(bullet, "Plackett–Burman：因子 = ")
                       && ends_with(
                           bullet, "。设计矩阵仅供筛选实验排布。")) {
                const std::string body = bullet.substr(
                    std::string("Plackett–Burman：因子 = ").size(),
                    bullet.size()
                        - std::string("Plackett–Burman：因子 = ").size()
                        - std::string("。设计矩阵仅供筛选实验排布。").size());
                const auto r_mark = body.find("，运行 = ");
                const auto c_mark = body.find("，中心点 = ");
                if (r_mark != std::string::npos && c_mark != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.plackett_burman_summary", language,
                            missing_out)
                            .text,
                        {{"%1", body.substr(0, r_mark)},
                         {"%2",
                          body.substr(
                              r_mark + std::string("，运行 = ").size(),
                              c_mark
                                  - (r_mark + std::string("，运行 = ").size()))},
                         {"%3",
                          body.substr(
                              c_mark + std::string("，中心点 = ").size())}});
                }
            } else if (starts_with(bullet, "Box–Behnken：因子 = ")
                       && ends_with(
                           bullet, "。不包含所有因子同时极端的角点。")) {
                const std::string body = bullet.substr(
                    std::string("Box–Behnken：因子 = ").size(),
                    bullet.size() - std::string("Box–Behnken：因子 = ").size()
                        - std::string("。不包含所有因子同时极端的角点。")
                              .size());
                const auto e_mark = body.find("，边中点 = ");
                const auto c_mark = body.find("，中心点 = ");
                const auto t_mark = body.find("，总运行 = ");
                if (e_mark != std::string::npos && c_mark != std::string::npos
                    && t_mark != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.bbd_design_summary", language, missing_out)
                            .text,
                        {{"%1", body.substr(0, e_mark)},
                         {"%2",
                          body.substr(
                              e_mark + std::string("，边中点 = ").size(),
                              c_mark
                                  - (e_mark
                                     + std::string("，边中点 = ").size()))},
                         {"%3",
                          body.substr(
                              c_mark + std::string("，中心点 = ").size(),
                              t_mark
                                  - (c_mark
                                     + std::string("，中心点 = ").size()))},
                         {"%4",
                          body.substr(
                              t_mark + std::string("，总运行 = ").size())}});
                }
            } else if (starts_with(bullet, "CCD（")
                       && bullet.find("）：因子 = ") != std::string::npos
                       && ends_with(bullet, "。")) {
                const std::string body = bullet.substr(
                    std::string("CCD（").size(),
                    bullet.size() - std::string("CCD（").size()
                        - std::string("。").size());
                const auto v_end = body.find("）：因子 = ");
                const auto cube = body.find("，立方点 = ");
                const auto star = body.find("，星点 = ");
                const auto center = body.find("，中心点 = ");
                const auto alpha = body.find("，α = ");
                if (v_end != std::string::npos && cube != std::string::npos
                    && star != std::string::npos && center != std::string::npos
                    && alpha != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.ccd_design_summary", language, missing_out)
                            .text,
                        {{"%1", body.substr(0, v_end)},
                         {"%2",
                          body.substr(
                              v_end + std::string("）：因子 = ").size(),
                              cube
                                  - (v_end
                                     + std::string("）：因子 = ").size()))},
                         {"%3",
                          body.substr(
                              cube + std::string("，立方点 = ").size(),
                              star
                                  - (cube
                                     + std::string("，立方点 = ").size()))},
                         {"%4",
                          body.substr(
                              star + std::string("，星点 = ").size(),
                              center
                                  - (star + std::string("，星点 = ").size()))},
                         {"%5",
                          body.substr(
                              center + std::string("，中心点 = ").size(),
                              alpha
                                  - (center
                                     + std::string("，中心点 = ").size()))},
                         {"%6",
                          body.substr(
                              alpha + std::string("，α = ").size())}});
                }
            } else if (starts_with(bullet, "已按 ")
                       && bullet.find(" 追溯 ") != std::string::npos
                       && ends_with(
                           bullet,
                           " 个分层分母；部分/全部分层 expected_failures 使用 "
                           "cause-specific 分模式 R(T_w)。")) {
                const auto mid = bullet.find(" 追溯 ");
                const auto end = bullet.find(" 个分层分母；");
                if (mid != std::string::npos && end != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.warranty_strata_mode", language, missing_out)
                            .text,
                        {{"%1",
                          bullet.substr(
                              std::string("已按 ").size(),
                              mid - std::string("已按 ").size())},
                         {"%2",
                          bullet.substr(
                              mid + std::string(" 追溯 ").size(),
                              end - (mid + std::string(" 追溯 ").size()))}});
                }
            } else if (starts_with(bullet, "已按 ")
                       && bullet.find(" 追溯 ") != std::string::npos
                       && ends_with(
                           bullet,
                           " 个分层分母；分层 expected_failures 使用池化 R(T_w)。")) {
                const auto mid = bullet.find(" 追溯 ");
                const auto end = bullet.find(" 个分层分母；");
                if (mid != std::string::npos && end != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.warranty_strata_pooled", language,
                            missing_out)
                            .text,
                        {{"%1",
                          bullet.substr(
                              std::string("已按 ").size(),
                              mid - std::string("已按 ").size())},
                         {"%2",
                          bullet.substr(
                              mid + std::string(" 追溯 ").size(),
                              end - (mid + std::string(" 追溯 ").size()))}});
                }
            } else if (starts_with(
                           bullet,
                           "分层未估计分模式可靠度（uses_pooled_reliability=")
                       && ends_with(
                           bullet,
                           "）；比例分摊暴露量须标注 proportional_scalar。")) {
                const std::string v = bullet.substr(
                    std::string(
                        "分层未估计分模式可靠度（uses_pooled_reliability=")
                        .size(),
                    bullet.size()
                        - std::string(
                              "分层未估计分模式可靠度（uses_pooled_reliability=")
                              .size()
                        - std::string(
                              "）；比例分摊暴露量须标注 proportional_scalar。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.warranty_pooled_honesty", language, missing_out)
                        .text,
                    "%1",
                    v);
            } else if (starts_with(bullet, "CCF：N = ")
                       && ends_with(
                           bullet,
                           "。越过带宽只提示相对独立性零假设的相关证据。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("CCF：N = ").size(),
                    bullet.size() - std::string("CCF：N = ").size()
                        - std::string(
                              "。越过带宽只提示相对独立性零假设的相关证据。")
                              .size());
                const auto lag = body.find("，max|lag| = ");
                if (lag != std::string::npos) {
                    const std::string n = body.substr(0, lag);
                    std::string rest = body.substr(
                        lag + std::string("，max|lag| = ").size());
                    std::string maxlag = rest;
                    std::string z_part;
                    const auto z_mark = rest.find("；lag0 ≈ ");
                    if (z_mark != std::string::npos) {
                        maxlag = rest.substr(0, z_mark);
                        z_part = en
                            ? ("; lag0 ≈ "
                               + rest.substr(
                                     z_mark + std::string("；lag0 ≈ ").size()))
                            : rest.substr(z_mark);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.ccf_summary", language, missing_out)
                            .text,
                        {{"%1", n}, {"%2", maxlag}, {"%3", z_part}});
                }
            } else if (starts_with(bullet, "Correlogram（")
                       && ends_with(
                           bullet, "。热图只展示两两相关结构。")) {
                const std::string body = bullet.substr(
                    std::string("Correlogram（").size(),
                    bullet.size() - std::string("Correlogram（").size()
                        - std::string("。热图只展示两两相关结构。").size());
                const auto m_end = body.find("）：变量数 = ");
                const auto p_mark = body.find("，成对数 = ");
                if (m_end != std::string::npos && p_mark != std::string::npos) {
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.correlogram_summary", language, missing_out)
                            .text,
                        {{"%1", body.substr(0, m_end)},
                         {"%2",
                          body.substr(
                              m_end + std::string("）：变量数 = ").size(),
                              p_mark
                                  - (m_end
                                     + std::string("）：变量数 = ").size()))},
                         {"%3",
                          body.substr(
                              p_mark + std::string("，成对数 = ").size())}});
                }
            } else if (starts_with(bullet, "逐步回归（")
                       && ends_with(
                           bullet, "。选入项只描述相对拟合证据。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("逐步回归（").size(),
                    bullet.size() - std::string("逐步回归（").size()
                        - std::string("。选入项只描述相对拟合证据。").size());
                const auto m_end = body.find("）：N = ");
                const auto c_mark = body.find("，候选 = ");
                const auto s_mark = body.find("，选入 = ");
                if (m_end != std::string::npos && c_mark != std::string::npos
                    && s_mark != std::string::npos) {
                    const std::string method = body.substr(0, m_end);
                    const std::string n = body.substr(
                        m_end + std::string("）：N = ").size(),
                        c_mark - (m_end + std::string("）：N = ").size()));
                    const std::string cand = body.substr(
                        c_mark + std::string("，候选 = ").size(),
                        s_mark - (c_mark + std::string("，候选 = ").size()));
                    std::string rest = body.substr(
                        s_mark + std::string("，选入 = ").size());
                    std::string sel = rest;
                    std::string r2_part;
                    const auto r2 = rest.find("；R² ≈ ");
                    if (r2 != std::string::npos) {
                        sel = rest.substr(0, r2);
                        r2_part = en
                            ? ("; R² ≈ "
                               + rest.substr(r2 + std::string("；R² ≈ ").size()))
                            : rest.substr(r2);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.stepwise_summary", language, missing_out)
                            .text,
                        {{"%1", method},
                         {"%2", n},
                         {"%3", cand},
                         {"%4", sel},
                         {"%5", r2_part}});
                }
            } else if (starts_with(bullet, "Multi-Vari 图用 ")
                       && bullet.find(" 个因子分层显示 ") != std::string::npos
                       && ends_with(bullet, "。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("Multi-Vari 图用 ").size(),
                    bullet.size() - std::string("Multi-Vari 图用 ").size()
                        - std::string("。").size());
                const auto mid = body.find(" 个因子分层显示 ");
                const auto valid_end = body.find(" 个有效测量的均值");
                if (mid != std::string::npos && valid_end != std::string::npos) {
                    const std::string factors = body.substr(0, mid);
                    const std::string valid = body.substr(
                        mid + std::string(" 个因子分层显示 ").size(),
                        valid_end
                            - (mid + std::string(" 个因子分层显示 ").size()));
                    std::string names_part = body.substr(
                        valid_end + std::string(" 个有效测量的均值").size());
                    if (en && starts_with(names_part, "（")
                        && ends_with(names_part, "）")) {
                        names_part = " ("
                            + names_part.substr(
                                  std::string("（").size(),
                                  names_part.size() - std::string("（").size()
                                      - std::string("）").size())
                            + ")";
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.multi_vari_summary", language, missing_out)
                            .text,
                        {{"%1", factors},
                         {"%2", valid},
                         {"%3", names_part}});
                }
            } else if (starts_with(bullet, "已跳过 ")
                       && ends_with(bullet, " 个缺失或不完整行。")) {
                const std::string n = bullet.substr(
                    std::string("已跳过 ").size(),
                    bullet.size() - std::string("已跳过 ").size()
                        - std::string(" 个缺失或不完整行。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.skipped_incomplete_rows", language, missing_out)
                        .text,
                    "%1",
                    n);
            } else if (starts_with(bullet, "变异性图按 ")
                       && bullet.find(" 个因子汇总 ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。图只用于探索变异结构，不是显著性检验或过程判定。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string("变异性图按 ").size(),
                    bullet.size() - std::string("变异性图按 ").size()
                        - std::string(
                              "。图只用于探索变异结构，不是显著性检验或过程判定。")
                              .size());
                const auto f_mark = body.find(" 个因子汇总 ");
                const auto cell_mark = body.find(" 个单元、");
                const auto valid_mark = body.find(" 个有效测量的均值与标准差");
                if (f_mark != std::string::npos && cell_mark != std::string::npos
                    && valid_mark != std::string::npos) {
                    const std::string factors = body.substr(0, f_mark);
                    const std::string cells = body.substr(
                        f_mark + std::string(" 个因子汇总 ").size(),
                        cell_mark
                            - (f_mark + std::string(" 个因子汇总 ").size()));
                    const std::string valid = body.substr(
                        cell_mark + std::string(" 个单元、").size(),
                        valid_mark
                            - (cell_mark + std::string(" 个单元、").size()));
                    std::string rest = body.substr(
                        valid_mark
                        + std::string(" 个有效测量的均值与标准差").size());
                    std::string mean_part;
                    std::string sd_part;
                    const auto mean_mark = rest.find("，总均值 = ");
                    const auto sd_mark = rest.find("，单元 StDev 平均 = ");
                    if (mean_mark != std::string::npos) {
                        std::size_t end = rest.size();
                        if (sd_mark != std::string::npos) {
                            end = sd_mark;
                        }
                        mean_part = en
                            ? (", overall mean = "
                               + rest.substr(
                                     mean_mark + std::string("，总均值 = ").size(),
                                     end
                                         - (mean_mark
                                            + std::string("，总均值 = ").size())))
                            : rest.substr(mean_mark, end - mean_mark);
                        rest = rest.substr(end);
                    }
                    if (starts_with(rest, "，单元 StDev 平均 = ")) {
                        sd_part = en
                            ? (", mean of cell SDs = "
                               + rest.substr(
                                     std::string("，单元 StDev 平均 = ").size()))
                            : rest;
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.variability_summary", language, missing_out)
                            .text,
                        {{"%1", factors},
                         {"%2", cells},
                         {"%3", valid},
                         {"%4", mean_part},
                         {"%5", sd_part}});
                }
            } else if (starts_with(bullet, "容差区间方法为 ")
                       && ends_with(
                           bullet,
                           "。区间只描述当前样本覆盖，不是规格覆盖或过程判定。")) {
                const bool en = language != "zh-CN" && language != "zh";
                std::string body = bullet.substr(
                    std::string("容差区间方法为 ").size(),
                    bullet.size() - std::string("容差区间方法为 ").size()
                        - std::string(
                              "。区间只描述当前样本覆盖，不是规格覆盖或过程判定。")
                              .size());
                std::string method;
                std::string family_part;
                std::string coverage_part;
                std::string conf_part;
                std::string achieved_part;
                std::string lower_part;
                std::string upper_part;
                const auto fam_open = body.find("（");
                const auto cov = body.find("，覆盖率 ");
                const auto conf = body.find("，置信水平 ");
                const auto ach = body.find("，achieved confidence ");
                const auto lo = body.find("，下限 ");
                const auto hi = body.find("，上限 ");
                std::size_t method_end = body.size();
                if (fam_open != std::string::npos) {
                    method_end = fam_open;
                } else if (cov != std::string::npos) {
                    method_end = cov;
                } else if (conf != std::string::npos) {
                    method_end = conf;
                } else if (ach != std::string::npos) {
                    method_end = ach;
                } else if (lo != std::string::npos) {
                    method_end = lo;
                } else if (hi != std::string::npos) {
                    method_end = hi;
                }
                method = body.substr(0, method_end);
                if (en && method == "未计算") {
                    method = "not computed";
                }
                if (fam_open != std::string::npos) {
                    const auto fam_close = body.find("）", fam_open);
                    if (fam_close != std::string::npos) {
                        const std::string fam = body.substr(
                            fam_open + std::string("（").size(),
                            fam_close - (fam_open + std::string("（").size()));
                        family_part = en ? (" (" + fam + ")")
                                         : ("（" + fam + "）");
                    }
                }
                auto take_part = [&](const std::string& mark, std::string& out,
                                     const std::string& en_label) {
                    const auto pos = body.find(mark);
                    if (pos == std::string::npos) {
                        return;
                    }
                    std::size_t end = body.size();
                    for (const auto& next :
                         {std::string("，覆盖率 "),
                          std::string("，置信水平 "),
                          std::string("，achieved confidence "),
                          std::string("，下限 "),
                          std::string("，上限 ")}) {
                        if (next == mark) {
                            continue;
                        }
                        const auto np = body.find(next, pos + mark.size());
                        if (np != std::string::npos && np < end) {
                            end = np;
                        }
                    }
                    const std::string val =
                        body.substr(pos + mark.size(), end - (pos + mark.size()));
                    out = en ? (", " + en_label + " = " + val)
                             : (mark + val);
                };
                take_part("，覆盖率 ", coverage_part, "coverage");
                take_part("，置信水平 ", conf_part, "confidence");
                take_part("，achieved confidence ", achieved_part,
                          "achieved confidence");
                take_part("，下限 ", lower_part, "lower");
                take_part("，上限 ", upper_part, "upper");
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.tolerance_summary", language, missing_out)
                        .text,
                    {{"%1", method},
                     {"%2", family_part},
                     {"%3", coverage_part},
                     {"%4", conf_part},
                     {"%5", achieved_part},
                     {"%6", lower_part},
                     {"%7", upper_part}});
            } else if (starts_with(
                           bullet,
                           "本方法假设测量近似正态且未在本页验证（assumption_status=")
                       && ends_with(
                           bullet,
                           "）；单侧使用 Natrella 近似，双侧使用 Howe 近似。")) {
                const std::string status = bullet.substr(
                    std::string(
                        "本方法假设测量近似正态且未在本页验证（assumption_status=")
                        .size(),
                    bullet.size()
                        - std::string(
                              "本方法假设测量近似正态且未在本页验证（assumption_status=")
                              .size()
                        - std::string(
                              "）；单侧使用 Natrella 近似，双侧使用 Howe 近似。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.tolerance_normal_honesty", language, missing_out)
                        .text,
                    "%1",
                    status);
            } else if (starts_with(bullet, "已跳过 ")
                       && ends_with(bullet, " 个缺失或非法数值。")) {
                const std::string n = bullet.substr(
                    std::string("已跳过 ").size(),
                    bullet.size() - std::string("已跳过 ").size()
                        - std::string(" 个缺失或非法数值。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.skipped_illegal_values", language, missing_out)
                        .text,
                    "%1",
                    n);
            } else if (starts_with(
                           bullet,
                           "个体分布识别按 Anderson-Darling 升序比较四族二参数分布；当前表内最优为 ")
                       && ends_with(
                           bullet,
                           "。排序结果不证明数据服从该分布，也不自动改写过程能力默认方法。")) {
                const bool en = language != "zh-CN" && language != "zh";
                const std::string body = bullet.substr(
                    std::string(
                        "个体分布识别按 Anderson-Darling 升序比较四族二参数分布；当前表内最优为 ")
                        .size(),
                    bullet.size()
                        - std::string(
                              "个体分布识别按 Anderson-Darling 升序比较四族二参数分布；当前表内最优为 ")
                              .size()
                        - std::string(
                              "。排序结果不证明数据服从该分布，也不自动改写过程能力默认方法。")
                              .size());
                std::string best = body;
                std::string ad_part;
                if (body.find("（AD = ") != std::string::npos
                    && ends_with(body, "）")) {
                    const auto ad = body.find("（AD = ");
                    best = body.substr(0, ad);
                    const std::string val = body.substr(
                        ad + std::string("（AD = ").size(),
                        body.size() - (ad + std::string("（AD = ").size())
                            - std::string("）").size());
                    ad_part = en ? (" (AD = " + val + ")")
                                 : ("（AD = " + val + "）");
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.distribution_id_summary", language, missing_out)
                        .text,
                    {{"%1", best}, {"%2", ad_part}});
            } else if (starts_with(bullet, "等方差检验方法为 ")
                       && bullet.find("。未拒绝原假设不能证明方差相等。")
                           != std::string::npos) {
                const auto note_mark =
                    bullet.find("。未拒绝原假设不能证明方差相等。");
                const std::string head = bullet.substr(
                    std::string("等方差检验方法为 ").size(),
                    note_mark - std::string("等方差检验方法为 ").size());
                std::string method = head;
                std::string p_part;
                const auto p_mark = head.find("，P = ");
                if (p_mark != std::string::npos) {
                    method = head.substr(0, p_mark);
                    p_part = ", P = "
                        + head.substr(p_mark + std::string("，P = ").size());
                    if (language == "zh-CN" || language == "zh") {
                        p_part = "，P = "
                            + head.substr(p_mark + std::string("，P = ").size());
                    }
                }
                std::string note = bullet.substr(
                    note_mark
                    + std::string("。未拒绝原假设不能证明方差相等。").size());
                std::string note_id = "interp.equal_variance_levene";
                if (starts_with(note, "Bonett ")) {
                    note_id = "interp.equal_variance_bonett";
                } else if (starts_with(note, "Bartlett ")) {
                    note_id = "interp.equal_variance_bartlett";
                } else if (starts_with(note, "F 检验")) {
                    note_id = "interp.equal_variance_f";
                }
                const std::string note_text = domain::resolve_report_text(
                                                  note_id, language, missing_out)
                                                  .text;
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.equal_variance_summary", language, missing_out)
                        .text,
                    {{"%1", method}, {"%2", p_part}, {"%3", note_text}});
            } else if (starts_with(bullet, "Kaplan-Meier 有效观测 ")
                       && bullet.find("，失效 ") != std::string::npos
                       && bullet.find("，删失 ") != std::string::npos
                       && ends_with(bullet, "。")) {
                const auto fmark = bullet.find("，失效 ");
                const auto cmark = bullet.find("，删失 ");
                if (fmark != std::string::npos && cmark != std::string::npos
                    && fmark < cmark) {
                    const std::string valid = bullet.substr(
                        std::string("Kaplan-Meier 有效观测 ").size(),
                        fmark - std::string("Kaplan-Meier 有效观测 ").size());
                    const std::string fails = bullet.substr(
                        fmark + std::string("，失效 ").size(),
                        cmark - (fmark + std::string("，失效 ").size()));
                    const std::string cens = bullet.substr(
                        cmark + std::string("，删失 ").size(),
                        bullet.size() - (cmark + std::string("，删失 ").size())
                            - std::string("。").size());
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.km_counts_summary", language, missing_out)
                            .text,
                        {{"%1", valid}, {"%2", fails}, {"%3", cens}});
                }
            } else if (starts_with(bullet, "已对 ")
                       && bullet.find(" 个失效模式做 cause-specific 分模式拟合（scheme=")
                           != std::string::npos
                       && ends_with(bullet, "）。")) {
                const auto mid = bullet.find(
                    " 个失效模式做 cause-specific 分模式拟合（scheme=");
                const std::string count = bullet.substr(
                    std::string("已对 ").size(),
                    mid - std::string("已对 ").size());
                const std::string scheme = bullet.substr(
                    mid
                        + std::string(
                              " 个失效模式做 cause-specific 分模式拟合（scheme=")
                              .size(),
                    bullet.size()
                        - (mid
                           + std::string(
                                 " 个失效模式做 cause-specific 分模式拟合（scheme=")
                                 .size())
                        - std::string("）。").size());
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.rel_mode_fits_count", language, missing_out)
                        .text,
                    {{"%1", count}, {"%2", scheme}});
            } else if (starts_with(bullet, "已计算 ")
                       && bullet.find(
                              " 个失效模式的 Aalen–Johansen 累计发生函数 CIF（algorithm=")
                           != std::string::npos
                       && ends_with(bullet, "）。")) {
                const auto mid = bullet.find(
                    " 个失效模式的 Aalen–Johansen 累计发生函数 CIF（algorithm=");
                const std::string count = bullet.substr(
                    std::string("已计算 ").size(),
                    mid - std::string("已计算 ").size());
                const std::string algorithm = bullet.substr(
                    mid
                        + std::string(
                              " 个失效模式的 Aalen–Johansen 累计发生函数 CIF（algorithm=")
                              .size(),
                    bullet.size()
                        - (mid
                           + std::string(
                                 " 个失效模式的 Aalen–Johansen 累计发生函数 CIF（algorithm=")
                                 .size())
                        - std::string("）。").size());
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.rel_cif_modes_count", language, missing_out)
                        .text,
                    {{"%1", count}, {"%2", algorithm}});
            } else if (starts_with(bullet, "Fine-Gray 多协变量子分布风险：目标原因=")
                       && ends_with(bullet, "。")) {
                const std::string body = bullet.substr(
                    std::string("Fine-Gray 多协变量子分布风险：目标原因=").size(),
                    bullet.size()
                        - std::string("Fine-Gray 多协变量子分布风险：目标原因=").size()
                        - std::string("。").size());
                const auto nmark = body.find("，项数=");
                if (nmark != std::string::npos) {
                    const std::string mode = body.substr(0, nmark);
                    std::string after = body.substr(nmark + std::string("，项数=").size());
                    std::string count;
                    std::string terms;
                    const auto semi = after.find('；');
                    if (semi == std::string::npos) {
                        count = after;
                    } else {
                        count = after.substr(0, semi);
                        terms = after.substr(semi);
                        if (language != "zh-CN" && language != "zh") {
                            for (std::size_t pos = 0;
                                 (pos = terms.find("；", pos)) != std::string::npos;) {
                                terms.replace(pos, std::string("；").size(), "; ");
                                pos += 2;
                            }
                        }
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.rel_fine_gray_multi_summary", language, missing_out)
                            .text,
                        {{"%1", mode}, {"%2", count}, {"%3", terms}});
                }
            } else if (starts_with(bullet, "Fine-Gray 连续协变量子分布风险：目标原因=")
                       && ends_with(bullet, "。")) {
                const std::string body = bullet.substr(
                    std::string("Fine-Gray 连续协变量子分布风险：目标原因=").size(),
                    bullet.size()
                        - std::string("Fine-Gray 连续协变量子分布风险：目标原因=").size()
                        - std::string("。").size());
                const auto cov_mark = body.find("，协变量=");
                const auto beta_mark = body.find("，β=");
                if (cov_mark != std::string::npos && beta_mark != std::string::npos
                    && beta_mark > cov_mark) {
                    const std::string mode = body.substr(0, cov_mark);
                    const std::string cov = body.substr(
                        cov_mark + std::string("，协变量=").size(),
                        beta_mark - (cov_mark + std::string("，协变量=").size()));
                    std::string rest = body.substr(beta_mark + std::string("，β=").size());
                    std::string beta;
                    std::string hr_part;
                    std::string p_part;
                    const auto hr_mark = rest.find("，HR(+1)=");
                    const auto p_mark = rest.find("，P=");
                    std::size_t beta_end = rest.size();
                    if (hr_mark != std::string::npos) {
                        beta_end = hr_mark;
                    } else if (p_mark != std::string::npos) {
                        beta_end = p_mark;
                    }
                    beta = rest.substr(0, beta_end);
                    const bool en = language != "zh-CN" && language != "zh";
                    if (hr_mark != std::string::npos) {
                        std::size_t hr_end = rest.size();
                        if (p_mark != std::string::npos && p_mark > hr_mark) {
                            hr_end = p_mark;
                        }
                        const std::string hr_val = rest.substr(
                            hr_mark + std::string("，HR(+1)=").size(),
                            hr_end - (hr_mark + std::string("，HR(+1)=").size()));
                        hr_part = en ? (", HR(+1)=" + hr_val) : ("，HR(+1)=" + hr_val);
                    }
                    if (p_mark != std::string::npos) {
                        const std::string p_val =
                            rest.substr(p_mark + std::string("，P=").size());
                        p_part = en ? (", P=" + p_val) : ("，P=" + p_val);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.rel_fine_gray_continuous_summary", language,
                            missing_out)
                            .text,
                        {{"%1", mode},
                         {"%2", cov},
                         {"%3", beta},
                         {"%4", hr_part},
                         {"%5", p_part}});
                }
            } else if (starts_with(bullet, "Fine-Gray 二分类子分布风险：目标原因=")
                       && ends_with(bullet, "。")) {
                const std::string body = bullet.substr(
                    std::string("Fine-Gray 二分类子分布风险：目标原因=").size(),
                    bullet.size()
                        - std::string("Fine-Gray 二分类子分布风险：目标原因=").size()
                        - std::string("。").size());
                // body: mode，group g0→0 / g1→1，β=...[，HR=...][，P=...]
                const auto gmark = body.find("，group ");
                const auto arrow = body.find("→0 / ");
                const auto beta_mark = body.find("，β=");
                if (gmark != std::string::npos && arrow != std::string::npos
                    && beta_mark != std::string::npos && arrow > gmark) {
                    const std::string mode = body.substr(0, gmark);
                    const std::string g0 = body.substr(
                        gmark + std::string("，group ").size(),
                        arrow - (gmark + std::string("，group ").size()));
                    const std::string after_arrow = body.substr(
                        arrow + std::string("→0 / ").size());
                    const auto to1 = after_arrow.find("→1");
                    const std::string g1 = to1 == std::string::npos
                        ? after_arrow
                        : after_arrow.substr(0, to1);
                    std::string rest = body.substr(beta_mark + std::string("，β=").size());
                    std::string beta;
                    std::string hr_part;
                    std::string p_part;
                    const auto hr_mark = rest.find("，HR=");
                    const auto p_mark = rest.find("，P=");
                    std::size_t beta_end = rest.size();
                    if (hr_mark != std::string::npos) {
                        beta_end = hr_mark;
                    } else if (p_mark != std::string::npos) {
                        beta_end = p_mark;
                    }
                    beta = rest.substr(0, beta_end);
                    const bool en = language != "zh-CN" && language != "zh";
                    if (hr_mark != std::string::npos) {
                        std::size_t hr_end = rest.size();
                        if (p_mark != std::string::npos && p_mark > hr_mark) {
                            hr_end = p_mark;
                        }
                        const std::string hr_val = rest.substr(
                            hr_mark + std::string("，HR=").size(),
                            hr_end - (hr_mark + std::string("，HR=").size()));
                        hr_part = en ? (", HR=" + hr_val) : ("，HR=" + hr_val);
                    }
                    if (p_mark != std::string::npos) {
                        const std::string p_val =
                            rest.substr(p_mark + std::string("，P=").size());
                        p_part = en ? (", P=" + p_val) : ("，P=" + p_val);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.rel_fine_gray_summary", language, missing_out)
                            .text,
                        {{"%1", mode},
                         {"%2", g0},
                         {"%3", g1},
                         {"%4", beta},
                         {"%5", hr_part},
                         {"%6", p_part}});
                }
            } else if (starts_with(bullet, "Fine-Gray 未计算（")
                       && ends_with(bullet, "）；不得伪造子分布风险比。")) {
                const std::string reason = bullet.substr(
                    std::string("Fine-Gray 未计算（").size(),
                    bullet.size() - std::string("Fine-Gray 未计算（").size()
                        - std::string("）；不得伪造子分布风险比。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.rel_fine_gray_not_computed", language, missing_out)
                        .text,
                    "%1",
                    reason);
            } else if (starts_with(bullet, "可靠性结果不可识别（")
                       && ends_with(bullet, "），不能估计寿命分位数。")) {
                const std::string reason = bullet.substr(
                    std::string("可靠性结果不可识别（").size(),
                    bullet.size() - std::string("可靠性结果不可识别（").size()
                        - std::string("），不能估计寿命分位数。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.rel_not_identifiable_reason", language, missing_out)
                        .text,
                    "%1",
                    reason);
            } else if (starts_with(bullet, "发现 ")
                       && ends_with(bullet, " 个控制图超限点。")) {
                const std::string count = bullet.substr(
                    std::string("发现 ").size(),
                    bullet.size() - std::string("发现 ").size()
                        - std::string(" 个控制图超限点。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.spc_ooc_count", language, missing_out)
                        .text,
                    "%1",
                    count);
            } else if (starts_with(bullet, "特殊原因规则证据：已触发 ")
                       && bullet.find(" / ") != std::string::npos
                       && ends_with(
                           bullet,
                           " 条；详见「特殊原因规则证据」表（含 rule_id、窗口、阈值与状态）。")) {
                const auto mid = bullet.find(" / ");
                const std::string triggered = bullet.substr(
                    std::string("特殊原因规则证据：已触发 ").size(),
                    mid - std::string("特殊原因规则证据：已触发 ").size());
                const std::string total = bullet.substr(
                    mid + std::string(" / ").size(),
                    bullet.size() - (mid + std::string(" / ").size())
                        - std::string(
                              " 条；详见「特殊原因规则证据」表（含 rule_id、窗口、阈值与状态）。")
                              .size());
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.spc_rule_evidence_counts", language, missing_out)
                        .text,
                    {{"%1", triggered}, {"%2", total}});
            } else if (starts_with(bullet, "规则「")
                       && bullet.find("」已触发") != std::string::npos) {
                const auto close = bullet.find("」已触发");
                const std::string name = localize_spc_rule_name(
                    bullet.substr(
                        std::string("规则「").size(),
                        close - std::string("规则「").size()),
                    language,
                    missing_out);
                const std::string rest = bullet.substr(
                    close + std::string("」已触发").size());
                if (rest == "。") {
                    bullet = replace_token(
                        domain::resolve_report_text(
                            "interp.spc_rule_triggered", language, missing_out)
                            .text,
                        "%1",
                        name);
                } else if (starts_with(rest, "：")) {
                    const std::string action = localize_spc_rule_action(
                        rest.substr(std::string("：").size()), language, missing_out);
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.spc_rule_triggered_action", language, missing_out)
                            .text,
                        {{"%1", name}, {"%2", action}});
                }
            } else if (starts_with(bullet, "特殊原因策略 = ")) {
                const std::string body =
                    bullet.substr(std::string("特殊原因策略 = ").size());
                const auto paren = body.find("（");
                const std::string policy =
                    paren == std::string::npos ? body : body.substr(0, paren);
                if (body.find("（多规则提高灵敏度也提高误报风险）") != std::string::npos) {
                    bullet = replace_token(
                        domain::resolve_report_text(
                            "interp.spc_policy_all_applicable", language, missing_out)
                            .text,
                        "%1",
                        policy);
                } else if (body.find("（默认接近 Minitab 仅「单点超出 3σ 控制限」）")
                           != std::string::npos) {
                    bullet = replace_token(
                        domain::resolve_report_text(
                            "interp.spc_policy_minitab_like", language, missing_out)
                            .text,
                        "%1",
                        policy);
                } else {
                    bullet = replace_token(
                        domain::resolve_report_text(
                            "interp.spc_policy_plain", language, missing_out)
                            .text,
                        "%1",
                        policy);
                }
            } else if (starts_with(bullet, "效应 Pareto 最大项为 ")
                       && bullet.find("；参考线方法为 ") != std::string::npos
                       && ends_with(
                           bullet,
                           "。条越过参考线只提供统计证据，不表示过程合格。")) {
                const auto mid = bullet.find("；参考线方法为 ");
                const std::string term = bullet.substr(
                    std::string("效应 Pareto 最大项为 ").size(),
                    mid - std::string("效应 Pareto 最大项为 ").size());
                std::string method = bullet.substr(
                    mid + std::string("；参考线方法为 ").size(),
                    bullet.size() - (mid + std::string("；参考线方法为 ").size())
                        - std::string(
                              "。条越过参考线只提供统计证据，不表示过程合格。")
                              .size());
                if (method == "未指定") {
                    method = domain::resolve_report_text(
                                 "interp.method_unspecified", language, missing_out)
                                 .text;
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.doe_pareto_largest", language, missing_out)
                        .text,
                    {{"%1", term}, {"%2", method}});
            } else if (starts_with(bullet, "等值线/曲面轴为 ")
                       && ends_with(bullet, "。")) {
                const std::string body = bullet.substr(
                    std::string("等值线/曲面轴为 ").size(),
                    bullet.size() - std::string("等值线/曲面轴为 ").size()
                        - std::string("。").size());
                const auto and_mark = body.find(" 与 ");
                if (and_mark != std::string::npos) {
                    const auto coded = body.find("；其余因子编码 hold=0：");
                    const auto actual = body.find("；其余因子实际单位 hold：");
                    const std::string x = body.substr(0, and_mark);
                    if (coded != std::string::npos && coded > and_mark) {
                        const std::string y = body.substr(
                            and_mark + std::string(" 与 ").size(),
                            coded - (and_mark + std::string(" 与 ").size()));
                        const std::string held = localize_zh_payload_punctuation(
                            body.substr(
                                coded + std::string("；其余因子编码 hold=0：").size()),
                            language);
                        bullet = replace_all_tokens(
                            domain::resolve_report_text(
                                "interp.doe_contour_axes_coded_hold", language,
                                missing_out)
                                .text,
                            {{"%1", x}, {"%2", y}, {"%3", held}});
                    } else if (actual != std::string::npos && actual > and_mark) {
                        const std::string y = body.substr(
                            and_mark + std::string(" 与 ").size(),
                            actual - (and_mark + std::string(" 与 ").size()));
                        const std::string held = localize_zh_payload_punctuation(
                            body.substr(
                                actual
                                + std::string("；其余因子实际单位 hold：").size()),
                            language);
                        bullet = replace_all_tokens(
                            domain::resolve_report_text(
                                "interp.doe_contour_axes_actual_hold", language,
                                missing_out)
                                .text,
                            {{"%1", x}, {"%2", y}, {"%3", held}});
                    } else {
                        const std::string y =
                            body.substr(and_mark + std::string(" 与 ").size());
                        bullet = replace_all_tokens(
                            domain::resolve_report_text(
                                "interp.doe_contour_axes", language, missing_out)
                                .text,
                            {{"%1", x}, {"%2", y}});
                    }
                }

            } else if (starts_with(bullet, "组内 σ = ")
                       && ends_with(
                           bullet,
                           "。这些标准差只描述当前子组分解，不是规格判定。")) {
                std::string body = bullet.substr(
                    std::string("组内 σ = ").size(),
                    bullet.size() - std::string("组内 σ = ").size()
                        - std::string(
                              "。这些标准差只描述当前子组分解，不是规格判定。")
                              .size());
                std::string method;
                const std::string open_paren = "（";
                const std::string close_paren = "）";
                if (ends_with(body, close_paren)) {
                    const auto open = body.rfind(open_paren);
                    if (open != std::string::npos) {
                        method = body.substr(
                            open + open_paren.size(),
                            body.size() - open - open_paren.size()
                                - close_paren.size());
                        body = body.substr(0, open);
                    }
                }
                const std::string between_mark = "，组间 σ = ";
                const std::string bw_mark = "，σ_BW = ";
                const auto between_pos = body.find(between_mark);
                const auto bw_pos = body.find(bw_mark);
                std::string within;
                std::string between;
                std::string bw;
                if (between_pos != std::string::npos) {
                    within = body.substr(0, between_pos);
                    if (bw_pos != std::string::npos && bw_pos > between_pos) {
                        between = body.substr(
                            between_pos + between_mark.size(),
                            bw_pos - (between_pos + between_mark.size()));
                        bw = body.substr(bw_pos + bw_mark.size());
                    } else {
                        between = body.substr(between_pos + between_mark.size());
                    }
                } else if (bw_pos != std::string::npos) {
                    within = body.substr(0, bw_pos);
                    bw = body.substr(bw_pos + bw_mark.size());
                } else {
                    within = body;
                }
                const bool has_method = !method.empty();
                if (!between.empty() && !bw.empty()) {
                    bullet = has_method
                        ? replace_all_tokens(
                              domain::resolve_report_text(
                                  "interp.spc_sigma_full_method", language,
                                  missing_out)
                                  .text,
                              {{"%1", within},
                               {"%2", between},
                               {"%3", bw},
                               {"%4", method}})
                        : replace_all_tokens(
                              domain::resolve_report_text(
                                  "interp.spc_sigma_full", language, missing_out)
                                  .text,
                              {{"%1", within}, {"%2", between}, {"%3", bw}});
                } else if (!between.empty()) {
                    bullet = has_method
                        ? replace_all_tokens(
                              domain::resolve_report_text(
                                  "interp.spc_sigma_within_between_method",
                                  language, missing_out)
                                  .text,
                              {{"%1", within}, {"%2", between}, {"%3", method}})
                        : replace_all_tokens(
                              domain::resolve_report_text(
                                  "interp.spc_sigma_within_between", language,
                                  missing_out)
                                  .text,
                              {{"%1", within}, {"%2", between}});
                } else if (!bw.empty()) {
                    bullet = has_method
                        ? replace_all_tokens(
                              domain::resolve_report_text(
                                  "interp.spc_sigma_within_bw_method", language,
                                  missing_out)
                                  .text,
                              {{"%1", within}, {"%2", bw}, {"%3", method}})
                        : replace_all_tokens(
                              domain::resolve_report_text(
                                  "interp.spc_sigma_within_bw", language,
                                  missing_out)
                                  .text,
                              {{"%1", within}, {"%2", bw}});
                } else {
                    bullet = has_method
                        ? replace_all_tokens(
                              domain::resolve_report_text(
                                  "interp.spc_sigma_within_method", language,
                                  missing_out)
                                  .text,
                              {{"%1", within}, {"%2", method}})
                        : replace_token(
                              domain::resolve_report_text(
                                  "interp.spc_sigma_within", language, missing_out)
                                  .text,
                              "%1",
                              within);
                }
            } else if (starts_with(bullet, "单样本 Z 检验（已知 σ")
                       && ends_with(
                           bullet,
                           "。P 值与置信区间只描述当前样本证据，统计显著不等于工程差异具有实际重要性，也不是过程合格判定。")) {
                const std::string suffix =
                    "。P 值与置信区间只描述当前样本证据，统计显著不等于工程差异具有实际重要性，也不是过程合格判定。";
                const std::string prefix = "单样本 Z 检验（已知 σ";
                std::string mid = bullet.substr(
                    prefix.size(),
                    bullet.size() - prefix.size() - suffix.size());
                std::string sigma_part;
                std::string z_part;
                std::string p_part;
                std::string diff_part;
                const auto close = mid.find("）");
                if (close != std::string::npos) {
                    sigma_part = mid.substr(0, close);
                    mid = mid.substr(close + std::string("）").size());
                }
                const auto z_mark = mid.find("，Z = ");
                const auto p_mark = mid.find("，P = ");
                const auto d_mark = mid.find("，差值 = ");
                const bool en = language != "zh-CN" && language != "zh";
                if (z_mark != std::string::npos) {
                    std::size_t z_end = mid.size();
                    if (p_mark != std::string::npos && p_mark > z_mark) {
                        z_end = p_mark;
                    } else if (d_mark != std::string::npos && d_mark > z_mark) {
                        z_end = d_mark;
                    }
                    if (en) {
                        z_part = ", Z = "
                            + mid.substr(z_mark + std::string("，Z = ").size(),
                                         z_end - (z_mark + std::string("，Z = ").size()));
                    } else {
                        z_part = mid.substr(z_mark, z_end - z_mark);
                    }
                }
                if (p_mark != std::string::npos) {
                    std::size_t p_end = mid.size();
                    if (d_mark != std::string::npos && d_mark > p_mark) {
                        p_end = d_mark;
                    }
                    if (en) {
                        p_part = ", P = "
                            + mid.substr(p_mark + std::string("，P = ").size(),
                                         p_end - (p_mark + std::string("，P = ").size()));
                    } else {
                        p_part = mid.substr(p_mark, p_end - p_mark);
                    }
                }
                if (d_mark != std::string::npos) {
                    if (en) {
                        diff_part = ", difference = "
                            + mid.substr(d_mark + std::string("，差值 = ").size());
                    } else {
                        diff_part = mid.substr(d_mark);
                    }
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.ttest_one_z", language, missing_out)
                        .text,
                    {{"%1", sigma_part},
                     {"%2", z_part},
                     {"%3", p_part},
                     {"%4", diff_part}});
            } else if (starts_with(bullet, "双样本 t 检验（")
                       && ends_with(
                           bullet,
                           "。P 值与置信区间只描述当前样本证据，统计显著不等于工程差异具有实际重要性，也不是过程合格判定。")) {
                const std::string suffix =
                    "。P 值与置信区间只描述当前样本证据，统计显著不等于工程差异具有实际重要性，也不是过程合格判定。";
                const std::string prefix = "双样本 t 检验（";
                std::string mid = bullet.substr(
                    prefix.size(),
                    bullet.size() - prefix.size() - suffix.size());
                std::string method;
                std::string p_part;
                std::string diff_part;
                const auto close = mid.find("）");
                if (close != std::string::npos) {
                    method = mid.substr(0, close);
                    mid = mid.substr(close + std::string("）").size());
                }
                const auto p_mark = mid.find("，P = ");
                const auto d_mark = mid.find("，差值 = ");
                const bool en = language != "zh-CN" && language != "zh";
                if (p_mark != std::string::npos) {
                    std::size_t p_end = mid.size();
                    if (d_mark != std::string::npos && d_mark > p_mark) {
                        p_end = d_mark;
                    }
                    if (en) {
                        p_part = ", P = "
                            + mid.substr(p_mark + std::string("，P = ").size(),
                                         p_end - (p_mark + std::string("，P = ").size()));
                    } else {
                        p_part = mid.substr(p_mark, p_end - p_mark);
                    }
                }
                if (d_mark != std::string::npos) {
                    if (en) {
                        diff_part = ", difference = "
                            + mid.substr(d_mark + std::string("，差值 = ").size());
                    } else {
                        diff_part = mid.substr(d_mark);
                    }
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.ttest_two_sample", language, missing_out)
                        .text,
                    {{"%1", method}, {"%2", p_part}, {"%3", diff_part}});
            } else if ((starts_with(bullet, "配对 t 检验")
                        || starts_with(bullet, "单样本 t 检验"))
                       && ends_with(
                           bullet,
                           "。P 值与置信区间只描述当前样本证据，统计显著不等于工程差异具有实际重要性，也不是过程合格判定。")) {
                const std::string suffix =
                    "。P 值与置信区间只描述当前样本证据，统计显著不等于工程差异具有实际重要性，也不是过程合格判定。";
                const bool paired = starts_with(bullet, "配对 t 检验");
                const std::string prefix =
                    paired ? "配对 t 检验" : "单样本 t 检验";
                std::string mid = bullet.substr(
                    prefix.size(),
                    bullet.size() - prefix.size() - suffix.size());
                std::string p_part;
                std::string diff_part;
                const auto p_mark = mid.find("，P = ");
                const auto d_mark = mid.find("，差值 = ");
                const bool en = language != "zh-CN" && language != "zh";
                if (p_mark != std::string::npos) {
                    std::size_t p_end = mid.size();
                    if (d_mark != std::string::npos && d_mark > p_mark) {
                        p_end = d_mark;
                    }
                    if (en) {
                        p_part = ", P = "
                            + mid.substr(p_mark + std::string("，P = ").size(),
                                         p_end - (p_mark + std::string("，P = ").size()));
                    } else {
                        p_part = mid.substr(p_mark, p_end - p_mark);
                    }
                }
                if (d_mark != std::string::npos) {
                    if (en) {
                        diff_part = ", difference = "
                            + mid.substr(d_mark + std::string("，差值 = ").size());
                    } else {
                        diff_part = mid.substr(d_mark);
                    }
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        paired ? "interp.ttest_paired" : "interp.ttest_one_sample",
                        language, missing_out)
                        .text,
                    {{"%1", p_part}, {"%2", diff_part}});
            } else if (starts_with(
                           bullet,
                           "Z 检验要求已知总体标准差且正态/大样本近似成立（assumption_status=")
                       && ends_with(bullet, "）；样本 StDev 仅展示，不参与 Z/CI。")) {
                const std::string prefix =
                    "Z 检验要求已知总体标准差且正态/大样本近似成立（assumption_status=";
                const std::string suffix = "）；样本 StDev 仅展示，不参与 Z/CI。";
                const std::string status = bullet.substr(
                    prefix.size(),
                    bullet.size() - prefix.size() - suffix.size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.ttest_z_assumption", language, missing_out)
                        .text,
                    "%1",
                    status);
            } else if (starts_with(bullet, "t 检验正态与独立假设未验证（assumption_status=")
                       && ends_with(bullet, "）。")) {
                const std::string prefix =
                    "t 检验正态与独立假设未验证（assumption_status=";
                const std::string status = bullet.substr(
                    prefix.size(),
                    bullet.size() - prefix.size() - std::string("）。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.ttest_assumption", language, missing_out)
                        .text,
                    "%1",
                    status);
            } else if ((starts_with(bullet, "Anderson-Darling 正态性检验判定为 ")
                        || starts_with(bullet, "Ryan–Joiner 正态性检验判定为 "))
                       && (ends_with(
                               bullet,
                               "。在 alpha 下未拒绝正态假设，不能写成数据已正态。")
                           || ends_with(bullet, "。证据反对正态假设，这不是规格判定。")
                           || ends_with(
                               bullet, "。统计量未计算，不能写成数据已正态。"))) {
                const bool ryan = starts_with(bullet, "Ryan–Joiner ");
                const std::string method_name =
                    ryan ? "Ryan–Joiner" : "Anderson-Darling";
                const std::string prefix = method_name + " 正态性检验判定为 ";
                std::string id = "interp.normality_other";
                std::string suffix = "。统计量未计算，不能写成数据已正态。";
                if (ends_with(
                        bullet,
                        "。在 alpha 下未拒绝正态假设，不能写成数据已正态。")) {
                    id = "interp.normality_fail_to_reject";
                    suffix = "。在 alpha 下未拒绝正态假设，不能写成数据已正态。";
                } else if (ends_with(bullet, "。证据反对正态假设，这不是规格判定。")) {
                    id = "interp.normality_reject";
                    suffix = "。证据反对正态假设，这不是规格判定。";
                }
                std::string mid = bullet.substr(
                    prefix.size(),
                    bullet.size() - prefix.size() - suffix.size());
                std::string decision;
                std::string r_part;
                std::string p_part;
                const auto r_mark = mid.find("，R = ");
                const auto p_mark = mid.find("，P = ");
                const bool en = language != "zh-CN" && language != "zh";
                std::size_t decision_end = mid.size();
                if (r_mark != std::string::npos) {
                    decision_end = r_mark;
                } else if (p_mark != std::string::npos) {
                    decision_end = p_mark;
                }
                decision = mid.substr(0, decision_end);
                if (r_mark != std::string::npos) {
                    std::size_t r_end = mid.size();
                    if (p_mark != std::string::npos && p_mark > r_mark) {
                        r_end = p_mark;
                    }
                    if (en) {
                        r_part = ", R = "
                            + mid.substr(r_mark + std::string("，R = ").size(),
                                         r_end - (r_mark + std::string("，R = ").size()));
                    } else {
                        r_part = mid.substr(r_mark, r_end - r_mark);
                    }
                }
                if (p_mark != std::string::npos) {
                    if (en) {
                        p_part = ", P = "
                            + mid.substr(p_mark + std::string("，P = ").size());
                    } else {
                        p_part = mid.substr(p_mark);
                    }
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(id, language, missing_out).text,
                    {{"%1", method_name},
                     {"%2", decision},
                     {"%3", r_part},
                     {"%4", p_part}});
            } else if (starts_with(bullet, "正态假设状态为 ")
                       && ends_with(bullet, "。")) {
                const std::string status = bullet.substr(
                    std::string("正态假设状态为 ").size(),
                    bullet.size() - std::string("正态假设状态为 ").size()
                        - std::string("。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.normality_assumption_status", language, missing_out)
                        .text,
                    "%1",
                    status);
            } else if (starts_with(bullet, "%Defective = ")
                       && bullet.find("（Average P = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "）。这是当前样本的不合格品率估计，不是过程合格判定。")) {
                const auto mid = bullet.find("（Average P = ");
                const std::string defective = bullet.substr(
                    std::string("%Defective = ").size(),
                    mid - std::string("%Defective = ").size());
                const std::string average_p = bullet.substr(
                    mid + std::string("（Average P = ").size(),
                    bullet.size()
                        - (mid + std::string("（Average P = ").size())
                        - std::string(
                              "）。这是当前样本的不合格品率估计，不是过程合格判定。")
                              .size());
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.attr_percent_defective", language, missing_out)
                        .text,
                    {{"%1", defective}, {"%2", average_p}});
            } else if (starts_with(bullet, "Process Z = ")
                       && ends_with(bullet, "，由 Average P 的标准正态右尾得到。")) {
                const std::string value = bullet.substr(
                    std::string("Process Z = ").size(),
                    bullet.size() - std::string("Process Z = ").size()
                        - std::string("，由 Average P 的标准正态右尾得到。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.attr_process_z", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "Mean DPU = ")
                       && ends_with(
                           bullet,
                           "。这是当前样本的单位缺陷率估计，不是过程合格判定。")) {
                const std::string value = bullet.substr(
                    std::string("Mean DPU = ").size(),
                    bullet.size() - std::string("Mean DPU = ").size()
                        - std::string(
                              "。这是当前样本的单位缺陷率估计，不是过程合格判定。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.attr_mean_dpu", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(
                           bullet,
                           "二项过程能力未验证独立性、恒定 p 与稳定性（assumption_status=")
                       && ends_with(bullet, "），不能写成过程合格。")) {
                const std::string prefix =
                    "二项过程能力未验证独立性、恒定 p 与稳定性（assumption_status=";
                const std::string status = bullet.substr(
                    prefix.size(),
                    bullet.size() - prefix.size()
                        - std::string("），不能写成过程合格。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.attr_binomial_assumption", language, missing_out)
                        .text,
                    "%1",
                    status);
            } else if (starts_with(
                           bullet,
                           "泊松过程能力未验证独立性、恒定 DPU 与稳定性（assumption_status=")
                       && ends_with(bullet, "），不能写成过程合格。")) {
                const std::string prefix =
                    "泊松过程能力未验证独立性、恒定 DPU 与稳定性（assumption_status=";
                const std::string status = bullet.substr(
                    prefix.size(),
                    bullet.size() - prefix.size()
                        - std::string("），不能写成过程合格。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.attr_poisson_assumption", language, missing_out)
                        .text,
                    "%1",
                    status);
            } else if (starts_with(bullet, "两比例检验方法为 ")
                       && ends_with(
                           bullet,
                           "。P 值只描述两组比例差异的证据强度，不是规格判定。")) {
                const std::string body = bullet.substr(
                    std::string("两比例检验方法为 ").size(),
                    bullet.size() - std::string("两比例检验方法为 ").size()
                        - std::string(
                              "。P 值只描述两组比例差异的证据强度，不是规格判定。")
                              .size());
                const auto p1m = body.find("，p̂1 = ");
                const auto p2m = body.find("，p̂2 = ");
                if (p1m != std::string::npos && p2m != std::string::npos && p2m > p1m) {
                    const std::string method = body.substr(0, p1m);
                    const std::string p1 = body.substr(
                        p1m + std::string("，p̂1 = ").size(),
                        p2m - (p1m + std::string("，p̂1 = ").size()));
                    std::string rest = body.substr(p2m + std::string("，p̂2 = ").size());
                    std::string p2;
                    std::string diff_part;
                    std::string wald_part;
                    std::string fisher_part;
                    const bool en = language != "zh-CN" && language != "zh";
                    const auto dm = rest.find("，差值 = ");
                    const auto wm = rest.find("，Wald P = ");
                    const auto fm = rest.find("，Fisher P = ");
                    std::size_t p2_end = rest.size();
                    if (dm != std::string::npos) {
                        p2_end = dm;
                    } else if (wm != std::string::npos) {
                        p2_end = wm;
                    } else if (fm != std::string::npos) {
                        p2_end = fm;
                    }
                    p2 = rest.substr(0, p2_end);
                    if (dm != std::string::npos) {
                        std::size_t end = rest.size();
                        if (wm != std::string::npos && wm > dm) {
                            end = wm;
                        } else if (fm != std::string::npos && fm > dm) {
                            end = fm;
                        }
                        const std::string val = rest.substr(
                            dm + std::string("，差值 = ").size(),
                            end - (dm + std::string("，差值 = ").size()));
                        diff_part = en ? (", difference = " + val) : ("，差值 = " + val);
                    }
                    if (wm != std::string::npos) {
                        std::size_t end = rest.size();
                        if (fm != std::string::npos && fm > wm) {
                            end = fm;
                        }
                        const std::string val = rest.substr(
                            wm + std::string("，Wald P = ").size(),
                            end - (wm + std::string("，Wald P = ").size()));
                        wald_part = en ? (", Wald P = " + val) : ("，Wald P = " + val);
                    }
                    if (fm != std::string::npos) {
                        const std::string val =
                            rest.substr(fm + std::string("，Fisher P = ").size());
                        fisher_part =
                            en ? (", Fisher P = " + val) : ("，Fisher P = " + val);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.prop_two", language, missing_out)
                            .text,
                        {{"%1", method},
                         {"%2", p1},
                         {"%3", p2},
                         {"%4", diff_part},
                         {"%5", wald_part},
                         {"%6", fisher_part}});
                }
            } else if (starts_with(bullet, "单比例检验方法为 ")
                       && ends_with(
                           bullet,
                           "。P 值只描述与假设比例的证据强度，不是规格判定。")) {
                const std::string body = bullet.substr(
                    std::string("单比例检验方法为 ").size(),
                    bullet.size() - std::string("单比例检验方法为 ").size()
                        - std::string(
                              "。P 值只描述与假设比例的证据强度，不是规格判定。")
                              .size());
                const auto pm = body.find("，p̂ = ");
                if (pm != std::string::npos) {
                    std::string method = body.substr(0, pm);
                    std::string ci_part;
                    const auto ci_open = method.find("（CI=");
                    const bool en = language != "zh-CN" && language != "zh";
                    if (ci_open != std::string::npos && ends_with(method, "）")) {
                        const std::string ci = method.substr(
                            ci_open + std::string("（CI=").size(),
                            method.size() - (ci_open + std::string("（CI=").size())
                                - std::string("）").size());
                        method = method.substr(0, ci_open);
                        ci_part = en ? (" (CI=" + ci + ")") : ("（CI=" + ci + "）");
                    }
                    std::string rest = body.substr(pm + std::string("，p̂ = ").size());
                    std::string p_hat;
                    std::string h0_part;
                    std::string p_part;
                    const auto h0 = rest.find("，H0: p = ");
                    const auto pv = rest.find("，P = ");
                    std::size_t p_end = rest.size();
                    if (h0 != std::string::npos) {
                        p_end = h0;
                    } else if (pv != std::string::npos) {
                        p_end = pv;
                    }
                    p_hat = rest.substr(0, p_end);
                    if (h0 != std::string::npos) {
                        std::size_t end = rest.size();
                        if (pv != std::string::npos && pv > h0) {
                            end = pv;
                        }
                        const std::string val = rest.substr(
                            h0 + std::string("，H0: p = ").size(),
                            end - (h0 + std::string("，H0: p = ").size()));
                        h0_part = en ? (", H0: p = " + val) : ("，H0: p = " + val);
                    }
                    if (pv != std::string::npos) {
                        const std::string val =
                            rest.substr(pv + std::string("，P = ").size());
                        p_part = en ? (", P = " + val) : ("，P = " + val);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.prop_one", language, missing_out)
                            .text,
                        {{"%1", method},
                         {"%2", ci_part},
                         {"%3", p_hat},
                         {"%4", h0_part},
                         {"%5", p_part}});
                }
            } else if (starts_with(
                           bullet, "比例假设未验证独立性与恒定 p（assumption_status=")
                       && ends_with(bullet, "）。")) {
                const std::string prefix =
                    "比例假设未验证独立性与恒定 p（assumption_status=";
                const std::string status = bullet.substr(
                    prefix.size(),
                    bullet.size() - prefix.size() - std::string("）。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.prop_assumption", language, missing_out)
                        .text,
                    "%1",
                    status);
            } else if (starts_with(bullet, "双样本泊松率方法为 ")
                       && ends_with(
                           bullet,
                           "。P 值只描述与假设发生率的证据强度，不是规格判定。")) {
                const std::string body = bullet.substr(
                    std::string("双样本泊松率方法为 ").size(),
                    bullet.size() - std::string("双样本泊松率方法为 ").size()
                        - std::string(
                              "。P 值只描述与假设发生率的证据强度，不是规格判定。")
                              .size());
                const bool en = language != "zh-CN" && language != "zh";
                std::string method = body;
                std::string mid;
                std::string p_part;
                const auto pmark = method.rfind("，P = ");
                if (pmark != std::string::npos) {
                    p_part = en
                        ? (", P = " + method.substr(pmark + std::string("，P = ").size()))
                        : method.substr(pmark);
                    method = method.substr(0, pmark);
                }
                const auto ratio = method.find("，比较量=率比 ρ=λ1/λ2，ρ̂ = ");
                const auto l1 = method.find("，λ̂1 = ");
                if (ratio != std::string::npos) {
                    mid = en
                        ? (", comparison=rate ratio ρ=λ1/λ2, ρ̂ = "
                           + method.substr(
                               ratio + std::string("，比较量=率比 ρ=λ1/λ2，ρ̂ = ").size()))
                        : method.substr(ratio);
                    method = method.substr(0, ratio);
                } else if (l1 != std::string::npos) {
                    std::string rest = method.substr(l1);
                    if (en) {
                        rest.replace(0, std::string("，λ̂1 = ").size(), ", λ̂1 = ");
                        const auto l2 = rest.find("，λ̂2 = ");
                        if (l2 != std::string::npos) {
                            rest.replace(l2, std::string("，λ̂2 = ").size(), ", λ̂2 = ");
                        }
                    }
                    mid = rest;
                    method = method.substr(0, l1);
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.poisson_two", language, missing_out)
                        .text,
                    {{"%1", method}, {"%2", mid}, {"%3", p_part}});
            } else if (starts_with(bullet, "单样本泊松率方法为 ")
                       && ends_with(
                           bullet,
                           "。P 值只描述与假设发生率的证据强度，不是规格判定。")) {
                const std::string body = bullet.substr(
                    std::string("单样本泊松率方法为 ").size(),
                    bullet.size() - std::string("单样本泊松率方法为 ").size()
                        - std::string(
                              "。P 值只描述与假设发生率的证据强度，不是规格判定。")
                              .size());
                const auto lm = body.find("，λ̂ = ");
                if (lm != std::string::npos) {
                    const std::string method = body.substr(0, lm);
                    std::string rest = body.substr(lm + std::string("，λ̂ = ").size());
                    const bool en = language != "zh-CN" && language != "zh";
                    std::string rate;
                    std::string h0_part;
                    std::string p_part;
                    const auto h0 = rest.find("，H0: λ = ");
                    const auto pv = rest.find("，P = ");
                    std::size_t end = rest.size();
                    if (h0 != std::string::npos) {
                        end = h0;
                    } else if (pv != std::string::npos) {
                        end = pv;
                    }
                    rate = rest.substr(0, end);
                    if (h0 != std::string::npos) {
                        std::size_t h_end = rest.size();
                        if (pv != std::string::npos && pv > h0) {
                            h_end = pv;
                        }
                        const std::string val = rest.substr(
                            h0 + std::string("，H0: λ = ").size(),
                            h_end - (h0 + std::string("，H0: λ = ").size()));
                        h0_part = en ? (", H0: λ = " + val) : ("，H0: λ = " + val);
                    }
                    if (pv != std::string::npos) {
                        const std::string val =
                            rest.substr(pv + std::string("，P = ").size());
                        p_part = en ? (", P = " + val) : ("，P = " + val);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.poisson_one", language, missing_out)
                            .text,
                        {{"%1", method},
                         {"%2", rate},
                         {"%3", h0_part},
                         {"%4", p_part}});
                }
            } else if (starts_with(
                           bullet,
                           "泊松率假设未验证独立同质发生率（assumption_status=")
                       && ends_with(bullet, "）。")) {
                const std::string prefix =
                    "泊松率假设未验证独立同质发生率（assumption_status=";
                const std::string status = bullet.substr(
                    prefix.size(),
                    bullet.size() - prefix.size() - std::string("）。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.poisson_rate_assumption", language, missing_out)
                        .text,
                    "%1",
                    status);
            } else if (starts_with(bullet, "Box-Cox 选定 λ = ")
                       && ends_with(bullet, "。概率图只是诊断，不能写成数据已正态。")) {
                const std::string body = bullet.substr(
                    std::string("Box-Cox 选定 λ = ").size(),
                    bullet.size() - std::string("Box-Cox 选定 λ = ").size()
                        - std::string("。概率图只是诊断，不能写成数据已正态。").size());
                const auto nm = body.find("，有效观测 N = ");
                if (nm != std::string::npos) {
                    const std::string lambda = body.substr(0, nm);
                    std::string rest = body.substr(
                        nm + std::string("，有效观测 N = ").size());
                    std::string n;
                    std::string sd_part;
                    const bool en = language != "zh-CN" && language != "zh";
                    const auto sd = rest.find("，变换后标准化 SD = ");
                    if (sd == std::string::npos) {
                        n = rest;
                    } else {
                        n = rest.substr(0, sd);
                        const std::string val = rest.substr(
                            sd + std::string("，变换后标准化 SD = ").size());
                        sd_part = en
                            ? (", transformed standardized SD = " + val)
                            : ("，变换后标准化 SD = " + val);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.boxcox_summary", language, missing_out)
                            .text,
                        {{"%1", lambda}, {"%2", n}, {"%3", sd_part}});
                }
            } else if (starts_with(
                           bullet,
                           "变换后能力指数（若出现）不是过程合格判定（assumption_status=")
                       && ends_with(bullet, "）。")) {
                const std::string prefix =
                    "变换后能力指数（若出现）不是过程合格判定（assumption_status=";
                const std::string status = bullet.substr(
                    prefix.size(),
                    bullet.size() - prefix.size() - std::string("）。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.boxcox_assumption", language, missing_out)
                        .text,
                    "%1",
                    status);

            } else if (
                bullet
                == "规格限无法变换或顺序无效，已跳过变换后过程能力表（非合格判定）。") {
                bullet = domain::resolve_report_text(
                    "interp.box_cox_spec_limit_gate", language, missing_out)
                             .text;

            } else if (
                bullet
                == "规格限落在 Johnson 变换定义域外，已跳过 overall 能力指数表（非合格判定）。") {
                bullet = domain::resolve_report_text(
                    "interp.johnson_spec_limit_gate", language, missing_out)
                             .text;

            } else if (
                bullet
                == "暴露量列无效或求和为零，已跳过保修摘要指标（非法律/质量承诺）。") {
                bullet = domain::resolve_report_text(
                    "interp.warranty_exposure_gate", language, missing_out)
                             .text;

            } else if (starts_with(bullet, "预测误差指标已计算；")) {
                const std::string rest = bullet.substr(
                    std::string("预测误差指标已计算；").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.forecast_mape", language, missing_out)
                        .text,
                    "%1",
                    rest);
            } else if (starts_with(bullet, "R² = ")
                       && ends_with(
                           bullet, " 只描述当前样本拟合程度，不能单独判定模型合格。")) {
                const std::string value = bullet.substr(
                    std::string("R² = ").size(),
                    bullet.size() - std::string("R² = ").size()
                        - std::string(
                              " 只描述当前样本拟合程度，不能单独判定模型合格。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.regression_r2", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "最大 VIF = ")
                       && ends_with(
                           bullet, "，提示共线性调查，不会自动删除预测变量。")) {
                const std::string value = bullet.substr(
                    std::string("最大 VIF = ").size(),
                    bullet.size() - std::string("最大 VIF = ").size()
                        - std::string("，提示共线性调查，不会自动删除预测变量。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.regression_vif_advice", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "当前有统计证据的项：")
                       && ends_with(bullet, "。")) {
                const std::string terms = bullet.substr(
                    std::string("当前有统计证据的项：").size(),
                    bullet.size() - std::string("当前有统计证据的项：").size()
                        - std::string("。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.anova_significant_terms", language, missing_out)
                        .text,
                    "%1",
                    terms);
            } else if (starts_with(bullet, "Tukey 同时置信水平 = ")
                       && ends_with(
                           bullet,
                           "；显著性由同时置信区间是否包含 0 决定。")) {
                const std::string level = bullet.substr(
                    std::string("Tukey 同时置信水平 = ").size(),
                    bullet.size()
                        - std::string("Tukey 同时置信水平 = ").size()
                        - std::string(
                              "；显著性由同时置信区间是否包含 0 决定。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "rule.anova.catalog.family_error_rate.computed",
                        language,
                        missing_out)
                        .text,
                    "%1",
                    level);
            } else if (starts_with(bullet, "Tukey 同时置信水平 = ")
                       && bullet.find("，显著比较对数 = ") != std::string::npos
                       && ends_with(
                           bullet,
                           "；区间含 0 不显著。同字母仅表示在本产品 Tukey 近似规则下未显著不同，"
                           "不能写成已证明相同。")) {
                const auto mid = bullet.find("，显著比较对数 = ");
                const std::string level = bullet.substr(
                    std::string("Tukey 同时置信水平 = ").size(),
                    mid - std::string("Tukey 同时置信水平 = ").size());
                const std::string pairs = bullet.substr(
                    mid + std::string("，显著比较对数 = ").size(),
                    bullet.size()
                        - (mid + std::string("，显著比较对数 = ").size())
                        - std::string(
                              "；区间含 0 不显著。同字母仅表示在本产品 Tukey 近似规则下未显著不同，"
                              "不能写成已证明相同。")
                              .size());
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.anova_tukey", language, missing_out)
                        .text,
                    {{"%1", level}, {"%2", pairs}});
            } else if (starts_with(bullet, "在效应量 = ")
                       && bullet.find("、α = ") != std::string::npos
                       && bullet.find(" 下，估计实际功效 = ") != std::string::npos
                       && ends_with(bullet, "。")
                       && bullet.find("数值是假设条件下的计算值") == std::string::npos) {
                const std::string body = bullet.substr(
                    std::string("在效应量 = ").size(),
                    bullet.size() - std::string("在效应量 = ").size()
                        - std::string("。").size());
                const auto am = body.find("、α = ");
                const auto pm = body.find(" 下，估计实际功效 = ");
                if (am != std::string::npos && pm != std::string::npos && pm > am) {
                    const std::string effect = body.substr(0, am);
                    const std::string alpha = body.substr(
                        am + std::string("、α = ").size(),
                        pm - (am + std::string("、α = ").size()));
                    const std::string power = body.substr(
                        pm + std::string(" 下，估计实际功效 = ").size());
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.power_ttest", language, missing_out)
                            .text,
                        {{"%1", effect}, {"%2", alpha}, {"%3", power}});
                }
            } else if (starts_with(bullet, "在效应量 = ")
                       && ends_with(bullet, "。数值是假设条件下的计算值。")) {
                const std::string body = bullet.substr(
                    std::string("在效应量 = ").size(),
                    bullet.size() - std::string("在效应量 = ").size()
                        - std::string("。数值是假设条件下的计算值。").size());
                const auto pm = body.find(" 下，估计实际功效 = ");
                if (pm != std::string::npos) {
                    std::string left = body.substr(0, pm);
                    std::string right = body.substr(
                        pm + std::string(" 下，估计实际功效 = ").size());
                    const bool en = language != "zh-CN" && language != "zh";
                    std::string effect;
                    std::string target_part;
                    const auto tm = left.find("、目标功效 = ");
                    if (tm == std::string::npos) {
                        effect = left;
                    } else {
                        effect = left.substr(0, tm);
                        const std::string tval = left.substr(
                            tm + std::string("、目标功效 = ").size());
                        target_part = en ? (", target power = " + tval)
                                         : ("、目标功效 = " + tval);
                    }
                    std::string power;
                    std::string n_part;
                    const auto nm = right.find("，对应样本量 n = ");
                    if (nm == std::string::npos) {
                        power = right;
                    } else {
                        power = right.substr(0, nm);
                        const std::string nval = right.substr(
                            nm + std::string("，对应样本量 n = ").size());
                        n_part = en ? (", corresponding sample size n = " + nval)
                                    : ("，对应样本量 n = " + nval);
                    }
                    bullet = replace_all_tokens(
                        domain::resolve_report_text(
                            "interp.power_generic", language, missing_out)
                            .text,
                        {{"%1", effect},
                         {"%2", target_part},
                         {"%3", power},
                         {"%4", n_part}});
                }
            } else if (starts_with(bullet, "高斯混合为 preferred_kcomp（k=")
                       && ends_with(
                           bullet,
                           "）：单一分布能力指数仅供调查，不得写成过程合格（formula_reference / "
                           "gaussian_mixture_k_bic；非 vendor_oracle）。")) {
                const std::string prefix = "高斯混合为 preferred_kcomp（k=";
                const std::string suffix =
                    "）：单一分布能力指数仅供调查，不得写成过程合格（formula_reference / "
                    "gaussian_mixture_k_bic；非 vendor_oracle）。";
                const std::string k = bullet.substr(
                    prefix.size(), bullet.size() - prefix.size() - suffix.size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.mixture_preferred_kcomp", language, missing_out)
                        .text,
                    "%1",
                    k);
            } else if ((starts_with(bullet, "Dixon r10 异常值检验")
                        || starts_with(bullet, "Grubbs 异常值检验"))
                       && (ends_with(
                               bullet,
                               "。在 α 下拒绝“无异常值”假设，嫌疑观测需工程调查，"
                               "P 值只描述与正态假设下的一致性，勿当作删点依据，也勿当成异常已核实。")
                           || ends_with(
                               bullet,
                               "。未拒绝“无异常值”假设，不能证明数据中没有异常值。"))) {
                const bool dixon = starts_with(bullet, "Dixon r10 异常值检验");
                const std::string method =
                    dixon ? "Dixon r10 异常值检验" : "Grubbs 异常值检验";
                const std::string en_method =
                    dixon ? "Dixon r10 outlier test" : "Grubbs outlier test";
                std::string id = "interp.outlier_not_reject";
                std::string suffix =
                    "。未拒绝“无异常值”假设，不能证明数据中没有异常值。";
                if (ends_with(
                        bullet,
                        "。在 α 下拒绝“无异常值”假设，嫌疑观测需工程调查，"
                        "P 值只描述与正态假设下的一致性，勿当作删点依据，也勿当成异常已核实。")) {
                    id = "interp.outlier_reject";
                    suffix =
                        "。在 α 下拒绝“无异常值”假设，嫌疑观测需工程调查，"
                        "P 值只描述与正态假设下的一致性，勿当作删点依据，也勿当成异常已核实。";
                }
                std::string mid = bullet.substr(
                    method.size(),
                    bullet.size() - method.size() - suffix.size());
                std::string stat_part;
                std::string p_part;
                const bool en = language != "zh-CN" && language != "zh";
                const auto p_mark = mid.find("，P = ");
                if (p_mark != std::string::npos) {
                    if (en) {
                        p_part = ", P = "
                            + mid.substr(p_mark + std::string("，P = ").size());
                    } else {
                        p_part = mid.substr(p_mark);
                    }
                    mid = mid.substr(0, p_mark);
                }
                if (!mid.empty()) {
                    if (en) {
                        if (mid.find(" r = ") == 0) {
                            stat_part = ", r = " + mid.substr(std::string(" r = ").size());
                        } else if (mid.find(" G = ") == 0) {
                            stat_part = ", G = " + mid.substr(std::string(" G = ").size());
                        } else {
                            stat_part = mid;
                        }
                    } else {
                        stat_part = mid;
                    }
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(id, language, missing_out).text,
                    {{"%1", en ? en_method : method},
                     {"%2", stat_part},
                     {"%3", p_part}});
            } else if (starts_with(
                           bullet,
                           "Dixon r10 要求近似正态、至多一个异常值，且 P 可能为临界值插值近似（assumption_status=")
                       && ends_with(bullet, "）。")) {
                const std::string prefix =
                    "Dixon r10 要求近似正态、至多一个异常值，且 P 可能为临界值插值近似（assumption_status=";
                const std::string status = bullet.substr(
                    prefix.size(),
                    bullet.size() - prefix.size() - std::string("）。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.outlier_dixon_assumption", language, missing_out)
                        .text,
                    "%1",
                    status);
            } else if (starts_with(
                           bullet,
                           "Grubbs 要求近似正态且至多一个异常值（assumption_status=")
                       && ends_with(bullet, "）。")) {
                const std::string prefix =
                    "Grubbs 要求近似正态且至多一个异常值（assumption_status=";
                const std::string status = bullet.substr(
                    prefix.size(),
                    bullet.size() - prefix.size() - std::string("）。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.outlier_grubbs_assumption", language, missing_out)
                        .text,
                    "%1",
                    status);
            } else if (starts_with(bullet, "相关分析使用 complete-case 对齐（N = ")
                       && bullet.find("），assumption_status=") != std::string::npos
                       && ends_with(bullet, "。")) {
                const auto mid = bullet.find("），assumption_status=");
                const std::string n = bullet.substr(
                    std::string("相关分析使用 complete-case 对齐（N = ").size(),
                    mid - std::string("相关分析使用 complete-case 对齐（N = ").size());
                const std::string status = bullet.substr(
                    mid + std::string("），assumption_status=").size(),
                    bullet.size()
                        - (mid + std::string("），assumption_status=").size())
                        - std::string("。").size());
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.corr_complete_case", language, missing_out)
                        .text,
                    {{"%1", n}, {"%2", status}});
            } else if (starts_with(bullet, "二项 OC 曲线描述在 n=")
                       && bullet.find("、c=") != std::string::npos
                       && ends_with(
                           bullet,
                           " 计划下，不同不合格品率 p 的接收概率 Pa(p)；"
                           "OC 用于评估抽样方案风险，不能替代对具体批次的接收判定。")) {
                const auto cmark = bullet.find("、c=");
                const std::string n = bullet.substr(
                    std::string("二项 OC 曲线描述在 n=").size(),
                    cmark - std::string("二项 OC 曲线描述在 n=").size());
                const std::string c = bullet.substr(
                    cmark + std::string("、c=").size(),
                    bullet.size() - (cmark + std::string("、c=").size())
                        - std::string(
                              " 计划下，不同不合格品率 p 的接收概率 Pa(p)；"
                              "OC 用于评估抽样方案风险，不能替代对具体批次的接收判定。")
                              .size());
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.oc_binomial", language, missing_out)
                        .text,
                    {{"%1", n}, {"%2", c}});
            } else if (starts_with(bullet, "Pa(AQL) = ")
                       && ends_with(bullet, " 只反映 AQL 风险点，不是生产判定结论。")) {
                const std::string value = bullet.substr(
                    std::string("Pa(AQL) = ").size(),
                    bullet.size() - std::string("Pa(AQL) = ").size()
                        - std::string(" 只反映 AQL 风险点，不是生产判定结论。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.oc_pa_aql", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "Pa(RQL) = ")
                       && ends_with(bullet, " 只反映 RQL 风险点，不是拒收证明。")) {
                const std::string value = bullet.substr(
                    std::string("Pa(RQL) = ").size(),
                    bullet.size() - std::string("Pa(RQL) = ").size()
                        - std::string(" 只反映 RQL 风险点，不是拒收证明。").size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.oc_pa_rql", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "批大小 N = ")
                       && ends_with(
                           bullet, " 仅作摘要；本轮 OC 仍用二项（无限批）近似。")) {
                const std::string value = bullet.substr(
                    std::string("批大小 N = ").size(),
                    bullet.size() - std::string("批大小 N = ").size()
                        - std::string(" 仅作摘要；本轮 OC 仍用二项（无限批）近似。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.oc_lot_size", language, missing_out)
                        .text,
                    "%1",
                    value);
            } else if (starts_with(bullet, "ANOM 在 α=")
                       && bullet.find(" 下比较各组均值与总体均值；超出 UDL/LDL 的组数 = ")
                           != std::string::npos
                       && ends_with(
                           bullet,
                           "。这标记值得进一步调查的组，不能写成组间无差异或应剔除该组。")) {
                const auto mid = bullet.find(
                    " 下比较各组均值与总体均值；超出 UDL/LDL 的组数 = ");
                const std::string alpha = bullet.substr(
                    std::string("ANOM 在 α=").size(),
                    mid - std::string("ANOM 在 α=").size());
                const std::string outside = bullet.substr(
                    mid
                        + std::string(
                              " 下比较各组均值与总体均值；超出 UDL/LDL 的组数 = ")
                              .size(),
                    bullet.size()
                        - (mid
                           + std::string(
                                 " 下比较各组均值与总体均值；超出 UDL/LDL 的组数 = ")
                                 .size())
                        - std::string(
                              "。这标记值得进一步调查的组，不能写成组间无差异或应剔除该组。")
                              .size());
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.anom_summary", language, missing_out)
                        .text,
                    {{"%1", alpha}, {"%2", outside}});
            } else if (starts_with(bullet, "决策限方法 = ")
                       && ends_with(bullet, "（正态近似）；二项/泊松计数不适用本命令。")) {
                const std::string method = bullet.substr(
                    std::string("决策限方法 = ").size(),
                    bullet.size() - std::string("决策限方法 = ").size()
                        - std::string("（正态近似）；二项/泊松计数不适用本命令。")
                              .size());
                bullet = replace_token(
                    domain::resolve_report_text(
                        "interp.anom_decision_limits", language, missing_out)
                        .text,
                    "%1",
                    method);
            } else if (starts_with(bullet, "TOST 依赖")
                       && bullet.find("假设（assumption_status=") != std::string::npos
                       && ends_with(bullet, "）。")) {
                const auto mid = bullet.find("假设（assumption_status=");
                const std::string assumption = bullet.substr(
                    std::string("TOST 依赖").size(),
                    mid - std::string("TOST 依赖").size());
                const std::string status = bullet.substr(
                    mid + std::string("假设（assumption_status=").size(),
                    bullet.size()
                        - (mid + std::string("假设（assumption_status=").size())
                        - std::string("）。").size());
                std::string assumption_en = assumption;
                if (language != "zh-CN" && language != "zh") {
                    static const std::pair<const char*, const char*> assum_map[] = {
                        {"正态与独立样本", "normality and independent samples"},
                        {"配对差值近似正态，且每对观测来自同一对象/批次的匹配测量",
                         "approximately normal paired differences from matched pairs"},
                        {"独立二项试验与大样本正态近似（Wald z-TOST）",
                         "independent binomial trials and large-sample normal approximation (Wald z-TOST)"},
                        {"独立对数正态样本与全正观测（均值比 TOST，对数变换 / 几何均值比）",
                         "independent lognormal samples with all-positive observations (mean-ratio TOST, log / geometric mean ratio)"},
                        {"独立正态样本与正参考均值（均值比 TOST，非对数）",
                         "independent normal samples with positive reference mean (mean-ratio TOST, non-log)"},
                    };
                    for (const auto& entry : assum_map) {
                        if (assumption == entry.first) {
                            assumption_en = entry.second;
                            break;
                        }
                    }
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(
                        "interp.tost_assumption", language, missing_out)
                        .text,
                    {{"%1", assumption_en}, {"%2", status}});
            } else if (starts_with(bullet, "等价性检验比较")
                       && ends_with(bullet, "这只陈述界限关系，不能写成已证明等价。")) {
                const bool within = bullet.find("，区间落在等价界限内。") != std::string::npos;
                const std::string id =
                    within ? "interp.equiv_within" : "interp.equiv_outside";
                const std::string after = bullet.substr(std::string("等价性检验比较").size());
                std::string quantity = "差值";
                std::string rest = after;
                if (starts_with(after, "比例差")) {
                    quantity = "比例差";
                    rest = after.substr(std::string("比例差").size());
                } else if (starts_with(after, "均值比")) {
                    quantity = "均值比";
                    rest = after.substr(std::string("均值比").size());
                } else if (starts_with(after, "差值")) {
                    quantity = "差值";
                    rest = after.substr(std::string("差值").size());
                }
                if (language != "zh-CN" && language != "zh") {
                    if (quantity == "比例差") {
                        quantity = domain::resolve_report_text(
                                       "interp.equiv_quantity_prop", language, missing_out)
                                       .text;
                    } else if (quantity == "均值比") {
                        quantity = domain::resolve_report_text(
                                       "interp.equiv_quantity_ratio", language, missing_out)
                                       .text;
                    } else {
                        quantity = domain::resolve_report_text(
                                       "interp.equiv_quantity_diff", language, missing_out)
                                       .text;
                    }
                }
                const auto lb = rest.find("与界限 [");
                const auto comma = rest.find(", ");
                const auto rb = rest.find("]");
                std::string lower = "*";
                std::string upper = "*";
                std::string ci_part;
                std::string p_part;
                if (lb != std::string::npos && comma != std::string::npos
                    && rb != std::string::npos && comma > lb && rb > comma) {
                    lower = rest.substr(
                        lb + std::string("与界限 [").size(),
                        comma - (lb + std::string("与界限 [").size()));
                    upper = rest.substr(comma + 2, rb - (comma + 2));
                    std::string tail = rest.substr(rb + 1);
                    const auto ci_mark = tail.find("；CI(");
                    if (ci_mark != std::string::npos) {
                        const auto ci_end = tail.find("]");
                        if (ci_end != std::string::npos) {
                            const std::string ci_chunk = tail.substr(
                                ci_mark, ci_end - ci_mark + 1);
                            if (language != "zh-CN" && language != "zh") {
                                // keep method/numbers; translate leading ； and 为
                                ci_part = "; CI" + ci_chunk.substr(std::string("；CI").size());
                                const auto wei = ci_part.find(" 为 ");
                                if (wei != std::string::npos) {
                                    ci_part.replace(wei, std::string(" 为 ").size(), " = ");
                                }
                            } else {
                                ci_part = ci_chunk;
                            }
                            tail = tail.substr(ci_end + 1);
                        }
                    }
                    const auto p_mark = tail.find(" 双单侧 P 值分别为 ");
                    if (p_mark != std::string::npos) {
                        const auto with_mark = tail.find(" 与 ");
                        const auto end_mark = tail.find("。");
                        if (with_mark != std::string::npos && end_mark != std::string::npos) {
                            const std::string p1 = tail.substr(
                                p_mark + std::string(" 双单侧 P 值分别为 ").size(),
                                with_mark
                                    - (p_mark + std::string(" 双单侧 P 值分别为 ").size()));
                            const std::string p2 = tail.substr(
                                with_mark + std::string(" 与 ").size(),
                                end_mark - (with_mark + std::string(" 与 ").size()));
                            if (language != "zh-CN" && language != "zh") {
                                p_part = " Two one-sided P-values are " + p1 + " and " + p2
                                    + ". ";
                            } else {
                                p_part = " 双单侧 P 值分别为 " + p1 + " 与 " + p2 + "。";
                            }
                        }
                    }
                }
                bullet = replace_all_tokens(
                    domain::resolve_report_text(id, language, missing_out).text,
                    {{"%1", quantity},
                     {"%2", lower},
                     {"%3", upper},
                     {"%4", ci_part},
                     {"%5", p_part}});
            } else {
                static const std::pair<const char*, const char*> exact_ids[] = {
                    {"无重复编码点：未报告纯误差/失拟；不得用残差 MS 冒充纯误差。",
                     "interp.rsm_no_replicates_lof"},
                    {"Pearson 相关系数表示变量关联方向与强度；P-Value 反映在零相关假设下的证据强度，不能单独证明因果关系。未拒绝零相关不能写成已证明无关。",
                     "interp.corr_pearson"},
                    {"Spearman 相关系数表示变量关联方向与强度；P-Value 反映在零相关假设下的证据强度，不能单独证明因果关系。未拒绝零相关不能写成已证明无关。",
                     "interp.corr_spearman"},
                    {"协方差矩阵基于 complete-case 样本协方差；对角线为各变量方差。",
                     "interp.corr_covariance"},
                    {"偏相关在控制其余变量后给出 Pearson 偏系数；不能写成已排除混杂或已证明因果。",
                     "interp.corr_partial"},
                    {"相关系数表示变量关联方向与强度；P-Value 反映在零相关假设下观察到当前系数的证据强度，不能单独证明因果关系。",
                     "interp.corr_generic"},
                    {"ANOVA 的总体 F 检验显著时，只能说明至少存在一组均值差异；需要后续多重比较确定具体组对。",
                     "interp.anova_f_followup"},
                    {"请结合 P-Value 与均值差置信区间判断差异；统计显著不等于工程差异具有实际重要性。",
                     "interp.ttest_generic_advice"},
                    {"响应优化在编码 ±1 空间枚举候选组合；缺协方差时置信/预测区间不可用，最佳组合不能外推到设计空间之外。",
                     "interp.response_opt_coded"},
                    {"响应优化在编码 ±1 空间枚举候选组合；结果不能外推到设计空间之外。",
                     "interp.doe_desirability_enum_only"},
                    {"多响应冲突时优先核对各响应 Desirability 与权重；确认性试验应覆盖总体 D 靠前且工程上可实施的组合。",
                     "interp.doe_desirability_conflict_advice"},
                    {"缺少 Actual Power 或 Effect Size；结果不能用于样本量决策。",
                     "interp.power_missing_metrics"},
                    {"将效应量预先定义为具有工程意义的最小差异，并同时考虑实际脱落、分组和方差不确定性。",
                     "interp.power_ttest_advice"},
                    {"样本量结果是模型和效应量假设的条件结果；试验前应进行敏感性分析并向上取整到可执行的分组方案。",
                     "interp.power_sample_advice"},
                    {"MASE > 1，当前模型不优于朴素基准；上线前应比较替代模型或扩大验证窗口。",
                     "interp.forecast_mase_advice"},
                    {"预测区间反映模型不确定性，不是规格上下限；应通过滚动验证检查不同预测期的稳定性。",
                     "interp.forecast_pi_not_spec"},
                    {"乘法分解要求观测值为正；季节指数与趋势外推对结构变化敏感，应核对残差与移动平均边界。",
                     "interp.decomp_multiplicative_advice"},
                    {"加法分解的季节指数与线性趋势外推对结构变化敏感；应核对残差与非等间隔时间诊断。",
                     "interp.decomp_additive_advice"},
                    {"未来预测依赖平滑/ARIMA 模型假设；应检查残差、自相关和结构变化，避免外推超出历史范围。",
                     "interp.forecast_smoothing_arima_advice"},
                    {"区间图是各组均值的个体置信区间（pooled MSE），不是 Tukey 同时比较。",
                     "interp.anova_interval_plot_not_tukey"},
                    {"残差正态或方差齐性检查提供了需要调查的证据，不能把 p 值直接写成工程差异。",
                     "interp.anova_residual_assumption_evidence"},
                    {"属性一致性报告观察一致率与 Kappa；≥3 名评估者使用 Fleiss，两两比较仍用 Cohen。",
                     "interp.attr_agree_kappa_summary"},
                    {"拒绝 Kappa=0 不等于已证明评估者一致；Kappa 只描述超出偶然的绝对一致率。",
                     "interp.attr_agree_kappa_honesty"},
                    {"有序评级已计算 Kendall 系数；拒绝 W=0 或 τ=0 不等于已证明有序一致。",
                     "interp.attr_agree_kendall_computed"},
                    {"已请求有序评级，但 Kendall 不可识别或等级不足；未伪造 W=1。",
                     "interp.attr_agree_kendall_unavailable"},
                    {"名义评级看 Kappa；有序评级请设置 ordinal=true 以计算 Kendall W/τ。",
                     "interp.attr_agree_ordinal_hint"},
                    {"属性一致性缺少 Facts，不能从表头反解析 Kappa 或 Kendall。",
                     "interp.attr_agree_missing_facts"},
                    {"控制限使用了历史 μ/σ；分阶段估计表仅供对照，不会自动改写全局限。",
                     "interp.imr_historical_limits"},
                    {"存在打标观测，请结合异常观测表调查；解释层不会自动删除这些观测。",
                     "interp.regression_flagged_obs"},
                    {"残差假设检查提供了需要调查的证据；不能把系数显著性直接写成因果关系。",
                     "interp.regression_assumption_against"},
                    {"结合残差对拟合值图、残差顺序图、拟合线图置信/预测带、杠杆值和 Cook's D 后再解释模型。",
                     "interp.regression_residual_advice"},
                    {"在当前显著性水平下，没有可报告为显著的 ANOVA 项；这不等于各组完全相同。",
                     "interp.anova_no_significant"},

                    {"在当前显著性水平下，没有发现可报告为显著的 DOE 项。",
                     "interp.doe_no_significant_terms"},
                    {"未配置中心点，无法用中心点直接检验曲率；二水平线性模型的适用范围有限。",
                     "interp.doe_no_center_points"},
                    {"Kaplan-Meier 生存表为空，不能估计生存曲线或中位寿命。", "interp.km_empty"},
                    {"存在右删失；尾部生存率由较少的风险集支持，不能把删失时间当作失效时间。",
                     "interp.km_right_censoring"},
                    {"按目标任务时间读取生存概率，并报告置信区间；需要比较产品/方案时使用分层或组间检验。",
                     "interp.km_advice"},
                    {"Weibull 参数缺失，不能解释失效率随时间的变化。",
                     "interp.weibull_params_missing"},
                    {"Weibull 参数未收敛，分位寿命不能作为决策依据。",
                     "interp.weibull_not_converged"},
                    {"指数模型估计的是恒定失效率假设下的寿命分布。",
                     "interp.exponential_constant_hazard"},
                    {"指数模型不能描述随时间上升或下降的失效率；应与 Weibull 等模型比较适配性。",
                     "interp.exponential_limitation"},
                    {"报告模型参数和删失处理，并用现场失效机理验证模型假设；不要把分位寿命当成单件保证寿命。",
                     "interp.reliability_advice_no_guarantee"},
                    {"对数正态模型估计了右删失下的位置/尺度参数；分位寿命由 exp(μ + σ Φ⁻¹(p)) 给出。",
                     "interp.lognormal_two_param_summary"},
                    {"缺协方差或残差自由度不足，置信/预测区间不可用；不要仅凭点预测确定最优设置。",
                     "interp.doe_opt_no_prediction_interval"},
                    {"不得把统计预测写成法律/质量承诺；分母与时间窗口必须一并阅读。",
                     "interp.warranty_not_legal_promise"},
                    {"不是 vendor_oracle；不得写成商业软件对齐。", "interp.not_vendor_oracle"},
                    {"不得写成过程合格（直方图初筛 ≠ 混合模型证明）。",
                     "interp.bimodality_not_mixture"},
                    {"双峰初筛为 suspected：单一分布能力指数仅供调查，不得写成过程合格（直方图初筛 ≠ 混合模型证明）。",
                     "interp.bimodality_suspected_full"},
                    {"Hartigan dip 为 evidence_against：单一分布能力指数仅供调查，不得写成过程合格（formula_reference；非 vendor_oracle）。",
                     "interp.hartigan_dip_evidence_against"},
                    {"二维高斯混合为 preferred_2comp：单一分布能力指数仅供调查，不得写成过程合格（formula_reference / gaussian_mixture_k_bic；非 vendor_oracle）。",
                     "interp.mixture_preferred_2comp"},
                    {"未满足 golden/尾部验收前不得开放合格判定。",
                     "interp.johnson_no_pass_fail_until_golden"},
                    {"未验证稳定性不等于过程合格。", "interp.stability_not_capability_pass"},
                    {"ndc 不可估计，不能据此评价测量系统分辨力。", "interp.ndc_unavailable"},
                    {"MSA 结果表为空，不能判定偏倚、线性或 Type 1 能力。", "interp.msa_empty"},
                    {"没有可用的超限点统计，不能判定量具稳定性。",
                     "interp.gage_stability_no_points"},
                    {"当前观测未发现稳定性图超限点；这不等同于量具满足能力要求。",
                     "interp.gage_stability_clear_not_capability"},
                    {"DOE 表中没有可用的 P-Value；不能据此宣称因子或交互作用显著。",
                     "interp.doe_no_pvalue"},
                    {"优先在确认性试验中复核显著项，并结合效应方向选择可操作的因子水平；显著性本身不代表工程影响足够大。",
                     "interp.doe_confirm_significant"},
                    {"未生成立方图（仅支持 2–3 个因子）；高维设计请结合 Pareto 与主效应/交互图。",
                     "interp.doe_no_cube_plot"},
                    {"残差图供调查残差形态；直方图不用于证明正态。",
                     "interp.doe_residual_plots"},
                    {"等值线/曲面在编码 [-1,1] 空间求值；非轴因子默认 hold 编码 0，也可按实际单位 hold；二水平模型无平方项，不能把曲面曲率当成已估计。",
                     "interp.doe_contour_coded_space"},
                    {"检查残差随机性、异常值和重复/纯误差后再确定最优设置；不要仅凭主效应图下结论。",
                     "interp.doe_check_residuals_before_opt"},
                    {"设计矩阵秩亏，回归系数不可解释。",
                     "interp.doe_rank_deficient"},
                    {"未检测到中心点；纯误差估计可能不可用。",
                     "interp.doe_no_center_pure_error"},
                    {"有纯误差但失拟自由度不足，未给出失拟 F/P。",
                     "interp.doe_lof_df_insufficient"},
                    {"检测到星点超出原始因素范围；需人工确认实验可行性。",
                     "interp.doe_axial_beyond_range"},
                    {"存在不可估计项或误差自由度不足，这些项不输出伪造 F/P。",
                     "interp.doe_unestimable_terms"},
                    {"误差自由度不足，不输出 t、F 与 P。",
                     "interp.doe_error_df_insufficient"},
                    {"RSM 系数在编码单位；显著不等于已找到工程最优设定。",
                     "interp.rsm_coded_coeffs"},
                    {"曲面图为静态栅格，不是可旋转交互 3D。",
                     "interp.rsm_static_surface"},
                    {"请结合残差与拟合值/顺序/正态/直方图判断模型充分性。",
                     "interp.rsm_check_model_adequacy"},
                    {"存在负方差分量并已截断为 0；解释时应同时查看原始估计。",
                     "interp.msa_negative_variance"},
                    {"按零件图展示各零件重复测量的离散程度；同一零件上点越集中表示重复性越小。该图不单独判定量具是否合格。",
                     "interp.msa_by_part_plot"},
                    {"将 %Study Var、%Tolerance 和现场公差一起看，不要把单个百分比写成量具合格。",
                     "interp.msa_study_tol_together"},
                    {"偏倚/线性表缺少斜率或端点偏倚，不能作完整判定。",
                     "interp.msa_bias_linearity_incomplete"},
                    {"将端点偏倚与产品公差和测量系统可接受界限比较；回归关系不能替代跨操作者、跨部件的完整 Gage R&R。",
                     "interp.msa_compare_endpoint_bias"},
                    {"按时间顺序回查校准、环境、操作者和量具维护记录；不要在特殊原因未排除前用该量具放行产品。",
                     "interp.msa_stability_investigate"},
                    {"评估者×零件图是观察一致率，不是量具通过或已证明一致。",
                     "interp.msa_appraiser_part_not_pass"},
                    {"Type 1 表缺少 P 或 Cgk，不能完整评价偏倚与重复性。",
                     "interp.msa_type1_incomplete"},
                    {"ndc<5 不是量具不合格的绝对结论，需要结合 %Study Var 和公差风险。",
                     "interp.msa_ndc_lt5_not_fail"},
                    {"Part×Operator 交互 p>0.25，传统流程可考虑缩减模型；当前结果仍保留完整交互。",
                     "interp.msa_interaction_reduce_hint"},
                    {"斜率检验 p≤0.05，提示线性显著；应分别解读各参考水平的偏倚，不宜只用平均偏倚。",
                     "interp.msa_slope_significant"},
                    {"分模式可靠度为 formula_reference（竞争失效作右删失），不是 vendor_oracle / golden；阈值模型与 pinned R 对齐仍未冻结。",
                     "interp.rel_mode_fit_honesty"},
                    {"CIF 为 formula_reference（Aalen–Johansen），不是 Fine-Gray 回归，也不是 cause-specific 可靠度或 vendor_oracle。",
                     "interp.rel_cif_honesty"},
                    {"Fine-Gray 为 formula_reference / fine_gray_binary_ipcw（单协变量 IPCW）；不是 cause-specific Cox、不是 vendor_oracle。",
                     "interp.rel_fine_gray_honesty_binary"},
                    {"Fine-Gray 为 formula_reference / fine_gray_continuous_ipcw（单协变量 IPCW）；不是 cause-specific Cox、不是 vendor_oracle。",
                     "interp.rel_fine_gray_honesty_continuous"},
                    {"Fine-Gray 为 formula_reference / fine_gray_multi_ipcw（多协变量 IPCW，均值中心化）；不是 cause-specific Cox、不是 vendor_oracle；未对齐 pinned R survival::finegray。",
                     "interp.rel_fine_gray_honesty_multi"},
                    {"可靠性结果当前不可识别，不能估计寿命分位数。",
                     "interp.rel_not_identifiable"},
                    {"生存曲线未下降到 0.5，中位寿命不可估计。",
                     "interp.rel_median_not_reached"},
                    {"按目标任务时间读取生存概率和置信区间；比较方案时使用分层或 Log-rank。",
                     "interp.rel_km_advice_logrank"},
                    {"指数拟合未拒绝假设不等于已证明寿命服从指数分布。",
                     "interp.rel_exp_not_proven"},
                    {"对数正态拟合未拒绝假设并不等于已证明寿命服从对数正态。",
                     "interp.rel_lognormal_not_proven"},
                    {"三参数拟合未拒绝假设不等于已证明寿命服从三参数 Weibull。",
                     "interp.rel_weibull3_not_proven"},
                    {"对数正态参数未收敛，分位寿命不能作为决策依据。",
                     "interp.rel_lognormal_not_converged"},
                    {"EDA 图用于探索形态与频数，不是假设检验结论。",
                     "interp.eda_not_hypothesis_test"},
                    {"若要比较原因频次优先级，请改用柏拉图并结合现场验证。",
                     "interp.pareto_use_with_field"},
                    {"本批观测均为精确失效（无左/区间/右删失）；仍走区间 NPMLE 路径，不自动等价替换为经典右删失 KM 报告。",
                     "interp.km_interval_exact_not_classic"},
                    {"默认带宽是 NIST 白噪声固定 ±z/√n（独立性检验），不是 ARIMA 识别用的 Bartlett 变带宽；亦非 Minitab golden。",
                     "interp.acf_nist_band_honesty"},
                    {"存在期望频数过小的类别，卡方近似可能不可靠。",
                     "interp.chi2_small_expected_cats"},
                    {"存在期望频数过小的单元格，卡方近似可能不可靠。",
                     "interp.chi2_small_expected_cells"},
                    {"建议合并相邻类别后复算；当前 P 值可作为探索性证据。",
                     "interp.gof_rec_merge_adjacent"},
                    {"期望频数过低，建议先调整分组（合并类别）再进行拟合优度检验。",
                     "interp.gof_rec_regroup_low_expected"},
                    {"存在期望频数小于 5 的类别，卡方近似可能不可靠。",
                     "interp.gof_diag_expected_lt5_cats"},
                    {"低期望频数比例偏高，建议合并类别并谨慎解释 P 值。",
                     "interp.gof_diag_high_low_expected_share"},
                    {"期望频数过低，卡方近似可靠性较差。",
                     "interp.gof_diag_expected_too_low"},
                    {"存在期望频数小于 1 的单元格，卡方近似 P 值不显示。",
                     "interp.chi2_diag_expected_lt1_cells"},
                    {"拟合优度至少需要两个类别，且类别与计数长度相同。",
                     "interp.gof_err_need_two_categories"},
                    {"各类观察计数必须为非负有限数，类别名不能为空。",
                     "interp.gof_err_nonneg_counts_names"},
                    {"期望比例个数必须与类别个数相同。",
                     "interp.gof_err_proportion_length"},
                    {"观察总计数必须大于 0。",
                     "interp.gof_err_total_count_positive"},
                    {"期望比例必须为正有限数。",
                     "interp.gof_err_proportion_positive"},
                    {"期望比例之和必须为 1。",
                     "interp.gof_err_proportions_sum_one"},
                    {"列联表至少需要两行两列，且标签数量必须匹配。",
                     "interp.chi2_err_table_shape"},
                    {"列联表每一行必须具有相同列数。",
                     "interp.chi2_err_ragged_rows"},
                    {"列联表单元格必须是非负有限计数。",
                     "interp.chi2_err_nonneg_cells"},
                    {"列联表总计数必须大于 0。",
                     "interp.chi2_err_total_positive"},
                    {"百分比表是描述性结果，不能写成已证明存在或不存在关联。",
                     "interp.percent_table_descriptive"},
                    {"行%/列%/合计% 是描述性汇总，不替代独立性检验结论。",
                     "interp.row_col_pct_descriptive"},
                    {"观察频数热图只展示列联表中的计数分布，不能证明因果关系。",
                     "interp.heatmap_counts_not_causal"},
                    {"调整残差热图只描述单元格相对独立假设的偏离方向与相对大小，不是因果证据。",
                     "interp.chi2_residual_heatmap"},
                    {"McNemar 未计算出统计量（非二元、无不一致对或输入不足）。这只说明当前配对表不支持该检验，不能写成前后比例已证明相同或不同。",
                     "interp.mcnemar_not_computable"},
                    {"Cochran Q 未计算出统计量（例如列数不足、编码失败或分母退化）。两列配对请用 McNemar；不得写成已证明各处理阳性率相同或不同。",
                     "interp.cochran_q_not_computable"},
                    {"总体 F 显著只说明至少一组不同，需要多重比较确定具体组对。",
                     "interp.anova_f_needs_posthoc"},
                    {"功效/样本量结果表为空，不能给出设计建议。",
                     "interp.power_table_empty"},
                    {"效应量或 α 配置不在有效范围，不能把数值当作正式设计依据。",
                     "interp.power_invalid_config"},
                    {"预测结果表为空，不能评价预测或未来期。",
                     "interp.forecast_table_empty"},
                    {"季节周期小于 2，不能支持有意义的季节性解释。",
                     "interp.season_period_lt2"},
                    {"季节周期小于 2，不能支持有意义的季节指数解释。",
                     "interp.season_index_lt2"},
                    {"特征分解未收敛时不对主成分或异常点给出稳定解释。",
                     "interp.pca_not_converged"},
                    {"存在小样本组，近似 P 值只作提示。",
                     "interp.small_sample_approx_p"},
                    {"建议回查对应原始行并确认特殊原因。",
                     "interp.review_special_causes"},
                    {"本轮为百分位法，非 BCa。",
                     "interp.percentile_not_bca"},
                    {"α 逐步；非 Best subsets；选择不稳定时勿外推。",
                     "interp.stepwise_not_best_subsets"},
                    {"本轮无负二项/零膨胀/偏移列；非 Minitab golden。",
                     "interp.no_negbin_zi_offset"},
                    {"自研实现；非 sklearn 运行时；非 TreeNet/RF。",
                     "interp.not_sklearn_treenet"},
                    {"模型出现完全分离，极大似然估计可能不存在，不对系数给出稳定解释。",
                     "interp.logistic_complete_separation"},
                    {"IRLS 未收敛，不对发散系数给出稳定解释。",
                     "interp.logistic_irls_not_converged"},
                    {"Hosmer–Lemeshow 在样本量不足、未收敛或完全分离时不计算。",
                     "interp.logistic_hl_not_computed"},
                    {"Hosmer–Lemeshow 分组期望方差过小，检验不可用。",
                     "interp.logistic_hl_low_group_variance"},
                    {"Hosmer–Lemeshow 有效组数不足，检验不可用。",
                     "interp.logistic_hl_too_few_groups"},
                    {"二元 Logistic 回归至少需要三个观测和一个预测变量。",
                     "interp.logistic_err_need_obs_predictor"},
                    {"置信水平、收敛容差和最大迭代次数必须有效。",
                     "interp.logistic_err_invalid_options"},
                    {"预测变量标签数量必须与预测变量列数一致。",
                     "interp.logistic_err_label_count"},
                    {"响应变量必须全部为 0 或 1。",
                     "interp.logistic_err_binary_response"},
                    {"Logistic 回归需要多于参数数量的观测。",
                     "interp.logistic_err_need_more_obs_than_params"},
                    {"Logistic 信息矩阵秩亏，无法进行 IRLS 更新。",
                     "interp.logistic_err_rank_deficient_info"},
                    {"线性预测量达到数值稳定性边界，概率已进行安全截断。",
                     "interp.logistic_warn_eta_clipped"},
                    {"IRLS 在最大迭代次数内未达到收敛容差。",
                     "interp.logistic_warn_irls_not_converged"},
                    {"预测变量完全分离了 0/1 响应，极大似然估计可能不存在。",
                     "interp.logistic_warn_complete_separation"},
                    {"预测概率达到边界，模型可能存在准分离。",
                     "interp.logistic_warn_quasi_separation"},
                    {"无法计算 Logistic 系数协方差矩阵。",
                     "interp.logistic_err_cov_failed"},
                    {"Poisson 回归至少需要三个观测和一个预测变量。",
                     "interp.poisson_reg_err_need_obs_predictor"},
                    {"响应必须为非负有限值，且预测矩阵规整。",
                     "interp.poisson_reg_err_invalid_response"},
                    {"预测变量含非有限值。",
                     "interp.poisson_reg_err_nonfinite_predictor"},
                    {"IRLS 信息矩阵奇异。",
                     "interp.poisson_reg_err_singular_info"},
                    {"已达最大迭代次数。",
                     "interp.glm_warn_max_iterations"},
                    {"无法计算系数协方差。",
                     "interp.poisson_reg_warn_cov_failed"},
                    {"逐步回归需要 ≥4 观测与 ≥2 候选预测变量。",
                     "interp.stepwise_err_need_obs_candidates"},
                    {"α_enter / α_remove 必须在 (0,1)。",
                     "interp.stepwise_err_alpha_range"},
                    {"α_remove < α_enter 可能导致振荡；仍继续。",
                     "interp.stepwise_warn_alpha_order"},
                    {"未选入任何预测变量（仅截距）。",
                     "interp.stepwise_warn_intercept_only"},
                    {"有序 Logistic 需要 ≥10 观测、≥3 有序水平与 ≥1 预测变量。",
                     "interp.ordinal_err_need_obs_levels"},
                    {"响应类别编码越界。",
                     "interp.ordinal_err_bad_category_code"},
                    {"有序 Logistic 信息矩阵奇异。",
                     "interp.ordinal_err_singular_info"},
                    {"检测到 1 点超出 3σ 控制限，建议复核该观测、测量和记录过程。",
                     "interp.spc_rule1_beyond_3sigma"},
                    {"检测到连续 9 点位于中心线同侧，建议复核阶段、设备或批次因素。",
                     "interp.spc_rule2_nine_same_side"},
                    {"检测到连续 6 点单调上升或下降，建议复核趋势、刀具磨损或过程漂移。",
                     "interp.spc_rule3_six_trend"},
                    {"检测到连续 14 点上下交替，建议复核系统性周期或两台设备交替影响。",
                     "interp.spc_rule4_fourteen_alternating"},
                    {"检测到 3 点中有 2 点同侧超过 2σ，提示可能存在较小的过程偏移。",
                     "interp.spc_rule5_two_of_three_2sigma"},
                    {"检测到 5 点中有 4 点同侧超过 1σ，提示可能存在较小的过程偏移。",
                     "interp.spc_rule6_four_of_five_1sigma"},
                    {"检测到连续 15 点落在 1σ 以内，控制限可能过宽或数据存在分层。",
                     "interp.spc_rule7_fifteen_within_1sigma"},
                    {"检测到连续 8 点落在 1σ 以外，提示可能存在混合总体或双群模式。",
                     "interp.spc_rule8_eight_beyond_1sigma"},
                    {"I-MR 不允许 NaN 或无穷观测。",
                     "interp.imr_err_non_finite"},
                    {"MSSD 需要至少一对相继差分。",
                     "interp.imr_err_mssd_need_pair"},
                    {"MSSD 无偏常数 c4' 使用近似式，不是完整 Minitab 表查表。",
                     "interp.imr_warn_mssd_c4_approx"},
                    {"Nelson estimate 仅适用于平均移动极差法；当前 MSSD 方法已忽略该选项。",
                     "interp.imr_warn_nelson_ignored_mssd"},
                    {"Nelson estimate 剔除后无剩余移动极差，回退到未剔除估计。",
                     "interp.imr_warn_nelson_fallback"},
                    {"Nelson estimate 仅适用于平均移动极差法；当前方法已忽略该选项。",
                     "interp.imr_warn_nelson_ignored_method"},
                    {"Xbar-R 要求各组样本量相等，不能用最后一组的 d2/A2 代替。",
                     "interp.xbar_r_err_unequal_n"},
                    {"Xbar-R 子组不允许 NaN 或无穷观测。",
                     "interp.xbar_r_err_non_finite"},
                    {"I-MR-R/S 至少需要两个子组。",
                     "interp.imr_rs_err_need_two_subgroups"},
                    {"各子组必须至少包含两个观测。",
                     "interp.spc_err_subgroup_min_two"},
                    {"I-MR-R/S 子组不允许 NaN 或无穷观测。",
                     "interp.imr_rs_err_non_finite"},
                    {"估计的组间方差为负，已截断为 0；σ_B 可能低估。",
                     "interp.spc_warn_neg_between_var"},
                    {"EWMA 要求非空数据、lambda 位于 (0,1] 且控制限倍数大于 0。",
                     "interp.ewma_err_options"},
                    {"EWMA 的过程标准差必须大于 0。",
                     "interp.ewma_err_sigma"},
                    {"CUSUM 要求非空数据、sigma/h 大于 0 且 k 不小于 0。",
                     "interp.cusum_err_options"},
                    {"区域图至少需要 2 个观测。",
                     "interp.zone_err_need_two"},
                    {"区域图不允许非有限观测。",
                     "interp.zone_err_non_finite"},
                    {"区域得分采用 Jaehn 1/2/4 权重、累计阈值 8（formula_reference），不是 Minitab 自定义权重 golden。",
                     "interp.zone_warn_jaehn_scoring"},
                    {"未提供完整历史 μ/σ；本轮用各组样本均值与全序列 MR/d2 估计 σ。",
                     "interp.zmr_warn_estimated_limits"},
                    {"CUSUM 不使用 Shewhart 特殊原因规则（beyond_control_limit 等），改用上/下侧累计和首次信号。",
                     "interp.cusum_info_not_shewhart_rules"},
                    {"删失工作表为空。",
                     "interp.censoring_worksheet_empty"},
                    {"删失工作表需要 censoring_type 与 time 列。",
                     "interp.censoring_worksheet_missing_columns"},
                    {"interval 行需要 interval_left / interval_right 列。",
                     "interp.censoring_worksheet_missing_interval_bounds"},
                    {"删失契约至少需要一条观测。",
                     "interp.censoring_empty"},
                    {"时间必须为有限非负数；负时间阻止分析。",
                     "interp.censoring_negative_or_nonfinite_time"},
                    {"出现时间为 0 的失效；请确认单位与记录口径。",
                     "interp.censoring_zero_failure_time"},
                    {"区间删失边界必须为有限非负数。",
                     "interp.censoring_interval_nonfinite"},
                    {"区间删失要求左端严格小于右端；反向区间阻止。",
                     "interp.censoring_interval_reversed"},
                    {"暴露量必须为有限非负数。",
                     "interp.censoring_invalid_exposure"},
                    {"观测时间单位不一致，阻止合并分析。",
                     "interp.censoring_time_unit_conflict"},
                    {"零失效（全删失或无 exact 事件）；生存/参数估计可能不可识别。",
                     "interp.censoring_zero_failures"},
                    {"全部为失效事件，无删失。",
                     "interp.censoring_all_failures"},
                    {"全部删失；中位寿命等点估计通常不可得。",
                     "interp.censoring_all_censored"},
                    {"删失契约证据类型 formula_reference；右删失不得当作失效。",
                     "interp.censoring_formula_reference"},
                    {"保修窗口 T_w 必须为正有限数。",
                     "interp.warranty_invalid_window"},
                    {"保修摘要需要明确时间单位。",
                     "interp.warranty_missing_time_unit"},
                    {"R(T_w) 必须落在 [0,1]。",
                     "interp.warranty_invalid_reliability"},
                    {"输入为全删失；预测摘要仍可计算，但不得宣称为观察失效率。",
                     "interp.warranty_all_censored_input"},
                    {"claims/1000 = 1000*(1-R(T_w))；证据类型 formula_reference，非 vendor_oracle。",
                     "interp.warranty_formula_reference"},
                    {"当前摘要标记为 prediction，不得与观察失效计数混读为同一口径。",
                     "interp.warranty_prediction_label"},
                    {"部分 failure_mode 层使用 cause-specific R(T_w)，其余仍用池化 R；证据类型 formula_reference，不是 vendor_oracle。",
                     "interp.warranty_strata_mixed_reliability"},
                    {"分层 expected_failures = 层暴露量 * F_mode(T_w)，F_mode 来自 cause-specific 分模式拟合（竞争失效作右删失）；formula_reference，不是 vendor_oracle。",
                     "interp.warranty_strata_mode_specific_reliability"},
                    {"分模式拟合当前仅支持二参数 Weibull / Lognormal / Exponential / KM；阈值模型请用总体拟合。",
                     "interp.mode_fit_threshold_model_unsupported"},
                    {"无带 failure_mode 标签的 exact 失效，跳过分模式拟合。",
                     "interp.mode_fit_no_labeled_failures"},
                    {"分模式可靠度 = cause-specific：目标模式 exact 为失效，其他已标注模式的 exact 作为右删失，原始 right 仍为右删失；evidence_type=formula_reference，algorithm_id=cause_specific_right_censored_competing；不是 vendor_oracle。",
                     "interp.mode_fit_cause_specific_scope"},
                    {"事件列只接受明确的失效/删失编码；未知值不会被静默当作删失。",
                     "interp.rel_invalid_event_value"},
                    {"删失类型列无法解析，或与事件列冲突；未知值不会被静默改写。left/interval 若出现在数据中，将由删失契约拒绝经典 KM 路径。",
                     "interp.rel_invalid_censoring_type_value"},
                    {"删失契约校验失败。",
                     "interp.censoring_contract_failed"},
                    {"需要至少一行多元观测。",
                     "interp.hotelling_err_empty_matrix"},
                    {"Hotelling T² 至少需要两个变量列。",
                     "interp.hotelling_err_need_multivariate"},
                    {"个体 T² Phase I 需要 m > p+1。",
                     "interp.hotelling_err_phase1_m_gt_p_plus_1"},
                    {"source_rows 长度必须匹配观测数。",
                     "interp.hotelling_err_source_row_mismatch"},
                    {"无法估计均值/协方差（非有限或不矩形）。",
                     "interp.hotelling_err_invalid_matrix"},
                    {"样本协方差奇异，无法计算 T²。",
                     "interp.hotelling_err_singular_covariance"},
                    {"本命令是正式多元 Hotelling T² 控制图，不是 PCA 经验分位 T²。",
                     "interp.hotelling_info_not_pca_empirical_t2"},
                    {"MEWMA 至少需要两个变量列。",
                     "interp.mewma_err_need_multivariate"},
                    {"MEWMA 至少需要 3 个观测。",
                     "interp.mewma_err_need_three_obs"},
                    {"无法估计均值/协方差。",
                     "interp.mewma_err_invalid_matrix"},
                    {"观测矩阵必须矩形。",
                     "interp.mewma_err_ragged_matrix"},
                    {"MEWMA 协方差在当前步奇异。",
                     "interp.mewma_err_singular_step_cov"},
                    {"默认 UCL 使用渐近 χ² 近似，不是仿真 ARL 校准常数；可手工指定 ucl。",
                     "interp.mewma_warn_ucl_not_arl_calibrated"},
                    {"MEWMA 是多元向量平滑；不要与单变量 EWMA 图混淆。",
                     "interp.mewma_info_not_univariate_ewma"},
                    {"需要至少一个子组。",
                     "interp.gv_err_empty_subgroups"},
                    {"子组不能为空。",
                     "interp.gv_err_empty_subgroup"},
                    {"GV 图至少需要两个变量。",
                     "interp.gv_err_need_multivariate"},
                    {"广义方差图要求每个子组大小 n > 变量数 p。",
                     "interp.gv_err_subgroup_too_small"},
                    {"广义方差图要求等量子组。",
                     "interp.gv_err_unequal_subgroups"},
                    {"子组观测必须具有相同变量数。",
                     "interp.gv_err_ragged_subgroup"},
                    {"b1/b2 常数无效。",
                     "interp.gv_err_invalid_b_constants"},
                    {"子组含非有限值。",
                     "interp.gv_err_non_finite"},
                    {"广义方差图按 Montgomery |S| 子组公式；个体观测路径不做假 |S|。",
                     "interp.gv_info_montgomery_subgroup"},
                    {"NIST 指出多元变差图存在争议；本输出仅作 |S| 探索信号，不是唯一变差判定。",
                     "interp.gv_warn_variability_chart_caveat"},
                    {"重复性方差为 0，Probable Error 为 0；请检查设计。",
                     "interp.emp_warn_zero_repeatability"},
                    {"EMP 分级基于 Wheeler ICC，不是 AIAG %Study Var 合格判定。",
                     "interp.emp_info_not_aiag_pass_fail"},
                    {"无带 failure_mode 标签的 exact 失效，跳过 Aalen–Johansen CIF。",
                     "interp.cif_info_no_labeled_failures"},
                    {"累计发生函数 CIF = Aalen–Johansen（formula_reference / aalen_johansen_cif）：总体生存把任一标注失效当作事件；CIF_k 为原因 k 的累计发生概率。不是 Fine-Gray 多协变量回归，不是 cause-specific（竞争删失）可靠度，不是 vendor_oracle。二分类 group 的 Fine-Gray 另有门禁路径。",
                     "interp.cif_info_aalen_johansen_scope"},
                    {"Aalen–Johansen CIF 本身不是 Fine-Gray 回归；不得把 CIF 表写成 Fine-Gray 或商业软件对齐。",
                     "interp.cif_warn_not_fine_gray"},
                    {"不得把本 Fine-Gray 结果写成商业软件对齐或 pinned R survival::finegray 黄金标准。",
                     "interp.fine_gray_warn_not_commercial_alignment"},
                    {"Fine-Gray 信息量退化，未能估计 β。",
                     "interp.fine_gray_err_singular_information_beta"},
                    {"Fine-Gray β 迭代发散。",
                     "interp.fine_gray_err_beta_diverged"},
                    {"Fine-Gray 未在迭代上限内收敛。",
                     "interp.fine_gray_err_not_converged"},
                    {"Fine-Gray 信息量不可逆，未能估计 SE。",
                     "interp.fine_gray_err_singular_information_se"},
                    {"Fine-Gray 协变量矩阵行数与观测不一致。",
                     "interp.fine_gray_err_covariate_row_mismatch"},
                    {"Fine-Gray 需要带 failure_mode 的 exact 失效。",
                     "interp.fine_gray_err_need_labeled_failures"},
                    {"Fine-Gray 协变量矩阵行长度不一致。",
                     "interp.fine_gray_err_ragged_covariate_matrix"},
                    {"fine_gray_multi 需要至少两个协变量；单列请用 fine_gray_continuous。",
                     "interp.fine_gray_err_need_two_covariates"},
                    {"Fine-Gray 连续协变量长度与观测不一致。",
                     "interp.fine_gray_err_continuous_length_mismatch"},
                    {"目标原因失效只出现在一个分组中；估计仍可运行但对比识别较弱。",
                     "interp.fine_gray_warn_target_one_group_only"},
                    {"已指定多协变量列：Fine-Gray 使用 multi IPCW，不与二分类 group Fine-Gray 同时运行。",
                     "interp.fine_gray_info_multi_priority"},
                    {"已指定连续协变量列：Fine-Gray 使用 continuous IPCW，不与二分类 group Fine-Gray 同时运行。",
                     "interp.fine_gray_info_continuous_priority"},
                    {"Isolation Forest 需要非空矩阵与 tree_count ≥ 1。",
                     "interp.iforest_err_empty"},
                    {"至少需要 2 个观测与 2 个变量。",
                     "interp.iforest_err_need_2x2"},
                    {"自研 Isolation Forest；多元异常辅助；非单变量 Grubbs/Dixon；非 TreeNet。",
                     "interp.iforest_scope"},
                    {"K-Means 需要 k ≥ 2。",
                     "interp.kmeans_err_k_lt2"},
                    {"K-Means 需要至少一行有效数值观测。",
                     "interp.kmeans_err_empty"},
                    {"有效观测数必须 ≥ k。",
                     "interp.cluster_err_n_lt_k"},
                    {"初始质心取前 k 个有效观测（分析尺度）；Lloyd 迭代（分配→更新质心）。",
                     "interp.kmeans_init_lloyd"},
                    {"已达最大迭代次数，分配可能尚未完全稳定。",
                     "interp.kmeans_warn_max_iter"},
                    {"CART 需要预测矩阵与响应对齐且非空。",
                     "interp.cart_err_empty"},
                    {"至少需要一个数值预测变量。",
                     "interp.cart_err_no_predictors"},
                    {"有效观测过少，无法按 min_leaf 分裂。",
                     "interp.cart_err_insufficient_for_min_leaf"},
                    {"自研 CART 单树；非 Minitab TreeNet/Random Forests 数值对齐；本轮无成本复杂度剪枝。",
                     "interp.cart_scope_producer"},
                    {"线性判别需要 ≥2 类、≥1 预测变量与足够观测。",
                     "interp.lda_err_invalid"},
                    {"类别编码或预测行列不一致。",
                     "interp.lda_err_bad_row"},
                    {"每个类至少需要 2 个观测。",
                     "interp.lda_err_small_class"},
                    {"合并协方差自由度不足。",
                     "interp.lda_err_pooled_df"},
                    {"合并协方差奇异。",
                     "interp.lda_err_singular_pooled"},
                    {"线性判别（等协方差）；非 Minitab golden；不做 QDA。",
                     "interp.lda_scope_producer"},
                    {"层次聚类需要非空矩阵且 k≥2。",
                     "interp.hclust_err_invalid"},
                    {"Complete linkage + 欧氏距离；非 Minitab golden。",
                     "interp.hclust_linkage_scope"},
                    {"观测不足（n<30），无法做二维高斯混合 EM；不得把不足样本写成已排除混合。",
                     "interp.mixture2_err_insufficient_n"},
                    {"二维高斯混合 EM 密度退化，未能收敛；不得伪造混合拟合。",
                     "interp.mixture2_err_density_degenerate"},
                    {"二维高斯混合 EM 未在迭代上限内收敛；不得把未收敛结果写成已确认混合。",
                     "interp.mixture2_err_not_converged"},
                    {"观测不足（n<30），无法做多 k 高斯混合 BIC 搜索；不得把不足样本写成已排除混合。",
                     "interp.mixture_k_err_insufficient_n"},
                    {"多 k 高斯混合 BIC 搜索未得到可用拟合；不得伪造混合结论。",
                     "interp.mixture_k_err_search_failed"},
                    {"能力指标未验证过程稳定性和正态性；数值仅供调查，不能单独作为过程合格结论。",
                     "interp.cap_assumption_stability_normality"},
                    {"能力指标未验证过程稳定性；数值仅供调查，不能单独作为过程合格结论。",
                     "interp.cap_assumption_stability_only"},
                    {"观测不足，无法做能力分析前的 I-MR 稳定性初筛。",
                     "interp.cap_stability_insufficient_n"},
                    {"I-MR Rule-1 初筛未检出超限点；这不是完整控制图验收，也不等于已验证稳定性/正态性，不得自动开放合格判定。",
                     "interp.capability_stability_clear_not_verified"},
                    {"能力分析稳定性前置：当前仅做公式参考级 I-MR Rule-1 初筛；合格判定保持关闭，直至独立稳定性与正态性验收工作流落地。",
                     "interp.capability_stability_prerequisite"},
                    {"观测不足（n<30），无法做能力分析前的双峰直方图初筛；不得把不足样本写成已排除双峰。",
                     "interp.cap_bimodality_insufficient_n"},
                    {"直方图双峰初筛未检出可分离峰；这不是单峰证明，不得据此开放过程合格判定。",
                     "interp.cap_bimodality_clear_not_proof"},
                    {"能力分析双峰前置：当前为公式参考级直方图峰谷初筛；Hartigan dip 与二维高斯混合另有门禁筛查；合格判定保持关闭。",
                     "interp.cap_bimodality_screen_note"},
                    {"观测不足（n<8），无法做 Hartigan dip 单峰门禁筛查；不得把不足样本写成已证明单峰。",
                     "interp.cap_hartigan_insufficient_n"},
                    {"Hartigan dip 门禁筛查未拒绝 Uniform 零假设下的单峰（formula_reference）；这不是过程单峰证明，也不得开放合格判定。",
                     "interp.cap_hartigan_consistent_not_proof"},
                    {"能力分析 Hartigan dip 前置：公式参考级研究筛查（Uniform 零假设 MC）；不是商业软件对齐；二维高斯混合另有门禁；合格判定保持关闭。",
                     "interp.cap_hartigan_screen_note"},
                    {"观测不足（n<30），无法做多 k 高斯混合门禁；不得把不足样本写成已排除混合。",
                     "interp.cap_mixture_insufficient_n"},
                    {"多 k 高斯混合门禁未得到可用拟合；不得伪造混合结论或开放合格判定。",
                     "interp.cap_mixture_failed"},
                    {"多 k 高斯混合门禁未优选多成分（formula_reference / gaussian_mixture_k_bic）；这不是单峰/单成分证明，也不得开放合格判定。",
                     "interp.cap_mixture_not_preferred"},
                    {"能力分析混合模型前置：高斯 EM + BIC 搜索 k=1..k_max（formula_reference / gaussian_mixture_k_bic）；不是非高斯混合，不是商业软件对齐，合格判定保持关闭。",
                     "interp.cap_mixture_screen_note"},
                    {"Johnson 能力为研究/预览结果：可显示拟合与变换尺度指数，但不得输出过程合格判定（门禁：缺 golden/尾部/人工解释验收）。",
                     "interp.johnson_capability_gated"},
                    {"正态能力未满足稳定性/正态性验收前置：禁止过程合格判定（pass_fail_judgment_allowed=false）。",
                     "interp.cap_pass_fail_blocked_stability"},
                    {"规格限落在 Johnson 变换定义域外，无法计算 Pp/Ppk。",
                     "interp.johnson_spec_outside_support"},
                    {"至少一个规格限落在变换定义域外；越界侧的百分位回推未实现，仅输出仍可变换规格的 overall 指数。",
                     "interp.johnson_spec_partial_outside"},
                    {"Johnson 变换路径只报告 overall Pp/Ppk，不报告 within Cp/Cpk。",
                     "interp.johnson_within_not_applicable"},
                    {"Johnson 变换至少需要 8 个有限观测。",
                     "interp.johnson_err_need_eight"},
                    {"样本量不足以估计 Johnson 分位匹配所需的尾部分位数。",
                     "interp.johnson_warn_tail_quantile"},
                    {"Johnson 变换按 Chou et al. (1998) 分位匹配与 AD p 值选择；数值是公式参考，不是 Minitab Individual Distribution Identification 导出。",
                     "interp.johnson_formula_reference"},
                    {"非正态能力使用拟合分布 CDF 的 Z-score 法计算 Pp/Ppk；不报告 Cp/Cpk。数值是公式参考，不是 Minitab 导出。",
                     "interp.cap_nonnormal_zscore_scope"},
                    {"Box-Cox 变换至少需要两个有效观测。",
                     "interp.box_cox_err_need_two"},
                    {"Box-Cox 变换要求所有观测严格大于 0。",
                     "interp.box_cox_err_positive"},
                    {"Box-Cox lambda 必须位于 [-5, 5]。",
                     "interp.box_cox_err_lambda_range"},
                    {"n<8 时正态性近似较粗糙，未拒绝正态假设不能当作已验证正态。",
                     "interp.normality_warn_n_lt8"},
                    {"二项过程能力未验证独立性、恒定 p 与稳定性，不能写成过程合格。",
                     "interp.attr_binomial_assumption_static"},
                    {"泊松过程能力未验证独立性、恒定 DPU 与稳定性，不能写成过程合格。",
                     "interp.attr_poisson_assumption_static"},
                    {"组间/组内能力至少需要两个子组。",
                     "interp.cap_between_within_need_two_subgroups"},
                    {"散点图至少需要两个有效的完整观测行。",
                     "diag.scatter_need_two_complete"},
                    {"区间图存在有效观测数少于 2 的分组，无法估计标准误。",
                     "diag.interval_group_lt2_se"},
                    {"相关图至少需要选择两个连续变量。",
                     "diag.correlogram_need_two_vars"},
                    {"正态概率图至少需要三个有效观测。",
                     "diag.normal_prob_need_three"},
                    {"矩阵图至少需要两个连续变量。",
                     "diag.matrix_need_two_vars"},
                    {"平行坐标图至少需要两个连续变量。",
                     "diag.parallel_need_two_vars"},
                    {"密度估计至少需要两个有限观测。",
                     "diag.density_need_two_finite"},
                    {"二维分箱至少需要两个 complete-case 点。",
                     "diag.hexbin_need_two_complete"},
                    {"自研二叉 CART；非 Minitab TreeNet/Random Forests 对齐；本轮无成本复杂度剪枝。",
                     "interp.cart_honesty"},
                    {"临界值为大样本 MacKinnon 风格常数；非 Minitab 导出；宜与 ACF/PACF 一并阅读。",
                     "interp.adf_mackinnon_honesty"},
                    {"本轮仅 complete linkage；非 K-Means；非 Minitab golden。",
                     "interp.hier_cluster_honesty"},
                    {"本轮仅 logit 链；非名义 Logistic；非 Minitab golden。",
                     "interp.ordinal_logistic_honesty"},
                    {"等协方差 LDA；不做 QDA；非 Minitab golden。",
                     "interp.lda_honesty"},
                    {"分层均值图只用于探索变异来源，不能把结果写成过程判定。",
                     "interp.layer_means_not_process_call"},
                    {"白噪声固定带宽；非预白化；宜与 ACF/PACF 对照。",
                     "interp.bw_fixed_not_prewhitened"},
                    {"不要把逐比较 alpha 当成家族错误率。",
                     "interp.do_not_treat_pairwise_alpha_as_fwer"},
                    {"等值线/曲面按前两因子绘制，其余因子编码 hold=0；用于探索响应形状。",
                     "interp.doe_contour_rsm_hold0"},
                    {"EWMA 仅启用「单点超出 3σ 控制限」；其余特殊原因规则不适用，超限点需结合原始观测调查。",
                     "interp.spc_ewma_limits_only"},
                    {"CUSUM 使用累计和决策间隔信号，不套用 Shewhart 特殊原因规则（beyond_control_limit 等）；信号点需结合原始行调查，不是删点指令。",
                     "interp.spc_cusum_not_shewhart"},
                    {"未发现当前规则能够自动判定的异常；请结合现场工艺、规格要求和图表进行确认。",
                     "interp.spc_no_auto_anomaly"},
                    {"将统计结果与规格、风险等级、历史基线和现场特殊原因结合后再采取措施。",
                     "interp.spc_combine_field_action"},
                    {"未提供完整历史 μ/σ 表；本轮用各组样本均值与全序列 MR/d2 估计 σ。",
                     "interp.z_mr_sample_params"},
                    {"移动平均图与 EWMA 不同；仅完整窗 MA 点参与判读，不是 Minitab golden。",
                     "interp.ma_chart_honesty"},
                    {"区域图采用 Jaehn 1/2/4 计分（formula_reference），不是完整 Shewhart 特殊原因规则（beyond_control_limit 等）的替代品。",
                     "interp.zone_chart_honesty"},
                    {"分辨率 III；非 CCD/BBD；非 Minitab golden。",
                     "interp.plackett_burman_honesty"},
                    {"证据类型 formula_reference；未宣称全域预测最优；非 vendor_oracle。",
                     "interp.bbd_design_honesty"},
                    {"证据类型 formula_reference；非 vendor_oracle / 商业软件对齐。",
                     "interp.ccd_design_honesty"},
                    {"分模式 R 为 cause-specific（竞争失效作右删失）formula_reference，不是 vendor_oracle；不得写成商业软件对齐。",
                     "interp.warranty_mode_r_honesty"},
                    {"与 correlation_plot 分流；非 Graph Builder。",
                     "interp.correlogram_honesty"},
                    {"结合因子均值表回查占主导的因子水平；需要检验显著性时使用 ANOVA 或回归。",
                     "interp.multi_vari_advice"},
                    {"先看均值面板中极差较大的单元，再对照标准差面板定位离散度来源。",
                     "interp.variability_advice"},
                    {"变异性图与 Multi-Vari 语义不同：本命令输出均值+极差与标准差双面板，不替代 ANOVA。",
                     "interp.variability_honesty"},
                    {"对照 P clustering / mixtures / trends / oscillation 与现场时序，小 P 只表示该模式方向上的偏离证据。",
                     "interp.run_chart_pattern_advice"},
                    {"结合直方图与方法族诊断调查覆盖区间；需要对照规格时使用过程能力分析。",
                     "interp.tolerance_advice"},
                    {"非参数容差依赖序统计与连续分布假设；achieved confidence 低于目标时，只说明样本量不足，不是计算错误。",
                     "interp.tolerance_nonparametric_honesty"},
                    {"Johnson 变换后的 Pp/Ppk 是变换尺度上的 overall 指数；未拒绝变换后正态假设不等于原始数据服从正态分布，也不能写成过程合格。",
                     "interp.johnson_pp_honesty"},
                    {"非正态 Z-score Pp/Ppk 依赖所选分布的 CDF；拟合未拒绝假设不等于已证明过程服从该分布，也不能写成过程合格。",
                     "interp.nonnormal_zscore_pp_honesty"},
                    {"组间/组内能力使用 σ_BW 计算 Cp/Cpk、样本标准差计算 Pp/Ppk；未验证稳定性不等于过程合格。",
                     "interp.between_within_capability_honesty"},
                };
                for (const auto& [zh, id] : exact_ids) {
                    if (bullet == zh) {
                        bullet = domain::resolve_report_text(id, language, missing_out).text;
                        break;
                    }
                }
            }
        }
    }
}

void localize_page_title(
    std::string& title,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out);

void localize_visible_tables(
    std::vector<domain::StatisticTable>& tables,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    static const std::pair<const char*, const char*> title_ids[] = {
        {"保修摘要", "table.warranty_summary"},
        {"Weibull 参数", "table.weibull_params"},
        {"三参数 Weibull 参数", "table.weibull3_params"},
        {"Lognormal 参数", "table.lognormal_params"},
        {"百分位寿命", "table.percentiles"},
        {"失效模式（观测到的 exact 失效）", "table.failure_modes"},
        {"Kaplan-Meier 摘要", "table.km_summary"},
        {"Kaplan-Meier 生存表", "table.km_survival"},
        {"生存估计", "table.survival_estimates"},
        {"分模式可靠度（cause-specific）", "table.mode_specific_reliability"},
        {"累计发生函数 CIF（Aalen-Johansen）", "table.cif_aalen_johansen"},
        {"Fine-Gray 子分布风险（多协变量）", "table.fine_gray_multi"},
        {"Fine-Gray 子分布风险（连续协变量）", "table.fine_gray_continuous"},
        {"Fine-Gray 子分布风险（二分类 group）", "table.fine_gray_binary"},
        {"Exponential 参数", "table.exponential_params"},
        {"两参数指数参数", "table.exponential2_params"},
        {"三参数对数正态参数", "table.lognormal3_params"},
        {"系数（编码单位）", "table.rsm_coefficients_coded"},
        {"二项能力", "table.binomial_capability"},
        {"泊松能力", "table.poisson_capability"},
        {"Laney 图逐子组统计", "table.laney_subgroup_stats"},
        {"区域图逐点统计", "table.zone_point_stats"},
        {"P 图", "plot.p_chart"},
        {"NP 图", "plot.np_chart"},
        {"C 图", "plot.c_chart"},
        {"U 图", "plot.u_chart"},
        {"Laney P' 图", "plot.laney_p_chart"},
        {"Laney U' 图", "plot.laney_u_chart"},
        {"参数分布比较", "table.param_dist_compare"},
        {"Log-rank 分组比较", "table.log_rank"},
        {"失效模式分母追溯", "table.warranty_failure_mode_denominators"},
        {"分组分母追溯", "table.warranty_group_denominators"},
        {"纯误差与失拟", "table.pure_error_lof"},
        {"变换后过程能力", "table.box_cox_capability"},
        {"Box-Cox λ 选择诊断", "table.box_cox_lambda"},
        {"Johnson 变换", "table.johnson_transform"},
        {"系数与效应", "table.doe_coefficients_effects"},
        {"模型项与区组", "table.doe_model_terms_blocks"},
        {"中心点与曲率", "table.doe_center_curvature"},
        {"残差诊断", "table.residual_diagnostics"},
        {"设计信息", "table.design_info"},
        {"设计生成器", "table.design_generators"},
        {"定义关系", "table.defining_relation"},
        {"别名结构", "table.alias_structure"},
        {"设计矩阵", "table.design_matrix"},
        {"因素定义", "table.factor_definitions"},
        {"Plackett–Burman 设计", "table.plackett_burman_design"},
        {"DOE 响应分析", "table.doe_response_analysis"},
        {"DOE 响应优化", "table.doe_response_optimization"},
        {"响应曲面分析", "table.rsm_analysis"},
        {"正态过程能力分析", "table.capability_normal"},
        {"Johnson 变换过程能力", "table.capability_johnson"},
        {"非正态过程能力", "table.capability_nonnormal"},
        {"组间/组内过程能力", "table.capability_between_within"},
        {"过程数据", "table.process_data"},
        {"分布参数", "table.distribution_params"},
        {"能力指数", "table.capability_indices"},
        {"过程能力 Sixpack", "table.capability_sixpack"},
        {"I-MR 参数", "table.imr_params"},
        {"I-MR-R/S 参数", "table.imrrs_params"},
        {"I-MR 控制图", "table.imr_chart"},
        {"I-MR 逐点统计", "table.imr_point_stats"},
        {"Xbar-R 参数", "table.xbar_r_params"},
        {"Xbar-R 控制图", "table.xbar_r_chart"},
        {"Xbar-R 逐子组统计", "table.xbar_r_subgroup_stats"},
        {"Xbar-S 参数", "table.xbar_s_params"},
        {"Xbar-S 控制图", "table.xbar_s_chart"},
        {"Xbar-S 逐子组统计", "table.xbar_s_subgroup_stats"},
        {"EWMA 参数", "table.ewma_params"},
        {"EWMA 控制图", "table.ewma_chart"},
        {"EWMA 逐点统计", "table.ewma_point_stats"},
        {"CUSUM 参数", "table.cusum_params"},
        {"CUSUM 控制图", "table.cusum_chart"},
        {"CUSUM 逐点统计", "table.cusum_point_stats"},
        {"CUSUM 信号", "table.cusum_signals"},
        {"Z-MR 参数", "table.z_mr_params"},
        {"Z-MR 控制图", "table.z_mr_chart"},
        {"Z-MR 逐点统计", "table.z_mr_point_stats"},
        {"移动平均参数", "table.ma_params"},
        {"移动平均控制图", "table.ma_chart"},
        {"移动平均逐点统计", "table.ma_point_stats"},
        {"历史参数与分阶段估计", "table.spc_historical_stage"},
        {"规则证据", "table.rule_evidence"},
        {"特殊原因规则证据", "table.special_cause_rule_evidence"},
        {"DOE ANOVA", "table.doe_anova"},
        {"双样本描述统计", "table.two_sample_descriptive"},
        {"检验结果", "table.test_results"},
        {"观察频数", "table.observed_counts"},
        {"行百分比", "header.row_percent"},
        {"列百分比", "header.column_percent"},
        {"合计百分比", "header.total_percent"},
        {"卡方检验", "table.chi_square_test"},
        {"单元格统计", "table.cell_statistics"},
        {"Grouping Information", "table.grouping_information"},
        {"Grouping Information (Nemenyi)", "table.grouping_information_nemenyi"},
        {"Durbin-Watson", "table.durbin_watson"},
        {"假设检查", "table.assumption_checks"},
        {"拟合与诊断", "table.fit_diagnostics"},
        {"配对差值统计", "table.paired_difference_stats"},
        {"等价性区间", "table.equivalence_interval"},
        {"变换参数", "table.transform_parameters"},
        {"观察与期望", "table.observed_expected"},
        {"单比例描述", "table.one_proportion_descriptive"},
        {"泊松率描述", "table.poisson_rate_descriptive"},
        {"双样本泊松率描述", "table.two_poisson_rate_descriptive"},
        {"MEWMA 摘要", "table.mewma_summary"},
        {"Nested Gage R&R ANOVA", "table.nested_gage_rr_anova"},
        {"Process Data", "table.process_data"},
        {"Overall Capability", "table.overall_capability"},
        {"Gage R&R 摘要", "table.gage_rr_summary"},
        {"Gage R&R 方差分析", "table.gage_rr_anova"},
        {"EMP 统计", "table.emp_stats"},
        {"Type 1 Gage 结果", "table.type1_gage_results"},
        {"Stability 统计", "table.stability_stats"},
        {"方差分量", "table.variance_components"},
        {"单因素 ANOVA", "table.oneway_anova"},
        {"组均值", "table.group_means"},
        {"方差分析", "table.anova_table"},
        {"ANOVA 表", "table.anova_effects_table"},
        {"方差分量（交叉 ANOVA）", "table.variance_components_crossed_anova"},
        {"因子 A 均值", "table.factor_a_means"},
        {"因子 B 均值", "table.factor_b_means"},
        {"拟合优度", "table.goodness_of_fit"},
        {"系数与 Odds Ratio", "table.coefficients_and_odds_ratio"},
        {"拟合与残差", "table.fitted_and_residuals"},
        {"参数估计", "table.parameter_estimates"},
        {"方差检验结果", "table.variance_test_results"},
        {"摘要", "table.km_interval_summary"},
        {"I-MR-R/S 逐子组统计", "table.imr_rs_subgroup_stats"},
        {"缺陷计数", "table.defect_counts"},
        {"泊松参数", "table.poisson_parameters"},
        {"交互均值", "table.interaction_means"},
        {"Tukey 同时比较", "table.tukey_comparisons"},
        {"Tukey 差值同时区间", "table.tukey_diff_intervals"},
        {"区间图", "table.interval_plot_table"},
        {"线性回归", "table.linear_regression"},
        {"模型摘要", "table.model_summary"},
        {"系数", "table.coefficients"},
        {"回归方差分析", "table.regression_anova"},
        {"异常观测", "table.unusual_observations"},
        {"正态性检验", "table.normality_test"},
        {"异常值检验", "table.outlier_test"},
        {"描述统计", "table.descriptive_stats"},
        {"显示描述性统计", "table.display_descriptive_stats"},
        {"方法", "table.method"},
        {"置信区间", "table.confidence_interval"},
        {"等价性检验", "table.equivalence_test"},
        {"单样本 t 检验", "table.one_sample_t"},
        {"单样本 Z 检验", "table.one_sample_z"},
        {"双样本 t 检验", "table.two_sample_t"},
        {"配对 t 检验", "table.paired_t"},
        {"相关系数矩阵", "table.correlation_matrix"},
        {"协方差矩阵", "table.covariance_matrix"},
        {"偏相关系数矩阵", "table.partial_correlation_matrix"},
        {"相关分析详细结果", "table.correlation_details"},
        {"响应目标", "table.doe_response_objectives"},
        {"最佳组合", "table.doe_best_combination"},
        {"候选组合", "table.doe_candidate_combinations"},
        {"响应预测", "table.doe_response_predictions"},
        {"因子均值", "table.factor_means"},
        {"单元均值", "table.cell_means"},
        {"单元统计", "table.variability_cell_stats"},
        {"四图说明", "table.eda_four_plot_notes"},
        {"箱线统计", "table.box_plot_stats"},
        {"结构摘要", "table.cause_effect_structure"},
        {"关于中位数的游程", "table.runs_about_median"},
        {"上升/下降游程", "table.runs_up_down"},
        {"区域图参数", "table.zone_params"},
        {"广义方差摘要", "table.gv_summary"},
        {"逐子组 |S|", "table.gv_subgroup_s"},
        {"T² 摘要", "table.t2_summary"},
        {"逐点 T²", "table.t2_pointwise"},
        {"逐点 MEWMA T²", "table.mewma_t2_pointwise"},
        {"秩和检验", "table.mann_whitney_rank_sum"},
        {"符号秩检验", "table.wilcoxon_signed_rank"},
        {"位置估计（Walsh）", "table.walsh_location"},
        {"符号摘要", "table.sign_summary"},
        {"中位数置信区间", "table.median_ci"},
        {"比较准则", "table.comparison_criterion"},
        {"Runs检验", "table.runs_test"},
        {"Fisher精确检验", "table.fisher_exact"},
        {"2×2 交叉表", "table.crosstab_2x2"},
        {"处理成功计数", "table.treatment_success_counts"},
        {"各处理阳性率", "table.treatment_positive_rates"},
        {"各组 Above/Below", "table.groups_above_below"},
        {"Kruskal-Wallis 结果", "table.kruskal_wallis_results"},
        {"检验统计量", "table.test_statistic"},
        {"处理摘要", "table.treatment_summary"},
        {"Nemenyi 成对比较", "table.nemenyi_pairwise"},
        {"Steel-Dwass 成对比较", "table.steel_dwass_pairwise"},
        {"Dunn 成对比较", "table.dunn_pairwise"},
        {"评估者内一致性", "table.aa_within_evaluator"},
        {"评估者间一致性", "table.aa_between_evaluator"},
        {"评估者间总体 Kappa（Fleiss）", "table.aa_overall_fleiss_kappa"},
        {"评估者间 Kendall W", "table.aa_between_kendall_w"},
        {"评估者内 Kendall W", "table.aa_within_kendall_w"},
        {"评估者 vs 标准 Kendall τ", "table.aa_evaluator_vs_standard_kendall_tau"},
        {"全体 vs 标准 Kendall τ", "table.aa_all_vs_standard_kendall_tau"},
        {"评估者×零件一致率", "table.aa_evaluator_part_agreement"},
        {"评估者一致率", "table.aa_evaluator_agreement_rate"},
        {"Cochran Q 检验", "page.cochran_q"},
        {"Mood 中位数检验", "page.mood_median"},
        {"Friedman 检验", "page.friedman"},
        {"ADF 检验", "table.adf_test"},
        {"ADF 回归系数", "table.adf_coefficients"},
        {"拟合与 Pearson 残差", "table.fitted_pearson_residuals"},
        {"异常分数", "table.anomaly_scores"},
        {"Bootstrap 摘要", "table.bootstrap_summary"},
        {"合并历程", "table.hclust_merge_history"},
        {"簇分配（切 k）", "table.hclust_cut_assignment"},
        {"系数（阈值与斜率）", "table.ordinal_thresholds_slopes"},
        {"类均值", "table.lda_class_means"},
        {"相关矩阵", "table.correlation_matrix"},
        {"拟合与预测明细", "table.fit_forecast_detail"},
        {"预测准确度", "table.forecast_accuracy"},
        {"候选模型比较", "table.candidate_model_comparison"},
        {"模型摘要与预测", "table.model_summary_forecast"},
        {"季节指数", "table.seasonal_indices"},
        {"SARIMA 候选模型比较", "table.sarima_candidate_comparison"},
        {"Rolling-origin 评估", "table.rolling_origin_eval"},
        {"Rolling-origin 明细", "table.rolling_origin_detail"},
        {"特征值与解释率", "table.pca_eigen"},
        {"主成分系数", "table.pca_coefficients"},
        {"相关载荷", "table.pca_loadings"},
        {"主成分得分", "table.pca_scores"},
        {"T² 与 Q 阈值", "table.pca_tq_limits"},
        {"T² 与 Q 残差", "table.pca_tq_residuals"},
        {"簇摘要", "table.cluster_summary"},
        {"质心（分析尺度）", "table.centroids_analysis_scale"},
        {"簇分配", "table.cluster_assignment"},
        {"树结点", "table.cart_nodes"},
        {"变量重要性", "table.variable_importance"},
        {"训练集混淆矩阵", "table.train_confusion"},
        {"逐步步骤", "table.stepwise_steps"},
        {"选入项", "table.selected_terms"},
        {"终模型系数", "table.final_model_coefficients"},
        {"功效与样本量", "table.power_sample_size"},
        {"抽样计划", "table.sampling_plan"},
        {"OC 曲线", "table.oc_curve"},
        {"决策限", "table.decision_limits"},
        {"组均值", "table.group_means"},
    };
    static const std::pair<const char*, const char*> header_ids[] = {
        {"属性", "table.property"},
        {"值", "table.value"},
        {"项目", "header.item"},
        {"指标", "header.metric"},
        {"数值", "header.numeric_value"},
        {"参数", "header.parameter"},
        {"来源", "header.source"},
        {"项", "header.term"},
        {"观测", "header.observation"},
        {"期望 Overall", "header.expected_overall"},
        {"期望 Within", "header.expected_within"},
        {"估计", "header.estimate"},
        {"下限", "header.lower"},
        {"上限", "header.upper"},
        {"组", "header.group"},
        {"均值", "header.mean"},
        {"标准差", "header.stdev"},
        {"水平", "header.level"},
        {"差值", "header.difference"},
        {"样本 1", "header.sample_1"},
        {"样本 2", "header.sample_2"},
        {"配对差值", "header.paired_difference"},
        {"参考样本", "header.reference_sample"},
        {"检验样本", "header.test_sample"},
        {"原因", "header.cause"},
        {"原因数", "plot.axis.cause_count"},
        {"目标值", "header.target_value"},
        {"权重", "header.weight"},
        {"目标", "header.goal"},
        {"最佳预测", "header.best_prediction"},
        {"单响应 D", "header.single_response_d"},
        {"排序", "header.rank"},
        {"置信下限", "header.confidence_lower"},
        {"置信上限", "header.confidence_upper"},
        {"预测下限", "header.prediction_lower"},
        {"预测上限", "header.prediction_upper"},
        {"预测值", "header.predicted_value"},
        {"预测", "header.prediction"},
        {"比值", "header.ratio"},
        {"下限 z", "header.lower_z"},
        {"下限 P", "header.lower_p"},
        {"上限 z", "header.upper_z"},
        {"上限 P", "header.upper_p"},
        {"下限 t", "header.lower_t"},
        {"上限 t", "header.upper_t"},
        {"CI 方法", "header.ci_method"},
        {"界限", "header.equivalence_limits"},
        {"结论", "header.conclusion"},
        {"方法", "header.method"},
        {"统计量", "header.statistic"},
        {"状态", "header.status"},
        {"说明", "header.notes"},
        {"检查项", "header.check_item"},
        {"判定区", "header.decision_zone"},
        {"顺序", "header.order"},
        {"原始行", "header.source_row"},
        {"规则ID", "header.rule_id"},
        {"规则名称", "header.rule_name"},
        {"判定窗口", "header.decision_window"},
        {"阈值", "header.threshold"},
        {"比较方向", "header.comparison_direction"},
        {"解释", "header.explanation"},
        {"触发图点", "header.triggered_plot_points"},
        {"建议动作", "header.suggested_action"},
        {"不适用原因", "header.not_applicable_reason"},
        {"未验证原因", "header.not_verified_reason"},
        {"计算失败原因", "header.calculation_failed_reason"},
        {"触发规则", "header.triggered_rules"},
        {"主要规则", "header.primary_rule"},
        {"子组", "header.subgroup"},
        {"阶段", "header.stage"},
        {"信号", "header.signal"},
        {"方向", "header.direction"},
        {"间隔", "header.interval"},
        {"规则", "header.rule"},
        {"中心线", "header.center_line"},
        {"绘制值", "header.plotted_value"},
        {"观测值", "header.observation_value"},
        {"观测序号", "header.observation_index"},
        {"累计值", "header.cumulative_value"},
        {"区域带", "header.zone_band"},
        {"累计得分", "header.cumulative_score"},
        {"Jaehn 信号", "header.jaehn_signal"},
        {"检验数", "header.inspected_count"},
        {"单位数", "header.unit_count"},
        {"不合格品率", "header.proportion_defective"},
        {"不合格品数", "header.number_defective"},
        {"缺陷数", "header.defect_count"},
        {"单位缺陷数", "header.defects_per_unit"},
        {"响应", "header.response"},
        {"拟合值", "header.fitted"},
        {"残差", "header.residual"},
        {"标准化残差", "header.std_residual"},
        {"显著", "header.significant"},
        {"族置信水平", "header.family_confidence_level"},
        {"误差 DF", "header.error_df"},
        {"最小期望频数", "header.min_expected_count"},
        {"<5 类别数", "header.categories_below_five"},
        {"期望<5 数", "header.expected_below_five_count"},
        {"类别数", "header.category_count"},
        {"有效性", "header.validity"},
        {"合计", "header.total"},
        {"行", "header.row"},
        {"列", "header.column"},
        {"行数", "header.row_count"},
        {"可估计性", "header.estimability"},
        {"因子 A", "header.factor_a"},
        {"因子 B", "header.factor_b"},
        {"内部标准化残差", "header.internally_standardized_residual"},
        {"学生化残差", "header.studentized_residual"},
        {"删除学生化残差", "header.deleted_studentized_residual"},
        {"杠杆值", "header.leverage"},
        {"杠杆", "header.leverage_abbr"},
        {"Cook 距离", "header.cooks_distance"},
        {"Cook", "header.cook_abbr"},
        {"诊断标记", "header.diagnostic_flags"},
        {"标记", "header.flag"},
        {"单元格", "header.cell"},
        {"计数", "header.count"},
        {"连续性校正", "header.continuity_correction_alt"},
        {"优势比 OR", "header.odds_ratio"},
        {"对比", "header.contrast"},
        {"未调整 P", "header.unadjusted_p"},
        {"Bonferroni P", "header.bonferroni_p"},
        {"平均秩差", "header.mean_rank_diff"},
        {"平均秩", "header.mean_rank"},
        {"组别", "header.group_label"},
        {"处理", "header.treatment"},
        {"Ties 修正", "header.ties_correction"},
        {"非零差值 N", "header.nonzero_diff_n"},
        {"正秩和", "header.positive_rank_sum"},
        {"负秩和", "header.negative_rank_sum"},
        {"估计中位数", "header.estimated_median"},
        {"有效符号 N", "header.valid_signs_n"},
        {"结数", "header.tie_count"},
        {"样本中位数", "header.sample_median"},
        {"假设中位数 η0", "header.hypothesized_median_eta0"},
        {"名义置信水平", "header.nominal_confidence"},
        {"达到水平", "header.attained_level"},
        {"准则", "header.criterion"},
        {"近似", "header.approximation"},
        {"成功数", "header.success_count"},
        {"成功率", "header.success_rate"},
        {"总体中位数 M", "header.overall_median_m"},
        {"中位数", "header.median"},
        {"图", "header.chart"},
        {"检查假设", "header.check_hypothesis"},
        {"最小值", "header.minimum"},
        {"最大值", "header.maximum"},
        {"下须", "header.lower_whisker"},
        {"上须", "header.upper_whisker"},
        {"异常点数", "header.outlier_count"},
        {"迭代次数", "header.iteration_count"},
        {"卡方", "header.chi_square"},
        {"分布", "header.distribution"},
        {"判定", "header.decision"},
        {"位置/形状", "header.location_shape"},
        {"尺度", "header.scale"},
        {"方法族", "header.method_family"},
        {"目标置信水平", "header.target_confidence"},
        {"覆盖率", "header.coverage"},
        {"调整后 H", "header.adjusted_h"},
        {"调整后 S", "header.adjusted_s"},
        {"区组数", "header.block_count"},
        {"处理数", "header.treatment_count"},
        {"截断", "header.truncated_flag"},
        {"N（可选）", "header.n_optional"},
        {"预测概率", "header.predicted_probability"},
        {"Pearson 残差", "header.pearson_residual"},
        {"Deviance 残差", "header.deviance_residual"},
        {"因子", "header.factor"},
        {"实际值", "header.actual_value"},
        {"序号", "header.sequence_index"},
        {"编码水平", "header.coded_level"},
        {"实际水平", "header.actual_level"},
        {"组合", "header.combination"},
        {"组中位数", "header.group_median"},
        {"CI 下限", "header.ci_lower"},
        {"CI 上限", "header.ci_upper"},
        {"第一组", "header.first_group"},
        {"第二组", "header.second_group"},
        {"N≤", "header.n_le"},
        {"N>", "header.n_gt"},
        {"不一致对数 b+c", "header.discordant_pairs"},
        {"有效配对数", "header.valid_pair_count"},
        {"分组", "plot.axis.grouping"},
        {"观测顺序", "plot.axis.observation_order"},
        {"成对比较", "plot.axis.pairwise_comparison"},
        {"参考值", "plot.axis.reference_value"},
        {"频数", "plot.axis.frequency"},
        {"零件", "plot.axis.part"},
        {"样本", "plot.axis.sample"},
        {"变量", "header.variable"},
        {"变量 1", "header.variable_1"},
        {"变量 2", "header.variable_2"},
        {"相关系数", "header.correlation"},
        {"置信区间", "header.confidence_interval"},
        {"置信水平", "header.confidence_level"},
        {"假设均值", "header.hypothesized_mean"},
        {"行百分比", "header.row_percent"},
        {"列百分比", "header.column_percent"},
        {"合计百分比", "header.total_percent"},
        {"均值差", "header.mean_difference"},
        {"SE 差值", "header.se_difference"},
        {"事件数", "header.event_count"},
        {"试验数", "header.trial_count"},
        {"比例", "header.proportion"},
        {"率", "header.rate"},
        {"观测长度", "header.exposure_length"},
        {"零件数", "header.part_count"},
        {"操作员数", "header.operator_count"},
        {"重复次数", "header.replicate_count"},
        {"第一组 N", "header.first_group_n"},
        {"第二组 N", "header.second_group_n"},
        {"秩和", "header.rank_sum"},
        {"位置差异", "header.location_difference"},
        {"连续性修正", "header.continuity_correction"},
        {"近似方法", "header.approximation_method"},
        {"小样本警告", "header.small_sample_warning"},
        {"效应量", "header.effect_size"},
        {"模型", "header.model"},
        {"最优模型", "header.best_model"},
        {"截距", "header.intercept"},
        {"系数/漂移", "header.coeff_drift"},
        {"预测期", "header.forecast_horizon"},
        {"分位数", "header.quantile"},
        {"T² 限", "header.t2_limit"},
        {"Q 限", "header.q_limit"},
        {"口径", "header.basis"},
        {"T² 异常", "header.t2_anomaly"},
        {"Q 残差", "header.q_residual"},
        {"Q 异常", "header.q_anomaly"},
        {"综合异常", "header.combined_anomaly"},
        {"超出决策限", "header.outside_decision_limit"},
        {"总体均值", "header.overall_mean"},
        {"组内 SD", "header.within_sd"},
        {"目标比例", "header.target_proportion"},
        {"迭代", "header.iterations"},
        {"收敛", "header.converged"},
        {"名称", "header.rule_name_col"},
        {"证据", "header.evidence"},
        {"关联行", "header.related_rows"},
        {"建议", "header.suggestion"},
        {"总体 D", "header.overall_desirability_d"},
        {"组数", "header.group_count"},
        {"检验", "header.test_name"},
        {"类别", "plot.axis.category"},
    };
    static const std::pair<const char*, const char*> status_ids[] = {
        {"已触发", "status.triggered"},
        {"未触发", "status.not_triggered"},
        {"不适用", "status.not_applicable"},
        {"未验证", "status.not_verified"},
        {"计算失败", "status.calculation_failed"},
        {"存在", "status.present"},
        {"缺失", "status.missing"},
        {"已截断", "status.truncated"},
        {"合计", "header.total"},
        {"组间", "status.anova_between"},
        {"误差", "status.anova_error"},
        {"回归", "status.anova_regression"},
        {"失拟", "status.anova_lack_of_fit"},
        {"纯误差", "status.anova_pure_error"},
        {"第一组", "header.first_group"},
        {"第二组", "header.second_group"},
        {"不一致对数 b+c", "header.discordant_pairs"},
        {"有效配对数", "header.valid_pair_count"},
        {"是", "status.yes"},
        {"否", "status.no"},
        {"上侧", "status.direction.upper"},
        {"下侧", "status.direction.lower"},
        {"无", "status.none"},
        {"未知特殊原因规则", "status.unknown_special_cause_rule"},
        {"规则策略", "metric.spc.rule_policy"},
        {"启用规则", "metric.spc.enabled_rules"},
        {"启用测试", "metric.spc.enabled_tests"},
        {"判定口径", "metric.spc.decision_basis"},
        {"有效子组数", "metric.spc.valid_subgroup_count"},
        {"子组数", "metric.spc.subgroup_count"},
        {"子组大小", "metric.spc.subgroup_size"},
        {"「单点超出 3σ 控制限」触发点数", "metric.spc.ooc_points_rule1"},
        {"Xbar「单点超出 3σ 控制限」触发点数", "metric.spc.xbar_ooc_points_rule1"},
        {"R「单点超出 3σ 控制限」触发点数", "metric.spc.r_ooc_points_rule1"},
        {"S「单点超出 3σ 控制限」触发点数", "metric.spc.s_ooc_points_rule1"},
        {"Xbar 启用测试", "metric.spc.xbar_enabled_tests"},
        {"I 图启用测试", "metric.spc.i_chart_enabled_tests"},
        {"适用性", "metric.spc.applicability"},
        {"参数来源", "metric.spc.parameter_source"},
        {"控制限倍数", "metric.spc.limit_sigma_multiple"},
        {"σ 估计方法", "metric.spc.sigma_estimation_method"},
        {"MR 图适用规则", "metric.spc.mr_applicable_rules"},
        {"MR/R/S 适用规则", "metric.spc.mr_rs_applicable_rules"},
        {"R 适用规则", "metric.spc.r_applicable_rules"},
        {"S 适用规则", "metric.spc.s_applicable_rules"},
        {"用户指定", "status.spc.policy_user_specified"},
        {"minitab_like（仅「单点超出 3σ 控制限」）", "status.spc.policy_minitab_like"},
        {"all_applicable（全部适用）", "status.spc.policy_all_applicable"},
        {"CUSUM 专用信号", "status.spc.cusum_dedicated_signal"},
        {"无（不套用 Tests 1–8）", "status.spc.no_shewhart_tests"},
        {"历史参数", "status.spc.historical_params"},
        {"估计", "header.estimate"},
        {"规格模式", "metric.capability.spec_mode"},
        {"Within σ 来源", "metric.capability.within_sigma_source"},
        {"Between σ 来源", "metric.capability.between_sigma_source"},
        {"Between/Within σ 来源", "metric.capability.bw_sigma_source"},
        {"Overall σ 方法", "metric.capability.overall_sigma_method"},
        {"AD 判定", "metric.capability.ad_decision"},
        {"无法计算", "status.unable_to_compute"},
        {"在 alpha 下拒绝正态假设", "status.ad_reject_normality"},
        {"在 alpha 下未拒绝正态假设", "status.ad_fail_reject_normality"},
        {"在 alpha 下拒绝", "status.ad_reject_at_alpha"},
        {"在 alpha 下未拒绝", "status.ad_fail_reject_at_alpha"},
        {"假设状态", "metric.capability.assumption_status"},
        {"低于 LSL", "metric.capability.ppm_below_lsl"},
        {"高于 USL", "metric.capability.ppm_above_usl"},
        {"合计", "header.total"},
        {"观测", "header.observation"},
        {"期望 Within", "header.expected_within"},
        {"期望 Overall", "header.expected_overall"},
        {"限方法", "metric.spc.limit_method"},
        {"超 UCL 点数", "metric.spc.points_above_ucl"},
        {"超限", "status.out_of_control"},
        {"目标 T", "metric.cusum.target_t"},
        {"上侧首次信号点", "metric.cusum.first_upper_signal"},
        {"下侧首次信号点", "metric.cusum.first_lower_signal"},
        {"信号总数", "metric.cusum.signal_count"},
        {"上侧/下侧累计和超过决策间隔 hσ 记为信号，原因待调查。",
         "status.cusum.decision_basis"},
        {"移动极差长度", "metric.spc.moving_range_length"},
        {"Jaehn 累计阈值", "metric.zone.jaehn_threshold"},
        {"Jaehn 信号点数", "metric.zone.jaehn_signal_count"},
        {"计分规则", "metric.zone.scoring_rule"},
        {"Jaehn 1/2/4 权重；不是完整 Western Electric Tests 1–8。",
         "status.zone.jaehn_scoring_value"},
        {"Z 图「单点超出 3σ 控制限」触发", "metric.zmr.z_rule1_triggers"},
        {"适用测试", "metric.spc.applicable_tests"},
        {"样本估计（未提供完整历史 μ/σ）", "status.zmr.sample_params"},
        {"历史 μ/σ", "status.zmr.historical_mu_sigma"},
        {"Z 图 Tests 1–4；MR 图无 Shewhart 测试。",
         "status.zmr.applicable_tests_value"},
        {"Nelson 剔除 MR 数", "metric.imr.nelson_excluded_mr"},
        {"历史 μ", "metric.spc.historical_mu"},
        {"历史 σ", "metric.spc.historical_sigma"},
        {"（未指定）", "status.not_specified_paren"},
        {"控制限参数来源", "metric.spc.limit_parameter_source"},
        {"历史参数优先", "status.spc.historical_params_preferred"},
        {"当前数据估计", "status.spc.estimated_from_current"},
        {"阶段数", "metric.spc.stage_count"},
        {"阶段列", "metric.spc.stage_column"},
        {"未指定", "status.not_specified"},
        {"分阶段估计仅作对照；全局控制限仍由历史参数或全样本估计决定，不自动按阶段切换限。",
         "status.spc.stage_estimate_note"},
        {"不合格品合计", "metric.attr.total_defectives"},
        {"缺陷合计", "metric.attr.total_defects"},
        {"检验数合计", "metric.attr.total_inspected"},
        {"单位数合计", "metric.attr.total_units"},
        {"目标不合格品率", "metric.attr.target_proportion_defective"},
        {"目标 DPU", "metric.attr.target_dpu"},
        {"位置是否漂移；散布是否大致恒定", "status.eda.run_sequence_hypothesis"},
        {"相邻观测是否呈结构（随机性）", "status.eda.lag1_hypothesis"},
        {"分布形态（是否近似钟形）", "status.eda.histogram_hypothesis"},
        {"正态分位是否近似直线", "status.eda.npp_hypothesis"},
        {"特殊原因规则 beyond_control_limit…eight_beyond_1sigma；超过 kσ 使用严格大于，窗口不跨阶段或缺失断点",
         "status.spc.attr_decision_basis"},
        {"EWMA 只开放「单点超出 3σ 控制限」；其余特殊原因规则不附加到 EWMA。",
         "status.spc.ewma_applicability"},
        {"单点超出 3σ 控制限、连续 9 点同侧、连续 6 点趋势、连续 14 点交替",
         "status.spc.mr_rule_list"},
        {"单点超出 3σ 控制限 / 连续 9 点同侧 / 连续 6 点趋势 / 连续 14 点交替（Minitab 不在 MR 图上启用后四条规则）",
         "status.spc.mr_rule_list_minitab_note"},
        {"经验分位（非 Minitab T² 控制图 UCL）",
         "status.pca_empirical_quantile_not_minitab"},
        {"(仅截距)", "status.intercept_only"},
        {"样本 1", "header.sample_1"},
        {"样本 2", "header.sample_2"},
        {"配对差值", "header.paired_difference"},
        {"参考样本", "header.reference_sample"},
        {"检验样本", "header.test_sample"},
        {"目标比例", "header.target_proportion"},
        {"样本", "plot.axis.sample"},
    };
    // EN fallback is skipped when multiple header IDs share the same en-US text
    // (notably table.value "值" and header.numeric_value "数值" both → "Value").
    std::unordered_map<std::string, int> en_header_counts;
    for (const auto& [zh, id] : header_ids) {
        (void)zh;
        ++en_header_counts[domain::resolve_report_text(id, "en-US", nullptr).text];
    }
    for (domain::StatisticTable& table : tables) {
        bool title_matched = false;
        for (const auto& [zh, id] : title_ids) {
            if (table.title == zh
                || table.title
                    == domain::resolve_report_text(id, "en-US", nullptr).text) {
                table.title = domain::resolve_report_text(id, language, missing_out).text;
                title_matched = true;
                break;
            }
        }
        if (!title_matched) {
            // Longer suffix first so " 方法与参数" wins over " 参数".
            static const std::pair<const char*, const char*> title_suffix_ids[] = {
                {" 方法与参数", "table.suffix.method_and_params"},
                {" 参数", "table.suffix.parameters"},
                {"逐子组统计", "table.suffix.subgroup_statistics"},
                {" 逐点统计", "table.suffix.point_statistics"},
            };
            for (const auto& [zh_suffix, suffix_id] : title_suffix_ids) {
                const std::string zh = zh_suffix;
                const std::string en_suffix =
                    domain::resolve_report_text(suffix_id, "en-US", nullptr).text;
                std::string base;
                if (table.title.size() > zh.size()
                    && table.title.compare(
                           table.title.size() - zh.size(), zh.size(), zh)
                        == 0) {
                    base = table.title.substr(0, table.title.size() - zh.size());
                } else if (table.title.size() > en_suffix.size()
                    && table.title.compare(
                           table.title.size() - en_suffix.size(), en_suffix.size(),
                           en_suffix)
                        == 0) {
                    base = table.title.substr(0, table.title.size() - en_suffix.size());
                } else {
                    continue;
                }
                const std::string original_base = base;
                localize_page_title(base, language, missing_out);
                if (base == original_base) {
                    // Also try table/plot chart titles already in title_ids.
                    for (const auto& [zh_base, id] : title_ids) {
                        if (original_base == zh_base
                            || original_base
                                == domain::resolve_report_text(id, "en-US", nullptr)
                                       .text) {
                            base = domain::resolve_report_text(id, language, missing_out)
                                       .text;
                            break;
                        }
                    }
                }
                if (base == original_base) {
                    break;
                }
                table.title = base
                    + domain::resolve_report_text(suffix_id, language, missing_out).text;
                title_matched = true;
                break;
            }
            (void)title_matched;
        }
        for (std::string& header : table.headers) {
            bool header_matched = false;
            for (const auto& [zh, id] : header_ids) {
                if (header == zh) {
                    header = domain::resolve_report_text(id, language, missing_out).text;
                    header_matched = true;
                    break;
                }
                const std::string en =
                    domain::resolve_report_text(id, "en-US", nullptr).text;
                if (header == en && en_header_counts[en] == 1) {
                    header = domain::resolve_report_text(id, language, missing_out).text;
                    header_matched = true;
                    break;
                }
            }
            if (header_matched) {
                continue;
            }
            // Multi-response DOE columns: "<response> 预测" / "<response> D".
            static const std::pair<const char*, const char*> header_suffix_ids[] = {
                {" 预测", "header.suffix.prediction"},
                {" D", "header.suffix.desirability_d"},
            };
            for (const auto& [zh_suffix, suffix_id] : header_suffix_ids) {
                const std::string zh = zh_suffix;
                const std::string en_sfx =
                    domain::resolve_report_text(suffix_id, "en-US", nullptr).text;
                std::string base;
                if (header.size() > zh.size()
                    && header.compare(header.size() - zh.size(), zh.size(), zh) == 0) {
                    base = header.substr(0, header.size() - zh.size());
                } else if (header.size() > en_sfx.size()
                           && header.compare(
                                  header.size() - en_sfx.size(), en_sfx.size(), en_sfx)
                               == 0) {
                    base = header.substr(0, header.size() - en_sfx.size());
                } else {
                    continue;
                }
                if (base.empty()) {
                    continue;
                }
                header = base
                    + domain::resolve_report_text(suffix_id, language, missing_out).text;
                break;
            }
        }
        for (std::vector<std::string>& row : table.rows) {
            for (std::string& cell : row) {
                bool matched = false;
                for (const auto& [zh, id] : status_ids) {
                    if (cell == zh
                        || cell
                            == domain::resolve_report_text(id, "en-US", nullptr).text) {
                        cell = domain::resolve_report_text(id, language, missing_out).text;
                        matched = true;
                        break;
                    }
                }
                if (matched) {
                    continue;
                }
                static const std::pair<const char*, const char*> cell_paren_tokens[] = {
                    {"（历史参数）", "param.summary.historical_paren"},
                    {"（估计）", "param.summary.estimated_paren"},
                };
                for (const auto& [zh, id] : cell_paren_tokens) {
                    const std::string zh_token = zh;
                    const auto pos = cell.find(zh_token);
                    if (pos == std::string::npos) {
                        continue;
                    }
                    cell.replace(
                        pos,
                        zh_token.size(),
                        domain::resolve_report_text(id, language, missing_out).text);
                    matched = true;
                    break;
                }
                if (matched) {
                    continue;
                }
                {
                    const std::string known =
                        localize_known_plain_message(cell, language, missing_out);
                    if (known != cell) {
                        cell = known;
                        continue;
                    }
                }
                {
                    const std::string stage_suffix_zh = "（N / 均值 / σ̂_MR）";
                    if (starts_with(cell, "阶段 ")
                        && ends_with(cell, stage_suffix_zh)
                        && cell.size() > std::string("阶段 ").size() + stage_suffix_zh.size()) {
                        const std::string label = cell.substr(
                            std::string("阶段 ").size(),
                            cell.size() - std::string("阶段 ").size()
                                - stage_suffix_zh.size());
                        cell = replace_token(
                            domain::resolve_report_text(
                                "metric.spc.stage_row_n_mean_sigma",
                                language,
                                missing_out)
                                .text,
                            "%1",
                            label);
                        continue;
                    }
                }
                const std::string localized_name =
                    localize_spc_rule_name(cell, language, missing_out);
                if (localized_name != cell) {
                    cell = localized_name;
                    continue;
                }
                const std::string localized_action =
                    localize_spc_rule_action(cell, language, missing_out);
                if (localized_action != cell) {
                    cell = localized_action;
                    continue;
                }
                const std::string localized_window =
                    localize_spc_rule_window(cell, language, missing_out);
                if (localized_window != cell) {
                    cell = localized_window;
                    continue;
                }
                const std::string localized_threshold =
                    localize_spc_rule_threshold(cell, language, missing_out);
                if (localized_threshold != cell) {
                    cell = localized_threshold;
                    continue;
                }
                const std::string localized_comparison =
                    localize_spc_rule_comparison(cell, language, missing_out);
                if (localized_comparison != cell) {
                    cell = localized_comparison;
                    continue;
                }
                const std::string localized_reason =
                    localize_spc_rule_reason(cell, language, missing_out);
                if (localized_reason != cell) {
                    cell = localized_reason;
                    continue;
                }
                const std::string localized_message =
                    localize_spc_rule_message(cell, language, missing_out);
                if (localized_message != cell) {
                    cell = localized_message;
                }
            }
        }
    }
}

void localize_graph_caption_tokens(
    std::string& text,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    // Longer tokens first so "分析 N(水平) =" is not partially matched by "分析 N =".
    static const std::pair<const char*, const char*> tokens[] = {
        {"分析 N(水平) = ", "graph.caption.analysis_n_level"},
        {"显示 N = ", "graph.caption.display_n"},
        {"分析 N = ", "graph.caption.analysis_n"},
        {"分面 = ", "graph.caption.facet"},
        {"相关矩阵热图单元格不是观测层；不伪造 per-cell member_source_rows",
         "param.summary.corr_heatmap_not_obs_layer"},
        {"相关矩阵单元格不是观测层", "param.summary.corr_cell_not_obs_layer"},
    };
    // Production panel captions use lowercase English "facet = " (not catalog "Facet = ").
    static const std::pair<const char*, const char*> alias_tokens[] = {
        {"facet = ", "graph.caption.facet"},
    };
    const auto replace_all = [](std::string& haystack,
                                const std::string& from,
                                const std::string& to) {
        if (from.empty()) {
            return;
        }
        std::size_t pos = 0;
        while ((pos = haystack.find(from, pos)) != std::string::npos) {
            haystack.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    for (const auto& [zh, id] : tokens) {
        const std::string en =
            domain::resolve_report_text(id, "en-US", nullptr).text;
        const std::string localized =
            domain::resolve_report_text(id, language, missing_out).text;
        replace_all(text, zh, localized);
        replace_all(text, en, localized);
    }
    for (const auto& [alias, id] : alias_tokens) {
        const std::string localized =
            domain::resolve_report_text(id, language, missing_out).text;
        replace_all(text, alias, localized);
    }
}

void localize_visible_plots(
    std::vector<domain::PlotSpec>& plots,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    static const std::pair<const char*, const char*> title_ids[] = {
        {"Box-Cox λ 选择诊断", "plot.box_cox_lambda"},
        {"Johnson 变换", "plot.johnson_transform"},
        {"过程能力直方图", "plot.capability_histogram"},
        {"残差与拟合值", "plot.residual_vs_fitted"},
        {"残差与观测顺序", "plot.residual_vs_order"},
        {"残差正态概率图", "plot.residual_normal_probability"},
        {"残差直方图", "plot.residual_histogram"},
        {"正态概率图", "plot.normal_probability"},
        {"变换后正态概率图", "plot.transformed_normal_probability"},
        {"箱线图", "plot.boxplot"},
        {"个体值图", "plot.individual_value"},
        {"区间图", "plot.interval_plot"},
        {"I-MR 控制图", "plot.imr_chart"},
        {"Xbar-R 控制图", "plot.xbar_r_chart"},
        {"Xbar-S 控制图", "plot.xbar_s_chart"},
        {"EWMA 控制图", "plot.ewma_chart"},
        {"CUSUM 控制图", "plot.cusum_chart"},
        {"Z-MR 控制图", "plot.z_mr_chart"},
        {"P 图", "plot.p_chart"},
        {"NP 图", "plot.np_chart"},
        {"C 图", "plot.c_chart"},
        {"U 图", "plot.u_chart"},
        {"累计 %Defective", "plot.cumulative_percent_defective"},
        {"累计 DPU", "plot.cumulative_dpu"},
        {"单值图 (I)", "plot.individuals_i"},
        {"移动极差图 (MR)", "plot.moving_range_mr"},
        {"I 图", "plot.i_chart"},
        {"MR 图", "plot.mr_chart"},
        {"R 图", "plot.r_chart"},
        {"S 图", "plot.s_chart"},
        {"Xbar 图", "plot.xbar_chart"},
        {"I 图（子组均值）", "plot.i_chart_subgroup_mean"},
        {"G 图", "plot.g_chart"},
        {"G 图（几何间隔）", "plot.g_chart_geometric"},
        {"T 图", "plot.t_chart"},
        {"T 图（时间间隔）", "plot.t_chart_time"},
        {"拟合线图", "plot.fitted_line"},
        {"交互均值图", "plot.interaction_means"},
        {"矩阵散点图", "plot.matrix_scatter"},
        {"直方图", "plot.histogram"},
        {"运行图", "plot.run_chart"},
        {"区域图", "plot.zone_chart"},
        {"柏拉图", "plot.pareto"},
        {"配对测量散点图", "plot.paired_measurement_scatter"},
        {"观察频数热图", "plot.observed_count_heatmap"},
        {"调整残差热图", "plot.adjusted_residual_heatmap"},
        {"Gage Run Chart", "plot.gage_run_chart"},
        {"等价性区间", "plot.equivalence_interval"},
        {"观察与期望", "plot.observed_expected"},
        {"Hotelling T² 图", "plot.hotelling_t2"},
        {"MEWMA T² 图", "plot.mewma_t2"},
        {"Isolation 分数", "plot.isolation_score"},
        {"Gage Stability Run Chart", "plot.gage_stability_run"},
        {"Bias versus Reference", "plot.bias_versus_reference"},
        {"立方图", "plot.doe_cube"},
        {"立方图（方形）", "plot.doe_cube_square"},
        {"效应 Pareto", "plot.effects_pareto"},
        {"标准化效应 Pareto", "plot.standardized_effects_pareto"},
        {"方差分量 %Contribution", "plot.msa_components_contribution"},
        {"方差分量 %Study Var", "plot.msa_components_study_var"},
        {"方差分量 %Tolerance", "plot.msa_components_tolerance"},
        {"按零件", "plot.msa_by_part"},
        {"按零件 Xbar", "plot.msa_by_part_xbar"},
        {"按零件 R", "plot.msa_by_part_r"},
        {"按操作者 Xbar", "plot.msa_by_operator_xbar"},
        {"按操作者 R", "plot.msa_by_operator_r"},
        {"上侧 CUSUM", "plot.cusum_upper"},
        {"下侧 CUSUM", "plot.cusum_lower"},
        {"操作者×零件交互", "plot.msa_operator_part_interaction"},
        {"Type 1 Gage 直方图", "plot.type1_gage_histogram"},
        {"Laney P' 图", "plot.laney_p_chart"},
        {"Laney U' 图", "plot.laney_u_chart"},
        {"最后 25 个子组", "plot.sixpack_last_25_subgroups"},
        {"最近 25 个观测", "plot.sixpack_last_25_observations"},
        {"变换前正态概率图", "plot.normal_probability_before_transform"},
        {"Kaplan-Meier 生存曲线", "plot.km_survival_curve"},
        {"Turnbull 生存曲线", "plot.turnbull_survival_curve"},
        {"散点图", "page.scatter"},
        {"区间散点图", "page.interval_scatter"},
        {"相关图", "page.correlogram"},
        {"气泡图", "page.bubble"},
        {"经验累积分布图", "page.ecdf"},
        {"矩阵图", "page.matrix_plot"},
        {"边际图", "page.marginal_plot"},
        {"平行坐标图", "page.parallel_coordinates"},
        {"热图", "page.heatmap"},
        {"时间序列图", "page.time_series"},
        {"等值线图", "page.contour"},
        {"饼图", "page.pie"},
        {"密度图", "page.density"},
        {"Hexbin / 二维分箱", "page.hexbin"},
        {"Hexbin", "page.hexbin_short"},
        {"二维分箱", "page.hexbin_2d"},
        {"二维分箱散点", "page.hexbin_2d_scatter"},
        {"小提琴图", "page.violin"},
        {"条形图", "page.bar"},
        {"Multi-Vari 图", "page.multi_vari"},
        {"变异性图", "page.variability"},
        {"因果图", "page.cause_effect"},
        {"正态概率图", "plot.normal_probability"},
        {"候选组合总体 Desirability", "plot.doe_overall_desirability"},
        {"均值与极差", "plot.mean_and_range"},
        {"标准差图", "plot.stdev_chart"},
        {"能力图", "plot.capability_chart"},
        {"区域累计得分", "plot.zone_cumulative_score"},
        {"广义方差 |S| 图", "plot.gv_s_chart"},
        {"面积区域图", "page.area_plot"},
        {"各处理阳性率", "table.treatment_positive_rates"},
        {"评估者×零件一致率", "table.aa_evaluator_part_agreement"},
        {"评估者一致率", "table.aa_evaluator_agreement_rate"},
        {"实际值、拟合值与预测区间", "plot.actual_fitted_forecast_interval"},
        {"ARIMA 拟合与预测", "plot.arima_fit_forecast"},
        {"时间序列分解拟合", "plot.decompose_fit"},
        {"季节性拟合与预测", "plot.seasonal_fit_forecast"},
        {"PCA 得分图", "plot.pca_scores"},
        {"K-Means 散点（前两列）", "plot.kmeans_scatter"},
        {"功效曲线", "plot.power_curve"},
        {"OC 曲线（二项）", "plot.oc_curve_binomial"},
        {"观测 vs 拟合", "plot.observed_vs_fitted"},
        {"序列", "plot.series_index"},
        {"拟合 vs Pearson 残差", "plot.fitted_vs_pearson"},
        {"Bootstrap 均值分布", "plot.bootstrap_mean_dist"},
        {"层次聚类散点（前两列）", "plot.hclust_scatter_first_two"},
        {"LD 投影（示意）", "plot.lda_projection"},
        {"相关热图", "plot.correlation_heatmap"},
        {"差值置信区间", "plot.diff_confidence_interval"},
        {"率比置信区间", "plot.rate_ratio_confidence_interval"},
        {"预测概率", "plot.predicted_probability"},
        {"容差区间直方图", "plot.tolerance_interval_histogram"},
    };
    // Longer dynamic prefixes first so RSM static surface wins over response surface.
    static const std::pair<const char*, const char*> prefix_ids[] = {
        {"静态响应曲面图（非可旋转 3D）- ", "plot.rsm_static_surface"},
        {"响应曲面图 - ", "plot.response_surface"},
        {"等值线图 - ", "plot.contour"},
        {"交互作用图 - ", "plot.doe_interaction"},
        {"主效应图 - ", "plot.doe_main_effects"},
        {"残差与预测变量 - ", "plot.residual_vs_predictor"},
    };
    // Graph Builder faceted panel titles use middle-dot (·), not " - ".
    // Longer bases first so "区间散点图 · " wins over "散点图 · ".
    static const std::pair<const char*, const char*> panel_prefix_ids[] = {
        {"经验累积分布图 · ", "page.ecdf"},
        {"区间散点图 · ", "page.interval_scatter"},
        {"平行坐标图 · ", "page.parallel_coordinates"},
        {"时间序列图 · ", "page.time_series"},
        {"正态概率图 · ", "plot.normal_probability"},
        {"二维分箱 · ", "page.hexbin_2d"},
        {"小提琴图 · ", "page.violin"},
        {"等值线图 · ", "page.contour"},
        {"散点图 · ", "page.scatter"},
        {"相关图 · ", "page.correlogram"},
        {"气泡图 · ", "page.bubble"},
        {"矩阵图 · ", "page.matrix_plot"},
        {"边际图 · ", "page.marginal_plot"},
        {"热图 · ", "page.heatmap"},
        {"密度图 · ", "page.density"},
        {"条形图 · ", "page.bar"},
        {"区域图 · ", "page.area_plot"},
    };
    // Longer model labels first so "三参数 Weibull" wins over bare "Weibull".
    static const std::pair<const char*, const char*> model_ids[] = {
        {"三参数 Weibull", "model.weibull3"},
        {"三参数对数正态", "model.lognormal3"},
        {"两参数指数", "model.exponential2"},
    };
    // Longer chart suffixes first so " 的正态概率图" wins over " 概率图".
    static const std::pair<const char*, const char*> suffix_ids[] = {
        {" 的正态概率图", "plot.suffix.normal_probability_of"},
        {" 的个体值图", "plot.suffix.individual_value_of"},
        {" 的直方图", "plot.suffix.histogram_of"},
        {" 的散点图", "plot.suffix.scatter_of"},
        {" 的 Pareto 图", "plot.suffix.pareto_of"},
        {" 游程序列图", "plot.suffix.runs_series"},
        {" 运行图", "plot.suffix.run_chart"},
        {" 生存曲线", "plot.suffix.survival_curve"},
        {" 概率图", "plot.suffix.probability_plot"},
        {" ANOM 图", "plot.suffix.anom_chart"},
        {" — 类别原因计数", "plot.suffix.category_cause_counts"},
    };
    static const std::pair<const char*, const char*> axis_ids[] = {
        {"时间", "plot.axis.time"},
        {"测量值", "plot.axis.measurement"},
        {"子组均值", "plot.axis.subgroup_mean"},
        {"移动极差", "plot.axis.moving_range"},
        {"子组极差", "plot.axis.subgroup_range"},
        {"子组标准差", "plot.axis.subgroup_stdev"},
        {"不合格品率", "plot.axis.proportion_defective"},
        {"不合格品数", "plot.axis.number_defective"},
        {"缺陷数", "plot.axis.defect_count"},
        {"单位缺陷数", "plot.axis.defects_per_unit"},
        {"不合格品率 p", "plot.axis.proportion_defective_p"},
        {"因子 A", "header.factor_a"},
        {"因子 B", "header.factor_b"},
        {"残差", "plot.axis.residual"},
        {"拟合值", "header.fitted"},
        {"观测顺序", "plot.axis.observation_order"},
        {"理论分位数", "plot.axis.theoretical_quantile"},
        {"频数", "plot.axis.frequency"},
        {"来源", "header.source"},
        {"零件", "plot.axis.part"},
        {"单元格均值", "plot.axis.cell_mean"},
        {"样本", "plot.axis.sample"},
        {"分组", "plot.axis.grouping"},
        {"观测", "header.observation"},
        {"观测序号", "header.observation_index"},
        {"理论标准正态分位数", "plot.axis.theoretical_standard_normal_quantile"},
        {"标准正态分位数", "plot.axis.standard_normal_quantile"},
        {"变换后测量", "plot.axis.transformed_measurement"},
        {"观测序", "plot.axis.obs_order_abbr"},
        {"成对比较", "plot.axis.pairwise_comparison"},
        {"均值差", "header.mean_difference"},
        {"参考值", "plot.axis.reference_value"},
        {"阳性率", "plot.axis.positive_rate"},
        {"评估者", "plot.axis.evaluator"},
        {"处理", "header.treatment"},
        {"接收概率 Pa(p)", "plot.axis.accept_probability_pa"},
        {"拟合", "plot.axis.fitted_short"},
        {"变量", "header.variable"},
        {"率比", "plot.axis.rate_ratio"},
        {"事件概率", "plot.axis.event_probability"},
        {"类别", "plot.axis.category"},
        {"类别序号", "plot.axis.category_index"},
        {"标准化变换 SD", "plot.axis.standardized_transform_sd"},
        {"变换值", "plot.axis.transformed_value"},
        {"候选排序", "plot.axis.candidate_rank"},
        {"测量序号", "plot.axis.measurement_index"},
        {"原因数", "plot.axis.cause_count"},
        {"|效应|", "plot.axis.abs_effect"},
        {"标准化值", "plot.axis.standardized_value"},
        {"组成比例", "plot.axis.composition_proportion"},
        {"密度", "plot.axis.density"},
        {"比例差", "plot.axis.proportion_difference"},
        {"比值", "header.ratio"},
        {"差值", "header.difference"},
        {"累计比例", "plot.axis.cumulative_proportion"},
        {"因子 1", "plot.axis.factor_1"},
    };
    static const std::pair<const char*, const char*> series_ids[] = {
        {"拟合 S(t)", "plot.series.fitted_st"},
        {"观测偏倚", "plot.series.observed_bias"},
        {"残差 = 0", "plot.series.residual_zero"},
        {"差 = 0", "plot.series.difference_zero"},
        {"棱", "plot.series.cube_edge"},
        {"预测值", "plot.series.forecast"},
        {"异常点", "plot.series.outliers"},
        {"拟合线", "plot.series.fitted_line"},
        {"选定 λ", "plot.series.selected_lambda"},
        {"嫌疑观测", "plot.series.suspect_observation"},
        {"嫌疑点", "plot.series.suspect_point"},
        {"零件均值", "plot.series.part_mean"},
        {"预测区间", "plot.series.prediction_interval"},
        {"趋势", "plot.series.trend"},
        {"功效", "plot.series.power"},
        {"实际值", "header.actual_value"},
        {"拟合值", "header.fitted"},
        {"残差", "header.residual"},
        {"总体均值", "header.overall_mean"},
        {"阈值", "header.threshold"},
        {"上置信限", "plot.series.upper_confidence_limit"},
        {"下置信限", "plot.series.lower_confidence_limit"},
        {"等价下限", "plot.series.equivalence_lower"},
        {"等价上限", "plot.series.equivalence_upper"},
    };
    const auto localize_exact_field =
        [&](std::string& field, const std::pair<const char*, const char*>* begin,
            const std::pair<const char*, const char*>* end) {
            for (const auto* it = begin; it != end; ++it) {
                const char* zh = it->first;
                const char* id = it->second;
                if (field == zh
                    || field == domain::resolve_report_text(id, "en-US", nullptr).text) {
                    field = domain::resolve_report_text(id, language, missing_out).text;
                    return true;
                }
            }
            return false;
        };
    for (domain::PlotSpec& plot : plots) {
        bool matched = false;
        for (const auto& [zh, id] : title_ids) {
            if (plot.title == zh
                || plot.title
                    == domain::resolve_report_text(id, "en-US", nullptr).text) {
                plot.title = domain::resolve_report_text(id, language, missing_out).text;
                matched = true;
                break;
            }
        }
        // Legacy English KM title from older builds.
        if (!matched
            && (plot.title == "Kaplan-Meier Survival Curve"
                || plot.title == "Kaplan-Meier survival curve")) {
            plot.title = domain::resolve_report_text(
                             "plot.km_survival_curve", language, missing_out)
                             .text;
            matched = true;
        }
        if (!matched) {
            for (const auto& [zh_prefix, id] : prefix_ids) {
                const std::string zh = zh_prefix;
                const std::string en_base =
                    domain::resolve_report_text(id, "en-US", nullptr).text;
                const std::string en_prefix = en_base + " - ";
                std::string suffix;
                if (plot.title.rfind(zh, 0) == 0) {
                    suffix = plot.title.substr(zh.size());
                } else if (plot.title.rfind(en_prefix, 0) == 0) {
                    suffix = plot.title.substr(en_prefix.size());
                } else {
                    continue;
                }
                const std::string localized_base =
                    domain::resolve_report_text(id, language, missing_out).text;
                plot.title = localized_base + " - " + suffix;
                matched = true;
                break;
            }
        }
        if (!matched) {
            for (const auto& [zh_prefix, id] : panel_prefix_ids) {
                const std::string zh = zh_prefix;
                const std::string en_base =
                    domain::resolve_report_text(id, "en-US", nullptr).text;
                const std::string en_prefix = en_base + " · ";
                std::string suffix;
                if (plot.title.rfind(zh, 0) == 0) {
                    suffix = plot.title.substr(zh.size());
                } else if (plot.title.rfind(en_prefix, 0) == 0) {
                    suffix = plot.title.substr(en_prefix.size());
                } else {
                    continue;
                }
                const std::string localized_base =
                    domain::resolve_report_text(id, language, missing_out).text;
                plot.title = localized_base + " · " + suffix;
                matched = true;
                break;
            }
        }
        if (!matched) {
            for (const auto& [zh_suffix, suffix_id] : suffix_ids) {
                const std::string zh_sfx = zh_suffix;
                const std::string en_sfx =
                    " " + domain::resolve_report_text(suffix_id, "en-US", nullptr).text;
                std::string model_part;
                if (plot.title.size() > zh_sfx.size()
                    && plot.title.compare(
                           plot.title.size() - zh_sfx.size(), zh_sfx.size(), zh_sfx)
                        == 0) {
                    model_part = plot.title.substr(0, plot.title.size() - zh_sfx.size());
                } else if (plot.title.size() > en_sfx.size()
                           && plot.title.compare(
                                  plot.title.size() - en_sfx.size(), en_sfx.size(), en_sfx)
                               == 0) {
                    model_part = plot.title.substr(0, plot.title.size() - en_sfx.size());
                } else {
                    continue;
                }
                std::string localized_model = model_part;
                bool model_ok = false;
                for (const auto& [zh_model, model_id] : model_ids) {
                    if (model_part == zh_model
                        || model_part
                            == domain::resolve_report_text(model_id, "en-US", nullptr)
                                   .text) {
                        localized_model = domain::resolve_report_text(
                                              model_id, language, missing_out)
                                              .text;
                        model_ok = true;
                        break;
                    }
                }
                // Survival/probability require a known model label; analysis chart
                // suffixes keep the response/variable/column name and only localize
                // the chart suffix (and optional " 与 " infix between two names).
                if (!model_ok) {
                    const std::string sid = suffix_id;
                    if ((sid != "plot.suffix.anom_chart"
                         && sid != "plot.suffix.pareto_of"
                         && sid != "plot.suffix.runs_series"
                         && sid != "plot.suffix.category_cause_counts"
                         && sid != "plot.suffix.normal_probability_of"
                         && sid != "plot.suffix.histogram_of"
                         && sid != "plot.suffix.individual_value_of"
                         && sid != "plot.suffix.scatter_of"
                         && sid != "plot.suffix.run_chart")
                        || model_part.empty()) {
                        break;
                    }
                }
                {
                    const std::string and_zh = " 与 ";
                    const std::string and_en =
                        domain::resolve_report_text("plot.infix.and", "en-US", nullptr)
                            .text;
                    const std::string and_loc =
                        domain::resolve_report_text(
                            "plot.infix.and", language, missing_out)
                            .text;
                    if (and_zh != and_loc) {
                        replace_all_inplace(localized_model, and_zh, and_loc);
                    }
                    if (and_en != and_loc) {
                        replace_all_inplace(localized_model, and_en, and_loc);
                    }
                }
                const std::string localized_suffix =
                    domain::resolve_report_text(suffix_id, language, missing_out).text;
                plot.title = localized_model + " " + localized_suffix;
                matched = true;
                break;
            }
        }
        if (!matched) {
            const std::string facet_zh = "（分面）";
            const std::string facet_en =
                domain::resolve_report_text("page.suffix.faceted", "en-US", nullptr).text;
            std::string base;
            if (plot.title.size() > facet_zh.size()
                && plot.title.compare(
                       plot.title.size() - facet_zh.size(), facet_zh.size(), facet_zh)
                    == 0) {
                base = plot.title.substr(0, plot.title.size() - facet_zh.size());
            } else if (plot.title.size() > facet_en.size()
                       && plot.title.compare(
                              plot.title.size() - facet_en.size(), facet_en.size(), facet_en)
                           == 0) {
                base = plot.title.substr(0, plot.title.size() - facet_en.size());
            }
            if (!base.empty()) {
                for (const auto& [zh, id] : title_ids) {
                    if (base == zh
                        || base == domain::resolve_report_text(id, "en-US", nullptr).text) {
                        plot.title =
                            domain::resolve_report_text(id, language, missing_out).text
                            + domain::resolve_report_text(
                                  "page.suffix.faceted", language, missing_out)
                                  .text;
                        matched = true;
                        break;
                    }
                }
            }
        }
        if (!matched) {
            const std::string panel_sep = " · ";
            const auto sep_pos = plot.title.find(panel_sep);
            if (sep_pos != std::string::npos && sep_pos > 0) {
                const std::string base = plot.title.substr(0, sep_pos);
                const std::string rest = plot.title.substr(sep_pos);
                for (const auto& [zh, id] : title_ids) {
                    if (base == zh
                        || base == domain::resolve_report_text(id, "en-US", nullptr).text) {
                        plot.title =
                            domain::resolve_report_text(id, language, missing_out).text
                            + rest;
                        matched = true;
                        break;
                    }
                }
            }
        }
        localize_exact_field(
            plot.x_axis_title, axis_ids, axis_ids + std::size(axis_ids));
        localize_exact_field(
            plot.y_axis_title, axis_ids, axis_ids + std::size(axis_ids));
        const auto localize_coded_axis_suffix = [&](std::string& field) {
            const std::string zh_sfx = "（编码）";
            const std::string en_sfx =
                domain::resolve_report_text(
                    "plot.axis.suffix.coded", "en-US", nullptr)
                    .text;
            const std::string localized_sfx =
                domain::resolve_report_text(
                    "plot.axis.suffix.coded", language, missing_out)
                    .text;
            if (ends_with(field, zh_sfx) || ends_with(field, en_sfx)) {
                const std::string sfx = ends_with(field, zh_sfx) ? zh_sfx : en_sfx;
                std::string base = field.substr(0, field.size() - sfx.size());
                localize_exact_field(
                    base, axis_ids, axis_ids + std::size(axis_ids));
                field = base + localized_sfx;
            }
        };
        localize_coded_axis_suffix(plot.x_axis_title);
        localize_coded_axis_suffix(plot.y_axis_title);
        {
            const std::string grouped_open = "（按 ";
            const std::string grouped_close = " 分组）";
            const std::string grouped_open_en =
                domain::resolve_report_text(
                    "plot.axis.suffix.grouped_by_open", "en-US", nullptr)
                    .text;
            const std::string grouped_close_en =
                domain::resolve_report_text(
                    "plot.axis.suffix.grouped_by_close", "en-US", nullptr)
                    .text;
            const auto localize_grouped_axis = [&](std::string& field) {
                const std::size_t open_pos = field.find(grouped_open);
                if (open_pos == std::string::npos) {
                    const std::size_t open_pos_en = field.find(grouped_open_en);
                    if (open_pos_en == std::string::npos
                        || !ends_with(field, grouped_close_en)) {
                        return;
                    }
                    std::string base = field.substr(0, open_pos_en);
                    const std::string grouped_name = field.substr(
                        open_pos_en + grouped_open_en.size(),
                        field.size() - (open_pos_en + grouped_open_en.size())
                            - grouped_close_en.size());
                    localize_exact_field(
                        base, axis_ids, axis_ids + std::size(axis_ids));
                    field = base
                        + domain::resolve_report_text(
                              "plot.axis.suffix.grouped_by_open", language, missing_out)
                              .text
                        + grouped_name
                        + domain::resolve_report_text(
                              "plot.axis.suffix.grouped_by_close", language, missing_out)
                              .text;
                    return;
                }
                if (!ends_with(field, grouped_close)) {
                    return;
                }
                std::string base = field.substr(0, open_pos);
                const std::string grouped_name = field.substr(
                    open_pos + grouped_open.size(),
                    field.size() - (open_pos + grouped_open.size())
                        - grouped_close.size());
                localize_exact_field(
                    base, axis_ids, axis_ids + std::size(axis_ids));
                field = base
                    + domain::resolve_report_text(
                          "plot.axis.suffix.grouped_by_open", language, missing_out)
                          .text
                    + grouped_name
                    + domain::resolve_report_text(
                          "plot.axis.suffix.grouped_by_close", language, missing_out)
                          .text;
            };
            localize_grouped_axis(plot.x_axis_title);
            localize_grouped_axis(plot.y_axis_title);
        }
        for (domain::PlotSeries& series : plot.series) {
            localize_exact_field(
                series.label, series_ids, series_ids + std::size(series_ids));
        }
        for (std::string& label : plot.point_labels) {
            localize_exact_field(
                label, series_ids, series_ids + std::size(series_ids));
        }
        localize_graph_caption_tokens(plot.title, language, missing_out);
        localize_graph_caption_tokens(plot.subtitle, language, missing_out);
    }
}

void localize_page_title(
    std::string& title,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    static const std::pair<const char*, const char*> title_ids[] = {
        {"正态过程能力分析", "page.capability_normal"},
        {"Johnson 变换过程能力", "page.capability_johnson"},
        {"非正态过程能力", "page.capability_nonnormal"},
        {"组间/组内过程能力", "page.capability_between_within"},
        {"二项过程能力", "page.attribute_capability_binomial"},
        {"泊松过程能力", "page.attribute_capability_poisson"},
        {"DOE 响应分析", "page.doe_response_analysis"},
        {"DOE 响应优化", "page.doe_response_optimization"},
        {"响应曲面分析", "page.rsm_analysis"},
        {"Plackett–Burman 设计", "page.plackett_burman_design"},
        {"2 水平全因子设计", "page.factorial_full_2level"},
        {"2 水平部分析因设计", "page.factorial_fractional_2level"},
        {"Box–Behnken 设计", "page.bbd_design"},
        {"中心复合设计 (CCD)", "page.ccd_design"},
        {"可靠性分析", "page.reliability_analysis"},
        {"区间删失 Kaplan–Meier（Turnbull）", "page.km_interval_turnbull"},
        {"区间删失 KM", "page.km_interval_short"},
        {"散点图", "page.scatter"},
        {"区间散点图", "page.interval_scatter"},
        {"相关图", "page.correlogram"},
        {"气泡图", "page.bubble"},
        {"经验累积分布图", "page.ecdf"},
        {"矩阵图", "page.matrix_plot"},
        {"边际图", "page.marginal_plot"},
        {"平行坐标图", "page.parallel_coordinates"},
        {"热图", "page.heatmap"},
        {"时间序列图", "page.time_series"},
        {"等值线图", "page.contour"},
        {"饼图", "page.pie"},
        {"密度图", "page.density"},
        {"Hexbin / 二维分箱", "page.hexbin"},
        {"Hexbin", "page.hexbin_short"},
        {"二维分箱", "page.hexbin_2d"},
        {"二维分箱散点", "page.hexbin_2d_scatter"},
        {"小提琴图", "page.violin"},
        {"条形图", "page.bar"},
        {"Multi-Vari 图", "page.multi_vari"},
        {"变异性图", "page.variability"},
        {"因果图", "page.cause_effect"},
        {"正态概率图", "plot.normal_probability"},
        {"Pearson 相关", "page.pearson_correlation"},
        {"Spearman 秩相关", "page.spearman_correlation"},
        {"单比例检验", "page.one_proportion"},
        {"两比例检验", "page.two_proportions"},
        {"单样本泊松率", "page.one_poisson_rate"},
        {"双样本泊松率", "page.two_poisson_rates"},
        {"列联表卡方", "page.chi_square_contingency"},
        {"交叉表", "page.crosstab"},
        {"卡方拟合优度", "page.chi_square_gof"},
        {"泊松拟合优度", "page.poisson_gof"},
        {"Box-Cox 变换", "page.box_cox"},
        {"双因素 ANOVA", "page.twoway_anova"},
        {"方差检验", "page.variance_test"},
        {"正态容差区间", "page.tolerance_normal"},
        {"非参数容差区间", "page.tolerance_nonparametric"},
        {"T 功效与样本量", "page.t_power_sample_size"},
        {"个体分布识别", "page.distribution_id"},
        {"均值分析 (ANOM)", "page.anom"},
        {"属性一次抽样", "page.attribute_sampling"},
        {"属性一致性分析", "page.attribute_agreement"},
        {"Mann-Whitney 检验", "page.mann_whitney"},
        {"Wilcoxon 符号秩检验", "page.wilcoxon_signed_rank"},
        {"符号检验", "page.sign_test"},
        {"游程检验", "page.runs_test"},
        {"Fisher 精确检验", "page.fisher_exact"},
        {"McNemar 检验", "page.mcnemar"},
        {"Cochran Q 检验", "page.cochran_q"},
        {"Mood 中位数检验", "page.mood_median"},
        {"Kruskal-Wallis 检验", "page.kruskal_wallis"},
        {"Friedman 检验", "page.friedman"},
        {"二元 Logistic 回归", "page.binary_logistic"},
        {"有序 Logistic 回归", "page.ordinal_logistic"},
        {"主成分分析", "page.pca"},
        {"K-Means 聚类", "page.kmeans"},
        {"CART 单树", "page.cart"},
        {"ADF 单位根检验", "page.adf"},
        {"Poisson 回归", "page.poisson_regression"},
        {"层次聚类（观测）", "page.hierarchical_clustering"},
        {"线性判别分析", "page.lda"},
        {"逐步回归", "page.stepwise_regression"},
        {"Best Subsets 回归", "page.best_subsets_regression"},
        {"批次过程能力", "page.batch_capability"},
        {"自相关 / 偏自相关", "page.acf_pacf"},
        {"互相关（CCF）", "page.ccf"},
        {"Correlogram（相关热图）", "page.correlogram_heatmap"},
        {"时间序列平滑", "page.time_series_smooth"},
        {"ARIMA 基础预测", "page.arima_forecast"},
        {"季节性预测", "page.seasonal_forecast"},
        {"时间序列分解", "page.time_series_decompose"},
        {"Bootstrap 均值置信区间", "page.bootstrap_mean_ci"},
        {"EDA 四图", "page.eda_four_plot"},
        {"I-MR-R/S 控制图", "page.imr_rs_chart"},
        {"广义方差图", "page.generalized_variance"},
        {"Hotelling T²", "page.hotelling_t2"},
        {"MEWMA", "page.mewma"},
        {"Isolation Forest", "page.isolation_forest"},
        {"Crossed Gage R&R", "page.crossed_gage_rr"},
        {"Nested Gage R&R", "page.nested_gage_rr"},
        {"Expanded Gage R&R", "page.expanded_gage_rr"},
        {"不平衡 Expanded Gage R&R", "page.expanded_gage_unbalanced"},
        {"裂区析因分析", "page.split_plot_analyze"},
        {"Mixture + 过程变量", "page.mixture_process_variable"},
        {"单因子 MANOVA", "page.manova_one_way"},
        {"General MANOVA", "page.general_manova"},
        {"混合效应 REML", "page.mixed_effects_reml"},
        {"二值 DOE Probit/Gompit", "page.binary_doe_probit"},
        {"寿命数据 Lognormal", "page.life_data_lognormal"},
        {"EMP Crossed", "page.emp_crossed"},
        {"Gage Stability", "page.gage_stability"},
        {"Bias/Linearity", "page.bias_linearity"},
        {"MSA Type 1 Gage", "page.msa_type1_gage"},
        {"单因素 ANOVA", "page.oneway_anova"},
        {"线性回归", "page.linear_regression"},
        {"显示描述性统计", "page.display_descriptive_stats"},
        {"正态性检验", "page.normality_test"},
        {"异常值检验", "page.outlier_test"},
        {"单样本 t 检验", "page.one_sample_t"},
        {"单样本 Z 检验", "page.one_sample_z"},
        {"双样本 t 检验", "page.two_sample_t"},
        {"配对 t 检验", "page.paired_t"},
        {"等价性检验", "page.equivalence_test"},
        {"单样本等价性检验", "page.equivalence_one_sample"},
        {"双样本等价性检验", "page.equivalence_two_sample"},
        {"双样本均值比等价性检验", "page.equivalence_two_sample_ratio"},
        {"配对等价性检验", "page.equivalence_paired"},
        {"单比例等价性检验", "page.equivalence_one_proportion"},
        {"两比例等价性检验", "page.equivalence_two_proportion"},
        {"I-MR 控制图", "page.imr_chart"},
        {"P 图", "page.p_chart"},
        {"NP 图", "page.np_chart"},
        {"C 图", "page.c_chart"},
        {"U 图", "page.u_chart"},
        {"G 图", "page.g_chart"},
        {"G 图（几何间隔）", "page.g_chart_geometric"},
        {"T 图", "page.t_chart"},
        {"T 图（时间间隔）", "page.t_chart_time"},
        {"Xbar-R 控制图", "page.xbar_r_chart"},
        {"Xbar-S 控制图", "page.xbar_s_chart"},
        {"EWMA 控制图", "page.ewma_chart"},
        {"CUSUM 控制图", "page.cusum_chart"},
        {"Z-MR 控制图", "page.z_mr_chart"},
        {"移动平均控制图", "page.ma_chart"},
        {"区域图", "page.zone_chart"},
        {"区域图（分面）", "page.area_faceted"},
        {"面积区域图", "page.area_plot"},
        {"过程能力 Sixpack", "page.capability_sixpack"},
        {"保修摘要", "page.warranty_summary"},
        {"直方图", "page.histogram"},
        {"运行图", "page.run_chart"},
        {"箱线图", "page.boxplot"},
        {"柏拉图", "page.pareto"},
    };
    // Accept legacy English reliability page title from older builds.
    if (title == "Reliability Analysis") {
        title = domain::resolve_report_text(
                    "page.reliability_analysis", language, missing_out)
                    .text;
        return;
    }
    for (const auto& [zh, id] : title_ids) {
        if (title == zh
            || title == domain::resolve_report_text(id, "en-US", nullptr).text) {
            title = domain::resolve_report_text(id, language, missing_out).text;
            return;
        }
    }
    const std::string facet_zh = "（分面）";
    const std::string facet_en =
        domain::resolve_report_text("page.suffix.faceted", "en-US", nullptr).text;
    std::string base;
    if (title.size() > facet_zh.size()
        && title.compare(title.size() - facet_zh.size(), facet_zh.size(), facet_zh) == 0) {
        base = title.substr(0, title.size() - facet_zh.size());
    } else if (title.size() > facet_en.size()
               && title.compare(title.size() - facet_en.size(), facet_en.size(), facet_en)
                   == 0) {
        base = title.substr(0, title.size() - facet_en.size());
    }
    if (!base.empty()) {
        for (const auto& [zh, id] : title_ids) {
            if (base == zh
                || base == domain::resolve_report_text(id, "en-US", nullptr).text) {
                title = domain::resolve_report_text(id, language, missing_out).text
                    + domain::resolve_report_text(
                          "page.suffix.faceted", language, missing_out)
                          .text;
                return;
            }
        }
    }
}

void localize_visible_diagnostics(
    std::vector<domain::DiagnosticMessage>& diagnostics,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    const bool english = language == "en" || language == "en-US" || language == "en_US"
        || language.rfind("en-", 0) == 0;
    if (!english) {
        (void)missing_out;
        return;
    }
    for (domain::DiagnosticMessage& diagnostic : diagnostics) {
        if (diagnostic.code == "expected_count_below_five") {
            diagnostic.message = domain::resolve_report_text(
                "interp.gof_diag_expected_lt5_cats", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "gof_validity_caution") {
            diagnostic.message = domain::resolve_report_text(
                "interp.gof_diag_high_low_expected_share", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "gof_validity_poor") {
            diagnostic.message = domain::resolve_report_text(
                "interp.gof_diag_expected_too_low", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "expected_count_below_one") {
            diagnostic.message = domain::resolve_report_text(
                "interp.chi2_diag_expected_lt1_cells", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "invalid_gof_categories") {
            diagnostic.message = domain::resolve_report_text(
                "interp.gof_err_need_two_categories", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "invalid_gof_count") {
            diagnostic.message = domain::resolve_report_text(
                "interp.gof_err_nonneg_counts_names", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "proportion_count_mismatch") {
            diagnostic.message = domain::resolve_report_text(
                "interp.gof_err_proportion_length", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "empty_gof_table") {
            diagnostic.message = domain::resolve_report_text(
                "interp.gof_err_total_count_positive", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "invalid_test_proportion") {
            diagnostic.message = domain::resolve_report_text(
                "interp.gof_err_proportion_positive", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "proportions_not_sum_to_one") {
            diagnostic.message = domain::resolve_report_text(
                "interp.gof_err_proportions_sum_one", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "invalid_contingency_table") {
            diagnostic.message = domain::resolve_report_text(
                "interp.chi2_err_table_shape", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "ragged_contingency_table") {
            diagnostic.message = domain::resolve_report_text(
                "interp.chi2_err_ragged_rows", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "invalid_cell_count") {
            diagnostic.message = domain::resolve_report_text(
                "interp.chi2_err_nonneg_cells", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "empty_contingency_table") {
            diagnostic.message = domain::resolve_report_text(
                "interp.chi2_err_total_positive", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "johnson_capability_gated") {
            diagnostic.message =
                "Johnson capability is research/preview only: fit and transform-scale "
                "indices may be shown, but process pass/fail judgment is blocked "
                "(gate: missing golden / tail / human review).";
        } else if (diagnostic.code == "hexbin_rectangular_bins") {
            diagnostic.message = domain::resolve_report_text(
                "diag.hexbin_rectangular_bins", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "cube_plot_requires_2_or_3_factors") {
            const std::string prefix = "立方图仅支持 2 或 3 个因子（当前 ";
            const std::string mid = " 个）；请用主效应图、交互图、等值线/曲面图查看高维设计。";
            if (starts_with(diagnostic.message, prefix)
                && ends_with(diagnostic.message, mid)) {
                const std::string count = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - mid.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.cube_plot_requires_2_or_3_factors",
                        language,
                        missing_out)
                        .text,
                    "%1",
                    count);
            } else {
                diagnostic.message = domain::resolve_report_text(
                    "diag.cube_plot_requires_2_or_3_factors", language, missing_out)
                                         .text;
            }
        } else if (diagnostic.code == "invalid_contour_factors") {
            if (diagnostic.message == "等值线因子索引无效。") {
                diagnostic.message = domain::resolve_report_text(
                    "diag.doe.invalid_contour_factor_index", language, missing_out)
                                         .text;
            } else {
                diagnostic.message = domain::resolve_report_text(
                    "diag.invalid_contour_factors", language, missing_out)
                                         .text;
            }
        } else if (diagnostic.code == "contour_requires_two_factors") {
            diagnostic.message = domain::resolve_report_text(
                "diag.contour_requires_two_factors", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "doe_worksheet_export_ready") {
            diagnostic.message = domain::resolve_report_text(
                "diag.doe_worksheet_export_ready", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "censoring_worksheet_export_ready") {
            const std::string prefix = "已生成逐观测删失状态工作表（";
            const std::string mid = " 行；censoring_type=exact|right|left|interval）。"
                "可写回活动表以便审计或再导入；不是 vendor_oracle。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, mid)) {
                const std::string n = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - mid.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.censoring_worksheet_export_ready", language, missing_out)
                        .text,
                    "%1",
                    n);
            } else {
                diagnostic.message = domain::resolve_report_text(
                    "diag.censoring_worksheet_export_ready", language, missing_out)
                                         .text;
                diagnostic.message = replace_token(diagnostic.message, "%1", "?");
            }
        } else if (diagnostic.code == "capability_pass_fail_blocked_by_stability_prerequisite") {
            diagnostic.message =
                "Normal capability has not met the stability/normality acceptance "
                "prerequisite: process pass/fail judgment is blocked "
                "(pass_fail_judgment_allowed=false).";
        } else if (diagnostic.code == "capability_stability_screen_signals") {
            if (const auto count = parse_leading_count_after_prefix(
                    diagnostic.message, "I-MR Rule-1 初筛检出")) {
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "interp.capability_stability_screen_signals", language, missing_out)
                        .text,
                    "%1",
                    std::to_string(*count));
            } else {
                diagnostic.message = domain::resolve_report_text(
                    "interp.capability_stability_screen_signals", language, missing_out)
                                         .text;
                diagnostic.message = replace_token(diagnostic.message, "%1", "n");
            }
        } else if (diagnostic.code == "capability_stability_screen_clear_not_verified") {
            diagnostic.message = domain::resolve_report_text(
                "interp.capability_stability_clear_not_verified", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "capability_stability_prerequisite") {
            diagnostic.message = domain::resolve_report_text(
                "interp.capability_stability_prerequisite", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "capability_bimodality_suspected") {
            const std::string prefix = "直方图双峰初筛检出约 ";
            const std::string suffix =
                " 个可分离峰；单一分布能力指数仅供调查，"
                "不得写成过程合格（直方图初筛 ≠ Hartigan；≠ 混合模型证明）。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string count = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.cap.bimodality_suspected", language, missing_out)
                        .text,
                    "%1",
                    count);
            }
        } else if (diagnostic.code == "capability_bimodality_clear_not_verified") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cap_bimodality_clear_not_proof", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "capability_bimodality_insufficient_n") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cap_bimodality_insufficient_n", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "capability_bimodality_screen_note") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cap_bimodality_screen_note", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "capability_stability_insufficient_n") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cap_stability_insufficient_n", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "capability_hartigan_dip_insufficient_n") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cap_hartigan_insufficient_n", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "capability_hartigan_dip_evidence_against") {
            const std::string prefix =
                "Hartigan dip 门禁筛查（formula_reference / hartigan_dip_1985）：dip=";
            const std::string tail =
                "；提示偏离单峰，单一分布能力指数仅供调查，"
                "不得写成过程合格（非 vendor_oracle；二维高斯混合另有门禁）。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, tail)) {
                const std::string body = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - tail.size());
                const std::string mc_mid = "，Uniform 零假设 MC p≈";
                const std::string mc_suffix = "（reps=";
                std::string dip_part = body;
                std::string mc_part;
                const auto mc_pos = body.find(mc_mid);
                if (mc_pos != std::string::npos) {
                    dip_part = body.substr(0, mc_pos);
                    const std::string rest = body.substr(mc_pos + mc_mid.size());
                    const auto reps_pos = rest.find(mc_suffix);
                    if (reps_pos != std::string::npos) {
                        const std::string p_val = rest.substr(0, reps_pos);
                        const std::string reps_rest = rest.substr(
                            reps_pos + mc_suffix.size());
                        const auto reps_end = reps_rest.find('）');
                        if (reps_end != std::string::npos) {
                            const std::string reps = reps_rest.substr(0, reps_end);
                            mc_part = replace_all_tokens(
                                domain::resolve_report_text(
                                    "diag.cap.hartigan_mc_clause", language, missing_out)
                                    .text,
                                {{"%1", p_val}, {"%2", reps}});
                        }
                    }
                }
                diagnostic.message = replace_all_tokens(
                    domain::resolve_report_text(
                        "diag.cap.hartigan_evidence_against", language, missing_out)
                        .text,
                    {{"%1", dip_part}, {"%2", mc_part}});
            }
        } else if (diagnostic.code == "capability_hartigan_dip_consistent_not_proof") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cap_hartigan_consistent_not_proof", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "capability_hartigan_dip_screen_note") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cap_hartigan_screen_note", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "capability_mixture_insufficient_n") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cap_mixture_insufficient_n", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "capability_mixture_failed") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cap_mixture_failed", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "capability_mixture_preferred_2comp"
                   || diagnostic.code == "capability_mixture_preferred_kcomp") {
            const std::string prefix =
                "高斯混合门禁（formula_reference / gaussian_mixture_k_bic）：BIC 更支持 k=";
            const std::string mid = "（ΔBIC≈";
            const std::string mid2 = "；k_max=";
            const std::string suffix =
                "）；单一分布能力指数仅供调查，不得写成过程合格（非 vendor_oracle）。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)) {
                const auto mid_pos = diagnostic.message.find(mid, prefix.size());
                const auto mid2_pos = diagnostic.message.find(mid2, mid_pos + mid.size());
                if (mid_pos != std::string::npos && mid2_pos != std::string::npos) {
                    const std::string k_sel = diagnostic.message.substr(
                        prefix.size(), mid_pos - prefix.size());
                    const std::string delta = diagnostic.message.substr(
                        mid_pos + mid.size(), mid2_pos - (mid_pos + mid.size()));
                    const std::string k_max = diagnostic.message.substr(
                        mid2_pos + mid2.size(),
                        diagnostic.message.size() - (mid2_pos + mid2.size())
                            - suffix.size());
                    diagnostic.message = replace_all_tokens(
                        domain::resolve_report_text(
                            "diag.cap.mixture_preferred_bic", language, missing_out)
                            .text,
                        {{"%1", k_sel}, {"%2", delta}, {"%3", k_max}});
                }
            }
        } else if (diagnostic.code == "capability_mixture_not_preferred") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cap_mixture_not_preferred", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "capability_mixture_screen_note") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cap_mixture_screen_note", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "between_variance_truncated") {
            diagnostic.message = domain::resolve_report_text(
                "diag.cap.between_variance_truncated", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "ci_df_used_sample_n") {
            diagnostic.message = domain::resolve_report_text(
                "diag.cap.ci_df_used_sample_n", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "capability_ci_not_computed") {
            diagnostic.message = domain::resolve_report_text(
                "diag.cap.ci_not_computed", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "johnson_spec_outside_support") {
            if (diagnostic.message
                == "规格限落在 Johnson 变换定义域外，无法计算 Pp/Ppk。") {
                diagnostic.message = domain::resolve_report_text(
                    "interp.johnson_spec_outside_support", language, missing_out)
                                         .text;
            } else {
                diagnostic.message = domain::resolve_report_text(
                    "interp.johnson_spec_partial_outside", language, missing_out)
                                         .text;
            }
        } else if (diagnostic.code == "box_cox_invalid_spec_limit") {
            if (diagnostic.message == "规格下限无法变换（须为正有限数）。") {
                diagnostic.message = domain::resolve_report_text(
                    "diag.box_cox_invalid_spec_limit_lsl", language, missing_out)
                                         .text;
            } else if (diagnostic.message == "规格上限无法变换（须为正有限数）。") {
                diagnostic.message = domain::resolve_report_text(
                    "diag.box_cox_invalid_spec_limit_usl", language, missing_out)
                                         .text;
            } else if (diagnostic.message == "规格目标无法变换（须为正有限数）。") {
                diagnostic.message = domain::resolve_report_text(
                    "diag.box_cox_invalid_spec_limit_target", language, missing_out)
                                         .text;
            }
        } else if (diagnostic.code == "box_cox_spec_limits_order") {
            diagnostic.message = domain::resolve_report_text(
                "diag.box_cox_spec_limits_order", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "nonnormal_z_score_formula_reference") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cap_nonnormal_zscore_scope", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "within_not_applicable_after_johnson") {
            diagnostic.message = domain::resolve_report_text(
                "interp.johnson_within_not_applicable", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "facet_levels_truncated") {
            const std::string prefix = "分面水平数 = ";
            const std::string mid = "，受控 Graph Builder 最多显示 ";
            const std::string mid2 = " 个面板；已截断 ";
            const std::string suffix = " 个水平（非自由拼版）。";
            if (starts_with(diagnostic.message, prefix)
                && diagnostic.message.find(mid) != std::string::npos
                && diagnostic.message.find(mid2) != std::string::npos
                && ends_with(diagnostic.message, suffix)) {
                const auto pos1 = diagnostic.message.find(mid);
                const auto pos2 = diagnostic.message.find(mid2, pos1 + mid.size());
                const std::string total = diagnostic.message.substr(
                    prefix.size(), pos1 - prefix.size());
                const std::string capped = diagnostic.message.substr(
                    pos1 + mid.size(), pos2 - (pos1 + mid.size()));
                const std::string truncated = diagnostic.message.substr(
                    pos2 + mid2.size(),
                    diagnostic.message.size() - (pos2 + mid2.size()) - suffix.size());
                diagnostic.message = replace_all_tokens(
                    domain::resolve_report_text(
                        "diag.graph.facet_levels_truncated", language, missing_out)
                        .text,
                    {{"%1", total}, {"%2", capped}, {"%3", truncated}});
            } else {
                diagnostic.message = domain::resolve_report_text(
                    "diag.graph.facet_levels_truncated", language, missing_out)
                                         .text;
                diagnostic.message = replace_all_tokens(
                    diagnostic.message, {{"%1", "n"}, {"%2", "max"}, {"%3", "k"}});
            }
        } else if (diagnostic.code == "facet_controlled_panels") {
            diagnostic.message = domain::resolve_report_text(
                "diag.graph.facet_controlled_panels", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "density_curve_not_discrete_marks") {
            diagnostic.message = domain::resolve_report_text(
                "diag.density_curve_not_discrete_marks", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "bar_hidden_excluded_distinct") {
            diagnostic.message = domain::resolve_report_text(
                                     "diag.graph.bar_hidden_excluded_distinct",
                                     language,
                                     missing_out)
                                     .text;
        } else if (diagnostic.code == "row_visibility_contract") {
            diagnostic.message = domain::resolve_report_text(
                "diag.row_visibility.contract", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "row_visibility_overlap") {
            diagnostic.message = domain::resolve_report_text(
                "diag.row_visibility.overlap", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "km_interval_scope") {
            diagnostic.message = domain::resolve_report_text(
                "diag.km_interval.scope", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "km_interval_evidence") {
            diagnostic.message = domain::resolve_report_text(
                "diag.km_interval.evidence", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "km_interval_not_vendor_oracle") {
            diagnostic.message =
                "Interval-censored Turnbull is a simplified-grid NPMLE "
                "(formula_reference); per-mode reliability fits and pinned R/commercial "
                "alignment are not frozen — do not claim vendor_oracle.";
        } else if (diagnostic.code == "km_interval_max_iter") {
            diagnostic.message = domain::resolve_report_text(
                "diag.km_interval.max_iter", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "km_interval_collapse") {
            diagnostic.message = domain::resolve_report_text(
                "diag.km_interval.mass_collapsed", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "km_interval_n") {
            diagnostic.message = domain::resolve_report_text(
                "diag.km_interval.need_three_obs", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "km_interval_order") {
            diagnostic.message = domain::resolve_report_text(
                "diag.km_interval.invalid_interval_order", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "km_interval_clean") {
            diagnostic.message = domain::resolve_report_text(
                "diag.km_interval.too_few_valid", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "km_interval_grid") {
            diagnostic.message = domain::resolve_report_text(
                "diag.km_interval.grid_failed", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "km_interval_mass") {
            diagnostic.message = domain::resolve_report_text(
                "diag.km_interval.no_mass_points", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "cif_unlabeled_exact_excluded") {
            const std::string prefix = "有 ";
            const std::string mid = " 条 exact 失效缺少 failure_mode，已从 CIF 排除。";
            const auto pos = diagnostic.message.find(mid);
            if (starts_with(diagnostic.message, prefix) && pos != std::string::npos) {
                const std::string count = diagnostic.message.substr(
                    prefix.size(), pos - prefix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.cif.unlabeled_exact_excluded", language, missing_out)
                        .text,
                    "%1",
                    count);
            }
        } else if (diagnostic.code == "cif_left_interval_omitted") {
            const std::string prefix = "CIF 省略 left/interval 行 ";
            const std::string suffix = " 条；请用 km_interval 路径处理。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string count = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.cif.left_interval_omitted", language, missing_out)
                        .text,
                    "%1",
                    count);
            }
        } else if (diagnostic.code == "cif_aalen_johansen_scope") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cif_info_aalen_johansen_scope", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "cif_not_fine_gray_multivar") {
            diagnostic.message = domain::resolve_report_text(
                "interp.cif_warn_not_fine_gray", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "mixture2_scope") {
            diagnostic.message = domain::resolve_report_text(
                "diag.gaussian_mixture2.scope", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "mixture_k_scope") {
            const std::string prefix =
                "多 k 高斯混合 = formula_reference / gaussian_mixture_k_bic（EM + BIC，k=1..";
            const std::string mid = "；选定 k=";
            const std::string suffix =
                "）；不是非高斯混合，不是 vendor_oracle，不得写成过程合格判定。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)) {
                const auto mid_pos = diagnostic.message.find(mid, prefix.size());
                if (mid_pos != std::string::npos) {
                    const std::string k_max = diagnostic.message.substr(
                        prefix.size(), mid_pos - prefix.size());
                    const std::string best_k = diagnostic.message.substr(
                        mid_pos + mid.size(),
                        diagnostic.message.size() - (mid_pos + mid.size())
                            - suffix.size());
                    diagnostic.message = replace_all_tokens(
                        domain::resolve_report_text(
                            "diag.gaussian_mixture_k.scope", language, missing_out)
                            .text,
                        {{"%1", k_max}, {"%2", best_k}});
                }
            }
        } else if (diagnostic.code == "mixture_k_skipped_sparse") {
            const std::string prefix = "跳过 k=";
            const std::string suffix = "（n < 15k）；不得把跳过写成已排除该 k。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string k = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.gaussian_mixture_k.skipped_sparse", language, missing_out)
                        .text,
                    "%1",
                    k);
            }
        } else if (diagnostic.code == "mixture_k_em_failed") {
            const std::string prefix = "k=";
            const std::string mid = " 高斯混合 EM 失败（";
            const std::string suffix = "）；该 k 不参与 BIC 优选。";
            if (starts_with(diagnostic.message, prefix)) {
                const auto mid_pos = diagnostic.message.find(mid, prefix.size());
                if (mid_pos != std::string::npos
                    && ends_with(diagnostic.message, suffix)) {
                    const std::string k = diagnostic.message.substr(
                        prefix.size(), mid_pos - prefix.size());
                    const std::string reason = diagnostic.message.substr(
                        mid_pos + mid.size(),
                        diagnostic.message.size() - (mid_pos + mid.size())
                            - suffix.size());
                    diagnostic.message = replace_all_tokens(
                        domain::resolve_report_text(
                            "diag.gaussian_mixture_k.em_failed", language, missing_out)
                            .text,
                        {{"%1", k}, {"%2", reason}});
                }
            }
        } else if (diagnostic.code == "mixture_k_not_converged") {
            const std::string prefix = "k=";
            const std::string suffix = " 高斯混合 EM 未收敛；该 k 不参与 BIC 优选。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string k = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.gaussian_mixture_k.not_converged", language, missing_out)
                        .text,
                    "%1",
                    k);
            }
        } else if (diagnostic.code == "missing_interval_bounds") {
            diagnostic.message =
                "Interval-censored rows require valid left/right bound columns "
                "(interval_left < interval_right); missing or non-finite values are not invented.";
        } else if (diagnostic.code == "invalid_exposure_value") {
            if (diagnostic.message
                == "暴露量列必须为有限非负数；缺失或非法值不会被静默当作 0 或 1。") {
                diagnostic.message = domain::resolve_report_text(
                    "diag.invalid_exposure_value_reliability", language, missing_out)
                                         .text;
            } else {
                diagnostic.message = domain::resolve_report_text(
                    "diag.invalid_exposure_value", language, missing_out)
                                         .text;
            }
        } else if (diagnostic.code == "warranty_zero_exposure") {
            diagnostic.message = domain::resolve_report_text(
                "diag.warranty_zero_exposure", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "censoring_left_interval_not_for_classic_km") {
            diagnostic.message = domain::resolve_report_text(
                "diag.censoring.classic_km_rejects_left_interval", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "censoring_worksheet_import_ok") {
            const std::string prefix = "已从工作表导入 ";
            const std::string suffix =
                " 条删失观测；证据类型仍为 formula_reference，不是 vendor_oracle。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string count = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.censoring.worksheet_imported", language, missing_out)
                        .text,
                    "%1",
                    count);
            }
        } else if (diagnostic.code == "censoring_worksheet_invalid_type") {
            const std::string prefix = "第 ";
            const std::string suffix = " 行 censoring_type 无法解析。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)) {
                const std::string row = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.censoring.worksheet_row_type_unparsed", language, missing_out)
                        .text,
                    "%1",
                    row);
            }
        } else if (diagnostic.code == "censoring_worksheet_invalid_time") {
            const std::string prefix = "第 ";
            const std::string suffix = " 行 time 不是有限数。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)) {
                const std::string row = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.censoring.worksheet_row_time_invalid", language, missing_out)
                        .text,
                    "%1",
                    row);
            }
        } else if (diagnostic.code == "censoring_worksheet_invalid_interval_bounds") {
            const std::string prefix = "第 ";
            const std::string suffix = " 行区间界缺失或非法（不静默补齐）。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)) {
                const std::string row = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.censoring.worksheet_row_interval_invalid", language, missing_out)
                        .text,
                    "%1",
                    row);
            }
        } else if (diagnostic.code == "censoring_worksheet_invalid_exposure") {
            const std::string prefix = "第 ";
            const std::string suffix = " 行 exposure 非法。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)) {
                const std::string row = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.censoring.worksheet_row_exposure_invalid", language, missing_out)
                        .text,
                    "%1",
                    row);
            }
        } else if (diagnostic.code == "mode_fit_unlabeled_exact_excluded") {
            const std::string prefix = "有 ";
            const std::string suffix =
                " 条 exact 失效缺少 failure_mode，已从分模式拟合排除"
                "（不作任一模式的竞争删失）。";
            const auto pos = diagnostic.message.find(suffix);
            if (starts_with(diagnostic.message, prefix) && pos != std::string::npos) {
                const std::string count = diagnostic.message.substr(
                    prefix.size(), pos - prefix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.mode_fit.unlabeled_exact_excluded", language, missing_out)
                        .text,
                    "%1",
                    count);
            }
        } else if (diagnostic.code == "mode_fit_left_interval_omitted") {
            const std::string prefix = "分模式拟合省略 left/interval 行 ";
            const std::string suffix = " 条；请用 km_interval / 总体路径处理。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string count = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.mode_fit.left_interval_omitted", language, missing_out)
                        .text,
                    "%1",
                    count);
            }
        } else if (diagnostic.code == "mode_fit_model_unsupported") {
            const std::string prefix = "分模式拟合不支持模型 '";
            const std::string suffix = "'。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string model = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.mode_fit.model_unsupported", language, missing_out)
                        .text,
                    "%1",
                    model);
            }
        } else if (diagnostic.code == "mode_fit_no_failures") {
            const std::string prefix = "模式 ";
            const std::string suffix = " 无 exact 失效，无法拟合。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string mode = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.mode_fit.no_failures", language, missing_out)
                        .text,
                    "%1",
                    mode);
            }
        } else if (diagnostic.code == "mode_fit_reliability_unavailable") {
            const std::string prefix = "模式 ";
            const std::string suffix = " 已拟合但未能在 T_w 计算 R。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string mode = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.mode_fit.reliability_at_tw_failed", language, missing_out)
                        .text,
                    "%1",
                    mode);
            }
        } else if (diagnostic.code == "warranty_stratum_invalid_mode_reliability") {
            const std::string prefix = "分层 ";
            const std::string suffix = " 提供的分模式 R(T_w) 非法，已回退到池化 R。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string label = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.warranty.stratum_invalid_mode_r", language, missing_out)
                        .text,
                    "%1",
                    label);
            }
        } else if (diagnostic.code == "warranty_exposure_column_overrides_scalar") {
            diagnostic.message = domain::resolve_report_text(
                "diag.warranty_exposure_column_overrides_scalar", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "warranty_stratum_exposure_proportional") {
            diagnostic.message = domain::resolve_report_text(
                "diag.warranty.stratum_exposure_proportional", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "warranty_stratum_exposure_sum_mismatch") {
            diagnostic.message = domain::resolve_report_text(
                "diag.warranty.stratum_exposure_sum_mismatch", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "warranty_stratum_no_denominator") {
            diagnostic.message = domain::resolve_report_text(
                "diag.warranty.stratum_no_denominator", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "warranty_stratum_kind_missing") {
            diagnostic.message = domain::resolve_report_text(
                "diag.warranty.stratum_kind_missing", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "warranty_stratum_kind_mixed") {
            diagnostic.message = domain::resolve_report_text(
                "diag.warranty.stratum_kind_mixed", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "warranty_stratum_invalid_exposure") {
            diagnostic.message = domain::resolve_report_text(
                "diag.warranty.stratum_invalid_exposure", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "warranty_strata_empty") {
            diagnostic.message = domain::resolve_report_text(
                "diag.warranty.no_strata_input", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "warranty_strata_pooled_reliability") {
            diagnostic.message = domain::resolve_report_text(
                "diag.warranty.strata_pooled_reliability", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "warranty_strata_mode_specific_reliability") {
            diagnostic.message = domain::resolve_report_text(
                "interp.warranty_strata_mode_specific_reliability", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "warranty_strata_mixed_reliability") {
            diagnostic.message = domain::resolve_report_text(
                "interp.warranty_strata_mixed_reliability", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "special_cause_policy_minitab_like") {
            const std::string prefix =
                "特殊原因策略=minitab_like：默认仅启用「";
            const std::string suffix = "」（与 Minitab 常见默认接近）。";
            if (starts_with(diagnostic.message, prefix)
                && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string rule_name = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.spc.policy_minitab_like", language, missing_out)
                        .text,
                    "%1",
                    localize_spc_rule_name(rule_name, language, missing_out));
            } else {
                diagnostic.message = domain::resolve_report_text(
                    "diag.spc.policy_minitab_like", language, missing_out)
                                         .text;
                diagnostic.message = replace_token(
                    diagnostic.message, "%1", "Rule 1");
            }
        } else if (diagnostic.code == "special_cause_policy_all_applicable") {
            const std::string prefix =
                "特殊原因策略=all_applicable：启用该图种全部适用特殊原因规则；"
                "多规则提高灵敏度也提高误报风险，与 Minitab 默认仅「";
            const std::string suffix = "」不同。";
            if (starts_with(diagnostic.message, prefix)
                && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string rule_name = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.spc.policy_all_applicable", language, missing_out)
                        .text,
                    "%1",
                    localize_spc_rule_name(rule_name, language, missing_out));
            } else {
                diagnostic.message = domain::resolve_report_text(
                    "diag.spc.policy_all_applicable", language, missing_out)
                                         .text;
                diagnostic.message = replace_token(
                    diagnostic.message, "%1", "Rule 1");
            }
        } else if (diagnostic.code == "test_not_applicable") {
            const std::string prefix =
                "已忽略不适用于此控制图的特殊原因规则：";
            const std::string suffix = "。";
            if (starts_with(diagnostic.message, prefix)
                && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string rules = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.spc.test_not_applicable", language, missing_out)
                        .text,
                    "%1",
                    localize_special_cause_rule_name_list(
                        rules, language, missing_out));
            } else {
                diagnostic.message = domain::resolve_report_text(
                    "diag.spc.test_not_applicable", language, missing_out)
                                         .text;
                diagnostic.message = replace_token(diagnostic.message, "%1", "?");
            }
        } else if (diagnostic.code == "nelson_estimate_applied") {
            const std::string prefix = "Nelson estimate 剔除了 ";
            const std::string suffix = " 个过大移动极差后重估 σ。";
            if (starts_with(diagnostic.message, prefix)
                && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string count = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "interp.nelson_estimate_applied", language, missing_out)
                        .text,
                    "%1",
                    count);
            } else {
                diagnostic.message = domain::resolve_report_text(
                    "interp.nelson_estimate_applied", language, missing_out)
                                         .text;
                diagnostic.message = replace_token(diagnostic.message, "%1", "n");
            }
        } else if (diagnostic.code == "variability_singleton_cell") {
            const std::string prefix = "单元「";
            const std::string suffix = "」仅 1 个点，SD 记为 0。";
            if (starts_with(diagnostic.message, prefix)
                && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string label = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.variability.singleton_cell", language, missing_out)
                        .text,
                    "%1",
                    label);
            } else {
                diagnostic.message = domain::resolve_report_text(
                    "diag.variability.singleton_cell", language, missing_out)
                                         .text;
                diagnostic.message = replace_token(diagnostic.message, "%1", "?");
            }
        } else if (diagnostic.code == "adf_default_lags") {
            const std::string prefix = "默认滞后 p = floor((T-1)^(1/3)) = ";
            const std::string suffix = "。";
            if (starts_with(diagnostic.message, prefix) && ends_with(diagnostic.message, suffix)
                && diagnostic.message.size() > prefix.size() + suffix.size()) {
                const std::string lags = diagnostic.message.substr(
                    prefix.size(),
                    diagnostic.message.size() - prefix.size() - suffix.size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.adf.default_lags", language, missing_out)
                        .text,
                    "%1",
                    lags);
            } else {
                diagnostic.message = domain::resolve_report_text(
                    "diag.adf.default_lags", language, missing_out)
                                         .text;
                diagnostic.message = replace_token(diagnostic.message, "%1", "?");
            }
        } else if (diagnostic.code == "adf_lags_too_large") {
            diagnostic.message = domain::resolve_report_text(
                "diag.adf.lags_too_large", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "adf_critical_source") {
            diagnostic.message = domain::resolve_report_text(
                "diag.adf.critical_source", language, missing_out)
                                     .text;
        } else if (diagnostic.code == "kde_silverman_bandwidth") {
            diagnostic.message = domain::resolve_report_text(
                "diag.eda.kde_silverman_bandwidth", language, missing_out)
                                     .text;
        } else if (starts_with(diagnostic.message, "第 ")
                   && diagnostic.message.find(" 行的字段数与列数不一致。")
                       != std::string::npos) {
            const std::string mid = " 行的字段数与列数不一致。";
            const auto pos = diagnostic.message.find(mid);
            if (pos != std::string::npos && pos > std::string("第 ").size()) {
                const std::string row = diagnostic.message.substr(
                    std::string("第 ").size(), pos - std::string("第 ").size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.import.contract.row_field_count_mismatch",
                        language,
                        missing_out)
                        .text,
                    "%1",
                    row);
            }
        } else if (starts_with(diagnostic.message, "第 ")
                   && diagnostic.message.find(" 行的单元格状态数与列数不一致。")
                       != std::string::npos) {
            const std::string mid = " 行的单元格状态数与列数不一致。";
            const auto pos = diagnostic.message.find(mid);
            if (pos != std::string::npos && pos > std::string("第 ").size()) {
                const std::string row = diagnostic.message.substr(
                    std::string("第 ").size(), pos - std::string("第 ").size());
                diagnostic.message = replace_token(
                    domain::resolve_report_text(
                        "diag.import.contract.row_cell_states_mismatch",
                        language,
                        missing_out)
                        .text,
                    "%1",
                    row);
            }
        } else {
            // Exact Chinese diagnostic bodies (shared codes like small_count_*).
            const std::string localized = localize_known_plain_message(
                diagnostic.message, language, missing_out);
            if (localized != diagnostic.message) {
                diagnostic.message = localized;
            }
        }
    }
}

void localize_parameter_summary(
    std::string& summary,
    const std::string& language,
    std::vector<domain::MissingTranslation>* missing_out)
{
    if (summary.empty()) {
        return;
    }
    // Graph Builder captions (显示 N / 分析 N / 分面) also appear in parameter_summary.
    localize_graph_caption_tokens(summary, language, missing_out);
    // Longer tokens first so "变量数 = " is not partially matched by "变量 = ".
    static const std::pair<const char*, const char*> tokens[] = {
        {"颜色范围固定为相关系数 [-1, 1]    方法 = ", "param.summary.corr_color_range"},
        {"面积表示相邻观测之间的数值区间，不是置信区间", "param.summary.area_not_ci"},
        {"单元格为组内均值", "param.summary.cell_group_mean"},
        {"直线性不能单独作为正态性证明；显示拟合省略 hidden，分析拟合保留 hidden",
         "param.summary.linearity_hidden_fit_note"},
        {"直线性不能单独作为正态性证明", "param.summary.linearity_not_normality_proof"},
        {"坐标已按各变量最小-最大范围标准化", "param.summary.coords_minmax_standardized"},
        {"相关矩阵热图单元格不是观测层；不伪造 per-cell member_source_rows",
         "param.summary.corr_heatmap_not_obs_layer"},
        {"相关矩阵单元格不是观测层", "param.summary.corr_cell_not_obs_layer"},
        {"（本命令不做卡方检验；关联检验请用列联表卡方）",
         "param.summary.crosstab_no_chi2_note"},
        {"线性+交互+纯二次（编码单位）",
         "param.summary.rsm_model_linear_interact_pq"},
        {"有效运行数 = ", "param.summary.effective_runs_eq"},
        {"偏相关 = 是", "param.summary.partial_corr_yes"},
        {"须线 = Tukey 1.5×IQR", "param.summary.whisker_tukey"},
        {"设计来源 ID = ", "param.summary.design_source_id_eq"},
        {"；设计族 = ", "param.summary.design_family_eq"},
        {"（Wheeler EMP；非全量 Expanded Gage）", "param.summary.emp_wheeler_note"},
        {"（平衡三因子随机；非全量 GLM）", "param.summary.expanded_gage_balanced_note"},
        {"零件 / 操作员 / ", "param.summary.part_operator_slash"},
        {"；总体中位数 M = ", "param.summary.overall_median_m_eq"},
        {"正态概率图相关系数 = ", "param.summary.normal_prob_corr_eq"},
        {"移动极差长度 = ", "param.summary.moving_range_length_eq"},
        {"预测期数 = ", "param.summary.forecast_periods_eq"},
        {"因子 A = ", "param.summary.factor_a_eq"},
        {"因子 B = ", "param.summary.factor_b_eq"},
        {"处理 = ", "param.summary.treatment_eq"},
        {"区组 = ", "param.summary.block_eq"},
        {"误差模型 = ", "param.summary.error_model_eq"},
        {"趋势模型 = ", "param.summary.trend_model_eq"},
        {"中位数 = ", "param.summary.median_eq"},
        {"比例 = ", "param.summary.proportion_eq"},
        {"迭代 = ", "param.summary.iterations_eq"},
        {"手工 ", "param.summary.manual_bins_prefix"},
        {"（信息）", "param.summary.info_paren"},
        {"标准化", "param.summary.standardized_flag"},
        {"控制限倍数 = ", "param.summary.limit_multiplier_eq"},
        {"选择准则 = ", "param.summary.selection_criterion_eq"},
        {"得分阈值 = ", "param.summary.score_threshold_eq"},
        {"深度上限 = ", "param.summary.max_depth_eq"},
        {"带宽 = ±", "param.summary.bandwidth_pm_eq"},
        {"分箱规则 = ", "param.summary.binning_rule_eq"},
        {"窗宽 w = ", "param.summary.window_w_eq"},
        {"Z 超限 = ", "param.summary.z_ooc_eq"},
        {"操作者数 = ", "param.summary.operator_count_eq"},
        {"操作员 = ", "param.summary.operator_eq"},
        {"零件 = ", "param.summary.part_eq"},
        {"周期 = ", "param.summary.period_eq"},
        {"叶数 = ", "param.summary.leaf_count_eq"},
        {"选入 = ", "param.summary.selected_in_eq"},
        {"暴露量合计 = ", "param.summary.exposure_total_eq"},
        {"（列求和，", "param.summary.column_sum_open"},
        {" 行）", "param.summary.rows_close"},
        {"暴露量 = ", "param.summary.exposure_eq"},
        {"精确 = ", "param.summary.exact_eq"},
        {"左 = ", "param.summary.left_eq"},
        {"区间 = ", "param.summary.interval_eq"},
        {"右 = ", "param.summary.right_eq"},
        {"事件水平 = ", "param.summary.event_level_eq"},
        {"预测变量数 = ", "param.summary.predictor_variable_count_eq"},
        {"覆盖率 = ", "param.summary.coverage_eq"},
        {"方向 = ", "param.summary.direction_eq"},
        {"「单点超出 3σ 控制限」= 超出 3σ 控制限的点（rule_id=beyond_control_limit）",
         "param.summary.rule1_gloss"},
        {"C 图要求每个子组单位数相同", "param.summary.c_chart_equal_units"},
        {"np̄_i = n_i p̄", "param.summary.npbar_formula"},
        {"Other 阈值 = ", "param.summary.other_threshold_eq"},
        {"总计数 = ", "param.summary.total_count_eq"},
        {"原因数 = ", "param.summary.cause_count_eq"},
        {"二项分布", "param.summary.binomial"},
        {"泊松分布", "param.summary.poisson"},
        {"二项 OC", "param.summary.binomial_oc"},
        {"二项", "param.summary.binomial_short"},
        {"泊松", "param.summary.poisson_short"},
        {"p̄ = Σ不合格品数 / Σ检验数", "param.summary.binomial_pbar_formula"},
        {"ū = Σ缺陷数 / Σ单位数", "param.summary.poisson_ubar_formula"},
        {"c̄ = 缺陷数均值", "param.summary.poisson_cbar_formula"},
        {"过程变差(6σ) = ", "param.summary.process_variation_eq"},
        {"界限 = [", "param.summary.limits_bracket_eq"},
        {"小类别合并阈值 = ", "param.summary.small_category_merge_threshold_eq"},
        {"显示类别数 = ", "param.summary.display_category_count_eq"},
        {"分析类别数 = ", "param.summary.analysis_category_count_eq"},
        {"类别数 = ", "param.summary.category_count_eq"},
        {"显示组数 = ", "param.summary.display_group_count_eq"},
        {"有效变量数 = ", "param.summary.valid_variable_count_eq"},
        {"置信水平 = ", "param.summary.confidence_level_eq"},
        {"备择：总体均值 ", "param.summary.alt_population_mean"},
        {"假设均值 = ", "param.summary.hypothesized_mean_eq"},
        {" 假设均值", "param.summary.hypothesized_mean_tail"},
        {"假设比例 = ", "param.summary.hypothesized_proportion_eq"},
        {"假设发生率 = ", "param.summary.hypothesized_rate_eq"},
        {"目标比例 = ", "param.summary.target_proportion_eq"},
        {"缺失值 N* = ", "param.summary.missing_nstar_eq"},
        {"缺失 = ", "param.summary.missing_eq"},
        {"组数 = ", "param.summary.group_count_eq"},
        {"因子数 k = ", "param.summary.factor_count_k_eq"},
        {"因子数 = ", "param.summary.factor_count_eq"},
        {"变量数 = ", "param.summary.variable_count_eq"},
        {"预测变量 = ", "param.summary.predictor_count_eq"},
        {"异常数 = ", "param.summary.anomaly_count_eq"},
        {"类数 = ", "param.summary.class_count_eq"},
        {"准确率 ≈ ", "param.summary.accuracy_approx_eq"},
        {"水平 = ", "param.summary.level_count_eq"},
        {"回归 = ", "param.summary.regression_eq"},
        {"链 = ", "param.summary.link_eq"},
        {"有效观测 = ", "param.summary.valid_obs_eq"},
        {"处理列数 = ", "param.summary.treatment_columns_eq"},
        {"观测数 = ", "param.summary.observation_count_eq"},
        {"子组数 = ", "param.summary.subgroup_count_eq"},
        {"子组大小 = ", "param.summary.subgroup_size_eq"},
        {"（历史参数）", "param.summary.historical_paren"},
        {"（估计）", "param.summary.estimated_paren"},
        {"部件数 = ", "param.summary.part_count_eq"},
        {"评估者数 = ", "param.summary.evaluator_count_eq"},
        {"Kappa权重 = ", "param.summary.kappa_weight_eq"},
        {"寿命列 = ", "param.summary.lifetime_column_eq"},
        {"响应列 = ", "param.summary.response_column_eq"},
        {"分类列 = ", "param.summary.category_column_eq"},
        {"类别列: ", "param.summary.category_column_colon"},
        {"分组列 = ", "param.summary.grouping_column_eq"},
        {"分组 = ", "param.summary.group_eq"},
        {"面板 = ", "param.summary.panel_eq"},
        {"大小 = ", "param.summary.size_eq"},
        {"数值 = ", "param.summary.numeric_value_eq"},
        {"值列 = ", "param.summary.value_column_eq"},
        {"第一样本 = ", "param.summary.first_sample_eq"},
        {"第一组 = ", "param.summary.first_group_eq"},
        {"第二组 = ", "param.summary.second_group_eq"},
        {"第一列 = ", "param.summary.first_column_eq"},
        {"第二列 = ", "param.summary.second_column_eq"},
        {"测量值 = ", "param.summary.measurement_eq"},
        {"参考值 = ", "param.summary.reference_eq"},
        {"参考 = ", "param.summary.reference_short_eq"},
        {"运行数 = ", "param.summary.run_count_eq"},
        {"分辨度 = ", "param.summary.resolution_eq"},
        {"变体 = ", "param.summary.variant_eq"},
        {"分布 = ", "param.summary.distribution_eq"},
        {"合并方差", "param.summary.pooled_variance"},
        {"不等于", "param.alt.not_equal"},
        {"小于", "param.alt.less"},
        {"大于", "param.alt.greater"},
        {"备择: ", "param.summary.alternative_colon"},
        {"方法: ", "param.summary.method_colon"},
        {"行: ", "param.summary.row_colon"},
        {"列: ", "param.summary.column_colon"},
        {"试验 = ", "param.summary.trials_eq"},
        {"比较 = ", "param.summary.comparison_eq"},
        {"变换 = ", "param.summary.transform_eq"},
        {"检验 = ", "param.summary.test_eq"},
        {"变量: ", "param.summary.variable_colon"},
        {"变量 = ", "param.summary.variable_eq"},
        {"响应 = ", "param.summary.response_eq"},
        {"方法 = ", "param.summary.method_eq"},
        {"测量 = ", "param.summary.measurement_short_eq"},
        {"事件 = ", "param.summary.events_eq"},
        {"缺陷 = ", "param.summary.defects_eq"},
        {"时间 = ", "param.summary.time_eq"},
        {"类别 = ", "param.summary.category_eq"},
        {"模式 = ", "param.summary.mode_eq"},
        {"任务 = ", "param.summary.task_eq"},
        {"效应: ", "param.summary.effect_colon"},
        {"模型 = ", "param.summary.model_eq"},
        {"因子 = ", "param.summary.factors_eq"},
        {"目标 = ", "param.summary.target_eq"},
    };
    // Longer tokens listed before shorter prefixes; Chinese producers → localize only.
    for (const auto& [zh, id] : tokens) {
        const std::string from = zh;
        const std::string to =
            domain::resolve_report_text(id, language, missing_out).text;
        if (from.empty() || from == to) {
            continue;
        }
        replace_all_inplace(summary, from, to);
    }
}

}  // namespace

ReportLocalizationResult localize_report_document(const domain::ReportDocument& document)
{
    ReportLocalizationResult result;
    result.document = document;
    const std::string language = document.profile.locale.language_tag;
    result.coverage = domain::report_text_coverage(language);

    // Touch key report strings so missing translations are diagnosed up front.
    const char* required_ids[] = {
        "report.title",
        "report.template",
        "report.language",
        "report.evidence_level",
        "report.generated_at",
        "report.source",
        "report.pdfa_banner",
        "template.customer",
        "template.engineer",
        "template.audit",
        "interp.excluded_rows",
        "interp.hidden_rows",
        "interp.section.conclusion",
        "interp.section.advice",
        "interp.section.limitations",
        "interp.section.key_risks",
        "interp.section.method",
        "table.warranty_summary",
        "table.warranty_failure_mode_denominators",
        "table.warranty_group_denominators",
        "table.pure_error_lof",
        "table.box_cox_capability",
        "table.johnson_transform",
        "plot.box_cox_lambda",
        "plot.johnson_transform",
        "table.property",
        "table.value",
        "interp.capability_stability_blocked",
        "interp.rsm_no_replicates_lof",
        "body.honesty.gap",
    };
    for (const char* id : required_ids) {
        domain::resolve_report_text(id, language, &result.missing_translations);
    }
    domain::localize_template_kind(
        document.profile.template_kind, language, &result.missing_translations);

    for (domain::ReportPageView& page : result.document.pages) {
        localize_page_title(
            page.source_page.title, language, &result.missing_translations);
        localize_visibility_limitation_bullets(
            page.visible_interpretation, language, &result.missing_translations);
        // Do not rewrite source_page.interpretation body wholesale — visible layer only
        // for tables/diagnostics; interpretation headings still localized on visible copy.
        localize_visible_tables(
            page.visible_tables, language, &result.missing_translations);
        localize_visible_plots(
            page.visible_plots, language, &result.missing_translations);
        localize_visible_diagnostics(
            page.visible_diagnostics, language, &result.missing_translations);
        localize_parameter_summary(
            page.source_page.parameter_summary, language, &result.missing_translations);
        if (!page.source_page.method_metadata.parameters.empty()) {
            localize_parameter_summary(
                page.source_page.method_metadata.parameters,
                language,
                &result.missing_translations);
        }
    }

    if (language == "en-US" || language == "en" || language.rfind("en-", 0) == 0) {
        domain::DiagnosticMessage honesty;
        honesty.severity = domain::DiagnosticMessage::Severity::info;
        honesty.code = "report_body_partial_bilingual";
        honesty.message =
            domain::resolve_report_text("body.honesty.gap", language).text;
        if (!result.document.pages.empty()) {
            result.document.pages.front().visible_diagnostics.push_back(honesty);
        }
    }
    if (!result.missing_translations.empty()) {
        domain::DiagnosticMessage diagnostic;
        diagnostic.severity = domain::DiagnosticMessage::Severity::warning;
        diagnostic.code = "missing_translation";
        diagnostic.message =
            domain::resolve_report_text("diag.missing_translation", language).text;
        for (const domain::MissingTranslation& missing : result.missing_translations) {
            diagnostic.message += " [" + missing.text_id + "→" + missing.fallback_language_tag
                + "]";
        }
        diagnostic.suggested_action =
            "Add stable catalog entry for report locale; do not mix languages silently.";
        if (!result.document.pages.empty()) {
            result.document.pages.front().visible_diagnostics.push_back(diagnostic);
            result.document.pages.front().source_page.diagnostics.push_back(diagnostic);
        }
    }

    // Ensure Facts fingerprint identity is unchanged by localization side effects:
    // we never rewrite InterpretationFacts numeric fields here.
    return result;
}

}  // namespace datalab::application
