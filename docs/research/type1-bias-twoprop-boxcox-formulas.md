# Type 1 Gage 图 / Bias-Linearity 图合同 / 两比例输出 / Box-Cox 输出合同

> 研究日期：2026-08-19  
> 访问日期：2026-08-19（UTC+8）  
> 本文只记录官方公式、Minitab 表形/图名与本轮实现边界。`formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Type 1 图 | [All statistics and graphs for Type 1 Gage Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/type-1-gage-study/interpret-the-results/all-statistics-and-graphs/) | 2026-08-19 |
| Type 1 概述 | [A type 1 gage study assesses the capability of a measurement process](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/supporting-topics/other-gage-studies-and-measures/type-1-gage-study/) | 2026-08-19 |
| Type 1 关键结果 | [Interpret the key results for Type 1 Gage Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/type-1-gage-study/interpret-the-results/key-results/) | 2026-08-19 |
| Gage Linearity 公式 | [Methods and formulas for Gage Linearity](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-linearity-and-bias-study/methods-and-formulas/gage-linearity/) | 2026-08-19 |
| Gage Linearity 图 | [All statistics and graphs for Gage Linearity and Bias Study](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-linearity-and-bias-study/interpret-the-results/all-statistics-and-graphs/) | 2026-08-19 |
| 线性最小二乘均值带 | [NIST 4.1.4.1 Linear Least Squares](https://www.itl.nist.gov/div898/handbook/pmd/section1/pmd141.htm) | 2026-08-19 |
| 2 Proportions 公式 | [Methods and formulas for 2 Proportions](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/2-proportions/methods-and-formulas/methods-and-formulas/) | 2026-08-19 |
| 2 Proportions 方法说明 | [Methods that Minitab uses to perform a 2 proportions test](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/supporting-topics/tests-of-proportions-and-variances/methods-that-minitab-uses-to-perform-a-2-proportions-test/) | 2026-08-19 |
| 两比例比较通则 | [NIST 7.3.3](https://www.itl.nist.gov/div898/handbook/prc/section3/prc33.htm) | 2026-08-19 |
| Box-Cox 公式 | [Methods and formulas for Box-Cox Transformation](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/box-cox-transformation/methods-and-formulas/methods-and-formulas/) | 2026-08-19 |
| Box-Cox 图 | [All statistics and graphs for Box-Cox Transformation](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/box-cox-transformation/interpret-the-results/all-statistics-and-graphs/) | 2026-08-19 |
| Box-Cox 通则 | [NIST EDA 1.3.3.6 Box-Cox Linearity Plot](https://www.itl.nist.gov/div898/handbook/eda/section3/eda336.htm) | 2026-08-19 |
| 单比例口径 | 见 [`proportion-tost-doe-residual-imr-rs-formulas.md`](proportion-tost-doe-residual-imr-rs-formulas.md) §2 | 2026-08-19 |

## 2. Type 1 Gage 图

对标 Stat > Quality Tools > Gage Study > Type 1 Gage Study。领域 Bias / t / Cg / Cgk **不改**。

DataLab 现有（全公差，与 Minitab 默认 K=20% **不同**）：

```text
Bias = ȳ − μ0
s = 样本标准差（n−1）
t = Bias / (s / √n)
Cg  = Tol / (6s)
Cgk = min(Tol/2 − Bias, Tol/2 + Bias) / (3s)
%Tolerance = 6s / Tol × 100
```

Minitab 默认常用 `Cg = (K/100)·Tol / (6s)`，K=20。本轮**不改** DataLab 指数，也不画 Run Chart 上的 ±10% 公差带（避免图与指数口径打架）。零重复性（s=0）不输出 p=0。

| 图 | 合同 |
|---|---|
| 直方图 | `PlotKind::histogram`；测量值；参考线 `target=μ0`（标签 Ref）；规格：用户 LSL/USL，否则 `tolerance>0` 时 `μ0 ± Tol/2` |
| Run Chart | 已有 `PlotKind::control`；中心=参考值；补 `source_rows` |

导入：测量列 `extract_numeric_column`；`*` 计入 missing 诊断。解释只读 `MsaFacts`，不写「量具通过」。

## 3. Bias / Linearity 图合同

对标 Gage Linearity and Bias Study。OLS 截距/斜率/斜率 SE/斜率 CI **不改**：

```text
bias_i = y_i − r_i
b = Sxy / Sxx
a = ȳ_bias − b x̄
斜率 CI = b ± t_{1−α/2, n−2} · SE(b)
```

本轮图合同：complete-case 对齐参考列与测量列；散点为逐次偏倚；拟合线；**均值** CI 带（不是预测带 PI）：

```text
ŷ(x) = a + b x
CI(x) = ŷ ± t_{1−α/2, n−2} · s · √(1/n + (x−x̄)²/Sxx)
```

`source_rows` 为原始工作表行。解释不写「量具通过」。

### 3.1 过程变差与 Minitab 全表形（2026-08-20 后）

访问日期：2026-08-20（UTC+8）。来源：[Gage Linearity methods](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-linearity-and-bias-study/methods-and-formulas/gage-linearity/)、[All statistics and graphs](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/gage-linearity-and-bias-study/interpret-the-results/all-statistics-and-graphs/)。`formula_reference ≠ golden`。

配置 `MsaConfiguration::process_variation`：**6 × 过程标准差**。缺省时不计算 Linearity/%Linearity/%Bias 列，只诊断 `process_variation_not_provided`；**Gage Bias** 表（Bias/t/P + Average）仍输出。

```text
Linearity           = |slope| × process_variation
%Linearity          = |slope| × 100
%Bias_level         = mean_bias_level / process_variation × 100
S                   = sqrt(MSE)                         df = n−2
Constant SE         = sqrt(MSE × (1/n + x̄²/Sxx))
Constant/Slope P    = 2 × (1 − F_t(|Coef/SE|, df))
Level t/P           = 单样本 t：mean_bias_level / (s_level/√n)，df = n−1；n=1 → *
Average Bias        = 全部偏倚观测均值
Average Bias t/P    = 单样本 t：mean / (s_all/√N)，df = N−1
```

| 输出 | 合同 |
|---|---|
| 图 | 不变：散点 + 拟合 + 均值 CI 带 |
| **Coef** | Term(Constant/Slope), Coef, SE Coef, T, P |
| **S and R-Sq** | S, R-Sq |
| **Gage Linearity** | Linearity, %Linearity, P Constant, P Slope（仅 PV>0） |
| **Gage Bias** | Reference, Bias, %Bias, t, P；末行 **Average**（始终） |
| Facts | 上表 + `intercept_p_value`, `residual_s`, `average_bias`, `average_bias_p` round-trip |

显著线性（slope p≤0.05）时解释陈述各级偏倚需分别解读；**不写**量具合格。

## 4. 两比例检验输出合同

对标 Stat > Basic Statistics > 2 Proportions。领域保持 **unpooled Wald** Z/CI + Fisher（H0: p1−p2=0）。**不做** Blaker / Wilson / Agresti–Coull。

```text
p̂1 = x1/n1 ,  p̂2 = x2/n2
Δ = p̂1 − p̂2
SE_sep = √[ p̂1(1−p̂1)/n1 + p̂2(1−p̂2)/n2 ]
Z = Δ / SE_sep
Wald CI: Δ ± z_{1−α/2} SE_sep
```

Fisher 仅在差值为 0 的精确 2×2 设定下输出（与现有实现一致）。

输入对齐单比例：每组事件/试验 **独立** complete-case 多行求和；不再要求每组恰好一行。一组多行诊断 `summarized_from_multiple_rows`。

| 输出 | 合同 |
|---|---|
| 描述表 | 组、事件、试验、比例；差值行；N*/行数 |
| 检验表 | Z、Wald P、Wald CI、Fisher P |
| 图 | `PlotKind::interval`；类别 `p1 - p2`；区间为 Wald CI |
| Facts | `ProportionFacts.kind=two_sample` |

解释陈述差值证据，不是规格判定。

## 5. Box-Cox 输出合同

对标 Control Charts / Quality Tools > Box-Cox Transformation。λ 网格搜索与圆整 **不改**。

标准化比较（仅用于选 λ，几何均值 G）：

```text
W = G ln(Y)                         λ = 0
W = (Y^λ − 1) / (λ G^{λ−1})         λ ≠ 0
选 λ ∈ [−5, 5]（步长 0.01）使样本 SD(W) 最小
可圆整到 {−2,−1,−0.5,0,0.5,1,2}
存储变换：λ=0 → ln Y；否则 (Y^λ−1)/λ
```

DataLab 用 **样本 SD(W)** 选 λ，不是 Minitab 个体数据的平均移动极差。不把 Minitab λ 的 95% CL 填成 golden。

| 输出 | 合同 |
|---|---|
| 变换参数表 | 已有 N / Lambda / Transformed StDev |
| λ 选择诊断 | x=λ 网格，y=SD(W)；竖线为选定 λ |
| 概率图 | 变换前、变换后各一张；`source_rows` |
| 变换后能力 | 有 LSL/USL 时保留；不作合格判定 |
| Facts | `BoxCoxFacts`；`assumption_status=not_verified` |

解释不写「已正态」或「过程合格」。
