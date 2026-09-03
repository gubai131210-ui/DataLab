# 学习中心文案口吻升温（并肩版）— Agent2 Wave 计划（锁表）

> **岗**：Agent2 Plan（**禁止改产品代码**；本文件除外）  
> **日期**：2026-09-03  
> **权威手册**：[`goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md`](goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md)  
> **调研基线**：[`learning-center-copy-voice-warmth-research.md`](learning-center-copy-voice-warmth-research.md)  
> **口吻标尺 Canvas**：`learning-center-copy-voice-target.canvas.tsx`（并肩版）  
> **金标深度 Canvas**：`learning-center-tutorial-example.canvas.tsx`  
> **id 锁表复用源**：[`goal-learning-center-pedagogy-upgrade-wave-plan.md`](goal-learning-center-pedagogy-upgrade-wave-plan.md)、[`goal-learning-center-copy-depth-wave-plan.md`](goal-learning-center-copy-depth-wave-plan.md)  
> **模型**：inherit（禁止建议换模型）  
> **本轮态度**：只换语气 / 加温度 / 加深可读；**禁止推倒**专用表、v2、0–6、导入、hint/why、图形名实、7B 隐藏

---

## 文首：现网硬约束 H1–H13（手册 §0.1 原文抄入）

| ID | 约束 |
|----|------|
| H1 | `catalog_version` **保持** `learning-center-v2`。禁止只改 sqlite、禁止偷偷升 v3 |
| H2 | `dataset_id` **不以 `demo_` 开头**。工作表仍是 `demo_{dataset_id}` |
| H3 | 文案必须进 `build_learning_center_db.py` / `tutorial_overlays` / `wave*_content.py` / `copy_depth.py` / `glossary_bank.py`；**禁止**只手改 sqlite |
| H4 | 同构共享仍以前一轮白名单为准；禁止回归旧 10 表 |
| H5 | `dialog_fill` 保持角色→列名 object；详解走 `dialog_fill_detail` |
| H6 | 禁止为术语课新增虚假 command_id |
| H7 | 禁止推倒 `LearningCenterPage` 导航树 / 导入 / `WorksheetRegistry` |
| H8 | 禁止改 `algorithm_help.json` 公式语义 |
| H9 | 金标 `imr`：**允许 0–6 加长并肩**，但片号、UCL≠USL、9 字段参数表等硬点**不得删成摘要** |
| H10 | 练习 JSON 形状以前一轮为准（`why`/`hint`/`model_answer`）；解析器缺字段降级为空，不得破坏旧库可读 |
| H11 | **7B「步间自解释」UI 保持隐藏**； scrub 成果保留；禁止把「自解释」字样再写回学员可见导语 |
| H12 | 学员可见区禁止开发黑话（`command_id` / `同构` / `WAVE` / `overlay` / 内部 id 当正确答案） |
| H13 | 图形名实对齐成果保留：`title`/`menu_path` = 菜单中文；禁止为润色把条形图改名成 Pareto |

### §0 锁定摘要（禁止重问 / 改口）

Q1 全集 184 · Q2 全字段（7B UI 继续隐藏）· Q3 并肩版口吻 · Q4 保 v2 / 不推倒导航导入 · Q5 金标可加长不砍硬点 · Q6 六 Agent inherit · Q7 Python verify + 用户本机自测 · Q9 只换语气不推倒骨架。

**计数不得截断**：Wave-0=1，Wave-1=20，Wave-2=24，Wave-3=56，Wave-4=83，Wave-5=0 新 id。**1+20+24+56+83=184**（与 pedagogy / copy-depth 锁表逐 id 相同；本文件已全表列出，不得漏）。

---

## 1. Wave 总表（本 Goal 施工语义）

| Wave | 施工内容 | n | 出口（对照手册 §4.2） |
|------|----------|---|----------------------|
| **Wave-0** | **`imr` 金标**：0–6 明显加长并肩 + 7A/7C/7D/7E 并肩；硬点不删；**先拆** `copy_depth.py` / `wave*_content.py` / `glossary_bank.py` 电报·说教·套壳源头 | 1 | Agent4+6：口吻通过 + 片 41/55、UCL≠USL、参数表 9 字段仍可指认 |
| **Wave-1** | 控制图包（除已在 Wave-0 的 `imr`）0–6 + 7A/7C/7D/7E 全文并肩 | 20 | gate：该包每课 §4.2 全字段 |
| **Wave-2** | 质量 / MSA / 能力：同上全字段 | 24 | gate：同上 |
| **Wave-3** | 推断 / ANOVA / 回归：同上全字段 | 56 | gate：同上 |
| **Wave-4** | 图形 / DOE / 可靠性 / 其余：同上全字段 | 83 | gate：同上；图形名实不回归 |
| **Wave-5** | 黑名单 verify、scrub「自解释」残留、文档、builder 重建 sqlite | 0 | Agent5+6；Python 口吻断言全集 PASS |

**禁止**：金标口吻未达标就批量灌水；也禁止金标过后借口「下一 Goal 再铺 184」。内部 Wave 只是施工队列。

---

## 2. command_id 锁表（复用前一轮，184 不漏）

dataset 列**不要改**（本 Goal 不是换表）。附加非 command：练习集 `imr_spi_spike_b`（只进 datasets；fade 引用）。

### Wave-0 command_id 锁表（n=1）— **imr 金标**

| command_id | dataset_id | 备注 |
|---|---|---|
| `imr` | `imr_spi_shift` | **金标闸门**：0–6 加长并肩；片 41/55、UCL≠USL、`dialog_fill_detail` ≥9；7+ 同步并肩；去掉「本课只练」说教收束 |

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

无新 `command_id`。只跑口吻 verify、scrub 残留、文档与重建库。

**锁表计数核对（Agent2 已跑）**：Wave-0…4 = 1+20+24+56+83 = **184**，且与 pedagogy plan 唯一并集一致。

---

## 3. Wave 出口标准（每 Wave 对照手册 §4.2）

每课学员可见字段须同时满足下表；Wave-0 仅强制 `imr`，Wave-k 完成后对该波**全部** id 强制，Wave-5 强制全集 184。

| 块 | 最低要求（手册 §4.2） |
|----|----------------------|
| `used_for` / `not_for` | 各至少 **2 句完整中文**；说清「适合什么现场问题 / 什么情况先别硬套」；红线带原因（「不等于放行样板 / 通常还要对照规程」），禁止纯「禁止过程合格。」 |
| `scenario` | 有产线/测量情境；有可指着的埋点位置；用「不妨…」邀请，不用「你的任务是」 |
| glossary | 术语 + 白话 + 怎么记；控制限课保留 UCL≠USL；非控制限课不为凑数考 UCL |
| `dialog_fill_detail` | 每字段 `meaning` 结合本课列名；**禁止**全课同一壳 |
| 埋点 / 读输出 | 指到行号或子组/图区；读出「看见什么」；放行/停线用「通常还要…」留白 |
| 误用 | 完整句子讲「容易漏什么、对调会怎样」 |
| 7A 先修 | 完整问句 + 完整选项；`why` 并肩解释，不训斥 |
| 7C 褪脚手架 | `student` 句可独立执行；语气邀请而非命令堆砌 |
| 7D 检索 | 完整问句 + `hint` 参考答；像助教笔记不是禁令牌 |
| 7E 错念 | `wrong`/`right` 完整；`right` 讲对在哪，不写「禁止已证明正态。」 |

**金标 `imr` 额外出口**：0–6 **允许明显加长并肩**；片 41/55、UCL≠USL、参数表硬点必须仍可被 Agent6 指认。7+ 同步并肩化。

### 各 Wave 可勾选出口 DoD

**Wave-0**

- [ ] `imr` 0–6 已并肩加长；无「本课只练 / 这一课只练」
- [ ] 硬点仍在（见 §9）
- [ ] 7A/7C/7D/7E 并肩；7B UI **未**恢复
- [ ] `copy_depth.py` / `wave1_content.py` / `wave2_content.py` / `wave4_content.py` / `glossary_bank.py` 电报·说教·套壳源头已先拆（见 §5）
- [ ] 禁止只改 sqlite

**Wave-1…4（每波）**

- [ ] 锁表每一个 id 的 overlays（或同源生成）已并肩化，**不得跳 id**
- [ ] 该波每课 `used_for`/`scenario`/`not_for` 达 §4.2 长度与语气
- [ ] 抽检无黑名单说教/电报壳（误用里讲清原因的完整句除外）
- [ ] `dialog_fill_detail` 无全课同一壳
- [ ] 前两轮基线保留：hint/why、图形名实、专用表、v2
- [ ] 该波 Python gate 口吻阈值 PASS

**Wave-5**

- [ ] verify 口吻断言已落地且 **184 全集** PASS
- [ ] scrub：学员可见无「自解释」导语回归；7B 仍隐藏
- [ ] builder 重建 `resources/help/learning-center.sqlite`；catalog 仍 v2
- [ ] 文档指向本 Goal research + 本 plan + 手册

---

## 4. 字段检查单（Agent3 每课对照勾选）

```text
[ ] used_for：≥2 句完整中文；并肩邀请；能指现场问题；非电报单行
[ ] not_for：≥2 句；红线带原因（通常还要… / 不等于放行样板）；无纯「禁止过程合格。」
[ ] scenario：有产线情境 + 可指埋点；含「不妨…」类邀请；无「你的任务是」
[ ] glossary：术语+白话+怎么记；SPC 课 UCL≠USL；非 SPC 不为凑数考 UCL
[ ] dialog_fill_detail：每字段 meaning 写本课列名/现场含义；禁止「这一项决定图上或表上对应哪一列」同壳
[ ] 埋点（buried_signals）：指到行号/子组/片号；expect 读「看见什么」
[ ] 读输出（output_guide）：对照埋点；放行/停线留白「通常还要对照规程」
[ ] 误用（common_mistakes）：完整句讲漏什么/对调会怎样
[ ] 7A 先修：完整问句+成句选项；why 并肩不训斥
[ ] 7C 褪脚手架：student 可独立执行；邀请语气
[ ] 7D 检索：完整问句 + hint 助教笔记风
[ ] 7E 错念：wrong/right 完整；right 讲对在哪
[ ] 稳词：用「波动主要落在…」；禁止「抖主要」
[ ] 学员区无：command_id / 同构 / WAVE / overlay / 内部 id 当答案 / 「自解释」导语
[ ] 7B：JSON 可留，UI 继续隐藏（不写回学员可见区）
```

---

## 5. 生成器改法（强制顺序）

### 5.1 顺序（不得颠倒）

```text
1) 先改模板源头（否则重建库回灌）
   - tools/learning_data/copy_depth.py
   - tools/learning_data/wave1_content.py
   - tools/learning_data/wave2_content.py
   - tools/learning_data/wave4_content.py
   - tools/learning_data/glossary_bank.py（白话并肩化；非电报主凶但须逐条过）
2) 再改 / 重生 overlays
   - tools/learning_data/tutorial_overlays/*.json（Wave-0 先手写打磨 imr.json）
3) 再 builder 重建 sqlite
   - tools/build_learning_center_db.py → resources/help/learning-center.sqlite
4) 若学员可见又出现「自解释」→ 跑 tools/scrub_self_explain_student_copy.py
   （禁止恢复 7B UI）
```

### 5.2 research 已证实的灌水点（Agent3 不许装没看见）

| 文件 | 仍在灌什么 | 证据（research §4） | 优先级 |
|------|------------|---------------------|--------|
| `copy_depth.py` | **模板壳** | `expand_meaning()` ≈L228：`这一项决定图上或表上对应哪一列。` → 114+ 课 | **必改源头** |
| `wave2_content.py` | **电报 + 说教禁令** | scenario「子组 n=5。子组12起批均值上移。」；大量「禁止过程合格」 | **必改** |
| `wave4_content.py` | **说教禁令灌图形/DOE/时序** | `not_for`/`expect`/`output_guide`/`misc` 复用「禁止过程合格 / 已证明正态 / 必须停线」；空课 `pad` 同文 | **必改** |
| `wave1_content.py` | **电报标题/故事 +「本课只练」** | used_for「本课只练「固定单位 + 缺陷台阶」…」；story「每子组 n=5。…」 | **必改** |
| `glossary_bank.py` | 黑名单字符串未命中 | 仍须检查白话是否并肩 | 升温时过一遍 |
| overlays 历史产物 | 167 电报字段 + 114 壳 + 38 禁令 not_for | 改源头后按 Wave 重写/重生 | 禁止只手改 sqlite |

### 5.3 改稿铁律（对齐 Canvas 并肩版）

| 要 | 不要 |
|----|------|
| 2～4 句说完；「不妨 / 常见读法 / 多半 / 通常还要…」 | 电报：`子组 n=5。子组12起批均值上移。` |
| 能指列名/子组/行号/菜单 | 「你的任务是 / 你应该能 / 这一课只练 / 本课只练」 |
| 红线带原因 | 纯「禁止过程合格。」 |
| 「波动主要落在批内还是批间」 | 「抖主要…」 |
| meaning 写本课现场含义 | 全课复制「这一项决定图上或表上对应哪一列」 |

---

## 6. verify 新断言规格（Agent4 落地；口吻为硬门）

扩展：`tools/verify_learning_center_copy_depth.py`、`tools/verify_learning_center_gate.py`、`tools/verify_learning_center_db.py`（按现网分工；禁止另起无关第三套 gate 名除非已有）。

| ID | 断言 | 说明 |
|----|------|------|
| V1 | `used_for` / `scenario` / `not_for` **最小汉字长度** | 挡电报。建议：`used_for`≥40、`scenario`≥40、`not_for`≥30（汉字计数；Wave-0 可先金标，Wave-5 全集）。禁止再出现 ≤28 汉字的「尖事实堆」作为终态 |
| V2 | 学员可见区 **黑名单** | 禁止匹配：`你的任务是`、`这一课只练`、`本课只练`、`禁止过程合格`、`抖主要`（扫描 used_for/not_for/scenario/output_guide/common_mistakes/glossary/dialog_fill_detail/prereq/fade/retrieval/misconceptions 等学员可见 JSON 文本；误用里若必须否定，须改写为「不等于… / 通常还要…」完整句，不得保留黑名单原串） |
| V3 | `dialog_fill_detail.meaning` **同壳检测** | 同一课内 ≥2 条 meaning **完全相同** → FAIL；或 meaning 含固定壳句「这一项决定图上或表上对应哪一列」→ FAIL |
| V4 | 金标 `imr` **硬点 + 长度提高** | glossary 仍含 UCL **与** USL；`buried_signals` 含 row/片 **41** 与 **55**；`dialog_fill_detail` length ≥ **9**；`used_for`/`scenario` 长度下限 **高于** 普通课（建议 ≥80 汉字，允许明显加长并肩）；禁止删硬点成摘要 |
| V5 | 保留上一轮可读性 | `good` 不得像 `^[a-z][a-z0-9_]+$`；先修题干长度等 R1–R4；`catalog_version == learning-center-v2`；无 `dataset_id` 以 `demo_` 开头 |
| V6 | 7B / 自解释导语 | 学员可见导语区不得出现「自解释」作为教学块标题/导语（与 scrub 一致）；不要求删除 self_explain JSON 列 |

C++ target（用户本机编；Agent 不强跑 cmake）：若本轮未改 C++ → 写明「无强制」。若定点改硬编码说教导语 → `learning_center_store_test` 等按需。

---

## 7. 禁止偷懒（手册 §7 全文粘贴）

1. **禁止**推倒学习中心窗口/导入链路/专用表/v2/0–6 骨架。  
2. **禁止**只改金标或只改 `used_for` 就宣称 Goal 完成。  
3. **禁止**金标口吻未达标就批量灌水；也**禁止**金标过后借口下轮再铺 184。  
4. **禁止**电报体交付（单行断句堆事实）。  
5. **禁止**说教壳：`你的任务是` / `你应该能` / `这一课只练` / 纯 `禁止过程合格`。  
6. **禁止**歧义口语「抖主要」。  
7. **禁止** `dialog_fill_detail` 全课复制同一句壳。  
8. **禁止**为省事删掉金标片号 / UCL≠USL / 参数表硬点。  
9. **禁止**重新显示 7B 或把「自解释」写回学员导语。  
10. **禁止**学员可见区出现 `dataset_id` / `同构` / `WAVE` 当教学内容。  
11. **禁止**只手改 sqlite。  
12. **禁止**中途换模型；所有 Task **`model: "inherit"`**。  
13. **禁止** Agent6 没出 QA 报告就 UpdateGoal complete。  
14. **禁止**用浏览器代替用户本机 package（除非用户本轮再要求）。  
15. **禁止**升 catalog 到 v3。  
16. **禁止** `dataset_id` 以 `demo_` 开头。  
17. **禁止** `dialog_fill` 覆盖成数组。  
18. **禁止**新增虚假 command_id。  
19. **禁止**改 `algorithm_help.json` 公式语义。  
20. **禁止**并行两人改同一大文件；禁止收尾塞无关重构。  
21. **禁止**把「短句」理解成「省字到尖」——细节要够，态度要稳。  
22. **禁止**抒情散文或堆砌无关比喻；一句现场比喻足够。  

### 本 plan 额外禁止（防执行偷懒）

- **禁止**把范围缩成「只改 imr」或「只改 used_for」。  
- **禁止**先改 overlays、后改 `copy_depth`/`wave*_content`（顺序颠倒必回灌）。  
- **禁止** Plan / 本阶段改产品代码、overlays、生成器、sqlite、C++。  
- **禁止**建议换模型。  
- **禁止**漏课：184 必须全部进锁表且全部施工。  
- **禁止**为润色把条形图改名成 Pareto（H13）。  
- **禁止** Agent 因中文路径强跑易失败的 cmake/package（除非用户本轮明确要求）。

---

## 8. 文件改动清单 + 明确不改

### 8.1 Agent3 允许改（按强制顺序）

| 路径 | 改什么 |
|------|--------|
| `tools/learning_data/copy_depth.py` | 拆 `expand_meaning` 套壳；去电报/说教默认句 |
| `tools/learning_data/wave1_content.py` | 控制图包源头：去「本课只练」、电报 story、禁令壳 |
| `tools/learning_data/wave2_content.py` | 质量/MSA/能力源头：scenario 电报、禁止过程合格 |
| `tools/learning_data/wave4_content.py` | 图形/DOE/可靠性源头：禁令模板与 pad 同文 |
| `tools/learning_data/glossary_bank.py` | 白话并肩化（稳词、邀请语气） |
| `tools/learning_data/tutorial_overlays/*.json` | 184 课学员可见文案（Wave-0 先打磨 `imr.json`） |
| `tools/build_learning_center_db.py` | merge 后重建；**不**改 `META_VERSION`（保持 v2） |
| `tools/verify_learning_center_copy_depth.py` | V1–V6 口吻断言 |
| `tools/verify_learning_center_gate.py` / `verify_learning_center_db.py` | 按需对齐口吻硬门；保留前两轮断言 |
| `tools/scrub_self_explain_student_copy.py` | 仅防「自解释」导语回归 |
| `src/ui/learning_center_page.*` | **原则上不改**；仅当发现学员可见硬编码说教导语时定点改字。禁止重做左树/导入；禁止加回 7B |
| `src/application/learning/*` | **原则上不改**；除非 JSON 增量键兼容 |
| `tests/learning_center_*_test.cpp` | 仅文案相关断言对齐；金标硬点断言保留 |
| `resources/help/learning-center.sqlite` | **只**由 builder 重建 |
| `docs/research/learning-center-copy-voice-warmth-qa-report.md` | Agent6 产出 |
| 本 Goal 手册 / research / 本 plan | 文档收口 |

### 8.2 明确不改哪些文件

| 路径 | 为何不动 |
|------|----------|
| `src/application/analysis_service.*` | 禁止顺手改分析内核 |
| `resources/help/algorithm_help.json` | H8：公式语义 |
| `src/ui/analysis_commands.cpp` / `.h` | 只读对齐；不为润色改菜单 id |
| `worksheet_registry.*` / `MainWindow::import_learning_dataset` 语义 | H7：保持新建表、不清输出页 |
| `LearningCenterPage` 左树 / 搜索 / 导入 / 公式 / 0–6 折叠骨架 | 不推倒导航与分节 |
| 专用表生成器配方 / 同构白名单 8 族 | 前两轮基线；本 Goal 不换表 |
| `CMakeLists.txt` / `package_dist.ps1` | 除非测试破编译 |
| 手工 sqlite hex 编辑 | H3 |
| 第 185 个假 `command_id` | H6 |
| `third_party/**`、无关 UI、许可证 | 超范围 |
| 升 catalog 到 v3 | H1 |

---

## 9. 金标 `imr`：加长但硬点清单（允许 0–6 明显加长并肩）

Agent3 改 `imr.json` 时**允许**加温度、过渡、「不妨/常见读法/通常还要」；下列硬点**必须仍可被 Agent4/6 指认**（对照金标 Canvas + 前两轮 verify）：

| # | 硬点 | 验收 |
|---|------|------|
| G1 | 片号 **41**（均值阶跃开始） | `buried_signals` / 读输出 / 场景文案可指认；不得删成「后段上移」无片号 |
| G2 | 片号 **55**（尖峰 → I 图 UCL 线索） | 同上；不得删成摘要 |
| G3 | **UCL ≠ USL** | glossary 同时出现 UCL 与 USL（及白话区分）；读图/错念不得暗示「出 UCL=超规格废品」 |
| G4 | 参数表 **9 字段** | `dialog_fill_detail` length ≥ 9（变量、阶段列、MR 长度、σ 方法、Nelson estimate、规则默认策略、特殊原因测试、历史均值、历史 Sigma 等真实对话框字段） |
| G5 | `dialog_fill` | 仍为 object：`{"variables":"锡膏高度_um"}`（不改为数组） |
| G6 | 工作表名 | 导入后 `demo_imr_spi_shift`（dataset_id=`imr_spi_shift`，不以 `demo_` 开头） |
| G7 | 练习表 | `imr_spi_spike_b` 仍可被 fade 引用；不映射第二 command |
| G8 | 口吻 | 去掉「本课只练…」说教收束；0–6 可明显加长；7A/7C/7D/7E 并肩；**不**恢复 7B UI |

**长度**：金标 `used_for`/`scenario` 下限高于普通课（见 V4）；加长≠抒情散文（手册 §7.22）。

---

## 10. Agent3 强制开工顺序

1. Wave-0：打磨 `imr.json` 并肩金标（硬点清单可勾）→ 口吻闸门。  
2. **先**改 `copy_depth.py` + `wave1/2/4_content.py` + `glossary_bank.py` 拆电报/说教/套壳。  
3. Wave-1（20）→ gate → Wave-2（24）→ gate → Wave-3（56）→ gate → Wave-4（83）→ gate。  
4. Wave-5：verify 黑名单落地、scrub、builder 重建、文档。  
5. 交给 Agent4（Python 口吻断言）→ Agent6 QA → Agent5 commit/push；提示用户本机 `package_dist` 抽查口吻。

并行纪律：禁止两人同时改同一大 `wave*_content.py` 或 `build_learning_center_db.py`。overlay 可按菜单包并行，merge 前跑 verify。

---

## 11. Agent2 DoD + 风险 + go/no-go

### 本阶段交付文件列表

| 文件 | 动作 |
|------|------|
| `docs/research/goal-learning-center-copy-voice-warmth-wave-plan.md` | **新建**（本文件） |

**未改**任何产品代码 / overlays / py 生成器 / sqlite / C++。

### Agent2 DoD 勾选

- [x] 文首抄入现网硬约束 **H1–H13**  
- [x] Wave 锁表复用前一轮 **184 id**，Wave-0…5 全表列出，**不得漏 id**；Wave-0 单独标 **imr 金标**（已计数 1+20+24+56+83=184）  
- [x] Wave 出口标准对照手册 **§4.2**  
- [x] 字段检查单（used_for/not_for/scenario/glossary/dialog_fill_detail/埋点/读输出/误用/7A·7C·7D·7E）  
- [x] 生成器改法：明确 **先** `copy_depth` / `wave*_content` / `glossary_bank`，再 overlays，再 builder；指出 research 已证实灌水点  
- [x] verify 新断言规格（长度下限、黑名单、同壳检测、金标硬点+长度提高）  
- [x] **§7 禁止偷懒全文**（手册 22 条）已粘贴  
- [x] 文件改动清单 + 「明确不改哪些文件」  
- [x] 金标 imr 加长但硬点清单（片 41/55、UCL≠USL、参数表 9 字段等）  
- [x] **禁止** Plan 阶段改产品代码  
- [x] 未建议换模型；未重问 §0  

**风险（一行）**：若 Agent3 不先拆 `copy_depth.expand_meaning` 与 `wave1/2/4` 的电报/禁令/套壳模板，手改 overlays 后 builder 重建仍会大面积回灌，口吻 Goal 白做。

**go/no-go**：**GO** — 可进入 Agent3 执行（先 Wave-0 `imr` 并肩金标 + 拆模板源头，再按锁表铺完 184）。
