# Wave 执行态：算法 Wave-5（2026-08-23）

> 计划：[`goal-wave-2026-08-23-algorithm-wave5-plan-and-mega-prompt.md`](goal-wave-2026-08-23-algorithm-wave5-plan-and-mega-prompt.md)  
> 调研：[`algorithm-wave5-market-formula-research-2026-08-23.md`](algorithm-wave5-market-formula-research-2026-08-23.md)  
> 门禁：`python tools/verify_algorithm_wave5_track.py`  
> **状态：待 `/goal` 执行；下列 DoD 由执行 agent 勾选。**

---

## DoD（W5-1～W5-4 全部勾选才算 complete）

### W5-1 `random_forest`

- [ ] `docs/research/p5_random_forest.md`（Primary URL + 访问日期 + 表形）
- [ ] domain + Facts + AnalysisService + analysis_commands（menu_path/menu_group）
- [ ] interpretation + help + formula_reference 测试
- [ ] 披露非 TreeNet / Minitab RF 数值对齐
- [ ] backlog / wiring / acceptance 登记

### W5-2 `weibayes`

- [ ] `docs/research/p5_weibayes.md`
- [ ] domain + Facts + service + commands + interp + help + 测试
- [ ] 右删失主路径；少失效窄化边界诚实
- [ ] 登记完成

### W5-3 `taguchi_orthogonal_design`

- [ ] `docs/research/p5_taguchi_orthogonal_design.md`
- [ ] 设计生成（L8/L9/L12 子集）+ 可写入工作表
- [ ] 完整竖切 + 导入衔接不破坏 A→B
- [ ] 登记完成

### W5-4 `distribution_calculator`

- [ ] `docs/research/p5_distribution_calculator.md`
- [ ] 正态/t/χ²/F/Weibull：PDF/CDF/分位
- [ ] 完整竖切（工具命令或对话框入口）
- [ ] 登记完成

### W5 验收门

- [ ] `tests/algorithm_wave5_track_test.cpp` + CMake
- [ ] `tools/verify_algorithm_wave5_track.py` PASS
- [ ] 回归 `verify_algorithm_wave4_track.py` + `verify_ui_menu_ia_track.py` PASS
- [ ] 本文件 DoD 勾选完成

### 明确不做（保持确认）

- [ ] 未做 TreeNet/AutoML/嵌 R·Python/G3·G4·G5/Minitab golden

---

## 人手门

1. Qt Creator Rebuild  
2. 分别跑通四命令主路径；Taguchi 导出矩阵后换文件确认排除不串  

---

**文档状态：** 骨架 2026-08-23；执行后勾选。
