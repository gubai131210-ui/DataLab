# Goal DoD：算法 Wave-8（2026-08-28）

> Orchestrator 在 `/goal` 中逐项勾选；**四项全 `[x]` + verify PASS** 才可 complete。  
> 计划：[`goal-wave-2026-08-28-algorithm-wave8-plan-and-mega-prompt.md`](goal-wave-2026-08-28-algorithm-wave8-plan-and-mega-prompt.md)  
> 调研：[`algorithm-wave8-market-formula-research-2026-08-28.md`](algorithm-wave8-market-formula-research-2026-08-28.md)

## Meta

| 字段 | 值 |
|------|-----|
| Goal | 算法 Wave-8 |
| Verify | `tools/verify_algorithm_wave8_track.py` |
| 测试 | `tests/algorithm_wave8_track_test.cpp` |

## W8-1 `binary_response_doe`

- [x] `docs/research/p8_binary_response_doe.md`（Primary URL + 公式 + 访问日期）
- [x] domain + Facts + AnalysisService + command（menu_path/menu_group）
- [x] Logit IRWLS；events/trials 或 0/1；Coefficients + OR + 拟合诊断
- [x] interpretation + serialization + help
- [x] **独立**多页对话框（≥4 页）
- [x] formula_reference 测试
- [x] wiring + backlog 更新

## W8-2 `cluster_variables`

- [x] `docs/research/p8_cluster_variables.md`
- [x] 相关距离 + 层次连结；dendrogram + amalgamation 表
- [x] 完整竖切 + formula_reference 测试
- [x] UI 变量选择与距离/连结分页
- [x] wiring + backlog

## W8-3 `glm_three_factor`

- [x] `docs/research/p8_glm_three_factor.md`
- [x] 三因子不平衡；Type III Adj SS；Fitted Means；无 ABC 三阶交互
- [x] 完整竖切 + 诊断 + formula_reference 测试
- [x] UI 列选择/模型/方法分页
- [x] wiring + backlog

## W8-4 `life_data_regression`

- [x] `docs/research/p8_life_data_regression.md`
- [x] Weibull MLE + 1～2 协变量；右删失；回归表 + 可选百分位
- [x] 完整竖切 + 测试
- [x] UI 时间/删失/协变量分页
- [x] wiring + backlog

## Wave 门禁

- [x] Planner 映射表已产出
- [x] `verify_algorithm_wave8_track.py` PASS
- [x] 回归 `verify_algorithm_wave7_track.py` PASS
- [x] 回归 `verify_algorithm_wave6_track.py` PASS
- [x] 回归 `verify_ui_menu_ia_track.py` PASS
- [x] Checker 无 Critical（脚本门 + 人工抽查；Bugbot 额度受限）
- [x] 告知用户 Qt Creator Rebuild 手测四点

## 禁止偷懒核对（Checker）

- [x] 无单页堆控件
- [x] 无嵌 Python/R
- [x] 无破坏 A→B / source_row
- [x] 无「见 md」帮助正文
- [x] 无假 golden
- [x] Binary DOE 未并入 `logistic_regression` 对话框
- [x] 三因子 GLM 未并入 `glm_two_way` 对话框
