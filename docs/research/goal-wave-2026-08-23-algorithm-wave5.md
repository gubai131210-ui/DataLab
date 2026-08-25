# Wave 执行态：算法 Wave-5（2026-08-23）

> 计划：[`goal-wave-2026-08-23-algorithm-wave5-plan-and-mega-prompt.md`](goal-wave-2026-08-23-algorithm-wave5-plan-and-mega-prompt.md)  
> 调研：[`algorithm-wave5-market-formula-research-2026-08-23.md`](algorithm-wave5-market-formula-research-2026-08-23.md)  
> 门禁：`python tools/verify_algorithm_wave5_track.py`  
> **状态：✅ 执行完成（DoD 勾选）。**

---

## DoD（W5-1～W5-4 全部勾选才算 complete）

### W5-1 `random_forest`

- [x] `docs/research/p5_random_forest.md`（Primary URL + 访问日期 + 表形）
- [x] domain + Facts + AnalysisService + analysis_commands（menu_path/menu_group）
- [x] interpretation + help + formula_reference 测试
- [x] 披露非 TreeNet / Minitab RF 数值对齐
- [x] backlog / wiring / acceptance 登记

### W5-2 `weibayes`

- [x] `docs/research/p5_weibayes.md`
- [x] domain + Facts + service + commands + interp + help + 测试
- [x] 右删失主路径；少失效窄化边界诚实
- [x] 登记完成

### W5-3 `taguchi_orthogonal_design`

- [x] `docs/research/p5_taguchi_orthogonal_design.md`
- [x] 设计生成（L8/L9/L12 子集）+ 可写入工作表
- [x] 完整竖切 + 导入衔接不破坏 A→B
- [x] 登记完成

### W5-4 `distribution_calculator`

- [x] `docs/research/p5_distribution_calculator.md`
- [x] 正态/t/χ²/F/Weibull：PDF/CDF/分位
- [x] 完整竖切（工具命令或对话框入口）
- [x] 登记完成

### W5 验收门

- [x] `tests/algorithm_wave5_track_test.cpp` + CMake
- [x] `tools/verify_algorithm_wave5_track.py` PASS
- [x] 回归 `verify_algorithm_wave4_track.py` + `verify_ui_menu_ia_track.py` PASS
- [x] 本文件 DoD 勾选完成

### 明确不做（保持确认）

- [x] 未做 TreeNet/AutoML/嵌 R·Python/G3·G4·G5/Minitab golden

---

## 人手门

1. Qt Creator Rebuild  
2. 分别跑通四命令主路径；Taguchi 导出矩阵后换文件确认排除不串  

---

**文档状态：** 执行完成 2026-08-23；DoD 已勾选。
