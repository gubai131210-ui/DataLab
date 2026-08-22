# PinLength 能力报告人工验收（Phase 3 S7）

配合 [`docs/research/phase3-cross-page-pdf-manual-acceptance.md`](../../docs/research/phase3-cross-page-pdf-manual-acceptance.md) §5 S7。

## 数据

| 项 | 值 |
|----|-----|
| 文件 | [`PinLength.csv`](PinLength.csv)（100 行；`Length` + `Machine`） |
| Unicode 变体 | [`PinLength_unicode.csv`](PinLength_unicode.csv)（同数据；变量列 **`长度μm`** — 与自动化 `pdf_pinlength_capability_unicode_columns_*` 对齐） |
| 规格 | LSL = **13**，USL = **25**（见 `tests/fixtures/minitab/SOURCE.md`） |

## Qt Creator 步骤

1. 导入 `PinLength.csv`（或 Unicode 路径：`PinLength_unicode.csv`，变量列 = **`长度μm`**）
2. **过程能力 → 正态**；变量列 = `Length`（或 `长度μm`）；规格 13 / 25
3. 导出报告 → 模板 **engineer**
4. 第一次 locale = **zh-CN**；第二次 = **en-US**（同一 OutputPage）

## 自动化预筛

- `pdf_pinlength_capability_unicode_columns_localize_without_cross_language_leak`（unicode 列名 + zh/en PDF 字节扫描；**不替代** UI 导入路径）
- `representative_normal_capability_three_report_profiles_*`
- `display_formatting_handles_empty_and_non_finite`（非有限值显示兜底）

## Pass 标准（肉眼）

- [ ] 列名在 PDF/参数行中无 mojibake
- [ ] zh/en **chrome 不混语**（用户数据列名可保留原文）
- [ ] `facts_hash` 两次导出一致
- [ ] manifest `locale_language_tag` 分别为 `zh-CN` / `en-US`
