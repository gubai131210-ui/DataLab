# Phase 2：PDF/A 与 PDF/UA 能力评估（诚实声明）

> 日期：2026-08-21  
> 状态：评估文档 + `assess_pdf_export_pipeline()` + 可选 `DATALAB_VERAPDF` 钩子 + manifest blockers；**未默认宣称合规**  
> 相关类型：`PdfComplianceStatus` / `PdfComplianceAssessment` / `ExternalPdfaValidatorResult` / `ReportExportManifest`

## 1. 结论（先读）

| 标准 | DataLab 当前状态 | 允许对外声明 |
|---|---|---|
| PDF/A（ISO 19005） | 默认 `not_validated`；仅当 `DATALAB_VERAPDF` 实际调用且 exit=0 时记 `validated_pass` | 无工具时只能写 not_validated；有工具时写工具名+版本+结果 |
| PDF/UA（ISO 14289） | 恒为 `unsupported`（QPainter 路径无结构树） | 不得写“合规”；**veraPDF 绿灯也不改 UA** |

**禁止：** 把“PDF 能打开”“写了标题/作者 metadata”“导出成功”写成 PDF/A 或 PDF/UA 合规。

## 2. 运行时评估器

`assess_pdf_export_pipeline()`（application）给出 QPainter 管线基线。

`run_optional_verapdf(pdf)`（infrastructure）读取环境变量 `DATALAB_VERAPDF`：

- 未设置 → `tool_configured=false`，PDF/A 保持 `not_validated`
- 路径不存在 → `not_validated` + blocker
- 成功启动 → `merge_external_pdfa_validator_result`：exit 0 → PDF/A `validated_pass`（必须带 `validator_name`）；非 0 → `validated_fail`
- **PDF/UA 始终 `unsupported`**

`run_optional_pac(pdf)`（infrastructure）读取 `DATALAB_PAC`：

- 未设置 → 仅记录 notes；UA 仍 `unsupported`
- 即便 exit_code=0 → `merge_optional_pac_result` **禁止**把 UA 升为 `validated_pass`（QPainter 无 tagged-PDF）
- 可在 summary/blockers 记录 PAC 版本与 exit，供审计旁路

导出 UI 在写 temp PDF 后、写 manifest 前依次合并 veraPDF 与 PAC。离线脚本：`tools/run_verapdf_assessment.py`。

**本地探针（不宣称 PDF/UA；PDF/A 仅反映 veraPDF exit code）：**

```powershell
$env:DATALAB_VERAPDF = "C:\path\to\verapdf.bat"   # 未设置则保持 not_validated
$env:DATALAB_PAC = ""                              # 即便 exit 0 也不升 PDF/UA
python tools/run_verapdf_assessment.py path\to\report.pdf
# 生成 report.pdf.verapdf.json；manifest 仍须通过 export_report_package 合并
```

`manifest_matches_document` 仍拒绝：

- `validated_pass` 且 `validator_name` 为空
- QPainter 管线上的 `pdfua_status=validated_pass`

## 3. 当前实现能力（基于 Qt `QPdfWriter`）

已具备：可读 PDF、metadata、页眉页脚诚实状态、旁路 `.manifest.json` / `.audit.json`、一致性校验、可选 veraPDF 钩子、en-US 本地化后原子导出包联调（locale + blockers + audit 证据结构）。

不具备：默认内嵌 PDF/A、tagged PDF、PAC/PDF/UA、字体/色域符合性证明。

## 4. Phase 2 验收边界

- ✅ PDF 与 manifest 的 report/template/locale/facts_hash 一致
- ✅ 失败清理临时文件
- ✅ 默认 manifest：PDF/A=`not_validated`、PDF/UA=`unsupported` + blockers
- ✅ 可选 veraPDF 钩子 + 合并规则测试（未配置时不伪造 pass）
- ⏸ 真实 tagged-PDF / PAC 结构树流水线（非 QPainter）→ 另立竖切；当前 `DATALAB_PAC` 仅为诚实探针
- ⏸ 把 QPainter 产物当作 PDF/A 产品目标 → **不做**（除非换导出引擎）
