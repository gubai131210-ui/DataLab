# P4：Reliability 竞争风险 / CIF 深化

> 研究日期：2026-08-22 · 访问 2026-08-22（UTC+8）  
> Wave-4 W4-2；复用 Phase-5 `censoring_contract` / Aalen–Johansen / Fine-Gray IPCW 骨架；`formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `reliability` **CIF 深化** | Aalen–Johansen **CIF 曲线** + **分失效模式 CIF 表** +（可选窄化）**Gray 检验**；扩展 `ReliabilityFacts.cif_*`；Fine-Gray 路径保持 IPCW 诚实披露 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/supporting-topics/basics/reliability-analyses-in-minitab/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/reliability/how-to/nonparametric-distribution-analysis-right-censoring/interpret-the-results/all-statistics-and-graphs/survival-plots/ | 2026-08-22 |
| Gray (1988) / 竞争风险 CIF 组间比较文献（formula_reference；非 Minitab 菜单项） | 2026-08-22 |

**诚实披露：** Minitab 可靠性菜单 **无原生 Fine-Gray**、**无 Gray 检验菜单克隆**；DataLab Fine-Gray 为既有 `fine_gray_*_ipcw`（formula_reference），CIF 为 Aalen–Johansen。

## Minitab 表形参考（非 golden）

### CIF 曲线（图）

- 每个失效模式一条 **Cumulative Incidence Function** 阶梯曲线（竞争风险下 ≠ 1−KM）。
- 可选 warranty 竖线 \(T_w\) 标注各模式 CIF(\(T_w\))。
- 轴：Time · CIF（0–1）。

### 分模式 CIF 表

| 列 | 含义 |
|---|---|
| Failure Mode / Cause | 失效模式标签 |
| N (failures) | 该模式 exact 失效数 |
| CIF at last event | 末次事件时刻累计发生率 |
| CIF at warranty | 保修时刻 CIF（`warranty_time>0` 时） |
| Point count | 曲线台阶点数 |

### CIF 时间点明细（可选窄化表）

| 列 | 含义 |
|---|---|
| Time | 事件时间 |
| Mode | 失效模式 |
| CIF | 该模式累计发生率 |
| S(t) | 总体生存（任一标注失效作事件） |
| At risk | 风险集大小 |
| Cause failures | 该模式本时刻失效数 |

### Gray 检验（可选 · 窄化）

| 列 | 含义 |
|---|---|
| Chi-Square | 组间 CIF 整体检验统计量 |
| DF | 自由度（(K−1)×(J−1) 或窄化实现文档化值） |
| P | 渐近 P 值 |

**窄化条件：** 仅当存在 **group 列** + **≥2 失效模式** + **每组至少 1 次标注失效**；否则跳过并写 `gray_not_computed_reason`。

## DataLab 交付范围

- **复用**：
  - `censoring_contract`：`parse_reliability_event`、exact+right 路径、`source_row`。
  - `aalen_johansen_cif`（`algorithm_id=aalen_johansen_cif`）。
  - `ReliabilityFacts.cif_algorithm_id` / `cif_modes` / `cif_evidence_type`。
  - `fine_gray_*` 既有 IPCW 路径 **不改为 Minitab 对齐**；help/catalog 保留 `:gate:fine_gray_formula_reference_only`。
- **本 Wave 补全**：
  - **CIF 曲线** 输出到 GraphService（多模式 overlay）。
  - **CIF 表形** 与 Minitab 语义对齐（模式摘要 + 可选时间点表）。
  - **Gray 检验**（若窄化可行）：Facts 字段 `gray_chi_square` / `gray_df` / `gray_p_value` / `gray_group_count`。
  - **Interpretation**：CIF ≠ Fine-Gray 回归；Log-rank 不用于竞争风险 CIF 组间比较。
  - **algorithm_help.json**：Aalen–Johansen 公式 + Primary URL + Fine-Gray IPCW 窄化声明。

## 公式（# source: formula_reference）

**Aalen–Johansen CIF（模式 k）：**

在每个 distinct 事件时间 \(t_j\)，令 \(d_j\) 为所有模式失效总数，\(d_{kj}\) 为模式 k 失效数，\(Y_j\) 为风险集大小，\(S(t)\) 为「任一标注失效」下的总体 KM 生存：

\[
\widehat{CIF}_k(t) = \sum_{j:\, t_j \le t} \frac{d_{kj}}{Y_j}\, S(t_j^-)
\]

**Gray 检验（窄化 · 组间 CIF 比较）：**

在各事件时刻对模式特异性计数构造加权卡方统计量（Gray 1988 思路）；实现须在 help 中注明 **formula_reference** 与 Minitab 无对应菜单项。

**Fine-Gray（既有 · 非本 Wave 重写）：**

子分布风险 IPCW 偏似然；`algorithm_id` ∈ {`fine_gray_binary_ipcw`, `fine_gray_continuous_ipcw`, `fine_gray_multi_ipcw`}。**不是** Minitab Fine-Gray 克隆、**不是** cause-specific Cox、**不是** vendor_oracle / pinned R `survival::finegray`。

## 测试要求

| 层级 | 要求 |
|---|---|
| **Domain** | `# source: formula_reference`：两模式竞争风险小样本 CIF 手算；无 `failure_mode` 标签时跳过 CIF |
| **Censoring** | 未知事件编码拒绝；left/interval 省略 CIF 并 diagnostic |
| **Service** | complete-case + **真·A→B**；换表后 CIF 点数 / 模式数变化 |
| **Serialize** | `cif_*`、Gray 字段（若有）round-trip |
| **Honesty** | interpretation 含 CIF≠Fine-Gray；Fine-Gray 含 formula_reference 门禁句 |
| **Regression** | 不破坏 Phase-5 `reliability_phase5_test` / Wave-3 KM Log-rank |

## 明确不做（延后）

- Minitab 原生 Fine-Gray / Gray 菜单数值对齐或 golden。
- Cause-specific Cox 多协变量竞争风险回归（与 `cox_regression` 分流）。
- 左/区间删失 CIF（走 `km_interval`）；计数过程 / 时依协变量 Fine-Gray。
- Log-rank 替代 Gray 作竞争风险组间检验。
- 可旋转 3D CIF 图；Weibayes 竞争风险扩展。
