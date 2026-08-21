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
| 时间序列分解 / 指数平滑 | 见 `docs/research/time-series-decomposition-smoothing-formulas.md` | 2026-08-18 |
| Unusual Observations / Multi-Vari / DOE 独立目标 / 图表选中联动 | 见 `docs/research/unusual-obs-multivari-doe-chart-formulas.md` | 2026-08-18 |
| 回归 Fitted Line 带 / DOE 析因图 / 正态容差区间 / 系列样式 | 见 `docs/research/regression-doe-tolerance-chart-formulas.md` | 2026-08-18 |
| 二项/泊松能力 / DOE 等值线曲面 / ANOVA 区间残差 / 侧栏线型点型 | 见 `docs/research/binomial-poisson-doe-anova-chart-formulas.md` | 2026-08-19 |
| 单比例 / TOST / DOE 残差 4 图 / I-MR-R/S | 见 `docs/research/proportion-tost-doe-residual-imr-rs-formulas.md` | 2026-08-19 |
| 泊松率 / 双因素图 / Winters / Gage 图 | 见 `docs/research/poisson-rate-anova-winters-gage-formulas.md` | 2026-08-19 |
| Nested Gage 图 / ARIMA 明细 / 属性一致图 / 单因素残差直方图 | 见 `docs/research/nested-gage-arima-kappa-anova-formulas.md` | 2026-08-19 |
| Type 1 图 / Bias 图合同 / 两比例 / Box-Cox | 见 `docs/research/type1-bias-twoprop-boxcox-formulas.md` | 2026-08-19 |
| t 区间图 / 描述箱线个体值 / 正态输出 / 相关矩阵散点 | 见 `docs/research/ttest-descriptive-normality-correlation-formulas.md` | 2026-08-19 |
| SPC Test 7 / Test 2–8 边界 / Johnson 概率图 | 见 `docs/research/spc-tests-capability-johnson-formulas.md` | 2026-08-20 |
| 交叉 Gage By Part / Operator×Part | 见 `docs/research/gage-by-part-interaction-formulas.md` | 2026-08-20 |
| Nested Gage By Part | 同上 §2（复用 crossed helper） | 2026-08-20 |

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
| SPC Test 2–8 边界 | 已有且需核对 | 测量、子组 | complete-case + 排除行 | Nelson Test 1–8；R/S/MR 仅 1–4 | Test 7 严格 `<σ`；Test 2–6/8 边界 synthetic + I-MR/Xbar 服务夹具已补 |
| I-MR / I-MR-R/S 输出 | 已接入且需手工验收 | 测量；可选阶段/子组 | complete-case；`source_row` 非 iota | 阶段打断窗口；历史 μ/σ 可覆盖估计 | I-MR 逐点表 + SpcFacts；I-MR-R/S 逐子组表；Xbar-R/S 双图触发测试合并表 + SpcFacts |
| P/NP/C/U / Laney P'/U' | 已接入且需手工验收 | 计数 + 分母（或 C 图单位数） | complete-case；`source_row` | 阶段 `phase_labels` 打断 Test 窗口；Laney Z/MR/Sigma Z | 逐子组表含阶段、触发测试、原始行；Laney 含 Test 1–8 列与 `SpcFacts` |
| EWMA / CUSUM | 已接入且需手工验收 | 单列测量 | complete-case；`source_row` | EWMA 仅 Test 1；CUSUM 累计和 hσ 信号 | 「EWMA/CUSUM 参数」+ 逐点表；CUSUM 全部信号表；`SpcFacts` round-trip |
| DOE 析因响应 | 已有且需核对 | 编码因子 + 响应 | 跳过非法水平 | 二水平主效应与交互；标准化效应 \|t\|；df=0 走 Lenth PSE；编码网格等值线（可选 X/Y，其余 hold 0 或实际单位 hold） | 响应页含 Pareto、2/3 因子立方图、主效应、等值线/静态曲面、残差 4 图；≥4 因子 info 诊断 + `DoeFacts.cube_plot_available=false` |
| 正态能力 / Sixpack | 已接入且需手工验收 | 测量、LSL/USL/Target | N/N* | Cp/Cpk 用 σwithin，Pp/Ppk 用 σoverall；CI：χ² 尺度 + Bissell | Process Data 含 AD；PPM 三列；能力表估计/下限/上限；Sixpack 六图标题；直方图 LSL/USL/Target + Within/Overall；解释不写合格 |
| 线性回归 | 已有且需核对 | 响应 + 预测变量 | complete-case | QR、VIF、Cook、DFITS、内部/删除学生化残差；DW + α=0.05 dL/dU 判定区 | Unusual 表仅列 R/X/I 打标行；单预测变量 Fitted Line 含 CI/PI 带；残差图含 y=0；多预测变量每 X 一张「残差与预测变量」+ `source_row`；`RegressionFacts` 含残差 AD、DW 判定与 plot 计数 |
| 单/双因素 ANOVA | 已有且需核对 | 响应 + 因子 | 不可估计项不输出 F/P | RSS 差值、Tukey；单因素组均值个体 CI 用 pooled MSE；Grouping CLD | 单因素含区间图、残差 4 图、Tukey 下限/上限 + 差值区间图 + Grouping Information；双因素含残差 4 图与交互均值连线 |
| 描述统计 / 卡方 / 非参数 | 已有且需核对 | 变量或分类列 | 跳过缺失 | 正态近似、ties 修正；Kruskal+Dunn 或 Steel–Dwass；Friedman±Nemenyi；Sign；McNemar | 描述页含箱线+个体值图；卡方三表 + 观察频数热图；非参数含 Mann-Whitney/Wilcoxon/Sign/Kruskal/Friedman；Kruskal 默认 Dunn，可选 Steel–Dwass；Friedman 可选 Nemenyi；Mann-Whitney McKean–Ryan CI；McNemar 独立命令 |
| 卡方拟合优度 | 已接入且需手工验收 | 一个分类列；可选期望比例 | complete-case；`*` 计 N* | Pearson χ²；E=pN；DF=k−1 | 命令 `chi_square_gof`；条图；不改关联热图；比例个数错只诊断 |
| G 图 / T 图 | 已接入且需手工验收 | 数值间隔列 | complete-case；`source_row` | 几何 INVCDF−1 / Weibull 分位；默认 Test 1 | 逐点表；不解析事件日期；0 间隔 T 图诊断 `zero_interval_regression_used` |
| t/方差/泊松 功效与样本量 | 已接入且需手工验收 | 不读工作表 | — | 非中心 t / F / 比例；单方差 χ²；双方差 F；泊松率正态近似；表含 Actual Power | `PowerFacts`；功效曲线；`one_poisson_*`/`two_poisson_*`；解释不写样本量足够 |
| Grubbs 异常值检验 | 已接入且需手工验收 | 单列测量 | complete-case；`*` 计入 N* | G=max\|y−ȳ\|/s；P 由 t_{n−2} 反解 | 命令 `outlier_test`；个体值图 `source_row`；不做 Dixon |
| 单/双样本 / 配对 t | 已有且需核对 | 一列或两列 | 双样本按组独立抽列；配对 complete-case | Welch/pooled/配对公式不改；区间图为展示 | 单样本均值区间（须=μ0+差值CI）；双样本组均值个体 CI；配对散点 `source_rows` + 差值区间；`TTestFacts` |
| 正态性检验 | 已有且需核对 | 单列 | N/N*；`*` 计入 N* | AD A²/A²*；未拒绝≠已正态 | 概率图+直方图悬停原始行；`NormalityFacts` |
| 相关 | 已有且需核对 | ≥2 数值列 | complete-case 行主序 | Pearson/Spearman 公式不改 | 矩阵散点 + 两列散点 `source_row`；禁止按索引 zip |
| 单比例检验 | 已接入且需手工验收 | 事件 + 试验 | complete-case 多行求和；`*` 计入 N* | exact=Clopper–Pearson；normal=Wald CI + score z；wilson=Wilson score；agresti_coull=Agresti–Coull | 命令 `one_proportion`；不做 Blaker |
| 两比例检验 | 已接入且需手工验收 | 两组事件 + 试验 | 每组独立 complete-case 多行求和；`*` 计入 N* | 检验 unpooled Wald Z；CI=`wald`（默认）或 `newcombe_wilson`（method=wilson）或 `agresti_coull_diff`（method=agresti_coull）；Fisher | 命令 `two_proportions`；差值区间图；`ProportionFacts.kind=two_sample`；不做 Blaker |
| Type 1 Gage | 已接入且需手工验收 | 测量 + 参考值 | 跳过缺失；`*` 诊断 | Bias t；Cg=Tol/(6s)（全公差，非 Minitab K=20%） | 直方图 Ref/规格 + Run Chart `source_row`；不改 Cg/Cgk |
| Bias/Linearity | 已接入且需手工验收 | 参考列 + 测量列 | complete-case | OLS bias~reference；均值 CI 带；可选过程变差 | 散点+拟合+CI；可选 Linearity/%Linearity/%Bias 表；`process_variation`=6σ |
| Box-Cox | 已接入且需手工验收 | 正值列 | 跳过缺失；非正值报错 | 网格最小化 SD(W)；可圆整 λ | λ–SD 图 + 变换前/后概率图；`BoxCoxFacts` |
| 1/2-Sample Poisson Rate | 已接入且需手工验收 | 缺陷 + 观测长度 | 1-sample 多行求和；2-sample 每组一行；`*` 计入 missing | λ̂=x/t；exact=Garwood+泊松尾 / 条件二项；normal=score/Wald；`comparison=ratio`→ρ=λ1/λ2 log-Wald | 命令 `one_poisson_rate` / `two_poisson_rate`；默认差值兼容；不做 Blaker；功效见 `t_power` `one_poisson_*`/`two_poisson_*` |
| 单/双样本 / 配对 / 比例 / 均值比 TOST | 已接入且需手工验收 | 一列、两列、配对、事件/试验或检验+参考 | 缺失跳过并计 N*；配对/比例走 complete-case | t1/t2、Wald z 或均值比 Fieller / 对数几何比；100(1−2α)% CI；within_limits⇔p1≤α且p2≤α | 命令含 `two_sample_equivalence_ratio`（`transform=none|log`；ρ̂/ρ̂_g 存 `difference`） |
| DOE 响应优化 | 已接入且需手工验收 | 因子列 + 1～N 响应列 | 无协方差时给诊断 | coded ±1 desirability；多响应几何平均 D；区间走 `MSE * (X'X)^-1` | 每响应独立 goal/权重已从 UI 写入 `optimization_objectives`；单响应旧字段兼容；缺协方差时 CI/PI=`*` |
| ARIMA / 季节预测 | 已有且需核对（Winters 输出已补；ARIMA 明细已补） | 时间、数值 | 乱序/重复时间报错；缺失计 `missing_values` | Winters 用户给定 α/β/γ；乘法 SARIMA CSS；ARIMA 候选 CSS | ARIMA「拟合与预测明细」含原始行；季节指数表；`holt_winters_additive`/`holt_winters_multiplicative`；SARIMA 仍非 TSERIES golden |
| 交叉 Gage R&R | 已接入且需手工验收 | 测量 + 零件 + 操作员 | 平衡设计；complete-case | ANOVA 分量；ndc=floor(1.41×Part/Gage)；负方差截断；%Tolerance=StudyVar/Tol×100 | 表不变；%Contribution + **%Study Var** + **%Tolerance** 条 + **Gage Run Chart** + **按零件 Xbar-R** + By Part + 交互图；无公差不出 `%Tolerance` 图 |
| Nested Gage R&R | 已接入且需手工验收 | 测量 + 零件 + 操作者（零件嵌套） | 平衡；complete-case；`*` 跳过 | 同 ndc/截断口径；Reproducibility=Operator；%Tolerance 不可用时输出 `*` | %Contribution + **%Study Var** + 条件式 **%Tolerance** 条 + 按操作者 Xbar-R + **By Part**；不改 ndc/截断；不做 Operator×Part 交互 |
| KM / Weibull / 指数 / 对数正态 | 已有且需核对 | 时间、事件 | 全删失不可识别 | 右删失 MLE；`t_p=exp(μ+σΦ⁻¹(p))` | 菜单 `lognormal` / `weibull3`；比较表仍为二参数三列；三参数族补生存/概率图（无 golden） |
| PCA | 已有且需核对 | ≥2 数值列 | 整行剔除 | 系数 V；解释率用全部 λ；T²/Q 经验分位 | 系数/得分/异常表与 PcaFacts（含残差 AD p）；解释不写过程合格 |
| Logistic | 已有且需核对 | 二元响应 + 预测变量 | complete-case | IRLS、HL、影响点 | 独立拟合优度表 + LogisticFacts；解释拒绝/未拒绝拟合不足 |
| 个体分布识别 | 已有且需核对 | 单列测量 | 非正值时三族 not_computed | AD 公共核 + 四族二参数 | 命令 `distribution_identification`；不改 capability 默认 |
| 组间/组内能力 | 已有且需核对 | 测量 + 子组列 | 严格子组 | σ_BW = sqrt(σ²_B+σ²_w)；Cp 用 σ_BW；组内 n≤8 用 R̄/d2、n≥9 用 S̄/c4（与 I-MR-R/S 同口径） | 命令 `between_within_capability`；无子组只诊断 |
| 二项过程能力 | 已接入且需手工验收 | 不合格品 + 检验数 | complete-case；`*` 计入 N* | p̄=Dtot/Ntot；Clopper–Pearson；Process Z=Φ⁻¹(1−p̄) | 命令 `binomial_capability`；Sixpack 仍只正态；无 Cp/Cpk |
| 泊松过程能力 | 已接入且需手工验收 | 缺陷数 + 单位数 | complete-case；`*` 计入 N* | Mean DPU=Dtot/Ntot；Garwood χ² CI | 命令 `poisson_capability` |
| Multi-Vari 图 | 已接入且需手工验收 | 测量 + 2～4 因子列 | complete-case；`*`/空计入 missing | 单元算术平均；覆盖率 <60% 只诊断不画图 | 命令 `multi_vari`；第 4 因子布局见 `p1_multi_vari_fourth_factor.md` |
| 正态容差区间 | 已接入且需手工验收 | 测量列 | N/N*；`*` 计入 N* | Howe 双侧 k₂；Natrella 单侧 k₁ | 命令 `tolerance_intervals`；诊断 Howe/Natrella 近似；解释不写合格/规格已覆盖 |
| Johnson / 非正态能力 | 已接入且需手工验收 | 测量、LSL/USL | complete-case | Chou+AD 选 SB/SL/SU；Z-score Pp/Ppk | Johnson 变换表 + Overall 指数 + **变换后正态概率图**（失败不出图）；非正态分布参数表；无 Within Cp/Cpk；**不是** Minitab golden |
| 图表主题 | 已有且需核对 | ChartModel.theme_preset | — | default/print/dark 背景与文字网格 | 系列色/CL/LCL/UCL 不被主题覆盖 |
| 三参数 Weibull | 公式已实现，等待 Minitab 导出 | 时间、事件 | 失效≥3 | 剖面似然；β>1；`t_p=λ+α[-ln(1-p)]^(1/β)` | 无界似然只诊断；**不是** Minitab golden |
| Fleiss Kappa | 公式已实现，等待 Minitab 导出 | 评级/部件/评估者 | 空评级不进分母 | ≥3 评估者 overall Fleiss；两两 Cohen | 热图+评估者条已接入；Fleiss overall 始终未加权 |
| Weighted Kappa / Kendall | 已接入（公式参考） | 有序评级 | 空评级不进分母 | Cohen linear/quadratic；Minitab 有序用 Kendall W/τ | Kendall（`ordinal=true`）；Weighted Kappa=`kappa_weight_scheme`；**不是** Minitab AAA golden |
| 两参数指数 / 三参数对数正态 | 公式已实现，等待 Minitab 导出 | 时间、事件 | 失效≥1 / ≥2 | 剖面 λ；`t_p=λ−θ ln(1-p)` / `λ+exp(μ+σΦ⁻¹(p))` | 菜单 `exponential2` / `lognormal3`；无界只诊断；**不是** Minitab golden |
| 图表框选缩放 | 已有且需核对 | ChartModel x_min/x_max | — | 可选数据刻度 | Shift+拖拽写入刻度；适合窗口清除 |
| 等方差 / Levene / Bonett / Bartlett | 已有且需核对 | 两列或测量+分组 | 每组 ≥2 | Minitab Levene = 中位数绝对偏差 ANOVA；Bonett = 两样本 SD 比；Bartlett = k 组 χ² | `levene` 对齐中位数；`bonett` 两样本；`bartlett` k≥2；k 组 Bonett 只诊断 |
| 图表刻度 Auto | 已有且需核对 | ChartModel 可选 min/max | — | Min/Max 分别 Auto | 清除 X/Y 范围；不做注释与 Layout |
| 图表属性预览 | 已有且需核对 | ChartModel | — | 预览右侧；控制图才显示参考线 Tab | 系列表显示 `series.label`；`graph_properties_dialog_test` |
| XBAR 图元交互与编辑器 | 已接入且需手工验收 | ChartModel + ChartViewState | 无命中对象只显示图形级菜单 | 延迟 tooltip、点选/框选、图元右键菜单、共享坐标命中 | 右侧 GraphPropertiesPanel 选中联动；选中系列写回 `series[].style.color/line_width/line_style/point_style`；系列列表来自 `ChartModel.series[].label` |

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

### 4.1 SPC Test 1–8

Minitab/NIST 口径见 `docs/research/spc-tests-capability-johnson-formulas.md` §2。

Test 7：连续 15 点位于 1σ **内**，边界 `|y-CL| < σ`（已完成）。

Test 2–6、Test 8：每 Test 独立边界 synthetic + I-MR 服务夹具；Test 8 同侧全 `>1σ` 触发、恰 `1σ` 不触发。

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
单预测变量追加 Fitted Line（观测 + 拟合线 + CI/PI 带）；残差散点含 y=0 参考线。多预测变量不画 2D Fitted Line，但为每个预测变量输出「残差与预测变量」散点（complete-case `source_row`）。

## 5. 高级算法（批次 3）

- 响应优化：二水平编码空间枚举，输出预测、desirability、最佳组合；缺协方差时给出区间不可用诊断。多响应时按几何平均合成总体 D，并输出「响应目标」与各响应 D 列。
- 时间序列：候选模型表保留 AIC/AICc/BIC；补充残差与 MAPE/MASE Facts。
- 可靠性：KM 风险集已有；全删失/无失效不得强行拟合。
- PCA：服务层输出系数、相关载荷、得分、T²/Q 阈值与残差表；解释率用全部特征值；T²/Q 限为经验分位。
- Logistic：完全/准完全分离已有诊断；Hosmer–Lemeshow 仅在分组可行时计算。
- 非参数：表暴露 ties、未调整 P、连续性修正、近似方法和小样本警告；组 Z 已输出。
- 等方差：`levene` 为中位数 Brown–Forsythe；支持测量列 + 分组列。两样本可选 `bonett`（公式参考）。k≥2 可选 `bartlett`（正态敏感）。

## 6. 图表编辑器（批次 4）

参考 Minitab “当前图属性可编辑、图元分组、区域属性与默认设置分离”。本阶段只做结构化属性页：

基本信息 / 坐标轴与网格图例 / 数据系列 / 参考线。

工作模型唯一为 `ChartModel`；确认后写回 `PlotSpec`，预览、PDF、PNG 共用同一转换。`theme_preset` 改变背景/文字/网格配色（default / print / dark），不覆盖用户系列色与控制限颜色。可选 `y_min`/`y_max`、`x_min`/`x_max` 与 `data_region_fill`。Min/Max 可分别 Auto；清除 X/Y 范围清空 optionals。Shift+拖拽可把框写入数据刻度。右键「编辑图形… / 编辑当前对象…」进入非覆盖编辑：图表左侧、`GraphPropertiesPanel` 右侧；`element_selected` 更新 `set_selected_path` 与系列列表。选中「数据系列 / …」时侧栏写回该系列颜色、线宽、线型与点样式。侧栏补齐 X/Y Min/Max 分别 Auto 与标题/轴/图例字号，写回同一 `ChartModel`。完整字体/刻度/系列色仍走 `GraphPropertiesDialog`（预览在 Tabs 右侧）。复制走 `ChartRenderer::render_to_pixmap`（空尺寸回退 640×480，剪贴板 image + PNG）。工作表空串 Display 为空白，显式 `*`/`NA` 仍显示 `*`。不实现拖拽布局和注释。

## 7. 分批验收

| 批次 | 自动化 | 手工（中文路径 Qt Creator） |
|---|---|---|
| 0 | 本文档入库 | — |
| 1 | 导入契约测试、重复导入清空状态 | 中文路径 CSV/Excel、BOM、中文列名、导入 A 再导入 B |
| 2 | Test 7 边界、回归 4 图、Facts round-trip | I-MR 恰好 1σ、回归残差概率图 |
| 3 | 响应优化服务测试、PCA 异常表、Logistic HL | DOE 优化菜单、PCA T²/Q、非参数 ties 列 |
| 4 | chart_model round-trip、chart_renderer pixmap、属性页选中联动、系列色/线宽/线型/点型写回、侧栏 Auto/字号 | 侧栏改 Auto/字号/主题后预览、PDF、复制一致；右侧面板不遮挡 |
| 5 | `ctest` 全绿、`tools/check_layering.ps1` | 见 `docs/quality-algorithms-acceptance.md` |

构建命令由使用者在 Qt Creator / 非损坏中文路径环境执行：

```
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug -j 8
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug --output-on-failure
powershell -NoProfile -ExecutionPolicy Bypass -File tools/check_layering.ps1
```
