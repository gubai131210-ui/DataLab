# 保修暴露量门禁人工验收（Phase 3 S4 加深）

配合 [`docs/research/phase3-cross-page-pdf-manual-acceptance.md`](../../docs/research/phase3-cross-page-pdf-manual-acceptance.md) §5 S4 / §E″。

## 目标

验证 **暴露量列** 三种诚实路径：非法值、零求和、列覆盖标量。导出 en-US engineer/audit PDF，确认无静默补齐、无中文泄漏。

## 路径 A — 非法暴露量（invalid）

| 项 | 值 |
|----|-----|
| 列 | `time,censor_type,exposure` |
| 行 | `10,exact,1.5` / `15,right,-1` / `20,exact,1.0` |
| 模型 | Weibull；T_w=1000 hours |
| 期望 | `invalid_exposure_value`；**无** Claims per 1000 表；EvidenceBundle `:gate:warranty_exposure` |

## 路径 B — 零求和（zero）

| 项 | 值 |
|----|-----|
| 列 | `exposure` |
| 行 | 10 / 20 / 30；**全部 excluded** |
| 标量 exposure | 500（不得静默兜底） |
| 期望 | `warranty_zero_exposure`；无摘要指标 |

## 路径 C — 列覆盖标量（override）

| 项 | 值 |
|----|-----|
| 列 | `time,censor_type,exposure` |
| 行 | `10,exact,1.5` / `15,right,2.5` / `20,exact,1.0` |
| 标量 exposure | 9999（应被忽略） |
| 期望 | 有效 exposure=**5.0**；`exposure_source=column_sum`；info 诊断 `warranty_exposure_column_overrides_scalar`（**engineer/audit** 可见；customer 裁剪 info 级诊断） |

## 自动化预筛（§E″，不在 §3.1 61 计数内）

- `representative_warranty_exposure_gate_three_report_profiles_localize_without_cross_language_leak`
- `representative_warranty_exposure_column_override_three_report_profiles_localize_without_cross_language_leak`
- `pdf_warranty_invalid_exposure_localizes_to_en_us_without_chinese_leak`
- `pdf_warranty_exposure_gate_localizes_and_audit_evidence_carries_label_text_id`
- `pdf_warranty_exposure_cross_template_table_visibility_and_en_us_locale`
- `pdf_warranty_exposure_column_override_localizes_to_en_us_without_chinese_leak`
- `interpretation_service_test::usesWarrantyExposureGateInterpretationBullet`
- `report_profile_phase1_test::customer_keeps_warranty_exposure_gate_evidence_under_truncation`
- `report_locale_phase3_test::warranty_exposure_diag_localizes_to_en_us`

## Pass 焦点（肉眼）

- [ ] 路径 A/B：**无** Claims per 1000 / 保修摘要指标表
- [ ] 路径 C：摘要 exposure=5.0；PDF 含英文 override 句；**不出现** 9999 作为有效暴露量
- [ ] audit 模板：Evidence appendix 含 `Warranty exposure gate`（路径 A/B）
- [ ] 三模板 facts_hash 相同（同一路径内）
- [ ] **不得**声称 vendor_oracle / PDF/A·UA 合规
