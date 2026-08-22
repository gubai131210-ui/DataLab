# G3 Graph 受控 Builder：UX / 开源调研（2026-08-23）

> 研究日期 / 访问日期：2026-08-23（UTC+8）  
> 用途：产品 Track **G3 Graph 受控 Builder** 的权威调研；配套 Goal 计划见  
> [`goal-wave-2026-08-23-g3-graph-builder-plan-and-mega-prompt.md`](goal-wave-2026-08-23-g3-graph-builder-plan-and-mega-prompt.md)  
> 产品队列：[`product-evolution-market-ux-architecture-research.md`](product-evolution-market-ux-architecture-research.md) §2.1 / §3.1 / §7 **G3**  
> 依赖水位：G1/G2/G6 ✅；Menu IA ✅；算法 Wave-4 ✅；Phase 7 `graph_service` + `hidden`/`excluded` 双口径已有基础

---

## §0 问题陈述

DataLab 已有多条图形命令（scatter / boxplot / histogram / hexbin / facet 等）与 `GraphConfiguration`，但用户仍要「先想好命令再填角色」。市场解法是 **Graph Builder**：同一数据集上换槽位与 geom，**探索优先、公式不动**。

| 派系 | 代表 | 行为 | DataLab 采纳 |
|------|------|------|--------------|
| **全拖拽分区** | [JMP Graph Builder](https://www.jmp.com/support/help/en/19.1/jmp/how-to-use-graph-builder.shtml) | 列拖入 X/Y/Group/Wrap/Color…；Element 画廊多选叠加 | **受控槽位**（下拉/列表点选，可后补拖拽）；**不做**全拖拽 + 无限叠加 |
| **图库预览选型** | [Minitab Graph Builder](https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/graph-builder/scatterplot/) | 选变量 → 候选图预览 → By/Overlay 分面 | 借鉴「按列型启用/置灰 geom」；预览走既有 chart 渲染 |
| **科学绘图工作区** | [LabPlot](https://labplot.org/)（Qt） | 图层、坐标系、交互选点 | 学 **独立页 + 预览区**；不克隆 Origin 全功能 |
| **分析侧栏结果** | [JASP](https://github.com/jasp-stats/jasp-desktop) | 选项与结果同屏、模块化 | 学 **分区布局**；不嵌 R |

**本 Wave 锁定产品句：**  
**独立 Graph Builder 页：指定 X/Y/（可选）Facet/Color + 选择适用 geom → 调用既有 `graph_service` / 图形命令路径生成预览与 OutputPage；尊重 `hidden` vs `excluded`。**

---

## §1 Primary Sources（网上调研）

| 主题 | URL | 访问 | 对本产品的采纳 |
|------|-----|------|----------------|
| JMP How to Use Graph Builder | https://www.jmp.com/support/help/en/19.1/jmp/how-to-use-graph-builder.shtml | 2026-08-23 | 流程：列 → zones → element → Done |
| JMP Graph Zones | https://www.jmp.com/support/help/en/19.1/jmp/graph-zones.shtml | 2026-08-23 | 本 Wave 槽位子集：X、Y、Wrap≈Facet、Color；Overlay/Page **不做** |
| JMP About Graph Builder Window | https://www.jmp.com/support/help/en/19.1/jmp/about-the-graph-builder-window.shtml | 2026-08-23 | 左列清单 + 中预览 + 上 Element 图标；Excluded 行隐藏 |
| JMP Element Types | https://www.jmp.com/support/help/en/19.1/jmp/element-types-and-options.shtml | 2026-08-23 | 按变量类型启用元素；本 Wave 用 **画廊按钮 + 置灰** |
| Minitab Scatterplot (Graph Builder) | https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/graph-builder/scatterplot/ | 2026-08-23 | By variables = 分面；Overlay = 同图叠加（本 Wave 先做 By/Facet） |
| Minitab Paneling | https://support.minitab.com/en-us/minitab/help-and-how-to/graphs/general-graph-options/groups-and-multiple-graphs/paneling-and-multiple-graphs/ | 2026-08-23 | 禁止同一变量既做分面又做轴分组；`facet_max_panels` 硬上限 |
| jamovi UI basic design | https://dev.jamovi.org/ui/basic-design/ | 2026-08-23 | 高级选项折叠；禁止单页堆控件 |
| LabPlot 官网 | https://labplot.org/ | 2026-08-23 | Qt 桌面绘图工作区参考 |
| NIST 4-plot（远期 G4） | https://www.itl.nist.gov/div898/handbook/eda/section3/eda3332.htm | 2026-08-23 | **本 Goal 不做**；登记为 G4 |

### §1.1 GitHub / 开源学习清单（必读意图，非抄代码）

| 仓库 | URL | 学什么 | 禁止 |
|------|-----|--------|------|
| **LabPlot** | https://github.com/KDE/labplot（主仓 invent.kde.org） | Qt 绘图页结构、预览、导入 | 嵌 notebook/CAS；Origin 全克隆 |
| **JASP Desktop** | https://github.com/jasp-stats/jasp-desktop | C++/Qt 分析页分区、结果区 | 嵌 R 运行时进 dist |
| **jamovi** | https://github.com/jamovi/jamovi | 选项折叠、结果与数据同生命周期 | Electron 栈 |
| **Cassini** | https://github.com/saturnis-io/cassini | 「数→公式可追溯」叙事（对齐 Evidence） | 云 RTSPC / 多租户 |
| **qcc / qicharts2** | https://github.com/luca-scr/qcc · https://github.com/anhoej/qicharts2 | SPC 图输出结构（G3 仅图形探索，不改 SPC 公式） | 当 golden 数值 |
| **pycontrolcharts** | https://github.com/suanto/pycontrolcharts | 固定 schema 导出思路 | 替换 domain Facts |

---

## §2 行业模式摘要（可执行原则）

1. **槽位驱动，不是命令名驱动** — 用户先放 X/Y，再选 geom。  
2. **geom 适用性由列型决定** — 不适用则置灰并 Tooltip 说明。  
3. **分面有上限** — 已有 `facet_max_panels`（建议默认 6，硬上限 12）。  
4. **Hidden ≠ Excluded** — 隐藏只影响显示；排除影响统计/拟合（Phase 7 契约，UI 必须两开关或脚注）。  
5. **Facts 不变，只改装配** — G3 优先走 `graph_service` / `GraphConfiguration`；禁止为「好看」改 domain 统计公式。  
6. **独立页** — `GraphBuilderPage` 或独立对话框/工作区；禁止塞进 MainWindow 巨型单页。  
7. **可选「应用到输出」** — 预览满意后再 `publish_page`；禁止静默批量出 20 张图。  
8. **诚实边界** — 未知列型 / 空选择 → 禁用生成 + 提示；不做「AI 自动选最佳图」。

---

## §3 DataLab 受控 Builder 规格（锁定 · 本 Wave）

### 3.1 槽位（本 Wave 最小集）

| 槽位 | 映射 | 必填 | 说明 |
|------|------|------|------|
| **X** | `graph.x_column` 或等价 | 视 geom | 数值或分类（依 geom） |
| **Y** | `graph.y_column` / measurement | 视 geom | 散点/箱线等多需 Y |
| **Facet** | `graph.facet_column` | 可选 | 对应 JMP Wrap / Minitab By |
| **Color** | `graph.color_column` 或 legend 角色 | 可选 | 本 Wave 若现有 GraphConfiguration 无 color 字段：仅 UI 预留或映射到已有分组角色；**禁止**为 color 大改 domain 公式 |

> Implementer：以 `quality_types.h` 中 `GraphConfiguration` **现有字段**为准；缺字段则本 Wave 用「可选角色」降级，并在 research/DoD 标明「G3.1 扩展」。

### 3.2 Geom 画廊（本 Wave · 须对接已有 `graph_kind` / 命令）

| geom id（建议） | 适用列型（启发式） | 既有能力锚点 |
|-----------------|-------------------|--------------|
| `scatter` | X num + Y num | `scatter_plot` / graph_kind scatter |
| `boxplot` | Y num +（可选）X cat | `boxplot` |
| `histogram` | 1× num（Y 或 X） | `histogram` |
| `bar` | cat +（可选）weight | 既有 bar / pareto 路径择一 |
| `hexbin` | X num + Y num | `hexbin_plot` |
| `density` | 1× num | Phase 7 density（若已接线） |

不适用组合：**按钮置灰**，不得静默生成错误图。

### 3.3 UI 信息架构（独立页）

```
┌─────────────────────────────────────────────────────────┐
│ Graph Builder                              [关闭]       │
├──────────┬──────────────────────────────┬───────────────┤
│ 列清单   │  预览区（主）                 │ Geom 画廊    │
│ + 类型   │  （调用既有 chart 渲染）       │ 适用/置灰    │
├──────────┴──────────────────────────────┴───────────────┤
│ 槽位：X | Y | Facet | Color（折叠高级）                   │
│ [显示隐藏行] [排除行口径说明]  [生成到输出] [复制图 G2]   │
└─────────────────────────────────────────────────────────┘
```

- **禁止**把槽位、画廊、报告模板、MSA 全堆一页。  
- 高级：`facet_max_panels`、图例选项 → 折叠。

### 3.4 与现有命令的关系

| 模式 | 说明 |
|------|------|
| **推荐** | Builder 内部组装 `GraphConfiguration` → `GraphService::run`（或等价）→ OutputPage |
| **兼容** | 菜单里旧图形命令保留；Builder 是探索入口，不删除 `analysis_commands` 图形项 |
| **G6** | Wizard 可推荐 `scatter_plot` 等；**本 Goal 不改** G6 引擎 |

---

## §4 明确不做（本 Wave）

| 项 | 原因 |
|----|------|
| JMP 全量 zones（Map/Freq/Size/Interval/Page/多 Overlay） | scope 爆炸 |
| 任意多 Element 同时叠加（Shift 多选无限） | 先单 geom |
| Brushing 多视图动态链接 | 登记 **G3.5 / 远期** |
| G4 Report Card / NIST 4-plot 全页 | 独占 G4 |
| G5 AnalysisService 大拆分 | 独占 G5 |
| 改 domain 统计数值路径「顺便修」 | 产品 UI Track |
| 内嵌 R/Python、云 RTSPC、Ribbon | deferred |
| LLM 自动选图 | 诚实策略 |

---

## §5 验收要点

| 门 | 内容 |
|----|------|
| 脚本 | `verify_g3_graph_builder_track.py`：页/对话框源文件、CMake、测试、DoD `[x]`、无非法 AnalysisService 乱入（允许经 GraphService） |
| QtTest | 列型→geom 启用矩阵；facet 上限；hidden/excluded 契约标记；空选择拒绝 |
| 人手 | 选 X/Y 数值 → scatter 可用 → 预览 → 生成到输出（或打开等价设置）；换 histogram 时 geom 规则正确 |

---

**文档状态：** 2026-08-23 首版；供 G3 Goal 直接引用。
