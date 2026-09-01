# Goal DoD：算法 Wave-11（2026-09-01）

> Orchestrator 在 `/goal` 中逐项勾选；**四项全 `[x]` + verify PASS** 才可 complete。  
> 计划：[`goal-wave-2026-09-01-algorithm-wave11-plan-and-mega-prompt.md`](goal-wave-2026-09-01-algorithm-wave11-plan-and-mega-prompt.md)  
> 调研：[`algorithm-wave11-market-formula-research-2026-09-01.md`](algorithm-wave11-market-formula-research-2026-09-01.md)

## Meta

| 字段 | 值 |
|------|-----|
| Goal | 算法 Wave-11 |
| Verify | `tools/verify_algorithm_wave11_track.py` |
| 测试 | `tests/algorithm_wave11_track_test.cpp` |

## W11-1 `simple_correspondence`

- [x] `docs/research/p11_simple_correspondence.md`（Primary URL + 公式 + 访问日期）
- [x] domain + Facts + AnalysisService + command（menu_path/menu_group）
- [x] 2 列分类 → 列联；惯性分解；行/列贡献；1～2 组件
- [x] interpretation + serialization + help
- [x] **独立**多页对话框（≥4 页）；≠ `multiple_correspondence` 对话框
- [x] formula_reference 测试
- [x] wiring + backlog 更新

## W11-2 `multiple_correspondence`

- [x] `docs/research/p11_multiple_correspondence.md`
- [x] 3～6 列分类；指示矩阵；Column Contributions
- [x] 完整竖切 + formula_reference 测试
- [x] **独立**多页对话框（≥4 页）；≠ `simple_correspondence` 对话框
- [x] UI 变量/组件/输出分页
- [x] wiring + backlog

## W11-3 `nonlinear_regression`

- [x] `docs/research/p11_nonlinear_regression.md`
- [x] 内置模型 + GN/LM；Parameter + Summary of Fit
- [x] 完整竖切 + 收敛失败 diagnostic + formula_reference 测试
- [x] UI 数据/模型/算法分页；≠ `linear_regression` 对话框
- [x] wiring + backlog

## W11-4 `split_plot_design`

- [x] `docs/research/p11_split_plot_design.md`
- [x] 2～4 因子、1 HTC；设计矩阵 + Whole plot 列
- [x] 完整竖切 + 与 `split_plot_analyze` 联测路径说明
- [x] UI 因子/HTC/复制分页；≠ `split_plot_analyze` 对话框
- [x] wiring + backlog

## Wave 门禁

- [x] Planner 映射表已产出
- [x] `verify_algorithm_wave11_track.py` PASS
- [x] 回归 `verify_algorithm_wave10_track.py` PASS
- [x] 回归 `verify_algorithm_wave9_track.py` PASS
- [x] 回归 `verify_ui_menu_ia_track.py` PASS
- [x] Checker 无 Critical
- [ ] 告知用户 Qt Creator Rebuild 手测四点

## 禁止偷懒核对（Checker）

- [x] 无单页堆控件
- [x] 无嵌 Python/R
- [x] 无破坏 A→B / source_row
- [x] 无「见 md」帮助正文
- [x] 无假 golden
- [x] SCA 未并入 MCA 对话框
- [x] Nonlinear 未并入 linear 对话框
- [x] Split design 未并入 split_plot_analyze 对话框
