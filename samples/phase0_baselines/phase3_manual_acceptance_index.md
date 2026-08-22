# Phase 3 人工验收索引（S1–S7）

> 对应 [`docs/research/phase3-cross-page-pdf-manual-acceptance.md`](../../docs/research/phase3-cross-page-pdf-manual-acceptance.md) §5–§6。  
> 自动化预筛：**61** 项（§3.1）+ **37** 项 deepen + **84** 项场景映射（`python tools/list_phase3_prefilter_by_scenario.py`）  
> Runbook：[`qt-creator-dual-line-acceptance-runbook.md`](../../docs/research/qt-creator-dual-line-acceptance-runbook.md)

## 剩余任务 checklist（dual-line 目标）

**脚本侧（Agent 已完成）：** `python tools/print_acceptance_status.py` → **12/12** 通过即可进入 Qt Creator。

**仍须你本地完成（阻塞 goal 完成）：**

| # | 任务 | 数量/范围 | 完成标准 |
|---|------|-----------|----------|
| A | 脚本预检 | 12 项 | `print_acceptance_status.py` 全绿 |
| B | Qt Creator §3.1 | **61** 项 | 3 目标全绿（`--by-target`） |
| C | Qt Creator deepen | **37** 项 | 6 目标全绿（`--deepen --by-target`） |
| D | 算法域回归 | 3 整包 | `ReliabilityPhase5Test` · `ResponseSurfaceDesignPhase4Test` · `NonNormalCapabilityPhase6Test` |
| E | 场景分批（可选核对） | global **6** + S1–S7 | `--global-only` 再 `--scenario-id S1`…`S7` |
| F | 肉眼 PDF + manifest/audit | **S1–S7** | 各场景样本文档 Pass 标准 |
| G | §6 签署 | 2 角色 × 3 列 | [`phase3-cross-page-pdf-manual-acceptance.md`](../../docs/research/phase3-cross-page-pdf-manual-acceptance.md) §6 |

**计划内诚实缺口（不阻塞当前竖切签收，但 dual-line 非 100%）：** PDF/A·UA 真 tagged-PDF · vendor oracle/golden 商业对齐 · 全量 UI i18n · Johnson 开放合格判定。

批次清单（可复制勾选）：`python tools/print_qt_creator_signoff_batches.py`

---

| 步骤 | 动作 |
|------|------|
| 1 | `python tools/print_acceptance_status.py`（**12/12** 脚本侧）或 `powershell -File tools/phase3_preflight.ps1`；批次清单：`python tools/print_qt_creator_signoff_batches.py` |
| 2 | Qt Creator 跑 §3.1 全部 **61** 项（`python tools/list_qt_creator_test_targets.py --by-target` 按 3 个目标分批） |
| 2b | （推荐）§3.1 全绿后跑 **F′/E′/E″ 加深 37** 项 |
| 2c | （可选）核对 **全场景 84** 项 |
| 2d | 肉眼 PDF 前：先 **global 6**（`--global-only --by-target`），再按场景 `--scenario-id S1` … `S7` |
| 2e | （推荐）算法域回归：`ReliabilityPhase5Test` + `ResponseSurfaceDesignPhase4Test` + `NonNormalCapabilityPhase6Test`（见 runbook §1.3） |
| 3 | 下表 S1→S7 肉眼 PDF + manifest/audit 核对 |
| 4 | 在 phase3 文档 **§6** 签署表勾选 |

## 场景 → 样本文档 → Qt Creator 分批 → 关键预筛

| 场景 | 样本文档 | Qt Creator 分批 | 关键自动化预筛 |
|------|----------|-----------------|----------------|
| **S1** | [`reliability_km_long_table.md`](reliability_km_long_table.md) + CSV；Weibull/Lognormal 见 [`../reliability/reliability_survival_manual_phase5.md`](../reliability/reliability_survival_manual_phase5.md) | `--scenario-id S1 --by-target` | KM 长表 + `representative_reliability_*_three_report_profiles_*` |
| **S2** | [`doe_manual_s2_zh_cn.md`](doe_manual_s2_zh_cn.md) + [`doe_ccd_k4_factors.json`](doe_ccd_k4_factors.json) | `--scenario-id S2 --by-target`（**6** 项） | `pdf_doe_ccd_k4_long_*` + `customer_keeps_ccd_bbd_design_*` |
| **S3** | [`graph_faceted_manual_s3.md`](graph_faceted_manual_s3.md) + CSV | `--scenario-id S3 --by-target`（**16** 项） | `pdf_graph_scatter_faceted_cross_template_*` + F′ hexbin/density |
| **S4** | [`warranty_strata_manual_s4.md`](warranty_strata_manual_s4.md) + [`warranty_exposure_manual_s4.md`](warranty_exposure_manual_s4.md) | `--scenario-id S4 --by-target`（**15** 项） | `pdf_warranty_strata_*` + `pdf_warranty_exposure_*` + `customer_keeps_cif_fine_gray_*` |
| **S5** | [`graph_empty_manual_s5.md`](graph_empty_manual_s5.md) + CSV | `--scenario-id S5 --by-target`（**2** 项） | `pdf_empty_chart_*` + `pdf_graph_scatter_all_excluded_*` |
| **S6** | [`audit_appendix_manual_s6.md`](audit_appendix_manual_s6.md) + Johnson/Box-Cox 样例 | `--scenario-id S6 --by-target`（**26** 项 / **6** 目标） | `pdf_audit_km_*` + gate PDF + `box_cox_service_skips_*` + `johnson_spec_outside_support_skips_overall_capability` |
| **S7** | [`pinlength_manual_s7.md`](../capability/pinlength_manual_s7.md) + Unicode CSV | `--scenario-id S7 --by-target` | `pdf_pinlength_capability_unicode_columns_*` |

命令前缀：`python tools/list_qt_creator_test_targets.py`；列表详情：`python tools/list_phase3_prefilter_by_scenario.py --scenario-id S4`

## 全局必跑（§3.1 A + B）

- `representative_vertical_slice_reports_localize_without_cross_language_leak`（13 竖切混语）
- `linguist_mirror_matches_catalog_and_qm_loads`
- §3.1 **B** 段 manifest / PDF/A·UA 诚实 5 项

## 禁止事项

- 不得将自动化 ≥2 PDF 页断言当作表头重复或 PDF/A·UA 合规证明。
- 不得在未跑 §3.1 全绿时签署「§3.1 自动化」列。
- 不得在未跑 deepen **37** 全绿时签署「F′/E′/E″ 加深」列（若勾选）。
