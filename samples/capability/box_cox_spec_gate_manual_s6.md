# Box-Cox 规格限门禁人工验收（Phase 3 S6 路径 F）

配合 [`audit_appendix_manual_s6.md`](../phase0_baselines/audit_appendix_manual_s6.md) **路径 F** 与 Phase 6 CAP-NN-2 Box-Cox 规格限变换门禁。

## 数据

| 项 | 值 |
|----|-----|
| 文件 | [`box_cox_spec_gate_s6.csv`](box_cox_spec_gate_s6.csv)（5 行；列 `Y`） |
| 场景 A（无效 LSL） | LSL = **-1**，USL = **10** |
| 场景 B（倒置限） | LSL = **10**，USL = **2** |

## Qt Creator 步骤

1. 导入 `box_cox_spec_gate_s6.csv`
2. **Box-Cox 变换**；变量列 = `Y`
3. **场景 A**：规格 LSL/USL 如上；确认诊断 `box_cox_invalid_spec_limit`；**无**「变换后过程能力」表；解释含「已跳过变换后过程能力表」
4. **场景 B**：倒置 LSL/USL；确认诊断 `box_cox_spec_limits_order`；同样无能力表
5. 导出 **customer / engineer / audit** × **en-US** PDF + manifest（场景 A 即可）
6. en-US PDF 不得泄漏「规格下限」「变换后过程能力」等中文；应见 `Lower spec limit cannot be transformed` 或 `out of order`

## 自动化预筛

- `representative_box_cox_three_report_profiles_localize_without_cross_language_leak`（含 invalid-spec 段）
- `pdf_box_cox_invalid_spec_limit_localizes_to_en_us_without_chinese_leak`
- `pdf_box_cox_invalid_spec_limit_cross_template_table_visibility_and_en_us_locale`
- `pdf_box_cox_spec_limit_gate_localizes_and_audit_evidence_carries_label_text_id`
- `report_profile_phase1_test::customer_keeps_box_cox_limiting_evidence_under_truncation`
- `quality_statistics_test::box_cox_service_skips_capability_on_invalid_spec_limits`
- `quality_statistics_test::box_cox_service_skips_capability_on_inverted_spec_limits`
- `report_locale_phase3_test::box_cox_spec_limit_diag_localizes_to_en_us`
- `interpretation_service_test::usesBoxCoxSpecLimitGateInterpretationBullet`

## Pass 标准

- [ ] 无效/倒置规格限均跳过变换后能力表
- [ ] en-US 诊断与解释 bullet 为 catalog 英文；zh-CN 保留中文 gate 句
- [ ] customer 模板无 capability 统计表；仍保留 pass/fail 诚实 limitation
- [ ] manifest `pdfa_status=not_validated`、`pdfua_status=unsupported`
- [ ] **不**声称过程合格 / PDF/A·UA 合规

## 证据

- `formula_reference`（Box-Cox 单调变换）；**非** vendor_oracle；**非** 合格判定。
