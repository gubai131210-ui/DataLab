# Goal DoD：算法 Wave-6（2026-08-25）

> Orchestrator 在 `/goal` 中逐项勾选；**四项全 `[x]` + verify PASS** 才可 complete。  
> 计划：[`goal-wave-2026-08-25-algorithm-wave6-plan-and-mega-prompt.md`](goal-wave-2026-08-25-algorithm-wave6-plan-and-mega-prompt.md)  
> 调研：[`algorithm-wave6-market-formula-research-2026-08-25.md`](algorithm-wave6-market-formula-research-2026-08-25.md)

## Meta

| 字段 | 值 |
|------|-----|
| Goal | 算法 Wave-6 |
| Verify | `tools/verify_algorithm_wave6_track.py` |
| 测试 | `tests/algorithm_wave6_track_test.cpp` |

## W6-1 `taguchi_analyze`

- [x] `docs/research/p6_taguchi_analyze.md`（Primary URL + 公式 + 访问日期）
- [x] domain + Facts + AnalysisService + command（menu_path/menu_group）
- [x] interpretation + serialization + help
- [x] ≥2 种 S/N；响应表 Delta/Rank；主效应图或显式无图说明
- [x] 独立多页对话框（非设计生成对话框加勾选）
- [x] formula_reference 测试
- [x] wiring + backlog 更新

## W6-2 `mixture_design`

- [x] `docs/research/p6_mixture_design.md`
- [x] domain + Facts + service + command（Menu IA）
- [x] simplex-lattice 或 centroid；q=3～4；写入工作表
- [x] 设计矩阵预览页与选项页分离
- [x] interpretation + help + 测试
- [x] wiring + backlog

## W6-3 `nhpp_repairable`

- [x] `docs/research/p6_nhpp_repairable.md`
- [x] 幂律 NHPP 参数估计 + 表（+可选图）
- [x] 完整竖切 + 诊断 + formula_reference 测试
- [x] UI 选项/方法分页
- [x] wiring + backlog

## W6-4 `reliability_test_plan`

- [x] `docs/research/p6_reliability_test_plan.md`（公式源钉死）
- [x] 演示型计划表（n / 允许失效 / 假设摘要）
- [x] 完整竖切 + 测试
- [x] UI 结果页与输入页分离
- [x] wiring + backlog

## Wave 门禁

- [x] Planner 映射表已产出
- [x] `verify_algorithm_wave6_track.py` PASS
- [x] 回归 `verify_algorithm_wave5_track.py` PASS
- [x] 回归 `verify_ui_menu_ia_track.py` PASS
- [x] Checker 无 Critical
- [x] 告知用户 Qt Creator Rebuild 手测四点

## 禁止偷懒核对（Checker）

- [x] 无单页堆控件
- [x] 无嵌 Python/R
- [x] 无破坏 A→B / source_row
- [x] 无「见 md」帮助正文
- [x] 无假 golden
