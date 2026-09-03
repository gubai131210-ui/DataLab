# 学习中心教学升级 — Agent2 Wave 计划（锁表）

> **岗**：Agent2 Plan（**禁止改产品代码**；本文件 + inventory JSON 除外）  
> **日期**：2026-09-03  
> **权威手册**：[`goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md`](goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md)  
> **调研总册**：[`learning-center-pedagogy-upgrade-research.md`](learning-center-pedagogy-upgrade-research.md)  
> **模型**：inherit（禁止建议换模型）  
> **机器锁表**：[`_tmp_command_inventory.json`](_tmp_command_inventory.json)（`analysis_commands::all()` ∪ `algorithm_help.json` = **184**）

---

## 文首锁定（对照手册 §0 / §0.1；旧 black-belt「强制共享」作废）

- **数据集策略以本手册为准；旧 10 表删除重建**（覆盖旧 black-belt「强制共享」）。
- **catalog 双写 `learning-center-v2`**：`tools/build_learning_center_db.py` 的 `META_VERSION` **与** `LearningDatasetStore::kExpectedCatalogVersion` 必须同时改；只改 sqlite 会让页面报版本不匹配并禁用导入。
- **`dataset_id` 不以 `demo_` 开头**。工作表公式是 `demo_{dataset_id}`。金标主键 **`imr_spi_shift`** → 显示名 `demo_imr_spi_shift`。Canvas 里的 `demo_imr_spi_shift` 是工作表名，不是 sqlite 主键。
- **`dialog_fill` 并列 `dialog_fill_detail`**。`dialog_fill` 保持角色→列名 JSON **object**（供现网 `parse_dialog_fill`）；详解走并列数组。禁止用详解数组覆盖 `dialog_fill`。

### §0 用户拍板（禁止子 Agent 改口）

| # | 锁定 |
|---|------|
| Q1 | inherit 当前对话模型；全程不换 |
| Q2 | **本 Goal 一次铺开全集 184 id**；Wave 只是施工队列，金标是闸门不是终点 |
| Q3 | **UI 一次做完**（0–6 分节折叠 + 7+ 练习区可交互展示） |
| Q4 | 默认一主命令一表；同构极小族白名单（本文件 §3） |
| Q5 | 删除旧 10 共享表并重建 |
| Q6 | Agent 跑 Python verify；用户本机 Qt Creator / `package_dist` 自测 |

### 旧 10 表（禁止残留为 dataset_id / 生成器键 / 测试期望）

`smt_paste_height`, `two_line_thickness`, `paired_rework`, `anova_cavity`, `corr_temp_offset`, `attribute_defect`, `gage_rr_balance`, `doe_factorial_demo`, `reliability_cycles`, `ts_weekly_yield`

旧 [`learning-center-dataset-mapping.md`](learning-center-dataset-mapping.md) 的 mapping **作废**，Wave-5 整篇按新锁表重写。

### 全集计数（不得截断）

| 源 | 数量 |
|----|------|
| `analysis_commands` | 181 |
| `algorithm_help.json` | 183 |
| **并集 tutorials 必须 =** | **184** |
| help-only | `database_import`, `report_templates`, `special_cause_rules` |
| command-only | `reliability_warranty` |
| 计划专用+练习 dataset | **93**（含金标练习表 `imr_spi_spike_b`） |

并集分波：**Wave-0=1，Wave-1=20，Wave-2=24，Wave-3=56，Wave-4=83，Wave-5=0（清理）**。1+20+24+56+83=184。

---

## 1. Wave-0…5 可勾选总表

| Wave | 施工内容 | command_id 数 | 出口 |
|------|----------|---------------|------|
| **Wave-0** | Schema + catalog 解析 + **完整教学 UI** + **仅 `imr` / `imr_spi_shift` 金标**（另生成练习表 `imr_spi_spike_b`，不映射第二命令） | 1 | Agent4+6 过金标后**立即** Wave-1 |
| **Wave-1** | 控制图包（除金标 `imr`）+ `special_cause_rules` | 20 | gate |
| **Wave-2** | 质量工具 / MSA / 能力（含菜单在「统计」的 `expanded_gage_unbalanced`） | 24 | gate |
| **Wave-3** | 统计推断 / ANOVA / 回归相关 | 56 | gate |
| **Wave-4** | 图形 + DOE + 可靠性 + 其余 | 83 | gate |
| **Wave-5** | 删除旧 10 表残留（文档/注释/测试字符串）+ mapping md 重写 + package 说明 | 0 新 id | Agent5+6 |

**Wave-0 期间**：sqlite **重建**后其余 183 条 tutorial 仍入库（`used_for`/`not_for`/`scenario`/`click_steps≥2` 保留），**`dataset_id` 一律空**，直到各内容 Wave 填锁表。禁止把 183 条临时挂到 `imr_spi_shift`。

---

### Wave-0 可勾选 DoD（闸门）

- [ ] `META_VERSION` 与 `kExpectedCatalogVersion` **双写** `learning-center-v2`
- [ ] `tutorials` 增量列已进 builder CREATE（见 §4）；C++ 缺列降级为空
- [ ] `LearningTutorialEntry` 扩字段；`dialog_fill` 仍为 object→`QMap`
- [ ] **完整教学 UI**：分节折叠（§7）+ glossary / dialog 详解 / 埋点 / 误用 / 7+ 可交互；**不**推倒导航树与导入按钮
- [ ] 金标 `imr` → `imr_spi_shift`（工作表 `demo_imr_spi_shift`）；列 `片号` / `锡膏高度_um` / `时段备注`；埋点片 **41 阶跃、55 尖峰**
- [ ] `dialog_fill` = `{"variables":"锡膏高度_um"}`（**省略** `stage` 键，不要空串）
- [ ] `dialog_fill_detail` 覆盖全部 **9** 个真实对话框字段（调研 §5.2；含 Nelson estimate / `rule_policy` / `tests` / 双历史限）
- [ ] glossary ≥3 且含 **UCL≠USL**；7+ 五块非空；`skill_mission` 非空
- [ ] 练习表 `imr_spi_spike_b` 已生成（尖峰行号与金标不同）；fade 独立练习引用它
- [ ] 旧 10 生成器/CSV **已删除**；测试「恰好 10 dataset」**已改掉**
- [ ] Python gate PASS（Wave-0 阈值：仅金标强制 glossary/buried；其余 id 尚未灌水）
- [ ] **禁止**金标未过就批量写 Wave-1…4 文案

### Wave-1…4 每波共同 DoD

- [ ] 该 Wave **锁表每一个** `command_id` 均有 tutorial 行（不得跳 id）
- [ ] implemented 且锁表有 dataset：生成器存在；`notes`+`buried_signals` 写清行号；列名业务化；2–6 列；30–200 行
- [ ] implemented：`dialog_fill` 只含真实 **role id**；`dialog_fill_detail` 覆盖该命令全部 roles+inputs（对照 `analysis_commands.cpp`，禁止虚构菜单）
- [ ] 该 Wave 全部 id：glossary ≥3（控制限课必含 UCL≠USL）或显式 `related_ids` 指向已有课且 glossary 至少 1 条说明「术语见关联课」——**优先每课 ≥3**
- [ ] formula_reference / 空 dataset：步骤标明菜单可能不可用；**不**为术语课新增假 command_id
- [ ] 导入仍走 `WorksheetRegistry::import_new`；不泄漏 QSql 连接
- [ ] 无旧 10 id；无 `dataset_id` 以 `demo_` 开头；同构共享仅 §3 白名单
- [ ] `python tools/verify_learning_center_gate.py` PASS

### Wave-5 可勾选 DoD

- [x] 全库 grep：旧 10 id 不出现在 `tools/learning_data/**`、`tools/build_learning_*.py`、`tests/learning_center_*.cpp`（verify 的 **banned 列表本身除外**）
- [x] `docs/research/learning-center-dataset-mapping.md` 按本锁表重写
- [x] `tools/dist_readme.txt` 说明 catalog v2、专用集、工作表 `demo_{id}`
- [x] 不新开功能；交给 Agent5/6

---

## 2. 各 Wave 完整 command_id 锁表

机器可读副本：`docs/research/_tmp_command_inventory.json` → `waves` / `dataset_by_command` / `lock_rows`。  
**dataset_id 空** = 无需表（`requires_data=false`、formula_reference、help-only、orchestration/graph_reference）。  
**禁止**把空改成挂旧宽表。

### Wave-0 command_id 锁表（n=1）

| command_id | dataset_id |
|---|---|
| `imr` | `imr_spi_shift` |

附加（非 command）：练习集 `imr_spi_spike_b`（只进 `datasets`，fade 文案引用）。

### Wave-1 command_id 锁表（n=20）

| command_id | dataset_id |
|---|---|
| `c_chart` | `c_chart_defect_step` |
| `cusum` | `spc_small_drift` |
| `ewma` | `spc_small_drift` |
| `g_chart` | `g_chart_gap_days` |
| `generalized_variance` | `genvar_two_var` |
| `hotelling_t2` | `t2_two_var_shift` |
| `imr_rs` | `imr_rs_subgroup_shift` |
| `laney_p_chart` | `laney_p_overdispersed` |
| `laney_u_chart` | `laney_u_overdispersed` |
| `mewma` | `mewma_two_var_drift` |
| `moving_average` | `ma_small_drift` |
| `np_chart` | `np_chart_const_n_step` |
| `p_chart` | `p_chart_variable_n_step` |
| `special_cause_rules` | （空） |
| `t_chart` | `t_chart_time_interval` |
| `u_chart` | `u_chart_variable_unit_step` |
| `xbar_r` | `xbar_r_n5_range_spike` |
| `xbar_s` | `xbar_s_n8_sd_shift` |
| `z_mr` | `z_mr_short_run` |
| `zone_chart` | `zone_chart_runs` |

### Wave-2 command_id 锁表（n=24）

| command_id | dataset_id |
|---|---|
| `acceptance_sampling` | （空） |
| `attribute_agreement` | （空） |
| `batch_capability` | （空） |
| `between_within_capability` | `cap_between_within` |
| `binomial_capability` | `cap_binomial_lots` |
| `box_cox` | `dist_skew_boxcox` |
| `capability` | `cap_stable_spec` |
| `capability_sixpack` | `cap_stable_spec` |
| `cause_and_effect` | `fishbone_solder_causes` |
| `distribution_identification` | `dist_id_candidates` |
| `emp_crossed` | `msa_crossed_aiag` |
| `expanded_gage_rr` | `msa_expanded_crossed` |
| `expanded_gage_unbalanced` | （空） |
| `gage_rr` | `msa_crossed_aiag` |
| `msa_type1` | `msa_type1_ref` |
| `multi_vari` | `multi_vari_pos_time` |
| `nested_gage_rr` | `msa_nested_operator` |
| `nonnormal_capability` | （空） |
| `nonparametric_capability` | （空） |
| `pareto` | `pareto_defect_tail` |
| `poisson_capability` | `cap_poisson_counts` |
| `run_chart` | `run_chart_median_trend` |
| `tolerance_intervals` | （空） |
| `variability_chart` | （空） |

### Wave-3 command_id 锁表（n=56）

| command_id | dataset_id |
|---|---|
| `anom` | （空） |
| `anom_attribute` | （空） |
| `best_subsets_regression` | （空） |
| `bootstrap_mean` | （空） |
| `bootstrap_two_sample` | （空） |
| `chi_square` | `cat_shift_line` |
| `chi_square_gof` | `gof_category_bias` |
| `cochran_q` | `cochran_three_repeat` |
| `correlation` | `corr_temp_offset_y` |
| `cross_tabulation` | `cat_shift_line` |
| `descriptive` | `desc_unimodal_stable` |
| `fisher_exact` | `fisher_small_counts` |
| `friedman` | `friedman_three_treat` |
| `general_manova` | （空） |
| `glm_three_factor` | （空） |
| `glm_two_way` | （空） |
| `kruskal_wallis` | `kw_three_cavity` |
| `logistic_regression` | `logit_pass_fail` |
| `mann_whitney` | `infer_two_sample_location` |
| `manova_one_way` | （空） |
| `mcnemar` | `mcnemar_paired_binary` |
| `mixed_effects_reml` | （空） |
| `mood_median` | `mood_two_group` |
| `nominal_logistic` | （空） |
| `nonlinear_regression` | （空） |
| `normality_test` | `norm_mild_skew` |
| `one_poisson_rate` | `pois_one_count` |
| `one_proportion` | `prop_one_lot` |
| `one_proportion_equivalence` | `equiv_prop_one` |
| `one_sample_equivalence` | `equiv_one_near_target` |
| `one_sample_t` | `infer_one_sample_mean` |
| `one_sample_z` | `infer_one_sample_mean` |
| `one_way_anova` | `anova_one_cavity` |
| `ordinal_logistic` | （空） |
| `orthogonal_regression` | （空） |
| `outlier_test` | `outlier_one_spike` |
| `paired_equivalence` | `equiv_paired_near` |
| `paired_t` | `infer_paired_shift` |
| `pls_regression` | （空） |
| `poisson_gof` | （空） |
| `poisson_regression` | （空） |
| `randomization_test` | （空） |
| `regression` | `regr_temp_strength` |
| `runs_test` | `runs_clustered` |
| `sign_test` | `infer_paired_shift` |
| `stepwise_regression` | （空） |
| `t_power` | （空） |
| `two_factor_anova` | `anova_two_factor` |
| `two_poisson_rate` | `pois_two_count` |
| `two_proportion_equivalence` | `equiv_prop_two` |
| `two_proportions` | `prop_two_line` |
| `two_sample_equivalence` | `equiv_two_near_equal` |
| `two_sample_equivalence_ratio` | `equiv_ratio_near_one` |
| `two_sample_t` | `infer_two_sample_location` |
| `variance_test` | `var_two_line_unequal` |
| `wilcoxon_signed_rank` | `infer_paired_shift` |

### Wave-4 command_id 锁表（n=83）

| command_id | dataset_id |
|---|---|
| `accelerated_life` | （空） |
| `acf_pacf` | （空） |
| `adf_test` | （空） |
| `analyze_definitive_screening` | （空） |
| `analyze_variability` | （空） |
| `area_plot` | `graph_area_time` |
| `arima` | （空） |
| `bar_chart` | `graph_bar_category` |
| `binary_doe_probit` | （空） |
| `binary_response_doe` | （空） |
| `boxplot` | `graph_two_group_box` |
| `bubble_plot` | `graph_bubble_xyz` |
| `cart_tree` | （空） |
| `ccf` | （空） |
| `chi_square_mosaic_link` | （空） |
| `cluster_observations` | （空） |
| `cluster_variables` | （空） |
| `contour_plot` | `graph_contour_xy` |
| `correlation_plot` | `graph_corr_matrix` |
| `correlogram` | （空） |
| `cox_counting_process` | （空） |
| `cox_regression` | （空） |
| `database_import` | （空） |
| `definitive_screening_design` | （空） |
| `density_plot` | `graph_density_unimodal` |
| `discriminant` | （空） |
| `distribution_calculator` | （空） |
| `doe_bbd` | （空） |
| `doe_ccd` | （空） |
| `doe_d_optimal` | （空） |
| `doe_factorial` | （空） |
| `doe_plackett_burman` | （空） |
| `doe_response` | `doe_factorial_y` |
| `dotplot` | （空） |
| `ecdf_plot` | `graph_ecdf_unimodal` |
| `eda_4plot` | `graph_eda4_series` |
| `factor_analysis` | （空） |
| `fine_gray_regression` | （空） |
| `graph_gallery` | （空） |
| `heatmap_plot` | `graph_heatmap_matrix` |
| `hexbin_plot` | `graph_hexbin_xy` |
| `histogram` | `graph_hist_prob` |
| `interval_plot` | `graph_interval_groups` |
| `isolation_forest` | （空） |
| `km_interval` | （空） |
| `kmeans` | （空） |
| `life_data_lognormal` | （空） |
| `life_data_regression` | （空） |
| `marginal_plot` | `graph_marginal_xy` |
| `matrix_plot` | `graph_matrix_three` |
| `mixture_analyze` | （空） |
| `mixture_design` | （空） |
| `mixture_extreme_vertices_design` | （空） |
| `mixture_process_variable` | （空） |
| `mosaic_plot` | `graph_mosaic_two_cat` |
| `multiple_correspondence` | （空） |
| `nhpp_repairable` | （空） |
| `parallel_plot` | `graph_parallel_multi` |
| `pca` | `pca_three_var` |
| `pie_plot` | `graph_pie_category` |
| `probability_plot` | `graph_hist_prob` |
| `probit_reliability` | （空） |
| `random_forest` | （空） |
| `reliability` | （空） |
| `reliability_test_plan` | （空） |
| `reliability_warranty` | `rel_warranty_counts` |
| `report_templates` | （空） |
| `response_optimization` | `doe_opt_two_resp` |
| `rsm_response` | （空） |
| `scatter_plot` | `graph_scatter_xy` |
| `seasonal_forecasting` | （空） |
| `simple_correspondence` | （空） |
| `simplex_design_plot` | `mix_simplex_3` |
| `split_plot_analyze` | （空） |
| `split_plot_design` | （空） |
| `taguchi_analyze` | （空） |
| `taguchi_orthogonal_design` | （空） |
| `time_series_decomposition` | `ts_decomp_seasonal` |
| `time_series_plot` | `ts_weekly_yield_series` |
| `time_series_smoothing` | `ts_smooth_weekly` |
| `trend_analysis` | （空） |
| `violin_plot` | `graph_violin_groups` |
| `weibayes` | （空） |

### Wave-5

无新 `command_id`。只清残留与文档。

---

## 3. 同构共享白名单（极小族；默认仍一命令一表）

只允许下表 **8 族 / 17 个 command_id**。族大小 ≤3。verify：**凡被 >1 条 tutorial 引用的 dataset_id 必须出现在本白名单**；禁止再扩，除非修订本 plan 并论证埋点仍成立。

| family_id / dataset_id | 服务 command_id | 为何同构 | 埋点是否仍成立 |
|------------------------|-----------------|----------|----------------|
| `msa_crossed_aiag` | `gage_rr`, `emp_crossed` | 交叉 MSA 三角色 `measurement`/`part`/`operator` 相同；`lsl`/`usl` 只在 gage_rr 对话框 | **是**：10×3×3 平衡 + 一名操作员偏倚 + 零件覆盖过程范围。禁止挂 nested / 属性一致性 / 能力规格列 |
| `cap_stable_spec` | `capability`, `capability_sixpack` | 同一稳定略偏心单值 Y；规格在对话框；Sixpack 是能力课诊断包装 | **是**：必须**无**片 41/55 失控信号；轻微偏心使 Cpk&lt;Cp。禁止与 I-MR 金标 / Box-Cox 偏态集共享 |
| `spc_small_drift` | `ewma`, `cusum` | 单值 Y；教学信号都是**微小持续漂移**（不是尖峰） | **是**：前段稳定 + 中后段小台阶。禁止复用 `imr_spi_shift`。`moving_average` **不**进本族 |
| `infer_one_sample_mean` | `one_sample_t`, `one_sample_z` | 单列 Y 相对假设均值偏移；已知 σ 只在 z 的对话框 | **是**：均值相对目标可检出偏移。禁止与 equivalence（TOST 近目标）共享 |
| `infer_paired_shift` | `paired_t`, `wilcoxon_signed_rank`, `sign_test` | 配对两列、同一前后差设计 | **是**：返工前后位置差。禁止挂 `paired_equivalence` |
| `infer_two_sample_location` | `two_sample_t`, `mann_whitney` | 两独立样本位置差；本软件双样本 t 为**两列** | **是**：两线均值差约 0.8–1.5σ、方差接近。禁止挂 `variance_test` / equivalence |
| `cat_shift_line` | `chi_square`, `cross_tabulation` | 同一张两类别列联表 | **是**：班次×缺陷类型有关联结构。禁止当控制图用 |
| `graph_hist_prob` | `histogram`, `probability_plot` | 同一单变量形状课 | **是**：近正态或轻微偏态、无 SPC 特殊原因。**禁止**扩到 density/ecdf/violin/dotplot |

### 明确拒绝的共享（Agent3 禁止「顺手合并」）

| 候选 | 拒绝原因 |
|------|----------|
| `xbar_r` ∪ `xbar_s` | 子组大小与 R vs S 教学不同；调研默认分开 |
| `p_chart` ∪ `np_chart` | 可变 n 教 p 限宽窄；np 课需要近似恒定 n |
| `p_chart` ∪ `laney_p_chart` | Laney 要过离散；普通 P 图台阶即可 |
| `u_chart` ∪ `laney_u_chart` | 同上 |
| `imr` ∪ 任何能力/直方图/t 检验 | 金标失控集算 Cpk 踩 §8.9 |
| `gage_rr` ∪ `nested_gage_rr` | 交叉 vs 嵌套结构不同 |
| 直方图族扩到 density/ecdf/violin | 变相大宽表 |
| 三条时间序列图共用一张周产量表 | 默认一表一命令（生成器可克隆配方，**id 必须分开**） |

---

## 4. Schema 增量 DDL / JSON 形状与兼容（对照调研 §2）

Builder **每次 unlink 后 CREATE**（不是对旧文件 ALTER）。C++ 解析器必须 `PRAGMA table_info(tutorials)`：缺新列 → 字段空，旧库不崩溃。嵌入库升级靠重建 sqlite + 双写 v2；用户导出的 v1 仍走「版本不匹配、禁用导入」（H1，保持）。

### 4.1 `meta`

```text
catalog_version = "learning-center-v2"   -- 字符串，不是整数 +1
generated_at, source_git 保留
```

### 4.2 `datasets`（列名不改；强化 notes）

现有：`dataset_id, title, industry, story, row_count, notes`  
`notes` **必须**含：埋了什么、哪一行（1-based，与片号一致时写片号）、图上期望什么。  
可选：`SELECT notes` 进 `LearningDatasetSummary` 供 §2 UI；**禁止**把全部 cells 常驻巨大 `QVector`。

### 4.3 `tutorials` 新增 TEXT 列（保留全部旧列名，含 `research_sources`）

```text
-- 旧列一律保留：
command_id, title, category, menu_path, implemented_status,
used_for, not_for, scenario, dataset_id,
click_steps, dialog_fill, output_guide, common_mistakes,
related_ids, research_sources

-- 新增：
glossary            TEXT  -- JSON [{term, plain, remember}]
dialog_fill_detail  TEXT  -- JSON [{field, put, meaning}]
buried_signals      TEXT  -- JSON [{row, what, expect}]   row=1-based
prereq_quiz         TEXT  -- JSON [{q, good, bad}]
self_explain        TEXT  -- JSON [{after, prompt}]
fade_levels         TEXT  -- JSON [{level, student, scaffold}]
retrieval_quiz      TEXT  -- JSON [string]
misconceptions      TEXT  -- JSON [{wrong, right}]
skill_mission       TEXT  -- 纯文本
```

`dataset_columns` / `dataset_cells` 不变。

### 4.4 JSON 形状（金标约束）

**`dialog_fill`**（H7）：JSON **object**，键 = `RoleSpec.id`，值 = 列名 string。  
- 金标：`{"variables":"锡膏高度_um"}`  
- 可选角色留空：**省略该键**（verify：禁止空字符串值）  
- **不要**把 input id 塞进 `dialog_fill`（现网 `parse_dialog_fill` 只服务角色映射）

**`dialog_fill_detail`**：数组。`field` 用「对话框 label (`id`)」。金标 9 行见调研 §5.2（变量、阶段列、移动极差长度、σ 方法、Nelson estimate、规则默认策略、特殊原因测试、历史均值、历史 Sigma）。

**`buried_signals`**：金标至少两条：`row=41` 阶跃、`row=55` 尖峰；`expect` 对着 I/MR 读图，禁止「过程合格」。

**`fade_levels.level`**：`0` 完整例，`1` 完成题（省略最后步），`2` 独立练（换 `imr_spi_spike_b`）。

### 4.5 C++ 类型（`learning_types.h`）

新增 struct：`LearningGlossaryItem`、`LearningDialogFillDetail`、`LearningBuriedSignal`、`LearningPrereqItem`、`LearningSelfExplain`、`LearningFadeLevel`、`LearningMisconception`。  
`LearningTutorialEntry` 追加对应 `QVector` / `QStringList` / `QString skill_mission`。  
**保持** `QMap<QString,QString> dialog_fill`。

`learning_tutorial_catalog.cpp`：`SELECT` 用 PRAGMA 拼列；解析 JSON 失败 → 空；**禁止**把 detail 数组写入 `dialog_fill`。

### 4.6 生成器覆盖层（H3）

无独立 `tutorials.json` 主源。  
建议：`tools/learning_data/tutorial_overlays/<command_id>.json` 由 `build_tutorial_row()` merge。  
Wave-0 **只允许**完整 overlay：`imr.json`。禁止只手改 sqlite。

---

## 5. 精确到路径的文件改动清单

### 5.1 Agent3 允许改（按波）

**Wave-0（schema / 金标 / UI / 拆掉旧 10 表）**

| 路径 | 改什么 |
|------|--------|
| `tools/build_learning_center_db.py` | `META_VERSION=v2`；新列 DDL；删除旧 10 `GENERATORS`；加入 `imr_spi_shift` / `imr_spi_spike_b`；overlay merge |
| `tools/build_learning_dataset_mapping.py` | **整段替换**默认 `smt_paste_height`；写出本锁表 |
| `tools/learning_data/dataset_mapping.json` | 184 条 mappings；Wave-0 仅 `imr` 有 dataset，其余先空，后续 Wave 填满 |
| `tools/learning_data/tutorial_overlays/imr.json` | 新建：0–6 + 7+ |
| `tools/learning_data/csv/` | **删除**旧 10 csv；生成新金标 csv |
| `src/application/learning/learning_types.h` | 扩字段 |
| `src/application/learning/learning_tutorial_catalog.cpp` | 解析新列；PRAGMA 兼容 |
| `src/application/learning/learning_dataset_store.h` | `kExpectedCatalogVersion = "learning-center-v2"` |
| `src/application/learning/learning_dataset_store.cpp` | 仅版本/可选 `notes`；**不**改连接清理 |
| `src/ui/learning_center_page.h` | 分节控件；保留 `import_demo_requested` / `open_formula_help_requested` |
| `src/ui/learning_center_page.cpp` | **完整** 0–6/7+ UI（§7）；保留左树/搜索/导入/公式/导出 |
| `tests/learning_center_store_test.cpp` | 去掉 `listsTenDatasets==10`；金标 `imr_spi_shift`；version v2 |
| `tests/learning_center_analysis_sample_test.cpp` | I-MR/直方图改新 dataset_id 与列名 |
| `tools/verify_learning_center_db.py` | 见 §6 |
| `tools/verify_learning_center_gate.py` | 见 §6 |
| `tools/verify_learning_dataset_mapping.py` | 见 §6 |
| `resources/help/learning_center.sqlite` | **只**由 builder 重建 |

**Wave-1…4**

| 路径 | 改什么 |
|------|--------|
| `tools/build_learning_center_db.py` | 该波 GENERATORS |
| `tools/learning_data/dataset_mapping.json` | 填该波 dataset_id / role_map |
| `tools/learning_data/tutorial_overlays/*.json` | 该波每课 overlay |
| `tools/learning_data/csv/*.csv` | 生成物 |
| `tools/learning_data/research_by_id.json` | `dataset_hint` 改新 id（可与 overlay 同波） |
| `tools/build_research_by_id.py` | 去掉默认挂 `smt_paste_height` |
| `tools/build_learning_research_notes.py` | 同上 |
| `tests/learning_center_analysis_sample_test.cpp` | 样本分析改新表（Pareto/卡方等） |

**Wave-5**

| 路径 | 改什么 |
|------|--------|
| `docs/research/learning-center-dataset-mapping.md` | 按本锁表重写 |
| `docs/research/learning-center-research-notes.md` | 建议 dataset_id 不再指向旧 10 表 |
| `tools/dist_readme.txt` | v2 / 专用集 / `demo_{id}` |
| `tools/verify_*.py` | banned 旧 id 全库扫描（测试与 verify 自身白名单除外） |

**C++ 测试（用户本机编；Agent 不强跑 cmake）**

- `learning_center_store_test`
- `learning_center_worksheet_registry_test`
- `learning_center_analysis_sample_test`

### 5.2 明确不改哪些文件

| 路径 | 为何不动 |
|------|----------|
| `src/application/analysis_service.cpp` / `.h` | 禁止顺手改分析内核 |
| `resources/help/algorithm_help.json` | **禁止改公式语义** / 删 interpretation 边界 |
| `src/ui/algorithm_help_dialog*`、`algorithm_help_catalog*` | 不建第二套帮助 |
| `src/ui/analysis_commands.cpp` / `.h` | 只读对齐 roles/inputs；不改菜单语义 |
| `src/application/learning/worksheet_registry.*` | 禁止回退单表覆盖；行为保持 |
| `src/ui/mainwindow.cpp` 中 `import_learning_dataset` **签名与「不清输出页」** | 只允许修编译破口；禁止改成覆盖当前表 |
| `CMakeLists.txt` | 不新增 test target / 不改 qrc 路径（仍 `resources/help/learning_center.sqlite`） |
| `tools/package_dist.ps1` | 已拷 sqlite；不必为 v2 改路径 |
| `tools/build_*.py` 中与学习中心无关的正式脚本 | 禁止顺手重构 |
| 任何 sqlite 手工 hex 编辑 | H3：下次 build 冲掉 |
| 新增第 185 个假 `command_id` | H6 |
| `third_party/**`、许可证、无关 UI | 超范围 |

`tools/_tmp_list_commands_for_wave_plan.py` 与 `_tmp_command_inventory.json` 是 Plan 产物，执行期可继续当锁表源；**不要**当成产品运行时依赖。

---

## 6. verify 脚本新增 / 修改断言清单

### 6.1 必须改掉的旧断言

| 位置 | 旧 | 新 |
|------|----|----|
| `tools/verify_learning_center_db.py` 打印「10 datasets」 | 恰好 10 | `len(datasets) == mapping.datasets`；Goal 完成后 = **93**（含练习表） |
| `tests/learning_center_store_test.cpp` `listsTenDatasets` | `QCOMPARE(..., 10)` | 改名为 `listsPlannedDatasets`；与 mapping 数量一致；**不得**写死 10 |
| 同上 `loadsSmtDatasetRowsAndColumns` | `smt_paste_height` 5 列 80 行 | 改为 `imr_spi_shift`：3 列、行数=生成器、首列或测量列为 `锡膏高度_um` |
| `tests/learning_center_analysis_sample_test.cpp` | IMR/直方图用 `smt_paste_height`；卡方/Pareto 用 `attribute_defect` | 改为 `imr_spi_shift` / `graph_hist_prob` / `cat_shift_line` / `pareto_defect_tail` |
| `tests/learning_center_store_test.cpp` `importPreservesExistingWorksheet` | `paired_rework` | Wave-0 用 `imr_spi_shift`；全量后可用 `infer_paired_shift` |

教程数 **184** 断言**保留**。

### 6.2 `verify_learning_center_db.py` 新增

- [ ] `meta.catalog_version == "learning-center-v2"`
- [ ] `tutorial.command_id` == `id_metadata.entries.id` == 184 并集
- [ ] 无悬空 `dataset_id` FK
- [ ] **banned**：旧 10 id 不得出现在 `datasets` 或 `tutorials.dataset_id`
- [ ] **无** `dataset_id LIKE 'demo_%'`
- [ ] 被 ≥2 条 tutorial 引用的 dataset ⊆ 白名单 8 族；每族 command 集合相等
- [ ] 白名单外 implemented 有数据 → 1 命令 1 表
- [ ] 金标强制：`imr.dataset_id==imr_spi_shift`；`glossary` ≥3 含 UCL 与 USL；`buried_signals` 含 row 41 与 55；`dialog_fill` 为 object 且仅 `variables`；`dialog_fill_detail` length ≥ 9；7+ 五字段非空；`skill_mission` 非空
- [ ] `imr_spi_spike_b` 存在于 datasets，且**不是**任何 tutorial 的主 `dataset_id`

### 6.3 `verify_learning_dataset_mapping.py` 新增

- [ ] mappings 覆盖 184；extra/missing FAIL
- [ ] `shared_families` 与本 plan §3 一致
- [ ] `import_worksheet_name == "demo_" + dataset_id`（有表时）
- [ ] `role_map` 键 ⊆ 该命令 role id
- [ ] 旧 mapping 主链（默认 smt）不存在

### 6.4 `verify_learning_center_gate.py` 新增 / 加严

保留：`used_for`/`not_for`/`scenario` 非空；`click_steps≥2`；formula 步骤含「公式」或「菜单」；`output_guide` 禁止句（过程合格 / 必须停线 / 已证明正态 / 量具通过）除非否定上下文。

新增：

- [ ] **Wave 感知阈值**（可用 env `LEARNING_CENTER_WAVE=0..5` 或读 sqlite 已填 dataset 的波次）：  
  - Wave-0：仅 `imr` 强制 glossary/buried/detail/7+  
  - Wave-k 完成后：该波 **全部** id glossary 阈值；该波锁表非空 dataset 必须有 `buried_signals` 且 `datasets.notes` 能搜到「行」或数字行号  
- [ ] Goal 完成（Wave-5）：**全部 184** glossary 阈值；全部非空 dataset 有埋点；banned 旧 10 字符串不在 `tools/learning_data` 与 `tools/build_learning_*.py`
- [ ] `dialog_fill` JSON 必须是 object（不是 array）；与 `dialog_fill_detail` 并存
- [ ] 控制图 implemented：glossary 或 misconceptions 出现 UCL/USL 区分

### 6.5 `verify_learning_research_notes.py`

保留 id 覆盖。Wave-5：notes 中「建议 dataset_id」不得再推荐旧 10 表（可用 WARN→Wave-5 FAIL）。

### 6.6 金标「导入后预期」检查单（给 Agent4 / 用户本机）

1. 学习中心选 I-MR → 导入 → 工作表名 **`demo_imr_spi_shift`**（不是 `demo_demo_…`）。  
2. 对话框：变量=`锡膏高度_um`；阶段列空；MR 长度=2；σ=平均移动极差；Nelson estimate=否；规则默认；tests 空；历史限空。  
3. I 图：后段上移（片 41）；片 55 相对近期波动不寻常 / 越 UCL 线索。  
4. 话术：**UCL≠USL**；禁止「过程合格 / 必须停线 / 点出 UCL=废品」。  
5. UI：0–6 默认展开；7+ 默认折叠；先修/检索可点。

---

## 7. UI 折叠默认策略（0–6 vs 7+）

**现网缺口**：右侧单块 `QTextBrowser` HTML，无折叠、无 glossary/7+。Qt Rich Text **不支持** `<details>`。禁止用「一大段 HTML」假装 Q3 已完成。

**布局（一次做完，禁止推倒左树/底栏）**

```
LearningCenterPage (QDialog)
 ├ 说明 Label（保留）
 ├ 搜索 QLineEdit（保留）
 ├ QSplitter
 │   ├ QTreeWidget（保留 entries_ 元数据缓存；H9）
 │   └ QScrollArea
 │       └ 分节控件（不是第二套导航）
 └ 底栏：导入 / 打开公式 / 导出 sqlite（保留信号）
```

**分节与默认展开**

| 节 | 标题 | 默认 |
|----|------|------|
| 头 | 标题 + 实现状态 | 展开（无折叠） |
| 0 | 关键词 glossary | **展开** |
| 1 | 背景（问题 / Y / DMAIC / 本课只回答什么） | **展开** |
| 2 | 专用数据（dataset 显示名 `demo_{id}`、列角色、埋点行号） | **展开** |
| 3 | 为何此工具 | **展开** |
| 4 | 逐步 + 参数表（`dialog_fill_detail`） | **展开** |
| 5 | 读输出（对照埋点） | **展开** |
| 6 | 误用 | **展开** |
| 7+ | 练习闭环（容器） | **折叠** |
| 7A–7E | 先修 / 自解释 / 褪脚手架 / 检索 / 错念 | 父级打开后仍 **各自折叠**（一次只摊一块，降 CLT） |

- 0–6：**默认全部展开**（完整 worked example 必须不用二次点击）。允许用户手动折上某一节。  
- 7+：**默认整块折叠**；禁止与 0–6 糊成一堵墙。  
- 不要用 `QToolBox` 做成「同时只显示一节」（会藏住 0–6 例题）。  
- 实现：可勾选 `QToolButton` + `QFrame`，或等价 collapsible section；节内可用小型 `QTextBrowser` 渲染表。  
- `entries_` 只缓存教程元数据；cells 仍按次 `load_dataset`。

**7+ 可交互（非仅 JSON 入库）**

| 块 | 控件 |
|----|------|
| 先修 | 题干 + good/bad 选项；选后对照反馈；错则提示回 §0 |
| 自解释 | 提示 `after` + `QPlainTextEdit`；按钮揭晓参考（含「为何关闭 Nelson estimate」） |
| 褪脚手架 | level 0/1/2；显示 student vs scaffold；level 2 写明导入 `demo_imr_spi_spike_b` |
| 检索 | 题目列表；可选本地作答 + 揭晓（不写回 sqlite） |
| 错念 | 先显示 `wrong`，点击展开 `right`；与 §6 禁止句互补 |

空字段的节：显示「本课无此块」但仍保留折叠标题（formula_reference 也要有 0–6 骨架）。

---

## 8. 禁止偷懒（手册 §8 全文粘贴）

1. **禁止**推倒重做学习中心窗口/导入链路「为了干净」。  
2. **禁止**恢复「10 张共享表挂几十命令」；同构共享必须有白名单且族要小。  
3. **禁止**金标未过就批量灌水文案；也**禁止**金标过后借口「下轮再铺」截断全集。  
4. **禁止**教程步骤不写真实 `menu_path` / 角色名。  
5. **禁止** glossary 为空却满篇 UCL/Cpk/%GR&R。  
6. **禁止**专用集无埋点注释（「随机正态」充数）。  
7. **禁止** dialog 只写列名不写「字段含义/为何留空」。  
8. **禁止**输出解读不对照埋点（通用套话）。  
9. **禁止**写成「过程合格 / 必须停线 / 已证明正态 / 点出 UCL=废品」。  
10. **禁止**改 `algorithm_help.json` 公式语义或删 interpretation 边界。  
11. **禁止**导入覆盖当前表、跳过 `WorksheetRegistry`。  
12. **禁止** QSql 默认连接残留。  
13. **禁止**整库载入常驻 `QVector`。  
14. **禁止**学习中心再做分析向导。  
15. **禁止**跳过 id 对齐 / mapping verify。  
16. **禁止**中途换模型或让子 Agent 用另一模型「省钱」。  
17. **禁止** Agent6 没出 QA 报告就 UpdateGoal complete。  
18. **禁止**用浏览器代替用户本机 `package_dist`。  
19. **禁止**把 Canvas 里的 skill 叠加块删掉只留摘要。  
20. **禁止** UI 只入库不展示，或把 0–6 与 7+ 糊成一堵墙且无折叠分节（Q3=一次做完）。  
21. **禁止**只改 sqlite / 不改 `build_learning_center_db.py` 生成器（下次 build 会冲掉）。  
22. **禁止** `dataset_id` 以 `demo_` 开头导致工作表变成 `demo_demo_…`。  
23. **禁止**只改 `META_VERSION` 或只改 `kExpectedCatalogVersion`（必须双写）。  
24. **禁止**用详解 JSON 覆盖原 `dialog_fill` 对象格式。  
25. **禁止**为术语课新增虚假 command_id 撑到 185+。  
26. **禁止**保留 `smt_paste_height` 等旧共享表「先留着」——Q5=删除重建。  
27. **禁止**把同构共享扩成变相大宽表（白名单必须写清服务哪些 command_id、埋点是否仍成立）。

### 本 plan 额外禁止（防执行偷懒）

- **禁止** Wave-0 只改 HTML 字符串、不做成可折叠 QWidget 分节。  
- **禁止** 用 `<details>` 冒充折叠（`QTextBrowser` 不支持）。  
- **禁止** 金标 `dialog_fill` 漏写 Nelson / `rule_policy` / `tests` / 双历史限的 **detail**。  
- **禁止** 把 `xbar_r`/`xbar_s` 或 `p`/`np` 合并成一张表。  
- **禁止** 184 课 overlay 写成同一段套话。  
- **禁止** 新增 CMake target / 第二窗口 / 内嵌分析对话框。  
- **禁止** Agent 因中文路径强跑 `cmake` / `package_dist` 导致损坏；verify 用 Python。

---

## 9. 风险与 Agent3 开工顺序

**风险（一行）**：184 课的 glossary/`dialog_fill_detail`/专用集是主要工期；Wave-0 若 UI 或金标未闸门就灌水，或金标过后停更，都会直接违反 Q2/Q3——必须按锁表连续清零，不得缩成「只做 imr」。

**库体积**：约 93 张少列表（2–6 列、30–200 行）可接受；禁止为「省体积」回到 10 宽表。

### Agent3 强制顺序（同一 Goal 内连续）

1. 双写 `learning-center-v2`（py + h）。  
2. `learning_types` + catalog PRAGMA 解析新列；`dialog_fill` 行为不变。  
3. 删除旧 10 `GENERATORS`/CSV；写入 `imr_spi_shift` + `imr_spi_spike_b` + `imr` overlay；builder 重建 sqlite。  
4. **完整** `LearningCenterPage` 分节 UI + 7+ 交互（Q3，不得留到 Wave-4）。  
5. 改 C++/Python「恰好 10 表」；落地 §6 Wave-0 金标断言。  
6. 跑 `python tools/verify_learning_center_gate.py`；金标未 PASS **停止灌水**。  
7. Wave-1 锁表 20 id → gate → Wave-2 24 → gate → Wave-3 56 → gate → Wave-4 83 → gate。  
8. Wave-5 残留清理 + mapping md + dist_readme。  
9. 交给 Agent4（Python gate + 用户编译三个 `learning_center_*` target）→ Agent6 QA → Agent5 commit/push（用户规则）；**提示用户本机** `package_dist`。

并行纪律：禁止两人同时改 `learning_center_page.cpp` 或 `build_learning_center_db.py`。overlay JSON 可按菜单包并行，但 merge 前跑 verify。

### 金标 overlay 要点（执行不得删字段）

- 菜单：`控制图` → `I-MR 控制图`；`click_steps` 写真实路径。  
- `related_ids` 可指 `xbar_r`、`capability`（对比课，**不是**共享表）。  
- 7+ 采用 Canvas `PREREQ_QUIZ` / `SELF_EXPLAIN` / `FADING` / `RETRIEVAL` / `MISCONCEPTIONS`，并补一条 self_explain：「为什么本课关闭 Nelson estimate？」

---

## 10. Agent2 DoD / go-no-go

- [x] Wave-0 = schema + **完整教学 UI** + 仅 `imr`/`imr_spi_shift` 金标  
- [x] Wave-1…4 **完整 command_id 锁表**（无「等等」）；并集 184  
- [x] 同构白名单 8 族，写清 command_id / 为何同构 / 埋点  
- [x] Schema DDL/JSON 与兼容策略  
- [x] 精确路径文件清单 + 明确不改  
- [x] verify 断言清单（改掉恰好 10）  
- [x] UI 0–6 默认展开 / 7+ 默认折叠  
- [x] 手册 §8 全文已粘贴  
- [x] **未改** `src/`、正式 `tools/build_*.py` 产品逻辑、sqlite、CMake、resources 产品资源（仅写 docs + 临时 inventory）

**go/no-go**：**GO** — 可进 Agent3。§0 已锁；金标字段与 Canvas 缺口已在调研闭合；184 id 已锁进本文件与 `_tmp_command_inventory.json`。

**用户本机（Q6）**：Agent 不跑 cmake/package。完成后请用 Qt Creator 编上述三个 test，再 `tools/package_dist.ps1`，抽查学习中心导入与分节 UI。
