# G9-D 验算轨迹深度矩阵

> 访问日期：2026-08-24（UTC+8）
> depth ∈ {L3, L2, L1, L0} · 权威命令源：`python tools/_list_command_ids.py`

| command_id | family | depth | notes |
|---|---|---|---|
| tests | 元命令 | L0 | 元命令豁免 |
| rule_policy | 元命令 | L0 | 元命令豁免 |
| descriptive | 基础统计 | L3 | Facts 优先分步验算 |
| normality_test | 基础统计 | L3 | Facts 优先分步验算 |
| outlier_test | 基础统计 | L3 | Facts 优先分步验算 |
| correlation | 基础统计 | L3 | Facts 优先分步验算 |
| one_sample_t | 基础统计 | L3 | Facts 优先分步验算 |
| one_sample_z | 基础统计 | L3 | Facts 优先分步验算 |
| one_proportion | 基础统计 | L3 | Facts 优先分步验算 |
| one_poisson_rate | 基础统计 | L3 | Facts 优先分步验算 |
| two_poisson_rate | 基础统计 | L3 | Facts 优先分步验算 |
| two_sample_t | 基础统计 | L3 | Facts 优先分步验算 |
| one_sample_equivalence | 基础统计 | L3 | Facts 优先分步验算 |
| two_sample_equivalence | 基础统计 | L3 | Facts 优先分步验算 |
| two_sample_equivalence_ratio | 基础统计 | L3 | Facts 优先分步验算 |
| paired_equivalence | 基础统计 | L3 | Facts 优先分步验算 |
| one_proportion_equivalence | 基础统计 | L3 | Facts 优先分步验算 |
| two_proportion_equivalence | 基础统计 | L3 | Facts 优先分步验算 |
| one_way_anova | 基础统计 | L3 | Facts 优先分步验算 |
| paired_t | 基础统计 | L3 | Facts 优先分步验算 |
| regression | 回归/多变量/ML | L3 | Facts 优先分步验算 |
| two_proportions | 基础统计 | L3 | Facts 优先分步验算 |
| chi_square | 基础统计 | L3 | Facts 优先分步验算 |
| cross_tabulation | 基础统计 | L3 | Facts 优先分步验算 |
| chi_square_gof | 基础统计 | L3 | Facts 优先分步验算 |
| poisson_gof | 基础统计 | L3 | Facts 优先分步验算 |
| anom | 基础统计 | L2 | 设计/生成规则代入验算 |
| mann_whitney | 基础统计 | L3 | Facts 优先分步验算 |
| wilcoxon_signed_rank | 基础统计 | L3 | Facts 优先分步验算 |
| sign_test | 基础统计 | L3 | Facts 优先分步验算 |
| runs_test | 基础统计 | L3 | Facts 优先分步验算 |
| mcnemar | 基础统计 | L3 | Facts 优先分步验算 |
| fisher_exact | 基础统计 | L3 | Facts 优先分步验算 |
| cochran_q | 基础统计 | L3 | Facts 优先分步验算 |
| mood_median | 基础统计 | L3 | Facts 优先分步验算 |
| kruskal_wallis | 基础统计 | L3 | Facts 优先分步验算 |
| friedman | 基础统计 | L3 | Facts 优先分步验算 |
| time_series_smoothing | 回归/多变量/ML | L2 | 指数平滑递推；展示 λ/水平一步代入 |
| arima | 回归/多变量/ML | L2 | ARIMA 拟合迭代；展示选定阶数参数代入 |
| two_factor_anova | 基础统计 | L3 | Facts 优先分步验算 |
| logistic_regression | 回归/多变量/ML | L3 | Facts 优先分步验算 |
| variance_test | 基础统计 | L3 | Facts 优先分步验算 |
| time_series_decomposition | 回归/多变量/ML | L2 | 分解滤波递推；展示季节/趋势摘要 |
| seasonal_forecasting | 回归/多变量/ML | L2 | 季节模型递推；展示参数一步代入 |
| pca | 回归/多变量/ML | L2 | 关键方程代入验算 |
| kmeans | 回归/多变量/ML | L2 | 关键方程代入验算 |
| cart_tree | 回归/多变量/ML | L2 | CART 分裂路径摘要；非全树逐步 |
| random_forest | 回归/多变量/ML | L2 | 袋装树路径摘要；非全树逐步 |
| adf_test | 回归/多变量/ML | L3 | Facts 优先分步验算 |
| poisson_regression | 回归/多变量/ML | L3 | Facts 优先分步验算 |
| isolation_forest | 回归/多变量/ML | L2 | 孤立路径摘要；非全路径逐步 |
| bootstrap_mean | 图形/工具 | L2 | Bootstrap 重采样路径全集不可稳定逐步展开 |
| distribution_calculator | 图形/工具 | L3 | 关键方程分步验算 |
| bootstrap_two_sample | 图形/工具 | L2 | Bootstrap 双样本重采样路径不可逐步展开 |
| probit_reliability | 可靠性 | L3 | Facts 优先分步验算 |
| cluster_observations | 回归/多变量/ML | L2 | 层次聚类树摘要 |
| ordinal_logistic | 回归/多变量/ML | L3 | Facts 优先分步验算 |
| nominal_logistic | 回归/多变量/ML | L3 | Facts 优先分步验算 |
| discriminant | 回归/多变量/ML | L2 | 关键方程代入验算 |
| ccf | 回归/多变量/ML | L2 | 互相关序列摘要 |
| correlogram | 回归/多变量/ML | L2 | 相关图序列摘要 |
| stepwise_regression | 回归/多变量/ML | L2 | 逐步选择迭代；展示最终系数代入 |
| best_subsets_regression | 回归/多变量/ML | L2 | 子集搜索组合爆炸；展示 Cp 最优子集 |
| km_interval | 可靠性 | L2 | Greenwood 递推区间；展示 KM 点估计代入 |
| doe_plackett_burman | DOE | L2 | 设计/生成规则代入验算 |
| taguchi_orthogonal_design | DOE | L3 | DOE 分析/设计规则分步验算 |
| doe_ccd | DOE | L2 | 设计/生成规则代入验算 |
| doe_bbd | DOE | L2 | 设计/生成规则代入验算 |
| reliability | 可靠性 | L3 | Facts 优先分步验算 |
| accelerated_life | 可靠性 | L3 | Facts 优先分步验算 |
| reliability_warranty | 可靠性 | L3 | Facts 优先分步验算 |
| t_power | 图形/工具 | L3 | 关键方程分步验算 |
| acf_pacf | 回归/多变量/ML | L2 | 自相关序列摘要；非逐步谱分解 |
| histogram | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| eda_4plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| boxplot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| pareto | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| run_chart | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| cause_and_effect | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| density_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| hexbin_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| violin_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| bar_chart | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| scatter_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| interval_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| correlation_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| bubble_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| probability_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| ecdf_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| matrix_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| marginal_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| parallel_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| heatmap_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| time_series_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| area_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| contour_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| pie_plot | 图形/工具 | L1 | 图形显示摘要；真实 N/规则 |
| imr | 控制图 | L3 | Facts 优先分步验算 |
| xbar_r | 控制图 | L3 | Facts 优先分步验算 |
| xbar_s | 控制图 | L3 | Facts 优先分步验算 |
| imr_rs | 控制图 | L3 | Facts 优先分步验算 |
| p_chart | 控制图 | L3 | Facts 优先分步验算 |
| np_chart | 控制图 | L3 | Facts 优先分步验算 |
| c_chart | 控制图 | L3 | Facts 优先分步验算 |
| u_chart | 控制图 | L3 | Facts 优先分步验算 |
| laney_p_chart | 控制图 | L3 | Facts 优先分步验算 |
| laney_u_chart | 控制图 | L3 | Facts 优先分步验算 |
| ewma | 控制图 | L3 | Facts 优先分步验算 |
| hotelling_t2 | 控制图 | L3 | 关键方程分步验算 |
| mewma | 控制图 | L3 | Facts 优先分步验算 |
| generalized_variance | 控制图 | L3 | Facts 优先分步验算 |
| cusum | 控制图 | L3 | Facts 优先分步验算 |
| zone_chart | 控制图 | L3 | Facts 优先分步验算 |
| z_mr | 控制图 | L3 | Facts 优先分步验算 |
| moving_average | 控制图 | L3 | Facts 优先分步验算 |
| g_chart | 控制图 | L3 | Facts 优先分步验算 |
| t_chart | 控制图 | L3 | Facts 优先分步验算 |
| capability | 能力/质量 | L3 | Facts 优先分步验算 |
| multi_vari | 能力/质量 | L2 | 设计/生成规则代入验算 |
| variability_chart | 能力/质量 | L1 | 图形显示摘要；真实 N/规则 |
| acceptance_sampling | 能力/质量 | L2 | 设计/生成规则代入验算 |
| tolerance_intervals | 能力/质量 | L3 | Facts 优先分步验算 |
| distribution_identification | 能力/质量 | L2 | 设计/生成规则代入验算 |
| between_within_capability | 能力/质量 | L3 | Facts 优先分步验算 |
| batch_capability | 能力/质量 | L3 | Facts 优先分步验算 |
| nonparametric_capability | 能力/质量 | L3 | Facts 优先分步验算 |
| cox_regression | 可靠性 | L3 | Facts 优先分步验算 |
| weibayes | 可靠性 | L3 | Facts 优先分步验算 |
| binomial_capability | 能力/质量 | L3 | Facts 优先分步验算 |
| poisson_capability | 能力/质量 | L3 | Facts 优先分步验算 |
| nonnormal_capability | 能力/质量 | L3 | Facts 优先分步验算 |
| capability_sixpack | 能力/质量 | L3 | Facts 优先分步验算 |
| box_cox | 能力/质量 | L2 | 设计/生成规则代入验算 |
| gage_rr | MSA | L3 | Facts 优先分步验算 |
| emp_crossed | MSA | L3 | Facts 优先分步验算 |
| expanded_gage_rr | MSA | L3 | Facts 优先分步验算 |
| msa_type1 | MSA | L3 | Facts 优先分步验算 |
| nested_gage_rr | MSA | L3 | Facts 优先分步验算 |
| attribute_agreement | MSA | L3 | Facts 优先分步验算 |
| doe_factorial | DOE | L3 | DOE 分析/设计规则分步验算 |
| doe_response | DOE | L3 | DOE 分析/设计规则分步验算 |
| rsm_response | DOE | L3 | DOE 分析/设计规则分步验算 |
| response_optimization | DOE | L2 | 设计/生成规则代入验算 |
