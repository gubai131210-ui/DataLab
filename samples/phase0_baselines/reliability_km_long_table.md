# KM 长表样本（S1 / S6 跨页 PDF 人工验收）

数据文件：`reliability_km_long_table.csv`

## 用途

- **Phase 3 S1** 人工验收：超长 Kaplan–Meier 生存表跨页、表头重复、行对齐（见 [`docs/research/phase3-cross-page-pdf-manual-acceptance.md`](../../docs/research/phase3-cross-page-pdf-manual-acceptance.md) §5 S1）。
- **Phase 3 S6** 人工验收：audit 模板 + 长表 + Evidence appendix 分页（同 CSV；模板 **audit**，语言 **en-US**）。
- 自动化预筛：
  - `pdf_reliability_km_long_table_survival_table_spans_pages_en_us`（engineer / en-US；≥2 PDF 页）
  - `representative_reliability_km_three_report_profiles_localize_without_cross_language_leak`
  - `representative_reliability_weibull_three_report_profiles_localize_without_cross_language_leak`
  - `representative_reliability_lognormal_three_report_profiles_localize_without_cross_language_leak`
  - `pdf_reliability_km_and_weibull_localize_to_en_us_without_chinese_leak`
  - `pdf_reliability_km_and_weibull_cross_template_table_visibility_and_en_us_locale`
  - `pdf_audit_km_long_table_evidence_appendix_spans_pages_en_us`（audit / en-US；≥2 PDF 页 + Evidence appendix + audit JSON `label_text_id`）
- **不替代**肉眼表头重复 / 附录布局签署。

## 数据说明

| 项 | 值 |
|----|-----|
| 行数 | 55 |
| 删失 | 无（全部 `exact` 失效） |
| 失效时刻 | 1 … 55（小时） |
| KM 生存表行数 | 55（每时刻一步） |

## Qt Creator 操作步骤

### 路径 A — S1（engineer / en-US）

1. 导入 `reliability_km_long_table.csv` 为工作表。
2. 列映射：`time` → 寿命列；`censor_type` → 删失类型；`mode` → 失效模式。
3. 分析：**可靠性 → Kaplan–Meier**。
4. 导出报告：模板 **engineer**，语言 **en-US**。
5. 对照 S1 Pass 标准检查 PDF（表头重复、无混语、manifest `facts_hash`）。

### 路径 B — S6（audit / en-US）

1. 同上导入与分析。
2. 导出报告：模板 **audit**，语言 **en-US**。
3. 对照 S6 Pass 标准：正文 + Evidence appendix 分页、`.audit.json` 的 `label_text_id` 与 PDF 可见 label 一致。

## 证据类型

- `formula_reference`（NIST product-limit）；**不得**标记为 `golden` 或 `vendor_oracle`。
- 与手算小样本 `reliability_km_handcalc.csv` 互补：本文件专用于 **跨页布局**，不做逐步 R 手算对齐。
