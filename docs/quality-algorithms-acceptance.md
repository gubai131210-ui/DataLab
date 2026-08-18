# 质量算法增强验收清单

## 自动化证据

- [x] 公式、假设、边界和容差记录在 `docs/research/quality-algorithms-next-stage.md` 与
      `docs/statistical-methodology.md`。
- [x] CSV 导入覆盖 BOM、引号分隔符、跨行引号、短行、缺失值和非法数值。
- [x] 导入结果包含 `RowId`、列类型和 `valid/missing/invalid` 单元格状态。
- [x] 正态概率图排序后仍保留原始行映射。
- [x] 控制图 Test 1–8 分别保留失败点集；Test 8 不要求交替。
- [x] 特殊原因规则可按控制图类型勾选；DataLab 新分析默认全选适用规则，旧 JSON `[1]` 保持只运行 Test 1。
- [x] R/S/MR 仅适用 Test 1–4，EWMA 仅 Test 1，CUSUM 使用上/下侧首次信号。
- [x] 描述统计、能力指标、诊断字段和 OutputPage 方法元数据可 JSON round-trip。
- [x] 跨算法公共契约：`QualityEvidence`、稳定诊断 code、`assumption_status`、
      `parameter_source` 和旧 JSON 安全默认值。
- [x] 过程能力单侧规格、LSL=USL、非有限规格/Target、sigma=0、等于规格限不计 PPM。
- [x] Anderson-Darling 输出 A²/A²*、alpha、reject/fail_to_reject/not_computed；n<8 警告。
- [x] 双因素 ANOVA 使用 RSS 差值，不可估计项不输出伪造 F/P。
- [x] 回归保留 source_row、内部标准化与删除学生化残差、error_df≤0 不输出 t/F/P。
- [x] Gage R&R 保存截断前方差分量、ndc 规则、invalid_tolerance。
- [x] Type 1 零重复性不输出 p=0；Kappa P_expected=1 不可识别；事件编码拒绝未知值。
- [x] 输出统计表支持 TSV 复制和 CSV 导出；图形预览与 PDF 继续复用同一模型。

## 中文路径 Qt Creator 手工验收

在中文目录下用 Qt Creator 打开项目并运行 Debug 构建：

- [ ] 导入中文路径 CSV，确认 BOM、中文列名、空单元格、`*`、`NA/N/A/NaN` 和非法数值
      的提示与行数正确。
- [ ] 导入包含跨行引号字段的 CSV，确认字段没有被拆成额外数据行。
- [ ] 先导入文件 A 并生成分析，再导入文件 B；确认旧排除行、旧输出页、旧行选择全部失效。
- [ ] 运行描述统计、正态性检验、能力 Sixpack，核对 N/N*、StDev、AD、A²*、Cp/Cpk/Pp/Ppk、
      PPM、规格限和“稳定性/假设未验证”提示；确认单侧规格另一侧为空，等于规格限不计超规。
- [ ] 运行 t/ANOVA/Tukey、回归诊断，核对 DF、同时置信区间（含 0 不显著）、效应量、残差图、
      Cook's D、DFITS、内部标准化/删除学生化残差和原始行号。
- [ ] 运行 Gage R&R / Nested / Type 1 / Kappa / 可靠性，核对 ndc、负方差截断提示、
      未知事件编码拒绝、KM 风险集和删失比例。
- [ ] 运行 I-MR、Xbar-R/S、P/NP/C/U/Laney，核对阶段、历史参数、Test 5–8、逐点表和
      悬停原始行号。
- [ ] 打开控制图设置弹窗，确认特殊原因测试默认全选适用规则；取消部分规则后输出只标记
      勾选的测试；R/S/MR 的 Test 5–8 置灰，EWMA 只保留 Test 1，CUSUM 显示专用信号说明。
- [ ] 悬停失败点确认显示完整触发测试集合和原始行号；保存项目后重载，旧 JSON `[1]`
      仍只运行 Test 1，新分析默认全选适用规则。
- [ ] 在统计表上使用右键复制 TSV、导出 CSV；确认只导出当前输出对象。
- [ ] 复制图形、导出 PDF/PNG，确认中文标题、长表头、多图页面、图例和诊断卡片不重叠。
- [ ] 保存项目并重新打开，确认原始数据、排除行、输出页、方法参数和诊断仍可用。

## 明确后续缺口（本轮不实现）

时间序列、DOE 响应优化、PCA、Logistic、非参数检验只记录缺口，除非另行批准扩展：

- 时间序列/SARIMA：完整识别、季节差分与不收敛诊断仍需独立契约。
- DOE：与双因素 ANOVA 相同的不可估计 SS 规则尚未全部下沉到析因响应模型。
- PCA：载荷稳定性、缺失策略和解释层结构化 facts 仍缺。
- Logistic：完全分离、Hosmer-Lemeshow 与影响点尚未纳入本轮公共契约。
- 非参数检验：仍可能通过表头反解析 p 值；未补独立 InterpretationFacts。
- 未实现三参数寿命分布、weighted Kappa、非正态能力指数。

使用 `tests/fixtures/minitab/VALIDATION_MATRIX.md` 的原始数据，在 Minitab 中按同一
配置导出结果后，填写文件映射、DataLab 数值、Minitab 数值、绝对误差、相对误差和
容差。未导出的 Minitab 结果不得填写为猜测值。
