# Phase 3 — UI menu Linguist (`DataLabUi`)

Status: **partial** (power / sample-size + forecast MAPE·MASE + regression R²/VIF + ANOVA/Tukey deepen 2026-08-22)

## Delivered

- `datalab::ui::ui_tr` → Qt context `DataLabUi`
- Main chrome menus/actions (File/Edit/Data/View/Help + common commands) go through `ui_tr`
- Analysis top menus / groups hashed on zh_CN source, displayed via `ui_tr`
- Analysis command `menu_label` / dialog title also call `ui_tr` (English present only when listed in `translations/ui_menu_strings.json`)
- **Analysis command catalog**: all 129 command menu/title/group labels covered in `ui_menu_strings.json`; `.ts`/`.qm` synced
- **Analysis setup dialog**: role labels, input labels, choice labels, and dialog chrome resolve via `ui_tr`
- Helpers: `tools/merge_analysis_command_ui_strings.py`, `tools/merge_dialog_role_ui_strings.py`
- **Fixed interpretation bilingual deepen**: reliability/DOE/capability-gate + ~54 DOE/MSA/reliability/RSM/EDA honesty bullets
- **Dynamic interpretation templates**: KM/warranty/evidence; DOE significant-terms / factors+centers; gage/MSA/KM/mode-fit/CIF; **SPC OOC / special-cause / Pareto / contour hold**; **σ / t·Z / normality / attribute**; **Grubbs/Dixon; Pearson/Spearman/partial/covariance; binomial OC; TOST equivalence; ANOM; ANOVA follow-up**; **one/two proportion; one/two Poisson-rate; Box-Cox λ/N/SD**; **power/sample-size conclusion lines + advice; forecast MAPE summary + MASE advice**; **regression R² / VIF / flagged-obs / residual advice; ANOVA significant terms + no-significant + Tukey** (reuses `power_table_empty` / `power_invalid_config` / `forecast_table_empty`)
- Exact SPC/DOE: EWMA/CUSUM/no-auto-anomaly/combine-field/RSM contour hold0
- `tools/sync_report_linguist.py` merges ReportCatalog + DataLabUi into `.ts` / `.qm`
- Test: `report_locale_phase3_test` (incl. `dynamic_sigma_ttest_normality_attr_*`, `dynamic_outlier_corr_oc_equiv_anom_*`, `dynamic_prop_poisson_boxcox_*`, `dynamic_power_forecast_interp_templates_localize_to_en_us`, `dynamic_forecast_mape_regression_anova_interp_templates_localize_to_en_us`)

## Honest gaps

- A few specialty input symbols (`α` / `α enter` / `α remove`) keep Greek as-is
- Input **placeholders** and most **help** tooltips still zh_CN
- Remaining dynamically composed interpretation bullets (other inference numeric lines, some rule-action free text beyond the MAPE/regression/ANOVA·Tukey set above) still zh_CN under en-US
- English long-text layout still needs human acceptance
- Status bar / remaining QMessageBox strings outside the setup dialog may still be Chinese
- This deepen does **not** claim PDF/A or PDF/UA compliance
