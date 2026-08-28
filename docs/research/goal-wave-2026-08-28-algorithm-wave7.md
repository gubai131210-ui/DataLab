# Goal DoD：算法 Wave-7（2026-08-28）



> Orchestrator 在 `/goal` 中逐项勾选；**四项全 `[x]` + verify PASS** 才可 complete。  

> 计划：[`goal-wave-2026-08-28-algorithm-wave7-plan-and-mega-prompt.md`](goal-wave-2026-08-28-algorithm-wave7-plan-and-mega-prompt.md)  

> 调研：[`algorithm-wave7-market-formula-research-2026-08-28.md`](algorithm-wave7-market-formula-research-2026-08-28.md)



## Meta



| 字段 | 值 |

|------|-----|

| Goal | 算法 Wave-7 |

| Verify | `tools/verify_algorithm_wave7_track.py` |

| 测试 | `tests/algorithm_wave7_track_test.cpp` |



## W7-1 `mixture_analyze`



- [x] `docs/research/p7_mixture_analyze.md`（Primary URL + 公式 + 访问日期）

- [x] domain + Facts + AnalysisService + command（menu_path/menu_group）

- [x] Scheffé 线性（+可选二次）；无常数项；Coefficients + ANOVA + 残差

- [x] interpretation + serialization + help

- [x] **独立**多页对话框（≠ `mixture_design`）

- [x] formula_reference 测试

- [x] wiring + backlog 更新



## W7-2 `glm_two_way`



- [x] `docs/research/p7_glm_two_way.md`

- [x] 不平衡双因子；Type III Adj SS；Fitted Means 表

- [x] 完整竖切 + 诊断 + formula_reference 测试

- [x] UI 选项/方法/结果分页

- [x] wiring + backlog



## W7-3 `analyze_variability`



- [x] `docs/research/p7_analyze_variability.md`

- [x] 2 水平；每运行标准差；ln(σ) 分散模型；效应/ANOVA 表

- [x] 完整竖切 + 测试

- [x] UI 重复列布局与方法分页

- [x] wiring + backlog



## W7-4 `factor_analysis`



- [x] `docs/research/p7_factor_analysis.md`

- [x] 主成分提取；Loadings + % Var；Scree 图

- [x] 完整竖切 + 测试

- [x] UI 变量选择与提取选项分页

- [x] wiring + backlog



## Wave 门禁

- [x] Planner 映射表已产出
- [x] `verify_algorithm_wave7_track.py` PASS
- [x] 回归 `verify_algorithm_wave6_track.py` PASS
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

- [x] Mixture 分析未塞进 `mixture_design` 对话框

