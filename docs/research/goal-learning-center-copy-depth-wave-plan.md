# 学习中心文案加深 + 图形名实对齐 — Agent2 Wave 计划（锁表）

> **岗**：Agent2 Plan（**禁止改产品代码**）  
> **日期**：2026-09-03  
> **权威手册**：[`goal-learning-center-copy-depth-and-graph-alignment-plan-and-mega-prompt.md`](goal-learning-center-copy-depth-and-graph-alignment-plan-and-mega-prompt.md)  
> **调研总册**：[`learning-center-copy-depth-and-graph-alignment-research.md`](learning-center-copy-depth-and-graph-alignment-research.md)  
> **前一轮锁表（id 全集复用）**：[`goal-learning-center-pedagogy-upgrade-wave-plan.md`](goal-learning-center-pedagogy-upgrade-wave-plan.md)  
> **模型**：inherit（禁止建议换模型）

---

## 文首硬约束（写进执行提示词）

| ID | 约束 |
|----|------|
| H1 | `catalog_version` **保持** `learning-center-v2`。禁止只改 sqlite、禁止升 v3 |
| H2 | `dataset_id` **不以 `demo_` 开头**。工作表仍是 `demo_{dataset_id}` |
| H3 | 文案必须进 `build_learning_center_db.py` / `tutorial_overlays` / `wave*_content.py` |
| H4 | 同构共享仍以前一轮 wave-plan **§3 白名单**为准（8 族 / 17 id）；禁止回归旧 10 表 |
| H5 | `dialog_fill` 保持角色→列名 object；详解走 `dialog_fill_detail` |
| H6 | 禁止为术语课新增虚假 command_id |
| H7 | 禁止推倒 `LearningCenterPage` 导航树 / 导入 / `WorksheetRegistry` |
| H8 | 禁止改 `algorithm_help.json` 公式语义 |
| H9 | 金标 `imr` 的 0–6（片 41/55、UCL≠USL、9 字段参数表）**不得删成摘要**；7+ 按本 Goal 重写可读性 |
| H10 | 练习 JSON **增量** `why`/`hint`；解析器缺字段降级为空 |
| H11 | 图形 `title`/`click_steps` 菜单名 = `analysis_commands.menu_label`；`related_ids` 禁止模板三件套 |

### §0（禁止改口）

Q1 inherit · Q2 184 全集 · Q3 补充不推倒 · Q4 非开发读者 · Q5 保持 v2 · Q6 揭晓读本课 hint/why · Q7 图形全表定点修 · Q8 Python verify / 用户本机 Qt。

**计数不得截断**：Wave-0=1，Wave-1=20，Wave-2=24，Wave-3=56，Wave-4=83，Wave-5=0 新 id。**1+20+24+56+83=184**。id 列表与前一轮 §2 **逐字相同**（仅本文件 Semi 语义改为「文案加深」，不是换 dataset）。

---

## 1. Wave 总表（本 Goal 施工语义）

| Wave | 施工内容 | n | 出口 |
|------|----------|---|------|
| **Wave-0** | catalog 解析兼容（why/hint、检索双形态）+ 练习揭晓 UI 读本课 JSON + **`imr` 7+ 金标加深**（0–6 不砍） | 1 | Agent4 可读性闸门 + 金标 7+ 不像 API |
| **Wave-1** | 控制图包 7+ 与过短 0–6 补句（除已在 Wave-0 的 `imr`） | 20 | gate：无内部 id 当 good；每课 self_explain 有 hint |
| **Wave-2** | 质量 / MSA / 能力 | 24 | 同上 |
| **Wave-3** | 推断 / ANOVA / 回归相关 | 56 | gate |
| **Wave-4** | **图形名实对齐** + DOE / 可靠性 / 其余 | 83 | 图形红表清零；related 去模板化；dialog_fill 补必填 |
| **Wave-5** | 生成器电报模板清干净 + verify 新断言 + 文档 | 0 | Agent5+6 |

**禁止**：金标 7+ 未达标就批量灌水；也禁止金标过后借口「下一 Goal 再铺 184」。

### Wave-0 DoD

- [ ] `LearningPrereqItem` 可选 `why`；`LearningSelfExplain` 必解析 `hint`（缺省空）
- [ ] 检索：`QStringList` 升级为可解析 string **或** `{q,hint}`；UI 用 `q` 显示、`hint` 揭晓
- [ ] `parse_*` 旧 JSON 不崩
- [ ] `LearningCenterPage`：先修反馈优先 `why`；自解释揭晓 = 本课 `hint`（删除「prompt 含 Nelson 则套 I-MR 话术」的全课特例，金标把原理写进自己的 hint）；检索揭晓读 hint，**禁止**无条件 UCL 套话
- [ ] **不**改左树 / 导入按钮 / 分节折叠骨架
- [ ] `imr.json` 0–6 与 Canvas 深度保留（片 41/55、UCL≠USL、9 行 detail）
- [ ] `imr` 7+ 达到手册 §4.2（完整问句、选项成句、hint 3–6 句、检索对象化）
- [ ] 禁止只改 sqlite

### Wave-1…4 每波共同 DoD

- [ ] 锁表每一个 id 都改 overlay（或该波 `wave*_content` 源头），不得跳 id
- [ ] 先修/检索不再出现纯内部 id 当正确答案（`^[a-z][a-z0-9_]+$`）
- [ ] 先修 `q` ≥12 汉字；`good`/`bad` ≥8 汉字且长度接近
- [ ] 每课 ≥2 条 self_explain 且均有 `hint`
- [ ] 检索 ≥3，新内容必须 `{q,hint}`
- [ ] 错念 `wrong`/`right` 完整句；`right` 讲对在哪
- [ ] 过短 0–6 `meaning` 补成可跟做说明；**禁止**删成摘要
- [ ] 学员可见区无：`command_id`、`同构`、`白名单`、`WAVE`、`role_map`；`dataset_id` 不以教学内容出现（工作表显示名 `demo_*` 可出现一次并解释）
- [ ] 非控制图课禁止无条件考 UCL
- [ ] 图形课（Wave-4）：title 含菜单中文；有 dataset 则必填角色都在 `dialog_fill`；related 非模板三件套
- [ ] Python gate 对该波阈值 PASS

### Wave-5 DoD

- [ ] `wave4_content.py` compact 循环不再写死 related 三件套与「dataset？」「禁止句」
- [ ] 各 `wave*_content._seven_plus` 默认文案不再是电报
- [ ] verify 可读性断言已落地且全集跑过
- [ ] 文档指向本 Goal research + 本 plan

---

## 2. command_id 锁表（复用前一轮，不得漏 id）

附加非 command：练习集 `imr_spi_spike_b`（只进 datasets；fade 引用）。dataset 列**不要改**，本 Goal 不是换表。

### Wave-0（n=1）

| command_id | dataset_id |
|---|---|
| `imr` | `imr_spi_shift` |

### Wave-1（n=20）— 与前一轮逐 id 相同

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

### Wave-2（n=24）

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

### Wave-3（n=56）

与前一轮 §2 Wave-3 **逐行相同**（勿漏）：`anom`, `anom_attribute`, `best_subsets_regression`, `bootstrap_mean`, `bootstrap_two_sample`, `chi_square`→`cat_shift_line`, `chi_square_gof`→`gof_category_bias`, `cochran_q`→`cochran_three_repeat`, `correlation`→`corr_temp_offset_y`, `cross_tabulation`→`cat_shift_line`, `descriptive`→`desc_unimodal_stable`, `fisher_exact`→`fisher_small_counts`, `friedman`→`friedman_three_treat`, `general_manova`, `glm_three_factor`, `glm_two_way`, `kruskal_wallis`→`kw_three_cavity`, `logistic_regression`→`logit_pass_fail`, `mann_whitney`→`infer_two_sample_location`, `manova_one_way`, `mcnemar`→`mcnemar_paired_binary`, `mixed_effects_reml`, `mood_median`→`mood_two_group`, `nominal_logistic`, `nonlinear_regression`, `normality_test`→`norm_mild_skew`, `one_poisson_rate`→`pois_one_count`, `one_proportion`→`prop_one_lot`, `one_proportion_equivalence`→`equiv_prop_one`, `one_sample_equivalence`→`equiv_one_near_target`, `one_sample_t`/`one_sample_z`→`infer_one_sample_mean`, `one_way_anova`→`anova_one_cavity`, `ordinal_logistic`, `orthogonal_regression`, `outlier_test`→`outlier_one_spike`, `paired_equivalence`→`equiv_paired_near`, `paired_t`→`infer_paired_shift`, `pls_regression`, `poisson_gof`, `poisson_regression`, `randomization_test`, `regression`→`regr_temp_strength`, `runs_test`→`runs_clustered`, `sign_test`→`infer_paired_shift`, `stepwise_regression`, `t_power`, `two_factor_anova`→`anova_two_factor`, `two_poisson_rate`→`pois_two_count`, `two_proportion_equivalence`→`equiv_prop_two`, `two_proportions`→`prop_two_line`, `two_sample_equivalence`→`equiv_two_near_equal`, `two_sample_equivalence_ratio`→`equiv_ratio_near_one`, `two_sample_t`→`infer_two_sample_location`, `variance_test`→`var_two_line_unequal`, `wilcoxon_signed_rank`→`infer_paired_shift`。无 dataset 的保持空。权威两列表：前一轮 plan 第 169–228 行。

### Wave-4（n=83）

权威两列表：前一轮 plan 第 230–316 行（`accelerated_life`…`weibayes`）。**本波必须消化调研 §3 红表**。图形有表者：`area_plot`→`graph_area_time`，`bar_chart`→`graph_bar_category`，`histogram`/`probability_plot`→`graph_hist_prob`，以及 box/bubble/contour/corr/density/ecdf/eda4/heatmap/hexbin/interval/marginal/matrix/mosaic/parallel/pie/scatter/simplex/time_series*/violin 等与前一轮相同。

### Wave-5

无新 id。

**勾 id 时同时打开前一轮 plan §2 全表；Wave-3/4 全表以该文件为准，本文件不截断计数。**

---

## 3. 每课文案检查单（Agent3 对照填写）

```text
[ ] 0–6：过短 meaning/used_for 已补句；金标未砍
[ ] glossary：术语首次白话；无电报缩写代替正经名
[ ] click_steps：菜单中文 = analysis_commands.menu_label
[ ] 学员区无开发口径
[ ] prereq ≥3：完整问句 + 成句选项 + 可选 why
[ ] self_explain ≥2：after=学员动作；hint 3–6 句讲为什么
[ ] fade 0/1/2：学员句能独立执行
[ ] retrieval ≥3：{q,hint}
[ ] misconceptions ≥2：完整句
[ ] 非 SPC：先修/检索/揭晓不无条件考 UCL
[ ] 图形：title / fill 必填 / related 非三件套 / 图标借用一句人话
```

---

## 4. JSON 解析兼容策略（不升 catalog 版本）

**不新增 sqlite 列**（Q5）：仍用现有 `prereq_quiz` / `self_explain` / `retrieval_quiz` TEXT。

```text
prereq_quiz:     [{q, good, bad, why?}]
self_explain:    [{after, prompt, hint}]      -- 新内容 hint 必填
fade_levels:     [{level, student, scaffold}] -- 只加长文案
retrieval_quiz:  ["旧字符串"] 或 [{q, hint}] -- 解析器两者都收
misconceptions:  [{wrong, right}]
```

C++：

- `LearningPrereqItem::why`
- `LearningSelfExplain::hint`
- 检索：新增 `struct LearningRetrievalItem { QString q; QString hint; }` + `QVector`；或保留 `QStringList` 另存 hint 平行数组。**推荐** `QVector<LearningRetrievalItem>`，string 元素 → `q=原文, hint=""`。
- `parse_retrieval`：array 元素是 string 或 object。
- 旧库 / 旧 overlay 缺 `hint`/`why` → 空字符串；UI 中性句。

**禁止** `META_VERSION` / `kExpectedCatalogVersion` 改成 v3。

---

## 5. verify 新断言（Agent4 落地；Wave-0 可先金标阈值，Wave-5 全集）

文件：`tools/verify_learning_center_db.py`、`tools/verify_learning_center_gate.py`（按现网分工扩展，禁止另起第三套 gate 名字除非现网已有）。

| ID | 断言 |
|----|------|
| R1 | 每课 `prereq_quiz` ≥3；每题 `q` 汉字 ≥12；`good`/`bad` 汉字 ≥8 |
| R2 | `good`/`bad` 不得匹配 `^[a-z][a-z0-9_]+$` |
| R3 | 每课 `self_explain` ≥2；每条 `hint` 非空；hint 不得等于 prompt |
| R4 | `retrieval_quiz` ≥3；元素为 object 则 `q` 与 `hint` 均非空；**新生成 overlay 禁止纯短字符串**（Wave-5 全集对象化） |
| R5 | 非控制图 command 的 prereq/retrieval 文本禁止出现无语境「UCL」（允许能力课谈 USL） |
| R6 | 图形 `menu_path==图形`：overlay `title` 包含 `menu_label` |
| R7 | related_ids 不得 **恰好** 为 `{histogram, scatter_plot, graph_gallery}`（允许画廊课自己引用其中子集并另加说明） |
| R8 | 有 `dataset_id` 的图形课：每个非 optional role 都是 `dialog_fill` 的键（值非空） |
| R9 | 金标 `imr`：glossary 含 UCL 与 USL；buried row 41 与 55 |
| R10 | `catalog_version == learning-center-v2`；无 `dataset_id` 以 `demo_` 开头 |

C++ target（用户本机编，Agent 不强跑 cmake）：

- `learning_center_store_test`
- `learning_center_worksheet_registry_test`
- `learning_center_analysis_sample_test`

解析器形状变化时对齐测试；金标 0–6 断言保留。

---

## 6. 图形红表施工队列（Wave-4 必须清零）

调研来源：research §3。

**A. dialog_fill 缺必填（有专用 dataset）**

| id | 补进 dialog_fill |
|----|-----------------|
| `scatter_plot` | `y_variable`=偏移_um（与 click_steps 一致） |
| `interval_plot` | `category`=腔号 |
| `bubble_plot` | `y_variable` + `size_variable` |
| `hexbin_plot` | `y_variable` |
| `marginal_plot` | `y_variable` |
| `time_series_plot` | `value`=产量 |
| `area_plot` | `value`=产量 |
| `contour_plot` | `y_variable` + `z_variable` |

**B. title / 菜单**

| id | 现网 | 改为 |
|----|------|------|
| `interval_plot` | 区间图 | **区间散点图**（click_steps 同步） |
| `probability_plot` | 正态概率图 | **概率图——正态参考线**（主名可检索） |
| `graph_gallery` | 探索性图形画廊 | **探索性图形**（副标题可破折号） |

**C. related_ids** 去掉模板三件套；映射用 research §3.2。

**D. 图标借用** 不改 `analysis_commands` 图标键；在 used_for 或 glossary 加一句「软件图标可能借用相近图，菜单以左侧树为准」。`bar_chart` **标题保持条形图**。

**E. 空 dataset 课**（`dotplot`/`correlogram`/`chi_square_mosaic_link`/`graph_gallery` 等）：不强行编造 fill；click_steps 保持诚实。

**F. histogram related** 含 `density_plot`：改为不暗示同表；文案禁止「同构」。

源头：`wave4_content.py` 约 1542 行 `related=["histogram", "scatter_plot", "graph_gallery"]` 及 compact prereq「dataset？」。

---

## 7. 精确文件清单

### 5.1 允许改

| 路径 | 本 Goal |
|------|--------|
| `src/application/learning/learning_types.h` | why / hint / retrieval 结构 |
| `src/application/learning/learning_tutorial_catalog.cpp` / `.h` | 兼容解析 |
| `src/ui/learning_center_page.cpp` | **只**练习揭晓来源（可极小改 .h 若要新 helper） |
| `tools/learning_data/wave1_content.py` … `wave4_content.py` | 电报模板源头 |
| `tools/learning_data/tutorial_overlays/*.json` | 184 课（由生成器写出，允许金标手写 imr 后 builder merge） |
| `tools/build_learning_center_db.py` | merge 新键；**不**改 META_VERSION |
| `tools/verify_learning_center_db.py` / `verify_learning_center_gate.py` | R1–R10 |
| `tests/learning_center_*_test.cpp` | 仅解析形状 / 金标保留 |

### 5.2 明确不改

| 路径 | 为何 |
|------|--------|
| `analysis_service.*` | 禁止顺手改内核 |
| `algorithm_help.json` | 公式语义 |
| `analysis_commands.cpp` | 只读对齐；不为图名改菜单 id |
| `worksheet_registry.*` / `import_learning_dataset` 语义 | 保持新建表 |
| `LearningCenterPage` 左树/搜索/导入/公式按钮布局 | H7 |
| `CMakeLists.txt` / `package_dist.ps1` | 除非测试破编译 |
| 手工 sqlite hex | H3 |
| 第 185 个假 command_id | H6 |

---

## 8. 禁止偷懒（手册 §7 全文粘贴）

1. **禁止**推倒学习中心窗口/导入链路。  
2. **禁止**丢掉前一轮 0–6 骨架、专用表、白名单、v2 catalog。  
3. **禁止**金标 7+ 未达标就批量灌水；也**禁止**金标过后借口下轮再铺 184。  
4. **禁止**学员可见区出现 `dataset_id` / `同构` / `WAVE` 当教学内容。  
5. **禁止**先修/检索用「列数？」「共享族？」「禁止句」当题干。  
6. **禁止**揭晓只把题目或选项复读一遍。  
7. **禁止**非控制图课无条件考 UCL。  
8. **禁止**图形 `related_ids` 再写死 histogram+scatter+gallery。  
9. **禁止**为对齐图名去改 `algorithm_help` 公式或乱改菜单 id。  
10. **禁止**把条形图教程改名为 Pareto（图标借用 ≠ 菜单名）。  
11. **禁止**只手改 sqlite。  
12. **禁止**中途换模型。  
13. **禁止** Agent6 没出 QA 报告就 UpdateGoal complete。  
14. **禁止**用浏览器代替用户本机 package（除非用户本轮再要求）。  
15. **禁止**把练习 JSON 删空赶进度。  
16. **禁止**新增虚假 command_id。  
17. **禁止** `dialog_fill` 覆盖成数组。  
18. **禁止** `dataset_id` 以 `demo_` 开头。  
19. **禁止**升 catalog 到 v3（本 Goal 已锁定保持 v2）。  
20. **禁止**并行两人改同一 cpp。  

另（本 plan）：**禁止**只润色 `imr` 就把 Wave-1…4 标完成；**禁止** UI 重做 0–6 折叠。

---

## 9. Agent2 DoD

- [x] Wave-0 = 揭晓 UI + `imr` 7+ 金标加深（0–6 不砍）  
- [x] Wave-1…5 铺完全集 184，不得「等等」  
- [x] 图形红表进入施工队列  
- [x] **禁止** Plan 阶段改产品代码  
- [x] JSON 兼容、verify 新断言、文件清单、§7 禁止偷懒全文  

**风险一行**：184 课 7+ 若仍用手改 overlay 而不改 `wave*_content` 模板，Wave-5 会被电报回灌。  

**go/no-go**：**go Agent3**（先 Wave-0：types + catalog + page 揭晓 + `imr` 7+，再改生成器源头连铺 1…5）。
