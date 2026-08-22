# Johnson 能力门禁 audit 人工验收（Phase 3 S6 路径 B）

配合 [`audit_appendix_manual_s6.md`](../phase0_baselines/audit_appendix_manual_s6.md) 与 Phase 6 Johnson 门禁契约。

## 数据

| 项 | 值 |
|----|-----|
| 文件 | [`johnson_gated_s6.csv`](johnson_gated_s6.csv)（40 行；列 `x`） |
| 来源 | 与 `nonnormal_capability_phase6_test::johnson_roundtrip_and_capability_gate` 同生成式 |
| LSL | **首行值**（≈ **1.151**） |
| USL | **末行 × 1.2**（≈ **8.987**） |

## Qt Creator 步骤

1. 导入 `johnson_gated_s6.csv`
2. **过程能力 → Johnson**
3. 变量列 = `x`；规格 = 上表 LSL / USL
4. 确认诊断含 `johnson_capability_gated`；**不得**输出合格判定
5. 导出 **audit** / **en-US** PDF + manifest + audit JSON
6. 对照 PDF 中 `Johnson capability gate` 与 JSON `evidence.johnson_capability_gated`

## 自动化预筛

- `representative_johnson_capability_three_report_profiles_localize_without_cross_language_leak`
- `pdf_johnson_capability_gate_localizes_and_audit_evidence_carries_label_text_id`
- `pdf_johnson_capability_cross_template_table_visibility_and_en_us_locale`

### 路径 D — 规格限越界（LSL 在定义域外）

1. 同上导入 `johnson_gated_s6.csv`
2. **Johnson**；变量 = `x`；**仅 LSL = -1000**（不设 USL）
3. 确认 `johnson_spec_outside_support` 错误诊断；**无** Overall 能力指数表
4. 导出 audit / en-US PDF + audit JSON
5. 对照 `:gate:johnson_spec_limit` 与 `Johnson spec limit gate` label

**预筛：**
- `representative_johnson_spec_outside_three_report_profiles_localize_without_cross_language_leak`
- `pdf_johnson_spec_outside_support_localizes_to_en_us_without_chinese_leak`
- `pdf_johnson_spec_outside_cross_template_table_visibility_and_en_us_locale`
- `report_profile_phase1_test::customer_keeps_johnson_spec_limit_evidence_under_truncation`
- `interpretation_service_test::usesJohnsonSpecLimitGateInterpretationBullet`
- `nonnormal_capability_phase6_test::johnson_spec_outside_support_skips_overall_capability`（域层）

## Pass 标准

- [ ] audit PDF 含 Evidence appendix；门禁 label 为 catalog 英文
- [ ] `.audit.json` 的 `label_text_id` 与 PDF 可见 label 一致
- [ ] manifest `pdfa_status=not_validated`、`pdfua_status=unsupported`
- [ ] **不**声称过程合格 / PDF/A·UA 合规

## 证据

- `formula_reference` + `gate_status=gated_research`；**非** vendor_oracle / golden 开放门禁。
