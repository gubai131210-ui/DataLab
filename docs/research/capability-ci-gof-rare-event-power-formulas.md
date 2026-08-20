# 正态能力指数 CI / 卡方拟合优度 / G·T 图 / 功效表形

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。对照 Minitab **表名、列名、诊断**，不填写未导出数值。

## 1. 正态过程能力 Cp/Cpk/Pp/Ppk 置信区间

### 1.1 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| 潜在能力（Cp/Cpk）方法 | [Minitab Potential capability](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/methods-and-formulas/potential-capability/) | 2026-08-20 |
| 总体能力（Pp/Ppk）方法 | [Minitab Overall capability](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/methods-and-formulas/overall-capability/) | 2026-08-20 |
| 指数定义 | [NIST e-Handbook 6.1.6 Process Capability](https://www.itl.nist.gov/div898/handbook/pmc/section1/pmc16.htm) | 2026-08-20 |
| Cpk 近似区间 | Bissell, A. F. (1990). How Reliable is Your Capability Index? *Applied Statistics* 39, 331–340 | 1990 |
| 方法比较（不实现） | Kushler, R. H. and Hurley, P. (1992). Confidence Bounds for Capability Indices. *JQT* 24, 188–195 | 1992 |

### 1.2 选型：Bissell，不是 Kushler–Hurwitz 全套

Minitab 对 **Cp / Pp** 使用 χ² 尺度区间；对 **Cpk / Ppk** 使用带 \(z_{1-\alpha/2}\) 的正态近似。该近似即 Bissell (1990) 的标准误。Kushler–Hurwitz (1992) 是比较文，评估 Bissell / Zhang et al. 等覆盖率，**DataLab 不实现 KH 的其它界**。

指数点估计不变（见 `docs/statistical-methodology.md`）：

```text
Cp  = (USL − LSL) / (6 σ_within)
Cpk = min( (μ−LSL)/(3σ_within), (USL−μ)/(3σ_within) )
Pp / Ppk 把 σ_within 换成 σ_overall
```

### 1.3 Cp / Pp（χ² 尺度）

\[
\widehat{C}_p \sqrt{\chi^2_{\alpha/2,\nu}/\nu}
\quad\text{到}\quad
\widehat{C}_p \sqrt{\chi^2_{1-\alpha/2,\nu}/\nu}
\]

Pp 同形，自由度 \(\nu=N-1\)。

**Within 自由度 \(\nu\)**（对齐 Minitab Potential 页）：

| σ_within 方法 | \(\nu\) |
|---|---|
| 个体 / 平均移动极差（Rspan=2） | \(n - 1\) |
| 合并标准差 | \(\sum(n_i-1)\) |
| Rbar | \(0.9\,k(n-1)\) |
| Sbar | \(f_n k(n-1)\)，\(f_n\) 见 Minitab 表（n=2→0.88 … n≥65→1） |

DataLab 默认个体数据：\(\nu_{\mathrm{within}}=\nu_{\mathrm{overall}}=N-1\)。组间/组内若缺少子组结构细节，CI 用 \(N-1\) 并诊断 `ci_df_used_sample_n`。

### 1.4 Cpk / Ppk（Bissell）

Toler = 6（故 3σ 项给出 \(1/(9N)\)）：

\[
\widehat{C}_{pk} \pm z_{1-\alpha/2}
\sqrt{\frac{1}{9N}+\frac{\widehat{C}_{pk}^{2}}{2\nu}}
\]

Ppk 用 overall \(\nu=N-1\)。CPL/CPU/PPL/PPU 各自有点估计时用同一标准误形式（把 \(\widehat{C}_{pk}\) 换成该单侧指数）。

### 1.5 表形与边界

Minitab Capability Analysis 在 Potential / Overall 表给出点估计及 CI。DataLab 列：

| 指标 | 估计 | 下限 | 上限 |

- 缺规格侧：该行估计与 CI 均为 `*`。
- Johnson / 非正态：不报告 Within Cp/Cpk CI；Overall 仅在正态 overall 公式适用时填写（Johnson 变换后按正态 overall 填 Pp/Ppk CI，仍标 `formula_reference`）。
- \(N<2\)、\(\sigma\le 0\)、\(\nu\le 0\)：不填假区间，诊断 `capability_ci_not_computed`。
- 解释不写过程合格。默认 α 来自分析置信水平（95% → α=0.05）。

## 2. 卡方拟合优度（新命令，区别于列联表）

### 2.1 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| 方法与公式 | [Chi-Square Goodness-of-Fit Test methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/how-to/chi-square-goodness-of-fit-test/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| 输出解读 | [Interpret all statistics](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/how-to/chi-square-goodness-of-fit-test/interpret-the-results/all-statistics-and-graphs/) | 2026-08-20 |
| 官网例（公式参考） | [Example](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/how-to/chi-square-goodness-of-fit-test/before-you-start/example/) | 2026-08-20 |

列联表 `chi_square` **保持**观察/检验/单元格三表 + 热图。拟合优度是命令 `chi_square_gof`。

### 2.2 公式

分类列 complete-case 计数。缺失/`*`/`NA` 计入 N*，不进类别。类别顺序 = **首次出现顺序**（禁止按行号 zip 两列）。

默认 \(p_i=1/k\)。用户可给逗号分隔比例，必须与类别个数相同且和为 1（允许 1e-8 容差）；否则只诊断、不算 χ²。

\[
E_i = p_i N,\quad
\chi^2=\sum_i (O_i-E_i)^2/E_i,\quad
\mathrm{DF}=k-1
\]

Pearson 残差 \((O-E)/\sqrt{E}\)。贡献 \((O-E)^2/E\)。P = \(P(\chi^2_{\mathrm{DF}} > \text{统计量})\)。

官网公式参考例（不是 golden 文件）：O=(5,15,10,10)，p=(0.1,0.2,0.3,0.4)，N=40 → χ²=8.9583，DF=3。

### 2.3 表形

**观察与期望**

| Category | Observed | TestProportion | Expected | Residual | Contribution to Chi-Square |

**卡方检验**

| N | N* | DF | Chi-Sq | P-Value |

期望 <5 诊断 `expected_count_below_five`。解释不写因果、不写已证明分布一致。条图：Observed 与 Expected 两系列。

## 3. G 图与 T 图（稀有事件间隔）

### 3.1 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| G 图方法 | [G Chart methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/rare-event-charts/g-chart/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| G 图概述 | [Overview for G Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/rare-event-charts/g-chart/before-you-start/overview/) | 2026-08-20 |
| T 图方法 | [T Chart methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/rare-event-charts/t-chart/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| T 图数据考虑 | [T Chart data considerations](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/rare-event-charts/t-chart/before-you-start/data-considerations/) | 2026-08-20 |

### 3.2 输入差异（硬约束）

Minitab G 图可吃**事件日期**或机会数；T 图可吃日期时间或连续时长。DataLab 本轮只接受 **`parse_numeric_cell` 得到的数值间隔列**（天数或机会数）。不另写日期解析。事件日期需用户先变成间隔。

complete-case；`source_row` 保留；`*` 计 N*。

### 3.3 G 图（几何）

点 \(x_i\) = 事件之间的机会数/天数（支持 0,1,2,…）。

\[
\bar x=\frac1n\sum x_i,\quad \hat p=\frac1{\bar x+1}
\]

几何分布（失败次数，CDF \(F(y)=1-(1-\hat p)^{y+1}\)）。对 q∈{0.00135, 0.5, 0.99865}：取满足 \(F(g)\ge q\) 的最小整数 \(g_b\)，\(g_a=g_b-1\)，线性插值

\[
G = g_a + \frac{q-F(g_a)}{F(g_b)-F(g_a)}
\]

中心线 CL=\(G_{0.5}-1\)，LCL=\(G_{0.00135}-1\)，UCL=\(G_{0.99865}-1\)（与 Minitab INVCDF 再减 1 一致）。LCL<0 时该点 LCL 记为不显示（非有限），Test 1 只对有限限比较。

默认 **Test 1**（点 < LCL 或 > UCL）。Benneyan 延后。逐点表：原始行、间隔、CL、LCL、UCL、Test 1。`SpcFacts.out_of_control_count`。

### 3.4 T 图（Weibull）

全部间隔 >0：用现有 `fit_weibull`（全部为失效），禁止 Kalman。

形状 β、尺度 η：

\[
t_q = \eta\,[-\ln(1-q)]^{1/\beta}
\]

CL=\(t_{0.5}\)，LCL=\(t_{0.00135}\)，UCL=\(t_{0.99865}\)。

存在 0 间隔：0 不进入 Weibull MLE；对正间隔做 \(\ln t\) 对 \(\ln(-\ln(1-\hat F))\) 的 OLS，\(\hat F=(i-0.3)/(n+0.4)\)，尺度 \(\exp(\beta_0)\)，形状 \(1/\beta_1\)；诊断 `zero_interval_regression_used`。无法估计则只诊断。

默认 Test 1。解释：超限点待调查；间隔变长（点高于 UCL）在稀有不良事件中常表示改善，**不自动写成过程合格**。

## 4. 功效与样本量（接线已有领域函数）

### 4.1 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| 1-Sample t 功效 | [Power and Sample Size for 1-Sample t](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/power-and-sample-size/how-to/hypothesis-tests/power-and-sample-size-for-1-sample-t/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| 解读 | [Interpret 1-Sample t power](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/power-and-sample-size/how-to/hypothesis-tests/power-and-sample-size-for-1-sample-t/interpret-the-results/all-statistics-and-graphs/) | 2026-08-20 |

领域 `t_power.h` 已有单/双样本 t、ANOVA、单/双比例的功效与所需 n。本轮不改这些公式。

### 4.2 表形

Minitab 给出 Sample Size、Difference（或效应）、Power；求 n 时另给 **Actual Power**（因 n 取整，实际功效 ≥ 目标）。

DataLab「功效与样本量」：

| Sample Size | Per Group | Total N | Effect Size | Target Power | Actual Power | DF | Alpha |

可对逗号分隔的多个 n 或效应各算一行。功率曲线：求功效时 x=效应网格、y=功效；求 n 时 x=n、y=实际功效。

`PowerFacts`：power、effect_size、mode、sample_size、target、actual_power。解释只陈述假设功效，禁止「样本量足够」。不读工作表（`requires_data=false`）。

## 5. 图表侧栏与复制/清除（非统计）

工作模型仍是 `ChartModel`。侧栏补齐对话框已有：X/Y Min/Max 分别 Auto、标题/轴/图例字号、主题。预览/PDF/PNG/复制同一 `ChartRenderer`。

复制对照 `copy_chart` → `copy_to_clipboard` → `render_to_pixmap`。清除对照 `clear_selection` → `clear_cells`。空单元格 Display 不再把空串画成 `*`（显式 `*`/`NA` 仍显示星号）。双击仍不打开属性。
