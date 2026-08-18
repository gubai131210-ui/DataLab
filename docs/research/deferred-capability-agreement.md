# 延后算法：Weighted Kappa / Minitab golden

> 研究日期：2026-08-18
> 状态：Kendall W/τ、两参数指数与三参数对数正态 **公式已实现**（非 Minitab golden）；Weighted Kappa 仍延后。

## 1. Johnson / 非正态能力

| 项 | 状态 |
|---|---|
| Minitab 参考 | [Johnson transformed data](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/methods-and-formulas/johnson-transformed-data/)；[Z-score method](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/supporting-topics/capability-metrics/z-score-method-for-nonnormal-capability/) |
| 领域接口 | `fit_johnson_transform`、`ProcessCapability::calculate_johnson` / `calculate_nonnormal` |
| 实现 | **公式已实现**（Chou 1998 + AD p>0.10；Weibull/Lognormal Z-score Pp/Ppk）。找不到变换或规格越界只输出诊断，不伪造 Pp。 |
| 与 Minitab | **未对齐** — 等待导出后再写入 `VALIDATION_MATRIX.md`；当前测试为 `# source: formula_reference` |
| 所需 golden | 至少 1 组非正态测量 + LSL/USL + Johnson 变换后 Pp/Ppk；另 1 组非正态 Z-score |

## 2. 三参数 Weibull / 两参数指数 / 三参数对数正态

| 项 | 状态 |
|---|---|
| 领域接口 | `fit_weibull3`、`fit_exponential2`、`fit_lognormal3` |
| 实现 | **公式已实现**（剖面似然）。菜单 `weibull3` / `exponential2` / `lognormal3`。无界似然只诊断。 |
| 与 Minitab | **未对齐** — 不实现 Lockhart–Stephens / Minitab bias-correction 数值 |
| 所需 golden | 含阈值的删失数据集 + Minitab 参数表（Shape/Scale/Threshold 或 Location/Scale/Threshold） |

## 3. Fleiss Kappa 与 Kendall

| 项 | 状态 |
|---|---|
| 当前实现 | 两两 `cohen_unweighted`；≥3 评估者 overall `fleiss`；`ratings_are_ordinal=true` 时 Kendall W/τ |
| 配置 | `MsaConfiguration::ratings_are_ordinal` 默认 false；`kappa_weight_scheme` 默认 `"none"` |
| 诊断 | `weighted_kappa_not_implemented`（配置非 `none` 时） |
| Weighted Kappa | **未实现** — Minitab AAA 无 linear/quadratic 加权 Kappa |
| Kendall W/τ | **公式已实现** — 有序评级官方用 Kendall；等待 Minitab 导出 golden |
| 所需 golden | 多评估者表 + Minitab Fleiss overall；有序表 + Kendall |

## 4. 下一批接入条件

Johnson / 非正态 / 三参数 Weibull / 两参数指数 / 三参数对数正态 / Fleiss / Kendall：公式已实现，只需从 Minitab 导出对照后写入 golden，禁止猜测数值。

Weighted Kappa：不作为 Minitab 功能实现。Minitab 无界似然 bias-correction 仍不实现。图表注释与区域拖拽布局仍延后。Bonett / 多重比较区间 / Bartlett / Jackson–Mudholkar T²/Q 解析限仍延后。PCA T²/Q 经验分位、非参数 ties、Levene 中位数口径是公式参考，不是 Minitab golden。
