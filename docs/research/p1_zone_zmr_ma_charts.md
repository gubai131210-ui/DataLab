# P1 Zone / Z-MR / Moving Average 控制图

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。

## 0. 锁定与禁止偷懒

**做：** 命令 `zone_chart`、`z_mr`、`moving_average_chart`（或 `ma_chart`），各有 domain→Facts→service→commands→解释→序列化/测试→help→backlog。  
**禁止偷懒：** 禁止只改菜单不接计算；禁止破坏现有 I-MR / EWMA / CUSUM；禁止解释「过程已失控 / 已证明稳定」；禁止把 Zone 伪装成完整 8 Western Electric 规则替代品而不写诊断。

---

## 1. Zone chart（区域图）

| 来源 | URL | 访问 |
|---|---|---|
| Minitab Zone | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/zone/before-you-start/overview/ | 2026-08-21 |
| NIST / Jaehn zone scoring | 教材口径 | 2026-08-21 |

**产品锁定（本轮）：**
- 输入：单列个体值（与 I 图相同）。
- 估计：CL = 均值；σ 用平均移动极差 /d2（与 I 图一致，复用现有 σ 估计）。
- 区域：相对 CL 的 ±1σ、±2σ、±3σ 带（四区：0–1、1–2、2–3、>3）。
- 计分：单侧累积（Jaehn）：同侧连续点，进入 1σ 区记 1、2σ 区记 2、3σ 区记 4；换侧或落在中心带内重置；累计 ≥8 报警。
- 输出：区域得分时序图 + CL/控制限 + 得分表；Facts：`ZoneChartFacts`。
- **不做：** 自定义权重表 UI、阶段多 σ 重估（可诊断引导）。

---

## 2. Z-MR chart

| 来源 | Minitab Z-MR overview | 2026-08-21 |

短流程 / 多产品共用历史 σ：
- 输入：测量值 + 可选分组（产品/批次标签）；每组可用历史均值 μ_i 与历史 σ_i，**本轮简化**：若无历史参数，则用各组自身均值与合并/组内 MR 估计；标准化 \(Z=(x-\mu)/\sigma\)。
- 图 1：Z 个体图（CL=0，LCL/UCL=±3）。
- 图 2：|Z| 的移动极差 MR 图（标准 MR 限）。
- 命令 `z_mr`；Facts：`ZmrFacts`。
- **不做：** 完整「历史参数表」导入 UI（可用诊断说明未提供历史时用样本估计）。

---

## 3. Moving average (MA) chart

| 来源 | Minitab MA | NIST PMC | 2026-08-21 |

- 输入：单列；窗宽 w（默认 2～5，产品默认 3）。
- \(MA_t = (x_{t-w+1}+\cdots+x_t)/w\)（t&lt;w 时用可用长度或从 t=w 起画，锁定：**从第 w 点起画**）。
- CL = 总体均值；UCL/LCL = CL ± 3 σ/√w'，其中 w'=min(w,t) 或固定 w（锁定**固定 w，仅完整窗**）。
- σ：平均 MR/d2（与 I 图）。
- 命令 `moving_average`；Facts：`MovingAverageChartFacts`。
- **不做：** 加权 MA 与 EWMA 混淆；EWMA 已有命令保持独立。

---

## 4. 测试（formula_reference）

- Zone：全部点在 ±1σ 内 → 无 ≥8 得分报警。
- Z-MR：常数列 → Z≈0。
- MA：常数列 → MA=常数且不越限。

## G. 已交付窄化（2026-08-21）

- `zone_chart`：Jaehn 1/2/4，阈值 8  
- `z_mr`：可选分组；无历史 μ/σ 时样本估计  
- `moving_average`：完整窗；`ma_window` 默认 3  
