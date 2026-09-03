# DataLab 学习中心 — Agent B 数据集映射表

> 生成日期：2026-09-03  
> 覆盖 command/help 并集：**184** 条  
> 权威计划（只读）：`docs/research/goal-learning-center-black-belt-plan.md`

## 多工作表接入方案（Agent C 必读）

### 现状（代码证据）

- `MainWindow` 仅持有一个 `table_` + `WorksheetModel`（`mainwindow.h`），**无** `QMap<工作表名, DataTable>`。
- `ProjectNavigator::add_worksheet()` 只在树节点追加名称，**不切换**底层数据。
- MES/数据库导入（`import_database` / `open_mes_tools`）会 `clear_contents()` 并 **替换** `table_`，且清空输出页 — **与学习中心的「不覆盖、不清输出」冲突**。

### 最小接缝（Agent C 实现，禁止静默丢数）

1. 在 `MainWindow` 增加 `std::map<std::string, datalab::domain::DataTable> worksheets_` 与 `std::string active_worksheet_`。
2. `import_learning_dataset(dataset_id)`：
   - 若 `table_` 非空且不在 `worksheets_` 中，先以当前名（或 `工作表_1`）存入 `worksheets_`。
   - 从 `LearningDatasetStore` 物化 `demo_<short>` 表，写入 `worksheets_`，设为 `active_worksheet_`，`display_table()`。
   - `navigator_->add_worksheet(demo名)`；**不**调用 `output_workspace_->clear_pages()`。
3. 导航器点击工作表（需补信号 `worksheet_activated(QString)`）：从 `worksheets_` 加载到 `table_` + model。
4. 若首版来不及做切换：至少 **导入前把当前表存入 map**，新表为活动表；旧表仍在 map 中可恢复（比单表覆盖安全）。

### SQLite / qrc 嵌入（Agent C）

- 库：`resources/help/learning_center.sqlite`
- 生成：`tools/build_learning_center_db.py` ← `tools/learning_data/*.csv` + `dataset_mapping.json`
- qrc：并入 `resources/help/learning_center.qrc` 或 `algorithm_help_resources`
- 连接名：`learning_center_<uuid>`，用完 `close()` + `removeDatabase()`

---

## 共享数据集定义

### `smt_paste_height` — SMT 锡膏印刷高度

- **行业**: electronics
- **故事**: 某 SMT 线体印刷后 3D 锡膏检测，按时间顺序记录高度，用于描述统计、正态性、I-MR、能力、直方图等。
- **行数**: 80
- **服务命令数**: 42

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `锡膏高度_um` | measurement | 3D 锡膏高度 |
| 1 | `产线` | factor | 产线 A/B |
| 2 | `班次` | factor | 早/中/晚班 |
| 3 | `检测时间` | time | ISO 时间戳 |
| 4 | `钢网编号` | factor | 钢网 ID |

<details><summary>服务的 command_id 列表</summary>

`area_plot`, `bar_chart`, `box_cox`, `bubble_plot`, `capability`, `capability_sixpack`, `chi_square_mosaic_link`, `correlation_plot`, `cusum`, `density_plot`, `descriptive`, `distribution_identification`, `dotplot`, `ecdf_plot`, `eda_4plot`, `ewma`, `graph_gallery`, `heatmap_plot`, `hexbin_plot`, `histogram`, `imr`, `imr_rs`, `interval_plot`, `marginal_plot`, `matrix_plot`, `mosaic_plot`, `multi_vari`, `nonnormal_capability`, `nonparametric_capability`, `normality_test`, `one_sample_t`, `one_sample_z`, `outlier_test`, `parallel_plot`, `pie_plot`, `probability_plot`, `run_chart`, `tolerance_intervals`, `variability_chart`, `violin_plot`, `z_mr`, `zone_chart`

</details>

### `two_line_thickness` — 两产线光学膜厚

- **行业**: electronics
- **故事**: 两条镀膜产线抽检膜厚，比较均值与方差、箱线图、双样本 t。
- **行数**: 60
- **服务命令数**: 10

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `膜厚_um` | measurement | μm |
| 1 | `产线` | factor | A 线 / B 线 |
| 2 | `抽检批次` | factor |  |
| 3 | `检测时间` | time |  |

<details><summary>服务的 command_id 列表</summary>

`batch_capability`, `boxplot`, `mann_whitney`, `one_proportion_equivalence`, `one_sample_equivalence`, `two_proportion_equivalence`, `two_sample_equivalence`, `two_sample_equivalence_ratio`, `two_sample_t`, `variance_test`

</details>

### `paired_rework` — 装配返工前后扭矩

- **行业**: assembly
- **故事**: 返工前后同一工件扭矩配对测量，用于 paired t / Wilcoxon / 符号检验。
- **行数**: 40
- **服务命令数**: 5

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `返工前扭矩_Nm` | measurement |  |
| 1 | `返工后扭矩_Nm` | measurement |  |
| 2 | `工件号` | id |  |
| 3 | `缺陷类型` | factor |  |

<details><summary>服务的 command_id 列表</summary>

`mcnemar`, `paired_equivalence`, `paired_t`, `sign_test`, `wilcoxon_signed_rank`

</details>

### `anova_cavity` — 注塑三模腔尺寸

- **行业**: molding
- **故事**: 三穴模具尺寸抽检，单因素 ANOVA、ANOM、Kruskal-Wallis、箱线图。
- **行数**: 90
- **服务命令数**: 8

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `模腔尺寸_mm` | measurement | mm |
| 1 | `模腔` | factor | 穴 1/2/3 |
| 2 | `材料批次` | factor |  |
| 3 | `机台号` | factor |  |

<details><summary>服务的 command_id 列表</summary>

`anom`, `between_within_capability`, `friedman`, `kruskal_wallis`, `mood_median`, `one_way_anova`, `xbar_r`, `xbar_s`

</details>

### `corr_temp_offset` — 回流温度与焊点偏移

- **行业**: electronics
- **故事**: 回流焊炉温与 AOI 焊点偏移，用于相关、回归、散点图、多变量图。
- **行数**: 55
- **服务命令数**: 31

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `炉温_℃` | measurement |  |
| 1 | `焊点偏移_um` | measurement |  |
| 2 | `链速_mm_min` | measurement |  |
| 3 | `产品型号` | factor |  |
| 4 | `轨道号` | factor |  |

<details><summary>服务的 command_id 列表</summary>

`best_subsets_regression`, `bootstrap_mean`, `bootstrap_two_sample`, `cart_tree`, `cluster_observations`, `cluster_variables`, `correlation`, `discriminant`, `factor_analysis`, `general_manova`, `generalized_variance`, `hotelling_t2`, `isolation_forest`, `kmeans`, `manova_one_way`, `mewma`, `mixed_effects_reml`, `multiple_correspondence`, `nominal_logistic`, `nonlinear_regression`, `ordinal_logistic`, `orthogonal_regression`, `pca`, `pls_regression`, `poisson_regression`, `random_forest`, `randomization_test`, `regression`, `scatter_plot`, `simple_correspondence`, `stepwise_regression`

</details>

### `attribute_defect` — 装配班次不良计数

- **行业**: assembly
- **故事**: 各班次抽检不良与缺陷分类，用于 p/np/c/u 图、卡方、帕累托、比例检验。
- **行数**: 48
- **服务命令数**: 23

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `班次` | factor |  |
| 1 | `检验数` | trials |  |
| 2 | `不良数` | events |  |
| 3 | `缺陷数` | defects |  |
| 4 | `缺陷类型` | category |  |
| 5 | `检验面积_dm2` | length |  |
| 6 | `产线` | factor |  |

<details><summary>服务的 command_id 列表</summary>

`anom_attribute`, `attribute_agreement`, `binomial_capability`, `c_chart`, `chi_square`, `chi_square_gof`, `cochran_q`, `cross_tabulation`, `fisher_exact`, `g_chart`, `laney_p_chart`, `laney_u_chart`, `logistic_regression`, `np_chart`, `one_poisson_rate`, `one_proportion`, `p_chart`, `pareto`, `poisson_capability`, `poisson_gof`, `two_poisson_rate`, `two_proportions`, `u_chart`

</details>

### `gage_rr_balance` — 三座标 MSA 交叉研究

- **行业**: msa
- **故事**: 10 零件 × 3 操作员 × 3 次重复测量，Gage R&R 系列教程。
- **行数**: 90
- **服务命令数**: 6

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `零件号` | part |  |
| 1 | `操作员` | operator |  |
| 2 | `重复序号` | replicate |  |
| 3 | `测量值_mm` | measurement | mm |

<details><summary>服务的 command_id 列表</summary>

`emp_crossed`, `expanded_gage_rr`, `expanded_gage_unbalanced`, `gage_rr`, `msa_type1`, `nested_gage_rr`

</details>

### `doe_factorial_demo` — 回流焊 2³ 析因试验

- **行业**: electronics
- **故事**: 温度×链速×氮气流量对焊点强度的全因子 DOE，含响应面与优化类命令。
- **行数**: 24
- **服务命令数**: 16

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `温度_℃` | factor |  |
| 1 | `链速_mm_min` | factor |  |
| 2 | `氮气流量_L_min` | factor |  |
| 3 | `响应_强度_MPa` | response |  |
| 4 | `响应_虚焊率` | response |  |
| 5 | `运行序号` | order |  |
| 6 | `成分_A_pct` | mixture |  |
| 7 | `成分_B_pct` | mixture |  |
| 8 | `成分_C_pct` | mixture |  |

<details><summary>服务的 command_id 列表</summary>

`analyze_definitive_screening`, `analyze_variability`, `binary_doe_probit`, `binary_response_doe`, `contour_plot`, `doe_response`, `glm_three_factor`, `glm_two_way`, `mixture_analyze`, `mixture_process_variable`, `response_optimization`, `rsm_response`, `simplex_design_plot`, `split_plot_analyze`, `taguchi_analyze`, `two_factor_anova`

</details>

### `reliability_cycles` — 电源模块寿命循环

- **行业**: reliability
- **故事**: 加速应力下循环至失效/删失，KM、Weibull、Cox、ALT 等可靠性教程。
- **行数**: 45
- **服务命令数**: 13

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `循环次数` | time |  |
| 1 | `失效状态` | event | 0=删失 1=失效 |
| 2 | `应力_V` | stress |  |
| 3 | `单元号` | id |  |
| 4 | `失效模式` | category |  |
| 5 | `事件时间_小时` | time |  |

<details><summary>服务的 command_id 列表</summary>

`accelerated_life`, `cox_counting_process`, `cox_regression`, `fine_gray_regression`, `km_interval`, `life_data_lognormal`, `life_data_regression`, `nhpp_repairable`, `probit_reliability`, `reliability`, `reliability_warranty`, `t_chart`, `weibayes`

</details>

### `ts_weekly_yield` — 周度装配良率

- **行业**: assembly
- **故事**: 52 周良率序列，用于时序图、平滑、分解、ARIMA、ADF、ACF。
- **行数**: 52
- **服务命令数**: 12

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `周次` | time |  |
| 1 | `良率_pct` | measurement |  |
| 2 | `产量_件` | measurement |  |
| 3 | `年份` | factor |  |

<details><summary>服务的 command_id 列表</summary>

`acf_pacf`, `adf_test`, `arima`, `ccf`, `correlogram`, `moving_average`, `runs_test`, `seasonal_forecasting`, `time_series_decomposition`, `time_series_plot`, `time_series_smoothing`, `trend_analysis`

</details>

---

## command_id → dataset_id → 角色映射（全集）

| command_id | dataset_id | 角色→列 | 工作表名 | 备注 |
|------------|------------|---------|----------|------|
| `area_plot` | `smt_paste_height` | `time`→`检测时间` | `demo_smt_paste_height` |  |
| `bar_chart` | `smt_paste_height` | `category`→`钢网编号` | `demo_smt_paste_height` |  |
| `boxplot` | `two_line_thickness` | `variables`→`膜厚_um` | `demo_two_line_thickness` |  |
| `bubble_plot` | `smt_paste_height` | — | `demo_smt_paste_height` |  |
| `chi_square_mosaic_link` | `smt_paste_height` | — | `demo_smt_paste_height` |  |
| `contour_plot` | `doe_factorial_demo` | — | `demo_doe_factorial_demo` |  |
| `correlation_plot` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `correlogram` | `ts_weekly_yield` | `variables`→`良率_pct` | `demo_ts_weekly_yield` |  |
| `density_plot` | `smt_paste_height` | — | `demo_smt_paste_height` |  |
| `dotplot` | `smt_paste_height` | — | `demo_smt_paste_height` |  |
| `ecdf_plot` | `smt_paste_height` | — | `demo_smt_paste_height` |  |
| `eda_4plot` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `graph_gallery` | `smt_paste_height` | — | `demo_smt_paste_height` |  |
| `heatmap_plot` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `hexbin_plot` | `smt_paste_height` | — | `demo_smt_paste_height` |  |
| `histogram` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `interval_plot` | `smt_paste_height` | — | `demo_smt_paste_height` |  |
| `marginal_plot` | `smt_paste_height` | — | `demo_smt_paste_height` |  |
| `matrix_plot` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `mosaic_plot` | `smt_paste_height` | — | `demo_smt_paste_height` |  |
| `parallel_plot` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `pie_plot` | `smt_paste_height` | `category`→`钢网编号` | `demo_smt_paste_height` |  |
| `probability_plot` | `smt_paste_height` | — | `demo_smt_paste_height` |  |
| `scatter_plot` | `corr_temp_offset` | — | `demo_corr_temp_offset` |  |
| `simplex_design_plot` | `doe_factorial_demo` | — | `demo_doe_factorial_demo` |  |
| `time_series_plot` | `ts_weekly_yield` | `time`→`周次` | `demo_ts_weekly_yield` |  |
| `violin_plot` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `special_cause_rules` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `c_chart` | `attribute_defect` | `defects`→`缺陷数` | `demo_attribute_defect` |  |
| `cusum` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `ewma` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `g_chart` | `attribute_defect` | `variables`→`不良数` | `demo_attribute_defect` |  |
| `generalized_variance` | `corr_temp_offset` | `variables`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `hotelling_t2` | `corr_temp_offset` | `variables`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `imr` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `imr_rs` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `laney_p_chart` | `attribute_defect` | — | `demo_attribute_defect` |  |
| `laney_u_chart` | `attribute_defect` | `defects`→`缺陷数` | `demo_attribute_defect` |  |
| `mewma` | `corr_temp_offset` | `variables`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `moving_average` | `ts_weekly_yield` | `variables`→`良率_pct` | `demo_ts_weekly_yield` |  |
| `np_chart` | `attribute_defect` | — | `demo_attribute_defect` |  |
| `p_chart` | `attribute_defect` | — | `demo_attribute_defect` |  |
| `t_chart` | `reliability_cycles` | `variables`→`循环次数` | `demo_reliability_cycles` |  |
| `u_chart` | `attribute_defect` | `defects`→`缺陷数` | `demo_attribute_defect` |  |
| `xbar_r` | `anova_cavity` | `variables`→`模腔尺寸_mm` | `demo_anova_cavity` |  |
| `xbar_s` | `anova_cavity` | `variables`→`模腔尺寸_mm` | `demo_anova_cavity` |  |
| `z_mr` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `zone_chart` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `database_import` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `report_templates` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `accelerated_life` | `reliability_cycles` | `time`→`循环次数` | `demo_reliability_cycles` |  |
| `acf_pacf` | `ts_weekly_yield` | `variables`→`良率_pct` | `demo_ts_weekly_yield` |  |
| `adf_test` | `ts_weekly_yield` | `variables`→`良率_pct` | `demo_ts_weekly_yield` |  |
| `analyze_definitive_screening` | `doe_factorial_demo` | `response`→`响应_强度_MPa` | `demo_doe_factorial_demo` |  |
| `analyze_variability` | `doe_factorial_demo` | `factors`→`温度_℃` | `demo_doe_factorial_demo` |  |
| `anom` | `anova_cavity` | `response`→`模腔尺寸_mm` | `demo_anova_cavity` |  |
| `anom_attribute` | `attribute_defect` | — | `demo_attribute_defect` |  |
| `arima` | `ts_weekly_yield` | `time`→`周次` | `demo_ts_weekly_yield` |  |
| `best_subsets_regression` | `corr_temp_offset` | `response`→`焊点偏移_um` | `demo_corr_temp_offset` |  |
| `binary_doe_probit` | `doe_factorial_demo` | `factors`→`温度_℃` | `demo_doe_factorial_demo` |  |
| `binary_response_doe` | `doe_factorial_demo` | `factors`→`温度_℃` | `demo_doe_factorial_demo` |  |
| `bootstrap_mean` | `corr_temp_offset` | `variables`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `bootstrap_two_sample` | `corr_temp_offset` | — | `demo_corr_temp_offset` |  |
| `cart_tree` | `corr_temp_offset` | `response`→`焊点偏移_um` | `demo_corr_temp_offset` |  |
| `ccf` | `ts_weekly_yield` | — | `demo_ts_weekly_yield` |  |
| `chi_square` | `attribute_defect` | — | `demo_attribute_defect` |  |
| `chi_square_gof` | `attribute_defect` | `category`→`缺陷类型` | `demo_attribute_defect` |  |
| `cluster_observations` | `corr_temp_offset` | `variables`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `cluster_variables` | `corr_temp_offset` | `variables`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `cochran_q` | `attribute_defect` | `variables`→`不良数` | `demo_attribute_defect` |  |
| `correlation` | `corr_temp_offset` | `variables`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `cox_counting_process` | `reliability_cycles` | — | `demo_reliability_cycles` |  |
| `cox_regression` | `reliability_cycles` | `time`→`循环次数` | `demo_reliability_cycles` |  |
| `cross_tabulation` | `attribute_defect` | — | `demo_attribute_defect` |  |
| `definitive_screening_design` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `descriptive` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `discriminant` | `corr_temp_offset` | `response`→`焊点偏移_um` | `demo_corr_temp_offset` |  |
| `distribution_calculator` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `doe_bbd` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `doe_ccd` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `doe_d_optimal` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `doe_factorial` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `doe_plackett_burman` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `doe_response` | `doe_factorial_demo` | `response`→`响应_强度_MPa` | `demo_doe_factorial_demo` |  |
| `expanded_gage_unbalanced` | `gage_rr_balance` | `measurement`→`测量值_mm` | `demo_gage_rr_balance` |  |
| `factor_analysis` | `corr_temp_offset` | `variables`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `fine_gray_regression` | `reliability_cycles` | `time`→`循环次数` | `demo_reliability_cycles` |  |
| `fisher_exact` | `attribute_defect` | `variables`→`不良数` | `demo_attribute_defect` |  |
| `friedman` | `anova_cavity` | `response`→`模腔尺寸_mm` | `demo_anova_cavity` |  |
| `general_manova` | `corr_temp_offset` | — | `demo_corr_temp_offset` |  |
| `glm_three_factor` | `doe_factorial_demo` | `response`→`响应_强度_MPa` | `demo_doe_factorial_demo` |  |
| `glm_two_way` | `doe_factorial_demo` | `response`→`响应_强度_MPa` | `demo_doe_factorial_demo` |  |
| `isolation_forest` | `corr_temp_offset` | `variables`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `km_interval` | `reliability_cycles` | — | `demo_reliability_cycles` |  |
| `kmeans` | `corr_temp_offset` | `variables`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `kruskal_wallis` | `anova_cavity` | `response`→`模腔尺寸_mm` | `demo_anova_cavity` |  |
| `life_data_lognormal` | `reliability_cycles` | `time`→`循环次数` | `demo_reliability_cycles` |  |
| `life_data_regression` | `reliability_cycles` | `time`→`循环次数` | `demo_reliability_cycles` |  |
| `logistic_regression` | `attribute_defect` | — | `demo_attribute_defect` |  |
| `mann_whitney` | `two_line_thickness` | `variables`→`膜厚_um` | `demo_two_line_thickness` |  |
| `manova_one_way` | `corr_temp_offset` | — | `demo_corr_temp_offset` |  |
| `mcnemar` | `paired_rework` | `variables`→`返工前扭矩_Nm`; `variables_second`→`返工后扭矩_Nm` | `demo_paired_rework` |  |
| `mixed_effects_reml` | `corr_temp_offset` | `response`→`焊点偏移_um` | `demo_corr_temp_offset` |  |
| `mixture_analyze` | `doe_factorial_demo` | — | `demo_doe_factorial_demo` |  |
| `mixture_design` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `mixture_extreme_vertices_design` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `mixture_process_variable` | `doe_factorial_demo` | — | `demo_doe_factorial_demo` |  |
| `mood_median` | `anova_cavity` | `response`→`模腔尺寸_mm` | `demo_anova_cavity` |  |
| `multiple_correspondence` | `corr_temp_offset` | `variables`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `nhpp_repairable` | `reliability_cycles` | `time`→`循环次数` | `demo_reliability_cycles` |  |
| `nominal_logistic` | `corr_temp_offset` | `response`→`焊点偏移_um` | `demo_corr_temp_offset` |  |
| `nonlinear_regression` | `corr_temp_offset` | `response`→`焊点偏移_um` | `demo_corr_temp_offset` |  |
| `normality_test` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `one_poisson_rate` | `attribute_defect` | `defects`→`缺陷数` | `demo_attribute_defect` |  |
| `one_proportion` | `attribute_defect` | `events`→`不良数` | `demo_attribute_defect` |  |
| `one_proportion_equivalence` | `two_line_thickness` | — | `demo_two_line_thickness` |  |
| `one_sample_equivalence` | `two_line_thickness` | `variables`→`膜厚_um` | `demo_two_line_thickness` |  |
| `one_sample_t` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `one_sample_z` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `one_way_anova` | `anova_cavity` | `response`→`模腔尺寸_mm` | `demo_anova_cavity` |  |
| `ordinal_logistic` | `corr_temp_offset` | `response`→`焊点偏移_um` | `demo_corr_temp_offset` |  |
| `orthogonal_regression` | `corr_temp_offset` | `x`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `outlier_test` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `paired_equivalence` | `paired_rework` | `variables`→`返工前扭矩_Nm`; `variables_second`→`返工后扭矩_Nm` | `demo_paired_rework` |  |
| `paired_t` | `paired_rework` | `variables`→`返工前扭矩_Nm`; `variables_second`→`返工后扭矩_Nm` | `demo_paired_rework` |  |
| `pca` | `corr_temp_offset` | `variables`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `pls_regression` | `corr_temp_offset` | `response`→`焊点偏移_um` | `demo_corr_temp_offset` |  |
| `poisson_gof` | `attribute_defect` | — | `demo_attribute_defect` |  |
| `poisson_regression` | `corr_temp_offset` | `response`→`焊点偏移_um` | `demo_corr_temp_offset` |  |
| `probit_reliability` | `reliability_cycles` | — | `demo_reliability_cycles` |  |
| `random_forest` | `corr_temp_offset` | `response`→`焊点偏移_um` | `demo_corr_temp_offset` |  |
| `randomization_test` | `corr_temp_offset` | — | `demo_corr_temp_offset` |  |
| `regression` | `corr_temp_offset` | `variables`→`炉温_℃` | `demo_corr_temp_offset` |  |
| `reliability` | `reliability_cycles` | `time`→`循环次数` | `demo_reliability_cycles` |  |
| `reliability_test_plan` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `reliability_warranty` | `reliability_cycles` | — | `demo_reliability_cycles` |  |
| `response_optimization` | `doe_factorial_demo` | `response`→`响应_强度_MPa` | `demo_doe_factorial_demo` |  |
| `rsm_response` | `doe_factorial_demo` | `response`→`响应_强度_MPa` | `demo_doe_factorial_demo` |  |
| `runs_test` | `ts_weekly_yield` | `variables`→`良率_pct` | `demo_ts_weekly_yield` |  |
| `seasonal_forecasting` | `ts_weekly_yield` | — | `demo_ts_weekly_yield` |  |
| `sign_test` | `paired_rework` | `variables`→`返工前扭矩_Nm`; `variables_second`→`返工后扭矩_Nm` | `demo_paired_rework` |  |
| `simple_correspondence` | `corr_temp_offset` | — | `demo_corr_temp_offset` |  |
| `split_plot_analyze` | `doe_factorial_demo` | `response`→`响应_强度_MPa` | `demo_doe_factorial_demo` |  |
| `split_plot_design` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `stepwise_regression` | `corr_temp_offset` | `response`→`焊点偏移_um` | `demo_corr_temp_offset` |  |
| `t_power` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `taguchi_analyze` | `doe_factorial_demo` | `factors`→`温度_℃` | `demo_doe_factorial_demo` |  |
| `taguchi_orthogonal_design` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `time_series_decomposition` | `ts_weekly_yield` | `time`→`周次` | `demo_ts_weekly_yield` |  |
| `time_series_smoothing` | `ts_weekly_yield` | `variables`→`良率_pct` | `demo_ts_weekly_yield` |  |
| `trend_analysis` | `ts_weekly_yield` | `time`→`周次` | `demo_ts_weekly_yield` |  |
| `two_factor_anova` | `doe_factorial_demo` | `response`→`响应_强度_MPa` | `demo_doe_factorial_demo` |  |
| `two_poisson_rate` | `attribute_defect` | `first_events`→`不良数` | `demo_attribute_defect` |  |
| `two_proportion_equivalence` | `two_line_thickness` | — | `demo_two_line_thickness` |  |
| `two_proportions` | `attribute_defect` | `first_events`→`不良数` | `demo_attribute_defect` |  |
| `two_sample_equivalence` | `two_line_thickness` | `variables`→`膜厚_um` | `demo_two_line_thickness` |  |
| `two_sample_equivalence_ratio` | `two_line_thickness` | `variables`→`膜厚_um` | `demo_two_line_thickness` |  |
| `two_sample_t` | `two_line_thickness` | `variables`→`膜厚_um`; `by`→`产线` | `demo_two_line_thickness` |  |
| `variance_test` | `two_line_thickness` | — | `demo_two_line_thickness` |  |
| `weibayes` | `reliability_cycles` | `time`→`循环次数` | `demo_reliability_cycles` |  |
| `wilcoxon_signed_rank` | `paired_rework` | `variables`→`返工前扭矩_Nm`; `variables_second`→`返工后扭矩_Nm` | `demo_paired_rework` |  |
| `acceptance_sampling` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `attribute_agreement` | `attribute_defect` | — | `demo_attribute_defect` |  |
| `batch_capability` | `two_line_thickness` | `measurement`→`膜厚_um` | `demo_two_line_thickness` |  |
| `between_within_capability` | `anova_cavity` | `variables`→`模腔尺寸_mm` | `demo_anova_cavity` |  |
| `binomial_capability` | `attribute_defect` | — | `demo_attribute_defect` |  |
| `box_cox` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `capability` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `capability_sixpack` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `cause_and_effect` | `—` | — | `—` | 无需导入：计算器/设计生成/编排参考，直接打开菜单 |
| `distribution_identification` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `emp_crossed` | `gage_rr_balance` | `measurement`→`测量值_mm` | `demo_gage_rr_balance` |  |
| `expanded_gage_rr` | `gage_rr_balance` | `measurement`→`测量值_mm` | `demo_gage_rr_balance` |  |
| `gage_rr` | `gage_rr_balance` | `measurement`→`测量值_mm` | `demo_gage_rr_balance` |  |
| `msa_type1` | `gage_rr_balance` | `measurement`→`测量值_mm` | `demo_gage_rr_balance` |  |
| `multi_vari` | `smt_paste_height` | `measurement`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `nested_gage_rr` | `gage_rr_balance` | `measurement`→`测量值_mm` | `demo_gage_rr_balance` |  |
| `nonnormal_capability` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `nonparametric_capability` | `smt_paste_height` | `measurement`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `pareto` | `attribute_defect` | `category`→`缺陷类型` | `demo_attribute_defect` |  |
| `poisson_capability` | `attribute_defect` | `defects`→`缺陷数` | `demo_attribute_defect` |  |
| `run_chart` | `smt_paste_height` | `variables`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `tolerance_intervals` | `smt_paste_height` | `measurement`→`锡膏高度_um` | `demo_smt_paste_height` |  |
| `variability_chart` | `smt_paste_height` | `measurement`→`锡膏高度_um` | `demo_smt_paste_height` |  |

## 审计

- 映射条目：184
- 数据集数量：10
- 无 dataset 的 implemented 需数据命令：0