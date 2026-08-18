# DataLab 算法与图表缺口对照矩阵

> 研究日期：2026-08-18  
> 访问日期：2026-08-18（UTC+8）  
> 本文只整理现状、官方公式来源和分批验收口径，不填写任何未从 Minitab 导出的对照数值。

## 1. 官方公式与输出口径来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| 正态过程能力 | [Normal Capability methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/normal-capability-analysis/methods-and-formulas/methods/) | 2026-08-18 |
| 回归 ANOVA | [Regression ANOVA methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-regression-model/methods-and-formulas/analysis-of-variance/) | 2026-08-18 |
| 图形编辑 | [Edit graphs](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-editing-basics/edit-graphs/) | 2026-08-18 |
| 图形属性 | [Graph attributes](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/graph-editing-basics/edit-attributes-for-fills-fonts-and-lines/) | 2026-08-18 |
| 数据类型 | [Minitab data types and formats](https://support.minitab.com/en-us/minitab/help-and-how-to/manipulate-data-in-worksheets-columns-and-rows/supporting-topics/data-types-and-arrangements/minitab-data-types-and-formats/) | 2026-08-18 |
| ARIMA | [ARIMA methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/time-series/how-to/arima/methods-and-formulas/methods-and-formulas/) | 2026-08-18 |
| Best ARIMA | [Forecast with Best ARIMA model](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/time-series/how-to/forecast-with-best-arima-model/methods-and-formulas/methods/) | 2026-08-18 |
| 可靠性分类 | [Reliability analyses in Minitab](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/supporting-topics/basics/reliability-analyses-in-minitab/) | 2026-08-18 |
| Kappa | [Kappa statistics for Attribute Agreement Analysis](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/attribute-agreement-analysis/attribute-agreement-analysis/methods-and-formulas/kappa-statistics/) | 2026-08-18 |
| 三参数 Weibull / Fleiss / 图表刻度 | 见 `docs/research/weibull3-fleiss-chart-formulas.md` | 2026-08-18 |
| Kendall / 两参数指数 / 三参数对数正态 / 框选缩放 | 见 `docs/research/kendall-exp2-lognormal3-formulas.md` | 2026-08-18 |
| PCA / 非参数 ties / 等方差 Levene / 刻度 Auto | 见 `docs/research/pca-nonparametric-variance-chart-formulas.md` | 2026-08-18 |
| Logistic HL / IDI / 组间组内能力 / 图属性预览 | 见 `docs/research/logistic-idi-between-within-chart-formulas.md` | 2026-08-18 |

对照数据继续使用 `tests/fixtures/minitab/` 原始文件。只有实际从 Minitab 导出的结果才写入 golden 数值。

## 2. 算法清单状态

状态约定：

- **已有且需核对**：领域计算与服务输出基本齐全，本轮只修边界或解释契约。
- **已有但输出不完整**：计算存在，缺 Minitab 风格表、Facts 或解释。
- **领域层已有但未接入**：`datalab_domain` 已实现，菜单/服务未接线。
- **尚未实现**：本轮只建立接口或诊断，不伪造结果。

| 方法 | 状态 | 输入列 | 缺失/有效 N | 公式要点 | 本轮动作 |
|---|---|---|---|---|---|
| CSV/Excel 导入 | 已有但输出不完整 | 全部列 | `*`/`NA`/`N/A`/`NaN`/`NULL` | RowId、列类型、单元格状态 | 增加 dataset_id、校验、重导入清空 undo/行选择 |
| I-MR / Xbar-R/S / P/NP/C/U / Laney | 已有且需核对 | 测量、子组、阶段 | complete-case + 排除行 | Nelson Test 1–8；R/S/MR 仅 1–4 | Test 7 改为严格 `<σ` |
| 正态能力 / Sixpack | 已有且需核对 | 测量、LSL/USL/Target | N/N* | Cp/Cpk 用 σwithin，Pp/Ppk 用 σoverall | 保持公式；Sixpack 仍只正态；解释不写合格/不合格 |
| 线性回归 | 已有但输出不完整 | 响应 + 预测变量 | complete-case | QR、VIF、Cook、DFITS、内部/删除学生化残差 | Seq/Adj SS golden 测试；ANOVA 项名用原始列名 |
| 单/双因素 ANOVA | 已有且需核对 | 响应 + 因子 | 不可估计项不输出 F/P | RSS 差值、Tukey | 保持；解释走 AnovaFacts |
| 描述统计 / 卡方 / 非参数 | 已有且需核对 | 变量或分类列 | 跳过缺失 | 正态近似、ties 修正 | 表暴露 ties/未调整 P/近似/小样本；Facts 只读 |
| DOE 析因响应 | 已有且需核对 | 编码因子 + 响应 | 跳过非法水平 | 二水平主效应与交互 | 保持 golden 表形 |
| DOE 响应优化 | 领域层已有但未接入 | 同上 + 目标/上下限 | 无协方差时给诊断 | coded ±1 desirability | 菜单、配置、服务页 |
| ARIMA / 季节预测 | 已有但输出不完整 | 时间、数值 | 乱序/重复时间报错 | 乘法 SARIMA CSS；混合 p/q | 诊断 `sarima_css_approximation`（非 Minitab TSERIES）；`arima_trend` 仍为非季节 CSS |
| KM / Weibull / 指数 / 对数正态 | 已有且需核对 | 时间、事件 | 全删失不可识别 | 右删失 MLE；`t_p=exp(μ+σΦ⁻¹(p))` | 菜单 `lognormal` / `weibull3`；比较表仍为二参数三列 |
| PCA | 已有且需核对 | ≥2 数值列 | 整行剔除 | 系数 V；解释率用全部 λ；T²/Q 经验分位 | 系数/得分/异常表与 PcaFacts；解释不写过程合格 |
| Logistic | 已有且需核对 | 二元响应 + 预测变量 | complete-case | IRLS、HL、影响点 | 独立拟合优度表 + LogisticFacts；解释拒绝/未拒绝拟合不足 |
| 个体分布识别 | 已有且需核对 | 单列测量 | 非正值时三族 not_computed | AD 公共核 + 四族二参数 | 命令 `distribution_identification`；不改 capability 默认 |
| 组间/组内能力 | 已有且需核对 | 测量 + 子组列 | 严格子组 | σ_BW = sqrt(σ²_B+σ²_w)；Cp 用 σ_BW | 命令 `between_within_capability`；无子组只诊断 |
| Johnson / 非正态能力 | 公式已实现，等待 Minitab 导出 | 测量、LSL/USL | complete-case | Chou+AD 选 SB/SL/SU；Z-score Pp/Ppk | 找不到变换或规格越界只诊断；无 Cp/Cpk；**不是** Minitab golden |
| 图表主题 | 已有且需核对 | ChartModel.theme_preset | — | default/print/dark 背景与文字网格 | 系列色/CL/LCL/UCL 不被主题覆盖 |
| 三参数 Weibull | 公式已实现，等待 Minitab 导出 | 时间、事件 | 失效≥3 | 剖面似然；β>1；`t_p=λ+α[-ln(1-p)]^(1/β)` | 无界似然只诊断；**不是** Minitab golden |
| Fleiss Kappa | 公式已实现，等待 Minitab 导出 | 评级/部件/评估者 | 空评级不进分母 | ≥3 评估者 overall Fleiss；两两 Cohen | 非 `none` 的加权方案仍诊断 |
| Weighted Kappa / Kendall | 公式已实现，等待 Minitab 导出 | 有序评级 | 空评级不进分母 | Minitab 有序评级用 Kendall W/τ | Kendall 已接入（`ordinal=true`）；Weighted Kappa 仍诊断 |
| 两参数指数 / 三参数对数正态 | 公式已实现，等待 Minitab 导出 | 时间、事件 | 失效≥1 / ≥2 | 剖面 λ；`t_p=λ−θ ln(1-p)` / `λ+exp(μ+σΦ⁻¹(p))` | 菜单 `exponential2` / `lognormal3`；无界只诊断；**不是** Minitab golden |
| 图表框选缩放 | 已有且需核对 | ChartModel x_min/x_max | — | 可选数据刻度 | Shift+拖拽写入刻度；适合窗口清除 |
| 等方差 / Levene | 已有且需核对 | 两列或测量+分组 | 每组 ≥2 | Minitab Levene = 中位数绝对偏差 ANOVA | `levene` 对齐中位数；k 组分组列；不做 Bonett |
| 图表刻度 Auto | 已有且需核对 | ChartModel 可选 min/max | — | Min/Max 分别 Auto | 清除 X/Y 范围；不做注释与 Layout |
| 图表属性预览 | 已有且需核对 | ChartModel | — | 预览右侧；控制图才显示参考线 Tab | 系列色表内点选；`graph_properties_dialog_test` |
| XBAR 图元交互与编辑器 | 已接入且需手工验收 | ChartModel + ChartViewState | 无命中对象只显示图形级菜单 | 延迟 tooltip、点选/框选、图元右键菜单、共享坐标命中 | 右侧 GraphPropertiesPanel；不覆盖图表；不改变分析服务 |

## 3. 数据契约（批次 1）

导入后必须满足：

- `columns`、`column_types`、每行 `cell_states` 宽度一致。
- `row_ids` 与 `rows` 等长且唯一。
- `import_metadata.original_row_count`、`column_count` 与当前表一致。
- `dataset_id` 由源路径、列名和行数派生，便于分析页回显数据集身份。
- `source_path` 对文件导入可追溯；粘贴数据允许为空。
- 原始单元格文本不被算法改写；排除行只作用于分析视图；图上每个点保留原始行号。

重导入文件 B 后必须失效：旧排除行、旧输出页、旧 undo、旧图表行选择。

## 4. 质量主流程（批次 2）

### 4.1 SPC Test 7

Minitab/NIST 口径：连续 15 点位于 1σ **内**，边界使用 `|y-CL| < σ`，等于 1σ 不计入“内”。

现有测试数据 0.4 vs σ=1 在 `<=` 与 `<` 下都会触发；需补“恰好 1σ 不触发”边界测试。

### 4.2 能力指数

```
Cp  = (USL - LSL) / (6σwithin)
Cpk = min((μ - LSL)/(3σwithin), (USL - μ)/(3σwithin))
Pp  = (USL - LSL) / (6σoverall)
Ppk = min((μ - LSL)/(3σoverall), (USL - μ)/(3σoverall))
```

已实现 Target/Cpm、Observed/Expected PPM、单侧规格。本轮不把能力指数解释为合格判定。

### 4.3 回归残差

Minitab 残差图契约：残差-拟合值、残差-顺序、残差-预测变量、正态概率图。  
内部标准化残差与删除学生化残差必须分列；DW 使用输入行顺序。

## 5. 高级算法（批次 3）

- 响应优化：二水平编码空间枚举，输出预测、desirability、最佳组合；缺协方差时给出区间不可用诊断。
- 时间序列：候选模型表保留 AIC/AICc/BIC；补充残差与 MAPE/MASE Facts。
- 可靠性：KM 风险集已有；全删失/无失效不得强行拟合。
- PCA：服务层输出系数、相关载荷、得分、T²/Q 阈值与残差表；解释率用全部特征值；T²/Q 限为经验分位。
- Logistic：完全/准完全分离已有诊断；Hosmer–Lemeshow 仅在分组可行时计算。
- 非参数：表暴露 ties、未调整 P、连续性修正、近似方法和小样本警告；组 Z 已输出。
- 等方差：`levene` 为中位数 Brown–Forsythe；支持测量列 + 分组列。Bonett 仍不做。

## 6. 图表编辑器（批次 4）

参考 Minitab “当前图属性可编辑、图元分组、区域属性与默认设置分离”。本阶段只做结构化属性页：

基本信息 / 坐标轴与网格图例 / 数据系列 / 参考线。

工作模型唯一为 `ChartModel`；确认后写回 `PlotSpec`，预览、PDF、PNG 共用同一转换。`theme_preset` 改变背景/文字/网格配色（default / print / dark），不覆盖用户系列色与控制限颜色。可选 `y_min`/`y_max`、`x_min`/`x_max` 与 `data_region_fill`。Min/Max 可分别 Auto；清除 X/Y 范围清空 optionals。Shift+拖拽可把框写入数据刻度。不实现拖拽布局和注释。

## 7. 分批验收

| 批次 | 自动化 | 手工（中文路径 Qt Creator） |
|---|---|---|
| 0 | 本文档入库 | — |
| 1 | 导入契约测试、重复导入清空状态 | 中文路径 CSV/Excel、BOM、中文列名、导入 A 再导入 B |
| 2 | Test 7 边界、回归 4 图、Facts round-trip | I-MR 恰好 1σ、回归残差概率图 |
| 3 | 响应优化服务测试、PCA 异常表、Logistic HL | DOE 优化菜单、PCA T²/Q、非参数 ties 列 |
| 4 | chart_model round-trip、chart_renderer pixmap | 编辑系列/参考线/字体 Tab 后预览、PDF、复制一致 |
| 5 | `ctest` 全绿、`tools/check_layering.ps1` | 见 `docs/quality-algorithms-acceptance.md` |

构建命令由使用者在 Qt Creator / 非损坏中文路径环境执行：

```
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug -j 8
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug --output-on-failure
powershell -NoProfile -ExecutionPolicy Bypass -File tools/check_layering.ps1
```
