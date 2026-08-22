# Phase 0 / Phase 3 能力样例

| 文件 | 用途 |
|------|------|
| [`PinLength.csv`](PinLength.csv) | Minitab 转换样本（100 行）；LSL=13，USL=25 |
| [`PinLength_unicode.csv`](PinLength_unicode.csv) | 同数据；变量列 **`长度μm`**（S7 Unicode / mojibake 肉眼验收） |
| [`johnson_gated_s6.csv`](johnson_gated_s6.csv) | Phase 6 Johnson 门禁；S6 audit 路径 B（40 行） |
| [`johnson_gated_manual_s6.md`](johnson_gated_manual_s6.md) | S6 Johnson audit 人工验收步骤（路径 B 门禁 / **路径 D 规格越界**） |
| [`box_cox_spec_gate_s6.csv`](box_cox_spec_gate_s6.csv) | Phase 6 Box-Cox 规格限门禁；S6 路径 C（5 行） |
| [`box_cox_spec_gate_manual_s6.md`](box_cox_spec_gate_manual_s6.md) | S6 Box-Cox 无效/倒置规格限人工验收 |
| [`pinlength_manual_s7.md`](pinlength_manual_s7.md) | Phase 3 **S7** 人工验收步骤 |
| `phase0_report_capability_pin` | `ReportExportPhase2Test::sample_table()` 元数据 ID |

自动化预筛：`pdf_pinlength_capability_unicode_columns_localize_without_cross_language_leak`（unicode 列名 + zh/en PDF；**不替代** UI 导入）。
