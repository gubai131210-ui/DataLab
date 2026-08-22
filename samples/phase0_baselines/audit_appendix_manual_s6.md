# Audit 证据附录多页人工验收（Phase 3 S6）

配合 [`docs/research/phase3-cross-page-pdf-manual-acceptance.md`](../../docs/research/phase3-cross-page-pdf-manual-acceptance.md) §5 S6。

## 目标

audit 模板 en-US PDF：**正文 + Evidence appendix** 分页合理；`.audit.json` 的 `label_text_id` 与 PDF 可见 label 一致；**不**声称 PDF/A·UA 合规。

---

## 路径 A — KM 长表 + audit（推荐先做）

与 S1 共用 CSV，模板改为 **audit**。

| 项 | 值 |
|----|-----|
| 数据 | [`reliability_km_long_table.csv`](reliability_km_long_table.csv) |
| 分析 | Kaplan–Meier |
| 模板 | **audit** |
| locale | en-US |

详细步骤见 [`reliability_km_long_table.md`](reliability_km_long_table.md) **路径 B**。

**自动化预筛：** `pdf_audit_km_long_table_evidence_appendix_spans_pages_en_us`（§3.1 #30）

**Pass 焦点：**

- [ ] ≥2 PDF 页；含 **Evidence appendix** 英文标题
- [ ] 生存表仍可见（audit 保留统计表）
- [ ] `.audit.json` evidence 条目含 `label_text_id`
- [ ] manifest `pdfa_status=not_validated`、`pdfua_status=unsupported`

---

## 路径 B — Johnson 门禁 audit（formula_reference）

| 项 | 值 |
|----|-----|
| 分析 | 过程能力 → **Johnson**（gated_research） |
| 数据 | [`johnson_gated_s6.csv`](../capability/johnson_gated_s6.csv)（步骤见 [`johnson_gated_manual_s6.md`](../capability/johnson_gated_manual_s6.md)） |
| 模板 | audit |
| locale | en-US |

**Qt Creator 步骤：**

1. 导入 [`johnson_gated_s6.csv`](../capability/johnson_gated_s6.csv)（或手工录入 ≥30 行正偏态数值 + 规格限）。
2. 运行 Johnson 能力；确认诊断含门禁（不得输出合格判定）。
3. 导出 audit en-US PDF + manifest + audit JSON。
4. 对照 PDF 中 `Johnson capability gate` 与 JSON 中 `evidence.johnson_capability_gated`。

**自动化预筛：**

- `representative_johnson_capability_three_report_profiles_localize_without_cross_language_leak`
- `pdf_johnson_capability_gate_localizes_and_audit_evidence_carries_label_text_id`
- `pdf_johnson_capability_cross_template_table_visibility_and_en_us_locale`
- `customer_keeps_capability_gate_limiting_evidence_under_truncation`

---

## 路径 C — RSM LOF audit（对照）

| 项 | 值 |
|----|-----|
| 分析 | 响应曲面；数据 [`rsm_lof_fixture.csv`](../phase0_baselines/rsm_lof_fixture.csv) |
| 模板 | audit |
| locale | en-US |

**Qt Creator：** 导入 fixture → **RSM 响应分析**（Y/A/B）→ audit en-US 导出。

**自动化预筛：** `representative_rsm_lof_*` + `pdf_rsm_lof_gate_*` + `pdf_rsm_lof_cross_template_*` + `customer_keeps_rsm_lof_limiting_evidence_under_truncation`

---

## 路径 D — Johnson 规格限越界 audit

| 项 | 值 |
|----|-----|
| 分析 | 过程能力 → **Johnson**；**仅 LSL = -1000** |
| 数据 | [`johnson_gated_s6.csv`](../capability/johnson_gated_s6.csv) |
| 模板 | audit |
| locale | en-US |

**步骤：** 见 [`johnson_gated_manual_s6.md`](../capability/johnson_gated_manual_s6.md) **路径 D**。

**自动化预筛：** `representative_johnson_spec_outside_*` + `pdf_johnson_spec_outside_*` + `customer_keeps_johnson_spec_limit_*` + `usesJohnsonSpecLimitGateInterpretationBullet` + `nonnormal_capability_phase6_test::johnson_spec_outside_support_skips_overall_capability`

---

## 路径 E — 保修暴露量门禁 audit

| 项 | 值 |
|----|-----|
| 分析 | 可靠性 → **保修摘要**；暴露量列含非法值 |
| 步骤 | 见 [`warranty_exposure_manual_s4.md`](warranty_exposure_manual_s4.md) **路径 A** |
| 模板 | audit |
| locale | en-US |

**自动化预筛：** `representative_warranty_exposure_gate_*` + `pdf_warranty_exposure_gate_*` + `customer_keeps_warranty_exposure_gate_*` + `usesWarrantyExposureGateInterpretationBullet`

---

## 路径 F — Box-Cox 规格限门禁 audit

| 项 | 值 |
|----|-----|
| 分析 | **Box-Cox 变换**；无效或倒置规格限 |
| 步骤 | 见 [`box_cox_spec_gate_manual_s6.md`](../capability/box_cox_spec_gate_manual_s6.md) **场景 A** |
| 模板 | audit |
| locale | en-US |

**自动化预筛：** `representative_box_cox_*` + `pdf_box_cox_invalid_spec_limit_*` + `pdf_box_cox_spec_limit_gate_*` + `customer_keeps_box_cox_limiting_*` + `box_cox_spec_limit_diag_localizes_to_en_us` + `usesBoxCoxSpecLimitGateInterpretationBullet`

---

## 全局对照（任一路径）

- [ ] `pdf_evidence_appendix_uses_localized_label_text_ids`
- [ ] `audit_json_doe_design_generation_carries_label_text_ids`（DOE 设计证据结构对照）

## 证据

- `formula_reference` / 产品契约；禁止 PDF/A·UA 合规声明。
