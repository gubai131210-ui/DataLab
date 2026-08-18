# 质量算法增强验收清单

## 自动化证据

- [x] 公式、假设、边界和容差记录在 `docs/research/quality-algorithms-next-stage.md` 与
      `docs/statistical-methodology.md`。
- [x] CSV 导入覆盖 BOM、引号分隔符、跨行引号、短行、缺失值和非法数值。
- [x] 导入结果包含 `RowId`、列类型、`valid/missing/invalid`、`dataset_id` 和契约校验。
- [x] 正态概率图排序后仍保留原始行映射。
- [x] 控制图 Test 1–8 分别保留失败点集；Test 7 使用严格 `<σ`；Test 8 不要求交替。
- [x] 特殊原因规则可按控制图类型勾选；DataLab 新分析默认全选适用规则，旧 JSON `[1]` 保持只运行 Test 1。
- [x] R/S/MR 仅适用 Test 1–4，EWMA 仅 Test 1，CUSUM 使用上/下侧首次信号。
- [x] 描述统计、能力指标、诊断字段和 OutputPage 方法元数据可 JSON round-trip。
- [x] 跨算法公共契约：`QualityEvidence`、稳定诊断 code、`assumption_status`、
      `parameter_source` 和旧 JSON 安全默认值。
- [x] 回归/ANOVA/MSA/可靠性输出结构化规则证据、假设状态和不可识别原因；解释层优先读取 Facts。
- [x] 过程能力单侧规格、LSL=USL、非有限规格/Target、sigma=0、等于规格限不计 PPM。
- [x] Anderson-Darling 输出 A²/A²*、alpha、reject/fail_to_reject/not_computed；n<8 警告。
- [x] 双因素 ANOVA 使用 RSS 差值，不可估计项不输出伪造 F/P。
- [x] 回归保留 source_row、内部标准化/学生化/删除学生化残差、error_df≤0 不输出 t/F/P。
- [x] Gage R&R 保存截断前方差分量、ndc 规则、invalid_tolerance。
- [x] Type 1 零重复性不输出 p=0；Kappa P_expected=1 不可识别；事件编码拒绝未知值。
- [x] 输出统计表支持 TSV 复制和 CSV 导出；图形预览与 PDF 继续复用同一模型。
- [x] 图表复制走 `ChartRenderer::render_to_pixmap`，支持 Ctrl+C 与 Edit 菜单路由。
- [x] 工作表 `clear_cells` 可清除超出当前数据范围的网格单元格并 undo。
- [x] 回归 ANOVA 多预测变量输出 Seq SS / Adj SS。
- [x] ARIMA Best 候选网格（p/q≤3,d≤2）；季节预测含乘法 SARIMA CSS 候选（可混合 p/q）与 rolling-origin 表。
- [x] 可靠性可配置百分位寿命表与 Weibull / 指数 / 对数正态分布比较。
- [x] 图表「字体与主题」独立 Tab；`title_font_size` / `axis_font_size` / `theme_preset` round-trip。
- [x] `theme_preset` default/print/dark 改变背景与文字/网格配色；未知 preset 回退 default。
- [x] Johnson 变换（Chou + AD p>0.10）与非正态 Z-score Pp/Ppk（Weibull/Lognormal）；找不到变换只诊断。
- [x] 能力配置 `capability_method` / `nonnormal_distribution` JSON 缺字段时安全默认为 `normal` / `weibull`。
- [x] Minitab golden 脚手架：`golden_loader`、`minitab_numerical_golden_test`（回归 Seq/Adj、ARIMA 候选/预测；golden 缺失时 QSKIP）。
- [x] 导入 A→B 契约：`import_state_reset_test` 验证 `dataset_id` 变更与不串数据。
- [x] 三参数 Weibull（剖面似然，β>1）；无界似然只诊断；菜单 `model=weibull3`。
- [x] Fleiss overall Kappa（≥3 评估者）；两两仍 Cohen；`kappa_weight_scheme != none` 仍诊断。
- [x] 图表复制使用 PNG+图像 MIME；编辑页可选 Y 刻度与数据区填色。
- [x] Kendall W/τ（`ordinal=true`，≥3 数值等级）；默认不计算；解释不写「已证明有序一致」。
- [x] 两参数指数 / 三参数对数正态（剖面似然）；菜单 `exponential2` / `lognormal3`；比较表仍二参数。
- [x] 可选 `x_min`/`x_max`；Shift+框选写入数据刻度；适合窗口清除。
- [x] PCA 系数表（特征向量 V）、相关载荷、得分表、T²/Q 阈值与残差；解释率用全部特征值；PcaFacts 含限；解释不写过程合格。
- [x] 非参数表暴露 ties、未调整 P、连续性修正、近似方法和小样本警告；Kruskal 组 Z；Facts 只读。
- [x] 等方差 `levene` 对齐中位数 Brown–Forsythe；测量+分组 k 组；VarianceFacts；不做 Bonett。
- [x] 图表属性 Min/Max 分别 Auto；清除 X/Y 范围；两侧手动时校验 min < max。
- [x] Logistic 独立「拟合优度」表（HL 卡方/DF/组数/P/状态）；「拟合与残差」含影响点；`LogisticFacts` round-trip。
- [x] 个体分布识别：四族二参数 AD 排序表 + 概率图；命令 `distribution_identification`；不改 `capability_method`。
- [x] 组间/组内能力：命令 `between_within_capability`；Process Data 含 Within/Between/BW/Overall σ；无子组只诊断。
- [x] 图表属性页预览在右侧；非控制图无「参考线」Tab；`graph_properties_dialog_test`。

## 中文路径 Qt Creator 手工验收

在中文目录下用 Qt Creator 打开项目并运行 Debug 构建：

- [ ] 导入中文路径 CSV，确认 BOM、中文列名、空单元格、`*`、`NA/N/A/NaN` 和非法数值
      的提示与行数正确。
- [ ] 导入包含跨行引号字段的 CSV，确认字段没有被拆成额外数据行。
- [ ] 先导入文件 A 并生成分析，再导入文件 B；确认旧排除行、旧输出页、旧 undo、旧行选择全部失效。
- [ ] 运行 DOE 响应优化，核对候选组合、desirability 和最佳组合；缺协方差时区间显示为 *。
- [ ] 编辑图表属性页（系列颜色/线型/参考线/字体 Tab），确认预览、报告、PDF/PNG 与复制一致。
- [ ] 图表 Ctrl+C、右键复制与 PDF 导出视觉一致。
- [ ] 打开任一控制图 → 属性 →「字体与主题」切 打印/深色：背景与文字网格变色；手改系列色后换主题不被冲掉。
- [ ] 工作表 Delete 清除单元格（含空白网格区），已空时状态栏提示；undo 可还原。
- [ ] 多预测变量回归 ANOVA 见 Seq/Adj 分列。
- [ ] ARIMA 候选表 >3 行；季节数据见 SARIMA 混合阶候选与 CSS 诊断文案。
- [ ] 可靠性选 `lognormal`：百分位表、分布比较三行（Weibull / Exponential / Lognormal）；解释不写「寿命服从对数正态」。
- [ ] 可靠性选 `weibull3`：参数表含 Threshold；无界/失效不足只有诊断；解释不写「寿命服从三参数 Weibull」。
- [ ] 可靠性选 `exponential2` / `lognormal3`：参数表含 Threshold；默认指数/对数正态不变；比较表仍三列二参数。
- [ ] 属性一致性 ≥3 评估者见 Fleiss 总表；两两仍 Cohen；解释不写「已证明一致」。
- [ ] `ordinal=true` 且有序等级 ≥3：出现 Kendall 表；两水平或非数值评级只有诊断。
- [ ] 编辑 → 复制图形，或输出页 Ctrl+C，粘到画图/Word；属性页改 Y/X 刻度与数据区填色后预览/复制一致。
- [ ] 统计 > 回归 > 二元 Logistic：n≥20 时见独立「拟合优度」表；n&lt;20 时 HL 状态为未计算；解释含「拒绝/未拒绝拟合不足」且无「模型已充分」。
- [ ] 质量工具 > 个体分布识别：四行 AD 排序 + 概率图；含非正值时 Weibull/指数/对数正态未计算；再开正态能力默认仍为 normal。
- [ ] 质量工具 > 组间/组内过程能力：无子组列被拒；有等量子组见 StDev Within/Between/BW/Overall；Between/Within Capability 表；解释无「合格」。
- [ ] 控制图 → 编辑图形属性：预览在 Tab 右侧且有「参考线」Tab；直方图/散点无「参考线」Tab；改系列色后预览/复制/PDF 一致。
- [ ] 任意图 → 属性：Y/X 的 Min、Max 可分别取消 Auto；清除 Y/X 范围后预览恢复自动刻度。
- [ ] 统计 > 多变量 > 主成分：特征值 Proportion；系数表与相关载荷表；得分表有原始行；T²/Q 有限；解释无「合格」。
- [ ] Mann-Whitney / Wilcoxon / Kruskal：有结数据见 ties 与两种 P；小组见警告；解释无「已证明相同」。
- [ ] 统计 > 假设检验 > 方差检验：两列 Levene（中位数）；测量+分组 k 组；一方差需假设方差；旧项目打开不坏。
- [ ] `PinLength` / `PistonRingDiameter` 先跑正态能力（默认行为不变）；再 Johnson；再非正态 Weibull。对照字段名（Pp/Ppk/Z.LSL），不要对数值。无变换时诊断清楚。
- [ ] 运行描述统计、正态性检验、能力 Sixpack，核对 N/N*、StDev、AD、A²*、Cp/Cpk/Pp/Ppk、
      PPM、规格限和“稳定性/假设未验证”提示；确认单侧规格另一侧为空，等于规格限不计超规。
- [ ] 运行 t/ANOVA/Tukey、回归诊断，核对 DF、同时置信区间（含 0 不显著）、效应量、残差图、
      Cook's D、DFITS、内部标准化/删除学生化残差、原始行号和“规则证据”表。
- [ ] 运行 Gage R&R / Nested / Type 1 / Kappa / 可靠性，核对 ndc、负方差截断提示、
      交互项保留说明、未知事件编码拒绝、KM 风险集和删失比例；确认解释层不写“合格/不合格”。
- [ ] 运行 I-MR、Xbar-R/S、P/NP/C/U/Laney，核对阶段、历史参数、Test 5–8、逐点表和
      悬停原始行号。
- [ ] 打开控制图设置弹窗，确认特殊原因测试默认全选适用规则；取消部分规则后输出只标记
      勾选的测试；R/S/MR 的 Test 5–8 置灰，EWMA 只保留 Test 1，CUSUM 显示专用信号说明。
- [ ] 悬停失败点确认显示完整触发测试集合和原始行号；保存项目后重载，旧 JSON `[1]`
      仍只运行 Test 1，新分析默认全选适用规则。
- [ ] 在统计表上使用右键复制 TSV、导出 CSV；确认只导出当前输出对象。
- [ ] 复制图形、导出 PDF/PNG，确认中文标题、长表头、多图页面、图例和诊断卡片不重叠。
- [ ] 保存项目并重新打开，确认原始数据、排除行、输出页、方法参数和诊断仍可用。
- [ ] 本地运行 `tools/check_layering.ps1`，确认 `ui → application/infrastructure/reporting → domain` 分层未破。

## 明确后续缺口（本轮不实现）

本轮已接入 Kendall W/τ（公式参考）、两参数指数、三参数对数正态、图表数据区 X 刻度与 Shift+框选缩放、PCA 系数/得分/T²Q 表、非参数 ties 暴露、Levene 中位数等方差、图表 Min/Max 分别 Auto。仍未实现（见 [`docs/research/deferred-capability-agreement.md`](research/deferred-capability-agreement.md)）：

- Weighted Kappa（linear/quadratic）。
- Minitab 无界似然 bias-correction 数值对齐。
- 图表拖拽布局、注释系统和多图拼版。
- Kalman 状态空间 MLE；Minitab TSERIES 迭代最小二乘 + back forecast 的数值对齐。
- Bonett / 多重比较区间 / Bartlett；Jackson–Mudholkar T²/Q 解析限。
- 将公式参考测试覆盖为真实 Minitab 导出 golden（Johnson / 非正态 / 对数正态 / SARIMA / Weibull3 / Fleiss / Kendall / exponential2 / lognormal3 / PCA / 非参数 / Levene）。

使用 `tests/fixtures/minitab/VALIDATION_MATRIX.md` 的原始数据，在 Minitab 中按同一
配置导出结果后，填写文件映射、DataLab 数值、Minitab 数值、绝对误差、相对误差和
容差。未导出的 Minitab 结果不得填写为猜测值。
