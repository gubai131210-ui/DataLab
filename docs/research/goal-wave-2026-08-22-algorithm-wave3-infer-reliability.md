# Goal Wave-3 推断/可靠性（2026-08-22）

> 访问日期：2026-08-22（UTC+8）  
> 前置：[`goal-wave-2026-08-22-algorithm-wave2.md`](goal-wave-2026-08-22-algorithm-wave2.md)

## Wave-2.5 修复

| # | 项 | 状态 |
|---|-----|------|
| H1 | `nominal_logistic` IRLS（解析得分/Hessian） | [x] |
| H2 | `accelerated_life` Newton MLE + observed 信息矩阵 SE | [x] |
| H3 | 测试补强（A→B、serialize、interpret） | [x] |

## Wave-3 四项

| # | command | 类型 | research | 状态 |
|---|---------|------|----------|------|
| W1 | `bootstrap_two_sample` | 新增 | `p3_bootstrap_two_sample.md` | [x] |
| W2 | KM/Log-rank K 组深化 | 深化 | `p3_km_logrank_multigroup.md` | [x] |
| W3 | `accelerated_life` 使用应力预测 + 图 | 深化 | `p3_accelerated_life.md` | [x] |
| W4 | `probit_reliability` | 新增 | `p3_probit_reliability.md` | [x] |

## 交付门

```powershell
python tools/verify_algorithm_wave3_track.py
```

脚本预检 PASS；统一 Qt Creator 手工验收见 [`unified_track_acceptance_plan.md`](../../samples/product_evolution/unified_track_acceptance_plan.md) §2 Wave-2.5/Wave-3。
