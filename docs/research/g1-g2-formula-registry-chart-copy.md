# G1 公式注册表 UI + G2 图表/表格复制

> 研究日期：2026-08-22（UTC+8）  
> 访问日期：2026-08-22  
> Track：**G1 + G2**（`product-evolution-market-ux-architecture-research.md` §7）

## 1. 目标

| Track | 交付 | 验收 |
|-------|------|------|
| **G1** | 独立 **公式注册表** 页（`FormulaRegistryDialog`） | 帮助 → 公式注册表；按 id/符号/URL 搜索；公式块 + NIST/Minitab 外链 + research md 路径 |
| **G2** | 输出区 **Copy PNG / Copy TSV** 增强 | 复制图形含 PNG+BMP+行可见性脚注；表格 TSV/CSV 含 `#` 注释行；多图页优先复制焦点图；输出页 ScrollArea 焦点 Ctrl+C 路由复制图形；表格 Ctrl+C 复制选中行 |

## 2. 架构

```
algorithm_help.json → AlgorithmHelpCatalogLoader → FormulaRegistryDialog (ui)
ChartModel + row_visibility → AnalysisChartWidget::copy_to_clipboard → row_visibility_clipboard
OutputPage.configuration + Facts.eda → page_renderer → append_clipboard_footnote_comments
OutputWorkspace QShortcut(Ctrl+C) on scroll → copy_chart_requested → MainWindow::copy_chart
OutputWorkspace::chart_for_copy() prefers last focused/clicked chart on current page (Sixpack)
```

- **分层**：无 domain 统计；G1 读既有 help catalog；G2 仅 UI/reporting 呈现，不破坏 `hidden`/`excluded` 语义（Phase 7 契约）。
- **新页**：公式注册表为独立 Dialog，不堆入主窗口单页。

## 3. Primary URL

| 主题 | URL |
|------|-----|
| NIST EDA 4-plot（G4 后续） | https://www.itl.nist.gov/div898/handbook/eda/section3/eda3332.htm |
| JMP Hidden vs Excluded | https://www.jmp.com/support/help/en/19.1/jmp/element-types-and-options.shtml |
| Minitab Graph export | https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/graph-builder/overview/ |

## 4. 测试

- `formula_registry_dialog_test`：树非空、选 capability 后详情含公式与 reference_links；按 id / URL / research md / 符号（Cpk）过滤
- `row_visibility_clipboard_test`：脚注文本、合成 pixmap 增高
- `analysis_chart_widget_test`：BMP mime、有 hidden 时复制更高 pixmap
- `output_workspace_test`：输出页 ScrollArea Ctrl+C 发射 `copy_chart_requested`；Sixpack 最后焦点图
- `algorithm_help_dialog_test`：「在公式注册表中打开」按钮存在

本地批量运行（默认 Debug 构建目录）：

```powershell
python tools/check_g1g2_build_ready.py
powershell -File tools/run_g1g2_tests.ps1
```

## 6. 交付清单（§八）

| # | 交付物 | 状态 |
|---|--------|------|
| 1 | 计划 + 涉及文件 + Qt Creator 目标名 | 本文 §1–4 |
| 2 | 实现（G1 Dialog + G2 复制/脚注/路由） | 见 `formula_registry_dialog.*`、`row_visibility_clipboard.*`、`page_renderer`、`output_workspace` |
| 3 | research / acceptance / wiring 文档 | 本文 + `quality-algorithms-acceptance.md` + `algorithm-wiring-index.md` |
| 4 | `python tools/print_acceptance_status.py` + `python tools/verify_g1_g2_track.py` | 13/13 + G1+G2 OK |
| 5 | 人工验收 | `samples/product_evolution/g1_g2_manual_acceptance.md` |
| 6 | Qt Creator 测试目标 | 5 个测试 + `tools/run_g1g2_tests.ps1`（默认 Debug 构建目录） |
| 7 | Qt Creator 主程序 | ✅ Debug **DataLab.exe** + 用户 G1/G2/测试 **全 PASS** 2026-08-22 |

## 7. 明确不做

- 不把 Minitab 数值当 golden
- 不合并 customer/engineer/audit 模板
- 不在公式页只链仓库 md 而无公式块（无块则显式说明）

**文档状态：** G1+G2 竖切 **COMPLETE**（2026-08-22）。用户 Qt Creator Debug 编译 + G1/G2 手工验收 + 测试 **全 PASS** 签收。详见 `samples/product_evolution/g1_g2_manual_acceptance.md`。
