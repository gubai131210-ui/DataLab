# 保修分层三模板人工验收（Phase 3 S4）

配合 [`docs/research/phase3-cross-page-pdf-manual-acceptance.md`](../../docs/research/phase3-cross-page-pdf-manual-acceptance.md) §5 S4。

## 目标

同一 **保修分层** OutputPage，分别导出 customer / engineer / audit（en-US），肉眼确认裁剪差异与 manifest 可比性。

## 数据（与自动化 fixture 同结构）

可直接导入 [`warranty_strata_s4.csv`](warranty_strata_s4.csv)（列：`time,censor_type,mode,exposure`）：

```csv
time,censor_type,mode,exposure
10,exact,wear,10
12,exact,wear,10
15,right,wear,20
8,exact,early,5
30,right,,5
```

| 项 | 值 |
|----|-----|
| 分析 | 可靠性 → **保修摘要（含分层）** / `reliability_warranty` |
| 模型 | Weibull |
| 保修时间 T_w | 1000 hours |
| 模板 | customer → engineer → audit（各导出一次） |
| locale | **en-US** |

## Qt Creator 步骤

1. 导入/录入上表，映射删失列、失效模式列、暴露量列。
2. 运行保修分析，确认 Session 有分层表（失效模式分母追溯）。
3. 导出报告三次：仅改模板，**facts 不变**。
4. 打开三份 PDF + `.manifest.json` + `.audit.json`（audit 模板）。

## 自动化预筛（§3.1）

- `representative_warranty_strata_three_report_profiles_localize_without_cross_language_leak`
- `pdf_warranty_summary_cross_template_table_visibility_and_en_us_locale`（scalar 对照）
- `pdf_warranty_cross_template_table_visibility_and_en_us_locale`
- `pdf_warranty_strata_three_templates_manifest_facts_hash_match_en_us`
- `customer_keeps_cif_fine_gray_warranty_strata_limiting_evidence_under_truncation`（customer 模板 plot-flood 截断仍保留 CIF/Fine-Gray/分层门禁 evidence）

## Pass 焦点（肉眼）

- [ ] 三份 manifest 的 `facts_hash` **相同**
- [ ] customer：**无** Failure-mode denominator trace / 工程师向统计表；保留摘要与限制
- [ ] engineer：分层表 + 摘要表可见
- [ ] audit：Evidence appendix 英文 label；`.audit.json` 中 `label_text_id` 可追溯
- [ ] 无“customer 更合格/更安全”暗示性文案差异

## 证据

- `formula_reference`；**不得**声称 vendor_oracle / PDF/A·UA 合规。
