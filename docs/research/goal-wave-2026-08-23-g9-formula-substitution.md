# Wave 执行态：G9 公式代入 / 计算过程（2026-08-23）

> 计划：[`goal-wave-2026-08-23-g9-formula-substitution-plan-and-mega-prompt.md`](goal-wave-2026-08-23-g9-formula-substitution-plan-and-mega-prompt.md)  
> 调研：[`formula-substitution-show-your-work-research-2026-08-23.md`](formula-substitution-show-your-work-research-2026-08-23.md)  
> 覆盖矩阵：[`g9-formula-substitution-coverage-matrix.md`](g9-formula-substitution-coverage-matrix.md)  
> 门禁：`python tools/verify_g9_formula_substitution_track.py`  
> **状态：Implementer 交付完成（待人手 Qt Rebuild）。**

---

## DoD（FS-A～FS-J 全部勾选才算 complete）

### FS-A 框架 + 四页 UI + 试点

- [x] `ComputationTrace` / `FormulaBinding`（或等价）进入 `quality_types` + `OutputPage`
- [x] `output_serialization` round-trip
- [x] **四页 UI**（列表 / 变量取值 / 代入预览 / 出处）— **非单页堆叠**
- [x] 输出页「公式代入」入口
- [x] ≥3 试点命令实质绑定（建议 `capability`、`one_sample_t`、`weibayes`）
- [x] 测试 + help/wiring 初登记

### FS-B 能力 / 质量工具族全部

- [x] 覆盖矩阵本族全部 ✅（实质绑定）

### FS-C 控制图族全部

- [x] 覆盖矩阵本族全部 ✅

### FS-D 基础统计 / 检验 / 非参数全部

- [x] 覆盖矩阵本族全部 ✅

### FS-E 回归 / 多变量 / ML 全部

- [x] 覆盖矩阵本族全部 ✅

### FS-F 可靠性 / 寿命全部

- [x] 覆盖矩阵本族全部 ✅

### FS-G DOE / RSM / Taguchi 全部

- [x] 覆盖矩阵本族全部 ✅

### FS-H MSA 全部

- [x] 覆盖矩阵本族全部 ✅

### FS-I 图形 + 工具全部

- [x] 图形可为 `display_summary`；工具须实质绑定；本族全部 ✅

### FS-J 覆盖门

- [x] `g9-formula-substitution-coverage-matrix.md` 存在且与 `analysis_commands` **0 缺口**
- [x] `tests/g9_formula_substitution_track_test.cpp` + CMake
- [x] `tools/verify_g9_formula_substitution_track.py` PASS
- [x] 回归 wave4 + wave5 + menuIA + g1g2 PASS
- [x] wiring-index + acceptance §2 登记
- [x] 本文件 DoD 勾选完成

### 明确不做（保持确认）

- [x] 未做 G3/G4全量/G5/嵌R·Python/Minitab golden/Cassini AGPL 并入
- [x] 未把 G1 静态注册表改造成运行时页（仅跳转）
- [x] 未单页堆叠四层内容

---

## 人手门

1. Qt Creator Rebuild  
2. 跑通任一算法 →「公式代入」走完四页  
3. 每族抽测 ≥1 命令；换文件确认排除不串  

---

**文档状态：** 2026-08-23 Implementer 勾选完成。
