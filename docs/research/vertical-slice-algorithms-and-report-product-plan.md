# DataLab 算法竖切加宽与报告产品化执行计划

> 研究日期：2026-08-21（UTC+8）  
> 文档性质：面向 `/goal` 执行的研究、范围、分阶段实现与验收合同  
> 适用项目：`D:\QT_CppPrograms\DataLab`  
> 当前状态：调研与计划已完成；**Phase 0 ✅**；**Phase 1–7 🟡**（报告链 + DOE/RSM + 可靠性/保修 + 非正态门禁 + Graph `hidden≠excluded` 契约 + CCD/BBD 工作表落表）。全量 dual-line 验收仍有诚实缺口（golden/vendor、剩余 geom、PDF/A·UA 工具链、完整双语正文等）。

## 0. 目标与使用方式

本计划针对两条产品线：

1. **算法加宽**：DOE 设计生成、可靠性/保修、非正态能力、受控范围内的 EDA Builder。
2. **报告产品化**：客户版/工程师版/审计版模板、PDF 证据链、中英双语。

目标不是把菜单数量铺满，也不是复制 Minitab/JMP 的全部 API，而是让每个能力成为可以：

- 明确输入契约；
- 明确统计假设；
- 生成结构化 `Facts`；
- 由解释层产生可追溯结论；
- 通过 JSON 序列化和反序列化；
- 在帮助系统中说明公式、来源、限制；
- 在单元测试、集成测试和人工 UI 验收中被证伪；
- 在报告中保留数据来源、参数、规则证据和版本信息。

### 0.1 Goal agent 的执行规则

每次 `/goal` 只允许实现一个明确的竖切，建议每轮 2–4 个互相独立但边界清楚的竖切。每个竖切必须按以下顺序交付：

1. 读取本计划和相关 research/backlog 文档；
2. 先确认当前代码 seam 和未提交改动；
3. 写或更新该竖切的公式/来源/范围说明；
4. 先添加失败测试或契约测试；
5. 实现 `domain → Facts → application → serialization → help → UI/report → tests`；
6. 运行静态检查或由用户在 Qt Creator 中构建；
7. 输出本轮验收表和未完成项；
8. 启动一个独立 review agent，只检查需求覆盖、分层和测试证据；
9. review 未通过时不得宣称完成；
10. 更新 backlog、acceptance matrix 和本文件状态。

### 0.2 禁止偷懒总清单

- 禁止只增加菜单项、图标或空对话框，不实现 domain 计算。
- 禁止只生成一张图而没有结构化 `Facts`、解释和序列化。
- 禁止把统计结果全部压成不可查询的字符串，丢失数值、单位、规则 ID 或来源行。
- 禁止把公式参考文献当成商业软件 golden；没有 golden 必须标记 `待校准`。
- 禁止把近似方法、探索性图表写成“证明合格”“证明稳定”。
- 禁止在没有缺失值、删失、异常值、重复行和边界值契约时宣称可靠性或能力分析完成。
- 禁止把 PDF 绘图成功写成 PDF/A 或 PDF/UA 合规。
- 禁止中英文只翻译菜单而遗漏报告标题、表头、诊断、单位、日期和图例。
- 禁止把中英文文本写死在 domain 统计事实中；事实使用稳定 ID，展示层本地化。
- 禁止把多个页面的连接参数、算法参数、模板参数堆在一个对话框。
- 禁止未经用户确认执行破坏性数据变换或覆盖原始快照。
- 禁止为了通过测试删除边界测试、放宽断言或静默吞掉诊断。
- 禁止未更新文档和状态矩阵就把 `❌/🟡/⏸` 改成 `✅`。

### 0.3 比对证据命名

执行 agent 必须区分以下四类证据，不能统称为 golden：

- `formula_reference`：官方公式、定义或手算推导，只证明口径，不证明实现数值完全一致；
- `reference_implementation`：R/Python/独立脚本的可执行参考，必须记录版本、参数、数据和舍入策略；
- `vendor_oracle`：商业软件或官方示例导出的结果，必须记录软件版本、选项、输入文件 hash、导出物 hash、导出方式和数值容差；
- `golden`：项目冻结的、可重复生成并经 review 批准的 fixture；可以来自
  `formula_reference`、`reference_implementation` 或 `vendor_oracle`，但必须记录来源类别、
  版本、数据 hash、容差和生成脚本。只有来源类别为 `vendor_oracle` 时，才允许声称
  “商业软件对齐”。

R/JMP 文档、API 页面或市场功能说明默认只能作为 `formula_reference` 或
`reference_implementation`，不得未经冻结流程直接命名为 `golden`。

## 1. 当前代码基础与复用边界

计划应优先复用当前项目已形成的四层边界：

```text
ui → application → infrastructure/reporting → domain
```

预计复用的 seam（执行 agent 必须重新核对实际代码）：

- `src/domain/quality_types.h`：`DataTable`、`OutputPage`、`Facts`、诊断和来源行契约；
- `src/domain/statistics/`：纯 C++ 统计算法、分布、回归、控制图和可靠性基础；
- `src/application/analysis_service.*`：分析命令的应用编排；
- `src/application/output_builder.*`、`interpretation_service.*`：表格、解释、诊断和规则证据；
- `src/infrastructure/output_serialization.*`：JSON round-trip；
- `src/infrastructure/pdf_report_writer.*`：当前 PDF 绘制出口，需增加可追溯元数据而不是让 writer 直接理解统计算法；
- `src/ui/analysis_commands.*`、`analysis_setup_dialog.*`：命令和分步参数 UI；
- `resources/help/algorithm_help.json`：算法帮助和限制；
- `tests/`：domain、application、serialization、chart、dialog、provider 的既有测试模式；
- `tools/check_layering.ps1`：分层检查；
- `docs/research/comprehensive-analytics-roadmap.md`、`minitab-market-algorithm-backlog.md`、`algorithm-chart-gap-matrix.md`：状态和缺口登记。

### 1.1 建议新增的稳定概念

这些是候选设计，不允许 agent 未经验证直接照抄：

- `DesignFacts`：设计类型、因素、编码水平、运行数、中心点、阻塞、随机化、别名结构、可行性警告；
- `ReliabilityFacts`：分布、删失摘要、参数估计、区间、拟合诊断、风险/保修摘要；
- `CapabilityFacts`：分布/变换、within/overall 估计、规格限、指数、分位数、适用性警告；
- `ReportProfile`：模板 ID、语言、信息密度、证据策略、导出策略；
- `EvidenceBundle`：稳定证据 ID、来源数据/行、参数、公式引用、诊断状态、软件版本；
- `LocaleTextId` 或等价稳定文本 ID：将用户可见文本与 domain 数值分离；
- `ReportExportManifest`：输出文件、模板、语言、输入快照、分析配方、生成时间、校验摘要。

若现有类型已经能表达上述信息，应扩展而不是重复建立平行模型。

## 2. 调研结论与产品决策

### 2.1 DOE：先设计生成，再连接 RSM 分析

NIST 工程统计手册明确将响应面设计用于拟合二次模型。CCD 包含因子或部分因子骨架、中心点和轴点；轴点距离由设计性质和因素数量决定。BBD 使用三水平、球面上的边中点结构，并在部分因素取中心值时减少运行；它避开所有因素同时处于极端的角点。不同设计的可行域、旋转性和预测质量不同，不能只把它们当作行数不同的模板。

产品决策：

- 第一阶段只做**设计生成和设计审查**，输出可导入 worksheet 的设计矩阵；
- 第二阶段才将设计矩阵接入 `rsm_response`；
- 每个设计输出 coded/uncoded 两套水平、运行顺序、标准顺序、随机化种子、中心点和阻塞信息；
- CCC/CCI/CCF 必须明确星点是否超出原始因素范围；
- 不自动保证实验可执行；对超范围、重复运行、不可行组合给出阻止/警告；
- 设计生成器使用确定性 seed，支持复现；
- 不在没有参考数值时声称和商业软件运行顺序完全一致。

主要来源：

- NIST Response Surface Designs：<https://itl.nist.gov/div898/handbook/pri/section3/pri336.htm>
- NIST Central Composite Designs：<https://itl.nist.gov/div898/handbook/pri/section3/pri3361.htm>
- NIST Response Surface Design Comparisons：<https://www.itl.nist.gov/div898/handbook/pri/section3/pri3363.htm>
- NIST Glossary（CCD/BBD/RSM）：<https://www.itl.nist.gov/div898/handbook/glossary.htm>

### 2.2 可靠性/保修：先做删失与审计，再做复杂模型

可靠性能力的核心不是再加一个分布名称，而是正确表达：

- 失效时间和时间单位；
- 右删失、左删失、区间删失；
- 失效模式和分组；
- 试验结束但未失效的观察；
- 参数估计方法和置信区间；
- 保修窗口、暴露量和按时间的可靠度/失效率；
- 小样本、全删失、零失效、重复时间和非法时间的诊断。

产品决策：

- 先统一 `CensoringObservation` 和 `ReliabilityFacts`；
- 先稳定 Weibull/Lognormal 的右删失 KM/参数估计与诊断；
- 再做区间/左删失；
- ALT、Weibayes、修复系统、竞争风险、保修预测暂列后续；
- 零失效报告必须明确“未观察到失效”不等于“证明无失效”；
- 可靠性报告默认生成数据表和删失计数，不只生成曲线。

本计划执行时应以 NIST/官方统计手册和项目已有可靠性研究文档为准；若没有可复核的一手公式或 golden，状态保持 `🟡/⏸`。

### 2.3 非正态能力：分布/变换/估计方法必须显式

非正态能力不能简单调用正态 `Cp/Cpk/Pp/Ppk` 再换一条曲线。必须在事实中记录：

- 原始尺度与分析尺度；
- 变换方法（Box-Cox、Johnson 或其他）；
- 规格限是否在原始尺度转换；
- within/overall 是否有定义；
- 估计方法、分位数算法和区间；
- 转换失败、规格限不适用、尾部样本不足的诊断。

产品决策：

- 先做“分布识别 + 适用性报告 + 明确不判定”的安全版本；
- 再做 Box-Cox 能力，要求可逆变换、规格限转换和结果双尺度展示；
- Johnson 或自动分布选择必须等 golden、尾部验证和人工解释验收后再开放；
- 不把自动选择的最佳拟合分布直接标记为真实过程分布；
- 非正态能力默认列出分布拟合/变换证据和限制。

参考入口：

- NIST EDA：<https://www.itl.nist.gov/div898/handbook/eda/eda.htm>
- NIST Process Monitoring/Quality 章节：<https://www.itl.nist.gov/div898/handbook/pmc/pmc.htm>
- Qt/项目现有能力研究文档中的非正态能力和 golden 延后条款。

### 2.4 Graph Builder：做“受控图表编排器”，不做第二个 Tableau

JMP Graph Builder 的关键是 zones、即时 geom 切换和局部过滤；这不意味着 DataLab 必须复制自由拖拽画布。DataLab 已有命令、角色列、图形 renderer 和属性侧栏，适合做可复现的 `PlotSpec`/图表编排器：

- 选择 X/Y/分组/颜色/面板角色；
- 选择有限 geom 白名单；
- 选择过滤、排序、聚合和缺失处理；
- 预览并生成可保存的图表配置；
- 图表与源行保持联动；
- 不做自由像素布局、插件脚本、可旋转 3D 和全量 dashboard。

产品决策：

- 先扩展现有 PlotSpec/ChartRenderer 的结构化契约；
- 先支持散点、线、柱、箱线、直方图、分组和 facet 的有限组合；
- 每张图必须显示 complete-case N、过滤条件、分组规则和警告；
- 每个图形配置可序列化并在报告重放；
- 采用“角色列 + 属性侧栏”而非大而全拖拽画布。

## 3. 报告产品化调研结论

### 3.1 三种模板不是三套算法

建议三种 `ReportProfile` 只改变展示策略，不改变统计事实：

| 模板 | 目标读者 | 默认内容 |
|---|---|---|
| 客户版 | 不需要复算细节的客户/管理者 | 结论摘要、关键图、规格/风险状态、限制说明、来源摘要 |
| 工程师版 | 工程和质量分析人员 | 参数、统计表、图、诊断、规则证据、异常行联动信息 |
| 审计版 | 审核、供应商、合规人员 | 输入快照、ImportPlan、完整配置、软件版本、公式来源、证据行、诊断、hash、生成时间 |

禁止在模板层重新计算算法。模板只能选择和排列 `OutputPage`、`Facts`、`EvidenceBundle`。

### 3.2 证据链必须是结构化数据

每个结论至少应可回答：

1. 结论由哪个分析产生？
2. 使用了哪个数据集/快照/worksheet？
3. 使用了哪些列、过滤、删失处理和缺失处理？
4. 使用了什么参数、规格限、规则 ID 和阈值？
5. 证据对应哪些源行、点位、窗口或表格单元？
6. 结论是事实、警告、近似、探索性提示还是人工备注？
7. 使用了哪个算法版本、模板版本、语言和生成时间？

建议证据 ID 稳定、可序列化、跨语言不变；中文和英文只改变 label/message。

### 3.3 PDF 出口的现实边界

Qt 官方文档说明 `QPdfWriter` 是通过 `QPainter` 绘制 PDF 的输出设备，支持标题、作者、版本、XMP 等能力；它本身不保证 PDF/A 或 PDF/UA 合规。PDF Association 的资料区分：

- PDF/A（ISO 19005）：长期保存；
- PDF/UA（ISO 14289）：可访问性；
- 两者可以组合，但需要正确的 PDF 版本、标签结构和 XMP 声明；
- 只写元数据声明不等于通过验证。

产品决策：

- 第一阶段只承诺“可读 PDF + 可追溯元数据 + 导出后 manifest”；
- 第二阶段建立 PDF/A/PDF/UA 能力评估，不在未接入验证器前宣称合规；
- PDF 导出必须保留 `ReportExportManifest`，可和 PDF 一起导出；
- 图表和表格需有标题、单位、替代文本或等价文本摘要；
- 需要明确分页、重复表头、字体嵌入、颜色对比、乱码 fallback；
- 若要正式 PDF/UA，优先评估支持 tagged PDF 的专用库/外部流程，而不是把责任全部压给 QPainter。

来源：

- Qt QPdfWriter：<https://doc.qt.io/qt-6/qpdfwriter.html>
- Qt QPdfDocument：<https://doc.qt.io/qt-6/qpdfdocument.html>
- PDF Association PDF/A 与 PDF/UA 指南：<https://pdfa.org/resource/conforming-to-both-pdf-a-and-pdf-ua-best-practice-guide>
- PDF Association ISO 19005-1：<https://pdfa.org/resource/iso-19005-1-pdf-a-1/>
- PDF Association ISO 14289：<https://pdfa.org/resource/iso-14289-pdfua/>

### 3.4 中英双语必须贯穿报告流水线

Qt 官方建议使用 `QTranslator` 加载 Qt Linguist 生成的 `.qm`，通过 `tr()`、`QCoreApplication::translate()` 或 ID-based translation 标记可翻译文本。Unicode UTS #35/CLDR 规定了 locale、数字、日期、排序和复数规则；因此不能简单对字符串做逐词替换。

产品决策：

- domain 只保存稳定 ID、数值、单位 ID、状态 ID；
- application/reporting 根据 locale 生成 label、解释、表头、日期和数字；
- 报告语言随报告 profile 固化，不跟随导出机器当前语言漂移；
- UI 语言与报告语言可以独立；
- 数字、百分号、科学计数法、日期、时间、复数和列排序全部走 `QLocale`/明确格式化器；
- 中英快照测试使用同一份 Facts，验证数值和稳定 ID 不变，只有文本/格式变化；
- 混合语言 fallback 要生成 warning，而不是静默输出部分中文部分英文。

来源：

- Qt Internationalization：<https://doc.qt.io/qt-6/internationalization.html>
- Qt Writing Source Code for Translation：<https://doc.qt.io/qt-6/i18n-source-translation.html>
- Qt QTranslator：<https://doc.qt.io/qt-6/qtranslator.html>
- Unicode UTS #35 LDML：<https://unicode.org/reports/tr35/tr35.html>

## 4. 总体目标拆分与推荐顺序

### Phase 0：契约冻结与基线

**状态：🟡（2026-08-21）** — 契约文档、domain 类型、契约测试与基准数据集已落地；模板渲染/JSON/PDF 仍属 Phase 1–2。

**目标**：在写新算法和模板之前，冻结事实、证据、语言和报告出口边界。

交付：

- [x] `EvidenceBundle` 或与现有类型等价的扩展设计 → `src/domain/report_types.h` + [`phase0-report-evidence-contracts.md`](phase0-report-evidence-contracts.md)
- [x] `ReportProfile` 三种模板的字段矩阵 → 同上
- [x] `LocaleTextId`/现有翻译机制审计 → ADR 0006 仍为单语；Phase 3 前须修订
- [x] PDF 现状能力矩阵 → phase0 文档 §6（默认 `not_validated`）
- [x] 选定基准数据集 → `samples/phase0_baselines/` + PinLength / reliability_survival
- [x] [`VALIDATION_MATRIX.md`](VALIDATION_MATRIX.md)

验收：

- [x] 能画出 `DataTable → OutputPage/Facts → ReportProfile → PDF/JSON` 的数据流
- [x] 模板选择不改变 Facts 数值（契约测试 `report_contract_phase0_test`；渲染未实现前以不变量函数保证）
- [x] 报告中至少能显示 source dataset、N、过滤、软件版本和生成时间 → **Phase 1 渲染**（`pdf_report_writer` / preview + `ReportProvenance`；`report_profile_phase1_test`）
- [x] 中英文本不进入统计核心（LocaleTextId 约定；双语未实现）
- [x] 未实现 PDF/UA 不得出现“合规”字样（manifest 默认 `not_validated`）

禁止偷懒：

- 禁止用三个 if 分支复制三套报告；
- 禁止先改 PDF writer 再补证据模型；
- 禁止用一个 `QString` 字段同时承载 ID、中文显示和英文显示；
- 禁止把人工 golden 写成未注明来源的硬编码数字。

### Phase 1：报告证据链与 Profile

**状态：🟡（2026-08-21）** — ReportDocument 装配、三模板过滤、JSON round-trip、模板选择页、预览与 PDF 接线已落地；PDF manifest/原子写属 Phase 2；双语属 Phase 3。

**目标**：先把当前已有分析变成可交付报告，再接新算法。

交付：

- [x] `ReportProfile`（`report_types.h` + `make_report_profile`）
- [x] 证据摘要、规则证据、来源行、参数、诊断和 provenance（`report_assembly_service`）
- [x] 客户/工程师/审计三模板展示过滤
- [x] JSON round-trip（`report_serialization`）
- [x] 页面目录和审计附录（审计版 evidence appendix + `.audit.json`）
- [x] UI 选择模板的独立页面（`report_template_dialog`）
- [x] 预览中显示模板、语言和证据级别（`report_preview_dialog`）

验收标准：

1. [x] 同一个 `OutputPage` 生成三种报告，事实数值逐字段相同（`report_profile_phase1_test`）
2. [x] 客户版隐藏技术细节，但不能隐藏关键限制和风险
3. [x] 工程师版显示公式参数、规则名称、阈值、异常行
4. [x] 审计版显示 ImportPlan/worksheet、过滤、算法版本、来源、hash、生成时间
5. [x] JSON 保存后重新加载，模板和证据 ID 不变
6. [x] 删除原始数据库连接后仍能识别 worksheet_snapshot
7. [x] 测试覆盖空证据、多个证据、证据行超出预览、无行 ID

比对：

- 对同一分析生成三种 JSON，使用脚本只比较 Facts 和 Evidence ID；
- 对比模板字段矩阵，确认不是简单“少画几张图”；
- UI 人工验收：长标题、中文、英文、超长表格、跨页表头、无图和诊断失败。

### Phase 2：PDF 可读性、manifest 和验证入口

**状态：🟡（2026-08-21）** — manifest/audit 旁路、一致性校验、原子导出、PDF 元数据与页眉页脚、PDF/A·UA 评估文档与 **可选 `DATALAB_VERAPDF` 钩子**已落地；默认仍 `not_validated` / UA `unsupported`；未默认捆绑 veraPDF/PAC 二进制。

交付：

- [x] PDF 元数据：标题、作者/creator、模板与 report id（页眉）
- [x] 统一页眉/页脚、页码；表头跨页重复；空值/非有限显示
- [x] PDF 外的 `.manifest.json` + `.audit.json`
- [x] 导出完成后的内部一致性检查（`manifest_matches_document`）
- [x] 可选 PDF 文本抽取测试（可后续加强）→ **字节级 header 字面量扫描**（`optional_pdf_byte_scan_finds_report_id_and_facts_hash_literals`；非完整 PDF 文本抽取 / 非合规证明）
- [x] en-US 本地化后原子导出包联调加深（`en_us_localized_package_keeps_locale_honesty_and_audit_evidence`：locale/manifest/audit/解释诚实句；仍默认 PDF/A `not_validated`、UA `unsupported`）
- [x] PDF/A/PDF/UA 能力评估文档和验证器接入点 → [`phase2-pdfa-pdfua-assessment.md`](phase2-pdfa-pdfua-assessment.md)（`DATALAB_VERAPDF` + merge 诚实规则）

验收：

- [ ] 同一报告在中英两种语言生成不混语 → **Phase 3**（automated：**13/13 竖切** 混语 guard + 三模板 guard + **13/13 cross_template PDF** + **13/13 scenario 映射**；§3.1 **61** 项 + deepen **37** + 场景 **84**；S1–S7 见 `tools/list_phase3_prefilter_by_scenario.py` / `--scenario-id`；**S1–S7 人工 PDF 待 sign-off** — 见 [`phase3-cross-page-pdf-manual-acceptance.md`](phase3-cross-page-pdf-manual-acceptance.md) §3.3 + §5）
- [x] PDF 和 manifest 中的 report ID、template ID、locale、Facts hash 一致
- [x] 源行证据在报告中能找到可读摘要（规则异常行）
- [x] 空值、无穷等显示兜底；Unicode 列名随 QString 路径
- [x] 导出失败不留下伪成功文件
- [x] 未通过专用验证器时状态为 `not_validated`，不能声称 PDF/A/UA

测试：

- [x] `report_export_phase2_test`：manifest 一致性、假合规拒绝、JSON RT、原子包导出、失败清理
- [x] EvidenceBundle `label_text_id`/`notes_text_id` en-US catalog 解析回归（`evidence_bundle_label_text_ids_resolve_in_en_us`）
- [x] PDF 证据附录使用 `label_text_id` 本地化标签（保留 `evidence_id` 追溯；`pdf_evidence_appendix_uses_localized_label_text_ids`）
- [x] `.audit.json` 证据条目携带 `label_text_id`/`kind`/`evidence_id` 且 en-US 可解析（`audit_json_evidence_refs_carry_label_text_ids`）
- [x] 图表空态 `chart.no_displayable_data` + PDF `language_tag` 传递（`pdf_empty_chart_renders_localized_no_data_message`）；预览证据行与 PDF 共用 `format_evidence_ref_display_line`
- [x] Phase 5 保修摘要 en-US PDF 字节扫描（`pdf_warranty_summary_page_localizes_to_en_us_without_chinese_leak`：先 `localize_report_document` 再导出；非 PDF/A/UA 合规证明）
- [x] Phase 5 scalar 保修摘要三模板 PDF 字节扫描（`pdf_warranty_summary_cross_template_table_visibility_and_en_us_locale`：customer 隐藏统计表、engineer/audit 保留；facts_hash 一致；与分层 `pdf_warranty_cross_template_*` 对称）
- [x] Phase 5 保修分层表 en-US PDF 字节扫描（`pdf_warranty_strata_tables_localize_to_en_us_without_chinese_leak`：失效模式分母追溯表；**+ 代表性竖切 visible_tables guard**；非跨页人工验收）
- [x] Phase 5 跨模板保修 PDF 可见性 + en-US 字节扫描（`pdf_warranty_cross_template_table_visibility_and_en_us_locale`：customer 隐藏统计表、engineer/audit 保留；facts_hash 一致；audit 证据附录；仍非多页人工验收）
- [x] Phase 6 Johnson 能力门禁 en-US PDF + audit JSON（`pdf_johnson_capability_gate_localizes_and_audit_evidence_carries_label_text_id`；合格判定保持关闭）
- [x] Phase 6 正态能力 stability 门禁 en-US PDF + audit JSON（`pdf_normal_capability_stability_gate_localizes_and_audit_evidence_carries_label_text_id`；`pass_fail_judgment_allowed=false`）
- [x] Phase 6 Johnson 能力三模板 PDF 字节扫描（`pdf_johnson_capability_cross_template_table_visibility_and_en_us_locale`：customer 隐藏 Selected Family 表；保留 gate 诊断；audit 证据附录）
- [x] Phase 6 非正态能力三模板 PDF 字节扫描（`pdf_nonnormal_capability_cross_template_table_visibility_and_en_us_locale`：customer 隐藏 Capability indices；保留 Z-score 警告；audit 证据附录）
- [x] Phase 5 KM/Weibull 三模板 PDF 字节扫描（`pdf_reliability_km_and_weibull_cross_template_table_visibility_and_en_us_locale`：customer 隐藏生存/参数表；audit 证据附录）
- [x] Phase 6 非正态能力 Weibull Z-score + stability 门禁 en-US PDF + audit JSON（`pdf_nonnormal_capability_stability_gate_localizes_and_audit_evidence_carries_label_text_id`；与 `nonnormal_capability_phase6_test` 域门禁互补）
- [x] Phase 6 Box-Cox en-US PDF 诚实句字节扫描（`pdf_box_cox_honesty_localizes_to_en_us_without_chinese_leak`；非合格判定）
- [x] Phase 6 Box-Cox 三模板 PDF 字节扫描（`pdf_box_cox_cross_template_table_visibility_and_en_us_locale`：customer 隐藏变换/能力表；保留 pass/fail 诚实 limitation）
- [x] Phase 7 Graph Builder 分面散点 en-US PDF 字节扫描（`pdf_graph_builder_faceted_scatter_localizes_to_en_us_without_chinese_leak`；真实 `GraphService::run` 路径）
- [x] Phase 7 Graph 分面散点三模板 PDF 字节扫描（`pdf_graph_scatter_faceted_cross_template_plot_visibility_and_en_us_locale`：customer 无 Facet=/Display N=；max_plots=1；**2026-08-22 F′ 加深 hidden 双口径**）
- [x] Phase 7 F′ 分面 scatter/bar/density/hexbin 三模板 + cross_template 加深（hidden/excluded 计数 + hexbin/density EvidenceBundle gate；见 §3.1 F′ + `tools/list_phase3_prefilter_by_scenario.py` S3）
- [x] Phase 5 保修暴露量 report chain（`:gate:warranty_exposure` + `interp.warranty_exposure_gate` + S4 人工路径 A/B/C；见 `warranty_exposure_manual_s4.md`）
- [x] Phase 6 Box-Cox 规格限加深 report chain（`:gate:box_cox_spec_limit` + `customer_keeps_box_cox_*` + `box_cox_spec_limit_diag_localizes_to_en_us` + S6 路径 C）
- [x] Phase 6 Johnson spec-outside report chain（`:gate:johnson_spec_limit` + interpretation + S6 路径 D）
- [x] Phase 3 **13/13 竖切** companion `pdf_*_cross_template_*` PDF 字节扫描（表/图/参数裁剪 + audit 证据附录；KM 长表 S1 + DOE CCD k=4 长表 S2 path B 跨页预筛）
- [x] Phase 7 Graph Builder 分面条形图 + 密度图 en-US PDF 字节扫描（`pdf_graph_builder_faceted_bar_and_density_localize_to_en_us_without_chinese_leak`；hidden/excluded 诚实诊断 + density 非离散 mark 诊断；真实 `GraphService::run` 路径）
- [x] Phase 7 Graph Builder 分面区间图 + 小提琴图 en-US PDF 字节扫描（`pdf_graph_builder_faceted_interval_and_violin_localize_to_en_us_without_chinese_leak`；受控分面截断诊断 + hidden/excluded；真实 `GraphService::run` 路径）
- [x] Phase 7 Graph Builder 分面 Hexbin en-US PDF 字节扫描（`pdf_graph_builder_faceted_hexbin_localizes_to_en_us_without_chinese_leak`；矩形分箱诚实诊断 + 受控分面截断；真实 `GraphService::run` 路径）
- [x] Phase 7 Graph Builder 分面等值线 + 矩阵图 en-US PDF 字节扫描（`pdf_graph_builder_faceted_contour_and_matrix_localize_to_en_us_without_chinese_leak`；受控分面截断 + hidden/excluded；真实 `GraphService::run` 路径）
- [x] Phase 7 Graph Builder 分面气泡图 + 时间序列图 en-US PDF 字节扫描（`pdf_graph_builder_faceted_bubble_and_time_series_localize_to_en_us_without_chinese_leak`；受控分面截断 + hidden/excluded + `row_visibility_contract` 诊断 en-US；真实 `GraphService::run` 路径）
- [x] Phase 7 Graph Builder 分面面积 + 平行坐标 + 边际图 en-US PDF 字节扫描（`pdf_graph_builder_faceted_area_parallel_and_marginal_localize_to_en_us_without_chinese_leak`；area 非 CI 诚实句 + parallel 标准化 caption；真实 `GraphService::run` 路径）
- [x] Phase 7 Graph Builder 分面正态概率图 + ECDF en-US PDF 字节扫描（`pdf_graph_builder_faceted_probability_and_ecdf_localize_to_en_us_without_chinese_leak`；直线性诚实句 + row_visibility 诊断；真实 `GraphService::run` 路径）
- [x] Phase 7 Graph Builder 分面相关图 + 相关热图 en-US PDF 字节扫描（`pdf_graph_builder_faceted_correlation_and_heatmap_localize_to_en_us_without_chinese_leak`；相关矩阵非观测层诚实句 + 颜色范围 caption；真实 `GraphService::run` 路径）
- [x] Phase 7 Graph Builder 分面类别热图（row/col/z）en-US PDF 字节扫描（`pdf_graph_builder_faceted_category_heatmap_localizes_to_en_us_without_chinese_leak`；单元格为组内均值 + member_source_rows 保留 hidden 过滤；真实 `GraphService::run` 路径）
- [x] Phase 7 Graph Builder 饼图（无分面）en-US PDF 字节扫描（`pdf_graph_builder_pie_localizes_to_en_us_without_chinese_leak`；类别/合并阈值 caption + row_visibility；真实 `GraphService::run` 路径）
- [x] Phase 3 三模板 × 双语 visible-layer guard（**13/13 代表性竖切各一测试**：保修摘要·分层 / Graph 分面散点 / DOE CCD·BBD / RSM LOF / KM·Weibull·Lognormal / 正态·非正态·Johnson·Box-Cox；与对应 `pdf_*` 字节扫描互补）
- [x] Phase 3 代表性竖切 guard 加深：`representative_vertical_slice_reports_localize_without_cross_language_leak` **13 竖切**（CCD+BBD/Graph scatter/KM/保修/保修分层/Box-Cox/Johnson/非正态/正态能力/RSM/Weibull/Lognormal）+ 显式禁词 + 门禁/表题正例断言
- [x] Phase 4 RSM 失拟门禁 en-US PDF + audit JSON（`pdf_rsm_lof_gate_localizes_and_audit_evidence_carries_label_text_id`；`formula_reference` 诚实；非 vendor_oracle 对齐）
- [x] Phase 4 RSM 失拟三模板 PDF 字节扫描（`pdf_rsm_lof_cross_template_table_visibility_and_en_us_locale`：customer 隐藏统计表；audit 证据附录；与保修 cross_template 对称）
- [x] Phase 5 KM + Weibull + Lognormal en-US PDF 字节扫描（`pdf_reliability_km_and_weibull_localize_to_en_us_without_chinese_leak`；真实 `AnalysisService::reliability` 路径）
- [x] Phase 4 CCD + BBD 设计页 en-US PDF 字节扫描（`pdf_doe_ccd_and_bbd_design_localize_to_en_us_without_chinese_leak`；真实 `doe_response_surface_design` 路径）
- [x] Phase 4 CCD + BBD 三模板 PDF 字节扫描（`pdf_doe_ccd_and_bbd_cross_template_table_visibility_and_en_us_locale`：customer 隐藏 Design matrix/Design information；与保修/RSM cross_template 对称）
- [x] Phase 4 DOE 设计生成 EvidenceBundle audit PDF + JSON（`audit_json_doe_design_generation_carries_label_text_ids`：`design_formula_reference_only`、`:formula:design_generation`、`bbd_no_corners`；audit profile + `localize_report_document` + 证据附录 PDF 字节扫描）
- [x] catalog↔Linguist 镜像同步（`tools/sync_report_linguist.py` + Phase 3 `linguist_mirror_matches_catalog_and_qm_loads`；含 `chart.no_displayable_data`、`diag.prefix.*` 等新 ID）
- [x] `param_true` 审计残渣双语回归（跳过 N* / 区组·缺失 param / Fisher 2×2 / 双样本 t 组 2 / SPC 未触发证据；`param_true_audit_leftovers_localize_to_en_us`）
- [x] Phase 3 S2 DOE CCD k=4 长设计矩阵跨页预筛（`pdf_doe_ccd_k4_long_design_matrix_spans_pages_en_us`：≥50 行设计矩阵、≥2 PDF 页、en-US + zh-CN 混语 guard；**不替代**表头重复肉眼验收）
- [x] Phase 3 S4 三模板 manifest `facts_hash` 预筛（`pdf_warranty_strata_three_templates_manifest_facts_hash_match_en_us`）
- [x] Phase 3 S5 Graph 全 excluded 空图预筛（`pdf_graph_scatter_all_excluded_renders_localized_no_data_en_us`；与 `pdf_empty_chart_*` 互补）
- [x] Phase 3 S6 audit 长表 + 证据附录跨页预筛（`pdf_audit_km_long_table_evidence_appendix_spans_pages_en_us`：audit + KM 长表、≥2 PDF 页、Evidence appendix + audit JSON `label_text_id`、PDF/A `not_validated`/UA `unsupported`；**不替代** S6 肉眼布局签署）
- [x] 跨页 PDF 人工验收 checklist（[`phase3-cross-page-pdf-manual-acceptance.md`](phase3-cross-page-pdf-manual-acceptance.md) §3.1 **61** + deepen **37** + 场景 **84** + **13/13 竖切 scenario**（`verify_vertical_slice_scenario_coverage.py`）+ [`phase3_manual_acceptance_index.md`](../samples/phase0_baselines/phase3_manual_acceptance_index.md)（含 `--scenario-id` 分批）+ [`qt-creator-dual-line-acceptance-runbook.md`](qt-creator-dual-line-acceptance-runbook.md) + `tools/phase3_preflight.ps1`；**S1–S7 肉眼签署待用户**）
- [ ] 跨页表格人工验收 **签署**（Qt Creator；见 checklist §6）
- [x] PAC / tagged-PDF **诚实接入点**（`DATALAB_PAC` + `merge_optional_pac_result`：即便 PAC exit 0 也保持 `pdfua_status=unsupported`；默认不宣称合规）
- [ ] 真实 PAC/tagged-PDF 流水线（非 QPainter）仍属另立竖切

### Phase 3：中英双语基础设施

**状态：🟡（2026-08-22）** — 报告稳定文本目录、报告语言独立选择、缺失翻译诊断、zh/en 快照测试与 ADR 0009 已落地；可见层表题/关键诊断码 en-US 映射已加强；**catalog↔`report_strings.json`/`DataLab_*.ts`/`*.qm` 镜像同步脚本 + UI `DATALAB_UI_LANG` 加载 en/zh `.qm` 已落地**；**主菜单/顶层分析菜单/`DataLabUi` tr() + `ui_menu_strings.json`（含全部分析命令 menu/title/group 英译，以及分析设置对话框角色/输入/chrome 英译，≥500 entries）已接线**；**可靠性/DOE/能力门禁固定解释句英译已加深**；**可见层 chrome、`请*` 门禁、METHOD 跳过诊断与 param chrome 继续加深（~2272 report catalog）**；动态拼接解释正文全量双语与人工 PDF 跨页验收仍未完成。

交付：

- [x] 语言选择和报告语言独立设置（模板对话框）
- [x] `.ts` 资源 + catalog 权威源；`tools/sync_report_linguist.py` 同步 JSON/.ts 并可 `lrelease` 出 `.qm`
- [x] UI 翻译器按 `DATALAB_UI_LANG` 加载 `DataLab_en_US.qm` / `DataLab_zh_CN.qm`（默认 zh-CN；报告 locale 仍独立于 Profile）
- [x] 稳定文本 ID（`report_text_catalog`）
- [x] 报告 chrome / 模板 / 证据级别 / 规则标签翻译
- [x] 报告 locale 字段驱动日期标签；数字格式不改写 Facts
- [x] 翻译缺失诊断和 fallback
- [x] 中英快照测试（`report_locale_phase3_test`，含 Linguist 镜像一致性）
- [x] 分析命令 menu/title/group 英译目录（`ui_menu_strings.json` + merge 脚本）
- [x] 分析设置对话框角色/输入/chrome 英译（`ui_tr` + `merge_dialog_role_ui_strings.py`；类型匹配仍用中文 token）
- [x] 可靠性/DOE/能力门禁等**固定**解释句英译（`report_localization` exact/clause map）
- [x] 动态模板加深：KM / 保修 / 证据类型 + 分析错误/限制前缀 + Weibull β 行
- [x] 动态模板加深：DOE 显著项/设计因子中心点、量具稳定性超限、MSA 偏倚斜率、KM 计数、分模式拟合数、不可识别原因
- [x] 固定解释句加深：~54 条 DOE/MSA/可靠性/RSM/EDA/表分析诚实句（exact map + catalog）
- [x] 动态模板加深：控制图超限计数、特殊原因策略/规则触发、效应 Pareto、等值线 hold（编码/实际）
- [x] 动态模板加深：σ 分解（组内/组间/σ_BW）、t/Z 检验结论与假设句、正态性判定、属性能力 %Defective/Process Z/DPU
- [x] 动态模板加深：Grubbs/Dixon 异常值、Pearson/Spearman/偏相关/协方差、二项 OC、TOST 等价性、ANOM、ANOVA 跟进句
- [x] 动态模板加深：单/两比例、单/双样本泊松率、Box-Cox λ/N/SD
- [x] 动态模板加深：功效/样本量结论与建议、预测 MASE 建议句
- [x] 动态模板加深：预测 MAPE、回归 R²/VIF/打标观测、ANOVA 显著项与 Tukey
- [x] 动态模板加深：DOE 设计类型/生成器、Desirability 多响应、RSM 二次摘要/|t|/设计来源/失拟（formula_reference）
- [x] 动态模板加深：MSA EMP ICC/Probable Error、ndc、%Study Var、偏倚/线性数值句、Type 1 P/Cgk/%Tolerance
- [x] 动态模板加深：非参数（Wilcoxon/Kruskal/Mood/Friedman/Sign/Runs、Dunn/Steel–Dwass/Nemenyi、Walsh/位置差 CI）
- [x] 动态模板加深：列联/Fisher/交叉表/GOF/McNemar/Cochran（含残差定位与未可计算诚实句）
- [x] 动态模板加深：Logistic/PCA/K-Means/CART/ADF/Poisson/Isolation Forest/描述统计（含诚实限制句）
- [x] 动态模板加深：EDA/Graph（密度/hexbin/小提琴/条形/NIST 四图/分面）+ 因果图 + Turnbull 区间 KM + ACF/PACF/Ljung–Box
- [x] 动态模板加深：SPC（Z-MR/MA/运行图/区域图）+ DOE 设计生成（PB/BBD/CCD）+ 保修分层 + CCF/Correlogram/逐步回归 + Multi-Vari/变异性图
- [x] 动态模板加深：容差区间 / 个体分布识别 / 等方差 + Cpk·Ppk 判定句 / Johnson·非正态·组间组内诚实句（保持门禁，不开放合格判定）
- [x] 动态模板加深：柏拉图最大类/累计占比 + 能力多筛查 gate_status 行（I-MR/双峰/Hartigan/混合；仍不开放合格判定）
- [x] 动态模板加深：预测/分解诚实建议 + 属性一致性 Kappa/Kendall + ANOVA Grouping/区间图 + Durbin-Watson + I-MR σ/阶段/历史限 + 多元 SPC + Laney Sigma Z
- [x] 动态模板加深：可靠性 Weibull β 提示/阈值·百分位、两参数指数、三参数/两参数对数正态 + DOE 优化缺预测区间诚实句（修复 β 提示 en 混语）
- [x] 可见层表题/表头/图题/页标题加深（DOE/能力/SPC/MSA/ANOVA/回归核心）+ Graph `显示 N`/`分析 N`/`分面` 字幕 token 英译（非全量 400+ 标题终态）
- [x] `parameter_summary` / `method_metadata.parameters` 常用 chrome token 英译（变量/响应/方法/测量/DOE/SPC 公式等；未知中文短语保持原样，不全量）
- [x] 可见层 chrome 加深（属性能力/SPC/Pareto·C&E；DOE/RSM 响应优化表头与 CCD/BBD NIST 诊断；可靠性暴露量/区间 KM 参数 + Graph 轴；EDA 四图检查假设 + 箱线统计须/异常点；Dist-ID/Logistic/容差表头·判定·params；Graph Builder 请选择门禁 + 相关矩阵字幕诚实句 + 条形 hidden/excluded；经典推断+非参数+MSA/RSM/可靠性/ML·TS 等 application 层 `请*` 门禁全覆盖；METHOD 跳过诊断模板 + 剩余 param.summary label=；Linguist ~2272 report entries）
- [x] 动态模板加深：卡方/GOF 期望频数有效性建议与相关诊断（合并类别建议、lt5/lt1、列联表形状错误等；经 `分析限制：`/`分析错误：` 细节与 diagnostic.code 双路径）
- [x] 动态模板加深：McNemar/二元编码/Cochran Q/Fisher/单双比例/泊松率诊断正文（`localize_known_plain_message` + diagnostics fallback；共享 code 按原文精确匹配）
- [x] 动态模板加深：非参数族诊断（Mann–Whitney/Wilcoxon/Kruskal/Friedman/Sign/Mood/Runs/Run Chart；共享 `small_sample_normal_approximation`/`zero_rank_variance` 按原文匹配）
- [x] 动态模板加深：MSA Gage R&R 族诊断（交叉/嵌套/Expanded；共享 `ndc_investigation`/`invalid_tolerance`/`negative_variance_component` 按原文匹配）
- [x] 动态模板加深：MSA Type 1 / Bias-Linearity / Stability 诊断与配置门禁句
- [x] 动态模板加深：属性一致性 Kappa/Kendall 诊断与配置门禁句（不伪造 W=1 / 不可识别 SE）
- [x] 动态模板加深：线性回归域诊断/假设说明/规则摘要（含表单元格 `localize_known_plain_message` 回退）
- [x] 动态模板加深：单因素/双因素 ANOVA + Tukey 诊断与假设说明（含配置门禁与伪造 F/P 诚实句）
- [x] 动态模板加深：DOE 因子/部分析因生成器 + CCD/BBD 设计契约 + 纯误差/失拟/曲率诊断
- [x] 动态模板加深：RSM 分析编码/纯误差/失拟诊断（含 `formula_reference` 非 vendor_oracle 诚实句英译）
- [x] 动态模板加深：经典可靠性 KM/Weibull/Log-rank/指数/对数正态域诊断（含似然无界不伪造参数诚实句）
- [x] 动态模板加深：Logistic/Poisson/逐步/有序 Logistic 域诊断（含 Hosmer–Lemeshow 不可用、IRLS/分离、α 振荡诚实句；共享 `hosmer_lemeshow_not_computed`/`max_iter` 按原文精确匹配）
- [x] 动态模板加深：单变量 SPC 域诊断（Nelson 八规则建议 + I-MR/Xbar-R/I-MR-R/S/EWMA/CUSUM/区域图/Z-MR 门禁与诚实句；`non_finite_input`/`nelson_estimate_ignored` 按原文精确匹配；与带「表」的 Z-MR 解释句共存）
- [x] 动态模板加深：删失契约/保修摘要/分模式拟合静态诊断（含 formula_reference 非 vendor_oracle 诚实句；事件列/删失类型列静默改写拒绝句）
- [x] 动态模板加深：多元 SPC 域诊断（Hotelling T² / MEWMA / GV / EMP；共享 `need_multivariate`/`invalid_matrix`/`insufficient_observations` 按原文精确匹配）
- [x] 动态模板加深：竞争风险 CIF/Fine-Gray 静态诊断（Aalen–Johansen 范围诚实句 + Fine-Gray IPCW 门禁/奇异/发散/协变量优先级；共享 `fine_gray_singular`/`fine_gray_covariate_priority` 按原文精确匹配）
- [x] 动态模板加深：ML 族静态诊断（Isolation Forest / K-Means / CART / LDA / 层次聚类 / 高斯混合；与解释诚实句近义不同原文共存；kmeans 长迭代句不抢 `已达最大迭代次数。`）
- [x] 动态模板加深：过程能力/非正态门禁静态诊断（稳定性·双峰·Hartigan·混合门禁诚实句 + Johnson 研究预览门禁 + Box-Cox/正态性/属性能力假设句；**不开放**合格判定；与 BIC 搜索「门禁」近义句精确共存）
- [x] 可见层表题/图题加深：DOE 响应优化嵌套表 + Multi-Vari/变异性图 + EDA 四图/箱线/因果/游程 + 区域图/广义方差/T² 点表（非全量标题终态）
- [x] Graph Builder 分面 chrome：`区域图（分面）`/`面积区域图` 与 SPC Zone 解耦；面板标题 `XXX · ` 前缀英译；Graph/EDA 静态门槛诊断 8 条
- [x] 可见层表题/图题加深：非参数族 + 属性一致性（Mann–Whitney/Wilcoxon/Sign/Runs/Fisher/Cochran Q/Mood/Kruskal/Friedman/Nemenyi + AA Kappa/Kendall 表题；秩和表头/是否；阳性率·评估者轴；部件/评估者/Kappa 参数摘要；`Ties 修正` 复用既有 `header.ties_correction`）
- [x] SPC 参数表 chrome：规则策略/启用规则·测试/判定口径/触发点数/政策值/适用性正文；`I-MR-R/S 参数`；属性图 ` 方法与参数`/` 参数` 后缀组合；子组数·大小与历史/估计括号摘要 token
- [x] 可见层表题/图题加深：等价性六页标题 + Forecast/ARIMA/季节/PCA/KMeans/CART/逐步/功效/OC/ANOM 嵌套表·图·表头·系列（含 ANOM 变量名后缀）
- [x] 可见层表题/图题加深：ADF/Poisson/Isolation Forest/Bootstrap/HClust/有序 Logistic/LDA/CCF/Correlogram 嵌套表·图·轴·系列与参数摘要 token（回归/预测变量/链/异常数/类数/准确率/水平）
- [x] 规则证据加深：MSA ndc/设计平衡（含交叉不平衡与嵌套）/交互/负方差/%Contribution/%Tolerance + 属性一致性 Kappa/加权κ/Kendall 证据与建议（`localize_known_plain_message` 精确匹配；rule_id 不译）
- [x] 可见层表题/图题加深：ANOVA/交叉方差分量/Logistic 拟合优度·OR·残差/容差直方图/I-MR-R/S 子组/缺陷计数·Pareto 后缀/泊松参数/KM 区间摘要 + 率比·事件概率·类别等轴
- [x] 跳过/complete-case 诊断加深：等价性·回归·比例·游程/运行图·平滑·Logistic·容差·分解·Nested Gage·PCA/DOE/小提琴/Multi-Vari/Target + `跳过 N 个…` 计数模板（含 * / N* / 描述统计 / 泊松 GOF）
- [x] SPC/能力/ANOM 门禁诊断加深：Zone/Z-MR/MA σ、组内 σ/c4、LSL/USL/均值、组间能力、ANOM 组数/方差（exact `localize_known_plain_message`）
- [x] RSM/前缀/验收/泊松 GOF 诊断加深：因素 ID·中心·alpha + `append_diagnostics` ZH 前缀剥离重包 + 验收 OC/ANOM 不等 n·决策限 + 泊松 GOF + 设计生成失败 + 图「没有可显示的数据」后缀
- [x] ARIMA/Best ARIMA 固定诊断加深：AICc/近零 SSE、AR/MA 拟合失败、差分样本、混合阶边界、预测期/非有限、CSS–AICc 非 Minitab 诚实声明（exact；同 code 不同 body 按全文匹配）
- [x] 非参数嵌套表头加深：Wilcoxon/Sign/Runs/Cochran/Mood/Kruskal/Friedman 列头 + Steel-Dwass/Dunn 表题 + 游程序列图后缀（变量名保留）
- [x] 等价性 TOST 嵌套 chrome：样本/配对/参考·检验单元格、比值·z/t·CI 方法·界限·结论表头、等价上下限系列、比例差轴、界限参数 token、缺界诊断
- [x] SPC/能力参数表 chrome：规格·σ 来源·AD 判定·PPM、T²/CUSUM/Zone/Z-MR/I-MR 历史与阶段行（含 `阶段 %1（N / 均值 / σ̂_MR）` 模板）
- [x] Graph Builder 参数摘要 chrome：显示/分析 N·分面 caption 复用到 parameter_summary + 面板/分组/大小/数值/置信水平/类别数/标准化坐标/直线性诚实句 + Hexbin（分面）+ 累计比例轴
- [x] 经典推断参数摘要 chrome：假设均值·备择（小于/大于/不等于）·合并方差·试验/目标·假设比例·发生率·行/列·变换/检验/参考·交叉表无卡方注记 + 行/列/合计百分比表题
- [x] 分析图题后缀 chrome：`的正态概率图`/`的直方图`/`的个体值图`/`的散点图`/`运行图` + `与` 中缀、嫌疑点、截断/N（可选）/因子 1、处理·区组·缺失·预测期数·因子 A/B·移动极差长度·正态概率图相关系数、Fisher 2×2 水平模板、部分析因分辨度、单位数列后缀
- [x] 残渣 parameter_summary eq-token：比例/组数/迭代/标准化/中位数/总体中位数 M、Wheeler EMP 注、零件/操作员斜杠、平衡三因子注、误差·趋势模型、手工分箱、（信息）
- [x] SPC/MSA 组装 chrome：属性 `{chart}逐子组统计` 表题后缀、G/T `间隔` 表头、CUSUM 上侧/下侧/无、阶段列缺失诊断、MSA 按零件/操作者 Xbar/R 图题、回归残差正态性解释块
- [x] 可靠性 2 参数模型提示 + T 功效/样本量诊断 + G/T `{chart} 逐点统计` 表题后缀
- [x] 分析可见 chrome：CUSUM 图题、零件均值系列、DOE 交互轴 `（按 … 分组）`、删失工作表导出诊断、时间序列无效观测、未知特殊原因规则单元格
- [x] 域 SPC 策略诊断 + 分析规则目录名/说明：`special_cause_policy_*`/`test_not_applicable` 诊断码 + regression/anova/msa/reliability `rule.*.catalog.*` 全量 exact map
- [x] G/T 图 + Z-MR/MA + Graph Builder 域诊断：`control_charts.cpp` G/T/Z-MR/MA 门禁 + `graph_visuals.cpp` 区间/气泡/热图/时序/等值线/饼图验证串 exact map
- [x] 质量图/导入契约域诊断：`quality_visuals.cpp` 因果/变异性图 + Nelson estimate 应用 + `column_extract` 导入契约校验串
- [x] ADF/EDA/分面/ImportPlan/MSA Type1 域诊断：`adf_test` 默认滞后与临界值说明、`eda_plots` Silverman 带宽、`graph_assembly` 分面截断模板、`import_plan_validation` 校验串、Type1 稳定性规则证据
- [x] 时间序列/正态性/个体分布识别/DOE 因子设计域诊断：`time_series.cpp` 单/双指数平滑门禁、`normality_test.cpp` Ryan–Joiner 与 n 门槛、`distribution_identification.cpp` 非正值 AD 范围、`doe_factorial.cpp` 全因子校验与 hold/clamp 诊断
- [x] DOE 因子设计剩余域诊断：`doe_factorial.cpp` 设计生成/响应拟合/纯误差·Lenth PSE/等值线栅格/hold 忽略等静态串 exact map（生成器/部分析因/曲率/纯误差既有 interp.* 不变）
- [x] 可靠性 CIF/KM 区间/高斯混合域诊断：`aalen_johansen_cif` 动态计数模板、`km_interval` 全部门禁/诚实句 catalog 化、`gaussian_mixture_2` scope 与 k-BIC 动态模板
- [x] 非正态能力门禁域诊断：`process_capability.cpp` 稳定性/双峰/Hartigan dip/混合 BIC 动态模板 + Johnson/组间方差/CI df 静态串 catalog 化（合格判定保持关闭）
- [x] 删失/保修/分模式拟合域诊断：`censoring_contract.cpp` 工作表行模板、经典 KM 门禁、保修分层/池化 honesty、mode-fit 动态模板 catalog 化
- [x] Tukey 计算规则证据动态句：`inference_extensions` 输出的「Tukey 同时置信水平 = …；显著性由同时置信区间是否包含 0 决定。」经 `localize_known_plain_message` + 解释 bullet 双路径英译（规则表单元格 + `分析限制：` 前缀）
- [x] 导入契约诊断补全测试：`column_extract` 单元格状态行数 / 元数据原始行数 mismatch 纳入 `quality_visuals_import_contract_diag_localizes_to_en_us`
- [x] 应用层错误页回退句：`analysis_service` 的「无法计算 I-MR-R/S 控制图。」与「无法计算组间/组内能力指标。」经 `localize_known_plain_message` exact map + 测试
- [x] Graph Builder 分面页标题全量回归：`graph_service` 16 种 `*（分面）` / `Hexbin（分面）` 页题经 `localize_page_title` + `page.suffix.faceted` 英译测试
- [x] `param_true` 审计残渣回归：`scripts/_tmp_phase3_param_true.txt` 所列 5 类 producer 串（跳过 N* / 区组·缺失 param / Fisher 2×2 / 双样本 t 组 2 / SPC 未触发证据）经既有 wiring + `param_true_audit_leftovers_localize_to_en_us` 验证
- [x] 比例组错误前缀：`第一组：`/`第二组：` + 内层 `diag.prop.*` 经 `localize_known_plain_message` 前缀剥离重包（`proportion_group_error_prefix_localizes_to_en_us`）
- [x] 图表渲染空态双语：`chart_renderer` 19 处空图占位改走 `chart.no_displayable_data` + `ChartModel.language_tag`；PDF 导出传递 report locale
- [x] Graph/报告 `row_visibility_contract` + `row_visibility_overlap` 诊断 catalog 化 + en-US 本地化（`diag.row_visibility.*`；`pdf_graph_builder_faceted_bubble_and_time_series_*` PDF 断言；Linguist 2539 镜像同步）
- [ ] 其余动态拼接解释正文全量双语（其余推断数值句等仍可能混语；解释 bullet 模板主路径已基本覆盖；`tools/audit_interpretation_localization.py` 静态审计 **316/316**，含 gate 句 `bullet ==` 精确匹配 handler）
- [ ] 其余表题/图题/表头全量覆盖（Graph 分面页题已 16/16 测试；仍有命令页标题/薄残渣未入 catalog）

验收：

- [x] 同一 Facts 在 zh-CN/en-US 下数值、稳定 ID、行 ID 一致
- [x] catalog 覆盖率统计（zh/en 对已登记条目为 100%）
- [x] 缺少翻译会产生 missing 记录/诊断，不静默混语
- [ ] 英文长文本布局人工验收
- [x] 报告 locale 不被系统 locale 覆盖（存于 Profile）
- [x] 规则 ID 不翻译；展示标签可翻译
- [ ] 同一报告在中英两种语言生成不混语（固定句+部分动态模板已覆盖；代表性竖切 automated visible-layer guard 已加；其余动态句仍可能混语） → **持续加深**

禁止偷懒：

- 禁止只翻菜单，不翻 PDF；
- 禁止将中文字符串拼接后再翻译；
- 禁止用字符串内容判断算法类型；
- 禁止用机器当前 locale 覆盖用户保存的报告 locale。

### Phase 4：DOE 设计生成竖切

#### 4.1 DOE-1：统一设计输入与可行性检查 ✅

输入：

- 因素 ID、显示名、单位；
- 因素类型（连续/分类，第一期只接受连续）；
- 原始低/高水平；
- 目标设计类型；
- 中心点、重复数、阻塞、随机化 seed；
- 是否允许超范围星点。

输出：

- coded design matrix；
- uncoded design matrix；
- run order、standard order；
- center/star/factorial/block 标签；
- feasibility diagnostics；
- `DesignGenerationFacts`（`InterpretationFacts.design_generation`）。

验收：

- [x] 低高水平相同、非有限值、反向范围、重复因素 ID 均阻止；
- [x] seed 相同得到完全相同的运行顺序；
- [x] seed 不同只改变随机化，不改变设计点集合；
- [x] 每行保留稳定 run ID；
- [x] 设计矩阵可导出 JSON（OutputPage serialization）；CCD/BBD 与 2 水平全因子/部分析因/Plackett–Burman 另有 `worksheet_export` 一键写回（见 4.4）。

#### 4.2 DOE-2：CCD ✅

至少支持 CCC、CCI、CCF 三种明确类型，显示 alpha/星点范围和中心点策略。

验收：

- [x] 2 因素 CCF 点数/标签符合 NIST 合约（`doe_ccd_k2_factors.json`）；
- [x] CCC 超出原始范围时有明确 warning/block；
- [x] CCI/CCF 不把不可行设置静默截断；
- [x] 中心点数量可追溯；
- [x] 与 baseline 点集计数与 round-trip 比对（formula_reference，非 vendor_oracle）。
- [x] en-US PDF 设计页字节扫描（`pdf_doe_ccd_and_bbd_design_localize_to_en_us_without_chinese_leak` CCD 部分）。
- [x] audit JSON 设计生成门禁 + formula ref + audit PDF 证据附录（`audit_json_doe_design_generation_carries_label_text_ids` CCD 部分）。

#### 4.3 DOE-3：BBD ✅

第一期限制为连续因素、3 水平、满足最小因素数；输出缺失角点说明。

验收：

- [x] 2 因素不接受；
- [x] 3 因素点集、中心点和每行水平符合 BBD 结构；
- [x] 报告/解释明确 BBD 避开所有因素同时极端的性质；
- [x] 不把 BBD 宣称为所有因素域内预测质量最优。
- [x] en-US PDF 设计页 + BBD 边界诚实诊断字节扫描（同上测试 BBD 部分）。
- [x] audit JSON `bbd_no_corners` 门禁 + audit PDF 证据附录（`audit_json_doe_design_generation_carries_label_text_ids` BBD 部分）。

#### 4.4 DOE-4：RSM 接线 🟡

在 DOE 设计可导出和重放后，才接入 `rsm_response`：

- [x] 设计来源 ID（`design_source_id` / `RsmFacts`）；
- [x] 二次项/交互项矩阵（既有 `fit_rsm_analysis`）；
- [x] 系数、ANOVA、残差四图、等值线/静态曲面；
- [x] 设计边界编码与 CCD/BBD 一致（`code_rsm_factors_from_design_bounds`）；
- [x] 无中心点 warning；静态曲面明确标注非可旋转 3D；
- [x] worksheet 一键从设计矩阵落表（CCD/BBD → `worksheet_export` + UI 确认替换活动表）。
- [x] 2 水平全因子/部分析因/Plackett–Burman 设计矩阵 → `worksheet_export`（实际水平 + 空 Response；与 RSM 共用 UI 写回）。
- [x] factorial worksheet 回导识别 `PointType=center` / 实际中心水平，保留 `DoeRun::center_point`（曲率/纯误差可用）。
- [x] pinned `reference_implementation` → 项目 `golden`（CCD k2 CCF / BBD k3 标准序；非 vendor_oracle）。
- [x] RSM 纯误差 / 失拟 ANOVA 独立 reference 脚本（`scripts/rsm_lof_reference.py` + `rsm_lof_fixture.csv`；pure_error_df=2；`formula_reference`）。
- [x] en-US PDF + audit JSON RSM LOF 门禁证据（`pdf_rsm_lof_gate_localizes_and_audit_evidence_carries_label_text_id`）。
- [x] RSM LOF 三模板 PDF 可见性（`pdf_rsm_lof_cross_template_table_visibility_and_en_us_locale`）。
- [x] S1 跨页预筛样本 + 测试（`samples/phase0_baselines/reliability_km_long_table.csv` + `pdf_reliability_km_long_table_survival_table_spans_pages_en_us`；肉眼表头重复仍属 §5 S1 人工项）。
- [ ] 与 R `rsm` / 商业软件 LOF 数值对齐仍属 ⏸（无 pinned CRAN / vendor_oracle）。

验收：

- [x] 设计生成 → 填响应 → RSM 分析闭环（`design_to_rsm_closed_loop_with_design_bounds`）；
- [x] 结果不将静态曲面冒充可旋转 3D。

实现落点（4.1–4.3）：

- `src/domain/statistics/response_surface_design.*`
- `AnalysisService::doe_response_surface_design` + UI `doe_ccd` / `doe_bbd`
- `tests/response_surface_design_phase4_test.cpp`

### Phase 5：可靠性/保修竖切

#### 5.1 REL-1：删失数据契约 🟡

支持：

- [x] failure / right-censored（`censoring_contract`）；
- [x] left / interval 校验与经典 KM 阻止；
- [x] 时间单位冲突阻止；
- [x] 工作表 `censoring_type_column`（可与 event 二选一或交叉校验）+ 可选 failure_mode；
- [x] `ReliabilityFacts.failure_modes` / 失效模式表表面化；
- [x] 区间左右界列接线（缺失拒绝；有效界仍诚实阻挡经典 KM）。
- [x] 独立 `km_interval` Turnbull 简化网格 NPMLE（`formula_reference` / `turnbull_npmle_simplified_grid`；反序列化拒绝假 vendor_oracle）。
- [x] 暴露量列求和全链路（可靠性 Facts + 保修摘要；列优先于标量，非法拒绝）。
- [x] 分组/失效模式保修分母追溯（池化 R(T_w)；实测暴露量优先，否则比例分摊并诊断；source_rows 可追溯）。
- [x] 分模式可靠度拟合（cause-specific：竞争失效作右删失；`formula_reference`；保修分层可选分模式 R）。
- [x] Aalen–Johansen CIF（`formula_reference` / `aalen_johansen_cif`；≠ Fine-Gray 多协变量；反序列化拒绝假 vendor_oracle/fine_gray）。
- [x] Fine-Gray 二分类 group 子分布风险（`fine_gray_binary_ipcw`；IPCW 偏似然；`formula_reference`；≠ 多协变量/Cox/vendor_oracle）。
- [x] Fine-Gray 连续单协变量（`fine_gray_continuous_ipcw`；均值中心化；HR 每 +1；`covariate` 角色优先于二分类）。
- [x] Fine-Gray 多协变量 IPCW（`formula_reference` / `fine_gray_multi_ipcw`；均值中心化；p≤5；target≥5p；≠ cause-specific Cox / vendor_oracle）。
- [ ] pinned R `survival` / 商业 Turnbull / 分模式 vendor_oracle 未冻结。

验收：

- [x] 负时间、反向区间阻止；
- [x] 右删失不被当成失效；
- [x] 零失效/全删失 warning；
- [x] 导入/导出逐观测 censoring 状态（`censoring_observations_to/from_worksheet` + reliability `worksheet_export`；OutputPage JSON `worksheet_export` round-trip）。

#### 5.2 REL-2：KM + 右删失报告 🟡

- [x] 既有 KM 生存表/图/Facts；
- [x] 手算小样本 baseline 比对（`reliability_phase5_test`）；
- [x] 独立 `reference_implementation` 脚本（`scripts/reliability_km_reference.py` ↔ `reliability_km_handcalc.csv`；非 vendor_oracle）。
- [x] 解释不把 KM 外推为长期保证（既有 interpretation 限制）。
- [x] en-US PDF 字节扫描（`pdf_reliability_km_and_weibull_localize_to_en_us_without_chinese_leak` KM 部分；非 vendor_oracle 对齐）。

#### 5.3 REL-3：Weibull/Lognormal 参数族 🟡

- [x] 既有独立拟合路径；
- [x] phase5 测试分别断言 Weibull vs Lognormal（含 lognormal 时间尺度中位数）；
- [x] en-US PDF 字节扫描（同上测试 KM / Weibull / Lognormal 部分；`formula_reference` 保持）；
- [ ] pinned `reference_implementation` / vendor_oracle 未冻结。

#### 5.4 REL-4：保修摘要 🟡

- [x] `summarize_warranty` + `reliability_warranty` 命令 + `WarrantyFacts`；
- [x] claims/1000 = 1000*(1-R(Tw))；prediction/observation 标签；
- [x] 独立 `reference_implementation` 脚本（`scripts/reliability_warranty_reference.py`；非 vendor_oracle）；
- [x] 失效模式/分组分母追溯（池化 R；有分模式拟合时 failure_mode 层可用 cause-specific R）；
- [x] PDF 联调加深（en-US 本地化包 + manifest/audit 诚实状态；仍禁止假 PDF/A·UA）；`pdf_warranty_summary_page_*` + `pdf_warranty_summary_cross_template_table_visibility_and_en_us_locale`（scalar 三模板）+ `pdf_warranty_strata_tables_*` + `pdf_warranty_cross_template_table_visibility_and_en_us_locale` 字节扫描；全量人工跨页表验收仍开放；
- [ ] 多页保修 PDF 人工验收与真实 tagged-PDF 仍属另立。

- 禁止把统计预测写成法律/质量承诺；
- 禁止只输出“每千件”数字而不输出分母和时间窗口；
- 禁止在模型未收敛或支持域不足时生成成功预测；
- 若保修/暴露量定义在当前数据源中无法验证，保持 `⏸`，不得用默认分母补齐。

### Phase 6：非正态能力竖切

#### 6.1 CAP-NN-1：识别和安全报告 🟡

- [x] 缺规格限阻止（既有 + phase6 测试）；
- [x] 解释层对非正态/Johnson 禁止“过程合格”口径；
- [x] 稳定性前置契约：I-MR Rule-1 初筛 + 正态能力默认 `pass_fail_judgment_allowed=false`（清屏≠verified）；
- [x] 双峰直方图峰谷初筛（`apply_capability_bimodality_screen`）；suspected 降级假设；clear ≠ 单峰证明；非混合模型。
- [x] Hartigan dip 门禁筛查（`apply_capability_hartigan_dip_screen` / `hartigan_dip_1985`，Uniform 零假设 MC，`formula_reference`）；evidence_against 不开放合格判定；consistent ≠ 单峰证明。
- [x] 固定 k=2 高斯混合（`fit_gaussian_mixture_2` / `gaussian_mixture_2_em`）保留供单测；能力门禁改走搜索。
- [x] 混合 k=1..k_max 高斯 BIC 搜索（`fit_gaussian_mixture_search` / `apply_capability_mixture_screen`；`formula_reference` / `gaussian_mixture_k_bic`；default k_max=4 clamp 2..5；preferred_2comp|preferred_kcomp|not_preferred；≠ 非高斯 / vendor_oracle；不开合格判定）。
- [ ] 非高斯混合 / 商业对齐仍属 ⏸。
- [ ] Hartigan 与商业临界表 / pinned reference_implementation 对齐仍属 ⏸。

#### 6.2 CAP-NN-2：Box-Cox 🟡

- [x] λ=0/1、负值/零值拒绝；
- [x] 规格限变换与序关系检查（`box_cox_transform_limit` / `box_cox_limits_order_ok`）；
- [x] 应用层规格限门禁（`AnalysisService::box_cox` → `box_cox_invalid_spec_limit` / `box_cox_spec_limits_order`；`quality_statistics_test` 无效/倒置限跳过能力表）；
- [x] 规格限门禁诊断 en-US 本地化（`diag.box_cox_invalid_spec_limit_*` / `diag.box_cox_spec_limits_order`；`report_locale_phase3_test::box_cox_spec_limit_diag_localizes_to_en_us`）；
- [x] 无效规格限 en-US PDF 字节扫描（`pdf_box_cox_invalid_spec_limit_localizes_to_en_us_without_chinese_leak`；13 竖切 guard 含 invalid-spec 诊断）；
- [x] 规格限门禁解释 bullet（`InterpretationService` + `interp.box_cox_spec_limit_gate`；`interpretation_service_test` / locale+PDF 回归）；
- [x] 无效规格限三模板 PDF 字节扫描（`pdf_box_cox_invalid_spec_limit_cross_template_table_visibility_and_en_us_locale`；`representative_box_cox_three_report_profiles_*` 含 invalid/inverted-spec 段）；
- [x] S6 路径 C 人工样例（[`samples/capability/box_cox_spec_gate_s6.csv`](../../samples/capability/box_cox_spec_gate_s6.csv) + [`box_cox_spec_gate_manual_s6.md`](../../samples/capability/box_cox_spec_gate_manual_s6.md)）；
- [x] EvidenceBundle 门禁（`:gate:box_cox_not_pass_fail` / `:gate:box_cox_spec_limit`；`pdf_box_cox_cross_template_*` + invalid-spec cross-template PDF + `pdf_box_cox_spec_limit_gate_localizes_and_audit_evidence_carries_label_text_id` + `report_profile_phase1_test` customer 截断保留）；
- [x] 不把 Box-Cox 写成合格判定（既有 interpretation）；
- [x] en-US PDF Box-Cox 诚实句 + 表题字节扫描（`pdf_box_cox_honesty_localizes_to_en_us_without_chinese_leak`；`assumption_status=not_verified` 保持）。
- [x] Box-Cox λ=0/1 独立 reference 脚本（`scripts/box_cox_reference.py`；`reference_implementation`）。
- [x] 三模板 PDF 可见性（`pdf_box_cox_cross_template_table_visibility_and_en_us_locale`）。

#### 6.3 CAP-NN-3：Johnson/自动方法 🟡（条件开放 / 门禁）

- [x] 拟合/正反变换既有；
- [x] `CapabilityFacts.gate_status=gated_research`，`pass_fail_judgment_allowed=false`；
- [x] 研究/预览诊断 + 解释门禁；
- [x] en-US PDF + audit JSON Johnson 门禁证据（`pdf_johnson_capability_gate_localizes_and_audit_evidence_carries_label_text_id`；`pass_fail_judgment_allowed` 保持 false）；
- [x] Johnson 规格限定义域外门禁（跳过 Overall 表 + `:gate:johnson_spec_limit` + 三模板 guard/PDF + customer plot-flood 保留）；
- [ ] golden/尾部/人工解释验收仍未满足 → 保持不得开放合格判定。

### Phase 7：受控 Graph Builder/EDA 编排器 🟡

交付顺序：

1. [x] PlotSpec/role contract（既有）；
2. [x] finite geom registry（既有命令集）；
3. [x] **hidden≠excluded** 过滤契约（全主 geom 双口径 + UI 隐藏/排除分轨）；
4. [x] **预览与属性侧栏**（图形属性面板只读可见性契约横幅 + page_renderer 注入排除/隐藏/N）；
5. [x] JSON 保存/重放（hidden_rows/excluded_rows）；
6. [x] 图表报告嵌入加深（`max_plots` 分模板、plot EvidenceRef、排除/隐藏字幕；非自由拼版）；
7. [x] source row 联动加深（`member_source_rows` + resolver；interval/bar/violin；非全 geom 终态）。
8. [x] pie / category-heatmap / hexbin 成员行扩展（相关矩阵热图不伪造 per-cell 观测层；hexbin 用矩形格边与 ContourCell 索引对齐）。
9. [x] **受控 scatter 分面**（`facet_column` + `facet_max_panels` 1..12；截断诊断；≠ by 图内着色；非自由拼版）。
10. [x] **density 非离散 mark**（清空曲线点 `source_rows`/`member_source_rows` + `density_curve_not_discrete_marks`）+ **bar/density 受控分面**。
11. [x] **interval / violin / hexbin 受控分面**（同 `facet_column`/`facet_max_panels`；hidden 不进显示面板成员行）。
12. [x] **contour 受控分面**（规则网格按分面水平切片；截断诊断同契约）。
13. [x] **matrix / parallel / time_series / area 受控分面**（矩阵切片；area 继承 time_series 分面后改 kind）。
14. [x] **bubble / probability / ecdf / marginal 受控分面**。
15. [x] **correlation / heatmap 受控分面**（相关矩阵不伪造 per-cell 观测层；类别热图保留 member 行）。

验收：

- [x] `hidden` 只影响显示，`excluded` 影响分析输入；可区分；
- [x] 至少 scatter + 聚合 bar + density + 缺失/排除场景测试；
- [x] 分面页题 en-US PDF 字节扫描（16/16 分面页题 + 类别热图第二路径 + 饼图非分面；见各 `pdf_graph_builder_faceted_*` / `pdf_graph_builder_pie_*` 测试；补充 locale 16/16 标题回归）；
- [ ] 自由像素拼版/可旋转 3D **不做**（保持禁止项）。

## 5. Agent 团队编排

### 5.1 角色

| Agent | 责任 | 禁止范围 | 输出 |
|---|---|---|---|
| Research Agent | 一手来源、公式、限制、引用 | 不改 C++ | research md 增量 |
| Domain Agent | 纯算法和 Facts | 不改 UI/CMake | domain + unit tests |
| Application Agent | service、解释、诊断、证据 | 不直接操作 Qt SQL/UI | application + tests |
| Serialization Agent | JSON schema、version、round-trip | 不重算算法 | serialization tests |
| UI Agent | 分页、多页面、模板选择、交互 | 不在 UI 写统计公式 | dialog/page tests |
| Reporting Agent | profile、PDF、manifest | 不改变 Facts | reporting tests/manual checklist |
| i18n Agent | TS/QM、locale、文本覆盖 | 不翻译 domain 数值 | translation tests |
| Comparison Agent | 独立参考脚本、golden、差异报告 | 不修改被测实现 | comparison artifacts |
| Review Agent | 需求、分层、回归、偷懒审计 | 不自行扩大范围 | PASS/FAIL review |

### 5.2 并行规则

- Research、现状审计、测试数据准备可以并行；
- Domain 完成契约后，Serialization 和 Comparison 可并行；
- UI/Reporting 必须等待 Facts 和 evidence schema 稳定；
- i18n 可早期审计，但最终翻译覆盖要等待字段 ID 冻结；
- 每个实现轮次结束必须再启动新的 review agent；
- agent 不得修改其他 agent 未完成的文件，除非 goal 明确授权并记录冲突解决。

### 5.3 每轮 agent 回报格式

```text
本轮目标：
实际修改：
未修改但被要求的内容：
实现证据：
测试证据：
公式/来源引用：
人工验收项：
已知风险：
状态：✅ / 🟡 / ❌ / ⏸
下一轮前置条件：
```

## 6. 比对与测试策略

### 6.1 测试层级

1. **Domain 单测**：边界、公式、错误、确定性、空输入。
2. **Facts/serialization**：字段完整性、version、round-trip、未知字段兼容。
3. **Application 集成测试**：DataTable 到 OutputPage、解释和证据。
4. **Report tests**：不同 profile、语言、空页、分页和 manifest 一致性。
5. **UI/dialog tests**：参数验证、多页面导航、取消、错误回退。
6. **人工 Qt Creator 验收**：中文路径启动、图标、字体、滚动、PDF、中文/英文布局。
7. **参考比对**：独立 Python/R/手算/官方例题；必须注明工具版本、公式、容差和是否 golden。

### 6.2 每项算法必须具备的 case matrix

- 正常最小样本；
- 空数据；
- 单行/重复值；
- 缺失值；
- 非有限值；
- 边界参数；
- 极端数值；
- 诊断阻止；
- 诊断警告但可继续；
- 序列化后重放；
- 中英文输出；
- 报告三模板输出。

### 6.3 比对纪律

- 先比数据结构，再比格式化文本；
- 先比稳定 ID、N、行 ID、参数，再比 display string；
- 数值容差必须写明绝对/相对容差和来源；
- 商业软件未导出的值不能伪造 golden；
- 参考实现不同的随机化顺序不等于设计点不同；
- PDF 像素快照只能作为辅助，不能替代 Facts 比对；
- 所有差异必须归类：实现错误、定义差异、随机化差异、格式差异、未校准。

## 7. 完整验收门槛

### 7.1 竖切完成定义

一项能力只有同时满足以下条件才能标记 `✅`：

- domain 算法可独立测试；
- Facts 包含所有影响解读的参数、样本量、缺失/删失/过滤摘要；
- application 有可读解释和分级诊断；
- JSON 可保存、加载和版本迁移；
- UI 有独立、可取消、可恢复的参数页面；
- 报告至少有工程师版输出；
- help 有公式、假设、限制和来源；
- 有正常、边界、错误和回归测试；
- 有独立 review agent PASS；
- Qt Creator 构建/运行由用户确认，或明确记录未完成；
- backlog、acceptance matrix、research 文档已同步。

### 7.2 报告产品化完成定义

- 三种模板由同一个结构化事实源驱动；
- PDF 与 JSON/manifest 可互相定位；
- 证据链能够定位到数据快照、列、过滤和源行；
- 中英双语覆盖报告所有用户可见文本；
- 语言不改变统计数值和稳定 ID；
- PDF 可读性测试通过；
- PDF/A/PDF/UA 若未验证，明确显示未验证；
- 导出失败可恢复且不留下成功假象；
- 审计版在无原始数据库连接时仍能解释快照来源。

## 8. 回滚、暂停与状态管理

任何阶段出现以下情况，状态必须回到 `🟡` 或 `⏸`：

- 统计定义仍有多个合理解释；
- 参考资料不足；
- golden 与实现差异未解释；
- PDF 需要引入新库但许可证/部署未确认；
- 设计超出可行域而 UI 没有阻止；
- i18n 文本 ID 尚未稳定；
- 大数据内存或耗时没有上界；
- agent 只能通过放宽测试断言才能通过。

回滚原则：

- 保留已通过的 Facts/serialization 契约；
- 回滚 UI/报告展示不应删除历史 JSON；
- 新 schema 使用 version/migration；
- 不删除失败测试，除非证明测试假设错误并在文档中记录。

## 9. 推荐的第一批 Goal

建议第一轮不要同时实现所有方向，而是选择一个能验证基础设施的组合：

1. **报告证据链 + 三模板 profile**；
2. **中英双语报告最小闭环**；
3. **DOE CCD/BBD 设计生成**；
4. **可靠性删失契约 + KM 报告**。

原因：报告产品化能立即复用现有算法，DOE 能验证新 `DesignFacts` 和 worksheet 接线，可靠性则能验证删失/诊断/审计契约。非正态能力和 Graph Builder 建议在这些事实模型稳定后推进。

## 10. 维护清单

每个 goal 完成后必须：

- 更新 `docs/research/comprehensive-analytics-roadmap.md` 的对应状态；
- 更新 `docs/research/minitab-market-algorithm-backlog.md`；
- 更新 `docs/quality-algorithms-acceptance.md`；
- 更新 `docs/algorithm-chart-gap-matrix.md`（如涉及图表/算法接线）；
- 更新帮助资源和翻译资源；
- 添加/更新 `VALIDATION_MATRIX.md` 或对应验证表；
- 更新 [`reference-implementation-index.md`](reference-implementation-index.md)（新增/变更 Python 对照脚本时）；
- 写明实际 Qt Creator 构建与人工测试结果；
- 由新 review agent 输出 PASS/FAIL；
- 不自动 commit/push，除非用户明确要求；若用户要求上传，先确认 remote 和变更范围。

## 11. 本次调研来源索引

算法方向的专门研究已拆分到：
[`algorithm-widening-ccd-reliability-johnson-eda.md`](algorithm-widening-ccd-reliability-johnson-eda.md)。
本文件负责把该研究转换成跨层实现、Goal 编排和报告交付验收；算法专门文档负责
保留更细的公式契约、来源等级、golden 风险和算法边界。

| 来源 | 用途 |
|---|---|
| NIST Response Surface Designs | 二次模型、CCD/BBD 选择边界 |
| NIST Central Composite Designs | 骨架、中心点、轴点、CCC/CCI/CCF |
| NIST Response Surface Comparisons | 可行域、预测质量、BBD 缺失角点 |
| NIST Glossary | CCD/BBD/RSM 定义交叉核对 |
| Qt QPdfWriter | PDF 绘制、元数据、版本和附件能力 |
| Qt QPdfDocument | PDF 读取、文本抽取和人工验收辅助 |
| PDF Association PDF/A + PDF/UA guide | 归档、无障碍、XMP 和“不宣称合规”边界 |
| PDF Association ISO 19005 / ISO 14289 | PDF/A、PDF/UA 的标准范围 |
| Qt Internationalization / QTranslator | `.ts/.qm`、翻译加载和可翻译源码 |
| Unicode UTS #35 LDML | locale、数字、日期、复数和显示名 |
| 项目现有 roadmap/backlog/research | DataLab 当前实现状态和不做清单 |

访问日期统一为 2026-08-21；后续执行 agent 若发现来源版本更新，必须保留原引用并在文首更新访问日期与版本。
