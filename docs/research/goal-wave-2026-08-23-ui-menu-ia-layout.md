# Wave 执行态：UI 菜单信息架构整理（2026-08-23）

> 计划：[`goal-wave-2026-08-23-ui-menu-ia-layout-plan-and-mega-prompt.md`](goal-wave-2026-08-23-ui-menu-ia-layout-plan-and-mega-prompt.md)  
> 分类权威：[`ui-menu-ia-minitab-taxonomy-2026-08-23.md`](ui-menu-ia-minitab-taxonomy-2026-08-23.md)  
> 全量映射：[`ui-menu-ia-command-taxonomy-map-2026-08-23.md`](ui-menu-ia-command-taxonomy-map-2026-08-23.md)  
> 门禁：`python tools/verify_ui_menu_ia_track.py`

---

## DoD（U1+U2+U3 全部勾选才算 complete）

### U1 声明式菜单数据

- [x] 全量 `AnalysisCommand`（137）填写非空 `menu_path` + `menu_group`
- [x] 分类与 taxonomy §3 / command-taxonomy-map 对齐
- [x] Wave-2～4 锚点归类：`cox_regression`→统计/可靠性；`bootstrap_two_sample`→推断/仿真；`stepwise_regression`/`nominal_logistic`→回归；`kmeans`/`cluster_observations`→多变量；`nonparametric_capability`→过程能力；`accelerated_life`/`probit_reliability`→可靠性
- [x] `cox_regression` 不再使用非法顶层「可靠性」；`pareto`→质量工具；DOE 族→统计/DOE

### U2 MainWindow 渲染

- [x] 按 `menu_path` → `menu_group` → 叶命令构建；级联深度硬上限 = 1
- [x] 删除 `primary_analysis_menu()` / `analysis_menu_group()` 硬编码白名单
- [x] 顶层固定：统计 / 控制图 / 质量工具 / 图形
- [x] 新增二级分组写入 `translations/ui_menu_strings.json`（zh_cn + en_us）

### U3 一致性与验收

- [x] `algorithm_help.json` `menu_path` 对齐为 `{path} > {group}`
- [x] `tests/ui_menu_ia_track_test.cpp` + CMake 注册
- [x] `tools/verify_ui_menu_ia_track.py` PASS
- [x] `samples/product_evolution/unified_track_acceptance_plan.md` §2 登记
- [x] `docs/algorithm-wiring-index.md` 菜单 IA 说明
- [x] 回归：`python tools/verify_algorithm_wave4_track.py` 仍 PASS

### 明确不做（保持未做）

- [x] 未做 Graph Builder / JMP 菜单偏好 / Ribbon / 新算法 / domain 数值改动

---

## 人手门（agent 不跑 cmake/ctest）

1. Qt Creator：**Rebuild**（中文路径勿用 agent 强跑）
2. 目视：统计下以子菜单为主；控制图独立顶层且有计量图/计数图等二级；Cox 在 统计→可靠性；逐步回归在 统计→回归

---

## 产物清单

| 产物 | 路径 |
|------|------|
| 映射表 | `docs/research/ui-menu-ia-command-taxonomy-map-2026-08-23.md` |
| 命令表 | `src/ui/analysis_commands.cpp` / `.h` |
| 渲染 | `src/ui/mainwindow.cpp` |
| i18n | `translations/ui_menu_strings.json` |
| help | `resources/help/algorithm_help.json` |
| verify | `tools/verify_ui_menu_ia_track.py` |
| QtTest | `tests/ui_menu_ia_track_test.cpp` |

**文档状态：** 2026-08-23 执行完成（脚本门 PASS 后勾选）。
