# Reference implementation 脚本索引

> 日期：2026-08-22（G-Trust 锁表脚本登记：2026-09-05）  
> 用途：独立 Python 再生/手算对照；证据类型 **`reference_implementation`**（**不是** `vendor_oracle`）  
> 一键预检：`powershell -File tools/reference_implementation_preflight.ps1`  
> G-Trust 门禁：`python tools/verify_g_trust_golden_gate.py`（默认全集 10 命令）

## 运行方式

```powershell
powershell -File tools/reference_implementation_preflight.ps1
python tools/verify_g_trust_golden_gate.py
```

Phase 3 报告预检 `tools/phase3_preflight.ps1` 会在 registry/interpretation 之后 **自动调用** 上述脚本，并校验 §3.1（61）、场景（84）、deepen（37）、**13/13 竖切**、**3/3 interpretation gate**、**9/9 customer_keeps**、**3/3 domain gate**（`verify_*_prefilter_registry.py`、`verify_vertical_slice_scenario_coverage.py`、`verify_interpretation_gate_scenario_coverage.py`、`verify_customer_keeps_scenario_coverage.py`、`verify_domain_gate_scenario_coverage.py`）。

## 脚本清单

| 脚本 | Phase | 对照源 | 样例/fixture | C++ 测试（Qt Creator） |
|------|-------|--------|--------------|------------------------|
| [`scripts/reliability_km_reference.py`](../../scripts/reliability_km_reference.py) | 5 | KM product-limit 手算 | [`reliability_km_handcalc.csv`](../../samples/phase0_baselines/reliability_km_handcalc.csv) | `reliability_phase5_test::km_handcalc_baseline_formula_reference` |
| [`scripts/reliability_warranty_reference.py`](../../scripts/reliability_warranty_reference.py) | 5 | claims/1000 + 分层池化 R | [`warranty_strata_s4.csv`](../../samples/phase0_baselines/warranty_strata_s4.csv) | `reliability_phase5_test::warranty_*` |
| [`scripts/rsm_lof_reference.py`](../../scripts/rsm_lof_reference.py) | 4 | 纯误差 df=2 / LOF 残差分解 | [`rsm_lof_fixture.csv`](../../samples/phase0_baselines/rsm_lof_fixture.csv) | `response_surface_design_phase4_test::rsm_lack_of_fit_*` |
| [`scripts/box_cox_reference.py`](../../scripts/box_cox_reference.py) | 6 | λ=0/1、规格限序 | — | `nonnormal_capability_phase6_test::box_cox_lambda_special_cases_and_limit_order` |
| [`scripts/doe_rsm_reference_points.py`](../../scripts/doe_rsm_reference_points.py) | 4 | CCD/BBD 标准序 golden | `doe_*_stdorder_golden.json` | `response_surface_design_phase4_test` |

### G-Trust 锁表（`golden` ← `reference_implementation`；≠ vendor_oracle）

| 脚本 | command_id | expected | C++ 测试槽 |
|------|------------|----------|------------|
| [`scripts/g_trust_imr_reference.py`](../../scripts/g_trust_imr_reference.py) | `imr` | `imr_ref_golden.tsv` | `imrMatchesRefGolden` |
| [`scripts/g_trust_xbar_r_reference.py`](../../scripts/g_trust_xbar_r_reference.py) | `xbar_r` | `xbar_r_ref_golden.tsv` | `xbarRMatchesRefGolden` |
| [`scripts/g_trust_p_chart_reference.py`](../../scripts/g_trust_p_chart_reference.py) | `p_chart` | `p_chart_ref_golden.tsv` | `pChartMatchesRefGolden` |
| [`scripts/g_trust_capability_reference.py`](../../scripts/g_trust_capability_reference.py) | `capability` | `capability_ref_golden.tsv` | `capabilityMatchesRefGolden` |
| [`scripts/g_trust_capability_sixpack_reference.py`](../../scripts/g_trust_capability_sixpack_reference.py) | `capability_sixpack` | `capability_sixpack_ref_golden.tsv` | `capabilitySixpackMatchesRefGolden` |
| [`scripts/g_trust_between_within_capability_reference.py`](../../scripts/g_trust_between_within_capability_reference.py) | `between_within_capability` | `between_within_capability_ref_golden.tsv` | `betweenWithinCapabilityMatchesRefGolden` |
| [`scripts/g_trust_gage_rr_reference.py`](../../scripts/g_trust_gage_rr_reference.py) | `gage_rr` | `gage_rr_ref_golden.tsv` | `gageRrMatchesRefGolden` |
| [`scripts/g_trust_two_sample_t_reference.py`](../../scripts/g_trust_two_sample_t_reference.py) | `two_sample_t` | `two_sample_t_ref_golden.tsv` | `twoSampleTMatchesRefGolden` |
| [`scripts/g_trust_normality_test_reference.py`](../../scripts/g_trust_normality_test_reference.py) | `normality_test` | `normality_test_ref_golden.tsv` | `normalityTestMatchesRefGolden` |
| [`scripts/g_trust_one_way_anova_reference.py`](../../scripts/g_trust_one_way_anova_reference.py) | `one_way_anova` | `one_way_anova_ref_golden.tsv` | `oneWayAnovaMatchesRefGolden` |

复跑：`python scripts/g_trust_<command_id>_reference.py` → 覆写对应 `tests/fixtures/minitab/expected/*_ref_golden.tsv`。  
Target：`minitab_numerical_golden_test`（Qt Creator）。

## 禁止事项

- 不得将脚本通过结果写成 Minitab/JMP **商业对齐**（无 `vendor_oracle` 导出）。
- 不得将 reference 脚本输出直接改名为项目 `golden` 而不走冻结流程（见 [`VALIDATION_MATRIX.md`](VALIDATION_MATRIX.md)）。
- PDF 预检通过 **不** 等于 PDF/A·UA 合规。
- G-Trust 锁表 **ref-golden 已冻** ≠ 「已与 Minitab 数值对齐」。

## 已知 domain 对齐（2026-08-22）

- **Box-Cox 规格限序**：`box_cox_limits_order_ok` 在 `(0,∞)` 上对全部 λ 单调递增；已移除错误的 λ<0 递减分支（`src/domain/statistics/box_cox.cpp`）。请在 Qt Creator 跑 `nonnormal_capability_phase6_test` 回归。

## 延后（⏸）

| 项 | 原因 |
|----|------|
| Weibull/Lognormal MLE vs R `survival` | 需 pinned CRAN + 可选 scipy；无 oracle 不得声称 |
| R `rsm::ccd/bbd` 点集 | 需 pinned CRAN |
| Johnson 能力 golden | `gated_research`；开放门禁前不得 reference 冒充 vendor |
| 真·Minitab `vendor_oracle` 导出对齐（锁表 10） | 后续 Goal；本轮仅 `reference_implementation` |
