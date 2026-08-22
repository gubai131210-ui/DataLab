# Qt Creator 双产品线验收 Runbook

> 日期：2026-08-22  
> 范围：报告 Phase 3（§3.1 **61** 项）+ 算法 reference 脚本 + S1–S7 肉眼 PDF  
> **不等于** goal 完成；PDF/A·UA 真合规、vendor oracle、全量 UI i18n 仍开放。

## 0. 脚本预检（无需编译）

```powershell
cd D:\QT_CppPrograms\DataLab
python tools/print_acceptance_status.py
powershell -File tools/phase3_preflight.ps1
```

### 0.1 MinGW 编译注意（中文 Windows 用户名）

若 **Compile Output** 仅显示 `report_text_catalog.cpp.obj Error 1` 而无具体行号，通常是 MinGW `cc1plus` 在 `%TEMP%` 含非 ASCII 路径时静默失败。仓库已在 CMake 中注入 ASCII 编译临时目录（`compile_with_ascii_tmp.cmd`），并将 catalog 拆为 `report_text_catalog_part{1..4}.cpp`。

**你需要：** Qt Creator → **Projects → Build → Run CMake**（重新配置）→ **Rebuild**。Configure 输出应含 `MinGW ASCII compiler temp: .../.compiler_tmp` 与 `MinGW big object format: -Wa,-mbig-obj`。

若仍失败，在 Build Environment 手动设 `TMP`/`TEMP` 为 ASCII 路径（如 `D:\QT_CppPrograms\DataLab\build\.compiler_tmp`）。

大翻译单元（如 `analysis_service.cpp`）若报 `too many sections` / `file too big`，同样由 `-Wa,-mbig-obj` 处理；需 **Run CMake** 后全量 Rebuild。

若 `ranlib` 报 `file truncated`（如 `response_surface_design.cpp.obj`），多为并行/中断构建留下的**损坏 .obj**：关闭正在运行的 DataLab → **构建 → 清除** 或删除对应 `*.obj` 与 `libdatalab_*.a` → 再 **Rebuild**（不必改代码）。

`print_acceptance_status.py` 输出脚本侧 **12 项** 汇总；`phase3_preflight.ps1` 为逐项详细日志。  
可打印 Qt Creator 签收批次：`python tools/print_qt_creator_signoff_batches.py`

通过条件：registry 61/61 · scenario 84/84 · **13/13 竖切** · **3/3 interpretation gate** · **9/9 customer_keeps** · **3/3 domain gate** · interpretation **316/316** · deepen **37/37** · doe-k4-fixture · 5× reference_implementation。

## 1. Qt Creator 测试（3 个目标）

```powershell
python tools/list_qt_creator_test_targets.py --by-target
python tools/list_qt_creator_test_targets.py --by-target --algorithm-regression
```

| 顺序 | Qt Creator 目标 | 项数 | 说明 |
|------|-----------------|------|------|
| 1 | `ReportExportPhase2Test` | 58 | PDF/三模板/跨页预筛主体 |
| 2 | `ReportLocalePhase3Test` | 2 | Linguist + 分面页题 |
| 3 | `NonNormalCapabilityPhase6Test` | 整包 | 含 **Box-Cox 规格限序** 回归（2026-08-22 domain 修正） |

**catalog 改动后（含 `evidence.warranty_exposure_gate` / `interp.warranty_exposure_gate` 等）：**

```powershell
python tools/sync_report_linguist.py --lrelease "D:\Qt\6.11.1\mingw_64\bin\lrelease.exe"
```

（路径按本机 Qt 安装调整；`--skip-qm` 仅同步 JSON/.ts 时不需 lrelease。）

### 1.1 F′ 加深预筛（不在 §3.1 61 计数内；**37** 项 / **6** 个 Qt Creator 目标）

§3.1 全绿后，建议按场景跑 **加深项**（场景映射 78 + global 6 = 84；其中 **37** 项不在 §3.1 61 内）：

```powershell
python tools/list_phase3_prefilter_by_scenario.py
python tools/list_qt_creator_test_targets.py --deepen --by-target
```

| 场景 | 加深重点（示例） |
|------|------------------|
| S3 F′ | scatter/bar/density/hexbin hidden/excluded + hexbin/density EvidenceBundle gate |
| S4 | 保修暴露量 invalid/zero/override + `:gate:warranty_exposure` + interpretation |
| S6 E′/E″/F | Box-Cox 规格限 + Johnson spec-outside + RSM LOF customer_keeps + 域层 `quality_statistics_test` + 路径 F audit |

加深目标分布：`ReportExportPhase2Test`（20）+ `ReportProfilePhase1Test`（9）+ `InterpretationServiceTest`（3）+ `ReportLocalePhase3Test`（2）+ `QualityStatisticsTest`（2）+ `NonNormalCapabilityPhase6Test`（1，Johnson spec-outside 域层）。

### 1.3 产品演进 Track（G1–G8）— **末尾统一测，中间不停**

> 清单：[`samples/product_evolution/unified_track_acceptance_plan.md`](../../samples/product_evolution/unified_track_acceptance_plan.md)

- **开发阶段**：每个 Track 只需 **脚本预检 OK**（如 `verify_g1_g2_track.py`）；**不要**每 Track 结束后强制跑本节全文。
- **统一验收门**（你准备好后 **只跑一次**）：
  1. §0 脚本预检 13/13
  2. **一次** Run CMake + Build All
  3. 各已交付 Track 的测试脚本（如 `run_g1g2_tests.ps1`）
  4. 本文 §1–§2 Phase 3 / deepen / 算法批次（**同一会话内连续跑，中间不停**）
  5. `unified_track_acceptance_plan.md` §5–§6 手工签署

G1+G2 已于 2026-08-22 签收；统一门时 **回归** 即可。

可选：按 **S1–S7 全场景 84 项** 分组列出 Qt Creator 目标：

```powershell
python tools/list_qt_creator_test_targets.py --scenario --by-target
```

### 1.2 按场景分批（S1–S7）

任意场景前先跑 **global 6**：

```powershell
python tools/list_qt_creator_test_targets.py --global-only --by-target
```

再按场景（示例 S4 保修）：

```powershell
python tools/list_phase3_prefilter_by_scenario.py --scenario-id S4
python tools/list_qt_creator_test_targets.py --scenario-id S4 --by-target
```

| 场景 | `--scenario-id` | 约项数 |
|------|-----------------|--------|
| S1 KM 长表 | `S1` | 6 |
| S2 中文 DOE | `S2` | 6 |
| S3 Graph F′ | `S3` | 16 |
| S4 保修/暴露量 | `S4` | 15 |
| S5 空图 | `S5` | 2 |
| S6 audit 附录 | `S6` | 26 |
| S7 PinLength | `S7` | 7 |

可选算法域回归（不在 §3.1，但 dual-line 相关）：

| 目标 | 说明 |
|------|------|
| `ResponseSurfaceDesignPhase4Test` | DOE/RSM/LOF |
| `ReliabilityPhase5Test` | KM/保修/删失 |

### 1.3 算法域回归（Phase 4–6；报告链之外）

报告 §3.1 + deepen 全绿后，建议跑 **reference_implementation 对照** 相关域测（整包即可）：

| 目标 | 推荐子集 | 样例 / 脚本 |
|------|----------|-------------|
| `ReliabilityPhase5Test` | `km_handcalc_baseline_formula_reference` · `weibull_and_lognormal_have_separate_assertions` · `warranty_strata_use_pooled_r_and_measured_exposure` | [`reliability_km_handcalc.csv`](../../samples/phase0_baselines/reliability_km_handcalc.csv) · [`reliability_survival_manual_phase5.md`](../../samples/reliability/reliability_survival_manual_phase5.md) · `scripts/reliability_km_reference.py` |
| `ResponseSurfaceDesignPhase4Test` | `ccd_and_bbd_match_pinned_reference_implementation_golden` · `rsm_lack_of_fit_uses_replicated_coded_points_not_residual_ms` | `scripts/doe_rsm_reference_points.py` · `scripts/rsm_lof_reference.py` |
| `NonNormalCapabilityPhase6Test` | `box_cox_lambda_special_cases_and_limit_order` | `scripts/box_cox_reference.py` |

```powershell
python tools/list_qt_creator_test_targets.py --by-target --algorithm-regression
powershell -File tools/reference_implementation_preflight.ps1
```

## 2. 肉眼 PDF（S1–S7）

索引：[`samples/phase0_baselines/phase3_manual_acceptance_index.md`](../samples/phase0_baselines/phase3_manual_acceptance_index.md)

| 场景 | 样例 | 模板/locale | 说明 |
|------|------|-------------|------|
| S1 | `reliability_km_long_table.csv` | engineer / en-US | KM 长表跨页 |
| S2 | `doe_bbd_k3_factors.json` 或 `doe_ccd_k4_factors.json` | engineer / zh-CN | 中文 DOE 长表 |
| S3 | `graph_faceted_s3.csv` + [`graph_faceted_manual_s3.md`](graph_faceted_manual_s3.md) | engineer / en-US | F′ scatter/bar/density/hexbin hidden/excluded |
| S4 | [`warranty_strata_manual_s4.md`](warranty_strata_manual_s4.md) + [`warranty_exposure_manual_s4.md`](warranty_exposure_manual_s4.md) | 三模板 / en-US | strata + exposure gate |
| S5 | `graph_scatter_all_excluded_s5.csv` | engineer / en-US | 空图 / 全 excluded |
| S6 | [`audit_appendix_manual_s6.md`](audit_appendix_manual_s6.md)（路径 A–E）+ Johnson/Box-Cox 样例 | audit / en-US | audit 附录 + gate PDF |
| S7 | `PinLength_unicode.csv` | engineer / zh-CN + en-US | Unicode 列名 |

签署：[`phase3-cross-page-pdf-manual-acceptance.md`](phase3-cross-page-pdf-manual-acceptance.md) §6。

## 3. 证据索引

- Reference 脚本：[`reference-implementation-index.md`](reference-implementation-index.md)
- 验证矩阵：[`VALIDATION_MATRIX.md`](VALIDATION_MATRIX.md)
- 总计划：[`vertical-slice-algorithms-and-report-product-plan.md`](vertical-slice-algorithms-and-report-product-plan.md)

## 4. 禁止事项

- 脚本/preflight 通过 **不得** 写入 PDF/A·UA 合规或 vendor 对齐。
- 自动化 ≥2 PDF 页 **不得** 替代表头重复肉眼签署。
- 未跑 Qt Creator §3.1 **61** 项 **不得** 勾选 §6「§3.1 自动化」。
- 勾选 §6「F′/E′/E″ 加深 (37)」前须 `--deepen --by-target` 全绿。
