# Phase 0 基准数据集

本目录存放 **Phase 0 契约冻结** 选定的最小数据集与因素配置，以及 Phase 4 起冻结的部分 `golden`（来源为 `reference_implementation`，**不是** `vendor_oracle`）。

| ID | 文件 | 用途 | 证据 |
|---|---|---|---|
| `phase0_reliability_km_hand` | `reliability_km_handcalc.csv` | KM 手算逐步比对（含右删失） | formula_reference |
| `phase3_reliability_km_long_table` | `reliability_km_long_table.csv` + [`reliability_km_long_table.md`](reliability_km_long_table.md) | S1/S6 跨页 PDF（55 步生存表） | formula_reference |
| `phase3_doe_manual_s2` | `doe_manual_s2_zh_cn.md` + `doe_bbd_k3_factors.json` | S2 中文 DOE 报告人工验收 | formula_reference |
| `phase3_warranty_strata_s4` | [`warranty_strata_manual_s4.md`](warranty_strata_manual_s4.md) + [`warranty_strata_s4.csv`](warranty_strata_s4.csv) | S4 三模板 manifest/PDF 人工验收 | formula_reference |
| `phase3_warranty_exposure_s4` | [`warranty_exposure_manual_s4.md`](warranty_exposure_manual_s4.md) | S4 暴露量 gate 路径 A/B/C | formula_reference |
| `phase3_audit_appendix_s6` | [`audit_appendix_manual_s6.md`](audit_appendix_manual_s6.md) | S6 audit 证据附录路径 A–E | formula_reference |
| `phase3_graph_empty_s5` | [`graph_empty_manual_s5.md`](graph_empty_manual_s5.md) + [`graph_scatter_all_excluded_s5.csv`](graph_scatter_all_excluded_s5.csv) | S5 空图 / 全 excluded 人工验收 | formula_reference |
| `phase3_graph_faceted_s3` | [`graph_faceted_manual_s3.md`](graph_faceted_manual_s3.md) + [`graph_faceted_s3.csv`](graph_faceted_s3.csv) | S3 Graph 分面英文长标题 | formula_reference |
| `phase4_rsm_lof_fixture` | [`rsm_lof_fixture.csv`](rsm_lof_fixture.csv) | RSM 纯误差/失拟 reference_implementation 输入 | reference_implementation |
| `phase3_manual_acceptance_index` | [`phase3_manual_acceptance_index.md`](phase3_manual_acceptance_index.md) | S1–S7 人工验收总索引（§3.1 **61** + deepen **37** + 场景 **84**） | formula_reference |
| `phase0_doe_ccd_k2` | `doe_ccd_k2_factors.json` | 2 因素 CCD 点集/编码契约 | formula_reference |
| `phase0_doe_bbd_k3` | `doe_bbd_k3_factors.json` | 3 因素 BBD 点集契约 | formula_reference |
| `phase4_doe_ccd_k4_long` | [`doe_ccd_k4_factors.json`](doe_ccd_k4_factors.json) | S2 path B 跨页 CCD k=4（≥50 行） | formula_reference |
| `phase4_doe_ccd_k2_ccf_stdorder` | `doe_ccd_k2_ccf_stdorder_golden.json` | CCF k=2 标准序编码点 | golden ← reference_implementation |
| `phase4_doe_bbd_k3_stdorder` | `doe_bbd_k3_stdorder_golden.json` | BBD k=3 标准序编码点 | golden ← reference_implementation |

独立再生脚本（索引见 [`docs/research/reference-implementation-index.md`](../../docs/research/reference-implementation-index.md)）：
- `scripts/doe_rsm_reference_points.py`（CCD/BBD 标准序 golden）
- `scripts/reliability_km_reference.py`（KM 手算 baseline；`reference_implementation`，非 vendor_oracle）
- `scripts/reliability_warranty_reference.py`（保修 claims/1000 + 分层池化 R；`reference_implementation`）
- `scripts/rsm_lof_reference.py`（RSM 纯误差/失拟 fixture；`reference_implementation`）

报告模板共用能力样例复用既有：[`samples/capability/PinLength.csv`](../capability/PinLength.csv)（`phase0_report_capability_pin`；S7 见 [`pinlength_manual_s7.md`](../capability/pinlength_manual_s7.md)）。  
右删失扩展样例复用：`samples/reliability/reliability_survival.csv`。

详见 [`docs/research/VALIDATION_MATRIX.md`](../../docs/research/VALIDATION_MATRIX.md)。  
Qt Creator 验收 runbook：[`docs/research/qt-creator-dual-line-acceptance-runbook.md`](../../docs/research/qt-creator-dual-line-acceptance-runbook.md)（预检：`powershell -File tools/phase3_preflight.ps1`）。
