# Graph 空图 / 全 excluded 人工验收（Phase 3 S5）

配合 [`docs/research/phase3-cross-page-pdf-manual-acceptance.md`](../../docs/research/phase3-cross-page-pdf-manual-acceptance.md) §5 S5。

## 路径 A — 空 PlotSpec（推荐先做）

| 项 | 值 |
|----|-----|
| 分析 | 任意带 OutputPage 的命令 + **空图表**（自动化：`pdf_empty_chart_*` 用 control 空 plot） |
| 模板 | engineer |
| locale | en-US |

**Pass：** 图表区显示 `No displayable data`（或 catalog 等价句），PDF 可打开，manifest `consistency_status=ok`。

**自动化预筛：** `pdf_empty_chart_renders_localized_no_data_message`（§3.1 #22）

## 路径 B — Graph scatter 全 excluded（真实 GraphService 路径）

| 项 | 值 |
|----|-----|
| 分析 | Graph Builder → **散点图** |
| 数据 | 导入 [`graph_scatter_all_excluded_s5.csv`](graph_scatter_all_excluded_s5.csv)（列 `x,y`；≥3 行） |
| 配置 | 将全部工作表行标记为 **excluded**（非 hidden） |
| 模板 | engineer |
| locale | en-US |

**Qt Creator 步骤：**

1. 导入 [`graph_scatter_all_excluded_s5.csv`](graph_scatter_all_excluded_s5.csv)（或等价两列数值）。
2. Graph Builder → scatter，选 X/Y。
3. 在工作表/图形属性中将 **全部行 excluded**。
4. 运行图形，确认诊断/参数摘要提及 excluded 计数。
5. 导出 engineer en-US PDF。

**自动化预筛：** `pdf_graph_scatter_all_excluded_renders_localized_no_data_en_us`（§3.1 #61）

**Pass 焦点：**

- [ ] PDF 图表区英文空态句（非空白崩溃）
- [ ] 页题/参数/诊断 **无中文 chrome**
- [ ] manifest 一致性 `ok`；**不**声称 PDF/A·UA 合规

## 证据

- 产品契约 + `chart.no_displayable_data` catalog；非 golden。
