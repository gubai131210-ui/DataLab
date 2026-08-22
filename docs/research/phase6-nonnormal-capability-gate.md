# Phase 6 — Non-normal capability gating

Status: **partial / gated** (must remain gated until Johnson golden + tail + human review)

## Delivered

- Box-Cox: λ=0/1 cases, non-positive rejection, limit transform + order check
- Application layer: `AnalysisService::box_cox` uses domain limit helpers; invalid/inverted spec limits skip capability table (`box_cox_invalid_spec_limit`, `box_cox_spec_limits_order`; `quality_statistics_test`)
- Report i18n: spec-limit gate diagnostics map to catalog IDs (`diag.box_cox_invalid_spec_limit_*`, `diag.box_cox_spec_limits_order`); `box_cox_spec_limit_diag_localizes_to_en_us`
- PDF: `pdf_box_cox_invalid_spec_limit_localizes_to_en_us_without_chinese_leak` (invalid LSL → no capability table + localized diagnostic in engineer PDF)
- Interpretation: spec-limit gate limitation bullet (`interp.box_cox_spec_limit_gate`) when `box_cox_invalid_spec_limit` / `box_cox_spec_limits_order` present; `interpretation_service_test::usesBoxCoxSpecLimitGateInterpretationBullet`
- Cross-template PDF: `pdf_box_cox_invalid_spec_limit_cross_template_table_visibility_and_en_us_locale` (customer/engineer/audit omit capability table; gate bullet + risk diagnostic)
- EvidenceBundle: `:gate:box_cox_not_pass_fail` + `:gate:box_cox_spec_limit` (`evidence.box_cox_*`; audit appendix + `pdf_box_cox_spec_limit_gate_localizes_and_audit_evidence_carries_label_text_id`; customer plot-flood retention in `report_profile_phase1_test`)
- Johnson capability: `gate_status=gated_research`, `pass_fail_judgment_allowed=false`, `johnson_capability_gated` diagnostic, research-preview
- **Johnson spec-limit gate**: `johnson_spec_outside_support` error skips Overall Capability table; interpretation `interp.johnson_spec_limit_gate`; EvidenceBundle `:gate:johnson_spec_limit` (`evidence.johnson_spec_limit_gate`); localization branches error vs partial-outside messages; tests `johnson_spec_outside_support_skips_overall_capability` / `pdf_johnson_spec_outside_*` / `representative_johnson_spec_outside_three_report_profiles_*` / `report_profile_phase1_test::customer_keeps_johnson_spec_limit_evidence_under_truncation`
- Parametric `non_normal` path: also `pass_fail_judgment_allowed=false` (no silent commercial pass/fail)
- **Stability prerequisite (CAP-NN-1)**:
  - `apply_capability_stability_screen` — I-MR Rule-1 only (`formula_reference`)
  - Clear screen ≠ `assumption_status=verified`; never auto-opens pass/fail
  - Normal capability: `gate_status=stability_unverified` / `stability_screen_signals`, `pass_fail_judgment_allowed=false`
  - Diagnostics: `capability_stability_prerequisite`, `capability_pass_fail_blocked_by_stability_prerequisite`
- **Bimodality screen (CAP-NN-1 deepen)**:
  - `apply_capability_bimodality_screen` — histogram peak/valley (`formula_reference`)
  - Suspected → `evidence_against` + `gate_status=bimodality_suspected`; clear ≠ unimodality proof
  - Pass/fail stays closed
- **Hartigan dip screen (CAP-NN-1 deepen)**:
  - `compute_hartigan_dip` / `apply_capability_hartigan_dip_screen` — Hartigan & Hartigan (1985) dip + Uniform-null MC p (`formula_reference` / `hartigan_dip_1985`)
  - `evidence_against` → assumption `evidence_against` + `gate_status=hartigan_dip_evidence_against` (after stability signals)
  - `consistent` ≠ unimodality proof; never opens pass/fail; not vendor_oracle
  - Facts + JSON round-trip: `hartigan_dip_status` / `statistic` / `p_value`
- **Gaussian mixture BIC search (CAP-NN-1 deepen)**:
  - `fit_gaussian_mixture_2` (fixed k=2) + `fit_gaussian_mixture_search` (k=1..k_max, default 4) (`formula_reference` / `gaussian_mixture_k_bic`)
  - Capability screen uses search; status `preferred_2comp` | `preferred_kcomp` | `not_preferred`
  - Preferred → assumption `evidence_against` + `gate_status=mixture_preferred_*` (after stability signals, before Hartigan)
  - Facts: `mixture_k_selected` / `mixture_k_max` / `mixture_components[]` / `mixture_*`; deserialize clamps fake vendor ids
  - Never opens pass/fail; not non-Gaussian mixtures; not vendor_oracle
- Interpretation refuses process-pass language under gate / stability / bimodality / Hartigan / mixture block
- **EvidenceBundle honesty (report line)**: when `pass_fail_judgment_allowed=false`, assembly emits early `EvidenceRole::limiting` gate refs (`:gate:johnson` / `:gate:capability_pass_fail_blocked`) with `evidence_type=formula_reference` only; `apply_report_profile` priority-keeps limiting (+ dataset/filter) under `max_evidence_rows` so customer templates cannot drop the gate behind plot flood. Does **not** open pass/fail or invent vendor_oracle.
- **Box-Cox EvidenceBundle**: `:gate:box_cox_not_pass_fail` when `assumption_status=not_verified`; `:gate:box_cox_spec_limit` when spec-limit diagnostics present (`evidence.box_cox_*`; audit PDF + `pdf_box_cox_*` tests).
- Test: `nonnormal_capability_phase6_test` (Johnson + non_normal + stability + bimodality + Hartigan + mixture 2/k)
- Report guard: `representative_vertical_slice_reports_localize_without_cross_language_leak` includes **non_normal** Weibull Z-score + stability prerequisite visible-layer checks (en/zh), and **normal** capability stability/pass-fail block diagnostics (en/zh).
- Three-template guard: `representative_nonnormal_capability_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en-US/zh-CN；`:gate:capability_pass_fail_blocked` + 风险级 Z-score/stability 诊断可见性。
- Three-template guard: `representative_normal_capability_three_report_profiles_localize_without_cross_language_leak` — 正态能力 stability 未验收 × 三模板 × 双语；pass/fail block 诊断 + gate evidence。
- PDF byte-scan: `pdf_normal_capability_stability_gate_localizes_and_audit_evidence_carries_label_text_id` — audit en-US PDF + JSON；与 Johnson 门禁测试对称。
- 三模板 PDF：`pdf_normal_capability_cross_template_table_visibility_and_en_us_locale` — customer 隐藏 Capability indices、保留 stability 警告；audit 证据附录。
- 三模板 PDF：`pdf_nonnormal_capability_cross_template_table_visibility_and_en_us_locale` — customer 隐藏 Capability indices、保留 Z-score 警告；audit 证据附录。
- 三模板 PDF：`pdf_johnson_capability_cross_template_table_visibility_and_en_us_locale` — customer 隐藏 Johnson transform 表；保留 gate 诊断；audit 证据附录。
- 三模板 PDF：`pdf_box_cox_cross_template_table_visibility_and_en_us_locale` — customer 隐藏 Capability after transform / Transform parameters；保留 pass/fail 诚实句；audit 证据附录。
- PDF byte-scan: `pdf_nonnormal_capability_stability_gate_localizes_and_audit_evidence_carries_label_text_id` — Weibull 非正态 Z-score + stability gate；与 `nonnormal_capability_phase6_test` 域断言互补。
- Three-template guard: `representative_box_cox_three_report_profiles_localize_without_cross_language_leak` — Box-Cox `assumption_status=not_verified` × 三模板；customer 无变换后能力表但保留 pass/fail 诚实 bullet。

## Evidence

| Item | Type |
| --- | --- |
| Box-Cox transform / limits | `formula_reference` |
| I-MR Rule-1 stability screen | `formula_reference` (preliminary only) |
| Histogram bimodality screen | `formula_reference` (preliminary only) |
| Hartigan dip + Uniform MC p | `formula_reference` (research screen) |
| Gaussian mixture EM + BIC (k≤5) | `formula_reference` (research screen) |
| Johnson fit preview | `formula_reference` (research) |
| Johnson capability pass/fail | **blocked** until golden/tail/review |
| Normal capability pass/fail | **blocked** until stability+normality verified |

## Honest gaps

- No approved Johnson golden / vendor_oracle — **do not open pass/fail**
- Full multi-rule / phased control-chart acceptance workflow not productized
- Mixture is **Gaussian only** — not non-Gaussian mixtures, not pinned commercial tables
- Hartigan MC uses Uniform(0,1) null only — not pinned commercial critical tables / vendor_oracle
- Dual-scale Cp table UI polish still thin
