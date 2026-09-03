# 学习中心文案加深 + 图形名实对齐 — Agent1 调研总册

> **岗**：Agent1 Research（**禁止改产品代码**；本文件为只读调研）  
> **日期 / 访问日期**：2026-09-03（UTC+8）  
> **权威手册**：[`goal-learning-center-copy-depth-and-graph-alignment-plan-and-mega-prompt.md`](goal-learning-center-copy-depth-and-graph-alignment-plan-and-mega-prompt.md)  
> **前一轮基线（必须保留）**：[`goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md`](goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md)  
> **前一轮锁表（184 id 可复用）**：[`goal-learning-center-pedagogy-upgrade-wave-plan.md`](goal-learning-center-pedagogy-upgrade-wave-plan.md)  
> **金标 Canvas（0–6 仍有效）**：`C:\Users\孤白赟悫\.cursor\projects\d-QT-CppPrograms-DataLab\canvases\learning-center-tutorial-example.canvas.tsx`  
> **模型**：inherit（与主对话相同；禁止建议换模型）  
> **扫描口径**：`src/ui/analysis_commands.cpp` 中 `menu_path == "图形"` 共 **27** 条；`tools/learning_data/tutorial_overlays/*.json` **184** 课全量。命令解析器匹配到 **181** 条 `AnalysisCommand`（另 3 课无独立菜单 id，见 §3.0）。

本文件是本 Goal 的**调研总册**。Agent2 据此写 Wave 锁表与施工队列；**不得**在本岗改 C++ / 生成器 / sqlite。

---

## 0. §0 已锁定（禁止重问）

| # | 锁定 |
|---|------|
| Q1 | 全程 inherit 当前对话模型 |
| Q2 | **184 课全集**加深 7+ 与过短 0–6；Wave 只是施工队列 |
| Q3 | **补充不推倒**：专用主集、同构白名单、0–6 分节、导入、`learning-center-v2`、金标 `imr`/`imr_spi_shift` 的 0–6 |
| Q4 | 读者：非开发、统计基础薄弱 |
| Q5 | catalog **保持 v2**；JSON 可在现有列内增量 `hint`/`why` |
| Q6 | 揭晓只显示本课 JSON 参考；禁止全课套 UCL |
| Q7 | 图形名实必须全表审计 |
| Q8 | Agent 跑 Python verify；用户本机 Qt / `package_dist` |

---

## 1. 给新手写练习题（网上巩固 + 落到本产品）

前一轮 pedagogy research 的 worked example / self-explanation / CLT 仍然成立。本轮补的是 **题干自足、选项可读、反馈讲为什么**。

### 1.1 Primary URL（2026-09-03 核实）

| 来源 | URL | 采纳到本产品 |
|------|-----|----------------|
| Manchester FSE：Writing MCQs | https://www.teachingcollege.fse.manchester.ac.uk/writing-multiple-choice-questions-a-handy-guide/ | 题干必须自成问题；干扰项用**常见错念**；形成性测验的反馈要能让猜对的人也知道「为什么」 |
| University of Manitoba CATL | https://umanitoba.ca/centre-advancement-teaching-learning/support/multiple-choice-questions | **不看选项也应能理解题目在问什么**；选项长度/语法同质，避免靠长短猜 |
| The eLearning Coach：MCQ rules | https://theelearningcoach.com/elearning_design/rules-for-multiple-choice-questions/ | 中心思想放题干；直接问句优于残缺填空；解释要覆盖对与错 |
| Haladyna, Downing, Rodriguez (2002) *A Review of Multiple-Choice Item-Writing Guidelines* | https://site.ufvjm.edu.br/fammuc/files/2016/05/item-writing-guidelines.pdf | Guideline 15：中心思想在 stem，禁止 unfocused stem（「效度？」这种半截题）；选项大致等长 |
| 前一轮 pedagogy research | [`learning-center-pedagogy-upgrade-research.md`](learning-center-pedagogy-upgrade-research.md) §1 | fade / 自解释 / 检索仍要；本轮是把槽位从「开发电报」改成「课堂提问」 |

Vanderbilt CFT 旧页 `writing-good-multiple-choice-test-questions` 已改版跳到 Office of Learning Innovation 门户（2026-09-03 fetch 无原指南正文）。**不以该门户为正文源**；Brame 类主张以 Manchester / Manitoba / Haladyna 为准。

### 1.2 落到 DataLab 学习中心的规则（给 Agent2/3）

| 外部主张 | 本课 JSON / UI |
|---------|----------------|
| Stem 自足 | `prereq_quiz.q` / `retrieval_quiz.q` ≥12 个汉字的完整问句；禁止「列数？」「dataset？」 |
| 选项同质、像句子 | `good`/`bad` ≥8 个汉字、长度接近；**禁止** `^[a-z][a-z0-9_]+$` 当正确答案 |
| 干扰项 = 错念 | `bad` 用车间常见误读（UCL=规格、已证明正态、量具通过），不要用「旧10表」这种开发黑话 |
| 反馈讲原理 | 先修用可选 `why`；自解释必填 `hint`（3–6 句，禁止复读 `prompt`）；检索对象带 `hint` |
| 形成性、不羞辱 | UI 揭晓读本课字段；缺省中性句「请回到上面第 0 节关键词和第 5 节读图」；**非控制图课禁止无条件提 UCL** |
| 一题一事 | 先修考概念，不把「禁止句」三个字当检索答案 |

**金标 `imr` 7+ 现状**：题干已是中文问句（明显好于图形/推断电报），但 (1) 若干 `q` 汉字仍可能 <12；(2) `self_explain` **全部无 `hint` 字段**；(3) `retrieval_quiz` 仍是纯字符串；(4) UI 揭晓仍复读题目或套 Nelson/UCL。本轮 **0–6 保留**，7+ 按手册 §4.2 加深，不是推倒。

---

## 2. 补充而非推倒的边界

### 2.1 必须保留

- 专用主集 + 同构白名单（wave-plan §3，8 族 / 17 id）  
- `catalog_version = learning-center-v2`（禁止升 v3）  
- 右侧 0–6 分节折叠 + 7+ 练习区骨架  
- `MainWindow` 导入 → `LearningDatasetStore` → `WorksheetRegistry`（新建工作表，不覆盖当前表）  
- 金标 `imr.json` 片 41/55、UCL≠USL、9 字段参数表  
- `dialog_fill` 继续是角色→列名 **object**

### 2.2 本 Goal 允许改

- overlay / `wave*_content.py` / `build_learning_center_db.py` 学员可见文案  
- `learning_types.h` 增量：`LearningPrereqItem::why`、`LearningSelfExplain::hint`、检索双形态  
- `learning_tutorial_catalog.cpp` 兼容旧 JSON（缺键不崩）  
- `LearningCenterPage` **只改练习揭晓来源**（及 glossary 页眉若仍无条件提 UCL——页眉现写「控制限课请盯住」，可保留；**检索揭晓**必须改）  
- `tools/verify_learning_center_*.py` 可读性断言  

### 2.3 明确不改

- 第二套帮助系统；`algorithm_help.json` 公式语义；分析向导内嵌学习中心  
- 推倒左树 / 导入按钮 / Table 内核  
- 把专用集再合成 10 张宽表  
- 把条形图教程改名为 Pareto  
- 只手改 sqlite  

---

## 3. 图形名实全表（27 / 27，不是抽样）

**范围定义**：`analysis_commands.cpp` 注释块「图形」起，所有 `menu_path == "图形"`。  
**字段**：菜单中文 = `menu_label`；对话框标题 = `dialog_title`；图标键 = `icon_file`（`:/icons/<file>.svg`）；`chart_type` 来自 apply 里赋值；必填角色 = `RoleSpec.optional == false`。

**灯号**：红 = 本 Goal 必须定点修；琥珀 = 应用一句人话说明或把 title 收成菜单名，不算「可以不管」。

### 3.0 课数口径

| 集合 | 数量 |
|------|------|
| overlay JSON | 184 |
| `analysis_commands` 解析命中 | 181 |
| overlay 有、命令表无独立 id | `special_cause_rules`、`report_templates`、`database_import`（术语/工具课，禁止伪造第 185 个分析 id） |
| 命令有、overlay 无 | **0** |
| 图形菜单 | **27** |

### 3.1 对照表

| id | 菜单中文 | 对话框标题 | overlay `title` | 图标键 | chart_type | 必填角色 | dialog_fill 现网 | 缺必填 | related_ids | 灯号 |
|----|----------|------------|-----------------|--------|------------|----------|------------------|--------|-------------|------|
| `histogram` | 直方图 | 直方图 | 直方图 | histogram | histogram | variables | variables=厚度_um | — | probability_plot, density_plot, capability | 琥珀：related 含 density（白名单禁止与 hist **同表**；相关菜单可以，文案须写清「另开密度图课、另一张表」）；先修电报+考 UCL |
| `probability_plot` | 概率图 | 正态概率图 | **正态概率图** | probability | probability_plot | variable | variable=厚度_um | — | histogram, normality_test | **红 title**：主名应可检索「概率图」（建议 `概率图——正态参考线`）；文案含「同构/白名单/variable 角色」 |
| `eda_4plot` | EDA 四图 | EDA 四图（NIST） | EDA 四图 | **histogram** | eda_4plot | variables | variables=厚度_um | — | **模板三件套** | **红 related + 图标借用**：须写「四宫格含游程/直方/概率/滞后，不是再画一张直方图」 |
| `boxplot` | 箱线图 | 箱线图 | 箱线图 | boxplot | boxplot | variables | variables=厚度_um | — | 模板三件套 | 红 related；建议 related→violin / interval / capability |
| `dotplot` | 点图 | 点图 | 点图 | **histogram** | dotplot | y_variable | {} | y_variable | histogram, scatter_plot | 琥珀：dataset 空（锁表）故无 fill 可接受；图标借用须一句说明 |
| `density_plot` | 密度图 | 密度图 | 密度图 | **histogram** | density_plot | variable | variable=厚度_um | — | 模板三件套 | 红 related+图标；建议 related→histogram, ecdf（并写清**不共享** hist 表） |
| `ecdf_plot` | 经验累积分布图 | 同左 | 经验累积分布图 | ecdf | ecdf_plot | variable | variable=厚度_um | — | 模板三件套 | 红 related |
| `violin_plot` | 小提琴图 | 小提琴图 | 小提琴图 | **boxplot** | violin_plot | variables | variables=厚度_um | — | 模板三件套 | 红 related+图标借用 |
| `hexbin_plot` | Hexbin | Hexbin / 二维分箱 | Hexbin | **scatter** | hexbin_plot | x,y | 仅 x | **y** | 模板三件套 | **红 fill + related + 图标** |
| `bar_chart` | **条形图** | 条形图 | **条形图**（勿改 Pareto） | **pareto** | bar_chart | category | category=缺陷类别 | —（value 可空计频，detail 已写） | 模板三件套 | **红 related + 图标**；value 过短「可空=计频。」须补句；**禁止改教程名为 Pareto** |
| `pie_plot` | 饼图 | 饼图 | 饼图 | pie | pie_plot | category | category=缺陷类别 | — | 模板三件套 | 红 related→bar_chart |
| `scatter_plot` | 散点图 | 散点图 | 散点图 | scatter | scatter_plot | x,y | 仅 x=温度_C | **y**（步骤写了偏移_um） | 模板三件套 | **红 fill**（导入预填缺 Y）+ related |
| `graph_gallery` | 探索性图形 | Graph Gallery | **探索性图形画廊** | **scatter** | graph_gallery | 皆可选 | {} | — | histogram, descriptive | 琥珀 title 多「画廊」；图标借用；dataset 空 |
| `interval_plot` | **区间散点图** | 区间散点图 | **区间图** | interval | interval_plot | response, category | 仅 response | **category** | 模板三件套 | **红 title + fill + related**；click_steps 菜单写成「区间图」 |
| `correlation_plot` | 相关图 | 相关图 | 相关图 | correlation | correlation_plot | variables | 三列 | — | 模板三件套 | 红 related→matrix / heatmap |
| `bubble_plot` | 气泡图 | 气泡图 | 气泡图 | bubble | bubble_plot | x,y,size | 仅 x | **y, size** | 模板三件套 | **红 fill + related** |
| `simplex_design_plot` | 混料三角图 | Simplex Design Plot | 混料三角图 | **mixture_design** | simplex_design_plot | components | 三组分 | — | 模板三件套 | 红 related（应→mixture_design/mixture_analyze）+ 图标借用 |
| `mosaic_plot` | 马赛克图 | Mosaic Plot | 马赛克图 | **bar_chart** | mosaic_plot | categories | 班次,缺陷类别 | — | 模板三件套 | 红 related→pie/chi_square_mosaic_link + 图标 |
| `chi_square_mosaic_link` | 卡方–马赛克联动 | Chi-Square Mosaic Link | 同菜单 | **chi_square** | chi_square_mosaic_link | row, column | {} | 两列 | histogram, scatter | 琥珀：dataset 空；图标借用；related 仍像模板 |
| `matrix_plot` | 矩阵图 | 矩阵图 | 矩阵图 | matrix | matrix_plot | variables | 三列 | — | 模板三件套 | 红 related→scatter / correlation |
| `marginal_plot` | 边际图 | 边际图 | 边际图 | marginal | marginal_plot | x,y | 仅 x | **y** | 模板三件套 | **红 fill + related** |
| `parallel_plot` | 平行坐标图 | 平行坐标图 | 平行坐标图 | parallel | parallel_plot | variables | 四轴 | — | 模板三件套 | 红 related |
| `heatmap_plot` | 热图 | 热图 | 热图 | heatmap | heatmap_plot | 无硬必填（多角色均可选） | variables 三列 | — | 模板三件套 | 红 related→correlation / correlogram |
| `correlogram` | Correlogram（相关热图） | Correlogram | 同菜单 | **heatmap** | correlogram | variables | {} | variables | histogram, scatter | 琥珀空 dataset；图标借用须说明「不是热图菜单」 |
| `time_series_plot` | 时间序列图 | 时间序列图 | 时间序列图 | time_series | time_series_plot | time, value | 仅 time | **value** | 模板三件套 | **红 fill + related**→area / smoothing |
| `area_plot` | 区域图 | 区域图 | 区域图 | area | area_plot | **time, value** | **仅 time** | **value** | 模板三件套 | **红 fill**（手册 §2 点名）+ related→time_series_plot |
| `contour_plot` | 等值线图 | 等值线图 | 等值线图 | contour | contour_plot | x,y,z | 仅 x | **y, z** | 模板三件套 | **红 fill + related** |

**模板三件套** = `histogram` + `scatter_plot` + `graph_gallery`。源头：`tools/learning_data/wave4_content.py` `build_data_overlays()` 里 compact `specs` 循环 **写死** `related=["histogram", "scatter_plot", "graph_gallery"]`（约 1542 行）。个别手写课（histogram / probability_plot）已躲开。

**图标借用（菜单名 ≠ 图标种类）必须在「为何此工具」用人话说明，禁止改菜单 id：**

| id | 图标键 | 学员可能看成 | 文案一句 |
|----|--------|----------------|----------|
| bar_chart | pareto | Pareto 图 | 左侧树菜单叫「条形图」；图标可能像 Pareto，本课不是 Pareto 分析 |
| eda_4plot | histogram | 直方图 | 这是四宫格诊断，不是直方图菜单 |
| density / dotplot | histogram | 直方图 | 菜单仍以树名为准 |
| violin | boxplot | 箱线 | 形状分布，不是箱线菜单 |
| mosaic | bar_chart | 条形图 | 马赛克看交叉占比 |
| hexbin / graph_gallery | scatter | 散点 | 分箱密度 / 画廊，不是散点菜单 |
| simplex | mixture_design | 混料设计 | 图是单纯形，设计命令是另一课 |
| correlogram | heatmap | 热图 | 相关格图，菜单名带 Correlogram |
| chi_square_mosaic_link | chi_square | 卡方检验 | 联动出马赛克，统计检验是另一入口 |

### 3.2 建议 related_ids（Agent3 Wave-4，须一句「为何相关」）

| id | 建议 related（示例） |
|----|----------------------|
| histogram | probability_plot, boxplot |
| probability_plot | histogram, normality_test |
| density_plot | histogram, ecdf_plot |
| ecdf_plot | density_plot, histogram |
| boxplot | violin_plot, interval_plot |
| violin_plot | boxplot, density_plot |
| bar_chart | pie_plot, mosaic_plot |
| pie_plot | bar_chart, mosaic_plot |
| area_plot | time_series_plot |
| time_series_plot | area_plot, time_series_smoothing |
| scatter_plot | hexbin_plot, marginal_plot |
| hexbin_plot | scatter_plot, heatmap_plot |
| interval_plot | boxplot, one_way_anova |
| bubble_plot | scatter_plot |
| matrix_plot | scatter_plot, correlation_plot |
| correlation_plot | heatmap_plot, matrix_plot |
| heatmap_plot | correlation_plot, correlogram |
| marginal_plot | scatter_plot, histogram |
| parallel_plot | matrix_plot |
| contour_plot | scatter_plot, rsm_response |
| mosaic_plot | pie_plot, chi_square_mosaic_link |
| eda_4plot | histogram, probability_plot, time_series_plot |
| simplex_design_plot | mixture_design, mixture_analyze |
| graph_gallery | histogram, scatter_plot（允许，但是**画廊课自己**，不要当所有图的默认） |

---

## 4. 练习题反面教材（真实路径 + 字段；≥10）

扫描：184 overlay；`prereq_quiz` 552 条；`self_explain` **357/357 无 hint**；`retrieval` 553 条中电报式 537；`dialog_fill_detail.meaning` 汉字 <8 共 **470** 处；学员可见「同构」**16** 课；非 SPC 先修提到 UCL **5** 课（含 histogram）。

| # | 文件 | 字段 | 现网原文 | 为何不合格 | 合格方向（保留教学意图） |
|---|------|------|----------|------------|--------------------------|
| 1 | `tutorial_overlays/histogram.json` `prereq_quiz[0]` | q/good | 「与概率图共享表？」/ `graph_hist_prob` | 内部 dataset_id 当答案 | 「画直方图和画概率图，能不能用同一张厚度练习表？」好：可以，本课只教直方图。坏：必须再造一张完全不同的宽表 |
| 2 | 同上 `[2]` | q | 「UCL=柱高？」 | 非控制图课硬塞 UCL | 改考「柱高是频数不是规格线」 |
| 3 | 同上 `retrieval_quiz` | 字符串 | `共享族？` / `行48？` / `禁止句` | 不像问题 | 完整问：右侧稍微拉长能否写成已证明正态？ |
| 4 | `two_sample_t.json` `prereq_quiz` | q/good | 「列数？」/`2`；「共享？」/`infer_two_sample_location` | 电报 + 内部 id | 「本软件双样本 t 要选几列独立测量？」选项写成完整句子 |
| 5 | `two_sample_t.json` `retrieval_quiz` | | `两列？` / `同构？` / `禁止句` | 开发口径 | 检索「为什么位置差课不要和方差专项用同一张表？」 |
| 6 | `bar_chart.json` `prereq_quiz[0]` | q/good | 「dataset？」/`graph_bar_category` | 学员看不见 id | 问「导入后左上角工作表显示名是什么」并解释一次 `demo_graph_bar_category` |
| 7 | `one_way_anova.json` | 先修+检索 | 「因子列？」/`腔位`；检索 `ANOVA？`/`腔3`/`禁止句` | 没头没脑 | 问「响应是厚度、分组是腔位时，ANOVA 在比什么」 |
| 8 | `p_chart.json` `prereq_quiz[2]` | good | 「UCL≠USL？」好=「是」 | 过短；控制图课**可以**考 UCL，但选项必须成句 | 「UCL 是过程警戒线，不是客户规格上限」 |
| 9 | `p_chart.json` `retrieval_quiz` | | `行22` / `可变 n 教学点` / `UCL≠USL` | 关键词不是题 | 完整问批 22 台阶、限宽随 n |
| 10 | `special_cause_rules.json` `prereq_quiz[1]` | q/good | 「本课是否新建假 command？」/`否，用 related_ids` | **开发黑话进学员区** | 改问「特殊原因规则要不要再开一个新菜单」；答「不用，用已有控制图课练习」 |
| 11 | `wave4_content.py` compact 循环 | prereq 模板 | 「dataset？」「过程合格？」「菜单？」 | 源头灌水 | 从生成器改掉，禁止只改 sqlite |
| 12 | `histogram.json` `self_explain` | 无 hint | prompt「为何不把 density 挂到同表？」 | 揭晓只能复读题目 | 3–6 句：白名单只让直方与概率同表，密度另课另表，避免埋点打架 |
| 13 | `area_plot.json` `dialog_fill` | | 仅 `time`，value 只在 detail | 导入预填缺必填 `value` | fill 同时写 `time`+`value` |
| 14 | `interval_plot.json` `title` | | 「区间图」vs 菜单「区间散点图」 | 名实不符 | title=区间散点图 |
| 15 | `gage_rr.json` `prereq_quiz[1]` | good | 「不能」 | 过短 | 完整句「不能把本课写成量具通过」 |

**金标对比（不是免责）**：`imr.json` 先修已是完整中文，检索是完整问句；仍缺 `hint`/`why`，且 UI 会套 Nelson 特例与检索 UCL 套话。Agent3 Wave-0 必须过这一课的可读性闸门。

### 4.1 菜单包抽检（每包 ≥3 课）

| 包 | 课 | 电报/内部 id | 揭晓复读 | 非 SPC 考 UCL |
|----|----|--------------|----------|----------------|
| 控制图 | `imr` | 较轻 | UI 仍复读/套 Nelson | 本课应该考 UCL≠USL |
| 控制图 | `p_chart` | 「是」；检索「行22」 | 无 hint | 合法但选项过短 |
| 控制图 | `special_cause_rules` | `related_ids` 当答案 | 同左 | 开发口径 |
| 质量/MSA | `gage_rr` | 「不能」 | 无 hint | 「LSL/USL 是控制限吗」——规格 vs 控制限，可保留但写完整 |
| 质量/MSA | `capability`（同类模板） | 预期与 wave2 `_seven_plus` 同构 | 无 hint | 能力课可谈 USL，不要谈 UCL 当柱高 |
| 质量/MSA | `acceptance_sampling` | 「n 与 c？」类短问 | 检索「禁止句」 | — |
| 推断 | `two_sample_t` | 列数/共享 id | 无 hint | 过程合格？好=禁止 |
| 推断 | `one_way_anova` | dataset_id 当 good | `ANOVA？` | — |
| 图形 | `histogram` / `bar_chart` / `area_plot` | 见上表 | 无 hint | histogram 先修 UCL=柱高 |

---

## 5. 揭晓 UI 现网（只读；Agent3 Wave-0 改）

文件：`src/ui/learning_center_page.cpp`

| 块 | 现网行为 | 手册要求 |
|----|----------|----------|
| 先修 | 点对：`正确。本课期望理解：%1`（把 **good 原文**再念一遍）；点错：复读 bad+good | 显示本课 `why`；缺省再中性 |
| 自解释 | `self_explain_hint`：prompt 含 `Nelson` 则全课 I-MR 话术；否则「对照本题提示『prompt』」= **复读题目** | 读 JSON `hint`；无 Nelson 特例绑定全产品 |
| 检索 | 始终「对照本题『q』…**控制限课请再核对 UCL≠USL**」 | 本课 `hint`；禁止直方图/ANOVA 看到 UCL 套话 |
| glossary 页眉 | 「控制限课请盯住 UCL≠USL」 | 可保留（已限定「控制限课」） |
| 解析器 | `parse_prereq_quiz` 只读 q/good/bad；`parse_self_explain` 只读 after/prompt；检索是 `QStringList` | 增量 why/hint；检索 string **或** `{q,hint}` |

`learning_types.h`：尚无 `hint`/`why`。旧库缺字段必须降级为空，不得崩。

---

## 6. 0–6 过短句（补句，不删节）

金标 `imr` 的 0–6 **以保留为主**。其余课大量 `meaning`：「连续 Y。」「可空=计频。」「横轴。」「面积高度。」——保留字段行，把 meaning 写成「填哪一列、图上是什么」。

`used_for` 模板「用专用集 `graph_*` 练习「某某图」」对学员像内部目录，应改成任务句，**不要删掉专用集事实**（显示名可出现一次）。

学员可见禁止词现网仍大量出现：`command_id` 式 id、`dataset_id`、`同构`、`白名单`、`roles/inputs`、`related_ids`（special_cause_rules）。`WAVE` 字面在 overlay 中为 0（好）。

---

## 7. 给 Agent2 的施工输入

1. Wave 锁表 **原样复用** pedagogy-upgrade-wave-plan §2 的 184 id；本轮 Wave 语义按手册 §6（Wave-0 揭晓+imr 7+，Wave-4 图形名实）。  
2. 从 **生成器模板** 改电报：`wave1_content._seven_plus`、`wave2`/`wave3` 同类、`wave4_content._seven_plus` + compact `related=` 写死。  
3. verify 新断言（建议，Agent4 落地）：  
   - 先修 `q` 汉字 ≥12；`good`/`bad` 汉字 ≥8 且不得匹配 `^[a-z][a-z0-9_]+$`  
   - 每条 self_explain 有非空 `hint`，且 hint 与 prompt 的最长公共子串不能等于整题  
   - 检索若为 object 则 `hint` 非空  
   - 图形 overlay：`menu_label in title`；模板三件套 related 失败  
   - 有 dataset 的课：必填角色 ⊆ dialog_fill 键  
   - 金标 imr：glossary UCL/USL、buried 41/55 仍强制  
4. JSON 不升 v3：现有 TEXT 列内增量。  
5. 图形红项进 Wave-4 队列（fill/title/related/图标说明）；Wave-0 不要假装图形已齐。

---

## 8. Agent1 DoD

- [x] 图形对照表覆盖全部 27 个图形 `command_id`  
- [x] 练习题反面教材 ≥10 条（真实路径+字段）  
- [x] 写清「补充而非推倒」边界  
- [x] **未改**产品代码（本文件 + 一次性扫描脚本将删除，不进入生成器）

**风险一行**：`prereq` 552 条按 §4.2 几乎全员不合格，执行岗必须改生成器源头，否则只润色金标会范围缩水。  

**go/no-go**：**go Agent2**（本岗未改产品代码；红表与电报源头已定位到 `wave4_content.py` 与揭晓 UI）。
