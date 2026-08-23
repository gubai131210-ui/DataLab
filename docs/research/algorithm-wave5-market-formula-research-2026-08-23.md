# 算法 Wave-5：市场对照、公式来源与开源启发（2026-08-23）

> 研究日期 / 访问日期：2026-08-23（UTC+8）  
> 用途：给下一会话 `/goal` **算法扩展 Wave-5** 的调研正文；配套计划见  
> [`goal-wave-2026-08-23-algorithm-wave5-plan-and-mega-prompt.md`](goal-wave-2026-08-23-algorithm-wave5-plan-and-mega-prompt.md)  
> 权威状态：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md) · 候选池 [`next-wave-algorithms-charts-ml-oss.md`](next-wave-algorithms-charts-ml-oss.md)  
> 执行框架：[`goal-execution-framework.md`](goal-execution-framework.md)

---

## §0 水位（勿重做）

| 批次 | 状态 |
|------|------|
| §12 P0–P2 主项 | ✅ / ⚪（见 backlog） |
| 算法 Wave-2～4 | ✅（nominal/logistic/ALT/Cox/nonparametric capability 等） |
| G1/G2/G6、Menu IA | ✅ |
| G3 Graph Builder | 计划已备，**本 Goal 不做**（产品 UI Track） |

**本 Wave 目标：** 在产品范围内继续 **新增 + 深化**，一次锁定 **4 项完整竖切**；保持导入 A→B / complete-case / `source_row`；`formula_reference ≠ golden`。

---

## §1 Primary Sources（网上调研 · 2026-08-23）

### 1.1 Minitab 市场与优势（可学、不克隆）

| 主题 | URL | 采纳到 DataLab |
|------|-----|----------------|
| Feature List 全模块 | https://www.minitab.com/en-us/products/minitab/features/ | 对照缺口；本 Wave 只取汽车质量相关子集 |
| What's New / Nonparametric Capability 等 | https://www.minitab.com/en-us/products/minitab/whats-new/ | 能力族已有窄化；Automated Capability 仍 ⏸ |
| Interactive Control Charts / Quick Designs | https://www.minitab.com/en-us/support/minitab/minitab-software-updates/ | 学交互与 DOE 引导叙事；不做云 Solution Center |
| Reliability Module 能力叙述 | 同上 Feature List Reliability | Weibayes / 寿命 / 保修为质量主路径候选 |
| RF / CART Methods（表形） | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/predictive-analytics/how-to/random-forests-regression/methods-and-formulas/methods/ | **表形与披露**；禁止宣称 TreeNet/RF 数值对齐 |

**Minitab 仍领先、本仓库可学的「非算法」点（登记，本 Wave 可选不写代码）：**

1. 分析后「预测 / 换模型」工具栏（远景）  
2. 假设自检 + Report Card（→ G4）  
3. DOE Quick Designs 引导（本 Wave Taguchi **窄化设计生成** 可借鉴「少参数出矩阵」）  
4. Interactive 控制图导航（→ 图表交互，非本 Wave）  

### 1.2 公式与方法学（必须写进各 `p5_*.md`）

| 主题 | URL | 用途 |
|------|-----|------|
| NIST PMC 过程监控 | https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm | 控制图/能力语境 |
| NIST 能力指数 | https://www.itl.nist.gov/div898/handbook/pmc/section1/pmc16.htm | Cp/Cpk 定义（深化时对照） |
| NIST APR 可靠性 | https://www.itl.nist.gov/div898/handbook/apr/apr.htm | Weibull / 试验 / 寿命 |
| NIST Weibull 专节 | https://www.itl.nist.gov/div898/handbook/apr/section4/apr4.htm | Weibayes 先验/少失效语境 |
| NIST 分布与检验总目录 | https://www.itl.nist.gov/div898/handbook/dtoc.htm | PDF/CDF 工具 |
| sklearn RandomForest（接口参考） | https://scikit-learn.org/stable/modules/generated/sklearn.ensemble.RandomForestClassifier.html | 仅公式/接口；**禁止**进 dist |

### 1.3 开源启发（能力灵感 · 非抄库）

| 项目 | URL | 可学 | 禁止 |
|------|-----|------|------|
| qcc | https://github.com/luca-scr/qcc | SPC/能力输出结构 | golden 数值 |
| u-analytics | https://github.com/iyulab/u-analytics | 工业质量算法清单对照 | 换栈 |
| Cassini | https://github.com/saturnis-io/cassini | Show Your Work / MSA 叙事 | AGPL 整站并入 |
| Tinker DOE | https://github.com/M-Elsaied/Tinker | 正交表/设计矩阵 UX | Web 栈 |
| JASP | https://github.com/jasp-stats/jasp-desktop | 结果区模块化 | 嵌 R |

---

## §2 Wave-5 锁定项（4 项 · 全部完成才 complete）

> Planner 可在「id 冲突 / 领域过大」时用括号内 **备选** 替换，但 **项数不得少于 4**，且须回写 backlog。

| # | 命令 id（建议） | 类型 | 窄化范围 | Primary 公式/方法入口 |
|---|-----------------|------|----------|------------------------|
| **W5-1** | `random_forest` | 新增 | 固定 `n_trees`、bootstrap、分类/回归择一默认；OOB 可选；变量重要性表；**披露非 Minitab RF/TreeNet 对齐** | Minitab RF Methods + sklearn RF |
| **W5-2** | `weibayes` | 新增 | 少失效/无失效 Weibull 形状先验 + 特征寿命点估计/区间窄化；右删失主路径；不做全量寿命回归全家桶 | NIST APR Weibull + Minitab Weibayes 帮助表形 |
| **W5-3** | `taguchi_orthogonal_design` | 新增 | **设计生成** L8/L9/L12 子集之一起步；输出设计矩阵工作表；不做全 Taguchi 分析套件 | Minitab Taguchi DOE Feature + NIST DOE 章节 |
| **W5-4** | `distribution_calculator` | 新增工具 | 正态/t/χ²/F/Weibull 子集：PDF、CDF、分位数；单页工具对话框或命令；**不**改既有 GOF 数值路径 | NIST 分布章 + Feature List Simulations |

**备选（仅当上表某项不可行）：**

| 备选 id | 条件 |
|---------|------|
| `cluster_variables` | 替代 W5-3 若 Taguchi 正交表实现风险过高 |
| `parallel_plot` | 替代 W5-4 若分布计算器与现有 help 冲突过大 |
| `mixture_design` 窄化 | 替代 W5-3（单纯形格点极窄） |

### 明确不做（本 Wave）

- TreeNet® / AutoML / 嵌 Python·R  
- GLM Mixed / MANOVA 全量  
- Automated Capability 向导  
- Graph Builder（G3）/ Report Card（G4）/ AnalysisService 大拆（G5）  
- Minitab golden 数值、`VALIDATION_MATRIX` 假对齐  
- 破坏导入 complete-case / `source_row` / A→B  

---

## §3 竖切与导入衔接（硬约束）

每项必须：

```
docs/research/p5_<id>.md（Primary URL + 访问日期 + 表形清单）
  → src/domain/statistics/*（纯 C++）
  → *Facts + serialization
  → AnalysisService::*
  → analysis_commands（menu_path/menu_group 声明式）
  → interpretation（只读 Facts；禁用「已证明合格/失控」越权句）
  → help / formula_reference 测试（# source: formula_reference）
  → wiring-index + backlog 行状态 + acceptance §2
```

**导入衔接检查（Tester 必测或回归）：**

1. complete-case：缺失不静默当 0  
2. `source_row` 稳定  
3. A→B 换文件后旧排除/隐藏不串数据  
4. Taguchi 设计写入工作表后可被后续分析读入（与现有 DOE 设计导出路径一致）  

---

## §4 建议后续（本 Wave 之后）

| 下一刀 | 内容 |
|--------|------|
| Wave-6 | `parallel_plot` / `cluster_variables` / 保修深化 / ALT 再深 |
| 产品 | G3 Graph Builder（计划已备） |
| 架构 | G5 AnalysisService 拆分（与算法 Goal 分会话） |

---

**文档状态：** 2026-08-23 首版；供 Wave-5 `/goal` 直接引用。
