# Phase 0：报告证据链与契约基线

> 状态：🟡 契约已冻结（设计 + domain 类型 + 契约测试）  
> 日期：2026-08-21（UTC+8）  
> 性质：Phase 0 交付物；**不等于** Phase 1 三种模板 PDF/JSON 已实现  
> 权威类型：`src/domain/report_types.h`  
> 契约测试：`tests/report_contract_phase0_test.cpp`  
> 验证矩阵：[`VALIDATION_MATRIX.md`](VALIDATION_MATRIX.md)

## 1. 基线审计（当前代码事实）

### 1.1 已有、可复用

| 概念 | 位置 | 审计结论 |
|---|---|---|
| `DataTable` + `ImportMetadata` | `quality_types.h` | 有 `source_path`、`dataset_id`、`filter_summary`、`provider_id`；可作 provenance 输入 |
| `OutputPage` | `quality_types.h` | 含 tables/plots/diagnostics/interpretation/`InterpretationFacts`/`MethodMetadata` |
| `InterpretationFacts` | `quality_types.h` | 结构化 Facts 已覆盖能力/SPC/DOE/RSM/可靠性等；中文解释在 application 层 |
| `RuleEvidence` | `quality_types.h` | 稳定 `id` + status + related_rows；适合挂入 EvidenceBundle |
| `QualityEvidence` / `MethodMetadata` | `quality_types.h` | 样本量、缺失、假设状态、算法版本字段已存在 |
| `StatisticTable` | `quality_types.h` | 主要为字符串表；可选 `column_kinds`/`row_ids`/`rule_ids` |
| JSON 序列化 | `output_serialization.cpp` | `schema_version`、method_metadata、facts round-trip 已有 |
| PDF | `pdf_report_writer.*` | `QPdfWriter` 绘制；仅标题/时间/源路径/页码；**无** Profile/manifest/证据附录 |
| 可靠性 | `reliability.*` + samples | KM/Weibull/右删失已有；删失契约需 Phase 5 加宽 |
| DOE/RSM | `doe_*` / `rsm_analysis.*` | 响应分析/RSM ✅；CCD/BBD 设计生成 **formula_reference + pinned `reference_implementation` golden** 🟡（非 vendor_oracle；见 Phase 4） |
| ADR 0003 | 结构化 Facts | 保持：解释消费 Facts，不反解析表头作为生产契约 |
| ADR 0006 | 简体中文单语 | Phase 3 双语前必须修订 ADR；Phase 0 只冻结 `LocaleTextId` 与报告 locale 字段 |
| ADR 0007 | SPC source_rows | hidden/excluded 不得混用；报告必须保留 RowId |

### 1.2 缺口（Phase 0 明确，留给后续 Phase）

- 无 `ReportProfile` 驱动的模板渲染（本轮仅冻结类型与字段矩阵）。
- 无 `EvidenceBundle` 组装与序列化（类型已定义，application/serialization 未接线）。
- 无 `ReportExportManifest` 写出与 PDF 一致性校验。
- PDF/A、PDF/UA：**默认 `not_validated`**，禁止合规声明。
- 中英文翻译资源与报告语言独立：未实现（ADR 0006 仍有效至 Phase 3）。
- `StatisticTable` 仍大量字符串化数值；Facts 才是数值权威源。

## 2. 数据流（验收要求）

```text
DataTable
  ├─ columns / rows / row_ids
  ├─ source_path + ImportMetadata
  └─ included_rows / excluded (分析配置)
        │
        ▼
  domain algorithms  ──► InterpretationFacts + QualityEvidence + RuleEvidence
        │
        ▼
  OutputPage (tables/plots/diagnostics are presentation of Facts)
        │
        ├── JSON serialization (facts + method_metadata; schema_version)
        │
        └── ReportProfile (customer | engineer | audit)
              │  仅选择/排列展示字段，禁止重算
              ▼
        EvidenceBundle + ReportProvenance
              │
              ├── 预览 / UI
              ├── PDF (可读 + metadata；合规状态 not_validated)
              └── ReportExportManifest + audit JSON
```

**不变量：**

1. 切换 `ReportProfile` **不得**改变 `InterpretationFacts` 任一数值字段。  
2. 中英文文案不得进入 domain Facts 数值字段；只允许稳定 ID / 枚举字符串。  
3. 报告至少能展示：数据源、N、过滤摘要、算法/软件版本、生成时间（Phase 1 起强制渲染）。  
4. 未经验证器时，`pdfa_status` / `pdfua_status` 只能是 `not_validated` 或 `unsupported`。

## 3. ReportProfile 字段矩阵

| 字段 / 内容块 | 客户版 | 工程师版 | 审计版 |
|---|---|---|---|
| 结论摘要 | ✅ | ✅ | ✅ |
| 关键风险与限制 | ✅（必须保留） | ✅ | ✅ |
| 主图 | ✅ | ✅ | ✅ |
| 完整统计表 | ❌ | ✅ | ✅ |
| 算法参数 | ❌ | ✅ | ✅ |
| 规则证据 / 阈值 | ❌ | ✅ | ✅ |
| 异常行联动 | ❌ | ✅ | ✅ |
| 过滤摘要 | 仅 N 影响 | ✅ | ✅ |
| 输入快照 / ImportPlan | ❌ | ❌ | ✅ |
| 算法版本 / 软件版本 | ❌ | ✅ | ✅ |
| 输入 hash / Facts hash | ❌ | ❌ | ✅ |
| 公式引用 / 证据附录 | ❌ | 摘要 | ✅ 完整 |
| 默认 `max_preview_rows` | 20 | 50 | 200 |

实现入口：`make_report_profile(ReportTemplateKind)`。

## 4. EvidenceBundle / EvidenceRef

每个结论应能回答计划书 §3.2 的七个问题。映射：

| 问题 | 字段 |
|---|---|
| 哪个分析 | `MethodMetadata.algorithm` + page id |
| 哪个数据集 | `ReportProvenance.source_dataset_id` / `source_path` |
| 列/过滤/删失/缺失 | `filter_summary` + EvidenceRef(kind=filter) + QualityEvidence |
| 参数/规格/规则 | EvidenceRef(kind=parameter\|rule) + RuleEvidence |
| 源行 | `EvidenceRef.source_rows` / `RuleEvidence.related_rows` |
| 事实/警告/近似 | `EvidenceRole` + diagnostic codes |
| 版本/语言/时间 | `ReportProvenance` + `ReportProfile.locale` |

证据类型枚举与计划书命名对齐：`formula_reference` / `reference_implementation` / `vendor_oracle` / `golden`。

## 5. 语言与 ADR 0006

- Phase 0 冻结 `LocaleTextId` 与 `ReportLocaleSettings`（报告语言 ≠ UI 语言）。  
- **当前产品仍按 ADR 0006 简体中文单语交付。**  
- Phase 3 开始前必须新增 ADR：修订 0006，引入 `.ts/.qm` 与稳定翻译 ID。  
- 在此之前不得声称“双语已完成”。

## 6. PDF 能力矩阵（诚实评估）

| 能力 | 现状 | Phase 目标 |
|---|---|---|
| 可读 PDF（QPdfWriter） | ✅ 基础绘制 | Phase 2 增强页眉页脚/表头 |
| 标题 metadata | 部分（固定中文标题） | Phase 2：标题/作者/语言/模板 |
| 页码页脚 | ✅ 简单页脚 | Phase 2：模板/locale 感知 |
| 字体嵌入策略 | 依赖系统中文字体 | Phase 2 记录 |
| tagged PDF / PDF/UA | ❌ | 评估后 `unsupported` 或外部验证器 |
| PDF/A XMP 声明 | ❌ | 未验证前不得声明合规 |
| manifest 旁路文件 | ❌ | Phase 2 |
| 原子写 + 失败清理 | ❌ | Phase 2 |

## 7. 与现有类型的关系（扩展而非平行重写）

- `OutputPage` 继续作为分析输出权威容器。  
- `EvidenceBundle` **聚合**已有 `RuleEvidence` / `MethodMetadata` / `QualityEvidence`，不复制统计结果。  
- `ReportProfile` 只存展示策略；渲染在 reporting/application。  
- 未来 `DesignFacts`（DOE 设计生成）另立于现有 `DoeFacts`（响应分析），避免语义混淆。

## 8. 人工验收清单（Phase 0）

- [ ] 阅读本文件与 `VALIDATION_MATRIX.md`，确认数据流理解一致。  
- [ ] Qt Creator 配置后构建 `report_contract_phase0_test` 并确认通过。  
- [ ] 确认未在 UI/PDF 文案中出现“PDF/A 合规”或“PDF/UA 合规”。  
- [ ] 确认工作区既有未提交算法改动未被本轮覆盖。  
- [ ] 确认基准数据集目录 `samples/phase0_baselines/` 存在且 README 可读。

## 9. 禁止偷懒（本轮）

- 禁止用三个 if 复制三套报告算法。  
- 禁止先改 PDF writer 再补证据模型（本轮只冻结契约）。  
- 禁止单个 `QString` 同时承载 ID、中文、英文。  
- 禁止把未注明来源的硬编码数字称为 golden。  
- 禁止把“PDF 能打开”写成 PDF/A 或 PDF/UA。
