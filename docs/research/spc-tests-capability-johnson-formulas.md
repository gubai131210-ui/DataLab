# SPC Test 2–8 边界 / 正态能力表形 / Johnson / 非正态能力

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> 本文只记录官方公式、Minitab 表形/图名与本轮实现边界。`formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 特殊原因检验 | [Using tests for special causes](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/basics/using-tests-for-special-causes/) | 2026-08-20 |
| NIST SPC 通则 | [NIST 6.3.2 What are Control Charts?](https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm) | 2026-08-20 |
| 正态过程能力输出 | [All statistics and graphs for Normal Capability](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/interpret-the-results/all-statistics-and-graphs/) | 2026-08-20 |
| 正态能力公式 | [Normal Capability methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/methods-and-formulas/methods/) | 2026-08-20 |
| Johnson 变换 | [Johnson transformed data](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/methods-and-formulas/johnson-transformed-data/) | 2026-08-20 |
| 非正态 Z-score | [Z-score method for nonnormal capability](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/supporting-topics/capability-metrics/z-score-method-for-nonnormal-capability/) | 2026-08-20 |

本轮 **不改** Test 7 `<σ` 语义、Cp/Cpk/Pp/Ppk 核心公式、Test 1/7 已完成边界。对照表形，不填写未导出对照数值。

## 2. SPC Test 1–8 边界（切片 1）

对每个点 `y_i`，中心线 `CL_i`，局部标准差 `σ_i = (UCL_i − CL_i)/3`（或 `point_sigma`）。

| 测试 | 触发条件 | 边界约定 |
|---|---|---|
| Test 1 | 1 点超出 3σ 控制限 | 恰在 LCL/UCL **不**触发（严格超出） |
| Test 2 | 连续 9 点中心线同侧 | 8 点同侧 **不**触发；中心线点重置运行 |
| Test 3 | 连续 6 点严格单调 | 含相等值 **不**触发 |
| Test 4 | 连续 14 点上下交替 | 第 14 点破坏交替 **不**触发 |
| Test 5 | 3 点中 ≥2 点同侧 **严格** `>2σ` | 恰在 2σ **不计**；仅 1 点 `>2σ` **不**触发 |
| Test 6 | 5 点中 ≥4 点同侧 **严格** `>1σ` | 恰在 1σ **不计**；仅 3 点 `>1σ` **不**触发 |
| Test 7 | 连续 15 点 `\|y−CL\| < σ` | 恰 1σ **不计**入「内」（已完成，本轮不改） |
| Test 8 | 连续 8 点 `\|y−CL\| > σ` 且不在中心线 | 恰 1σ **不**触发；**不要求**两侧交替 |

I 图与 Xbar 图适用 Test 1–8；R/S/MR 仅 Test 1–4。

| 输出 | 合同 |
|---|---|
| 失败点集 | `special_cause_points[test−1]` 每 Test 独立 |
| 服务层 | I-MR / Xbar-R `triggered_tests` 与领域一致 |
| `source_row` | 控制图点对应原始行；子组图为子组首行 |

解释不把 Test 失败写成过程合格或控制图通过。

## 3. 正态能力 / Sixpack 表形（切片 2）

指数公式不改：

```text
Cp  = (USL − LSL) / (6 σ_within)
Cpk = min((μ − LSL)/(3 σ_within), (USL − μ)/(3 σ_within))
Pp  = (USL − LSL) / (6 σ_overall)
Ppk = min((μ − LSL)/(3 σ_overall), (USL − μ)/(3 σ_overall))
```

| 表 | 列/行 | 合同 |
|---|---|---|
| Process Data | LSL, Target, USL, Sample Mean, StDev(Within), StDev(Overall), Sample N, Missing N*, Anderson-Darling (A²*, P, 判定), 假设状态 | AD 只展示证据，不写「已证明正态」 |
| Performance (PPM) | 行：低于 LSL / 高于 USL / 合计；列：观测 / 期望 Within / 期望 Overall | 等于规格限 **不计** 超规；单侧缺失侧为 `*` |
| Potential (Within) Capability | Cp, CPL, CPU, Cpk, Cpm, Z.Bench | 正态路径 |
| Overall Capability | Pp, PPL, PPU, Ppk, Z.LSL, Z.USL | 正态路径 |
| Sixpack 子图顺序 | I/Xbar → 直方图 → MR/R → 正态概率图 → 最后 25 点 → 能力图 | 直方图与正态能力同一 PlotSpec 合同 |

解释不写合格 / Cpk 通过。

## 4. Johnson 变换能力（切片 3）

Chou + AD p>0.10 选 SB/SL/SU；变换后 overall Pp/Ppk；**不**报告 within Cp/Cpk。

| 表 | 合同 |
|---|---|
| Process Data | 原尺度 N/N*、均值、Overall StDev、假设状态 |
| Johnson 变换 | Selected family (SB/SL/SU)、P-value (AD on transformed)、AD |
| Overall Capability | Pp, PPL, PPU, Ppk, Z.LSL, Z.USL（无 Cp/Cpk 表或列为 `*`） |
| Observed Performance | 原尺度 PPM |
| Expected Overall Performance | 变换尺度期望 PPM；**无** Expected Within 列 |

找不到变换或规格越界：只诊断（`johnson_transform_not_found` / `johnson_spec_outside_support`），不伪造 Pp/Ppk。

## 5. 非正态 Z-score 能力（切片 3）

Weibull / Lognormal 拟合 + CDF Z-score Pp/Ppk；**不**报告 Cp/Cpk。

| 表 | 合同 |
|---|---|
| Process Data | N/N*、均值、分布名、假设状态 |
| 分布参数 | Weibull: Shape, Scale；Lognormal: Location, Scale |
| Overall Capability | Pp, PPL, PPU, Ppk, Z.LSL, Z.USL |
| Observed / Expected Overall PPM | 无 Within 期望列 |

数值为 `# source: formula_reference`，不是 Minitab golden。

## 6. SPC 逐点表 / 阶段 / 历史参数（2026-08-20 后）

| 主题 | 来源 | 访问日期 |
|---|---|---|
| I 图全部统计 | [All statistics and graphs for Individuals Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/individuals-chart/interpret-the-results/all-statistics-and-graphs/) | 2026-08-20 |
| 历史参数 | [Enter historical parameters for Individuals Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/individuals-chart/perform-the-analysis/i-chart-options/enter-historical-parameters/) | 2026-08-20 |
| 阶段 | [Define stages for Individuals Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/individuals-chart/perform-the-analysis/i-chart-options/define-stages/) | 2026-08-20 |
| 逐点存储 | [Store statistics for Individuals Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/individuals-chart/perform-the-analysis/i-chart-options/store-statistics/) | 2026-08-20 |

Minitab 可把每个绘制点的值、中心线、控制限、阶段名、各 Test 失败标记存回工作表。未给历史参数时从数据估计 μ、σ；只给其中一个则另一个仍从数据估计。阶段列每个新值开始新阶段；默认每阶段重算中心线与控制限。Test 窗口与 MR 不得跨阶段。

DataLab 合同（ADR 0007）：

| 输出 | 合同 |
|---|---|
| I-MR 逐点统计 | 原始行（1-based 显示）、阶段、观测值、I CL/LCL/UCL、MR、触发测试集、最小测试；`source_row` 来自导入行，禁止 `iota` |
| 参数表 | 有历史 μ 或 σ 时标注「（历史参数）」，否则「（估计）」 |
| I-MR-R/S | I 图逐点表 + 子组逐点表（对齐 Xbar-R 表头） |
| 阶段 | `stage_column` → `phase_labels`；特殊原因窗口与 MR 窗口不跨阶段 |

不改 Test 1–8 边界语义。MR 图仍仅 Test 1–4。

## 7. Johnson 变换后概率图（2026-08-20 后）

来源同第 1 节 Johnson 变换页。Minitab 在选中变换后对变换数据做正态能力，并可用变换后样本做正态概率图。Box-Cox 路径已有变换前/后概率图。

```text
变换成功：JohnsonTransformResult.transformed 上做正态概率图（Blom 分位，保留 source_row）
变换失败 / 规格越界：只诊断，不出「变换后正态概率图」
```

不改 Chou 选族、Pp/Ppk 公式。解释不写已正态/合格。`formula_reference ≠ golden`。
