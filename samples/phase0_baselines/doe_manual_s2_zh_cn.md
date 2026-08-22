# DOE 中文报告人工验收（Phase 3 S2）

配合 [`docs/research/phase3-cross-page-pdf-manual-acceptance.md`](../../docs/research/phase3-cross-page-pdf-manual-acceptance.md) §5 S2。

## 路径 A — 中文 catalog + 诚实句（推荐先做）

**目标：** zh-CN 页题/表头/BBD 无角点诊断，无英文 catalog 泄漏。

| 项 | 值 |
|----|-----|
| 设计 | Box–Behnken k=3 |
| 配置 | [`doe_bbd_k3_factors.json`](doe_bbd_k3_factors.json) |
| 模板 | engineer |
| locale | **zh-CN** |

**Qt Creator 步骤：**

1. DOE → 响应曲面设计 → **Box–Behnken**
2. 三因素：`X1/X2/X3`，低/高/中心 = JSON 中 `-1/1/0`
3. 中心点 = 1，`random_seed = 7`（与 baseline 一致）
4. 生成设计矩阵，确认 Session 有 OutputPage
5. 导出报告 → engineer → **zh-CN**
6. 对照 S2 Pass：页题「Box–Behnken 设计」、设计矩阵表头中文、BBD 无角点诚实句中文

**自动化预筛：** `representative_doe_bbd_three_report_profiles_*`（zh-CN 段）；跨页见 `pdf_doe_ccd_k4_long_design_matrix_spans_pages_en_us`（**不替代**表头重复肉眼验收）。customer 模板截断：`report_profile_phase1_test::customer_keeps_ccd_bbd_design_limiting_evidence_under_truncation`。

## 路径 B — 长设计矩阵跨页（可选，同 S1 表头重复）

配置契约：[`doe_ccd_k4_factors.json`](doe_ccd_k4_factors.json)（与 `pdf_doe_ccd_k4_long_design_matrix_spans_pages_en_us` 一致）

| 项 | 值 |
|----|-----|
| 设计 | 中心复合设计 (CCD) **k=4**，变体 **CCF** |
| 因素 | F1–F4；低/高/中心见 JSON |
| 中心点 | **30** |
| 随机化 | 是，`random_seed = **11**` |
| 模板 | engineer |
| locale | zh-CN（跨页表头）；en-US 对照见自动化测试 |

**Pass 焦点：** 设计矩阵跨页时每页顶部重复表头；页题/表头仍为中文 catalog。

**自动化预筛：** `representative_doe_ccd_three_report_profiles_*`（zh-CN 段）；`pdf_doe_ccd_k4_long_design_matrix_spans_pages_en_us`（≥2 PDF 页 + zh-CN 无英文 catalog 泄漏；**不替代**表头重复肉眼验收）；`customer_keeps_ccd_bbd_design_limiting_evidence_under_truncation`。

## 证据

- `formula_reference`；**不得**标记 `golden` / `vendor_oracle`
- manifest `locale_language_tag` = `zh-CN`
