# Phase 5 — Reliability / warranty

Status: **partial**

## Delivered

- `censoring_contract`: exact / right / left / interval validation; classic KM only exact+right
- Reliability path uses contract-normalized vectors (right never treated as failure)
- **Worksheet `censoring_type_column`**: optional; may replace or cross-check `event_column`
- Optional `failure_mode_column` carried into contract observations
- **`interval_left_column` / `interval_right_column`**: required when any row is typed `interval`; missing/non-finite → `missing_interval_bounds`; valid bounds still **block classic KM**
- **`exposure_column`**: per-row finite non-negative; reliability Facts `total_exposure`/`column_sum`; warranty prefers column sum over scalar (explicit override diagnostic); invalid → `invalid_exposure_value` (no silent 0/1); zero sum after exclude → `warranty_zero_exposure`; en-US catalog `diag.invalid_exposure_value*` / `diag.warranty_zero_exposure` / `diag.warranty_exposure_column_overrides_scalar` — locale unit `report_locale_phase3_test::warranty_exposure_diag_localizes_to_en_us`; interpretation `interp.warranty_exposure_gate` + `interpretation_service_test::usesWarrantyExposureGateInterpretationBullet`; report chain `pdf_warranty_invalid_exposure_*` + `pdf_warranty_exposure_gate_*` + `pdf_warranty_exposure_cross_template_*` + `pdf_warranty_exposure_column_override_*` + `representative_warranty_exposure_*` + `report_profile_phase1_test::customer_keeps_warranty_exposure_gate_evidence_under_truncation`；人工 [`samples/phase0_baselines/warranty_exposure_manual_s4.md`](../../samples/phase0_baselines/warranty_exposure_manual_s4.md)
- Weibull / Lognormal / Exponential families + warranty summary (`claims_per_1000 = 1000*(1-R)`)
- **Warranty strata** (`summarize_warranty_strata`): failure_mode / group denominators with measured exposure or proportional_scalar warning; Facts + table + source_rows + JSON round-trip
- **Cause-specific per-mode fits** (`fit_reliability_by_failure_mode`): competing exact failures treated as right-censored; Weibull / Lognormal / Exponential / KM; `ReliabilityFacts.mode_fits` + table; warranty strata may use mode-specific R(Tw) when fitted (`uses_mode_specific_reliability`)
- Evidence: `formula_reference` / `algorithm_id=cause_specific_right_censored_competing` (deserialize clamps fake `vendor_oracle`/`golden`)
- **Aalen–Johansen CIF** (`aalen_johansen_cif`): nonparametric cumulative incidence under competing risks; Facts `cif_*` + table; deserialize clamps fake `vendor_oracle`/`fine_gray`; honesty: CIF ≠ Fine-Gray multivar regression
- **Fine–Gray binary IPCW** (`fine_gray_binary` / `fine_gray_binary_ipcw`): single binary `group` covariate (`observation.group` assigned from group column)
- **Fine–Gray continuous IPCW** (`fine_gray_continuous` / `fine_gray_continuous_ipcw`): single continuous `covariate_column` (priority over binary); mean-centered; HR per +1 unit; Facts `fine_gray_kind` / `fine_gray_covariate_*`
- **Fine–Gray multi IPCW** (`fine_gray_multi` / `fine_gray_multi_ipcw`): `covariate_columns` with p≥2 (priority over continuous/binary); mean-centered; p≤5; target failures ≥5p; Facts `fine_gray_terms`; `formula_reference` only (deserialize clamps fake `vendor_oracle`/`golden` / unknown algorithm ids)
- **EvidenceBundle limiting honesty (report line)**: CIF → `:gate:cif_not_fine_gray`; Fine-Gray → `:gate:fine_gray_formula_reference_only`; warranty strata → `:gate:warranty_strata_exposure_honesty` / `:gate:warranty_strata_reliability_basis`; warranty exposure gate → `:gate:warranty_exposure` (`evidence.warranty_exposure_gate`; invalid/zero column sum skips summary). Forced `formula_reference`; customer retention under plot flood. Does **not** invent pinned R / vendor_oracle.
- **Turnbull / interval KM** (`km_interval` / `kaplan_meier_interval`): simplified-grid NPMLE; Facts carry `evidence_type`/`algorithm_id`; deserialize clamps fake `vendor_oracle`/`golden` back to `formula_reference`
- Tests: `reliability_phase5_test` (warranty strata + Turnbull honesty + mode fits + CIF + Fine-Gray binary/continuous/multi + ser/de clamp)
- **Per-observation censoring worksheet I/O**: `censoring_observations_to_worksheet` / `censoring_observations_from_worksheet`; reliability page sets `worksheet_export` (`name=censoring_observations`); UI write-back prompt distinct from DOE; **OutputPage JSON round-trip** for `worksheet_export` (name/columns/rows)
- **Report three-template guards** (Phase 3): `representative_reliability_km_three_report_profiles_*`, `representative_reliability_weibull_three_report_profiles_*`, `representative_reliability_lognormal_three_report_profiles_*`, `representative_warranty_summary_three_report_profiles_*`, `representative_warranty_strata_three_report_profiles_*` — customer/engineer/audit × en-US/zh-CN visible-layer; complements `pdf_reliability_km_and_weibull_*` / `pdf_reliability_km_and_weibull_cross_template_*` / `pdf_warranty_summary_*` / `pdf_warranty_summary_cross_template_*` / `pdf_warranty_cross_template_*`

## Honest gaps

- Turnbull is **simplified grid** NPMLE only — not pinned R `survival` / commercial Turnbull (`vendor_oracle` ⏸)
- Threshold models (weibull3 / lognormal3) not yet in per-mode path
- Pinned R / `reference_implementation` goldens for KM/Weibull/Lognormal/Turnbull / mode fits / CIF / Fine-Gray (including multi) — not `vendor_oracle`
- Fine-Gray multi is **formula_reference** IPCW Newton only — not cause-specific Cox, not pinned R `survival::finegray`
- `worksheet_export` JSON omits column_types/cell_states (string grid only; enough for censoring re-import)
