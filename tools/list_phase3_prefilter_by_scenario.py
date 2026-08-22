#!/usr/bin/env python3
"""Print Phase 3 S1-S7 manual scenarios mapped to prefilter test names."""

from __future__ import annotations

import argparse

SCENARIOS: dict[str, list[str]] = {
    "S1 — KM 长表跨页 (engineer/en-US)": [
        "pdf_reliability_km_long_table_survival_table_spans_pages_en_us",
        "representative_reliability_km_three_report_profiles_localize_without_cross_language_leak",
        "representative_reliability_weibull_three_report_profiles_localize_without_cross_language_leak",
        "representative_reliability_lognormal_three_report_profiles_localize_without_cross_language_leak",
        "pdf_reliability_km_and_weibull_localize_to_en_us_without_chinese_leak",
        "pdf_reliability_km_and_weibull_cross_template_table_visibility_and_en_us_locale",
    ],
    "S2 — 中文 DOE 长表 (zh-CN / path B)": [
        "representative_doe_ccd_three_report_profiles_localize_without_cross_language_leak",
        "representative_doe_bbd_three_report_profiles_localize_without_cross_language_leak",
        "pdf_doe_ccd_and_bbd_design_localize_to_en_us_without_chinese_leak",
        "pdf_doe_ccd_and_bbd_cross_template_table_visibility_and_en_us_locale",
        "pdf_doe_ccd_k4_long_design_matrix_spans_pages_en_us",
        "customer_keeps_ccd_bbd_design_limiting_evidence_under_truncation",
    ],
    "S3 — Graph 英文长标题 (en-US)": [
        "representative_graph_scatter_faceted_three_report_profiles_localize_without_cross_language_leak",
        "representative_graph_bar_faceted_three_report_profiles_localize_without_cross_language_leak",
        "representative_graph_hexbin_faceted_three_report_profiles_localize_without_cross_language_leak",
        "representative_graph_density_faceted_three_report_profiles_localize_without_cross_language_leak",
        "pdf_graph_builder_faceted_scatter_localizes_to_en_us_without_chinese_leak",
        "pdf_graph_scatter_faceted_cross_template_plot_visibility_and_en_us_locale",
        "pdf_graph_bar_faceted_cross_template_plot_visibility_and_en_us_locale",
        "pdf_graph_builder_faceted_hexbin_localizes_to_en_us_without_chinese_leak",
        "pdf_hexbin_rectangular_bins_gate_localizes_and_audit_evidence_carries_label_text_id",
        "pdf_graph_hexbin_faceted_cross_template_plot_visibility_and_en_us_locale",
        "pdf_graph_builder_faceted_bar_and_density_localize_to_en_us_without_chinese_leak",
        "pdf_density_curve_not_discrete_marks_gate_localizes_and_audit_evidence_carries_label_text_id",
        "pdf_graph_density_faceted_cross_template_plot_visibility_and_en_us_locale",
        "graph_builder_faceted_page_titles_localize_to_en_us",
        "customer_keeps_hexbin_rectangular_bins_limiting_evidence_under_truncation",
        "customer_keeps_density_curve_not_discrete_marks_limiting_evidence_under_truncation",
    ],
    "S4 — 三模板差异 (warranty strata)": [
        "representative_warranty_strata_three_report_profiles_localize_without_cross_language_leak",
        "representative_warranty_summary_three_report_profiles_localize_without_cross_language_leak",
        "representative_warranty_exposure_gate_three_report_profiles_localize_without_cross_language_leak",
        "pdf_warranty_summary_cross_template_table_visibility_and_en_us_locale",
        "pdf_warranty_cross_template_table_visibility_and_en_us_locale",
        "pdf_warranty_invalid_exposure_localizes_to_en_us_without_chinese_leak",
        "pdf_warranty_exposure_gate_localizes_and_audit_evidence_carries_label_text_id",
        "pdf_warranty_exposure_cross_template_table_visibility_and_en_us_locale",
        "pdf_warranty_exposure_column_override_localizes_to_en_us_without_chinese_leak",
        "representative_warranty_exposure_column_override_three_report_profiles_localize_without_cross_language_leak",
        "pdf_warranty_strata_three_templates_manifest_facts_hash_match_en_us",
        "customer_keeps_warranty_exposure_gate_evidence_under_truncation",
        "usesWarrantyExposureGateInterpretationBullet",
        "warranty_exposure_diag_localizes_to_en_us",
        "customer_keeps_cif_fine_gray_warranty_strata_limiting_evidence_under_truncation",
    ],
    "S5 — 空图 / 全 excluded": [
        "pdf_empty_chart_renders_localized_no_data_message",
        "pdf_graph_scatter_all_excluded_renders_localized_no_data_en_us",
    ],
    "S6 — audit 证据附录多页": [
        "pdf_audit_km_long_table_evidence_appendix_spans_pages_en_us",
        "representative_johnson_capability_three_report_profiles_localize_without_cross_language_leak",
        "pdf_johnson_capability_gate_localizes_and_audit_evidence_carries_label_text_id",
        "pdf_johnson_spec_outside_support_localizes_to_en_us_without_chinese_leak",
        "pdf_johnson_spec_outside_cross_template_table_visibility_and_en_us_locale",
        "representative_johnson_spec_outside_three_report_profiles_localize_without_cross_language_leak",
        "pdf_johnson_capability_cross_template_table_visibility_and_en_us_locale",
        "pdf_box_cox_invalid_spec_limit_localizes_to_en_us_without_chinese_leak",
        "pdf_box_cox_invalid_spec_limit_cross_template_table_visibility_and_en_us_locale",
        "pdf_box_cox_spec_limit_gate_localizes_and_audit_evidence_carries_label_text_id",
        "representative_box_cox_three_report_profiles_localize_without_cross_language_leak",
        "customer_keeps_box_cox_limiting_evidence_under_truncation",
        "box_cox_spec_limit_diag_localizes_to_en_us",
        "representative_rsm_lof_three_report_profiles_localize_without_cross_language_leak",
        "pdf_rsm_lof_gate_localizes_and_audit_evidence_carries_label_text_id",
        "pdf_rsm_lof_cross_template_table_visibility_and_en_us_locale",
        "customer_keeps_rsm_lof_limiting_evidence_under_truncation",
        "customer_keeps_capability_gate_limiting_evidence_under_truncation",
        "audit_json_doe_design_generation_carries_label_text_ids",
        "pdf_evidence_appendix_uses_localized_label_text_ids",
        "customer_keeps_johnson_spec_limit_evidence_under_truncation",
        "usesJohnsonSpecLimitGateInterpretationBullet",
        "usesBoxCoxSpecLimitGateInterpretationBullet",
        "box_cox_service_skips_capability_on_invalid_spec_limits",
        "box_cox_service_skips_capability_on_inverted_spec_limits",
        "johnson_spec_outside_support_skips_overall_capability",
    ],
    "S7 — Unicode 列名 / PinLength": [
        "representative_normal_capability_three_report_profiles_localize_without_cross_language_leak",
        "representative_nonnormal_capability_three_report_profiles_localize_without_cross_language_leak",
        "pdf_normal_capability_stability_gate_localizes_and_audit_evidence_carries_label_text_id",
        "pdf_normal_capability_cross_template_table_visibility_and_en_us_locale",
        "pdf_pinlength_capability_unicode_columns_localize_without_cross_language_leak",
        "display_formatting_handles_empty_and_non_finite",
        "nonnormal_capability_phase6_test",
    ],
}

GLOBAL = [
    "representative_vertical_slice_reports_localize_without_cross_language_leak",
    "linguist_mirror_matches_catalog_and_qm_loads",
    "manifest_matches_document_identity_fields",
    "merge_verapdf_result_keeps_pdfua_unsupported",
    "merge_pac_result_never_validates_pdfua_even_on_exit_zero",
    "after_pdf_hook_keeps_pdfua_unsupported_when_unset",
]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--scenario-id",
        metavar="S4",
        help="Print only one scenario (S1-S7)",
    )
    args = parser.parse_args()

    print("# Phase 3 scenario -> prefilter tests (84 mapped; section 3.1 has 61 numbered items)")
    print("# Full section 3.1: python tools/list_phase3_prefilter_tests.py")
    print("# Qt Creator targets: python tools/list_qt_creator_test_targets.py --scenario-id S4 --by-target\n")

    if args.scenario_id:
        sid = args.scenario_id.strip().upper()
        if not sid.startswith("S"):
            sid = "S" + sid
        matched = False
        for scenario, tests in SCENARIOS.items():
            if scenario.startswith(sid + " "):
                print(f"## {scenario}")
                for name in tests:
                    print(f"  - {name}")
                print(f"\n# Scenario {sid} tests: {len(tests)}")
                matched = True
                break
        if not matched:
            print(f"ERROR: unknown scenario id {args.scenario_id!r}", flush=True)
            return 1
        return 0

    print("## Global (run before any S1-S7 scenario)")
    for name in GLOBAL:
        print(f"  - {name}")
    print()
    for scenario, tests in SCENARIOS.items():
        print(f"## {scenario}")
        for name in tests:
            print(f"  - {name}")
        print()
    total = len(GLOBAL) + sum(len(v) for v in SCENARIOS.values())
    print(f"# Scenario-mapped tests: {total} (section 3.1 numbered list has 61 items; deepen adds 37 outside 3.1)")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
