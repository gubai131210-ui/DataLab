# P0：Runs / Fisher 2×2 / 独立 Run Chart / 鱼骨图

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。禁止用 Minitab 示例页数字当 golden。  
> 队列项：Runs test、Fisher exact 表形深化、独立 Run Chart、Cause-and-effect（鱼骨）。**禁止重做**已有 `pareto`；**禁止拆坏** `two_proportions`。

---

## A. Runs Test（Wald–Wolfowitz / Minitab Runs Test）

### A.1 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 方法 | [Runs Test — Methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/runs-test/methods-and-formulas/methods-and-formulas/) | 2026-08-21 |
| Minitab 比较准则 | [Specify the comparison criterion](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/runs-test/perform-the-analysis/specify-the-comparison-criterion/) | 2026-08-21 |
| Minitab 数据注意 / 示例表形 | [Data considerations](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/runs-test/before-you-start/data-considerations/)；[Example](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/runs-test/before-you-start/example/) | 2026-08-21 |
| NIST 正态近似公式 | [NIST e-Handbook 1.3.5.13 Runs Test](https://www.itl.nist.gov/div898/handbook/eda/section3/eda35d.htm) | 2026-08-21 |

### A.2 产品选型

- **新命令** `runs_test`（Stat > Nonparametrics）
- 输入：一列数值，**时间/行序保留**；列中间缺失 → 诊断、不出检验（对齐 Minitab：中间 `*` 不可跨过）
- 比较准则 `K`：
  - 默认 **`mean`**（Minitab 默认）
  - 可选 `median` 或用户常数 `value`
- 符号编码：**A** = \(X_i > K\)；**B** = \(X_i \le K\)（与 Minitab 记号一致；等号归入 B）
- 主检验：正态近似双侧 P；若 \(\min(A,B)<10\) → 仍可算 Z/P，但加诊断 `runs_normal_approx_weak`（Minitab：约 10–11 两侧才“quite good”）
- Facts：独立 `RunsTestFacts`（N、K、A、B、observed_runs、expected、z、p、criterion）

**明确不做：** 精确小样本表查表；连续性校正；把 Run Chart 四模式检验并入本命令；假 Minitab golden（示例 Observed=17 / Expected=16.77 / P=0.930 仅作公式核对，不作断言测试）。

### A.3 公式（锁定）

有限观测按行序 \(x_1,\ldots,x_N\)。令 \(A=\#\{x_i>K\}\)，\(B=\#\{x_i\le K\}\)，\(N=A+B\)（\(N\ge 2\)，且 \(A\ge 1\) 且 \(B\ge 1\)；否则诊断）。

1. **Observed runs \(R\)**：连续同侧（相对 K）的段数。从 \(i=2\) 起，若符号相对 K 的侧别改变则 +1；初值 1。
2. **期望与方差**（NIST / Minitab 记号一致）：

\[
\mathrm{Expected}=\frac{2AB}{N}+1
\]

\[
\mathrm{Variance}=\frac{2AB\,(2AB-N)}{N^{2}(N-1)}
\]

3. **Z 与双侧 P**（**无**连续性校正；与 Minitab 示例 Expected≈16.77 对齐）：

\[
Z=\frac{R-\mathrm{Expected}}{\sqrt{\mathrm{Variance}}},\qquad
p=2\bigl(1-\Phi(|Z|)\bigr)
\]

H0：顺序随机；H1：非随机。判定相对产品 `alpha`（默认 0.05）：`reject` ⇔ \(p<\alpha\)。

### A.4 表形（对齐 Minitab 可读性）

| 表 | 列 / 合同 |
|---|---|
| 描述统计 | N、K、≤K（=B）、>K（=A） |
| 假设 | H0：顺序随机；H1：非随机 |
| 游程检验 | Observed、Expected、Z、P-Value |
| 诊断 | 中间缺失、全同侧、N 过小、正态近似弱 |

### A.5 测试

手算小序列 \(R\)；等号归 B；mean/median/常数 K；中间缺失诊断；`# source: formula_reference`。禁止断言 Minitab 示例 P=0.930。

---

## B. Fisher exact 2×2 表形深化

### B.1 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 2×2 方法（Fisher） | [Test for 2×2 tables — methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/how-to/cross-tabulation-and-chi-square/methods-and-formulas/test-for-2x2-tables/) | 2026-08-21 |
| Minitab 概念 / 入口 | [What is Fisher's exact test?](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/supporting-topics/other-statistics-and-tests/what-is-fisher-s-exact-test/) | 2026-08-21 |
| Minitab Other Stats 勾选 | [Select other tests and statistics](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/how-to/cross-tabulation-and-chi-square/perform-the-analysis/select-the-other-tests-and-statistics/) | 2026-08-21 |
| 教材 / 综述（双侧定义） | Agresti, *Categorical Data Analysis*；Agresti (1992) *Statist. Sci.* 7:131–153（exact contingency survey） | 2026-08-21 |

> DataLab 已有 `fisher_two_sided`（`inference_extensions.cpp`），由 `two_proportions` 在 H0: 差=0 时调用。本轮是 **表形/命令深化**，复用同一核，不改两比例路径行为。

### B.2 产品选型

- **新命令** `fisher_exact`（Stat > Tables；对标 Cross Tabulation > Other Stats > Fisher）
- 输入（二选一，配置互斥）：
  1. **两列分类**：complete-case 交叉计数；恰好 2×2 水平，否则诊断
  2. **四格计数** \(a,b,c,d\)（非负整数）
- 双侧精确 P：**复用**现有 `fisher_two_sided(a, a+b, c, c+d)` 映射（行1 事件=a、试验=a+b；行2 事件=c、试验=c+d）
- 可选附带：Pearson χ² / 期望频数（只读辅助，默认开）；**不**默认改 `chi_square` 命令契约
- Facts：`FisherExactFacts`（a,b,c,d、row/col 标签、p、optional odds_ratio 点估计 \(ad/bc\) 当 \(bc>0\)）
- `two_proportions`：**零行为 diff**（仍可输出 Fisher 列；不得因本命令改其 Wald/CI/诊断）

**明确不做：** 拆坏或重写 `two_proportions`；rxc (r>2 或 c>2) FEXACT/Mehta–Patel；单侧默认；Blaker / mid-p；假 Minitab golden（Help 示例 P≈0.263、cookie≈0.0054775 仅公式核对）。

### B.3 公式（锁定）

2×2 表（固定边际）：

|  | 列1 | 列2 | 行合计 |
|---|---|---|---|
| 行1 | \(a\) | \(b\) | \(a+b\) |
| 行2 | \(c\) | \(d\) | \(c+d\) |
| 列合计 | \(a+c\) | \(b+d\) | \(N\) |

条件分布：\(a \mid\) 边际 \(\sim\) Hypergeometric。单表概率：

\[
P(a)=\frac{\binom{a+b}{a}\binom{c+d}{c}}{\binom{N}{a+c}}
\]

**双侧 P（Minitab / R `fisher.test` / 现有实现）：** 在所有同边际可行格 \(a'\in[a_{\min},a_{\max}]\) 上，

\[
p=\sum_{\{a':\,P(a')\le P(a)\}} P(a')
\]

（实现用 log-组合累加 + 相对容差，与现网 `fisher_two_sided` 一致。）

H0：行×列独立（OR=1）。判定：`reject` ⇔ \(p<\alpha\)。

### B.4 表形

| 表 | 列 / 合同 |
|---|---|
| 交叉表 | 带行列标签的 2×2 计数 + 边际合计 |
| Fisher 精确检验 | 方法=`Fisher exact`、P-Value；可选 OR 点估计 |
| （可选）卡方辅助 | Pearson χ²、DF、P、期望频数 |
| 诊断 | 非 2×2、负计数、空表 |

### B.5 测试

手算极小表（如 \([[1,0],[0,1]]\)）P；与 `fisher_two_sided` 同输入一致；两分类列交叉与手填四格一致；**回归** `two_proportions`；`# source: formula_reference`。

---

## C. 独立 Run Chart（Quality Tools > Run Chart）

### C.1 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 方法（含四模式 P） | [Run Chart — Methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/run-chart/methods-and-formulas/methods-and-formulas/) | 2026-08-21 |
| 随机性检验叙述 | [Tests for randomness in a run chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/supporting-topics/test-for-randomness/) | 2026-08-21 |
| 统计量定义 / 表形 | [All statistics and graphs](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/run-chart/interpret-the-results/all-statistics-and-graphs/)；[Key results](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/run-chart/interpret-the-results/key-results/) | 2026-08-21 |
| 假设 / 数据 | [Hypotheses](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/run-chart/before-you-start/hypotheses/)；[Data considerations](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/run-chart/before-you-start/data-considerations/) | 2026-08-21 |

> Minitab Help 的 MathML 在抓取时常被剥离；下列期望/方差/Z 与 Help 记号（R,m,n,N,V）及示例一致性（E≈6.5、V 的 E=7、cluster/mixture 互补、trend/oscillation 在 Z=0 时各 0.5）对齐，属 **formula_reference**，非 golden。

### C.2 产品选型

- **新命令** `run_chart`（Stat > Quality Tools）；**独立于** Gage/Type1 内嵌 run chart
- P0 输入：**单列个体观测**（subgroup size = 1）；行序 = 时间序；跳过缺失（两端可，中间缺失打断 → 诊断或仅用完整前缀，产品锁定：**中间缺失 → 诊断、不出检验图统计**）
- 中心线：**全体中位数**（个体时与 Minitab 一致）
- 图：`PlotKind::control`（或现有 run 序列图）；中心=中位数；点带 `source_rows`；**不跑** SPC Test 1–8
- 输出四模式近似 P：clustering / mixtures / trends / oscillation
- Facts：`RunChartFacts`（N、median、runs_about、expected_about、longest_about、runs_updown、expected_updown、longest_updown、四 P）

**明确不做：** 本轮 subgroup>1 的组均值/组中位数路径（可后续加深）；控制限；与 `runs_test` 命令合并；假 Minitab 示例 P golden。

### C.3 公式（锁定）

设有序点 \(y_1,\ldots,y_N\)（\(N\ge 3\)），中心线 \(M=\mathrm{median}(y)\)。

#### C.3.1 关于中位数的游程（clustering / mixtures）

- 侧别：\(y_i > M\) 为上；\(y_i \le M\) 为下（**落在中心线上的点归下侧**，对齐 Minitab）
- \(m=\#\{y_i>M\}\)，\(n=\#\{y_i\le M\}\)，\(N=m+n\)
- Observed \(R\)：同侧连续段数（连线跨过中心线则结束一段）
- Longest about：最长同侧段点数

\[
E_{\mathrm{about}}=1+\frac{2mn}{N}
\]

\[
\mathrm{Var}_{\mathrm{about}}=\frac{2mn\,(2mn-N)}{N^{2}(N-1)}
\]

\[
Z_{\mathrm{about}}=\frac{R-E_{\mathrm{about}}}{\sqrt{\mathrm{Var}_{\mathrm{about}}}}
\]

（**无**连续性校正。）

| 模式 | 含义（相对期望） | P（锁定） |
|---|---|---|
| Clustering | 游程偏少 | \(p=\Phi(Z_{\mathrm{about}})\) |
| Mixtures | 游程偏多 | \(p=1-\Phi(Z_{\mathrm{about}})\) |

若 \(m=0\) 或 \(n=0\) 或 \(\mathrm{Var}=0\) → 诊断、该对 P 不出。

#### C.3.2 上升/下降游程（trends / oscillation）

- 方向：对相邻差分；**严格上升**开/续上行；**严格下降或相等（flat）**计入下行（Minitab：flat 属 downward）
- Observed \(V\)：方向改变时结束一段；总上行+下行段数
- Longest up/down：最长段的**点数**（按 Minitab 解释页“points in the longest run”计数）

\[
E_{\mathrm{ud}}=\frac{2N-1}{3},\qquad
\mathrm{Var}_{\mathrm{ud}}=\frac{16N-29}{90}
\]

\[
Z_{\mathrm{ud}}=\frac{V-E_{\mathrm{ud}}}{\sqrt{\mathrm{Var}_{\mathrm{ud}}}}
\]

| 模式 | 含义 | P（锁定） |
|---|---|---|
| Trends | 游程偏少 | \(p=\Phi(Z_{\mathrm{ud}})\) |
| Oscillation | 游程偏多 | \(p=1-\Phi(Z_{\mathrm{ud}})\) |

H0（两组检验共用叙述）：序列随机。解释禁止「已证明过程受控/失控」。

### C.4 表形

| 表 | 列 / 合同 |
|---|---|
| 关于中位数的游程 | Number of runs、Expected、Longest run、P clustering、P mixtures |
| 上升/下降游程 | Number of runs、Expected、Longest run、P trends、P oscillation |
| 图 | 时序点 + 中位数中心线 |

### C.5 测试

构造明显聚集/交替序列看 cluster vs mixture 方向；单调序列 trend P 小；全相等诊断；与 Gage 内嵌图命令隔离；`# source: formula_reference`。

---

## D. Cause-and-Effect / 鱼骨图（Ishikawa）

### D.1 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 概述 | [Overview for Cause-and-Effect](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/cause-and-effect-diagram/before-you-start/overview/) | 2026-08-21 |
| 分支标签 / 原因录入 | [Enter and label the causes](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/cause-and-effect-diagram/perform-the-analysis/enter-and-label-the-causes/) | 2026-08-21 |
| 子分支 | [Enter the sub-causes](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/cause-and-effect-diagram/perform-the-analysis/enter-the-sub-causes/)；[Example with sub-branches](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/cause-and-effect-diagram/before-you-start/example-with-sub-branches/) | 2026-08-21 |
| 与 Pareto 分工 | 同上 Overview：「identify which causes are most common → use Pareto」 | 2026-08-21 |

**这不是统计检验**——无 P 值、无分布假设；产品合同是结构化图 + 可序列化模型。

### D.2 产品合同（锁定）

- **新命令** `cause_and_effect` / `fishbone`（Stat > Quality Tools）
- 模型：
  - `effect_title`：可选字符串（默认空或「效应」）
  - `categories[]`：默认 **6** 支，标签默认对齐 Minitab 英/中产品文案：  
    Personnel / Machines / Materials / Methods / Measurements / Environment  
    （UI 可显示：人员、机器、材料、方法、测量、环境）
  - 每支 `causes[]`：字符串列表（可空 → 只画骨架）
  - 可选一层 `subcauses`：挂在某个 cause 下的字符串列表（P0 支持 **一层** 子因即可）
- 布局约定（可读性，非算法）：奇数支在上、偶数支在下（Minitab 叙述）；实现可用简单鱼骨几何，不强制像素级对齐
- 输出：一页图 + 可选「结构摘要」表（类别、原因数、子因数）；**无**检验表
- 输入：对话框常量表 或 每支一列文本；字符长度产品上限建议 72（对齐 Minitab）

**明确不做：** 统计检验 / 权重 / 自动排序；与 `pareto` 合并或重做 Pareto；多级无限深树编辑器；假「根因已验证」解释文案。

### D.3 「公式」

无。持久化 JSON / OutputPage 结构即可，例如：

```text
effect, categories[{label, causes[{text, subcauses[text]}]}]
```

### D.4 表形 / 图

| 产出 | 合同 |
|---|---|
| 鱼骨图 | 效应标题 + 类别骨 + 原因刺 + 可选子刺 |
| 结构摘要（可选） | 类别、原因、子因计数 |

### D.5 测试

空骨架可出图；6 默认标签；自定义类别数（允许 ≠6）；一层子因；不触碰 `pareto` 回归。

---

## E. 跨项「明确不做」总表

| 禁止 | 原因 |
|---|---|
| 假 Minitab numeric golden | `formula_reference ≠ golden` |
| 拆坏 `two_proportions` / 重做 `pareto` |  backlog / session brief |
| Runs 精确表 + Run Chart 四模式混进同一命令 | 产品入口不同（Nonparametrics vs Quality Tools） |
| Fisher r×c 网络算法 | 超出 P0 2×2 表形 |
| Run Chart SPC 控制限 / Test 1–8 | Minitab Run Chart 不做控制限 |
| 鱼骨当假设检验 | 仅为结构化头脑风暴图 |

## F. 实现提示（C++ agent）

1. Runs：domain `runs_test`；复用正态 CDF；接线 `analysis_commands` + 帮助目录条目。  
2. Fisher：抽出/复用 `fisher_two_sided`；新 UI 表形；**禁止**改 two_proportions 断言。  
3. Run Chart：domain 计算四 P + UI 图；与 `append_gage_run_chart` 分离。  
4. Fishbone：以数据模型 + 绘制为主；acceptance 手工看图即可。  
5. 全部测试标注 `# source: formula_reference`。

## G. 本轮落地备注（2026-08-21，相对上文研究合同）

| 项 | 已落地 | 相对研究的窄化（下轮可选加深，勿当未接线重做） |
|---|---|---|
| `runs_test` | ✅ 公式 A.3；mean/median/value；`NonparametricFacts` | 诊断码为 `runs_normal_approximation_thin`（非文中 `runs_normal_approx_weak`）；无独立 `RunsTestFacts` |
| `fisher_exact` | ✅ 两分类列→2×2；复用 `fisher_two_sided`；不改 `two_proportions` | **未做**四格 a/b/c/d 直接输入；无独立 `FisherExactFacts`（挂 `ChiSquareFacts.method=fisher_exact`） |
| `run_chart` | ✅ 关于中位数四模式 P；**平坦差分归下行**（C.3.2） | Facts 未全量序列化 expected/longest；subgroup>1 仍不做 |
| `cause_and_effect` | ✅ 效果标题 + 类别/原因列 → 结构表 + 条图 | **未做**默认 6 骨几何鱼骨 / 一层子因画布；产品口径见 backlog「结构化表+条图」 |

[Research P0 formulas](8493fa6f-7521-4dc3-961c-c6d4e69a3a52) 产出本文；实现以 §G 为准，避免按 D.2 全量鱼骨几何误判为缺口。
