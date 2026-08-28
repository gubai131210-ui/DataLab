# Goal DoD：算法 Wave-9（2026-08-28）

> Orchestrator 在 `/goal` 中逐项勾选；**四项全 `[x]` + verify PASS** 才可 complete。  
> 计划：[`goal-wave-2026-08-28-algorithm-wave9-plan-and-mega-prompt.md`](goal-wave-2026-08-28-algorithm-wave9-plan-and-mega-prompt.md)  
> 调研：[`algorithm-wave9-market-formula-research-2026-08-28.md`](algorithm-wave9-market-formula-research-2026-08-28.md)

## Meta

| 字段 | 值 |
|------|-----|
| Goal | 算法 Wave-9 |
| Verify | `tools/verify_algorithm_wave9_track.py` |
| 测试 | `tests/algorithm_wave9_track_test.cpp` |

## W9-1 `expanded_gage_unbalanced`

- [ ] `docs/research/p9_expanded_gage_unbalanced.md`（Primary URL + 公式 + 访问日期）
- [ ] domain + Facts + AnalysisService + command（menu_path/menu_group）
- [ ] 不平衡 GLM 方差分量；VarComp + %Contribution + %Study Var + NDC
- [ ] interpretation + serialization + help
- [ ] **独立**多页对话框（≥4 页）；≠ `expanded_gage_rr` 对话框
- [ ] formula_reference 测试
- [ ] wiring + backlog 更新

## W9-2 `split_plot_analyze`

- [ ] `docs/research/p9_split_plot_analyze.md`
- [ ] 裂区双误差项；难改/易改因子；ANOVA + WP/SP residuals
- [ ] 完整竖切 + formula_reference 测试
- [ ] UI 因子分层/模型/方法分页
- [ ] wiring + backlog

## W9-3 `mixture_process_variable`

- [ ] `docs/research/p9_mixture_process_variable.md`
- [ ] Scheffé + 1 过程变量 + 可选组分×过程交互
- [ ] 完整竖切 + 诊断 + formula_reference 测试
- [ ] UI 组分/过程变量/模型分页；≠ `mixture_analyze` 对话框
- [ ] wiring + backlog

## W9-4 `manova_one_way`

- [ ] `docs/research/p9_manova_one_way.md`
- [ ] 2～4 响应 + 1 因子；Wilks/Pillai/LH/Roy；H/E SSCP
- [ ] 完整竖切 + 测试
- [ ] UI 多响应/检验/方法分页
- [ ] wiring + backlog

## Wave 门禁

- [ ] Planner 映射表已产出
- [ ] `verify_algorithm_wave9_track.py` PASS
- [ ] 回归 `verify_algorithm_wave8_track.py` PASS
- [ ] 回归 `verify_algorithm_wave7_track.py` PASS
- [ ] 回归 `verify_ui_menu_ia_track.py` PASS
- [ ] Checker 无 Critical
- [ ] 告知用户 Qt Creator Rebuild 手测四点

## 禁止偷懒核对（Checker）

- [ ] 无单页堆控件
- [ ] 无嵌 Python/R
- [ ] 无破坏 A→B / source_row
- [ ] 无「见 md」帮助正文
- [ ] 无假 golden
- [ ] Expanded Gage 不平衡未并入 `expanded_gage_rr` 对话框
- [ ] Mixture 过程变量未并入 `mixture_analyze` 对话框「加勾选」
- [ ] Split-plot 未塞进 `doe_factorial` 单页
