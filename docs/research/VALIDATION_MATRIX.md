# DataLab 验证矩阵（Validation Matrix）

> 创建日期：2026-08-21  
> 用途：区分 `formula_reference` / `reference_implementation` / `vendor_oracle` / `golden`  
> 规则：只有来源类别为 `vendor_oracle` 且经冻结流程的 `golden` 才能声称商业软件对齐  
> Reference 脚本索引：[`reference-implementation-index.md`](reference-implementation-index.md) · Qt Creator runbook：[`qt-creator-dual-line-acceptance-runbook.md`](qt-creator-dual-line-acceptance-runbook.md)  
> 状态列：❌ 未开始 · 🟡 进行中 · ✅ 已冻结 · ⏸ 延后 · ⚪ 待校准

## 证据类型定义

| 类型 | 含义 | 最低元数据 | 可声称 |
|---|---|---|---|
| `formula_reference` | 标准/手册/手算推导 | 公式出处 URL 或文献 + 手算步骤 | 口径正确；**不**声称数值完全一致 |
| `reference_implementation` | R/Python/独立脚本 | 版本、参数、输入 hash、输出、舍入、容差 | 与参考实现一致；**不**自动等于商业软件对齐 |
| `vendor_oracle` | 商业软件/官方示例导出 | 软件版本、选项、输入 hash、导出 hash、导出方式、容差 | 可作为商业对齐候选 |
| `golden` | 项目冻结可重复 fixture | 来源类别 + 版本 + 输入 hash + 参数 + 容差 + 生成脚本 + review | 回归基线；仅当来源=`vendor_oracle` 时可称商业对齐 |

## Phase 0 基线数据集

| 数据集 ID | 路径 | 用途 | 证据类型 | 状态 |
|---|---|---|---|---|
| `phase0_report_capability_pin` | `samples/capability/PinLength.csv` | 报告三模板共用 Facts 基线（既有能力样例） | `formula_reference`（现有能力口径） | 🟡 选定 |
| `phase0_reliability_km_hand` | `samples/phase0_baselines/reliability_km_handcalc.csv` | KM 手算逐步比对（小 N） | `formula_reference` | 🟡 选定 |
| `phase0_reliability_right_censor` | `samples/reliability/reliability_survival.csv` | 右删失 KM/Weibull 既有样例 | `formula_reference` | 🟡 复用；见 [`reliability_survival_manual_phase5.md`](../samples/reliability/reliability_survival_manual_phase5.md) |
| `phase0_doe_ccd_k2` | `samples/phase0_baselines/doe_ccd_k2_factors.json` | 2 因素 CCD 点集契约 | `formula_reference`（NIST CCD） | 🟡 选定；`response_surface_design_phase4_test` 覆盖点数/round-trip |
| `phase3_doe_ccd_k4_long` | `samples/phase0_baselines/doe_ccd_k4_factors.json` | S2 path B 跨页 CCD k=4 | `formula_reference` | 🟡 `verify_doe_ccd_k4_fixture.py` |
| `phase0_doe_bbd_k3` | `samples/phase0_baselines/doe_bbd_k3_factors.json` | 3 因素 BBD 无角点契约 | `formula_reference`（NIST BBD） | 🟡 选定；`response_surface_design_phase4_test` 覆盖 |
| `phase4_doe_ccd_k2_ccf_stdorder` | `samples/phase0_baselines/doe_ccd_k2_ccf_stdorder_golden.json` | CCF k=2 标准序编码点冻结 | `golden`←`reference_implementation` | 🟡 已冻结；**非** vendor_oracle |
| `phase4_doe_bbd_k3_stdorder` | `samples/phase0_baselines/doe_bbd_k3_stdorder_golden.json` | BBD k=3 标准序编码点冻结 | `golden`←`reference_implementation` | 🟡 已冻结；**非** vendor_oracle |

## 报告产品化

| 项 | 证据/验收 | 类型 | 状态 |
|---|---|---|---|
| ReportProfile 三模板字段矩阵 | `phase0-report-evidence-contracts.md` + `report_types.h` | `formula_reference`（产品契约） | ✅ |
| 模板切换不改 Facts | `report_contract_phase0_test` / `report_profile_phase1_test` | 契约测试 | ✅ |
| EvidenceBundle schema | `report_types.h` + `report_assembly_service` | 契约 | ✅ |
| JSON round-trip Profile/Evidence | `report_serialization` + `report_profile_phase1_test` | 契约 | ✅ |
| 客户/工程师/审计渲染 | PDF/`ReportDocument` 过滤；UI 模板页 | 产品 | 🟡 |
| PDF metadata + manifest | `report_export_writer` + `phase2-pdfa-pdfua-assessment.md` | 契约 | 🟡 |
| PDF/A | 默认 `not_validated`；可选 `DATALAB_VERAPDF` 记 pass/fail | 默认未验证；UA 永不由 veraPDF 翻转 | 🟡 |
| PDF/UA | QPainter 无结构树 | 恒 `unsupported` | 🟡 |
| PDF/UA | 验证器接入前 | 默认 `not_validated` / 可能 `unsupported` | ⏸ |
| 中英双语报告 | ADR 0009 + `report_text_catalog` + Linguist 镜像 + `report_locale_phase3_test` | 契约 | 🟡 壳层+关键表题/门禁诊断+catalog↔`.ts/.qm`；**13/13 竖切三模板 guard**（`representative_*_three_report_profiles_*`）；全量菜单 `tr()`/解释正文仍缺口 |
| 三模板 × 双语 visible-layer | `report_export_phase2_test` §3.1（61）+ deepen（37）+ 场景（84）+ **13/13** + **3/3 interp gate** + **9/9 customer_keeps** + **3/3 domain gate** + `tools/phase3_preflight.ps1`（**12/12** 脚本侧） | 契约 | 🟡 脚本侧全绿；Qt Creator 61+37 + S1–S7 肉眼待 sign-off |

## DOE CCD / BBD / RSM

| 项 | 比对计划 | 类型 | 状态 |
|---|---|---|---|
| CCD k=2 点集（cube/star/center） | NIST 定义 + 手算点数 | `formula_reference` | 🟡 |
| CCD CCC/CCI/CCF alpha 行为 | NIST + 诊断契约 | `formula_reference` | 🟡 含客户版 EvidenceBundle `:gate:ccd_beyond_range` 优先保留 |
| BBD k=3 边中点、无角点 | NIST + baseline | `formula_reference` | 🟡 含客户版 EvidenceBundle `:gate:bbd_no_corners` 优先保留 |
| coded ↔ actual round-trip | 固定 half_range 公式 | `formula_reference` | 🟡 |
| 同 seed 运行顺序 | 确定性 RNG | 契约 | 🟡 |
| **CCD k=2 CCF 标准序编码点** | `doe_ccd_k2_ccf_stdorder_golden.json` + `scripts/doe_rsm_reference_points.py` | `golden`←`reference_implementation` | 🟡 已冻结；**非** vendor_oracle |
| **BBD k=3 标准序编码点** | `doe_bbd_k3_stdorder_golden.json` + 同上脚本 | `golden`←`reference_implementation` | 🟡 已冻结；**非** vendor_oracle |
| 与 R `rsm::ccd/bbd` 点集 | 固定 CRAN 版本 + 输入 hash | `reference_implementation` | ⏸ |
| 商业软件运行顺序对齐 | 需导出 | `vendor_oracle` | ⏸ 无 oracle 不得声称 |
| RSM 接线（二次/交互/失拟） | 现有 `rsm_response` 扩展 | 见 backlog | 🟡 设计边界+来源 ID+纯误差/失拟 ANOVA（重复编码点，`formula_reference`）+ [`rsm_lof_fixture.csv`](../samples/phase0_baselines/rsm_lof_fixture.csv) + `scripts/rsm_lof_reference.py`；R/vendor LOF 对齐仍 ⏸ |

## 可靠性 / 保修

| 项 | 比对计划 | 类型 | 状态 |
|---|---|---|---|
| 删失契约（exact/right） | 负时间/反向区间阻止 | 契约 | 🟡 `censoring_contract` + `reliability_phase5_test` |
| KM 手算小样本 | `reliability_km_handcalc.csv` | `formula_reference` | 🟡 |
| KM 独立 reference_implementation | `scripts/reliability_km_reference.py` + handcalc md/csv | `reference_implementation` | 🟡 脚本自验；**非** vendor_oracle |
| KM vs R `survival::survfit` | 固定 R 版本 + hash | `reference_implementation` | ⏸ |
| Weibull MLE + 删失 | NIST 公式 + 参考实现 | `formula_reference` / `reference_implementation` | 🟡 独立断言在 phase5 test |
| Lognormal（独立断言） | 独立参考；禁与 Weibull 共用未验证断言 | `reference_implementation` | 🟡 独立断言；未 pinned R |
| 保修摘要 claims/1000 | `1000*(1-R(Tw))` 手算 | `formula_reference` | 🟡 |
| 保修/可靠性暴露量门禁诊断 en-US | `invalid_exposure_value` / `warranty_zero_exposure` / override 诊断 | `formula_reference` | ✅ catalog + locale + interp `interp.warranty_exposure_gate` + EvidenceBundle `:gate:warranty_exposure` + PDF/report guards（`warranty_exposure_diag_localizes_to_en_us` / `usesWarrantyExposureGateInterpretationBullet` / `pdf_warranty_exposure_*` / `representative_warranty_exposure_*` / `customer_keeps_warranty_exposure_gate_*`） |
| 保修分层分母（失效模式/分组） | 池化 R + 实测/比例暴露量 | `formula_reference` | 🟡 `summarize_warranty_strata` + [`warranty_strata_s4.csv`](../samples/phase0_baselines/warranty_strata_s4.csv)；客户版 EvidenceBundle 分层暴露 limiting 门禁 |
| 保修 reference_implementation | `scripts/reliability_warranty_reference.py` | `reference_implementation` | 🟡 脚本自验；**非** vendor_oracle |
| Aalen–Johansen CIF | 竞争风险 CIF ≠ Fine-Gray | `formula_reference` | 🟡 客户版 `:gate:cif_not_fine_gray`；非 vendor |
| Fine-Gray IPCW | 非 cause-specific Cox / pinned R | `formula_reference` | 🟡 客户版 `:gate:fine_gray_formula_reference_only`；R `survival::finegray` 仍 ⏸ |
| 区间删失 Turnbull（简化网格） | `km_interval` NPMLE | `formula_reference` | 🟡 非 vendor；反序列化拒假合规 |
| 商业软件对齐 | 需 vendor 导出 | `vendor_oracle` | ⏸ |

## 非正态能力

| 项 | 比对计划 | 类型 | 状态 |
|---|---|---|---|
| 稳定性前置 + 不自动合格 | 诊断契约 | 契约 | 🟡 I-MR Rule-1 初筛 + `pass_fail_judgment_allowed=false`（清屏≠verified）；`nonnormal_capability_phase6_test` |
| Box-Cox λ=0/1、负值、规格限 | 既有 Box-Cox + 扩展验收 | `formula_reference` | 🟡 域+应用+解释+i18n+EvidenceBundle（`:gate:box_cox_*`）+ PDF/audit；`scripts/box_cox_reference.py`；S6 路径 C 样例 |
| Johnson 拟合/可逆（研究） | R SuppDists 固定版本 | `reference_implementation` | 🟡 可逆自测；spec-outside 跳过 overall 表 + PDF/report chain（`johnson_spec_outside_support_skips_overall_capability` / `pdf_johnson_spec_outside_support_localizes_to_en_us_without_chinese_leak`）；能力判定仍门禁 |
| Johnson 能力开放门禁 | golden + 尾部 + 人工解释 | `golden`（来源须标明） | ⏸ `gated_research` |

## Graph Builder / EDA

| 项 | 比对计划 | 类型 | 状态 |
|---|---|---|---|
| 受控槽位 + 有限 geom | PlotSpec 扩展 | 契约 | 🟡 主 geom 已接线；facet 编排仍缺口 |
| hidden vs excluded 可区分 | UI/Facts/JSON/报告 | 契约 | 🟡 `row_visibility` + 主 geom 双口径 + `member_source_rows`（interval/bar/violin/pie/category-heatmap/hexbin）+ **scatter/bar/density 受控分面**；Scatter/Bar `pdf_graph_*_faceted_cross_template_*` + `representative_graph_*_faceted_*`（Display N / Analysis N / hidden 计数）；Hexbin `:gate:hexbin_rectangular_bins` + 三模板/PDF/audit chain；Density `:gate:density_curve_not_discrete_marks` + 三模板/PDF/audit chain；相关矩阵热图不伪造 cell 成员 |
| RowId 悬停/联动 | ADR 0007 | 契约 | 🟡 部分图表已有 |

## 更新规则

1. 状态从 ❌/🟡 改为 ✅ 必须附：证据类型、输入 hash（如适用）、容差、测试名、review PASS。  
2. 禁止把 R/Python 对照直接改名为 `golden` 而不走冻结流程。  
3. 禁止在矩阵中把 PDF 打开成功记为 PDF/A 或 PDF/UA ✅。
