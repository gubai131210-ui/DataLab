# Goal：DataLab 学习中心（全算法/图表说明书 + SQLite 测试数据）

**给下一场 Goal 对话用。不要改本文件的决策门，除非用户当场改口。**

## 已拍板决策（2026-09-03 用户确认）

| 项 | 决定 |
|----|------|
| 覆盖范围 | **全部** `analysis_commands::all()` 命令 + `algorithm_help.json` 全部条目（含 `implemented` / `formula_reference` / `partial` / `graph_reference` / `orchestration`） |
| UI | **新建独立「学习中心」页面**；现有「帮助 → 算法、公式与参考资料」对话框 **原样保留**，只做入口互链 |
| 导入 | 「导入到工作区」**始终新建一张工作表**，不覆盖当前表、不清输出页、不新建工程 |
| 场景 | **混合制造**：电子/光测、注塑、装配、量具、寿命/可靠性各有典型故事 |
| 数据包 | **一份只读 SQLite 打进 qrc**；「导出库」拷到用户选的路径。程序运行时不改官方库 |

## 背景：必须衔接的现有框架

不要另起一套帮助系统。现网已经有：

1. **命令表** — `src/ui/analysis_commands.h/.cpp` 的 `AnalysisCommand`（`id` / `menu_label` / `menu_path` / `roles` / `inputs` / `run`）。菜单由 `MainWindow` 遍历 `analysis_commands::all()` 生成。
2. **公式说明目录** — `resources/help/algorithm_help.json`（约 183 条），加载器 `AlgorithmHelpCatalogLoader`，对话框 `AlgorithmHelpDialog`，资源 `:/help/algorithm_help.json`。
3. **导入工作区先例** — MES `MesToolDialog::import_to_workspace`；主窗口用 `table_` + `navigator_->add_worksheet` + `display_table()`。学习中心导入必须走 **同一 Table 领域模型**，禁止第二套网格。
4. **SQLite 驱动** — 发布包已带 `qsqlite.dll`（`package_dist`）。用 `QSQLITE` + **独立 connection name**，用完 `close` + `QSqlDatabase::removeDatabase`，禁止和工程库、MES 库抢默认连接。
5. **解释边界** — 现有 help 的 `interpretation_limits`：只陈述统计证据，不写「过程合格 / 量具通过 / 必须删点」。学习中心教程必须遵守同一纪律。

**完成标准（整场 Goal）**：小白能从「帮助 → 学习中心」打开任意一条命令，读懂「用来找什么/预测什么」，点一次导入得到带场景列名的表，按逐步按钮说明跑出结果，并看懂每张输出表/图在说什么。`formula_reference` 条目必须标明「本程序尚未提供该菜单或仅公式参考」，仍给场景与「若已实现应点哪里」。

---

## 五 Agent 流水线（一场 Goal 内顺序执行，可并行的只并行调研）

全程一个会话编排。每个 Agent 有 **完成标准**；未达标不得进入下一岗。

### Agent A — 调研（网络 + 代码，先于写文案）

**做什么**

- 对 **每一条** command_id / help id 检索权威来源：ISO/ASQ/AIAG、NIST、Minitab Help、Montgomery/Wheeler 等，记录：常用场景、能回答的问题、不能回答的问题、典型样本量、常见误用。
- 对照 `analysis_commands.cpp` 的真实 `roles`/`inputs`/`menu_path`，禁止写菜单里不存在的按钮。
- 对照 `algorithm_help.json` 的 `purpose`/`output_description`，教程不得与公式说明打架。
- 产出 `docs/research/learning-center-research-notes.md`：**每条命令一小节**，含来源 URL + accessed 日期 + 选用的制造场景。

**完成标准**

- 命令清单 = `analysis_commands::all()` 的 id 全集 ∪ help json 的 id 全集。缺一条即失败。
- 每条至少 1 个权威来源；图表类至少说明「看什么模式 / 不替代假设检验」。

### Agent B — 计划落地（本文件的执行切片，不重写决策）

**做什么**

- 按菜单切包：`统计` / `控制图` / `质量工具` / `图形` / `DOE` / 其他。
- 决定 **哪些命令共享同一 `dataset_id`**（例如同一「SMT 锡膏厚度」表服务描述统计、正态性、I-MR、能力指数）。禁止为每条命令复制一份几乎相同的大表。
- 写出 SQLite 建库脚本路径与 CMake 嵌入步骤。

**完成标准**

- 有「command_id → dataset_id → 列角色映射」总表，覆盖全集。
- 共享数据集在注释里写清服务哪些命令。

### Agent C — 执行（框架一次做对，内容按包填满）

顺序固定：

1. SQLite schema + 生成脚本 + 嵌入 qrc  
2. 领域服务：只读打开、按 id 取教程、物化为 `datalab::domain` 表  
3. `LearningCenterPage`（独立窗口/页，**不是**往 `AlgorithmHelpDialog` 里堆控件）  
4. `MainWindow`：帮助菜单加「学习中心」；信号 `importDemoRequested` → **新建工作表**  
5. 按包写入教程与数据，直到清单清零  

**完成标准**

- 新页面可搜索、按菜单树浏览。
- 每条 **implemented** 命令：场景说明 + 逐步点击 + 输出解读 + 可导入数据（或明确「本命令不需要数据」，如部分计算器/设计生成）。
- 每条 **非 implemented**：场景 + 限制说明 + 可选示意数据（导入后提示菜单不可用）。
- 不改 `algorithm_help.json` 的公式语义；最多加可选字段或旁路 JSON，避免 1.8 万行单文件再膨胀到不可维护。

### Agent D — 测试

**做什么**

- 单元：catalog 与命令 id 对齐；SQLite 能列出全部 dataset；导入列数/列名/行数符合 schema。
- 连接名：打开/关闭库后 `QSqlDatabase::connectionNames()` 不残留学习中心连接。
- 导入：当前表有数据时再导入，**旧表仍在导航器中**，新表被选中。
- 抽样手工脚本：每个菜单至少 1 条「导入 → 按步骤跑」的断言（能跑到 `AnalysisService` 的用现有 test 夹具）。

**完成标准**

- 新 test target 进 `CMakeLists.txt`，本机 `build-mingw` 能编过。
- 对齐测试：help id / command id / tutorial id / dataset 引用无悬空外键。

### Agent E — 收尾检查

**做什么**

- 对照本文「禁止偷懒」。
- 查泄漏：QSql 连接、QFile、页面 `WA_DeleteOnClose`、大表不常驻内存（按需读行）。
- 文案：小白可读；黑带级「用途/误用」在；不承诺过程合格。
- 用户本机 `package_dist`（中文路径由用户跑）。Git commit + push（用户规则）。

**完成标准**

- 检查清单全部打勾；缺口写进 PR/commit 说明，不假装做完。

---

## 架构（保持平衡，禁止第二套数据内核）

```
帮助菜单
 ├─ 算法、公式与参考资料     → 现有 AlgorithmHelpDialog（公式/假设）
 ├─ 公式注册表
 └─ 学习中心（新）           → LearningCenterPage
                                 ├ 左：搜索 + 与命令表同步的树
                                 ├ 右：教程（用途 / 场景 / 步骤 / 输出解读）
                                 └ 底：导入测试数据 | 打开公式说明 | 导出 SQLite
                                          │
                                          ▼
                                 LearningDatasetStore
                                   打开 :/help/learning_center.sqlite（拷到临时只读或 QFile 映射）
                                   load_dataset(dataset_id) → domain Table
                                          │
                                          ▼
                                 MainWindow::import_learning_dataset()
                                   克隆为新 worksheet 名称 = demo_<短名>
                                   navigator_->add_worksheet
                                   切换当前表，不清 output pages
```

### UI 约束（用户规则：不要堆在一页）

- **学习中心 = 独立窗口**（`QMainWindow` 或大 `QDialog` + 自身布局），不要塞进现有帮助对话框。
- 右侧教程用分节：① 一句话用途 ② 现场故事 ③ 逐步操作（编号，菜单路径必须与 `menu_path`+`menu_label` 一致）④ 输出表/图逐项解读 ⑤ 常见误用 ⑥ 相关命令。
- 「打开公式说明」只 `select_entry(id)` 现有对话框，不复制公式 HTML。
- 禁止在学习中心再做一遍分析设置对话框。

### 内存 / 资源

- SQLite：每次操作 `addDatabase("QSQLITE", uniqueName)`，读完关闭并 `removeDatabase`。
- qrc 内 sqlite：先 `QFile::copy` 到 `QTemporaryFile`（只读打开），或 `QFile` 读到内存仅当库 < 约定上限（建议生成后 < 20MB）。超过则按表分页读。
- 演示表建议每表 **30–200 行**（控制图可到 50 子组），禁止万行演示。
- `LearningCenterPage` 关闭即销毁；不缓存全部 dataset 的 `QVector`。
- 导入只拷当前 dataset 到 `table_` 的工作表列表；不要把整个 sqlite 载入 RAM。

---

## SQLite schema（单库，方便查询与导出）

文件：`resources/help/learning_center.sqlite`  
生成：`tools/build_learning_center_db.py`（用户允许用 Python 生成）  
版本表必须有，启动时校验。

```sql
CREATE TABLE meta (
  key TEXT PRIMARY KEY,
  value TEXT NOT NULL
);
-- catalog_version, generated_at, source_git

CREATE TABLE datasets (
  dataset_id TEXT PRIMARY KEY,
  title TEXT NOT NULL,
  industry TEXT NOT NULL,          -- electronics | molding | assembly | msa | reliability | mixed
  story TEXT NOT NULL,             -- 现场故事，中文
  row_count INTEGER NOT NULL,
  notes TEXT
);

CREATE TABLE dataset_columns (
  dataset_id TEXT NOT NULL,
  column_index INTEGER NOT NULL,   -- 0-based，与导入后列序一致
  name TEXT NOT NULL,
  role_hint TEXT NOT NULL,         -- measurement / factor / by / time / defect / ...
  unit TEXT,
  description TEXT,
  PRIMARY KEY (dataset_id, column_index)
);

CREATE TABLE dataset_cells (
  dataset_id TEXT NOT NULL,
  row_index INTEGER NOT NULL,
  column_index INTEGER NOT NULL,
  value TEXT NOT NULL,             -- 与工作表一样存文本，解析走现有列类型推断
  PRIMARY KEY (dataset_id, row_index, column_index)
);

CREATE TABLE tutorials (
  command_id TEXT PRIMARY KEY,     -- 对齐 AnalysisCommand.id 或 help id
  title TEXT NOT NULL,
  category TEXT NOT NULL,
  menu_path TEXT NOT NULL,
  implemented_status TEXT NOT NULL,
  used_for TEXT NOT NULL,          -- 常用来做什么（预测/找关联/监控/比较…）
  not_for TEXT NOT NULL,           -- 不能当什么用
  scenario TEXT NOT NULL,
  dataset_id TEXT,                 -- 可空：无需数据的命令
  click_steps TEXT NOT NULL,       -- JSON 数组：["帮助 → 学习中心 → …", "统计 → …"]
  dialog_fill TEXT NOT NULL,       -- JSON：角色名 → 列名；选项名 → 值
  output_guide TEXT NOT NULL,      -- JSON 数组：{name, meaning}
  common_mistakes TEXT NOT NULL,
  related_ids TEXT NOT NULL,       -- JSON 数组
  research_sources TEXT NOT NULL,  -- JSON：{label,url,accessed}
  FOREIGN KEY (dataset_id) REFERENCES datasets(dataset_id)
);

CREATE INDEX idx_cells_dataset ON dataset_cells(dataset_id);
CREATE INDEX idx_tutorials_category ON tutorials(category);
```

查询示例（说明书里可写给高级用户）：

```sql
SELECT command_id, title, dataset_id FROM tutorials WHERE category = '控制图';
SELECT name, role_hint FROM dataset_columns WHERE dataset_id = 'smt_paste_height';
```

---

## 教程内容模板（每条必须填满，禁止只写 purpose 一句）

1. **用来做什么**（黑带口吻，但句子短）：监控、比较、找关联、预测、筛选因子、评估量具……选且写清。
2. **现场故事**：混合制造，列名用业务名（`锡膏高度_um`、`模腔`、`操作员`、`循环次数`），不要 `x1,x2`。
3. **逐步点击**：从当前主窗口出发，菜单文字与代码一致。对话框里「把哪一列拖到哪个角色、填哪个数」必须对照 `RoleSpec`/`InputSpec`。
4. **输出解读**：对该命令 `AnalysisService` 实际产出的 **每一张表、每一个常用图** 写「它说明了什么 / 什么时候不要过度解释」。
5. **误用**：例如把控制图当能力证明、把相关当因果、把 p>α 当「已证明正态」。

`formula_reference`：第 3 步写成「当前版本菜单中可能没有此项；公式见帮助对话框。导入数据仅供对照学习。」

---

## 数据集复用（强制，防止库膨胀）

最少共享包（Agent B 可增，不可无故一对一复制）：

| dataset_id | 场景 | 至少服务的命令 |
|------------|------|----------------|
| smt_paste_height | SMT 锡膏高度 | descriptive, normality_test, outlier_test, i_mr / individuals, capability, histogram, boxplot, time_series |
| two_line_thickness | 两产线膜厚 | two_sample_t, variance_test, boxplot, histogram |
| paired_rework | 返工前后 | paired_t, sign_test, wilcoxon |
| anova_cavity | 三模腔尺寸 | one_way_anova, anom, kruskal_wallis, boxplot |
| corr_temp_offset | 温度 vs 偏移 | correlation, regression, scatter |
| attribute_defect | 班次不良计数 | pareto, chi_square, u/c/p/np 图, two_proportions |
| gage_rr_balance | 量具 R&R | gage 系列 |
| doe_factorial_demo | 析因/Taguchi/混料（可拆几个小表） | DOE 生成与分析 |
| reliability_cycles | 寿命循环 | KM / life regression / probit |
| ts_weekly_yield | 周良率 | 平滑、分解、趋势、ARIMA、季节、ADF |

没有表的命令（分布计算器、部分设计生成）：`dataset_id` 空，步骤写「无需导入，直接打开菜单」。

---

## 代码落点（预期，执行时可微调路径但不可换领域模型）

| 新增 | 职责 |
|------|------|
| `src/application/learning/learning_dataset_store.h/.cpp` | 打开 qrc sqlite、校验 meta、按 id 物化 Table |
| `src/application/learning/learning_tutorial_catalog.h/.cpp` | 读 tutorials 行 → 结构体 |
| `src/ui/learning_center_page.h/.cpp` | 独立学习中心 UI |
| `resources/help/learning_center.sqlite` | 只读库 |
| `resources/help/learning_center.qrc` 或并入 `algorithm_help_resources` | 嵌入 |
| `tools/build_learning_center_db.py` | 从 `tools/learning_data/*.csv` + `tutorials.json` 生成 sqlite |
| `tests/learning_center_store_test.cpp` | 连接清理、对齐、导入形状 |

`MainWindow`：加菜单与 `import_learning_dataset(dataset_id)`。先查项目里 **多工作表** 是否已存在；若当前只有单 `table_`，按现有 navigator 语义实现「新表」——**若内核仍是单表，必须先加最小多表或「另存当前再替换并保留撤销提示」并在 Agent B 写明**。执行前 Agent C 必须读 `MainWindow`/`ProjectNavigator`：若只有一张 `table_`，允许的最小接法是：

- **优先**：若工程已支持多 worksheet，挂新表。
- **否则**：导入前把当前表推进 undo 或自动改名备份工作表结构；**禁止静默丢数据**。与用户已选「新建工作表」冲突时，以「不丢当前数据」为更高优先级，可在导入前把旧表导出到输出页或第二份 `Table` 成员。发现单表限制时 **先补最小多表缝**，不要假装已经多表。

---

## 禁止偷懒

- **禁止**只改帮助对话框加一段 HTML，却不建学习中心页面。
- **禁止**只覆盖描述统计/控制图等「好看」的十几条，清单未清零就收工。
- **禁止**教程步骤写「点分析」而不写真实菜单路径与角色名。
- **禁止**测试数据全是 `x,y` 或标准正态乱数、没有现场故事。
- **禁止**每条命令复制一份互不相干的大表（必须共享 dataset）。
- **禁止**导入覆盖当前工作表且不备份。
- **禁止**把 sqlite 用默认连接名打开后不 `removeDatabase`。
- **禁止**把整库 load 进单个 `QVector` 常驻。
- **禁止**改公式说明语义或删 `interpretation_limits`。
- **禁止**输出解读写成「过程合格 / 必须停线 / 已证明正态」。
- **禁止**为未实现命令伪造可运行菜单步骤而不加限制说明。
- **禁止**跳过命令 id 对齐测试。
- **禁止**在 Plan/本文件里改产品代码。
- **禁止**用浏览器代替用户本机 `package_dist` 验收（中文路径）。
- **禁止**把学习中心做成第二个分析向导（不重复 `AnalysisSetupDialog`）。

---

## Goal 对话开场应粘贴的指令（给编排 Agent）

```
按 docs/research/goal-learning-center-black-belt-plan.md 执行。
决策已拍板，不要重问范围/页面/导入/行业/sqlite。
顺序：Agent A 调研清单清零 → B 映射表 → C 框架+全量内容 → D 测试 → E 对照禁止偷懒。
完成后 git commit + push；提示我本机 cmake --build build-mingw --target package_dist。
不要编辑该 plan 文件的决策表。
```

---

## 验收（用户本机）

1. 帮助 → 学习中心：树与菜单分类一致，搜索「回归」「控制图」能命中。
2. 任选 implemented 命令：导入后导航器多一张 `demo_*` 表，旧表还在。
3. 按步骤点菜单能跑通，输出页能对上「输出解读」。
4. formula_reference 条目有「未实现/仅公式」提示，不崩溃。
5. 「导出 SQLite」得到可 `sqlite3` 查询的文件。
6. 关学习中心再开，无连接泄漏、无明显变慢。
