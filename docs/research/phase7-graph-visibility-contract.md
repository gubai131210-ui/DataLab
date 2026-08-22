# Phase 7 — Controlled Graph Builder visibility contract

Status: **partial**

## Delivered

- `hidden_rows` vs `excluded_rows` as **separate** vectors on `AnalysisConfiguration`
- Domain `summarize_row_visibility` — never merges into one bool
- Dual-wired geoms (analysis excluded-only; display drops hidden): scatter, bar, pie, density, hexbin, violin, interval, bubble, probability, ecdf, matrix, marginal, parallel, contour, **correlation, heatmap, time_series, area**
- **Source-row linkage (aggregated)**: `member_source_rows` + `chart_interaction::resolve_selected_source_rows`; **interval / bar / violin / pie / category-heatmap / hexbin**; bar+violin+pie+hexbin hit-test; UI emit uses resolver
- Dual-line Facts: `analysis_n` / `analysis_category_count`
- **UI**: Data menu “隐藏选中行（仅显示）” / “清除隐藏标记”; worksheet blue highlight vs yellow exclude; `base_configuration` passes both; context dock shows both counts
- **Graph properties panel**: read-only visibility contract banner (`row_visibility_banner`); fed from `page_renderer` via `AnalysisChartWidget::set_row_visibility_summary`
- Report provenance + en-US localization of excluded/hidden bullets + section titles
- **Chart-in-report deepen**: `ReportProfile::max_plots` (customer=1 / engineer=8 / audit=16); `EvidenceKind::plot` refs; visible plot subtitle
- **Controlled facet**: scatter, bar, density, interval, violin, hexbin, contour, matrix, parallel, time_series/area, bubble, probability, ecdf, marginal, **correlation**, **heatmap** (`facet_column` + `facet_max_panels` 1..12; correlation cells never invent per-cell member rows; category-heatmap keeps members)
- **Density honesty**: KDE curve is not discrete marks — `source_rows` / `member_source_rows` cleared; diagnostic `density_curve_not_discrete_marks`（catalog `diag.density_curve_not_discrete_marks`；不再硬编码 en-US）
- **Bar faceted report chain** (Phase 7 deepen): `representative_graph_bar_faceted_three_report_profiles_*` + `pdf_graph_bar_faceted_cross_template_*`（hidden/excluded 诚实诊断 + customer max_plots=1）
- **Scatter faceted report chain** (Phase 7 deepen): `representative_graph_scatter_faceted_three_report_profiles_*` + `pdf_graph_scatter_faceted_cross_template_*`（Display N / Analysis N / hidden 计数 + row_visibility_contract）
- **Hexbin report chain** (Phase 7 deepen): `:gate:hexbin_rectangular_bins` + `representative_graph_hexbin_faceted_three_report_profiles_*` + `pdf_hexbin_rectangular_bins_gate_*` + `pdf_graph_hexbin_faceted_cross_template_*`（hidden/excluded 计数 + rectangular-bin 诚实诊断）+ `report_profile_phase1_test::customer_keeps_hexbin_rectangular_bins_limiting_evidence_under_truncation`
- **Density report chain** (Phase 7 deepen): `:gate:density_curve_not_discrete_marks` + `representative_graph_density_faceted_three_report_profiles_*` + `pdf_density_curve_not_discrete_marks_gate_*` + `pdf_graph_density_faceted_cross_template_*`（Display N / Analysis N / hidden + KDE 诚实诊断）+ `report_profile_phase1_test::customer_keeps_density_curve_not_discrete_marks_limiting_evidence_under_truncation`
- Tests: `graph_visibility_phase7_test`, `report_profile_phase1_test` chart embed slot, `chart_interaction_test` member-row resolve
- **Report three-template guard** (Phase 3): `representative_graph_scatter_faceted_three_report_profiles_localize_without_cross_language_leak` — customer max_plots=1 vs engineer/audit multi-panel; complements scatter/bar/hexbin/density `pdf_*` + `representative_graph_*_faceted_three_report_profiles_*` + hexbin/density EvidenceBundle gate PDF/audit tests

## Honest gaps

- Facet coverage is complete for primary Graph Builder geoms; free-form Tableau canvas remains out of scope
- Correlation-matrix heatmap cells are not observation strata (no fake per-cell member rows)
- Hexbin remains rectangular bins (product name Hexbin; not true hexagonal tessellation)
- **EvidenceBundle honesty**: Hexbin pages emit early `:gate:hexbin_rectangular_bins` (`limiting`, `binning=rectangular`, `hexagonal_tessellation=false`, forced `formula_reference`); customer templates retain under plot flood. Diagnostic `hexbin_rectangular_bins` localizes to en-US.
- No free-form Tableau canvas (intentionally out of scope)
