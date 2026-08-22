# Goal Wave-2 算法竖切（2026-08-22）

> 访问日期：2026-08-22（UTC+8）  
> 权威框架：[`goal-execution-framework.md`](goal-execution-framework.md)  
> 验收：[`unified_track_acceptance_plan.md`](../../samples/product_evolution/unified_track_acceptance_plan.md) §2

## Wave-1 锁定清单（4 项 · 全部完成才 Goal complete）

| # | command id | 类型 | research | 状态 |
|---|------------|------|----------|------|
| 1 | `nominal_logistic` | 新增 | `p3_nominal_logistic.md` | [x] |
| 2 | `nonparametric_capability` | 新增 | `p3_nonparametric_capability.md` | [x] |
| 3 | `stepwise_regression` | 深化 AICc/BIC 表形 | `p3_stepwise_regression.md` 更新 | [x] |
| 4 | `accelerated_life` | 可靠性窄化（ALT · Arrhenius+Weibull） | `p3_accelerated_life.md` | [x] |

**可靠性择一结论：** warranty 已在 Phase 5 闭环（`reliability_warranty`）；本 Wave 交付 **ALT** 窄化竖切。

## 禁止偷懒（goal-execution-framework.md §6 全文）

1. 禁止只做 UI 壳不算 domain/Facts  
2. 禁止跳过 interpretation 与 catalog 双语  
3. 禁止把 Minitab 数值当 golden  
4. 禁止单页堆叠超过一层主流程控件  
5. 禁止破坏 `row_visibility` hidden/excluded 语义  
6. 禁止 infrastructure 新增对 ui 的 include  
7. 禁止合并 customer/engineer/audit 为单模板  
8. 禁止省略 help catalog / `algorithm_help.json`  
9. 禁止大 catalog 单 TU（>500 条）  
10. 禁止宣称 PDF/A·UA 合规无验证器  
11. 禁止每 Wave 强制停 Qt Creator 才允许下一 Wave  
12. **禁止 Goal 在只完成 1 个算法/1 个小优化后标记 complete**  
13. **禁止跳过网上 Primary URL 调研**

## 竖切 DoD（每项）

- [ ] research md（Primary URL + 2026-08-22）  
- [ ] `src/domain/**` 纯 C++  
- [ ] `*Facts` + `AnalysisService::*` + `analysis_commands`  
- [ ] `interpretation_service` + catalog 双语  
- [ ] `algorithm_help.json` + `algorithm-wiring-index.md`  
- [ ] CMake 测试 + `# source: formula_reference`  
- [ ] complete-case / `source_row` / A→B 失效  
- [ ] backlog ✅ + acceptance §2  

## Wave 交付门

```powershell
python tools/verify_algorithm_wave2_track.py
```

## 多 Agent 记录

| 角色 | agent | 输出 |
|------|-------|------|
| Researcher | explore ea56da93 | backlog/wiring 地图；ALT 优先于 warranty |
| Orchestrator | 主对话 | 连续竖切 + 文档 |
