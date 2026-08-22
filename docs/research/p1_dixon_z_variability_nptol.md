# P1：Dixon 异常值 / 1-Sample Z / Variability Chart / 非参数容差深化

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。禁止用 Minitab Help 示例页数字当断言测试 golden。  
> 队列项：Dixon（加深 `outlier_test`）、1-sample Z、Variability Chart、非参数容差 UI/表形深化。  
> **禁止拆坏**现有 Grubbs；**禁止**把 Variability 做成 Multi-Vari 重做；**禁止**移除正态容差路径。

---

## 禁止偷懒清单（执行 agent 必读）

1. **禁止**只改文档/菜单文案而不接 domain 核与 `AnalysisService` / 表形 / Facts / 序列化。
2. **禁止**删除或改坏现有 `grubbs` 路径；Dixon 只能是 `method` 开关新增。
3. **禁止**把 Dixon 做成“只报临界值、不报 P”却声称对齐 Minitab；若本轮暂用查表，须在诊断标明 `dixon_p_approx_or_table`，不得静默。
4. **禁止**用样本 SD 冒充 1-Sample Z 的已知 σ；σ 必须来自配置输入。
5. **禁止**把 `one_sample_z` 并入 `one_sample_t` 同一命令；可共享表形/输出建造 seam，命令必须独立。
6. **禁止**把 Variability Chart 命令 ID 做成 `multi_vari` 别名或复用其图表语义；允许复用 cell 聚合辅助函数，但命令/Facts/图面板契约独立。
7. **禁止**本轮实现 Minitab 全 8 因子 Variability；锁定 **测量 + 1～2 因子**。
8. **禁止**只画均值面板不画 SD 面板（产品锁定双面板）。
9. **禁止**非参数容差继续藏在隐式 `variance_method=nonparametric` 且 UI 无显式 method 选项。
10. **禁止**假 Minitab golden（示例 Mean/Z/P、示例 cell mean、示例 achieved confidence 数值不得进断言测试）。
11. **禁止**跳过 `OutlierTestFacts` / 新 Z Facts / Variability Facts / `ToleranceFacts` 的 method 字段与 round-trip。
12. **禁止**把 Multi-Vari 的“均值连线交互探索”UI 原样搬到 Variability；Variability 是 mean+range + SD 双图，不是 Multi-Vari redo。

---

## A. Dixon Outlier（加深 `outlier_test`）

### A.1 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab Dixon/Grubbs 方法 | [Outlier Test — Methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/outlier-test/methods-and-formulas/methods-and-formulas/) | 2026-08-21 |
| Minitab 方法选择 / n 建议 | [Select the analysis options](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/outlier-test/perform-the-analysis/select-the-analysis-options/) | 2026-08-21 |
| Minitab 结果表形 | [Interpret all statistics and graphs](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/outlier-test/interpret-the-results/all-statistics-and-graphs/) | 2026-08-21 |
| NIST Grubbs | [NIST e-Handbook 1.3.5.17.1 Grubbs](https://www.itl.nist.gov/div898/handbook/eda/section3/eda35h1.htm) | 2026-08-21 |
| NIST / Dataplot Dixon 入口 | [DIXON TEST (Dataplot)](https://www.itl.nist.gov/div898/software/dataplot/refman1/auxillar/dixon.htm) | 2026-08-21 |
| 临界值表（核对用） | Rorabacher (1991), *Anal. Chem.* 63:139–146；Dixon (1951) *Ann. Math. Statist.* 22:68–78；King (1953) *JASA* 48:531–533；McBane (2006) *J. Stat. Softw.* 16(3) | 2026-08-21 |

### A.2 产品选型

- **加深命令** `outlier_test`（Stat > Basic Statistics > Outlier Test）
- 配置 `outlier_method`（或复用已有 method 字段）：
  - **`grubbs`**（默认，保持现状）
  - **`dixon_r10`**（Dixon Q）— **本轮 P0 深度必做**
  - stretch：`dixon_r11` / `dixon_r21` / `dixon_r22`
- 备选假设（与现有对齐）：`two_sided` | `smallest` | `largest`
- 输入：单列数值；complete-case；`*` → N*；保留 `source_row`
- 假设：近似正态；单异常值设计（与 Minitab 一致）
- Facts：扩展 `OutlierTestFacts`：
  - 保留 `g_statistic`（Grubbs）
  - 新增 `method`（`grubbs`|`dixon_r10`|…）、`r_statistic`（Dixon）、可选 `rij` 标签
  - 共用：`n`、`p_value`、`outlier_value`、`source_row`、`direction`、`alternative`、`alpha`

**明确不做：** 移除 Grubbs；迭代删点后再检；Tietjen–Moore / ESD 多异常；假 Minitab golden P；把 Dixon 做成独立命令拆走现有入口。

### A.3 公式（锁定）

有限观测排序 \(y_1\le y_2\le\cdots\le y_n\)（\(n\ge 3\)；否则诊断）。  
下标：\(i\)=与可疑异常值同侧极端个数（1 或 2）；\(j\)=对侧极端个数（0、1、2）。

#### A.3.1 单侧统计量（Minitab / Dixon）

检验**最小值** \(y_1\)：

\[
r_{ij}^{\mathrm{low}}=\frac{y_{1+i}-y_1}{y_{n-j}-y_1}
\]

检验**最大值** \(y_n\)：

\[
r_{ij}^{\mathrm{high}}=\frac{y_n-y_{n-i}}{y_n-y_{1+j}}
\]

展开（实现查表用）：

| 方法 | 低端（测 \(y_1\)） | 高端（测 \(y_n\)） |
|---|---|---|
| **r10**（Dixon Q） | \((y_2-y_1)/(y_n-y_1)\) | \((y_n-y_{n-1})/(y_n-y_1)\) |
| r11 | \((y_2-y_1)/(y_{n-1}-y_1)\) | \((y_n-y_{n-1})/(y_n-y_2)\) |
| r21 | \((y_3-y_1)/(y_{n-1}-y_1)\) | \((y_n-y_{n-2})/(y_n-y_2)\) |
| r22 | \((y_3-y_1)/(y_{n-2}-y_1)\) | \((y_n-y_{n-2})/(y_n-y_3)\) |

分母 ≤ 0 或索引越界 → 诊断、不出检验。

#### A.3.2 双侧统计量（King / Minitab）

\[
r_{ij}=\max\bigl(r_{ij}^{\mathrm{low}},\,r_{ij}^{\mathrm{high}}\bigr)
\]

可疑点取使该 max 成立的一端；`direction`=`low`|`high`；`outlier_value`/`source_row` 对应该端极端观测（若并列，优先 \|偏离中位更大\| 或先 high——实现固定一种并写进诊断）。

#### A.3.3 P 值（目标对齐 Minitab）

在正态假设下，高低端单侧 \(r_{ij}\) 同分布。设观测单侧统计量为 \(r\)：

- **单侧：** \(p=1-F_{ij}(r)\)
- **双侧（King）：** \(p=1-[F_{ij}(r)]^{2}\)  
  （对 r10，该式为精确关系；其它 \(ij\) Minitab 同用此式作双侧）

\(F_{ij}\)：Dixon (1951) / McBane (2006) 密度的数值积分（Minitab：内层 Gauss–Laguerre、外层 Gauss–Hermite、CDF 用 Gauss–Legendre）。

**本轮可实施分层：**

1. **P0（必做）：** 锁定 `dixon_r10` 统计量 + 能产出可比较的 P（优先移植 McBane 数值 CDF；若工期不够，允许 Rorabacher 临界值插值得近似 P，诊断标记，**不得**声称 exact Minitab P）。
2. **Stretch：** `r11/r21/r22` 全套 + 完整 quadrature。

Dixon 建议样本量（Minitab Options；仅作 UI 提示，**不**自动改 method）：

| n | 建议比率 |
|---|---|
| 3–7 | r10 |
| 8–10 | r11 |
| 11–13 | r21 |
| ≥14 | r22 |

判定：`reject` ⇔ \(p\le\alpha\)（默认 α=0.05）。H0：无异常值；H1：按 alternative。

### A.4 表形

| 表 | 合同 |
|---|---|
| 方法 | Grubbs / Dixon r10（及 stretch 其它 rij） |
| 描述 | N、N*、Mean、StDev（Grubbs 需要；Dixon 仍可显示作对照） |
| 检验 | Outlier、Row、Statistic（G 或 r）、P-Value、Decision |
| Dixon 附加 | 按 Minitab：可显示 x[1]、x[2]、x[N-1]、x[N] 等括号序统计（r10 至少 min/max） |
| 图 | 现有个体值图 + `source_row` 高亮；不因 Dixon 换图类型 |

### A.5 测试

手算小样本 r10 高低端；分母为 0；n&lt;3；Grubbs 回归；`# source: formula_reference`。禁止断言 Minitab 示例 P。

---

## B. 1-Sample Z（已知 σ）

### B.1 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 方法 | [1-Sample Z — Methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-sample-z/methods-and-formulas/methods-and-formulas/) | 2026-08-21 |
| Minitab 数据注意 | [Data considerations](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-sample-z/before-you-start/data-considerations/) | 2026-08-21 |
| Minitab 选项 / 备择 | [Select the analysis options](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-sample-z/perform-the-analysis/select-the-analysis-options/) | 2026-08-21 |
| NIST 已知 σ 检验 | [NIST e-Handbook 7.2.2](https://www.itl.nist.gov/div898/handbook/prc/section2/prc22.htm) | 2026-08-21 |
| NIST 检验↔CI | [NIST e-Handbook 7.1.5](https://www.itl.nist.gov/div898/handbook/prc/section1/prc15.htm) | 2026-08-21 |

### B.2 产品选型

- **新命令** `one_sample_z`（Stat > Basic Statistics；对标 1-Sample Z）
- 输入：一列测量；必填 **known σ &gt; 0**；`hypothesized_mean` μ0；`confidence`（默认 0.95）；`alternative`：`less`|`two_sided`|`greater`
- Seam：复用 `one_sample_t` 的列选择 / OutputBuilder 描述表骨架；**核函数独立**（Z 用 Φ，不用 t）
- Facts：新建 `OneSampleZFacts` 或扩展现有 t Facts 并标 `test_family=z`（推荐独立结构避免污染 t）：N、N*、mean、sample_sd（仅展示）、known_sigma、mu0、z、p、ci_lower/ci_upper、alternative、confidence

**明确不做：** 用样本 s 代替 σ；改 `one_sample_t` 行为；功效/样本量本轮；假 Minitab golden。

### B.3 公式（锁定）

有限观测 \(n\ge 1\)，\(\bar x=\frac1n\sum x_i\)，已知 \(\sigma>0\)。

1. **Z 统计量**

\[
Z=\frac{\bar x-\mu_0}{\sigma/\sqrt{n}}
\]

2. **P 值**（\(\Phi\)=标准正态 CDF）

| 备择 | P |
|---|---|
| \(\mu<\mu_0\) | \(\Phi(Z)\) |
| \(\mu\neq\mu_0\) | \(2\bigl(1-\Phi(|Z|)\bigr)\) |
| \(\mu>\mu_0\) | \(1-\Phi(Z)\) |

3. **置信区间**（名义水平 \(\gamma=1-\alpha\)；\(z_{\alpha/2}=\Phi^{-1}(1-\alpha/2)\)）

- 双侧：\(\bar x\pm z_{\alpha/2}\,\sigma/\sqrt{n}\)
- 单侧 lower bound（备择 greater）：\(L=\bar x-z_{\alpha}\,\sigma/\sqrt{n}\)，\(U=+\infty\)
- 单侧 upper bound（备择 less）：\(U=\bar x+z_{\alpha}\,\sigma/\sqrt{n}\)，\(L=-\infty\)

Minitab 同时展示 **Known σ**（计算用）与 **StDev**（样本，不参与 Z/CI）。产品锁定同此。

### B.4 表形

| 表 | 合同 |
|---|---|
| 描述统计 | N、Mean、StDev（样本）、SE Mean=\(\sigma/\sqrt{n}\)、Known σ |
| 检验 | μ0、Z、P-Value |
| 置信区间 | 名义水平、下限、上限（单侧一端为 ∞/省略） |

### B.5 测试

手算 \(Z\) 与双侧 CI；σ≤0 诊断；与同数据 `one_sample_t` 对比仅作差异说明（不得 equalfail）；`# source: formula_reference`。

---

## C. Variability Chart

### C.1 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab Overview | [Overview for Variability Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/variability-chart/before-you-start/overview/) | 2026-08-21 |
| Minitab 数据注意 | [Data considerations](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/variability-chart/before-you-start/data-considerations/) | 2026-08-21 |
| Minitab 方法 | [Methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/variability-chart/methods-and-formulas/methods-and-formulas/) | 2026-08-21 |
| Minitab 解读 | [Key results](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/variability-chart/interpret-the-results/key-results/) | 2026-08-21 |
| 对比：Multi-Vari（勿混） | 现有 `multi_vari` / `docs/research/unusual-obs-multivari-doe-chart-formulas.md` | 2026-08-21 |

### C.2 产品选型

- **新命令** `variability_chart`（Stat > Quality Tools > Variability Chart）
- **本轮锁定：** Response（测量）+ **1 或 2** 个因子（每个因子 ≥2 水平）；Minitab 上限 8 因子仅作路线图，本轮不做
- 输出 **两面板**（与 Quality Tools 版一致，非 Graph Builder 阉割版）：
  1. **Average response with variation**：各 factor-level **cell mean**；cell 内 **min–max range bar**（无重复则无 range bar）；可选 overall mean 水平线、cell mean 连线
  2. **Standard deviation chart**：各 cell 样本 SD；水平线 = 各 cell SD 的平均
- 相对 `multi_vari`：**加深 seam 可复用 cell 分组/坐标映射**，但命令、Facts、图类型、解释文案必须 DISTINCT
- Facts：`VariabilityChartFacts`（factor_count、factor_names、valid_count、missing_count、cell 数、overall_mean、mean_of_cell_sds）

**明确不做：** Multi-Vari redo；3–8 因子本轮；ANOVA/显著性检验并入本命令；IQR box 变体（stretch）；假 Minitab 示例 cell mean golden。

### C.3 公式（锁定）

按 Factor1 → Factor2（本轮最多 2）字典序/水平出现序排列形成 **cells**（完整因子水平组合）。对 cell \(c\) 内有限响应 \(\{y_{c,1},\ldots,y_{c,n_c}\}\)：

\[
\bar y_c=\frac1{n_c}\sum_{k=1}^{n_c} y_{c,k}
\]

\[
s_c=\sqrt{\frac1{n_c-1}\sum_{k=1}^{n_c}(y_{c,k}-\bar y_c)^2}\quad(n_c\ge 2;\ \text{否则 SD 点缺失})
\]

Range bar：\( [\min y_c,\,\max y_c] \)（仅 \(n_c\ge 2\)）。

Overall mean：全部有限响应的算术平均（非 cell 均值的等权平均）。

SD 图中心线：

\[
\overline{s}=\frac1{m}\sum_{c:\,n_c\ge 2} s_c
\]

（\(m\)=有 SD 的 cell 数；与 Minitab“average of all standard deviations”对齐。）

因子水平均值（可选表/辅助线，Minitab “means from right to left”）：对某一因子某水平，聚合该水平下所有观测的均值（非仅 cell 等权）。本轮 **必做 cell 统计**；因子水平均值线为 **推荐同做**（1–2 因子成本低）。

### C.4 表形 / 图合同

| 输出 | 合同 |
|---|---|
| Cell 表 | Factor levels、N、Mean、StDev、Min、Max |
| 图1 | Cell means + range bars +（可选）overall mean / connecting line |
| 图2 | Cell SDs + mean-of-SDs 线 |
| 诊断 | 因子&lt;1 或 &gt;2；某因子 &lt;2 水平；全 cell n=1（SD 图空） |

### C.5 测试

手算 2 因子小表 cell mean/SD/range；单因子路径；与 `multi_vari` 命令并存回归；`# source: formula_reference`。禁止断言 Minitab Help 算例 0.35 / 0.615 等为 golden（可作人工公式核对）。

---

## D. Nonparametric Tolerance（加深 `tolerance_intervals`）

### D.1 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 正态页内非参数公式 | [Tolerance Intervals (Normal) — Methods](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/tolerance-intervals-normal-distribution/methods-and-formulas/methods-and-formulas/) | 2026-08-21 |
| Minitab 非正态解读 / 表 | [Tolerance intervals (Nonnormal) — interpret](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/tolerance-intervals-nonnormal-distribution/interpret-the-results/all-statistics-and-graphs/tolerance-intervals/) | 2026-08-21 |
| Minitab 非正态方法 | [Tolerance intervals methods (Nonnormal)](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/tolerance-intervals-nonnormal-distribution/methods-and-formulas/tolerance-intervals/) | 2026-08-21 |
| 仓库既有笔记 | `docs/research/p1_tolerance_intervals_methods.md` | 2026-08-21 |
| 实现现状 | `nonparametric_tolerance_interval`；`variance_method=="nonparametric"` 切换 | 2026-08-21 |

### D.2 产品选型

- **加深** `tolerance_intervals`：UI **显式** `method` = `normal` | `nonparametric`（不要只靠隐式 `variance_method`；可双写兼容旧配置）
- 正态路径：保持 Howe 双侧 / Natrella 单侧 + 现有诊断（不在本轮改成 Minitab 精确 k）
- 非参数路径：连续分布、序统计；**必须**填充：
  - `Facts.method` = `order_statistic_nonparametric`（或现名）
  - `Facts.method_family` = `nonparametric`
  - `Facts.achieved_confidence`
  - 区间上下限（样本不足时的有限 range 回退须诊断 + 报告偏低 achieved）
- 表形对齐 Minitab 可读性：分表或分列展示 Method / Lower / Upper / Achieved confidence / Target confidence / Coverage；正态与非参数可同页并列或按 method 单显（产品：**按 method 单显主表**，另一方法不强制同屏）

**明确不做：** 本轮改写正态精确积分 k；参数非正态族（Weibull 等）；样本量表；假 Minitab achieved golden。

### D.3 公式（锁定 — 序统计 / Minitab exact nonparametric）

令 \(X_{(1)}\le\cdots\le X_{(n)}\)，覆盖率 \(P\in(0,1)\)，目标置信 \(1-\alpha\)。

Wilks：对 \(1\le r&lt;s\le n\)，区间 \((X_{(r)},X_{(s)})\) 的覆盖量有 Beta\((r,\,n-s+1)\) 性质（分布自由）。

#### D.3.1 单侧

令 \(Y\sim\mathrm{Bin}(n,\,1-P)\)。取最大整数 \(k\) 使

\[
P(Y\le k)\le \alpha
\]

（与 Krishnamoorthy–Mathew / Minitab：最大 \(k\) 满足该尾概率约束；实现时用现有 binomial CDF，单测对齐手算。）

- 下界：\(L=X_{(k)}\)，\(U=+\infty\)
- 上界：\(U=X_{(n-k+1)}\)，\(L=-\infty\)
- **Achieved confidence：** \(P(Y>k)=1-P(Y\le k)\)

若无一 \(k\) 使目标达到：退回最外层有限端点并报告偏低 `achieved_confidence` + 诊断 `nonparametric_sample_too_small`（对齐 Minitab“非信息区间→显示数据 range、achieved 远低于目标”的产品语义）。

#### D.3.2 双侧（Minitab 对称序统计）

令 \(V\sim\mathrm{Bin}(n,\,P)\)。取**最小**整数 \(k\) 使

\[
P(V\le k-1)\ge 1-\alpha
\]

等价 \(k-1=F_V^{-1}(1-\alpha)\)（实现取满足不等式的最小 \(k\)）。

Minitab 取对称：

\[
r=\left\lfloor\frac{n-k+1}{2}\right\rfloor,\qquad s=n-r+1
\]

区间 \([X_{(r)},\,X_{(s)}]\)（若 \(r&lt;1\) 或 \(s&gt;n\) 或 \(r\ge s\) → 样本不足路径）。

**Achieved confidence（锁定 Minitab Normal 方法页措辞）：** \(P(V&lt;k-1)\)；若与 `P(V≤k-1)` 实现差 1 个质量单位，以二项式离散手算单测钉死一种，并在 `method` 注释写清。  
推荐实现钉死：**achieved = P(V ≤ k−1)**（与“最小 k 使该概率 ≥1−α”自洽）；若严格跟页内 “P(V&lt;k−1)” 字符串，须在代码注释引用页并单测。

> 现状代码对双侧穷举 \((r,s)\) 宽搜索；**本轮产品锁定改为 Minitab 对称 \(s=n-r+1\)**，避免与 Help 表形/achieved 语义漂移。旧穷举可留测试对照但不得作为默认。

### D.4 表形

| 表 | 合同 |
|---|---|
| 过程数据 | N、N*、Mean、StDev（非参数仍可显示描述） |
| 容差区间 | Method family、Coverage P、Target confidence、Achieved confidence、Lower、Upper、Interval type |
| 诊断 | 样本不足、achieved≪target、参数非法 |

### D.5 测试

单侧/双侧小 n 手算 \(k,r,s\)；UI method 切换 normal↔nonparametric；Facts round-trip；`# source: formula_reference`。禁止 Minitab 示例 achieved 数值断言。

---

## E. 接线清单（实现顺序建议）

1. `outlier_test`：`method` + `dixon_r10` 核 + Facts/序列化/解释；Grubbs 回归绿。
2. `one_sample_z`：domain Z + 命令/UI（known σ）+ 表形；不碰 t 核行为。
3. `variability_chart`：1–2 因子聚合 + 双图 + Facts；与 `multi_vari` 并行。
4. `tolerance_intervals`：显式 method UI；非参数改为 Minitab 对称序统计 + achieved 合同；表形可读。

每项完成后：更新 `algorithm-wiring-index.md` 一行；测试标记 `# source: formula_reference`。

## G. 本轮落地备注（2026-08-21）

| 项 | 状态 |
|---|---|
| Dixon r10 | ✅ 加深 `outlier_test`；P 插值近似 |
| one_sample_z | ✅ 独立命令 |
| variability_chart | ✅ 1～2 因子双面板 |
| tolerance method UI | ✅ normal\|nonparametric |

未做：Dixon 其他 rij；Acceptance sampling；Zone/Z-MR/MA；ANOM；Poisson GOF。

---

## F. formula_reference ≠ golden（总则）

- Minitab/NIST URL 与访问日期仅证明公式与产品口径来源。
- 允许用 Help 算例做**人工**核对；**禁止**写入 `EXPECT_*` / 快照 golden。
- 单测只断言：手算小样本、边界诊断、回归不破坏、序列化字段存在性与方法标签。
