# DataLab 学习中心 — 数据集映射表（v2 锁表）

> **状态（Wave-5）**：本文件按 [`goal-learning-center-pedagogy-upgrade-wave-plan.md`](goal-learning-center-pedagogy-upgrade-wave-plan.md) **整篇重写**。
> 旧 black-belt「约 10 张共享宽表挂几十命令」的 mapping **已作废**，禁止再当施工依据。
> 权威：[`goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md`](goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md) §0/§2。
> 机器源：`tools/learning_data/dataset_mapping.json`（`catalog_version=learning-center-mapping-v2`）。

## 0. 关键与命名

| 项 | 锁定 |
|----|------|
| 产品 catalog | `learning-center-v2`（`META_VERSION` ∩ `kExpectedCatalogVersion` 双写） |
| mapping JSON | `learning-center-mapping-v2` |
| `dataset_id` | **不以** `demo_` 开头 |
| 导入工作表名 | `demo_{dataset_id}` |
| 默认策略 | 一主命令一表；同构共享仅 §3 白名单 8 族 |
| 计划专用+练习集 | **93**（含练习表 `imr_spi_spike_b`） |
| tutorials 并集 | **184** |

### 已作废的旧 10 共享表（禁止残留为 dataset_id / 生成器键 / 测试期望）

`smt_paste_height`, `two_line_thickness`, `paired_rework`, `anova_cavity`, `corr_temp_offset`, `attribute_defect`, `gage_rr_balance`, `doe_factorial_demo`, `reliability_cycles`, `ts_weekly_yield`

## 1. 同构共享白名单（§3；极小族）

| family_id / dataset_id | 服务 command_id | 为何同构 | 埋点仍成立 |
|------------------------|-----------------|----------|------------|
| `msa_crossed_aiag` | `emp_crossed`, `gage_rr` | 交叉 Gage 三角色 measurement/part/operator 相同；规格仅 gage_rr 对话框填写 | 是 |
| `cap_stable_spec` | `capability`, `capability_sixpack` | 同一稳定略偏心单值；规格在对话框；Sixpack 是同一课的诊断包装 | 是 |
| `spc_small_drift` | `cusum`, `ewma` | 单值 Y + 微小持续漂移；禁止复用 I-MR 金标阶跃/尖峰 | 是 |
| `infer_one_sample_mean` | `one_sample_t`, `one_sample_z` | 单列 Y 相对假设均值偏移；已知 σ 只在 z 对话框 | 是 |
| `infer_paired_shift` | `paired_t`, `sign_test`, `wilcoxon_signed_rank` | 配对两列前后差；参数/非参数同一设计 | 是 |
| `infer_two_sample_location` | `mann_whitney`, `two_sample_t` | 两独立样本位置差；方差接近；禁止挂 equivalence / variance_test | 是 |
| `cat_shift_line` | `chi_square`, `cross_tabulation` | 同一张两类别列联表；卡方检验 vs 交叉表展示 | 是 |
| `graph_hist_prob` | `histogram`, `probability_plot` | 同一单变量形状课；禁止扩到 density/ecdf/violin，禁止 I-MR 失控集 | 是 |

## 2. 专用数据集目录（主集 + 练习表）

共 **93** 张。

### `anova_one_cavity`

- **标题**: 单因素三腔位 ANOVA
- **行业**: electronics
- **故事**: 三腔位厚度，腔位3 均值偏低。
- **行数**: 45
- **notes（埋点）**: 埋点：腔位=3（约行31–45）均值约低2μm；期望单因素ANOVA F显著。勿写成过程合格。
- **服务 command_id**: `one_way_anova`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `厚度_um` | response | Y |
| 1 | `腔位` | factor | 1/2/3 |
| 2 | `备注` | note | 腔3偏低 |

### `anova_two_factor`

- **标题**: 双因素腔位×班次 ANOVA
- **行业**: electronics
- **故事**: 腔位与班次两因子；晚班主效应。
- **行数**: 48
- **notes（埋点）**: 埋点：班次=晚班 的行均值上移；腔位效应较小。期望双因素ANOVA 班次主效应显著线索。
- **服务 command_id**: `two_factor_anova`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `厚度_um` | response | Y |
| 1 | `腔位` | factor_a | 左/中/右 |
| 2 | `班次` | factor_b | 早班/晚班 |
| 3 | `备注` | note | 晚班抬高 |

### `c_chart_defect_step`

- **标题**: 回流焊焊点缺陷计数（固定单位台阶）
- **行业**: electronics
- **故事**: 每炉固定检验 1 个标准托盘。前段缺陷数稳定，批26起均值台阶抬高。
- **行数**: 40
- **notes（埋点）**: 埋点：批26（行26）起缺陷数由基线约3抬到约8；期望 C 图后段上移或越 UCL。固定单位，不要当 u 图。UCL≠USL。
- **服务 command_id**: `c_chart`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–40 炉次 |
| 1 | `焊点缺陷数` | defects | 固定托盘内缺陷计数 |
| 2 | `备注` | note | 基线/台阶；不进对话框 |

### `cap_between_within`

- **标题**: 组间/组内能力（子组均值台阶）
- **行业**: electronics
- **故事**: 每子组 n=5。子组12起批均值上移，组内散度仍稳。
- **行数**: 100
- **notes（埋点）**: 埋点：子组12起（约行56）批均值由约100抬到约102；期望组间方差抬高、组内相对稳。子组列必选。
- **服务 command_id**: `between_within_capability`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `子组` | subgroup | 1–20，每组5行 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 基线/组间台阶 |

### `cap_binomial_lots`

- **标题**: 批次不合格品（二项能力）
- **行业**: electronics
- **故事**: 可变检验数下的不合格品。批18起不合格率抬高。
- **行数**: 36
- **notes（埋点）**: 埋点：批18（行18）起不合格率由约2%抬到约7%；检验数50–150可变。期望二项能力/PPM 线索反映后段抬高。勿写成过程合格。
- **服务 command_id**: `binomial_capability`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–36 |
| 1 | `不合格品数` | defectives | 不良件数 |
| 2 | `检验数` | inspected | 可变 n |

### `cap_poisson_counts`

- **标题**: 单位缺陷计数（泊松能力）
- **行业**: electronics
- **故事**: 可变单位数下的缺陷。批16起 DPU 抬高。
- **行数**: 35
- **notes（埋点）**: 埋点：批16（行16）起单位缺陷率由约0.03抬到约0.10；单位数可变。期望泊松能力/DPU 反映后段抬高。
- **服务 command_id**: `poisson_capability`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–35 |
| 1 | `缺陷数` | defects | 缺陷计数 |
| 2 | `单位数` | units | 可变单位 |

### `cap_stable_spec`

- **标题**: 稳定略偏心单值（能力/Sixpack 同构）
- **行业**: electronics
- **故事**: 稳定近正态厚度；均值略偏高使 Cpk<Cp。无片41/55 失控尖峰。服务 capability+capability_sixpack。
- **行数**: 80
- **notes（埋点）**: 埋点：全列稳定、无特殊原因尖峰；均值约101.2μm（目标100），LSL=95 USL=105 时 Cpk 低于 Cp（偏心）。禁止与 I-MR 金标/Box-Cox 偏态集共享。行号仅作顺序，无失控行。
- **服务 command_id**: `capability`, `capability_sixpack`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–80 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 稳定略偏心 |

### `cat_shift_line`

- **标题**: 班次×缺陷类型列联（卡方/交叉表同构）
- **行业**: electronics
- **故事**: 班次与缺陷类型有关联结构。服务 chi_square+cross_tabulation。
- **行数**: 120
- **notes（埋点）**: 埋点：晚班行中「虚焊」比例显著高于早班（频数配方固定）；期望卡方关联显著、交叉表边际差可见。禁止当控制图用。
- **服务 command_id**: `chi_square`, `cross_tabulation`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `班次` | row_category | 早班/晚班 |
| 1 | `缺陷类型` | column_category | 虚焊/偏移/桥连/其他 |
| 2 | `备注` | note | 关联结构 |

### `cochran_three_repeat`

- **标题**: 三方法配对二元（Cochran Q）
- **行业**: electronics
- **故事**: 同一批板用三种目检方法判合格(1)/不合格(0)。方法C 更严。
- **行数**: 36
- **notes（埋点）**: 埋点：方法C 列不合格(0) 比例高于A/B（全表行1–36）；期望 Cochran Q 拒绝三方法一致。需≥3列配对二元。
- **服务 command_id**: `cochran_q`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `板号` | order | 1–36 |
| 1 | `方法A` | binary | 0/1 |
| 2 | `方法B` | binary | 0/1 |
| 3 | `方法C` | binary | 0/1 更严 |

### `corr_temp_offset_y`

- **标题**: 炉温与偏移量相关
- **行业**: electronics
- **故事**: 回流峰值温度与贴片偏移正相关。
- **行数**: 45
- **notes（埋点）**: 埋点：炉温与偏移量近似线性正相关（ρ≈0.7）；约行40–42 有轻微离群但不毁相关。期望 Pearson/Spearman 正相关显著。
- **服务 command_id**: `correlation`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `序号` | order | 1–45 |
| 1 | `炉温_C` | x | 相关X |
| 2 | `偏移_um` | y | 相关Y |
| 3 | `备注` | note | 正相关 |

### `desc_unimodal_stable`

- **标题**: 单峰稳定描述统计
- **行业**: electronics
- **故事**: 近正态单峰厚度，供描述统计课读均值/散度。
- **行数**: 60
- **notes（埋点）**: 埋点：全列近正态单峰、无尖峰剧情；均值约100μm。期望描述统计均值/标准差可读，勿写成过程合格。
- **服务 command_id**: `descriptive`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–60 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 单峰稳定 |

### `dist_id_candidates`

- **标题**: 候选分布识别（近对数正态）
- **行业**: electronics
- **故事**: 正值寿命/厚度候选分布。形状更接近对数正态而非正态。
- **行数**: 80
- **notes（埋点）**: 埋点：近似 lognormal(μ=4.6,σ=0.35) 生成；期望个体分布识别中对数正态/Weibull 优于正态。行无失控尖峰剧情。
- **服务 command_id**: `distribution_identification`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `序号` | order | 1–80 |
| 1 | `测量值` | measurement | 正值 Y |
| 2 | `备注` | note | 候选分布 |

### `dist_skew_boxcox`

- **标题**: 右偏正值厚度（Box-Cox）
- **行业**: electronics
- **故事**: 强右偏正值序列，适合讨论 λ 变换，不是稳定正态能力课。
- **行数**: 60
- **notes（埋点）**: 埋点：全列右偏（近似对数正态）；约行45–48 有若干极大值拉长右尾。期望 Box-Cox 推荐 λ 远离1。禁止与 cap_stable_spec 共享。
- **服务 command_id**: `box_cox`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–60 |
| 1 | `厚度_um` | measurement | 正值 Y |
| 2 | `备注` | note | 右偏/尾部 |

### `doe_factorial_y`

- **标题**: 2因子析因响应表
- **行业**: electronics
- **故事**: 温度×压力 编码 ±1；响应含主效应。
- **行数**: 32
- **notes（埋点）**: 埋点：温度=+1 的行响应系统性抬高；压力效应较弱。2×2×8 重复=32 行。期望效应图温度主效应明显。
- **服务 command_id**: `doe_response`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `温度` | factor | ±1 |
| 1 | `压力` | factor | ±1 |
| 2 | `响应Y` | response | Y |
| 3 | `备注` | note | 温度主效应 |

### `doe_opt_two_resp`

- **标题**: 双响应优化样点
- **行业**: electronics
- **故事**: 两因子两响应；目标冲突弱。
- **行数**: 32
- **notes（埋点）**: 埋点：温度高时 Y1 升、Y2 略降；2×2×8 重复=32 行。期望优化折中区。禁止过程合格。
- **服务 command_id**: `response_optimization`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `温度` | factor | ±1 |
| 1 | `压力` | factor | ±1 |
| 2 | `强度Y1` | response | Y1 |
| 3 | `翘曲Y2` | response | Y2 |
| 4 | `备注` | note | 折中 |

### `equiv_one_near_target`

- **标题**: 单样本近目标等价（TOST）
- **行业**: electronics
- **故事**: 厚度贴近目标100，落在等价窗，不是显著偏移课。
- **行数**: 40
- **notes（埋点）**: 埋点：均值约100.1（近目标）；等价界±1.0时期望等价通过线索。禁止与 infer_one_sample_mean 共享。
- **服务 command_id**: `one_sample_equivalence`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–40 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 近目标 |

### `equiv_paired_near`

- **标题**: 配对近零差等价
- **行业**: electronics
- **故事**: 校准前后差值接近0，落在等价窗。
- **行数**: 35
- **notes（埋点）**: 埋点：校准后−校准前差值约0附近（|差|<0.4主导）；等价界±1.0期望等价线索。禁止与 infer_paired_shift 共享。
- **服务 command_id**: `paired_equivalence`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `件号` | order | 1–35 |
| 1 | `校准前_um` | before | 配对前 |
| 2 | `校准后_um` | after | 配对后 |
| 3 | `备注` | note | 近零差 |

### `equiv_prop_one`

- **标题**: 单比例近目标等价
- **行业**: electronics
- **故事**: 不合格比例贴近目标0.05，落在等价窗内。
- **行数**: 28
- **notes（埋点）**: 埋点：比例约0.05附近（行1–28）；等价界±0.03时期望支持等价线索。勿写成过程合格。
- **服务 command_id**: `one_proportion_equivalence`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–28 |
| 1 | `不合格品数` | events | 事件 |
| 2 | `检验数` | trials | 试验 |
| 3 | `备注` | note | 近目标 |

### `equiv_prop_two`

- **标题**: 两比例近相等价
- **行业**: electronics
- **故事**: 两线不合格比例接近，差落在等价窗。
- **行数**: 24
- **notes（埋点）**: 埋点：两线比例差约0（行1–24）；等价界±0.04期望等价线索。勿写成过程合格。
- **服务 command_id**: `two_proportion_equivalence`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–24 |
| 1 | `A不合格数` | first_events | A事件 |
| 2 | `A检验数` | first_trials | A试验 |
| 3 | `B不合格数` | second_events | B事件 |
| 4 | `B检验数` | second_trials | B试验 |
| 5 | `备注` | note | 近相等 |

### `equiv_ratio_near_one`

- **标题**: 双样本比值近1等价
- **行业**: electronics
- **故事**: 试验列相对参考列比值近1。
- **行数**: 20
- **notes（埋点）**: 埋点：试验/参考比值约1.0附近（行1–20对齐）；等价界对数比窗时期望等价线索。
- **服务 command_id**: `two_sample_equivalence_ratio`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `次序` | order | 1–20 |
| 1 | `试验厚度_um` | sample1 | 试验 |
| 2 | `参考厚度_um` | sample2 | 参考 |
| 3 | `备注` | note | 比近1 |

### `equiv_two_near_equal`

- **标题**: 双样本均值近相等价
- **行业**: electronics
- **故事**: 两线均值接近，差落在等价窗。
- **行数**: 25
- **notes（埋点）**: 埋点：A/B两列均值差约0.1（近相等）；等价界±1.0期望等价。禁止与 infer_two_sample_location 共享。
- **服务 command_id**: `two_sample_equivalence`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `次序` | order | 1–25 |
| 1 | `A线厚度_um` | sample1 | 样本1 |
| 2 | `B线厚度_um` | sample2 | 样本2 |
| 3 | `备注` | note | 近相等 |

### `fishbone_solder_causes`

- **标题**: 焊点不良鱼骨原因清单
- **行业**: electronics
- **故事**: 头脑风暴列出的 5M1E 类别与具体原因，供因果图展示结构。
- **行数**: 24
- **notes（埋点）**: 埋点：类别「方法」下列出「钢网张力未校准」（约行9）；「设备」列「回流峰值偏低」（约行4）。期望鱼骨图按类别分枝。非 SPC 读图课。
- **服务 command_id**: `cause_and_effect`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `类别` | category | 人机料法环测 |
| 1 | `原因` | cause | 具体原因短句 |

### `fisher_small_counts`

- **标题**: 小样本 2×2（Fisher）
- **行业**: electronics
- **故事**: 两线×良/不良，单元格计数小，适合 Fisher 精确。
- **行数**: 28
- **notes（埋点）**: 埋点：行1–12 为A线（其中不良约行11–12）；行13–28 为B线且不良更多（约行21–28）。期望 Fisher 精确检验提示关联。小计数勿硬套大样本卡方结论话术。
- **服务 command_id**: `fisher_exact`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `产线` | factor | A/B |
| 1 | `结果` | outcome | 良/不良 |
| 2 | `备注` | note | 小计数2×2 |

### `friedman_three_treat`

- **标题**: 三处理×区组（Friedman）
- **行业**: electronics
- **故事**: 每块板（区组）接受三种助焊剂处理，测润湿评分。
- **行数**: 36
- **notes（埋点）**: 埋点：处理=C 的响应系统性偏低（全部C行）；期望 Friedman 拒绝处理无差。区组=板号。
- **服务 command_id**: `friedman`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `润湿分` | response | Y |
| 1 | `处理` | treatment | A/B/C |
| 2 | `板号` | block | 区组1–12 |
| 3 | `备注` | note | C偏低 |

### `g_chart_gap_days`

- **标题**: 客户投诉间隔天数（稀有事件 G 图）
- **行业**: electronics
- **故事**: 相邻重大投诉间隔（天）。前段间隔长，事件28起间隔变短。
- **行数**: 36
- **notes（埋点）**: 埋点：事件28（行28）起间隔天数由基线约10–14天缩短到约2–4天；期望 G 图后段点下移（间隔变短=事件更密）。
- **服务 command_id**: `g_chart`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `事件序号` | order | 1–36 |
| 1 | `间隔天数` | measurement | 距上次投诉天数 |
| 2 | `备注` | note | 基线/变密 |

### `genvar_two_var`

- **标题**: 焊盘高宽联合波动（广义方差）
- **行业**: electronics
- **故事**: 每子组 n=5 测高度与宽度。子组18起联合协方差膨胀。
- **行数**: 125
- **notes（埋点）**: 埋点：子组18起（约行86–90 起）高度与宽度联合方差放大；期望 |S| 图后段抬高。子组大小 n=5 > p=2。行按子组连续堆叠。
- **服务 command_id**: `generalized_variance`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `子组` | subgroup | 1–25，每组5行 |
| 1 | `高度_um` | measurement | 变量1 |
| 2 | `宽度_um` | measurement | 变量2 |

### `gof_category_bias`

- **标题**: 类别比例偏离均匀（拟合优度）
- **行业**: electronics
- **故事**: 四类缺陷名义应近似均匀，实际「虚焊」过多。
- **行数**: 100
- **notes（埋点）**: 埋点：类别「虚焊」约占45%（远高于均匀25%）；期望卡方GOF 拒绝均匀。行1–100为分类标签。
- **服务 command_id**: `chi_square_gof`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `缺陷类别` | category | 四类之一 |
| 1 | `备注` | note | 偏离均匀 |

### `graph_area_time`

- **标题**: 按周面积序列
- **行业**: electronics
- **故事**: 周序产量面积图；后段抬升。
- **行数**: 36
- **notes（埋点）**: 埋点：周次≥25（行25起）产量台阶抬高。期望面积图后段抬升。
- **服务 command_id**: `area_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `周次` | time | 1–36 |
| 1 | `产量` | value | Y |
| 2 | `备注` | note | 台阶 |

### `graph_bar_category`

- **标题**: 缺陷类别条形计数
- **行业**: electronics
- **故事**: 缺陷类别频次；虚焊主导。
- **行数**: 80
- **notes（埋点）**: 埋点：全表约80行配方中虚焊约占一半（对照行1起任意抽样可见最高频类）；期望条形最高。禁止过程合格。
- **服务 command_id**: `bar_chart`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `缺陷类别` | category | 类别 |
| 1 | `备注` | note | 频次配方 |

### `graph_bubble_xyz`

- **标题**: 气泡 XYZ
- **行业**: electronics
- **故事**: X/Y 位置 + 尺寸；大泡集中一角。
- **行数**: 36
- **notes（埋点）**: 埋点：行30–36 尺寸列显著更大。期望大气泡聚集。
- **服务 command_id**: `bubble_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `X坐标` | x | X |
| 1 | `Y坐标` | y | Y |
| 2 | `尺寸` | size | 气泡大小 |
| 3 | `备注` | note | 大气泡 |

### `graph_contour_xy`

- **标题**: 响应等值面样点
- **行业**: electronics
- **故事**: X/Y/Z 网格样点；Z 在中心高。
- **行数**: 64
- **notes（埋点）**: 埋点：X≈0,Y≈0 附近 Z 最高（约行28–36）。期望等值线中心峰。
- **服务 command_id**: `contour_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `X因子` | x | X |
| 1 | `Y因子` | y | Y |
| 2 | `响应Z` | z | Z |
| 3 | `备注` | note | 中心峰 |

### `graph_corr_matrix`

- **标题**: 三变量相关矩阵
- **行业**: electronics
- **故事**: A 与 B 强相关，C 较弱。
- **行数**: 40
- **notes（埋点）**: 埋点：全列 A–B 相关约 0.85；A–C 弱。期望相关图 A-B 深色。
- **服务 command_id**: `correlation_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `变量A` | measurement | A |
| 1 | `变量B` | measurement | B |
| 2 | `变量C` | measurement | C |
| 3 | `备注` | note | 相关结构 |

### `graph_density_unimodal`

- **标题**: 单峰密度厚度
- **行业**: electronics
- **故事**: 稳定单峰厚度，专供密度图课（不与 hist/prob 共享）。
- **行数**: 55
- **notes（埋点）**: 埋点：行1–55 单峰集中约100μm；约行40–42 略抬高形成肩部。期望密度峰清晰；禁止已证明正态。
- **服务 command_id**: `density_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–55 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 单峰/肩部 |

### `graph_ecdf_unimodal`

- **标题**: 经验分布厚度
- **行业**: electronics
- **故事**: 单峰厚度供 ECDF 课。
- **行数**: 50
- **notes（埋点）**: 埋点：行45–50 上分位抬高；ECDF 后段变陡。禁止过程合格。
- **服务 command_id**: `ecdf_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–50 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 上分位 |

### `graph_eda4_series`

- **标题**: EDA 四图单列
- **行业**: electronics
- **故事**: 单列时间序；中段漂移供四图诊断。
- **行数**: 48
- **notes（埋点）**: 埋点：行25–36 均值抬高约 +1.2。期望 run/lag 等面板显示非随机。禁止已证明正态。
- **服务 command_id**: `eda_4plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–48 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 中段漂移 |

### `graph_heatmap_matrix`

- **标题**: 热图三变量
- **行业**: electronics
- **故事**: 与相关结构类似的热图课专用表。
- **行数**: 36
- **notes（埋点）**: 埋点：变量A–B 高相关块；行号仅顺序。期望热图对角外 A-B 亮。
- **服务 command_id**: `heatmap_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `变量A` | measurement | A |
| 1 | `变量B` | measurement | B |
| 2 | `变量C` | measurement | C |
| 3 | `备注` | note | 热图块 |

### `graph_hexbin_xy`

- **标题**: 密集 XY Hexbin
- **行业**: electronics
- **故事**: 大量 XY 点云；中心密度高。
- **行数**: 120
- **notes（埋点）**: 埋点：约行1–100 集中椭圆核；行101–120 外缘稀疏散点。期望 hex 中心深色。
- **服务 command_id**: `hexbin_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `X坐标` | x | X |
| 1 | `Y坐标` | y | Y |
| 2 | `备注` | note | 核/外缘 |

### `graph_hist_prob`

- **标题**: 近正态厚度（直方图/概率图同构）
- **行业**: electronics
- **故事**: 单变量厚度，近正态轻微右偏，无 SPC 特殊原因。服务 histogram+probability_plot。
- **行数**: 60
- **notes（埋点）**: 埋点：行48–50 轻微右尾偏高（约+3μm）；全列无片41/55 失控尖峰。期望直方图单峰、概率图大致贴线；禁止已证明正态/过程合格。
- **服务 command_id**: `histogram`, `probability_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–60 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 基线/右尾 |

### `graph_interval_groups`

- **标题**: 三组区间均值
- **行业**: electronics
- **故事**: 腔号三组；腔3 均值偏低。
- **行数**: 45
- **notes（埋点）**: 埋点：腔号=3 的行（约行31–45）均值约低 2μm。期望区间图位置差。
- **服务 command_id**: `interval_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `厚度_um` | measurement | 响应 |
| 1 | `腔号` | category | 1/2/3 |
| 2 | `备注` | note | 腔3偏低 |

### `graph_marginal_xy`

- **标题**: 边际图 XY
- **行业**: electronics
- **故事**: XY 相关 + 边际分布。
- **行数**: 50
- **notes（埋点）**: 埋点：行1–50 正相关；边缘约正态分布。期望边际直方图单峰。
- **服务 command_id**: `marginal_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `X值` | x | X |
| 1 | `Y值` | y | Y |
| 2 | `备注` | note | 相关 |

### `graph_matrix_three`

- **标题**: 矩阵散点三列
- **行业**: electronics
- **故事**: 三连续变量矩阵图。
- **行数**: 40
- **notes（埋点）**: 埋点：列1–2 正斜率；列3 独立噪声。期望矩阵图对应面板相关。
- **服务 command_id**: `matrix_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `厚度_um` | measurement | V1 |
| 1 | `偏移_um` | measurement | V2 |
| 2 | `温度_C` | measurement | V3 |
| 3 | `备注` | note | 面板相关 |

### `graph_mosaic_two_cat`

- **标题**: 班次×缺陷马赛克
- **行业**: electronics
- **故事**: 两分类列；晚班虚焊偏多。
- **行数**: 100
- **notes（埋点）**: 埋点：晚班×虚焊格占比抬高（约100行配方）；期望马赛克格面积差。对照行号仅作顺序，关联在类别组合。
- **服务 command_id**: `mosaic_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `班次` | category | 早/晚 |
| 1 | `缺陷类别` | category | 类型 |
| 2 | `备注` | note | 关联 |

### `graph_parallel_multi`

- **标题**: 平行坐标多变量
- **行业**: electronics
- **故事**: 四指标；差品在后两轴偏低。
- **行数**: 30
- **notes（埋点）**: 埋点：行21–30（备注=差品）在轴3/轴4 系统性偏低。期望平行坐标交叉簇。
- **服务 command_id**: `parallel_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `轴1` | measurement | 指标1 |
| 1 | `轴2` | measurement | 指标2 |
| 2 | `轴3` | measurement | 指标3 |
| 3 | `轴4` | measurement | 指标4 |
| 4 | `备注` | note | 良/差 |

### `graph_pie_category`

- **标题**: 缺陷类别饼图
- **行业**: electronics
- **故事**: 类别份额；虚焊最大片。
- **行数**: 70
- **notes（埋点）**: 埋点：虚焊约 40%+；期望饼图最大扇区。禁止过程合格。
- **服务 command_id**: `pie_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `缺陷类别` | category | 类别 |
| 1 | `备注` | note | 份额 |

### `graph_scatter_xy`

- **标题**: 温度-偏移散点
- **行业**: electronics
- **故事**: 回流温度 vs 偏移；正相关 + 离群。
- **行数**: 45
- **notes（埋点）**: 埋点：行42 偏移尖峰离群；整体正斜率。期望散点趋势+离群可见。
- **服务 command_id**: `scatter_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `温度_C` | x | X |
| 1 | `偏移_um` | y | Y |
| 2 | `备注` | note | 相关/离群 |

### `graph_two_group_box`

- **标题**: 两线箱线对比
- **行业**: electronics
- **故事**: A/B 两线厚度；B 线位置抬高。
- **行数**: 40
- **notes（埋点）**: 埋点：线别=B 的全部行（约行21–40）中位约高 +1.5μm。期望箱线图位置差可见。
- **服务 command_id**: `boxplot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `厚度_um` | measurement | Y |
| 1 | `线别` | group | A/B |
| 2 | `备注` | note | B抬高 |

### `graph_violin_groups`

- **标题**: 两组小提琴形状
- **行业**: electronics
- **故事**: 两组厚度形状对比；B 组更宽。
- **行数**: 48
- **notes（埋点）**: 埋点：线别=B 行（约行25–48）散度更大。期望小提琴宽度差；禁止过程合格。
- **服务 command_id**: `violin_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `厚度_um` | measurement | Y |
| 1 | `线别` | group | A/B |
| 2 | `备注` | note | B更宽 |

### `imr_rs_subgroup_shift`

- **标题**: 子组均值台阶（I-MR-R/S）
- **行业**: electronics
- **故事**: 每批抽5件测厚度。子组16起批均值上移，组内极差仍稳。
- **行数**: 125
- **notes（埋点）**: 埋点：子组16（约行76起）批均值由约100抬到约102；期望 Xbar/I 侧后段上移，R/S 侧不明显乱。子组列必选。
- **服务 command_id**: `imr_rs`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `子组` | subgroup | 1–25，每组5行 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 基线/均值台阶 |

### `imr_spi_shift`

- **标题**: SMT 钢网更换后锡膏高度（阶跃+尖峰）
- **行业**: electronics
- **故事**: 单条 SMT 线 SPI 高度按片序记录。前段基线，片41起钢网更换后均值上移，片55尖峰。仅服务 I-MR 课，不用于能力分析。
- **行数**: 60
- **notes（埋点）**: 埋点：片41（行41）均值阶跃，基线约120μm→更换后约124μm，期望 I 图后段上移；片55（行55）尖峰约132μm，期望相对近期波动不寻常或越 UCL。MR 图在尖峰处应变大。禁止用本失控教学集算 Cpk。
- **服务 command_id**: `imr`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–60 生产顺序 |
| 1 | `锡膏高度_um` | measurement | SPI 高度 Y |
| 2 | `时段备注` | note | 基线 / 钢网更换后 / 尖峰；不进对话框 |

### `imr_spi_spike_b`（**练习表**；非任何 tutorial 主 `dataset_id`）

- **标题**: SMT 锡膏高度独立练习（尖峰行号不同）
- **行业**: electronics
- **故事**: 与金标同构的练习表：片41仍有均值阶跃，尖峰改在片48。供 fade level 2 独立练，不作为任何 tutorial 的主 dataset_id。
- **行数**: 60
- **notes（埋点）**: 练习表埋点：片41（行41）均值阶跃；尖峰在片48（行48），不是金标的片55。导入工作表 demo_imr_spi_spike_b。
- **服务 command_id**: （无主映射；练习或待挂）

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–60 生产顺序 |
| 1 | `锡膏高度_um` | measurement | SPI 高度 Y |
| 2 | `时段备注` | note | 基线 / 钢网更换后 / 尖峰；不进对话框 |

### `infer_one_sample_mean`

- **标题**: 单样本均值偏移（t/z 同构）
- **行业**: electronics
- **故事**: SPI 高度相对目标 120μm 有可检出均值偏移。服务 one_sample_t+one_sample_z。已知 σ 只在 z 对话框。
- **行数**: 50
- **notes（埋点）**: 埋点：全列相对目标120μm 均值约123.0（约+3μm≈+1.5σ，σ≈2）；行1–50均可对照假设均值120。期望单样本t/z 拒绝H0。禁止与等价性近目标表共享。
- **服务 command_id**: `one_sample_t`, `one_sample_z`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–50 |
| 1 | `锡膏高度_um` | measurement | Y |
| 2 | `备注` | note | 相对目标偏移 |

### `infer_paired_shift`

- **标题**: 返工前后配对位置差
- **行业**: electronics
- **故事**: 同一焊点返工前后高度。返工后系统性抬高。服务 paired_t+wilcoxon+sign_test。
- **行数**: 40
- **notes（埋点）**: 埋点：返工后列相对返工前约+2.5μm（行1–40 差值为正主导）；期望配对t/符号秩/符号检验检出位置差。禁止挂配对等价性表。
- **服务 command_id**: `paired_t`, `sign_test`, `wilcoxon_signed_rank`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `焊点号` | order | 1–40 |
| 1 | `返工前_um` | before | 配对前 |
| 2 | `返工后_um` | after | 配对后 |
| 3 | `备注` | note | 返工抬高 |

### `infer_two_sample_location`

- **标题**: 两线独立样本位置差
- **行业**: electronics
- **故事**: A线与B线厚度。B线均值高约1σ、方差接近。服务 two_sample_t+mann_whitney。
- **行数**: 30
- **notes（埋点）**: 埋点：A线列均值约100、B线列约101.2（差约1.0–1.2σ，σ≈1.0）；每行两列并存共30行。期望双样本t/Mann-Whitney 检出位置差。禁止挂方差检验/等价性。
- **服务 command_id**: `mann_whitney`, `two_sample_t`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `次序` | order | 1–30 |
| 1 | `A线厚度_um` | sample1 | 独立样本1 |
| 2 | `B线厚度_um` | sample2 | 独立样本2 |
| 3 | `备注` | note | B线偏高 |

### `kw_three_cavity`

- **标题**: 三腔位非参数位置（KW）
- **行业**: electronics
- **故事**: 三腔位厚度，腔位3 位置偏低（可能非正态）。
- **行数**: 45
- **notes（埋点）**: 埋点：腔位=3 的行（约行31–45）均值偏低；期望 Kruskal-Wallis 检出组间位置差。
- **服务 command_id**: `kruskal_wallis`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `厚度_um` | response | Y |
| 1 | `腔位` | factor | 1/2/3 |
| 2 | `备注` | note | 腔3偏低 |

### `laney_p_overdispersed`

- **标题**: 过离散批次不合格率（Laney P'）
- **行业**: electronics
- **故事**: 可变检验数下的不合格品。额外批间波动使普通 P 限过窄。
- **行数**: 40
- **notes（埋点）**: 埋点：批间额外波动（过离散）；约批22、31不合格率相对普通二项限会假性越界。期望 Laney Sigma Z>1、P' 限更宽。不要与普通 p_chart 共享。
- **服务 command_id**: `laney_p_chart`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–40 |
| 1 | `不合格品数` | defectives | 不良件数 |
| 2 | `检验数` | inspected | 可变 n≈80–200 |

### `laney_u_overdispersed`

- **标题**: 过离散缺陷率（Laney U'）
- **行业**: electronics
- **故事**: 可变单位数下的缺陷计数，批间额外波动。
- **行数**: 40
- **notes（埋点）**: 埋点：过离散缺陷率；约批20、29相对普通 u 限易假性报警。期望 Laney U' 限更宽。不要与普通 u_chart 共享。
- **服务 command_id**: `laney_u_chart`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–40 |
| 1 | `缺陷数` | defects | 缺陷计数 |
| 2 | `单位数` | units | 可变面积/件数 |

### `logit_pass_fail`

- **标题**: 二元通过/失败 Logistic
- **行业**: electronics
- **故事**: 偏移量越大越易失败(1=失败)。
- **行数**: 50
- **notes（埋点）**: 埋点：偏移_um 升高时失败概率抬高（约偏移>8 的行失败居多）；期望 Logistic 系数为正、事件水平=1。逐步默认关。
- **服务 command_id**: `logistic_regression`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `序号` | order | 1–50 |
| 1 | `失败` | response | 0通过/1失败 |
| 2 | `偏移_um` | predictor | 预测变量 |
| 3 | `备注` | note | 偏移↑失败↑ |

### `ma_small_drift`

- **标题**: 贴片厚度小漂移（移动平均专用）
- **行业**: electronics
- **故事**: 与 EWMA 同构信号但独立表：片30起小台阶。不进 spc_small_drift 族。
- **行数**: 55
- **notes（埋点）**: 埋点：片30（行30）起厚度由约50抬到约50.6；期望移动平均曲线后段上移。禁止挂到 spc_small_drift。
- **服务 command_id**: `moving_average`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–55 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 基线/小漂移 |

### `mcnemar_paired_binary`

- **标题**: 配对前后二元（McNemar）
- **行业**: electronics
- **故事**: 培训前后目检判定（1=检出缺陷）。培训后检出率上升。
- **行数**: 40
- **notes（埋点）**: 埋点：培训后=1 且培训前=0 的不一致对多于反向；期望 McNemar 检出边际变化。
- **服务 command_id**: `mcnemar`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `检验员` | order | 1–40 |
| 1 | `培训前` | before | 0/1 |
| 2 | `培训后` | after | 0/1 |
| 3 | `备注` | note | 后检出↑ |

### `mewma_two_var_drift`

- **标题**: 双变量微小联合漂移（MEWMA）
- **行业**: electronics
- **故事**: 长度与宽度同时缓慢漂移。片28起各约 +0.4σ。
- **行数**: 50
- **notes（埋点）**: 埋点：片28（行28）起长度与宽度同时小幅上移；期望 MEWMA 统计量后段爬升。勿与一元 ewma 表混用。
- **服务 command_id**: `mewma`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–50 |
| 1 | `长度_mm` | measurement | 变量1 |
| 2 | `宽度_mm` | measurement | 变量2 |

### `mix_simplex_3`

- **标题**: 三组分混料点
- **行业**: electronics
- **故事**: 三组分和为1；顶点与中心点。
- **行数**: 30
- **notes（埋点）**: 埋点：行1–3 为顶点（单组分≈1）；行4–9 为边中点；行10–30 为内部/扰动点（和≈1）。期望单纯形点落在三角形内。
- **服务 command_id**: `simplex_design_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `组分A` | component | 比例 |
| 1 | `组分B` | component | 比例 |
| 2 | `组分C` | component | 比例 |
| 3 | `备注` | note | 顶点/中心 |

### `mood_two_group`

- **标题**: 两组中位数（Mood）
- **行业**: electronics
- **故事**: 两供应商厚度中位数不同。
- **行数**: 40
- **notes（埋点）**: 埋点：供应商=B 的行（约行21–40）中位数更高；期望 Mood 中位数检验检出组差。
- **服务 command_id**: `mood_median`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `厚度_um` | response | Y |
| 1 | `供应商` | factor | A/B |
| 2 | `备注` | note | B中位高 |

### `msa_crossed_aiag`

- **标题**: 交叉 Gage R&R（10×3×3 AIAG 型）
- **行业**: electronics
- **故事**: 10 个零件×3 名操作员×3 次重复。零件覆盖过程范围；操作员B 有固定正偏倚。服务 gage_rr+emp_crossed。
- **行数**: 90
- **notes（埋点）**: 埋点：操作员B（全部其重复行）测量值系统偏高约+2μm；零件1–10覆盖约98–110μm 过程范围。期望 %GR&R/重复性可分，且操作员方差可见。禁止挂 nested/能力规格列当失控课。行按零件×操作员×重复堆叠。
- **服务 command_id**: `emp_crossed`, `gage_rr`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `零件号` | part | P01–P10 |
| 1 | `操作员` | operator | A/B/C |
| 2 | `测量值_um` | measurement | Y |
| 3 | `重复次` | note | 1–3；不进对话框 |

### `msa_expanded_crossed`

- **标题**: 三因子平衡 Expanded Gage R&R
- **行业**: electronics
- **故事**: 零件×操作员×工装 三因子平衡设计。工装2 抬高测量噪声。
- **行数**: 72
- **notes（埋点）**: 埋点：工装=F2 的全部行（约行25–48）测量噪声放大；期望附加因子方差可见。6零件×2操作员×2工装×3重复=72。禁止与交叉两因子表共享。
- **服务 command_id**: `expanded_gage_rr`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `零件号` | part | P01–P06 |
| 1 | `操作员` | operator | A/B |
| 2 | `工装` | additional | F1/F2 |
| 3 | `测量值_um` | measurement | Y |

### `msa_nested_operator`

- **标题**: 嵌套 Gage R&R（操作员嵌套于零件批）
- **行业**: electronics
- **故事**: 每批零件只由一名操作员测（嵌套）。批内重复可见；操作员不可交叉比较。
- **行数**: 60
- **notes（埋点）**: 埋点：操作员C 负责的零件批（约行41–60）组内重复性更差（σ放大）。嵌套结构：零件→操作员一对多，禁止当交叉表用。
- **服务 command_id**: `nested_gage_rr`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `零件号` | part | N01–N20 |
| 1 | `操作员` | operator | 嵌套于零件批 |
| 2 | `测量值_um` | measurement | Y |

### `msa_type1_ref`

- **标题**: Type 1 量具偏倚（参考件重复）
- **行业**: electronics
- **故事**: 同一参考件重复测量 50 次。参考真值 100μm；本集均值偏高约 +1.5μm。
- **行数**: 50
- **notes（埋点）**: 埋点：全列相对参考值100μm 有约+1.5μm 正偏倚；行1–50 均可对照 Bias。期望 Type1 Cg/偏倚表显示偏倚，不是「量具通过」。禁止写成量具合格结论。
- **服务 command_id**: `msa_type1`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `次序` | order | 1–50 |
| 1 | `测量值_um` | measurement | Y |
| 2 | `备注` | note | 参考件重复 |

### `multi_vari_pos_time`

- **标题**: 位置×时段 Multi-Vari
- **行业**: electronics
- **故事**: 同一厚度在腔位与时段两因子下的分层。后段时段偏高。
- **行数**: 48
- **notes（埋点）**: 埋点：时段=晚班 的行（约行25–48）均值上移；腔位间差异较小。期望 Multi-Vari 显示时段主效应大于腔位。因子列≥2。
- **服务 command_id**: `multi_vari`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `厚度_um` | measurement | Y |
| 1 | `腔位` | factor | 左/中/右 |
| 2 | `时段` | factor | 早班/晚班 |
| 3 | `备注` | note | 基线/晚班抬高 |

### `norm_mild_skew`

- **标题**: 轻度右偏（正态性）
- **行业**: electronics
- **故事**: 轻度右偏正值，正态性检验多拒绝严格正态。
- **行数**: 50
- **notes（埋点）**: 埋点：全列轻度右偏；约行48–50 有较大值拉尾。期望正态性检验 p 偏低——禁止写成「已证明正态」。
- **服务 command_id**: `normality_test`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–50 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 轻度右偏 |

### `np_chart_const_n_step`

- **标题**: 恒定检验数不合格品（NP 图）
- **行业**: electronics
- **故事**: 每批固定检验100件。批21起不合格品数台阶。
- **行数**: 35
- **notes（埋点）**: 埋点：批21（行21）起不合格品数由约3抬到约9（n=100恒定）；期望 NP 图后段上移。禁止与可变 n 的 p_chart 共享。
- **服务 command_id**: `np_chart`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–35 |
| 1 | `不合格品数` | defectives | 不良件数 |
| 2 | `检验数` | inspected | 恒定100 |

### `outlier_one_spike`

- **标题**: 单尖峰异常值
- **行业**: electronics
- **故事**: 近正态序列中一片尖峰。
- **行数**: 40
- **notes（埋点）**: 埋点：片28（行28）尖峰约112相对基线100；期望异常值检验标记该点。禁止「已证明正态」。
- **服务 command_id**: `outlier_test`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–40 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 基线/尖峰 |

### `p_chart_variable_n_step`

- **标题**: 可变检验数不合格率台阶（P 图）
- **行业**: electronics
- **故事**: 检验数随批变化。批22起不合格率台阶；限宽随 n 变。
- **行数**: 36
- **notes（埋点）**: 埋点：批22（行22）起不合格率由约3%抬到约8%；检验数在50–180间变化，期望 p 限随 n 宽窄变化且后段比例上移。禁止与 np 共享。
- **服务 command_id**: `p_chart`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–36 |
| 1 | `不合格品数` | defectives | 不良件数 |
| 2 | `检验数` | inspected | 可变 n |

### `pareto_defect_tail`

- **标题**: 缺陷类别长尾（柏拉图）
- **行业**: electronics
- **故事**: 焊点缺陷类别。虚焊/偏移占绝大多数，其余长尾。
- **行数**: 120
- **notes（埋点）**: 埋点：类别「虚焊」「偏移」合计约前80%计数（行随机但频数固定配方）；期望柏拉图累计到约80%时主要看前2–3类。Other 阈值可试用95。
- **服务 command_id**: `pareto`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `缺陷类别` | category | 分类标签 |
| 1 | `备注` | note | 可选 |

### `pca_three_var`

- **标题**: 三相关变量 PCA
- **行业**: electronics
- **故事**: 三指标共线结构；PC1 解释主变异。
- **行数**: 40
- **notes（埋点）**: 埋点：三列强共线；行35–40 在第三方向略离群。期望 PC1 方差占比高。
- **服务 command_id**: `pca`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `指标X` | measurement | X |
| 1 | `指标Y` | measurement | Y |
| 2 | `指标Z` | measurement | Z |
| 3 | `备注` | note | 共线/离群 |

### `pois_one_count`

- **标题**: 单样本泊松缺陷率
- **行业**: electronics
- **故事**: 各批缺陷数相对假设发生率偏高。
- **行数**: 30
- **notes（埋点）**: 埋点：缺陷率相对假设0.05 偏高（约0.10）；观测长度列可变。期望单泊松率拒绝H0。行1–30。
- **服务 command_id**: `one_poisson_rate`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–30 |
| 1 | `缺陷数` | defects | 计数 |
| 2 | `观测长度` | length | 单位数 |
| 3 | `备注` | note | 率偏高 |

### `pois_two_count`

- **标题**: 双样本泊松率
- **行业**: electronics
- **故事**: 两线缺陷率，B线更高。
- **行数**: 24
- **notes（埋点）**: 埋点：B线缺陷率高于A线（行1–24两侧列）；期望双泊松率比较显著。
- **服务 command_id**: `two_poisson_rate`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–24 |
| 1 | `A缺陷数` | first_events | A事件 |
| 2 | `A观测长度` | first_trials | A长度 |
| 3 | `B缺陷数` | second_events | B事件 |
| 4 | `B观测长度` | second_trials | B长度 |
| 5 | `备注` | note | B率高 |

### `prop_one_lot`

- **标题**: 单比例不合格品
- **行业**: electronics
- **故事**: 各批不合格比例相对假设0.02 偏高。
- **行数**: 30
- **notes（埋点）**: 埋点：不合格率约0.06相对假设0.02偏高；行1–30。期望单比例检验拒绝。
- **服务 command_id**: `one_proportion`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–30 |
| 1 | `不合格品数` | events | 事件 |
| 2 | `检验数` | trials | 试验 |
| 3 | `备注` | note | 比例偏高 |

### `prop_two_line`

- **标题**: 两比例产线差
- **行业**: electronics
- **故事**: B线不合格比例明显高于A线。
- **行数**: 24
- **notes（埋点）**: 埋点：B线不合格率约高4–6个百分点（行1–24）；期望两比例检验拒绝相等。
- **服务 command_id**: `two_proportions`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–24 |
| 1 | `A不合格数` | first_events | A事件 |
| 2 | `A检验数` | first_trials | A试验 |
| 3 | `B不合格数` | second_events | B事件 |
| 4 | `B检验数` | second_trials | B试验 |
| 5 | `备注` | note | B比例高 |

### `regr_temp_strength`

- **标题**: 温度→强度线性回归
- **行业**: electronics
- **故事**: 固化温度升高，剪切强度上升。
- **行数**: 40
- **notes（埋点）**: 埋点：强度对温度正斜率；约行38–40 残差稍大但不毁斜率。期望回归斜率>0、R²中等。第一列响应=强度，其余预测=温度。
- **服务 command_id**: `regression`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `序号` | order | 1–40 |
| 1 | `剪切强度_N` | response | 响应Y（对话框第一列） |
| 2 | `固化温度_C` | predictor | 预测X |
| 3 | `备注` | note | 正斜率 |

### `rel_warranty_counts`

- **标题**: 保修暴露量汇总
- **行业**: electronics
- **故事**: 各批次暴露量；供保修摘要可选列。
- **行数**: 30
- **notes（埋点）**: 埋点：行1–30 暴露量列可求和；对话框仍需填保修窗口与 R(Tw)。期望暴露列优先于标量。
- **服务 command_id**: `reliability_warranty`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批次` | order | 1–30 |
| 1 | `暴露量` | exposure | 台时/件数 |
| 2 | `备注` | note | 暴露 |

### `run_chart_median_trend`

- **标题**: 运行图中位数趋势
- **行业**: electronics
- **故事**: 单值序列。片28起相对中位数同侧偏高游程。
- **行数**: 50
- **notes（埋点）**: 埋点：片28–40（行28–40）连续落在总体中位数上方；期望运行图标记簇/趋势线索。不是控制限课，勿把中位数当 UCL。
- **服务 command_id**: `run_chart`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–50 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 基线/同侧游程 |

### `runs_clustered`

- **标题**: 游程簇集（非随机）
- **行业**: electronics
- **故事**: 相对中位数出现同侧长游程。
- **行数**: 50
- **notes（埋点）**: 埋点：片20–35（行20–35）连续高于中位数一侧；期望游程检验提示非随机/簇集。
- **服务 command_id**: `runs_test`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–50 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 基线/同侧游程 |

### `spc_small_drift`

- **标题**: 贴片厚度微小持续漂移（EWMA/CUSUM 同构）
- **行业**: electronics
- **故事**: 单值厚度序列。前段稳定，片31起约 +0.8μm 小台阶（约 2σ），不是尖峰。服务 ewma+cusum。
- **行数**: 60
- **notes（埋点）**: 埋点：片31（行31）起均值由约100μm 抬到约100.8μm（小漂移）；期望 EWMA/CUSUM 比 Shewhart 更早爬升越界。禁止复用 imr_spi_shift。moving_average 不进本族。
- **服务 command_id**: `cusum`, `ewma`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–60 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 基线/小漂移 |

### `t2_two_var_shift`

- **标题**: 双尺寸联合均值偏移（Hotelling T²）
- **行业**: electronics
- **故事**: 每片测长度与宽度。片36起长度均值台阶，宽度仍稳。
- **行数**: 50
- **notes（埋点）**: 埋点：片36（行36）起长度_mm 均值上移约0.15；期望 T² 后段抬高。宽度列几乎无台阶，用来对比一元图可能漏检联合偏移。
- **服务 command_id**: `hotelling_t2`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–50 |
| 1 | `长度_mm` | measurement | 变量1 |
| 2 | `宽度_mm` | measurement | 变量2 |

### `t_chart_time_interval`

- **标题**: 设备宕机间隔小时（T 图）
- **行业**: electronics
- **故事**: 相邻宕机间隔（小时）。事件25起间隔变短。
- **行数**: 32
- **notes（埋点）**: 埋点：事件25（行25）起间隔由基线约40–60小时缩短到约8–15小时；期望 T 图后段下移。
- **服务 command_id**: `t_chart`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `事件序号` | order | 1–32 |
| 1 | `间隔小时` | measurement | 距上次宕机 |
| 2 | `备注` | note | 基线/变密 |

### `ts_decomp_seasonal`

- **标题**: 季节分解周序列
- **行业**: electronics
- **故事**: 周期=4 的季节+趋势序列。
- **行数**: 48
- **notes（埋点）**: 埋点：季节周期4；行1–48 叠加缓慢上趋势。期望分解季节分量周期≈4。
- **服务 command_id**: `time_series_decomposition`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `周次` | time | 1–48 |
| 1 | `产量` | value | Y |
| 2 | `备注` | note | 季节+趋势 |

### `ts_smooth_weekly`

- **标题**: 平滑用周产量
- **行业**: electronics
- **故事**: 含噪声的周产量；平滑可见趋势。
- **行数**: 40
- **notes（埋点）**: 埋点：行1–40 含噪声上升趋势；期望指数平滑轨迹比原始更顺。禁止过程合格。
- **服务 command_id**: `time_series_smoothing`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `周次` | order | 1–40 |
| 1 | `产量` | measurement | 序列 Y |
| 2 | `备注` | note | 噪声趋势 |

### `ts_weekly_yield_series`

- **标题**: 周产量时间序列图
- **行业**: electronics
- **故事**: 周次+产量；后段抬升。专用 ts 图（不与分解/平滑共享）。
- **行数**: 40
- **notes（埋点）**: 埋点：周次≥30（行30起）产量抬高。期望时间序列图后段上移。
- **服务 command_id**: `time_series_plot`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `周次` | time | 1–40 |
| 1 | `产量` | value | Y |
| 2 | `备注` | note | 后段抬升 |

### `u_chart_variable_unit_step`

- **标题**: 可变单位缺陷率台阶（U 图）
- **行业**: electronics
- **故事**: 单位面积/件数变化。批20起单位缺陷率台阶。
- **行数**: 35
- **notes（埋点）**: 埋点：批20（行20）起缺陷率由约0.04抬到约0.12；单位数可变；期望 u 限随单位数变化且后段上移。禁止与 laney_u 共享。
- **服务 command_id**: `u_chart`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `批号` | order | 1–35 |
| 1 | `缺陷数` | defects | 缺陷计数 |
| 2 | `单位数` | units | 可变单位 |

### `var_two_line_unequal`

- **标题**: 两线方差不等
- **行业**: electronics
- **故事**: B线波动明显大于A线，均值接近。
- **行数**: 25
- **notes（埋点）**: 埋点：B线列σ约A线的2倍（行1–25对齐）；均值接近。期望方差检验拒绝等方差。禁止当位置差课。
- **服务 command_id**: `variance_test`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `次序` | order | 1–25 |
| 1 | `A线厚度_um` | first | 样本1 |
| 2 | `B线厚度_um` | second | 样本2 方差大 |
| 3 | `备注` | note | B方差大 |

### `xbar_r_n5_range_spike`

- **标题**: 子组 n=5 极差尖峰（Xbar-R）
- **行业**: electronics
- **故事**: 每批抽5件。子组12组内极差尖峰；子组20起均值台阶。
- **行数**: 125
- **notes（埋点）**: 埋点：子组12（约行56–60）组内极差尖峰，期望 R 图报警、先勿读该段 Xbar 限；子组20（约行96起）均值台阶，期望 Xbar 后段上移。禁止与 xbar_s 共享。
- **服务 command_id**: `xbar_r`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `子组` | subgroup | 1–25，每组5行 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 基线/极差尖峰/均值台阶 |

### `xbar_s_n8_sd_shift`

- **标题**: 子组 n=8 标准差台阶（Xbar-S）
- **行业**: electronics
- **故事**: 每批抽8件。子组14起组内标准差放大。
- **行数**: 160
- **notes（埋点）**: 埋点：子组14（约行105起）组内σ放大；期望 S 图后段上移。子组大小=8。禁止与 xbar_r（n=5/R）共享。
- **服务 command_id**: `xbar_s`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `子组` | subgroup | 1–20，每组8行 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 基线/σ台阶 |

### `z_mr_short_run`

- **标题**: 短跑多型号标准化（Z-MR）
- **行业**: electronics
- **故事**: 三种产品短跑混排。分组后标准化；型号B后段有台阶。
- **行数**: 48
- **notes（埋点）**: 埋点：行33–40 为型号B且均值相对该型号目标上移；期望 Z 图上对应点抬高。分组列=产品型号。
- **服务 command_id**: `z_mr`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–48 |
| 1 | `尺寸_mm` | measurement | Y |
| 2 | `产品型号` | group | A/B/C 短跑 |

### `zone_chart_runs`

- **标题**: 同侧游程积分（区域图）
- **行业**: electronics
- **故事**: 单值序列。片24–33 连续落在中心线同侧 Zone C/B，积分配分。
- **行数**: 50
- **notes（埋点）**: 埋点：片24–33（行24–33）连续同侧偏高；期望区域图累计分抬高触发警戒。不是尖峰课。UCL≠USL。
- **服务 command_id**: `zone_chart`

| 列序 | 列名 | role_hint | 说明 |
|------|------|-----------|------|
| 0 | `片号` | order | 1–50 |
| 1 | `厚度_um` | measurement | Y |
| 2 | `备注` | note | 基线/同侧游程 |

## 3. command_id → dataset_id 全表（184）

| command_id | dataset_id | 工作表 | 无表原因 |
|------------|------------|--------|----------|
| `accelerated_life` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `acceptance_sampling` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `acf_pacf` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `adf_test` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `analyze_definitive_screening` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `analyze_variability` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `anom` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `anom_attribute` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `area_plot` | `graph_area_time` | `demo_graph_area_time` | — |
| `arima` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `attribute_agreement` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `bar_chart` | `graph_bar_category` | `demo_graph_bar_category` | — |
| `batch_capability` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `best_subsets_regression` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `between_within_capability` | `cap_between_within` | `demo_cap_between_within` | — |
| `binary_doe_probit` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `binary_response_doe` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `binomial_capability` | `cap_binomial_lots` | `demo_cap_binomial_lots` | — |
| `bootstrap_mean` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `bootstrap_two_sample` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `box_cox` | `dist_skew_boxcox` | `demo_dist_skew_boxcox` | — |
| `boxplot` | `graph_two_group_box` | `demo_graph_two_group_box` | — |
| `bubble_plot` | `graph_bubble_xyz` | `demo_graph_bubble_xyz` | — |
| `c_chart` | `c_chart_defect_step` | `demo_c_chart_defect_step` | — |
| `capability` | `cap_stable_spec` | `demo_cap_stable_spec` | — |
| `capability_sixpack` | `cap_stable_spec` | `demo_cap_stable_spec` | — |
| `cart_tree` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `cause_and_effect` | `fishbone_solder_causes` | `demo_fishbone_solder_causes` | — |
| `ccf` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `chi_square` | `cat_shift_line` | `demo_cat_shift_line` | — |
| `chi_square_gof` | `gof_category_bias` | `demo_gof_category_bias` | — |
| `chi_square_mosaic_link` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `cluster_observations` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `cluster_variables` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `cochran_q` | `cochran_three_repeat` | `demo_cochran_three_repeat` | — |
| `contour_plot` | `graph_contour_xy` | `demo_graph_contour_xy` | — |
| `correlation` | `corr_temp_offset_y` | `demo_corr_temp_offset_y` | — |
| `correlation_plot` | `graph_corr_matrix` | `demo_graph_corr_matrix` | — |
| `correlogram` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `cox_counting_process` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `cox_regression` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `cross_tabulation` | `cat_shift_line` | `demo_cat_shift_line` | — |
| `cusum` | `spc_small_drift` | `demo_spc_small_drift` | — |
| `database_import` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `definitive_screening_design` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `density_plot` | `graph_density_unimodal` | `demo_graph_density_unimodal` | — |
| `descriptive` | `desc_unimodal_stable` | `demo_desc_unimodal_stable` | — |
| `discriminant` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `distribution_calculator` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `distribution_identification` | `dist_id_candidates` | `demo_dist_id_candidates` | — |
| `doe_bbd` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `doe_ccd` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `doe_d_optimal` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `doe_factorial` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `doe_plackett_burman` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `doe_response` | `doe_factorial_y` | `demo_doe_factorial_y` | — |
| `dotplot` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `ecdf_plot` | `graph_ecdf_unimodal` | `demo_graph_ecdf_unimodal` | — |
| `eda_4plot` | `graph_eda4_series` | `demo_graph_eda4_series` | — |
| `emp_crossed` | `msa_crossed_aiag` | `demo_msa_crossed_aiag` | — |
| `ewma` | `spc_small_drift` | `demo_spc_small_drift` | — |
| `expanded_gage_rr` | `msa_expanded_crossed` | `demo_msa_expanded_crossed` | — |
| `expanded_gage_unbalanced` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `factor_analysis` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `fine_gray_regression` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `fisher_exact` | `fisher_small_counts` | `demo_fisher_small_counts` | — |
| `friedman` | `friedman_three_treat` | `demo_friedman_three_treat` | — |
| `g_chart` | `g_chart_gap_days` | `demo_g_chart_gap_days` | — |
| `gage_rr` | `msa_crossed_aiag` | `demo_msa_crossed_aiag` | — |
| `general_manova` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `generalized_variance` | `genvar_two_var` | `demo_genvar_two_var` | — |
| `glm_three_factor` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `glm_two_way` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `graph_gallery` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `heatmap_plot` | `graph_heatmap_matrix` | `demo_graph_heatmap_matrix` | — |
| `hexbin_plot` | `graph_hexbin_xy` | `demo_graph_hexbin_xy` | — |
| `histogram` | `graph_hist_prob` | `demo_graph_hist_prob` | — |
| `hotelling_t2` | `t2_two_var_shift` | `demo_t2_two_var_shift` | — |
| `imr` | `imr_spi_shift` | `demo_imr_spi_shift` | — |
| `imr_rs` | `imr_rs_subgroup_shift` | `demo_imr_rs_subgroup_shift` | — |
| `interval_plot` | `graph_interval_groups` | `demo_graph_interval_groups` | — |
| `isolation_forest` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `km_interval` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `kmeans` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `kruskal_wallis` | `kw_three_cavity` | `demo_kw_three_cavity` | — |
| `laney_p_chart` | `laney_p_overdispersed` | `demo_laney_p_overdispersed` | — |
| `laney_u_chart` | `laney_u_overdispersed` | `demo_laney_u_overdispersed` | — |
| `life_data_lognormal` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `life_data_regression` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `logistic_regression` | `logit_pass_fail` | `demo_logit_pass_fail` | — |
| `mann_whitney` | `infer_two_sample_location` | `demo_infer_two_sample_location` | — |
| `manova_one_way` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `marginal_plot` | `graph_marginal_xy` | `demo_graph_marginal_xy` | — |
| `matrix_plot` | `graph_matrix_three` | `demo_graph_matrix_three` | — |
| `mcnemar` | `mcnemar_paired_binary` | `demo_mcnemar_paired_binary` | — |
| `mewma` | `mewma_two_var_drift` | `demo_mewma_two_var_drift` | — |
| `mixed_effects_reml` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `mixture_analyze` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `mixture_design` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `mixture_extreme_vertices_design` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `mixture_process_variable` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `mood_median` | `mood_two_group` | `demo_mood_two_group` | — |
| `mosaic_plot` | `graph_mosaic_two_cat` | `demo_graph_mosaic_two_cat` | — |
| `moving_average` | `ma_small_drift` | `demo_ma_small_drift` | — |
| `msa_type1` | `msa_type1_ref` | `demo_msa_type1_ref` | — |
| `multi_vari` | `multi_vari_pos_time` | `demo_multi_vari_pos_time` | — |
| `multiple_correspondence` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `nested_gage_rr` | `msa_nested_operator` | `demo_msa_nested_operator` | — |
| `nhpp_repairable` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `nominal_logistic` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `nonlinear_regression` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `nonnormal_capability` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `nonparametric_capability` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `normality_test` | `norm_mild_skew` | `demo_norm_mild_skew` | — |
| `np_chart` | `np_chart_const_n_step` | `demo_np_chart_const_n_step` | — |
| `one_poisson_rate` | `pois_one_count` | `demo_pois_one_count` | — |
| `one_proportion` | `prop_one_lot` | `demo_prop_one_lot` | — |
| `one_proportion_equivalence` | `equiv_prop_one` | `demo_equiv_prop_one` | — |
| `one_sample_equivalence` | `equiv_one_near_target` | `demo_equiv_one_near_target` | — |
| `one_sample_t` | `infer_one_sample_mean` | `demo_infer_one_sample_mean` | — |
| `one_sample_z` | `infer_one_sample_mean` | `demo_infer_one_sample_mean` | — |
| `one_way_anova` | `anova_one_cavity` | `demo_anova_one_cavity` | — |
| `ordinal_logistic` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `orthogonal_regression` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `outlier_test` | `outlier_one_spike` | `demo_outlier_one_spike` | — |
| `p_chart` | `p_chart_variable_n_step` | `demo_p_chart_variable_n_step` | — |
| `paired_equivalence` | `equiv_paired_near` | `demo_equiv_paired_near` | — |
| `paired_t` | `infer_paired_shift` | `demo_infer_paired_shift` | — |
| `parallel_plot` | `graph_parallel_multi` | `demo_graph_parallel_multi` | — |
| `pareto` | `pareto_defect_tail` | `demo_pareto_defect_tail` | — |
| `pca` | `pca_three_var` | `demo_pca_three_var` | — |
| `pie_plot` | `graph_pie_category` | `demo_graph_pie_category` | — |
| `pls_regression` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `poisson_capability` | `cap_poisson_counts` | `demo_cap_poisson_counts` | — |
| `poisson_gof` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `poisson_regression` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `probability_plot` | `graph_hist_prob` | `demo_graph_hist_prob` | — |
| `probit_reliability` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `random_forest` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `randomization_test` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `regression` | `regr_temp_strength` | `demo_regr_temp_strength` | — |
| `reliability` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `reliability_test_plan` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `reliability_warranty` | `rel_warranty_counts` | `demo_rel_warranty_counts` | — |
| `report_templates` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `response_optimization` | `doe_opt_two_resp` | `demo_doe_opt_two_resp` | — |
| `rsm_response` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `run_chart` | `run_chart_median_trend` | `demo_run_chart_median_trend` | — |
| `runs_test` | `runs_clustered` | `demo_runs_clustered` | — |
| `scatter_plot` | `graph_scatter_xy` | `demo_graph_scatter_xy` | — |
| `seasonal_forecasting` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `sign_test` | `infer_paired_shift` | `demo_infer_paired_shift` | — |
| `simple_correspondence` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `simplex_design_plot` | `mix_simplex_3` | `demo_mix_simplex_3` | — |
| `special_cause_rules` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `split_plot_analyze` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `split_plot_design` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `stepwise_regression` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `t_chart` | `t_chart_time_interval` | `demo_t_chart_time_interval` | — |
| `t_power` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `taguchi_analyze` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `taguchi_orthogonal_design` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `time_series_decomposition` | `ts_decomp_seasonal` | `demo_ts_decomp_seasonal` | — |
| `time_series_plot` | `ts_weekly_yield_series` | `demo_ts_weekly_yield_series` | — |
| `time_series_smoothing` | `ts_smooth_weekly` | `demo_ts_smooth_weekly` | — |
| `tolerance_intervals` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `trend_analysis` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `two_factor_anova` | `anova_two_factor` | `demo_anova_two_factor` | — |
| `two_poisson_rate` | `pois_two_count` | `demo_pois_two_count` | — |
| `two_proportion_equivalence` | `equiv_prop_two` | `demo_equiv_prop_two` | — |
| `two_proportions` | `prop_two_line` | `demo_prop_two_line` | — |
| `two_sample_equivalence` | `equiv_two_near_equal` | `demo_equiv_two_near_equal` | — |
| `two_sample_equivalence_ratio` | `equiv_ratio_near_one` | `demo_equiv_ratio_near_one` | — |
| `two_sample_t` | `infer_two_sample_location` | `demo_infer_two_sample_location` | — |
| `u_chart` | `u_chart_variable_unit_step` | `demo_u_chart_variable_unit_step` | — |
| `variability_chart` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `variance_test` | `var_two_line_unequal` | `demo_var_two_line_unequal` | — |
| `violin_plot` | `graph_violin_groups` | `demo_graph_violin_groups` | — |
| `weibayes` | （空） | — | 无需导入：公式参考 / 设计生成 / 编排，或本课不依赖演示表 |
| `wilcoxon_signed_rank` | `infer_paired_shift` | `demo_infer_paired_shift` | — |
| `xbar_r` | `xbar_r_n5_range_spike` | `demo_xbar_r_n5_range_spike` | — |
| `xbar_s` | `xbar_s_n8_sd_shift` | `demo_xbar_s_n8_sd_shift` | — |
| `z_mr` | `z_mr_short_run` | `demo_z_mr_short_run` | — |
| `zone_chart` | `zone_chart_runs` | `demo_zone_chart_runs` | — |

## 4. 接入提醒（保持现网，勿推倒）

- 导入仍走 `WorksheetRegistry::import_new` → 工作表名 `demo_{dataset_id}`。
- 嵌入库：`resources/help/learning_center.sqlite`，由 `tools/build_learning_center_db.py` 重建。
- 升级后用户本机再跑 `tools/package_dist.ps1`；catalog 必须为 `learning-center-v2`。
- 详见 `tools/dist_readme.txt`。

