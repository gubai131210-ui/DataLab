# P3：ADF 单位根检验

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。与已有 `acf_pacf` 配用；不做 CCF。

## 0. 本轮锁定与禁止偷懒

**做：**

| 命令 | 交付 |
|---|---|
| `adf_test` | Augmented Dickey–Fuller；漂移/趋势可选；滞后；τ 与临界值；`AdfFacts` |

**禁止偷懒：**

- 禁止只给 p 值无回归规格说明  
- 禁止菜单占位  
- 禁止复活 Best ARIMA 自动选模 / Kalman  

---

## 1. 权威来源

| 来源 | URL | 访问 |
|---|---|---|
| Minitab Feature List（Time Series / ADF*） | https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-21 |
| NIST：过程监控 / 时序相关章（背景） | https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc3.htm | 2026-08-21 |
| Wikipedia / 教材 MacKinnon 临界值表（次级对照，写清） | 实现内嵌渐进临界值；非 Minitab 导出 | 2026-08-21 |

## 2. 产品锁定

- 命令 `adf_test`；单列数值序列；按行序；跳过非有限。  
- 回归规格（可选 `regression`）：  
  - `none`：\(\Delta y_t = \gamma y_{t-1} + \sum_{i=1}^{p}\delta_i\Delta y_{t-i}+\varepsilon_t\)  
  - `drift`（默认）：加常数  
  - `trend`：加常数 + 线性时间趋势  
- 滞后 `p`：用户指定；默认 \(\lfloor(T-1)^{1/3}\rfloor\)（Schwert 简化上界类），诊断写明。  
- 统计量：\(\tau=\hat\gamma / \mathrm{se}(\hat\gamma)\)（OLS 正态方程 + 对角近似 SE，与教材 ADF 一致口径）。  
- 报告：τ、渐近临界值（1%/5%/10%，MacKinnon 常用常数表）、相对 5% 临界值的比较结论（证据陈述，非「过程失控」）。  
- 可选：系数表（γ、漂移、趋势、滞后差分）。  
- `AdfFacts`：`n`、`lags`、`regression`、`tau`、`critical_5`、`reject_unit_root_at_5`、`used_observations`。  
- **不做：** KPSS、PP、自动滞后信息准则全套、CCF。

## 3. 接线

`adf_test.cpp` → `AdfFacts` → `AnalysisService::adf_test` → 命令/解释/序列化/测试/help；同步 backlog ACF/PACF/CCF/ADF 行。
