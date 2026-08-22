# Goal Wave-4 质量表形 + 可靠性竞争风险 + Cox/Logistic 深化（2026-08-22）

> 访问日期：2026-08-22（UTC+8）  
> 计划与 Mega 提示词：[`goal-wave-2026-08-22-algorithm-wave4-plan-and-mega-prompt.md`](goal-wave-2026-08-22-algorithm-wave4-plan-and-mega-prompt.md)  
> 前置：[`goal-wave-2026-08-22-algorithm-wave3-infer-reliability.md`](goal-wave-2026-08-22-algorithm-wave3-infer-reliability.md) ✅

## Wave-4.0 前置

| # | 项 | 状态 |
|---|-----|------|
| P0 | Wave-3 命令公式注册表回填（algorithm_help.json formula + Primary URL + research md） | [x] |
| P0 | size_t/int std::min 编译扫描（全库 grep；已知 best_subsets 已修；无新增混型） | [x] |

## Wave-4 四项

| # | command / 主题 | research | 状态 |
|---|----------------|----------|------|
| W4-1 | `nonparametric_capability` 深化（Capability Histogram + Observed Performance PPM + Facts） | `p4_nonparametric_capability_deepen.md` | [x] |
| W4-2 | `reliability` 竞争风险 / CIF 深化（CIF 曲线 + Gray 检验窄化） | `p4_reliability_competing_risks_cif.md` | [x] |
| W4-3 | `cox_regression` 窄化（固定协变量 PH） | `p4_cox_regression.md` | [x] |
| W4-4 | `logistic_regression` 深化（forward/backward 逐步 AIC/BIC） | `p4_logistic_regression_deepen.md` | [x] |

## 竖切 DoD

| 层级 | 状态 |
|------|------|
| 4× p4 research md（Primary URL + 2026-08-22） | [x] |
| domain（nonparametric hist、cox_regression、gray_test、logistic stepwise） | [x] |
| Facts + AnalysisService + commands | [x] |
| interpretation + catalog | [x] |
| output_serialization round-trip | [x] |
| algorithm_help.json + algorithm-wiring-index.md | [x] |
| tests/algorithm_wave4_track_test.cpp | [x] |
| tools/verify_algorithm_wave4_track.py PASS | [x] |
| backlog / acceptance §2 / roadmap 同步 | [x] |
| Wave-3 verify 不回退 | [x] |

## 交付门

```powershell
python tools/verify_algorithm_wave4_track.py
```

**脚本预检：PASS（2026-08-22）**

人手门：Qt Creator **Rebuild** + 跑 `algorithm_wave4_track_test`（见 `unified_track_acceptance_plan.md` §2 Wave-4）。
