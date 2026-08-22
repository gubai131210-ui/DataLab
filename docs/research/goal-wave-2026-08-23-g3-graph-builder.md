# Wave 执行态：G3 Graph 受控 Builder（2026-08-23）

> 计划：[`goal-wave-2026-08-23-g3-graph-builder-plan-and-mega-prompt.md`](goal-wave-2026-08-23-g3-graph-builder-plan-and-mega-prompt.md)  
> 调研：[`g3-graph-builder-ux-research-2026-08-23.md`](g3-graph-builder-ux-research-2026-08-23.md)  
> 门禁：`python tools/verify_g3_graph_builder_track.py`  
> **状态：待 `/goal` 执行；下列 DoD 由执行 agent 勾选。**

---

## DoD（W1–W4 全部勾选才算 complete）

### W1 geom 适用矩阵

- [ ] 纯函数/可测矩阵落地（无 Qt 优先）
- [ ] 规则覆盖 research §3.2 主路径
- [ ] 不适用组合置灰/拒绝；推荐 geom 可对接既有 graph_kind/命令
- [ ] QtTest ≥10 例

### W2 Graph Builder UI

- [ ] 独立 `GraphBuilderPage` 或等价对话框
- [ ] 列清单 → 槽位（X/Y/Facet/Color）→ geom 画廊 → 预览 → 生成到输出
- [ ] 高级选项折叠；**未**单页堆叠报告/MSA
- [ ] 尊重 `hidden` / `excluded` 双口径（UI 可见或脚注）

### W3 接线与 i18n

- [ ] MainWindow「图形」菜单 chrome 入口
- [ ] 双语文案（`ui_tr` / `ui_menu_strings.json`）
- [ ] wiring-index + acceptance §2

### W4 验收

- [ ] `tests/g3_graph_builder_track_test.cpp` + CMake
- [ ] `tools/verify_g3_graph_builder_track.py` PASS
- [ ] 回归 menu IA + g6 + wave4 verify PASS
- [ ] 本文件 DoD 勾选完成

### 明确不做（保持确认）

- [ ] 未做 G4 全量 / G5 大拆 / JMP 全 zones / 嵌 R / 一键全套图 / 改 domain 公式

---

## 人手门

1. Qt Creator Rebuild  
2. 图形 → Graph Builder… → 选 X/Y 数值 → scatter 预览 → 生成到输出  

---

**文档状态：** 骨架 2026-08-23；执行后勾选。
