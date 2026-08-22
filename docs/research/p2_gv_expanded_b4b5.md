# P2 收口：GV · Expanded Gage(3因子) · B4/B5 窄化

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。

## 0. 锁定

| ID | 交付 | 命令 |
|---|---|---|
| GV | 广义方差 \|S\| 子组图（Montgomery b1/b2） | `generalized_variance` |
| Expanded | **平衡** Part×Operator×附加因子 三因子随机 ANOVA | `expanded_gage_rr` |
| B4 | I-MR 历史参数 + 分阶段估计表 | 深化 `imr` |
| B5 | `special_cause_rules` 交叉链接 Run Chart / ANOM / Zone | help |

**禁止偷懒：**

- 禁止把个体观测假扮成 \|S\| 子组图（n≤p 必须诊断）  
- 禁止 Expanded 冒充任意不平衡 GLM / 固定效应全套  
- 禁止 B4 只改文案不写参数表  
- 禁止帮助正文「见 md」  

**刻意延后（⚪）：** 不平衡 Expanded、固定效应 φ、嵌套混合、>3 因子自动选模；GV 个体标准化替代图。

---

## 1. Generalized Variance（子组）

| 来源 | URL | 访问 |
|---|---|---|
| Minitab GV 公式 | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/multivariate-charts/generalized-variance-chart/methods-and-formulas/methods-and-formulas-for-generalized-variance-chart/ | 2026-08-21 |
| Montgomery (引用) | Introduction to Statistical Quality Control | — |

对等量子组大小 \(n>p\)：

\[
b_1=\frac{1}{(n-1)^p}\prod_{i=1}^{p}(n-i),\quad
b_2=b_1\left(\prod_{i=1}^{p}\frac{n-i+2}{(n-1)^2}-b_1\right)
\]

点：\(|S_i|=\det(S_i)\)。估 \(|\hat\Sigma|=\overline{|S|}/b_1\)。  
CL\(=b_1|\hat\Sigma|\)；UCL/LCL\(=|\hat\Sigma|(b_1\pm 3\sqrt{b_2})\)，LCL≥0。

NIST 亦指出多元变差图争议；产品披露「Montgomery \|S\| 子组图，非个体替代」。

---

## 2. Expanded Gage（3 因子平衡）

| 来源 | URL | 访问 |
|---|---|---|
| Minitab Expanded | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/expanded-gage-r-r-study/methods-and-formulas/methods-and-formulas/ | 2026-08-21 |

因子：Part、Operator、Additional（如 Gage/Location），**全部随机**，平衡交叉 + 重复。  
模型：主效应 + 全部二阶交互 + 残差（三阶并入残差若重复不足）。  
EMS → VarComp；Repeatability=残差；Reproducibility=Operator+Additional+相关交互（不含 Part）；Part-To-Part=Part。  
诊断：不平衡 → error；附加因子缺失 → 引导用 `gage_rr`。

---

## 3. B4 Historical / 阶段

I-MR 已有 `historical_center` / `historical_sigma` / `stage_column`。本轮增：

- 表「参数来源」：历史 μ/σ 或估计  
- 若有阶段：每阶段 N、均值、MR̄/d2 σ（估计，**不**覆盖全局历史限）  
- `SpcFacts`：`historical_parameters_used`、`stage_count`

---

## 4. B5 交叉链接

`special_cause_rules` 正文增加：Run Chart（游程）、ANOM（决策限≠控制图 Tests）、Zone（Jaehn≠Tests 1–8）交叉说明。
