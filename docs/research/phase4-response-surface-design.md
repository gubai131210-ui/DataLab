# Phase 4 — Response Surface Design (CCD / BBD / RSM)

Status: **partial** (4.1–4.4 core + worksheet + pinned goldens + RSM pure-error/LOF; R/vendor LOF still open).

## Delivered

- Domain generators: CCD (CCF/CCI/CCC) and BBD with continuous-factor validation
- `DesignGenerationFacts` + design matrix page (coded + actual)
- RSM coding from design bounds (`design_source_id`, `coding_mode`, center-point count)
- **Worksheet export**: `response_surface_design_to_worksheet` → `OutputPage::worksheet_export` (CCD/BBD)
- **Factorial worksheet export**: `factorial_design_to_worksheet` → same UI write-back path (full/fractional/Plackett–Burman via `doe_design_page`)
- **Factorial worksheet re-import**: `import_factorial_runs_from_worksheet` recognizes `PointType=center` and actual midpoints; keeps `DoeRun::center_point` for curvature / pure-error
- **Pinned goldens** (source = `reference_implementation`, **not** `vendor_oracle`):
  - `samples/phase0_baselines/doe_ccd_k2_ccf_stdorder_golden.json`
  - `samples/phase0_baselines/doe_bbd_k3_stdorder_golden.json`
  - Regenerator: `scripts/doe_rsm_reference_points.py`
  - Tests: `ccd_and_bbd_match_pinned_reference_implementation_golden`
- **RSM pure error / lack-of-fit ANOVA** (`formula_reference`):
  - Replicated coded points → Pure Error + Lack of Fit rows in ANOVA
  - No replicates → `rsm_insufficient_pure_error` (does **not** fake PE from residual MS)
  - Facts: `pure_error_available` / `lack_of_fit_*`; serialization round-trip
  - Tests: `rsm_lack_of_fit_uses_replicated_coded_points_not_residual_ms`,
    `rsm_without_replicates_refuses_fake_pure_error`
- Evidence elsewhere on RSM fit pages remains `formula_reference` unless noted
- **EvidenceBundle honesty (report line)**: when `facts.rsm` is present, assembly emits early `EvidenceRole::limiting` gates (`:gate:rsm_insufficient_pure_error`, `:gate:rsm_lof_formula_reference`, optional `:gate:rsm_static_surface`) with forced `evidence_type=formula_reference` and `residual_ms_as_pure_error=false`. Customer templates keep these under `max_evidence_rows` plot flood via priority truncation; formula appendix stays engineer/audit-only. Does **not** claim R/vendor LOF alignment.
- **DesignGeneration EvidenceBundle honesty**: when `facts.design_generation` is present, emit `:gate:design_formula_reference_only` always; `:gate:ccd_beyond_range` when CCD `beyond_range_detected`; `:gate:bbd_no_corners` for BBD. Forced `formula_reference` / `not_vendor_oracle=true`; customer retention under plot flood. Does **not** claim commercial CCD/BBD alignment.

## Honest gaps

- No commercial `vendor_oracle` alignment
- No pinned CRAN `rsm` / vendor LOF numeric goldens
- Factorial / fractional / Plackett–Burman design → worksheet delivered (`factorial_design_to_worksheet` via `doe_design_page`)
