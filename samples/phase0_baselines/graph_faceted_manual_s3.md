# Graph 分面英文长标题人工验收（Phase 3 S3）

配合 [`docs/research/phase3-cross-page-pdf-manual-acceptance.md`](../../docs/research/phase3-cross-page-pdf-manual-acceptance.md) §5 S3。

## 目标

en-US engineer PDF 中，分面 Graph 页题、参数行、截断诊断 **完整可读**，无中文 chrome 泄漏。

## 推荐路径 — 分面 scatter（≥2 面板）

| 项 | 值 |
|----|-----|
| 分析 | Graph Builder → **散点图（分面）** |
| 数据 | ≥5 行；含 facet 分组列（如 `A/B` 至少 2 水平） |
| X / Y | 两列数值 |
| Facet 列 | 分组列（如第 4 列） |
| `facet_max_panels` | 4（可触发截断诊断） |
| 模板 | engineer |
| locale | **en-US** |

**示例工作表：** 可直接导入 [`graph_faceted_s3.csv`](graph_faceted_s3.csv)（列 `id,x,y,facet`）：

| id | x | y | facet |
|----|---|---|-------|
| 1 | 1 | 2 | A |
| 2 | 2 | 3 | A |
| 3 | 3 | 1 | B |
| 4 | 4 | 0 | B |
| 5 | 5 | 2 | C |

## Qt Creator 步骤

1. 导入 [`graph_faceted_s3.csv`](graph_faceted_s3.csv) 或手工录入等价表。
2. Graph Builder → scatter；选 X、Y、**Facet 列**。
3. 确认 OutputPage 页题为「散点图（分面）」类文案；参数摘要含分面信息。
4. 导出 engineer **en-US** PDF。
5. 肉眼检查页题是否为 `Scatterplot (faceted)`（或 catalog 等价），参数含 `Facet =`。

## 可选加深 — contour / matrix / hexbin / density 分面

| 路径 | 数据 | 预筛测试 |
|------|------|----------|
| 散点图（分面 + hidden） | [`graph_faceted_s3.csv`](graph_faceted_s3.csv) + `hidden_rows` | `pdf_graph_scatter_faceted_cross_template_*` + `representative_graph_scatter_faceted_*` |
| 条形图（分面 + hidden） | 同上 + `hidden_rows` | `pdf_graph_bar_faceted_cross_template_*` + `representative_graph_bar_faceted_*` |
| Hexbin（分面 + hidden/excluded） | 列 `y,cat,facet,x` ≥7 行 + `hidden_rows` / `excluded_rows` | `pdf_graph_hexbin_faceted_cross_template_*` + `representative_graph_hexbin_faceted_*` + `pdf_hexbin_rectangular_bins_gate_*` |
| 密度图（分面 + hidden） | 列 `x,cat,facet` ≥6 行 + `hidden_rows` | `pdf_graph_density_faceted_cross_template_*` + `representative_graph_density_faceted_*` + `pdf_density_curve_not_discrete_marks_gate_*` |
| contour / matrix | 见 §3.1 F 段 | `pdf_graph_builder_faceted_contour_and_matrix_*` |

## 自动化预筛（§3.1 + F′）

- `representative_graph_scatter_faceted_three_report_profiles_localize_without_cross_language_leak`
- `pdf_graph_builder_faceted_scatter_localizes_to_en_us_without_chinese_leak`
- `pdf_graph_scatter_faceted_cross_template_plot_visibility_and_en_us_locale`
- `graph_builder_faceted_page_titles_localize_to_en_us`（页题 locale 回归）
- **F′ 推荐：** `representative_graph_scatter_faceted_*` / `representative_graph_bar_faceted_*` / `representative_graph_hexbin_faceted_*` / `representative_graph_density_faceted_*` 及对应 `pdf_*_gate_*` / `pdf_*_cross_template_*`（见 acceptance doc §3.1 F′）

## Pass 焦点

- [ ] 页题 `* (faceted)` 未被截断为乱码
- [ ] `Facet =` / `Variable =` / `Display N =` 等 parameter 行可读
- [ ] 分面截断诊断（若有）为完整英文
- [ ] `Display N =` / `Analysis N =` 与 hidden/excluded 计数一致（F′ 路径）
- [ ] **无中文** 页题/参数/诊断（工作表数据值除外）

## 证据

- 产品契约 + catalog；非 PDF/A·UA 合规证明。
