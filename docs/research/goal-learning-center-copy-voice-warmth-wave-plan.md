# 学习中心文案口吻升温（并肩版）— Agent2 Wave 计划（锁表 · 2026-09-05 重写）

> **岗**：Agent2 Plan（**禁止改产品代码**；本文件除外）  
> **日期**：2026-09-05（UTC+8）— **按今日调研重写**；旧 2026-09-03 稿以「电报/禁令」为主战场，**已过时**  
> **权威手册**：[`goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md`](goal-learning-center-copy-voice-warmth-plan-and-mega-prompt.md)  
> **调研基线（今日）**：[`learning-center-copy-voice-warmth-research.md`](learning-center-copy-voice-warmth-research.md)（2026-09-05）  
> **口吻标尺 Canvas**：`learning-center-copy-voice-target.canvas.tsx`（并肩版）  
> **金标深度 Canvas**：`learning-center-tutorial-example.canvas.tsx`  
> **id 锁表复用源**：[`goal-learning-center-pedagogy-upgrade-wave-plan.md`](goal-learning-center-pedagogy-upgrade-wave-plan.md)、[`goal-learning-center-copy-depth-wave-plan.md`](goal-learning-center-copy-depth-wave-plan.md)  
> **模型**：inherit（禁止建议换模型）  
> **本轮态度**：只换语气 / 加温度 / 加深可读；**禁止推倒**专用表、v2、0–6、导入、hint/why、图形名实、7B 隐藏  
> **今日主战场（不许装没看见）**：**soft warmth 软壳**（练习线是为 / 目标不是背菜单 / 更像一张对照纸条 / 不妨先当一次助手 / 还是那张本课练习表 / 导入本课练习表后打开），**不是**旧电报。黑名单 0 命中 ≠ Goal 完成。

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

### 今日现网事实（调研 2026-09-05，写入执行纪律）

| 信号 | 含义 → Agent3 动作 |
|------|-------------------|
| 旧黑名单（你的任务是 / 这一课只练 / 禁止过程合格 / 抖主要）overlays **0 命中** | 防回归仍进 verify；**禁止**据此宣称完成 |
| 软壳 A–F 共约 40–50 课/句式命中；粗分桶 ~5 真并肩 / ~133 软壳 / ~46 边界 | **主战场**：换成带本课现场细节的真正并肩文 |
| `warmth_wave234_overlays.py` 仍灌软壳 | **必改源头**，否则 builder 回灌 |
| `dialog_fill_detail` 旧壳句 0 命中，但 **跨课同文 meaning** ≥8 组 | verify 加跨课同文上限 |
| 真并肩样板课：`between_within_capability`、`imr`、`xbar_r`、`c_chart`、`ewma` | Wave-0/1 对照，不得推倒 |

---

## 1. Wave 总表 + 「金标闸门」与「包内仍含 imr」关系

| Wave | 施工内容 | 本波列出 id 数 | 出口 |
|------|----------|----------------|------|
| **Wave-0** | **`imr` 并肩金标闸门**：0–6+7A/7C/7D/7E（硬点不删）+ **拆模板源头**（`warmth_*` / `copy_depth` / `wave*_content`） | 1（闸门课） | Agent4+6：口吻通过 + 片 41/55、UCL≠USL、参数表 9 字段仍可指认；软壳短语不得作为终态 |
| **Wave-1** | **控制图包全文**（0–6 + 7A/7C/7D/7E）；**包内仍含 `imr`** | 21（含 imr） | gate：该包每课 §4.2 全字段；`imr` 在闸门后再做包内复核，不二次推倒硬点 |
| **Wave-2** | 质量 / MSA / 能力：同上全字段 | 24 | gate：同上；去软壳万能文 |
| **Wave-3** | 推断 / ANOVA / 回归：同上全字段 | 56 | gate：同上 |
| **Wave-4** | 图形 / DOE / 可靠性 / 其余：同上全字段 | 83 | gate：同上；图形名实不回归 |
| **Wave-5** | verify 黑名单+软壳门+scrub+文档+重建 sqlite | 0 新 id | Agent5+6；Python 口吻/软壳断言 **184 全集** PASS |

### 计数纪律（必须写清）

1. **金标闸门**：Wave-0 **只**把 `imr` 当作口吻+硬点闸门；未过闸门 **禁止**批量灌 Wave-2…4。  
2. **包内仍含 imr**：Wave-1「控制图包」锁表 **显式包含** `imr`（与 pedagogy「除金标」的旧写法不同——本 Goal 要在控制图包出口时 **复核** imr 仍达 §4.2，且硬点未丢）。  
3. **全集唯一计数 = 184**：`imr` 在 Wave-0 与 Wave-1 **列表可重复出现**，但并集计数时 **只算一次**。  
4. **公式**：唯一并集 = Wave-0∪Wave-1∪Wave-2∪Wave-3∪Wave-4  
   - Wave-1 独有（除 imr）= 20  
   - Wave-2 = 24，Wave-3 = 56，Wave-4 = 83  
   - **1 + 20 + 24 + 56 + 83 = 184**  
5. **禁止**「等等」、漏 id、把范围缩成只改 imr / 只清黑名单。

**禁止**：金标口吻未达标就批量灌水；也禁止金标过后借口「下一 Goal 再铺 184」。内部 Wave 只是施工队列。  
**禁止**：把「黑名单 0 命中」当成 Wave 出口或 Goal 完成。

---

## 2. command_id 锁表（复用前一轮，184 不漏）

dataset 列**不要改**（本 Goal 不是换表）。附加非 command：练习集 `imr_spi_spike_b`（只进 datasets；fade 引用）。

### Wave-0 command_id 锁表（n=1）— **imr 金标闸门**

| command_id | dataset_id | 备注 |
|---|---|---|
| `imr` | `imr_spi_shift` | **金标闸门**：0–6 加长并肩；片 41/55、UCL≠USL、`dialog_fill_detail` ≥9；7A/7C/7D/7E 同步并肩；**禁止**软壳万能文交差；硬点不删；7B UI 不恢复 |

### Wave-1 command_id 锁表（n=21，**包内含 imr**）— 控制图包

> 下列 **21** 课为本波施工/复核全集。其中 `imr` 已在 Wave-0 闸门打磨；本波仍列入，出口时做 §4.2 + 硬点复核（**不得**为「包内统一」而删硬点或换成软壳）。  
> 相对唯一并集：本波贡献 **20** 个新 id + 已计入的 `imr`。

| command_id | dataset_id | 备注 |
|---|---|---|
| `imr` | `imr_spi_shift` | **闸门课复核**（与 Wave-0 同一 id，唯一计数只算 1 次） |
| `c_chart` | `c_chart_defect_step` | |
| `cusum` | `spc_small_drift` | |
| `ewma` | `spc_small_drift` | 现网真并肩样板之一；复核勿回退软壳 |
| `g_chart` | `g_chart_gap_days` | |
| `generalized_variance` | `genvar_two_var` | |
| `hotelling_t2` | `t2_two_var_shift` | |
| `imr_rs` | `imr_rs_subgroup_shift` | |
| `laney_p_chart` | `laney_p_overdispersed` | |
| `laney_u_chart` | `laney_u_overdispersed` | |
| `mewma` | `mewma_two_var_drift` | |
| `moving_average` | `ma_small_drift` | |
| `np_chart` | `np_chart_const_n_step` | |
| `p_chart` | `p_chart_variable_n_step` | |
| `special_cause_rules` | （空） | |
| `t_chart` | `t_chart_time_interval` | |
| `u_chart` | `u_chart_variable_unit_step` | |
| `xbar_r` | `xbar_r_n5_range_spike` | 现网真并肩样板之一 |
| `xbar_s` | `xbar_s_n8_sd_shift` | |
| `z_mr` | `z_mr_short_run` | |
| `zone_chart` | `zone_chart_runs` | |

### Wave-2 command_id 锁表（n=24）— 质量 / MSA / 能力

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

> `between_within_capability` 为 Canvas 金样现网正面课：Wave-2 可对照其 `scenario`/`used_for`，**禁止**用软壳把它冲淡。

### Wave-3 command_id 锁表（n=56）— 推断 / ANOVA / 回归

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

### Wave-4 command_id 锁表（n=83）— 图形 / DOE / 可靠性 / 其余

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

无新 `command_id`。只跑口吻/软壳 verify、scrub 残留、文档与重建库。

**锁表计数核对（Agent2 已跑）**：唯一并集 = Wave-0∪…∪Wave-4 = **1 + 20 + 24 + 56 + 83 = 184**（`imr` 在 Wave-0 与 Wave-1 列表重复出现，唯一计数 1 次）。与 pedagogy / copy-depth 锁表逐 id 一致，**不得漏**。

---

## 3. Wave 出口标准（每 Wave 对照手册 §4.2）

每课学员可见字段须同时满足下表；Wave-0 仅强制 `imr` 闸门，Wave-1…4 对该波锁表**全部** id 强制（Wave-1 含 imr 复核），Wave-5 强制全集 184。

| 块 | 最低要求（手册 §4.2） |
|----|----------------------|
| `used_for` / `not_for` | 各至少 **2 句完整中文**；说清「适合什么现场问题 / 什么情况先别硬套」；红线带原因（「不等于放行样板 / 通常还要对照规程」）；**禁止**纯「禁止过程合格。」；**禁止**软壳万能文（见 §6 V7） |
| `scenario` | 有**本课**产线/测量情境（列名/子组/行号/片号可指）；用「不妨…」邀请；不用「你的任务是」；**禁止**只换菜单名的「练习线是为…准备的」 |
| glossary | 术语 + 白话 + 怎么记；控制限课保留 UCL≠USL；非控制限课不为凑数考 UCL |
| `dialog_fill_detail` | 每字段 `meaning` 结合本课列名；**禁止**全课同一壳；**禁止**跨课大面积完全相同 meaning（无表壳等） |
| 埋点 / 读输出 | 指到行号或子组/图区；读出「看见什么」；放行/停线用「通常还要…」留白 |
| 误用 | 完整句子讲「容易漏什么、对调会怎样」 |
| 7A 先修 | 完整问句 + 完整选项；`why` 并肩解释，不训斥 |
| 7C 褪脚手架 | `student` 句可独立执行；语气邀请而非命令堆砌 |
| 7D 检索 | 完整问句 + `hint` 参考答；像助教笔记不是禁令牌 |
| 7E 错念 | `wrong`/`right` 完整；`right` 讲对在哪，不写「禁止已证明正态。」 |

**金标 `imr` 额外出口**：0–6 **允许明显加长并肩**；片 41/55、UCL≠USL、参数表硬点必须仍可被 Agent6 指认。7+ 同步并肩化。现网已接近达标时：**定点加长短板，禁止推倒重写骨架**。

### 各 Wave 可勾选出口 DoD

**Wave-0（金标闸门 + 拆源头）**

- [ ] `imr` 0–6 已并肩加长；无「本课只练 / 这一课只练」；无软壳 A–F 作为终态主句
- [ ] 硬点仍在（见 §9：片 41/55、UCL≠USL、参数表 ≥9）
- [ ] 7A/7C/7D/7E 并肩；7B UI **未**恢复
- [ ] **模板源头已先拆**（见 §5）：`warmth_wave234_overlays.py` / `warmth_wave1_overlays.py` / `copy_depth.py` / `wave1_content.py` / `wave2_content.py` / `wave3_content.py` / `wave4_content.py` / `glossary_bank.py`
- [ ] 禁止只改 sqlite；禁止只改 imr 就宣称 Goal 完成

**Wave-1（控制图包，含 imr 复核）**

- [ ] 锁表 21 个 id（含 `imr`）全部达 §4.2；**不得跳 id**
- [ ] `imr` 硬点复核仍可指认（闸门成果未丢）
- [ ] 抽检无旧黑名单 + 无软壳万能文终态
- [ ] `dialog_fill_detail` 无全课同一壳
- [ ] 该波 Python gate 口吻/软壳阈值 PASS

**Wave-2…4（每波）**

- [ ] 锁表每一个 id 的 overlays（或同源生成）已**真并肩化**（本课现场细节），**不得跳 id**
- [ ] 该波每课 `used_for`/`scenario`/`not_for` 达 §4.2；**禁止**软壳 A–F 批量交差
- [ ] 抽检无旧黑名单说教/电报壳（误用里讲清原因的完整句除外）
- [ ] `dialog_fill_detail` 无全课同一壳；跨课同文 meaning 受控
- [ ] 前两轮基线保留：hint/why、图形名实、专用表、v2
- [ ] 该波 Python gate PASS

**Wave-5**

- [ ] verify：旧黑名单 + 长度下限 + **软壳短语门** + 跨课 meaning 上限 + 金标硬点 — **184 全集** PASS
- [ ] scrub：学员可见无「自解释」导语回归；7B 仍隐藏
- [ ] builder 重建 `resources/help/learning-center.sqlite`；catalog 仍 v2
- [ ] 文档指向本 Goal research + 本 plan + 手册
- [ ] **禁止**以「黑名单 0 命中」单独勾选完成

---

## 4. 字段检查单（对照手册 §4.2 全字段 · Agent3 每课勾选）

```text
[ ] used_for：≥2 句完整中文；并肩邀请；能指本课现场问题；非电报单行；非「更像一张对照纸条」万能壳
[ ] not_for：≥2 句；红线带原因（通常还要… / 不等于放行样板）；无纯「禁止过程合格。」
[ ] scenario：有本课产线/测量情境 + 可指埋点；含「不妨…」类邀请；无「你的任务是」
[ ] scenario 禁软壳：无「这条练习线是为…准备的」仅换菜单名；无「不妨先当一次助手」空现场；无「还是那张本课练习表」无埋点故事
[ ] glossary：术语+白话+怎么记；SPC 课 UCL≠USL；非 SPC 不为凑数考 UCL
[ ] dialog_fill_detail：每字段 meaning 写本课列名/现场含义；禁止「这一项决定图上或表上对应哪一列」同壳
[ ] dialog_fill_detail 跨课：本课 meaning 不得与 ≥7 课完全相同（无表壳等）——见 V8
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
[ ] 口吻验收：像同事带看，不是考试官；不是软壳填空题
```

---

## 5. 生成器改法（强制顺序 · 今日主战场 = 软壳）

### 5.1 顺序（不得颠倒）

```text
1) 先改模板源头（否则重建库回灌软壳）
   - tools/learning_data/warmth_wave234_overlays.py   ★ 今日主凶：软壳万能文
   - tools/learning_data/warmth_wave1_overlays.py     ★ 控制图包核对+补洞
   - tools/learning_data/copy_depth.py                ★ 半成品 studentize；加深本课专义
   - tools/learning_data/wave1_content.py
   - tools/learning_data/wave2_content.py             （import apply_warmth → 同源治理）
   - tools/learning_data/wave3_content.py             （import apply_warmth → 同源治理）
   - tools/learning_data/wave4_content.py             （import apply_warmth → 同源治理）
   - tools/learning_data/glossary_bank.py             （白话并肩化；非主凶但须过）
2) 再改 / 重生 overlays
   - tools/learning_data/tutorial_overlays/*.json
   - Wave-0 先核对/定点加长 imr.json（硬点不删）
3) 再 builder 重建 sqlite
   - tools/build_learning_center_db.py → resources/help/learning-center.sqlite
4) 若学员可见又出现「自解释」→ 跑 tools/scrub_self_explain_student_copy.py
   （禁止恢复 7B UI）
```

### 5.2 research 已证实的灌水点（2026-09-05 · Agent3 不许装没看见）

| 文件 | 仍在灌什么 | 证据（research §3–4） | 优先级 |
|------|------------|----------------------|--------|
| **`warmth_wave234_overlays.py`** | **软壳并肩**：练习线是为 / 目标不是背菜单 / 对照纸条 / 当一次助手 / 还是那张本课练习表 | overlays 40–50 课命中同构；wave2/3/4 `apply_warmth` | **必改源头 #1** |
| **`warmth_wave1_overlays.py`** | 控制图包有真并肩样板，但仍含替换式 scrub；需全包达 §4.2 | Wave-1 部分课已达标（xbar_r 等） | **核对 + 补洞** |
| **`copy_depth.py`** | 旧壳→空/`studentize` 替换；`_role_aware_meaning` 半通用 → **跨课同文 meaning** | 无表课 meaning ≥18 课同文 | **加深本课专义**，勿只靠全局 replace |
| `wave1_content.py` | 经 warmth/scrub 路径；防旧电报/「本课只练」回归 | research 防回归教材 | 与 warmth1 同源治理 |
| `wave2_content.py` / `wave3_content.py` / `wave4_content.py` | `apply_warmth` → 若 warmth 仍出软壳则连带污染 | import 证据 | **与 warmth234 同源治理** |
| `glossary_bank.py` | 黑名单未命中 | Grep 干净 | 口吻升温时抽检白话 |
| `dataset_mapping.json` / `id_metadata.json` | 内部仍「禁止过程合格」 | research §3.1 #14–15 | **勿写回 overlays**；建议改成并肩红线以免误用 |
| overlays 本身 | ~133 软壳课 | 粗分桶 | 改源头后按 Wave 重写；**禁止只手改 sqlite** |

### 5.3 改稿铁律（对齐 Canvas 并肩版 · 软壳升级）

| 要 | 不要（旧战场） | 不要（**今日主战场**） |
|----|----------------|------------------------|
| 2～4 句说完；本课列名/子组/行号/菜单 | 电报：`子组 n=5。子组12起批均值上移。` | 「这条练习线是为「某某菜单」准备的。埋点见第 5 节。」 |
| 「不妨 / 常见读法 / 多半 / 通常还要…」+ **现场细节** | 「你的任务是 / 你应该能 / 这一课只练」 | 「目标不是背菜单，而是…」（说教变体万能壳） |
| 红线带原因 | 纯「禁止过程合格。」 | 「更像一张对照纸条：告诉你该看哪一列…」（可替换菜单名） |
| 「波动主要落在批内还是批间」 | 「抖主要…」 | 「不妨先当一次助手：打开「菜单」…」（有邀请无现场） |
| meaning 写本课现场含义 | 「这一项决定图上或表上对应哪一列」 | 跨课复制同一「无练习表」meaning |

**正确样例锚点（改稿对照）**：Canvas `COLLAB` + 现网 `between_within_capability` / `imr` / `xbar_r`。

---

## 6. verify 新断言规格（Agent4 落地；口吻+软壳为硬门）

扩展：`tools/verify_learning_center_copy_depth.py`、`tools/verify_learning_center_gate.py`、`tools/verify_learning_center_db.py`（按现网分工；禁止另起无关第三套 gate 名除非已有）。

| ID | 断言 | 说明 |
|----|------|------|
| **V1** | `used_for` / `scenario` / `not_for` **最小汉字长度** | 挡电报。建议：`used_for`≥40、`scenario`≥40、`not_for`≥30（汉字计数；Wave-0 可先金标，Wave-5 全集）。禁止再出现 ≤28 汉字的「尖事实堆」作为终态 |
| **V2** | 学员可见区 **旧黑名单** | 禁止匹配：`你的任务是`、`你应该能`、`这一课只练`、`本课只练`、`禁止过程合格`、`抖主要`（扫描 used_for/not_for/scenario/output_guide/common_mistakes/glossary/dialog_fill_detail/prereq/fade/retrieval/misconceptions 等学员可见 JSON 文本；误用里若必须否定，须改写为「不等于… / 通常还要…」完整句，不得保留黑名单原串） |
| **V3** | 课内 `dialog_fill_detail.meaning` **同壳检测** | 同一课内 ≥2 条 meaning **完全相同** → FAIL；或 meaning 含固定壳句「这一项决定图上或表上对应哪一列」→ FAIL |
| **V4** | 金标 `imr` **硬点 + 长度提高** | glossary 仍含 UCL **与** USL；`buried_signals` 含 row/片 **41** 与 **55**；`dialog_fill_detail` length ≥ **9**；`used_for`/`scenario` 长度下限 **高于** 普通课（建议 ≥80 汉字，允许明显加长并肩）；禁止删硬点成摘要 |
| **V5** | 保留上一轮可读性 | `good` 不得像 `^[a-z][a-z0-9_]+$`；先修题干长度等；`catalog_version == learning-center-v2`；无 `dataset_id` 以 `demo_` 开头 |
| **V6** | 7B / 自解释导语 | 学员可见导语区不得出现「自解释」作为教学块标题/导语（与 scrub 一致）；不要求删除 self_explain JSON 列 |
| **V7** | **软壳短语门**（今日新增 · 硬门） | 学员可见 `used_for`/`scenario`/`not_for`/`output_guide` 禁止作为终态主句匹配下列短语（允许极低频例外须人工豁免清单；默认 FAIL）：`这条练习线是为`、`目标不是背菜单`、`更像一张对照纸条`、`不妨先当一次助手`、`还是那张本课练习表`、`导入本课练习表后打开`。**黑名单 0 命中不能代替本条** |
| **V8** | **跨课** `dialog_fill_detail.meaning` 同文上限 | 任意一条 meaning 字符串若在 ≥ **8** 课完全相同 → FAIL（挡「无练习表」等跨课模板壳；阈值可与 research「≥8 课同文」对齐） |
| **V9** | 软壳频次闸（可选加强） | Wave-5 报告：软壳短语命中课数须 **远低于** research 基线（46/50/45/42…）；建议命中课数 = 0，或仅允许白名单课（须写入 verify 注释） |

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

### 本 plan 额外禁止偷懒（针对本轮软壳 · 防执行交差）

- **禁止**把「旧黑名单 0 命中」当成 Wave 出口或 Goal 完成。  
- **禁止**用软壳万能文（练习线是为 / 目标不是背菜单 / 对照纸条 / 当一次助手 / 还是那张本课练习表 / 导入本课练习表后打开）批量交差。  
- **禁止**只改 `imr`（或只改 Wave-0）就宣称 184 完成。  
- **禁止**先改 overlays、后改 `warmth_*` / `copy_depth` / `wave*_content`（顺序颠倒必回灌软壳）。  
- **禁止** Plan / 本阶段改产品代码、overlays、生成器、sqlite、C++。  
- **禁止**建议换模型。  
- **禁止**漏课：184 必须全部进锁表且全部施工；锁表不得写「等等」。  
- **禁止**为润色把条形图改名成 Pareto（H13）。  
- **禁止** Agent 因中文路径强跑易失败的 cmake/package（除非用户本轮明确要求）。  
- **禁止**用「换菜单名的并肩模板」冒充 Canvas 金样级本课故事。  
- **禁止**推倒现网已合格金样课（`imr` / `between_within_capability` / `xbar_r` 等）硬点与真并肩段落。

---

## 8. 文件改动清单 + 明确不改

### 8.1 Agent3 允许改（按强制顺序）

| 路径 | 改什么 |
|------|--------|
| **`tools/learning_data/warmth_wave234_overlays.py`** | **拆软壳万能句**；改为按课可注入现场细节的真正并肩生成（或停止批量灌壳，改为课级手写/表驱动） |
| **`tools/learning_data/warmth_wave1_overlays.py`** | 控制图包核对+补洞；禁止回退已达标课 |
| **`tools/learning_data/copy_depth.py`** | 加深 `_role_aware_meaning` / 本课专义；挡跨课同文壳 |
| `tools/learning_data/wave1_content.py` | 与 warmth1 同源；防电报/说教/软壳回归 |
| `tools/learning_data/wave2_content.py` | 与 warmth234 同源治理 |
| `tools/learning_data/wave3_content.py` | 与 warmth234 同源治理 |
| `tools/learning_data/wave4_content.py` | 与 warmth234 同源治理；图形名实不回归 |
| `tools/learning_data/glossary_bank.py` | 白话并肩化（稳词、邀请语气） |
| `tools/learning_data/tutorial_overlays/*.json` | 184 课学员可见文案（Wave-0 先复核/定点加长 `imr.json`） |
| `tools/learning_data/dataset_mapping.json` / `id_metadata.json` | **可选**：内部「禁止过程合格」改并肩红线；**禁止**写回学员 overlays |
| `tools/build_learning_center_db.py` | merge 后重建；**不**改 `META_VERSION`（保持 v2） |
| `tools/verify_learning_center_copy_depth.py` | V1–V9 口吻/软壳断言 |
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
| Plan 阶段任何 overlays / py / sqlite / C++ | Agent2 纪律 |

---

## 9. 金标 `imr`：加长但硬点清单（允许 0–6 明显加长并肩）

Agent3 改 `imr.json` 时**允许**加温度、过渡、「不妨/常见读法/通常还要」；下列硬点**必须仍可被 Agent4/6 指认**（对照金标 Canvas + 前两轮 verify）。现网已接近达标：**定点补短板，禁止推倒**。

| # | 硬点 | 验收 |
|---|------|------|
| G1 | 片号 **41**（均值阶跃开始） | `buried_signals` / 读输出 / 场景文案可指认；不得删成「后段上移」无片号 |
| G2 | 片号 **55**（尖峰 → I 图 UCL 线索） | 同上；不得删成摘要 |
| G3 | **UCL ≠ USL** | glossary 同时出现 UCL 与 USL（及白话区分）；读图/错念不得暗示「出 UCL=超规格废品」 |
| G4 | 参数表 **9 字段** | `dialog_fill_detail` length ≥ 9（变量、阶段列、MR 长度、σ 方法、Nelson estimate、规则默认策略、特殊原因测试、历史均值、历史 Sigma 等真实对话框字段） |
| G5 | `dialog_fill` | 仍为 object：`{"variables":"锡膏高度_um"}`（不改为数组） |
| G6 | 工作表名 | 导入后 `demo_imr_spi_shift`（dataset_id=`imr_spi_shift`，不以 `demo_` 开头） |
| G7 | 练习表 | `imr_spi_spike_b` 仍可被 fade 引用；不映射第二 command |
| G8 | 口吻 | 去掉「本课只练…」说教收束；**禁止**软壳万能文；0–6 可明显加长；7A/7C/7D/7E 并肩；**不**恢复 7B UI |

**长度**：金标 `used_for`/`scenario` 下限高于普通课（见 V4）；加长≠抒情散文（手册 §7.22）。

---

## 10. Agent3 强制开工顺序

1. Wave-0：复核/定点加长 `imr.json` 并肩金标（§9 硬点清单可勾）→ **口吻闸门**。  
2. **先**改 `warmth_wave234_overlays.py` + `warmth_wave1_overlays.py` + `copy_depth.py` + `wave1/2/3/4_content.py` + `glossary_bank.py` — **拆软壳源头**（今日主战场）。  
3. Wave-1（控制图包 21，**含 imr 复核**）→ gate → Wave-2（24）→ gate → Wave-3（56）→ gate → Wave-4（83）→ gate。  
4. Wave-5：verify V1–V9 落地、scrub、builder 重建、文档。  
5. 交给 Agent4（Python 口吻/软壳断言）→ Agent6 QA → Agent5 commit/push；提示用户本机 `package_dist` 抽查口吻。

并行纪律：禁止两人同时改同一大 `warmth_*` / `wave*_content.py` 或 `build_learning_center_db.py`。overlay 可按菜单包并行，merge 前跑 verify。

---

## 11. Agent2 DoD + 风险 + go/no-go

### 本阶段交付文件列表

| 文件 | 动作 |
|------|------|
| `docs/research/goal-learning-center-copy-voice-warmth-wave-plan.md` | **按 2026-09-05 调研重写**（本文件） |

**未改**任何产品代码 / overlays / py 生成器 / sqlite / C++。

### Agent2 DoD 勾选

- [x] 文首抄入现网硬约束 **H1–H13**  
- [x] Wave 锁表复用前一轮 **184 id**，Wave-0…5 全表列出，**不得漏 id**  
- [x] 写清 **「金标闸门」与「Wave-1 包内仍含 imr」**；唯一并集计数 **184**（1+20+24+56+83）  
- [x] Wave 出口标准对照手册 **§4.2**（含软壳终态禁止）  
- [x] 字段检查单（used_for/not_for/scenario/glossary/dialog_fill_detail/埋点/读输出/误用/7A·7C·7D·7E + 软壳禁项）  
- [x] 生成器改法：重点 **`warmth_wave234_overlays.py` / `warmth_wave1_overlays.py` / `copy_depth.py` / `wave*_content.py`**；主战场 = 软壳短语，非旧电报  
- [x] verify 新断言：长度下限、旧黑名单、**软壳短语**、跨课 meaning 上限、金标硬点  
- [x] **§7 禁止偷懒全文**（手册 22 条）已粘贴  
- [x] 额外禁止偷懒：禁止黑名单 0 命中当完成；禁止软壳万能文交差；禁止只改 imr  
- [x] 文件改动清单 + 「明确不改哪些文件」  
- [x] 金标 imr 加长但硬点清单（片 41/55、UCL≠USL、参数表 9 字段等）  
- [x] **禁止** Plan 阶段改产品代码  
- [x] 未建议换模型；未重问 §0  

**风险（一行）**：若 Agent3 不先拆 `warmth_wave234_overlays.py` 软壳源头、却把「旧黑名单 0 命中」当完成，重建库后仍会把约 130+ 课万能软壳交付给学员。

**go/no-go**：**GO** — 可进入 Agent3 执行（先 Wave-0 `imr` 并肩金标闸门 + 拆 warmth/copy_depth/wave* 模板源头，再按锁表铺完唯一并集 184）。
