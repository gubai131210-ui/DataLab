# 算法竖切批次计划（2026-08-22）

> 研究日期：2026-08-22 · 访问日期：2026-08-22（UTC+8）  
> 验收策略：连续交付 · 末尾统一 Qt Creator 门（`samples/product_evolution/unified_track_acceptance_plan.md`）  
> **大批量 Goal 框架**：[`goal-execution-framework.md`](goal-execution-framework.md)  
> `formula_reference ≠ golden`；参考 Minitab **表形/输出结构**，不对齐 vendor 数值。

## 锁定批次

| # | 类型 | command id | 状态目标 | research md |
|---|------|------------|----------|-------------|
| A1 | 新增 | `best_subsets_regression` | ❌→✅ 完整竖切 | `p3_best_subsets_regression.md` |
| A2 | 深化 | `logistic_regression` | 🟡→✅ concordance + 分类表 | `p1_logistic_diagnostics_minitab.md`（增补 §2） |
| A3 | 新增 | `batch_capability` | ❌→✅ 按批次分组能力 | `p3_batch_capability.md` |

**不在本批：** G Track UI、nominal logistic（独立大批）、GLM/Mixed/MANOVA。

## 禁止偷懒（执行清单）

1. 禁止只做 UI 壳不算 domain/Facts  
2. 禁止跳过 interpretation 与 catalog 双语  
3. 禁止把 Minitab 数值当 golden  
4. 禁止单页堆叠超过一层主流程控件  
5. 禁止破坏 `row_visibility` hidden/excluded 语义  
6. 禁止 infrastructure 新增对 ui 的 include  
7. 禁止合并 customer/engineer/audit 为单模板  
8. 禁止省略 help catalog / `algorithm_help.json`  
9. 禁止大 catalog 单 translation unit（>500 条）  
10. 禁止宣称 PDF/A·UA 合规无验证器  
11. 禁止每算法强制停 Qt Creator 才允许下一算法  

## 导入契约（每算法测试必含）

- complete-case 行筛选  
- `source_row` 回映射  
- 导入 A→B 后旧输出/排除/undo 失效（service 层或 serialization 回归）

## 交付物检查表

- [ ] research md（Primary URL + 访问日期）  
- [ ] domain + Facts + service + commands + interpretation  
- [ ] CMake 测试 + `tools/verify_algorithm_batch_2026_08.py`  
- [ ] `algorithm_help.json` + wiring / backlog 状态更新  
- [ ] `unified_track_acceptance_plan.md` §2 算法批行  
