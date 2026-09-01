# Goal DoD：算法 Wave-10（2026-08-31）

> Orchestrator 在 `/goal` 中逐项勾选；**四项全 `[x]` + verify PASS** 才可 complete。  
> 计划：[`goal-wave-2026-08-31-algorithm-wave10-plan-and-mega-prompt.md`](goal-wave-2026-08-31-algorithm-wave10-plan-and-mega-prompt.md)  
> 调研：[`algorithm-wave10-market-formula-research-2026-08-31.md`](algorithm-wave10-market-formula-research-2026-08-31.md)

## Meta

| 字段 | 值 |
|------|-----|
| Goal | 算法 Wave-10 |
| Verify | `tools/verify_algorithm_wave10_track.py` |
| 测试 | `tests/algorithm_wave10_track_test.cpp` |

## W10-1 `general_manova`

- [x] `docs/research/p10_general_manova.md`（Primary URL + 公式 + 访问日期）
- [x] domain + Facts + AnalysisService + command（menu_path/menu_group）
- [x] 2～4 响应 + 1～2 因子 + 可选协变量；Wilks/Pillai/LH/Roy；H/E SSCP
- [x] interpretation + serialization + help
- [x] **独立**多页对话框（≥4 页）；≠ `manova_one_way` 对话框
- [x] formula_reference 测试
- [x] wiring + backlog 更新

## W10-2 `mixed_effects_reml`

- [x] `docs/research/p10_mixed_effects_reml.md`
- [x] 1 随机 + 1～2 固定 + 可选协变量；REML 方差分量
- [x] 完整竖切 + formula_reference 测试
- [x] UI 随机/固定/方法分页
- [x] wiring + backlog

## W10-3 `binary_doe_probit`

- [x] `docs/research/p10_binary_doe_probit.md`
- [x] Probit/Gompit link；IRWLS；Events/Trials
- [x] 完整竖切 + 诊断 + formula_reference 测试
- [x] UI 因子/link/方法分页；≠ `binary_response_doe` 对话框
- [x] wiring + backlog

## W10-4 `life_data_lognormal`

- [x] `docs/research/p10_life_data_lognormal.md`
- [x] Lognormal MLE + 删失；回归表 + 百分位
- [x] 完整竖切 + 测试
- [x] UI 数据/分布/方法分页；≠ `life_data_regression` 对话框
- [x] wiring + backlog

## Wave 门禁

- [x] Planner 映射表已产出
- [x] `verify_algorithm_wave10_track.py` PASS
- [x] 回归 `verify_algorithm_wave9_track.py` PASS
- [x] 回归 `verify_algorithm_wave8_track.py` PASS
- [x] 回归 `verify_ui_menu_ia_track.py` PASS
- [x] Checker 无 Critical
- [x] 告知用户 Qt Creator Rebuild 手测四点

## 禁止偷懒核对（Checker）

- [x] 无单页堆控件
- [x] 无嵌 Python/R
- [x] 无破坏 A→B / source_row
- [x] 无「见 md」帮助正文
- [x] 无假 golden
- [x] General MANOVA 未并入 `manova_one_way` 对话框
- [x] Binary probit 未并入 `binary_response_doe` 对话框
- [x] Life lognormal 未并入 `life_data_regression` 对话框
- [x] Mixed REML 未做成单页 GLM 壳
