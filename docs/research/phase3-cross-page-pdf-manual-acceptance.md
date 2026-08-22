# Phase 3：跨页 PDF 人工验收 Checklist

> 状态：2026-08-22 交付；自动化预筛 **61 项**（§3.1）+ **13/13 cross_template PDF** 已落地，待 Qt Creator 本地全绿 + §6 签署  
> 性质：**人工签署**清单；自动化预筛见 §3.1（`python tools/list_phase3_prefilter_tests.py` 打印用例名）、`representative_vertical_slice_reports_localize_without_cross_language_leak` 与各 `pdf_*` 字节扫描测试。  
> **不等于** PDF/A 或 PDF/UA 合规证明；默认 manifest 仍为 `not_validated` / `unsupported`。

## 1. 目的

在 Qt Creator / 桌面环境中，对 **真实导出 PDF** 做肉眼与布局验收，补齐自动化字节扫描无法覆盖的项：

- 跨页表头重复与行对齐
- 长英文标题/表头换行与截断
- 中英文各自 **零混语**（可见 chrome 层）
- 空图、诊断失败、证据附录可读性
- customer / engineer / audit 三模板差异（非仅“少几张图”）

## 2. 前置条件

- [ ] 本地 Release/Debug 构建可通过 Qt Creator 运行 DataLab
- [ ] 已执行 `python tools/sync_report_linguist.py --lrelease "<Qt>/bin/lrelease.exe"`（若刚改过 catalog）
- [ ] 了解报告语言在 **报告模板对话框** 独立设置（`Profile.locale`，不受系统 locale 覆盖）
- [ ] 导出路径可写；建议每次验收使用新目录

## 3. 自动化预筛（应先全绿）

在 Qt Creator 中运行 `report_export_phase2_test` 子集：

| 测试 | 覆盖 |
|------|------|
| `representative_vertical_slice_reports_localize_without_cross_language_leak` | **13 竖切**（CCD/BBD/Graph scatter/KM/保修/保修分层/Box-Cox/Johnson/非正态/正态能力/RSM/Weibull/Lognormal）可见层 en CJK + zh catalog leak guard + 显式禁词 |
| `representative_warranty_strata_three_report_profiles_localize_without_cross_language_leak` | customer/engineer/audit × en-US/zh-CN；保修分层表可见性 + 证据附录 + 混语 guard |
| `representative_johnson_capability_three_report_profiles_localize_without_cross_language_leak` | customer/engineer/audit × en-US/zh-CN；Johnson 门禁诊断 + limiting evidence + 混语 guard |
| `representative_rsm_lof_three_report_profiles_localize_without_cross_language_leak` | customer/engineer/audit × en-US/zh-CN；RSM LOF 门禁 + 表/附录可见性 + 混语 guard |
| `representative_doe_ccd_three_report_profiles_localize_without_cross_language_leak` | customer/engineer/audit × en-US/zh-CN；DOE CCD design_formula 门禁 + 设计矩阵可见性 + 混语 guard |
| `representative_doe_bbd_three_report_profiles_localize_without_cross_language_leak` | customer/engineer/audit × en-US/zh-CN；DOE BBD design_formula + bbd_no_corners 门禁 + 混语 guard |
| `representative_nonnormal_capability_three_report_profiles_localize_without_cross_language_leak` | customer/engineer/audit × en-US/zh-CN；非正态能力 stability 门禁 + 风险诊断 + 混语 guard |
| `representative_normal_capability_three_report_profiles_localize_without_cross_language_leak` | customer/engineer/audit × en-US/zh-CN；正态能力 pass/fail block + stability 门禁 + 混语 guard |
| `representative_box_cox_three_report_profiles_localize_without_cross_language_leak` | customer/engineer/audit × en-US/zh-CN；Box-Cox 假设诚实 bullet + 变换后能力表可见性 + 混语 guard |
| `representative_reliability_km_three_report_profiles_localize_without_cross_language_leak` | customer/engineer/audit × en-US/zh-CN；KM 生存表 + audit formula ref + 混语 guard |
| `representative_reliability_weibull_three_report_profiles_localize_without_cross_language_leak` | customer/engineer/audit × en-US/zh-CN；Weibull 表/二参数 info 诊断可见性 + 混语 guard |
| `representative_reliability_lognormal_three_report_profiles_localize_without_cross_language_leak` | customer/engineer/audit × en-US/zh-CN；Lognormal 表/二参数 info 诊断可见性 + 混语 guard |
| `representative_graph_scatter_faceted_three_report_profiles_localize_without_cross_language_leak` | customer/engineer/audit × en-US/zh-CN；分面散点 max_plots 截断 + Facet 参数 + 混语 guard |
| `representative_warranty_summary_three_report_profiles_localize_without_cross_language_leak` | customer/engineer/audit × en-US/zh-CN；scalar 保修摘要表 + 双 warranty gate + 混语 guard |
| `pdf_warranty_summary_page_localizes_to_en_us_without_chinese_leak` | 保修摘要 audit PDF |
| `pdf_warranty_summary_cross_template_table_visibility_and_en_us_locale` | scalar 保修摘要三模板 PDF 字节扫描 |
| `pdf_warranty_cross_template_table_visibility_and_en_us_locale` | 三模板 PDF 字节扫描 + 表裁剪 |
| `pdf_warranty_strata_three_templates_manifest_facts_hash_match_en_us` | S4 manifest `facts_hash` 三模板一致 |
| `pdf_doe_ccd_and_bbd_design_localize_to_en_us_without_chinese_leak` | DOE 设计页 |
| `pdf_doe_ccd_and_bbd_cross_template_table_visibility_and_en_us_locale` | DOE CCD/BBD 三模板 PDF 字节扫描 |
| `pdf_doe_ccd_k4_long_design_matrix_spans_pages_en_us` | S2 path B 长设计矩阵跨页预筛（≥2 PDF 页；en-US + zh-CN） |
| `audit_json_doe_design_generation_carries_label_text_ids` | DOE audit PDF+JSON 证据 |
| `pdf_graph_builder_faceted_*` / `pdf_graph_builder_pie_*` 系列 | Graph Builder 分面 geom + 饼图 + 类别热图第二路径 |
| `pdf_reliability_km_and_weibull_localize_to_en_us_without_chinese_leak` | KM/Weibull/Lognormal |
| `pdf_reliability_km_long_table_survival_table_spans_pages_en_us` | S1 长表跨页预筛（≥2 PDF 页） |
| `pdf_audit_km_long_table_evidence_appendix_spans_pages_en_us` | S6 audit 证据附录 + 长表跨页预筛（≥2 PDF 页） |
| `pdf_reliability_km_and_weibull_cross_template_table_visibility_and_en_us_locale` | KM/Weibull 三模板 PDF 字节扫描 |
| `pdf_johnson_capability_cross_template_table_visibility_and_en_us_locale` | Johnson 三模板 PDF 字节扫描 |
| `pdf_box_cox_cross_template_table_visibility_and_en_us_locale` | Box-Cox 三模板 PDF 字节扫描 |
| `pdf_graph_scatter_faceted_cross_template_plot_visibility_and_en_us_locale` | Graph 分面散点三模板 PDF 字节扫描 |
| `pdf_rsm_lof_cross_template_table_visibility_and_en_us_locale` | RSM LOF 三模板 PDF 字节扫描 |
| `pdf_johnson_capability_gate_localizes_and_audit_evidence_carries_label_text_id` | Johnson 门禁 |
| `pdf_normal_capability_stability_gate_localizes_and_audit_evidence_carries_label_text_id` | 正态能力 stability 门禁 + audit JSON |
| `pdf_normal_capability_cross_template_table_visibility_and_en_us_locale` | 正态能力三模板 PDF 字节扫描 |
| `pdf_pinlength_capability_unicode_columns_localize_without_cross_language_leak` | S7 PinLength unicode 列 zh/en PDF 预筛 |
| `pdf_nonnormal_capability_stability_gate_localizes_and_audit_evidence_carries_label_text_id` | 非正态能力 Weibull Z-score + stability 门禁 + audit JSON |
| `pdf_empty_chart_renders_localized_no_data_message` | 空图 catalog 句 |
| `pdf_graph_scatter_all_excluded_renders_localized_no_data_en_us` | S5 Graph 全 excluded 空图 |
| `display_formatting_handles_empty_and_non_finite` | S7 Unicode/NaN/inf 显示兜底 |
| `linguist_mirror_matches_catalog_and_qm_loads` | catalog↔Linguist 镜像 |

预筛 **失败则先修回归**，再进入人工 PDF 验收。

### 3.1 完整 Qt Creator 预筛列表（推荐顺序）

在 Qt Creator **Tests** 视图中对 `report_export_phase2_test` / `report_locale_phase3_test` 运行下列用例（可分批；**全部通过**后再做 §5 人工 PDF）。

**A. 混语 guard（必跑）**

1. `representative_vertical_slice_reports_localize_without_cross_language_leak` — 13 竖切 bidirectional visible-layer
2. `representative_warranty_strata_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en/zh（保修分层）
3. `representative_johnson_capability_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en/zh（Johnson 门禁）
4. `representative_rsm_lof_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en/zh（RSM LOF 门禁）
5. `representative_doe_ccd_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en/zh（DOE CCD 设计门禁）
6. `representative_doe_bbd_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en/zh（DOE BBD 设计门禁）
7. `representative_nonnormal_capability_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en/zh（非正态能力门禁）
8. `representative_normal_capability_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en/zh（正态能力 stability 门禁）
9. `representative_box_cox_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en/zh（Box-Cox 假设诚实）
10. `representative_reliability_km_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en/zh（Kaplan-Meier）
11. `representative_reliability_weibull_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en/zh（Weibull）
12. `representative_reliability_lognormal_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en/zh（Lognormal）
13. `representative_graph_scatter_faceted_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en/zh（Graph 分面散点）
14. `representative_warranty_summary_three_report_profiles_localize_without_cross_language_leak` — customer/engineer/audit × en/zh（保修摘要 scalar）
15. `linguist_mirror_matches_catalog_and_qm_loads` — 在 `report_locale_phase3_test` 中

**B. 报告产品 / 证据 / 合规诚实**

16. `manifest_matches_document_identity_fields`
17. `merge_verapdf_result_keeps_pdfua_unsupported`
18. `merge_pac_result_never_validates_pdfua_even_on_exit_zero`
19. `after_pdf_hook_keeps_pdfua_unsupported_when_unset`
20. `audit_json_doe_design_generation_carries_label_text_ids`
21. `pdf_evidence_appendix_uses_localized_label_text_ids`
22. `pdf_empty_chart_renders_localized_no_data_message`

**C. DOE / RSM**

23. `pdf_doe_ccd_and_bbd_design_localize_to_en_us_without_chinese_leak`
24. `pdf_doe_ccd_and_bbd_cross_template_table_visibility_and_en_us_locale`
25. `pdf_doe_ccd_k4_long_design_matrix_spans_pages_en_us` — S2 path B 跨页预筛（≥2 PDF 页；含 en-US + zh-CN 混语 guard）
26. `pdf_rsm_lof_gate_localizes_and_audit_evidence_carries_label_text_id`
27. `pdf_rsm_lof_cross_template_table_visibility_and_en_us_locale`

**D. 可靠性 / 保修**

28. `pdf_reliability_km_and_weibull_localize_to_en_us_without_chinese_leak`
29. `pdf_reliability_km_long_table_survival_table_spans_pages_en_us`
30. `pdf_audit_km_long_table_evidence_appendix_spans_pages_en_us` — S6 audit 附录 + 长表跨页预筛
31. `pdf_reliability_km_and_weibull_cross_template_table_visibility_and_en_us_locale`
32. `pdf_warranty_summary_page_localizes_to_en_us_without_chinese_leak`
33. `pdf_warranty_summary_cross_template_table_visibility_and_en_us_locale`
34. `pdf_warranty_strata_tables_localize_to_en_us_without_chinese_leak`
35. `pdf_warranty_cross_template_table_visibility_and_en_us_locale`
36. `pdf_warranty_strata_three_templates_manifest_facts_hash_match_en_us` — S4 manifest `facts_hash` 三模板一致

**E. 非正态能力 / Box-Cox / Johnson**

37. `pdf_johnson_capability_gate_localizes_and_audit_evidence_carries_label_text_id`
38. `pdf_johnson_capability_cross_template_table_visibility_and_en_us_locale`
39. `pdf_normal_capability_stability_gate_localizes_and_audit_evidence_carries_label_text_id`
40. `pdf_normal_capability_cross_template_table_visibility_and_en_us_locale`
41. `pdf_nonnormal_capability_stability_gate_localizes_and_audit_evidence_carries_label_text_id`
42. `pdf_nonnormal_capability_cross_template_table_visibility_and_en_us_locale`
43. `pdf_box_cox_honesty_localizes_to_en_us_without_chinese_leak`
44. `pdf_box_cox_cross_template_table_visibility_and_en_us_locale`
45. `nonnormal_capability_phase6_test` — 在 `nonnormal_capability_phase6_test` 目标中（域门禁；与 #41 PDF 互补）

**E′. Box-Cox 规格限加深（Phase 6；不在 §3.1 61 计数内，S6 路径 C 预筛推荐）**

- `pdf_box_cox_invalid_spec_limit_localizes_to_en_us_without_chinese_leak`
- `pdf_box_cox_invalid_spec_limit_cross_template_table_visibility_and_en_us_locale`
- `pdf_box_cox_spec_limit_gate_localizes_and_audit_evidence_carries_label_text_id`
- `report_profile_phase1_test::customer_keeps_box_cox_limiting_evidence_under_truncation`
- `report_locale_phase3_test::box_cox_spec_limit_diag_localizes_to_en_us`
- `quality_statistics_test::box_cox_service_skips_capability_on_invalid_spec_limits`
- `quality_statistics_test::box_cox_service_skips_capability_on_inverted_spec_limits`

**F. Graph Builder 分面 / 饼图 en-US PDF 字节扫描**

46. `pdf_graph_builder_faceted_scatter_localizes_to_en_us_without_chinese_leak`
47. `pdf_graph_scatter_faceted_cross_template_plot_visibility_and_en_us_locale`
48. `pdf_graph_builder_faceted_bar_and_density_localize_to_en_us_without_chinese_leak`
49. `pdf_graph_builder_faceted_interval_and_violin_localize_to_en_us_without_chinese_leak`
50. `pdf_graph_builder_faceted_hexbin_localizes_to_en_us_without_chinese_leak`
51. `pdf_graph_builder_faceted_contour_and_matrix_localize_to_en_us_without_chinese_leak`
52. `pdf_graph_builder_faceted_bubble_and_time_series_localize_to_en_us_without_chinese_leak`
53. `pdf_graph_builder_faceted_area_parallel_and_marginal_localize_to_en_us_without_chinese_leak`
54. `pdf_graph_builder_faceted_probability_and_ecdf_localize_to_en_us_without_chinese_leak`
55. `pdf_graph_builder_faceted_correlation_and_heatmap_localize_to_en_us_without_chinese_leak`
56. `pdf_graph_builder_faceted_category_heatmap_localizes_to_en_us_without_chinese_leak`
57. `pdf_graph_builder_pie_localizes_to_en_us_without_chinese_leak`

**F′. Graph Builder 诚实性 / 三模板加深（Phase 7；不在 §3.1 61 计数内，S3 预筛推荐）**

- **分面散点图（hidden/excluded）**
  - `representative_graph_scatter_faceted_three_report_profiles_localize_without_cross_language_leak`
  - `pdf_graph_scatter_faceted_cross_template_plot_visibility_and_en_us_locale`
- **分面条形图（hidden/excluded）**
  - `representative_graph_bar_faceted_three_report_profiles_localize_without_cross_language_leak`
  - `pdf_graph_bar_faceted_cross_template_plot_visibility_and_en_us_locale`
- **Hexbin 矩形分箱**
  - `representative_graph_hexbin_faceted_three_report_profiles_localize_without_cross_language_leak`（含 hidden + excluded 双口径）
  - `pdf_hexbin_rectangular_bins_gate_localizes_and_audit_evidence_carries_label_text_id`
  - `pdf_graph_hexbin_faceted_cross_template_plot_visibility_and_en_us_locale`（含 hidden/excluded 计数）
  - `report_profile_phase1_test::customer_keeps_hexbin_rectangular_bins_limiting_evidence_under_truncation`
- **密度图 KDE 非离散 mark**
  - `representative_graph_density_faceted_three_report_profiles_localize_without_cross_language_leak`（含 hidden 双口径）
  - `pdf_density_curve_not_discrete_marks_gate_localizes_and_audit_evidence_carries_label_text_id`
  - `pdf_graph_density_faceted_cross_template_plot_visibility_and_en_us_locale`（含 hidden 双口径）
  - `report_profile_phase1_test::customer_keeps_density_curve_not_discrete_marks_limiting_evidence_under_truncation`

**E″. Johnson 规格越界 / 保修暴露量（Phase 5–6；不在 §3.1 61 计数内）**

- Johnson spec-outside：`representative_johnson_spec_outside_three_report_profiles_*` / `pdf_johnson_spec_outside_*` / `report_profile_phase1_test::customer_keeps_johnson_spec_limit_evidence_under_truncation`（S6 路径 D）
- 保修暴露量：`representative_warranty_exposure_gate_three_report_profiles_*` / `representative_warranty_exposure_column_override_three_report_profiles_*` / `pdf_warranty_invalid_exposure_localizes_to_en_us_without_chinese_leak` / `pdf_warranty_exposure_gate_localizes_and_audit_evidence_carries_label_text_id` / `pdf_warranty_exposure_cross_template_table_visibility_and_en_us_locale` / `pdf_warranty_exposure_column_override_localizes_to_en_us_without_chinese_leak` / `report_profile_phase1_test::customer_keeps_warranty_exposure_gate_evidence_under_truncation` / `interpretation_service_test::usesWarrantyExposureGateInterpretationBullet` / `report_locale_phase3_test::warranty_exposure_diag_localizes_to_en_us`（S4）；人工路径见 [`samples/phase0_baselines/warranty_exposure_manual_s4.md`](../../samples/phase0_baselines/warranty_exposure_manual_s4.md)

**G. Graph Builder 页题 locale 回归（非 PDF）**

58. `graph_builder_faceted_page_titles_localize_to_en_us` — 在 `report_locale_phase3_test` 中（16/16 分面页题）

**H. S7 Unicode / PinLength 预筛**

59. `pdf_pinlength_capability_unicode_columns_localize_without_cross_language_leak`

**I. S4–S7 补充预筛**

60. `display_formatting_handles_empty_and_non_finite` — Unicode/NaN/inf 显示兜底
61. `pdf_graph_scatter_all_excluded_renders_localized_no_data_en_us` — S5 Graph 全 excluded 空图

### 3.2 代表性竖切 × 测试追溯矩阵（13/13）

每条算法竖切由 **三层自动化** 互补覆盖（均不替代 §5 肉眼 PDF）：

| # | 竖切 | 混语 guard（engineer × en/zh） | 三模板 guard（customer/engineer/audit × en/zh） | PDF 字节扫描（en-US，多为 engineer/audit） |
|---|------|--------------------------------|-----------------------------------------------|---------------------------------------------|
| 1 | DOE CCD | `representative_vertical_slice_*` CCD 段 | `representative_doe_ccd_three_report_profiles_*` | `pdf_doe_ccd_and_bbd_*` CCD 部分 + `pdf_doe_ccd_and_bbd_cross_template_*` |
| 2 | DOE BBD | 同上 BBD 段 | `representative_doe_bbd_three_report_profiles_*` | 同上 BBD 部分 + `pdf_doe_ccd_and_bbd_cross_template_*` |
| 3 | RSM LOF | 同上 RSM 段 | `representative_rsm_lof_three_report_profiles_*` | `pdf_rsm_lof_gate_*` + `pdf_rsm_lof_cross_template_*` |
| 4 | Graph scatter（分面） | 同上 scatter 段 | `representative_graph_scatter_faceted_three_report_profiles_*` | `pdf_graph_builder_faceted_scatter_*` + `pdf_graph_scatter_faceted_cross_template_*` |
| 5 | KM | 同上 KM 段 | `representative_reliability_km_three_report_profiles_*` | `pdf_reliability_km_and_weibull_*` KM 部分 + `pdf_reliability_km_long_table_*` + `pdf_reliability_km_and_weibull_cross_template_*` KM 部分 |
| 6 | Weibull | 同上 Weibull 段 | `representative_reliability_weibull_three_report_profiles_*` | 同上 Weibull 部分 + `pdf_reliability_km_and_weibull_cross_template_*` Weibull 部分 |
| 7 | Lognormal | 同上 Lognormal 段 | `representative_reliability_lognormal_three_report_profiles_*` | 同上 Lognormal 部分 + `pdf_reliability_km_and_weibull_cross_template_*` Lognormal 部分 |
| 8 | 保修摘要（scalar） | 同上 warranty 段 | `representative_warranty_summary_three_report_profiles_*` | `pdf_warranty_summary_page_*` + `pdf_warranty_summary_cross_template_*` |
| 9 | 保修分层 | 同上 strata 段 | `representative_warranty_strata_three_report_profiles_*` | `pdf_warranty_strata_*` + `pdf_warranty_cross_template_*` |
| 10 | Box-Cox | 同上 Box-Cox 段 | `representative_box_cox_three_report_profiles_*` | `pdf_box_cox_honesty_*` + `pdf_box_cox_cross_template_*` |
| 11 | Johnson 能力（gated） | 同上 Johnson 段 | `representative_johnson_capability_three_report_profiles_*` | `pdf_johnson_capability_gate_*` + `pdf_johnson_capability_cross_template_*` |
| 12 | 非正态能力 | 同上 nonnormal 段 | `representative_nonnormal_capability_three_report_profiles_*` | `pdf_nonnormal_capability_stability_gate_*` + `pdf_nonnormal_capability_cross_template_*` + `nonnormal_capability_phase6_test`（域） |
| 13 | 正态能力（stability gate） | 同上 normal 段 | `representative_normal_capability_three_report_profiles_*` | `pdf_normal_capability_stability_gate_*` + `pdf_normal_capability_cross_template_*` |

**说明：** `representative_vertical_slice_reports_localize_without_cross_language_leak` 单测覆盖上表全部 13 竖切的 engineer 混语 guard；三模板 guard 为 **14 个独立测试**（含 warranty 摘要与分层各一）。**13/13 竖切** 均有 companion `pdf_*_cross_template_*` 或等价三模板 PDF 字节扫描（Graph scatter 为 plot/parameter 裁剪；KM 长表另含 S1 跨页预筛）。

### 3.3 三模板 PDF 字节扫描索引（13/13）

| # | 竖切 | `pdf_*_cross_template_*` 测试名 |
|---|------|----------------------------------|
| 1–2 | DOE CCD + BBD | `pdf_doe_ccd_and_bbd_cross_template_table_visibility_and_en_us_locale` |
| 3 | RSM LOF | `pdf_rsm_lof_cross_template_table_visibility_and_en_us_locale` |
| 4 | Graph scatter（分面） | `pdf_graph_scatter_faceted_cross_template_plot_visibility_and_en_us_locale` |
| 5–7 | KM + Weibull + Lognormal | `pdf_reliability_km_and_weibull_cross_template_table_visibility_and_en_us_locale` |
| 8 | 保修摘要（scalar） | `pdf_warranty_summary_cross_template_table_visibility_and_en_us_locale` |
| 9 | 保修分层 | `pdf_warranty_cross_template_table_visibility_and_en_us_locale` |
| 10 | Box-Cox | `pdf_box_cox_cross_template_table_visibility_and_en_us_locale` |
| 11 | Johnson（gated） | `pdf_johnson_capability_cross_template_table_visibility_and_en_us_locale` |
| 12 | 非正态能力 | `pdf_nonnormal_capability_cross_template_table_visibility_and_en_us_locale` |
| 13 | 正态能力 | `pdf_normal_capability_cross_template_table_visibility_and_en_us_locale` |

**Qt Creator 操作提示**

- 测试类：`ReportExportPhase2Test`、`ReportLocalePhase3Test`、`NonNormalCapabilityPhase6Test`
- 过滤器：在 Tests 面板搜索 `representative_` 或 `pdf_graph_builder_faceted` 分批运行；或运行  
  `python tools/list_phase3_prefilter_tests.py` 打印 §3.1 全部 **61** 项；  
  `python tools/list_phase3_prefilter_by_scenario.py` 按 S1–S7 打印必跑子集
- **catalog 改动后**：本地执行  
  `python tools/sync_report_linguist.py --lrelease "D:\Qt\6.11.1\mingw_64\bin\lrelease.exe"`  
  （路径按本机 Qt 安装调整）
- 本清单 **不替代** §5 S1–S7 肉眼 PDF 验收

### 3.4 S1–S7 场景 → 预筛测试（子集索引）

完整 §3.1 共 **61** 项。各人工场景 **最低必跑** 子集见下表；可用  
`python tools/list_phase3_prefilter_by_scenario.py` 打印完整列表。

| 场景 | 必跑预筛（摘要） |
|------|------------------|
| **S1** | `pdf_reliability_km_long_table_*` + KM/Weibull/Lognormal `representative_reliability_*` + cross_template |
| **S2** | `representative_doe_*` + `pdf_doe_ccd_k4_long_design_matrix_spans_pages_en_us` |
| **S3** | `representative_graph_scatter_faceted_*` + `pdf_graph_builder_faceted_scatter_*` + cross_template |
| **S4** | `representative_warranty_strata_*` + `pdf_warranty_*_cross_template_*` + `pdf_warranty_strata_three_templates_manifest_facts_hash_match_en_us` + **暴露量加深** `representative_warranty_exposure_*` / `pdf_warranty_exposure_*` |
| **S5** | `pdf_empty_chart_*` + `pdf_graph_scatter_all_excluded_renders_localized_no_data_en_us` |
| **S6** | `pdf_audit_km_*` + Johnson/RSM/Box-Cox gate + `representative_rsm_lof_*` + `pdf_evidence_appendix_*` |
| **S7** | `pdf_pinlength_*` + `representative_normal/nonnormal_capability_*` + `display_formatting_*` |

**全局（任一场景前建议先绿）：** `representative_vertical_slice_*`、`linguist_mirror_*`、§3.1 **B** 段 manifest/合规诚实 5 项。

## 4. 通用导出步骤（每个场景重复）

1. 打开样本工作表或导入 CSV（见各场景）
2. 运行分析命令，确认 Session 有 OutputPage
3. **文件 → 导出报告**（或等价入口）
4. 选择模板：**customer / engineer / audit**（按场景）
5. 报告语言：**zh-CN** 或 **en-US**（按场景；勿依赖系统语言）
6. 导出 `.pdf` + `.manifest.json` + `.audit.json`
7. 打开 PDF，对照下方 Pass/Fail 项记录

**Fail 时记录：** 场景 ID、模板、locale、页码、截图路径、混语原文、是否阻塞发布。

## 5. 场景清单

### S1 — 超长统计表跨页（表头重复）

| 项 | 值 |
|----|-----|
| 分析 | 可靠性 → Kaplan–Meier（或 Weibull，含 **>40 行** 生存/参数表） |
| 数据 | [`samples/phase0_baselines/reliability_km_long_table.csv`](../../samples/phase0_baselines/reliability_km_long_table.csv)（55 步；步骤见 [`reliability_km_long_table.md`](../../samples/phase0_baselines/reliability_km_long_table.md)）或自建 ≥50 行 |
| 模板 | engineer |
| locale | en-US |

**自动化预筛（§3.1 #9–#12 + KM 长表 + cross_template）：**

- [ ] `pdf_reliability_km_long_table_survival_table_spans_pages_en_us` 全绿（≥2 PDF 页）
- [ ] `representative_reliability_km_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `representative_reliability_weibull_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `representative_reliability_lognormal_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `pdf_reliability_km_and_weibull_localize_to_en_us_without_chinese_leak` 全绿
- [ ] `pdf_reliability_km_and_weibull_cross_template_table_visibility_and_en_us_locale` 全绿

**Pass 标准：**

- [ ] `pdf_reliability_km_long_table_survival_table_spans_pages_en_us` 全绿（自动化预筛：≥2 PDF 页）
- [ ] 表格跨页时 **每页顶部重复表头**（列名可读、对齐）
- [ ] 行序连续；页脚页码递增
- [ ] 无整行被页边界拦腰切断（允许单元格内换行）
- [ ] en-US PDF **无中文 chrome**（列名/页题/参数摘要）
- [ ] manifest `locale_language_tag` = `en-US`；`facts_hash` 与 audit 一致

### S2 — 中文长标题 + 多表（zh-CN）

| 项 | 值 |
|----|-----|
| 分析 | DOE → 中心复合设计 (CCD) k=3 或 BBD k=3 |
| 数据 | 配置见 [`samples/phase0_baselines/doe_bbd_k3_factors.json`](../../samples/phase0_baselines/doe_bbd_k3_factors.json) 或 [`doe_ccd_k2_factors.json`](../../samples/phase0_baselines/doe_ccd_k2_factors.json)；**跨页**可选 CCD k=4 + `center_point_count≥8`（见 [`doe_manual_s2_zh_cn.md`](../../samples/phase0_baselines/doe_manual_s2_zh_cn.md)） |
| 模板 | engineer |
| locale | zh-CN |

**自动化预筛（§3.1 #5–#6 + #24–#25）：**

- [ ] `representative_doe_ccd_three_report_profiles_localize_without_cross_language_leak` 全绿（含 zh-CN 段）
- [ ] `representative_doe_bbd_three_report_profiles_localize_without_cross_language_leak` 全绿（含 zh-CN 段）
- [ ] `pdf_doe_ccd_and_bbd_cross_template_table_visibility_and_en_us_locale` 全绿（en-US 对照）
- [ ] `pdf_doe_ccd_k4_long_design_matrix_spans_pages_en_us` 全绿（S2 path B ≥2 PDF 页；含 zh-CN 无英文 catalog 泄漏）
- [ ] `customer_keeps_ccd_bbd_design_limiting_evidence_under_truncation` 全绿（customer 模板截断仍保留 DOE 设计门禁 evidence）

**Pass 标准：**

- [ ] 页题、设计信息表、设计矩阵表头均为中文 catalog 文案
- [ ] 长表跨页表头重复（同 S1）
- [ ] **无英文 catalog 泄漏**（如 `Design matrix`、`Central composite design (CCD)` 整句）
- [ ] BBD 诚实句（无角点/边界）为中文且不与证据门禁矛盾

### S3 — 英文长标题换行（en-US）

| 项 | 值 |
|----|-----|
| 分析 | Graph Builder → 分面 scatter / interval / hexbin / **contour / matrix**（≥2 面板）；步骤见 [`samples/phase0_baselines/graph_faceted_manual_s3.md`](../../samples/phase0_baselines/graph_faceted_manual_s3.md) |
| 模板 | engineer |
| locale | en-US |

**自动化预筛（§3.1 #13 + #43–#44）：**

- [ ] `representative_graph_scatter_faceted_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `pdf_graph_builder_faceted_scatter_localizes_to_en_us_without_chinese_leak` 全绿
- [ ] `pdf_graph_scatter_faceted_cross_template_plot_visibility_and_en_us_locale` 全绿
- [ ] （可选）`pdf_graph_builder_faceted_contour_and_matrix_*` 等 §3.1 F 段 geom 字节扫描

**F′ 加深预筛（§3.1 之外；S3 推荐）：**

- [ ] `representative_graph_bar_faceted_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `pdf_graph_bar_faceted_cross_template_plot_visibility_and_en_us_locale` 全绿
- [ ] `representative_graph_hexbin_faceted_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `pdf_hexbin_rectangular_bins_gate_localizes_and_audit_evidence_carries_label_text_id` 全绿
- [ ] `pdf_graph_hexbin_faceted_cross_template_plot_visibility_and_en_us_locale` 全绿
- [ ] `customer_keeps_hexbin_rectangular_bins_limiting_evidence_under_truncation` 全绿
- [ ] `representative_graph_density_faceted_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `pdf_density_curve_not_discrete_marks_gate_localizes_and_audit_evidence_carries_label_text_id` 全绿
- [ ] `pdf_graph_density_faceted_cross_template_plot_visibility_and_en_us_locale` 全绿
- [ ] `customer_keeps_density_curve_not_discrete_marks_limiting_evidence_under_truncation` 全绿

**Pass 标准：**

- [ ] 页题 `* (faceted)` 完整可见或合理换行，未被截断为乱码
- [ ] `Facet =` / `Response =` / `Variable =` 等 parameter 行可读
- [ ] 分面截断诊断（truncated levels）英文完整
- [ ] `Display N =` / `Analysis N =` / `hidden =` / `excluded =` 参数行可读（F′ scatter/bar/hexbin/density）
- [ ] **无中文** 页题/参数/诊断

### S4 — 三模板差异（同一 OutputPage）

| 项 | 值 |
|----|-----|
| 分析 | 保修摘要 **含分层表**（failure mode strata）；步骤见 [`samples/phase0_baselines/warranty_strata_manual_s4.md`](../../samples/phase0_baselines/warranty_strata_manual_s4.md) |
| locale | en-US |
| 模板 | customer → engineer → audit 各导出一次 |

**自动化预筛（§3.1 #3–#4 + #14 + §3.3 + #36）：**

- [ ] `representative_warranty_strata_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `representative_warranty_summary_three_report_profiles_*` 全绿（scalar 对照）
- [ ] `pdf_warranty_summary_cross_template_table_visibility_and_en_us_locale` 全绿
- [ ] `pdf_warranty_cross_template_table_visibility_and_en_us_locale` 全绿（分层对照）
- [ ] `pdf_warranty_strata_three_templates_manifest_facts_hash_match_en_us` 全绿（S4 manifest `facts_hash`）
- [ ] （任选 1 条非保修竖切）§3.3 中任意 `pdf_*_cross_template_*` 全绿，确认 customer/engineer/audit 裁剪一致

**E″ 暴露量加深预筛（§3.1 之外；S4 推荐）：**

- [ ] `representative_warranty_exposure_gate_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `representative_warranty_exposure_column_override_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `pdf_warranty_invalid_exposure_localizes_to_en_us_without_chinese_leak` 全绿
- [ ] `pdf_warranty_exposure_gate_localizes_and_audit_evidence_carries_label_text_id` 全绿
- [ ] `pdf_warranty_exposure_cross_template_table_visibility_and_en_us_locale` 全绿
- [ ] `pdf_warranty_exposure_column_override_localizes_to_en_us_without_chinese_leak` 全绿
- [ ] `customer_keeps_warranty_exposure_gate_evidence_under_truncation` 全绿
- [ ] `usesWarrantyExposureGateInterpretationBullet` 全绿
- [ ] `warranty_exposure_diag_localizes_to_en_us` 全绿
- [ ] `customer_keeps_cif_fine_gray_warranty_strata_limiting_evidence_under_truncation` 全绿（分层 + CIF/Fine-Gray 门禁 evidence 截断保留）

**Pass 标准：**

- [ ] 三次导出 `facts_hash` **相同**（manifest/audit 可比）
- [ ] customer：**隐藏** 工程师向统计表；保留摘要与关键限制
- [ ] engineer：统计表可见
- [ ] audit：证据附录含 strata / formula_reference 等 **label 为 en-US**；`evidence_id` 可追溯
- [ ] 不得出现“customer 版更合格/更不安全”的暗示性文案差异

### S5 — 空图与诊断失败

| 项 | 值 |
|----|-----|
| 分析 | Graph Builder scatter（无有效 X/Y 或全 excluded）；步骤见 [`samples/phase0_baselines/graph_empty_manual_s5.md`](../../samples/phase0_baselines/graph_empty_manual_s5.md) |
| 模板 | engineer |
| locale | en-US |

**自动化预筛：** `pdf_empty_chart_renders_localized_no_data_message`（§3.1 #22）+ `pdf_graph_scatter_all_excluded_renders_localized_no_data_en_us`（§3.1 #61）

**Pass 标准：**

- [ ] 图表区显示 `No displayable data`（或 catalog 等价句），非空白崩溃
- [ ] 诊断区有可读错误/限制句（英文）
- [ ] PDF 仍可打开；manifest 一致性 `ok`

### S6 — audit 证据附录多页

| 项 | 值 |
|----|-----|
| 分析 | Johnson 能力（gated）或 RSM LOF（formula_reference）；步骤见 [`samples/phase0_baselines/audit_appendix_manual_s6.md`](../../samples/phase0_baselines/audit_appendix_manual_s6.md) |
| 模板 | audit |
| locale | en-US |

**自动化预筛（§3.1 #3–#4、#8 + §B #20–#21 + §3.3 + #30）：**

- [ ] `representative_johnson_capability_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `representative_rsm_lof_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `pdf_johnson_capability_gate_*` / `pdf_rsm_lof_gate_*` / `pdf_normal_capability_stability_gate_*`
- [ ] `pdf_audit_km_long_table_evidence_appendix_spans_pages_en_us` 全绿（S6 ≥2 PDF 页 + Evidence appendix + audit JSON `label_text_id`）
- [ ] 对应 §3.3 `pdf_*_cross_template_*`（Johnson / RSM / 正态能力 任选一）
- [ ] `audit_json_doe_design_generation_carries_label_text_ids`（DOE 设计附录对照）
- [ ] `pdf_evidence_appendix_uses_localized_label_text_ids`

**E″ customer 截断保留（§3.1 之外；S6 路径 B/C）：**

- [ ] `customer_keeps_capability_gate_limiting_evidence_under_truncation` 全绿（Johnson gated 门禁）
- [ ] `customer_keeps_rsm_lof_limiting_evidence_under_truncation` 全绿（RSM LOF 门禁）

**E″ 加深预筛（§3.1 之外；S6 路径 D/E/F）：**

- [ ] `representative_johnson_spec_outside_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `pdf_johnson_spec_outside_support_localizes_to_en_us_without_chinese_leak` 全绿
- [ ] `pdf_johnson_spec_outside_cross_template_table_visibility_and_en_us_locale` 全绿
- [ ] `customer_keeps_johnson_spec_limit_evidence_under_truncation` 全绿
- [ ] `usesJohnsonSpecLimitGateInterpretationBullet` 全绿
- [ ] `johnson_spec_outside_support_skips_overall_capability` 全绿（域层；`nonnormal_capability_phase6_test`）
- [ ] `usesBoxCoxSpecLimitGateInterpretationBullet` 全绿
- [ ] `pdf_box_cox_invalid_spec_limit_localizes_to_en_us_without_chinese_leak` 全绿
- [ ] `pdf_box_cox_spec_limit_gate_localizes_and_audit_evidence_carries_label_text_id` 全绿
- [ ] `customer_keeps_box_cox_limiting_evidence_under_truncation` 全绿
- [ ] `box_cox_spec_limit_diag_localizes_to_en_us` 全绿
- [ ] `box_cox_service_skips_capability_on_invalid_spec_limits` 全绿（域层；`quality_statistics_test`）
- [ ] `box_cox_service_skips_capability_on_inverted_spec_limits` 全绿（域层；`quality_statistics_test`）
- [ ] `customer_keeps_warranty_exposure_gate_evidence_under_truncation`（路径 E audit 对照）全绿

**Pass 标准：**

- [ ] 正文与 **Evidence appendix** 分页合理
- [ ] 门禁 label 为 catalog 英文（如 `Johnson capability gate` / `RSM lack-of-fit ANOVA`）
- [ ] `.audit.json` 中 `label_text_id` 与 PDF 可见 label 一致（非自由文本漂移）
- [ ] **不** 声称 PDF/A·UA 合规

### S7 — Unicode 列名与数值格式

| 项 | 值 |
|----|-----|
| 数据 | [`samples/capability/PinLength.csv`](../../samples/capability/PinLength.csv)（步骤见 [`pinlength_manual_s7.md`](../../samples/capability/pinlength_manual_s7.md)）；或 `tests/fixtures/minitab/converted/PinLength.csv` |
| 分析 | 过程能力（normal；规格限按样本列） |
| 模板 | engineer |
| locale | zh-CN **与** en-US 各一次 |

**自动化预筛（§3.1 #8 + #39–#40 + #59 + #60）：**

- [ ] `representative_normal_capability_three_report_profiles_localize_without_cross_language_leak` 全绿（含 zh-CN / en-US）
- [ ] `representative_nonnormal_capability_three_report_profiles_localize_without_cross_language_leak` 全绿
- [ ] `pdf_normal_capability_stability_gate_localizes_and_audit_evidence_carries_label_text_id` 全绿
- [ ] `pdf_normal_capability_cross_template_table_visibility_and_en_us_locale` 全绿
- [ ] `pdf_pinlength_capability_unicode_columns_localize_without_cross_language_leak` 全绿
- [ ] `display_formatting_handles_empty_and_non_finite` 全绿（`report_export_phase2_test`）
- [ ] `nonnormal_capability_phase6_test`（域门禁；与 PinLength 无直接绑定，作 Phase 6 回归）

**Pass 标准：**

- [ ] 列名按 QString 路径正确显示（不 mojibake）
- [ ] 非有限值有兜底显示（非空白崩溃）
- [ ] Facts 数值不被 locale 改写（仅 display 格式变化）
- [ ] zh/en 各语言 **chrome 不混语**（数据列名可保留用户原文）

## 6. 签署

| 角色 | 姓名 | 日期 | §3.1 自动化 (61) | F′/E′/E″ 加深 (37) | S1–S7 肉眼 PDF |
|------|------|------|------------------|---------------------|----------------|
| 测试 | | | ☐ | ☐ | ☐ |
| 产品/统计 | | | ☐ | ☐ | ☐ |

**§3.1 自动化签署前建议运行：**

```powershell
powershell -File tools/phase3_preflight.ps1
python tools/list_qt_creator_test_targets.py --by-target
python tools/list_qt_creator_test_targets.py --deepen --by-target
```

或手动：

```powershell
python tools/verify_phase3_prefilter_registry.py
python tools/verify_deepen_prefilter_registry.py
python tools/audit_interpretation_localization.py
python tools/list_phase3_prefilter_tests.py --verify
```

Qt Creator 跑完全部 **61** 项后再勾选「§3.1 自动化」；**F′/E′/E″ 加深 37** 项（`--deepen --by-target`）全绿后勾选加深列。肉眼 PDF 见 [`samples/phase0_baselines/phase3_manual_acceptance_index.md`](../../samples/phase0_baselines/phase3_manual_acceptance_index.md)。

**开放项（不在本清单范围）：**

- veraPDF / PAC 真合规流水线
- 全量 400+ 命令页标题逐页目检
- 动态拼接解释正文未模板化数值句（`tools/audit_interpretation_localization.py` 静态审计 **316/316** 已覆盖 handler 路径，含 `bullet ==` 精确匹配 gate 句）

## 7. 相关文档

- [`qt-creator-dual-line-acceptance-runbook.md`](qt-creator-dual-line-acceptance-runbook.md) — Qt Creator + S1–S7 一站式验收 runbook
- [`vertical-slice-algorithms-and-report-product-plan.md`](vertical-slice-algorithms-and-report-product-plan.md) — Phase 2/3/7 总计划
- [`phase2-pdfa-pdfua-assessment.md`](phase2-pdfa-pdfua-assessment.md) — 诚实 PDF/A·UA 评估
- [`0009-report-bilingual-locale-separation.md`](../adr/0009-report-bilingual-locale-separation.md) — 报告语言 ADR
- [`../../tools/print_acceptance_status.py`](../../tools/print_acceptance_status.py) — 脚本侧 **12 项** 一键汇总（无 Qt Creator）
- [`../../tools/print_qt_creator_signoff_batches.py`](../../tools/print_qt_creator_signoff_batches.py) — 打印 Qt Creator 签收批次 checklist（用户本地勾选）
- [`../../tools/list_phase3_prefilter_tests.py`](../../tools/list_phase3_prefilter_tests.py) — 打印 §3.1 全部 61 项测试名（可加 `--verify`）
- [`../../tools/list_qt_creator_test_targets.py`](../../tools/list_qt_creator_test_targets.py) — §3.1 / `--deepen` / `--scenario` / `--scenario-id S4` / `--global-only` → Qt Creator 目标
- [`../../tools/list_phase3_prefilter_by_scenario.py`](../../tools/list_phase3_prefilter_by_scenario.py) — 按 S1–S7 场景打印预筛（`--scenario-id S4`）
- [`../../tools/verify_phase3_prefilter_registry.py`](../../tools/verify_phase3_prefilter_registry.py) — 校验 §3.1 测试名 ↔ `tests/*.cpp` 注册一致
- [`../../tools/verify_scenario_prefilter_registry.py`](../../tools/verify_scenario_prefilter_registry.py) — 校验 S1–S7 场景预筛 84 项 ↔ `tests/*.cpp`
- [`../../tools/verify_vertical_slice_scenario_coverage.py`](../../tools/verify_vertical_slice_scenario_coverage.py) — 校验 13/13 竖切 representative 测试均在场景预筛中
- [`../../tools/verify_interpretation_gate_scenario_coverage.py`](../../tools/verify_interpretation_gate_scenario_coverage.py) — 校验 3/3 gate 解释 bullet 测试（S4 保修 + S6 Johnson/Box-Cox）均在场景预筛中
- [`../../tools/verify_customer_keeps_scenario_coverage.py`](../../tools/verify_customer_keeps_scenario_coverage.py) — 校验 9/9 customer 模板 limiting evidence 截断保留测试均在场景预筛中
- [`../../tools/verify_deepen_prefilter_registry.py`](../../tools/verify_deepen_prefilter_registry.py) — 校验 F′/E′/E″ deepen 37 项 ↔ `tests/*.cpp`
- [`../../tools/verify_domain_gate_scenario_coverage.py`](../../tools/verify_domain_gate_scenario_coverage.py) — 校验 3/3 域层 gate 测试（S6 Box-Cox/Johnson spec-limit）均在场景预筛中
- [`../../tools/reference_implementation_preflight.ps1`](../../tools/reference_implementation_preflight.ps1) — 算法 reference_implementation 脚本预检（5 项；见 [`reference-implementation-index.md`](reference-implementation-index.md)）
- [`../../tools/phase3_preflight.ps1`](../../tools/phase3_preflight.ps1) — Qt Creator 测试前一键预检（registry + 解释审计 + reference_implementation）
- [`../../tools/audit_interpretation_localization.py`](../../tools/audit_interpretation_localization.py) — 解释 bullet ↔ 本地化 handler 静态审计（316/316）
- [`../../samples/phase0_baselines/phase3_manual_acceptance_index.md`](../../samples/phase0_baselines/phase3_manual_acceptance_index.md) — S1–S7 人工验收总索引
- [`../../samples/phase0_baselines/reliability_km_long_table.md`](../../samples/phase0_baselines/reliability_km_long_table.md) — S1 长表样本
- [`../../samples/phase0_baselines/doe_manual_s2_zh_cn.md`](../../samples/phase0_baselines/doe_manual_s2_zh_cn.md) — S2 中文 DOE
- [`../../samples/phase0_baselines/graph_faceted_manual_s3.md`](../../samples/phase0_baselines/graph_faceted_manual_s3.md) — S3 Graph 分面英文长标题
- [`../../samples/phase0_baselines/audit_appendix_manual_s6.md`](../../samples/phase0_baselines/audit_appendix_manual_s6.md) — S6 audit 证据附录
- [`../../samples/phase0_baselines/warranty_strata_manual_s4.md`](../../samples/phase0_baselines/warranty_strata_manual_s4.md) — S4 保修分层三模板
- [`../../samples/phase0_baselines/graph_empty_manual_s5.md`](../../samples/phase0_baselines/graph_empty_manual_s5.md) — S5 空图 / 全 excluded
- [`../../samples/capability/pinlength_manual_s7.md`](../../samples/capability/pinlength_manual_s7.md) — S7 PinLength
