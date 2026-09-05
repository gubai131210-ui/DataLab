# DataLab 下一步方向调研（2026-09-05）

> **用途**：产品完善方向分析（非施工 Goal）。  
> **现网探索**：仓库模块 / `docs/research` / 算法 backlog。  
> **对外对照**：同类质量统计与 SPC 软件公开材料（见 §4 Primary URL）。  
> **并排摘要**：Cursor Canvas `datalab-next-direction-analysis.canvas.tsx`

---

## 1. 一句话定位（现网）

面向汽车制造质量工程师的 **Windows Qt/C++ 桌面质量分析台**：导入表 → SPC/MSA/能力/推断/DOE/可靠性等竖切分析 → PDF 报表；对照 Minitab 市场清单做主路径覆盖。产品自认 **不做** Predictive Analytics 全模块、Assistant、Graph Builder 拖拽全量、嵌 Python/R、可旋转 3D（见 `minitab-market-algorithm-backlog.md` 文首）。

README 仍偏早期 MVP，已落后代码。

---

## 2. 现网水位（摘要）

| 面 | 水位 | 依据 |
|----|------|------|
| 算法竖切 | **强** | ~180+ 命令；backlog §12 P0–P2 主项已 ✅/⚪ |
| 分层 / 帮助公式 | **强** | ui→application→domain；`algorithm_help` |
| 学习中心 | **中→强** | 教学/深度/口吻多轮 Goal 已收口（184 课） |
| 许可 / MES | **中** | 门禁接线；MES 旁路 `Mes_For_Q` |
| 图形交互 / 报表产品化 | **中** | 行联动主路径；注释/拼版/Word 导出仍弱（下一 Goal：G-Chart+Teach） |
| Minitab golden | **中** | G-Trust（`2118ad2`）已冻 10 命令 `golden`←`reference_implementation`；真·vendor 仍待导出 |

**总判**：命令面已接近「可卖的 Minitab 质量子集」；下一杠杆在 **可信度、工作流、报告、轻闭环**，不是再堆偏门命令。

---

## 3. 市场对照（原则，非广告）

| 竞品位 | 他们卖什么 | 对 DataLab |
|--------|------------|------------|
| **Minitab** | 质量课堂标准路径；Assistant；项目文件记忆对话框；Word/PPT 导出；报告卡 | 学「可重复 + 可读报告」，不必克隆 Assistant |
| **JMP** | 交互探索、链接图、DOE profiler、脚本 | 拖拽全量图构建器已延后；可窄化加深联动 |
| **云 SPC**（InfinityQS / NEXSPC / 国产 B/S 等） | 实时监控、API、批量 Cpk、MES 回写 | 分析台 ≠ 监视器；可做「导入→分析→结论摘要」 |
| **国产 QM 宣传位** | 信创、PPAP 模板、批量报表 | 汽车品管卖点是审核可读报告 |

---

## 4. Primary URL（访问日期 2026-09-05 UTC+8）

| # | URL | 用途 |
|---|-----|------|
| 1 | https://www.selecthub.com/statistical-analysis-software/minitab-vs-jmp/ | Minitab vs JMP 能力侧重 |
| 2 | https://www.minitab.com/en-us/products/minitab/assistant/ | Assistant / 报告卡 / 导出叙事 |
| 3 | https://support.minitab.com/en-us/minitab/help-and-how-to/data-input-and-output/save-and-export-data-and-output/save-and-export-minitab-projects-and-worksheets/ | 项目文件可重复性 |
| 4 | https://gitnux.org/best/statistical-process-control-software/ | SPC 软件谱系（桌面分析 vs 企业监控） |
| 5 | https://ellistat.com/en/minitab-jmp-and-ellistat-statistical-software/ | 质量场景选型叙事 |
| 6 | https://nexspc.com/zh-CN/tools | 云端「复制即分析 / 批量 Cpk」对照 |
| 7 | 仓库内 `docs/research/minitab-market-algorithm-backlog.md` | 产品边界与已完成队列（权威） |
| 8 | 仓库内 `docs/research/deferred-capability-agreement.md` | golden / 刻意延后 |

---

## 5. 建议轨道（杠杆序）

### Track A — 可信度与可卖性（优先）

- 高频 8～15 命令 Minitab golden（能力六合一、Gage R&R、I-MR/Xbar-R、两样本 t、正态能力…）
- 对齐 `VALIDATION_MATRIX` / 公式参考
- 修好 LicenseAdmin moc，完整 `package_dist`
- 刷新 README 与 backlog 状态行

### Track B — 分析工作流产品化（优先）

- `.dlab` 记住同命令对话框参数（换表重跑）
- 交客户 PDF 模板：能力 / MSA / 失控调查
- 分析历史；可选更好的多页图文 PDF（不必先追求 Word 全兼容）

### Track C — 图形交互窄化

- 注释、参考线、双图拼版（roadmap 已列延后，可开窄 Wave）
- **不做** Graph Builder 拖拽全量

### Track D — 现场轻闭环（差异化）

- MES 字段映射模板 → 一键 I-MR / 能力
- 多列能力或控制图汇总（桌面版「切片 Cpk」）
- 结论摘要可复制回工单（不做云告警中心）

### Track E — 教学继续变现

- 岗位路径（进料 / 制程 / PPAP）+「跟做报告」端到端
- 口吻/套话防回归（verify 已有门）

### Track F — 算法深化（择优）

- 仅客户点名或对齐驱动（如稳定性研究、部分 deferred 对齐）
- **继续不做**：PA 全模块、Assistant 大树、Graph Builder 全量、Python/R、3D、产线实时云 SPC

---

## 6. 下一 Goal 候选（择一开工）

| ID | 名称 | 验收抓手 |
|----|------|----------|
| G-Trust | 高频命令 golden | **已完成** `2118ad2`；手册 [`goal-g-trust-minitab-golden-plan-and-mega-prompt.md`](goal-g-trust-minitab-golden-plan-and-mega-prompt.md) |
| **G-Chart+Teach** | Track C 图形窄化 + Track E 岗位路径 | **下一场**；权威手册：[`goal-g-chart-edit-and-teach-path-plan-and-mega-prompt.md`](goal-g-chart-edit-and-teach-path-plan-and-mega-prompt.md)（注释+自定义参考线+双图拼版 PDF；进料/制程/PPAP 跟做路径） |
| G-Workflow | 工程记忆 + 报告模板 | 换表重跑同设置；PDF 可给审核 |
| G-MES-Lite | 映射 + 多列汇总 | 旁路导入到一张汇总表 |

---

## 7. 边界

- 本文件 **未改** 产品代码。  
- 不把国产竞品宣传数字当事实；只取「报告/集成/信创叙事」方向。  
- 若开 Goal，须另写权威手册并锁定 §0，禁止在分析文里直接改口产品边界。
