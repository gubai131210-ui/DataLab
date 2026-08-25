# P6：Weibull 演示型可靠性试验计划

> 研究日期：2026-08-25 · 访问 2026-08-25（UTC+8）  
> Wave-6 W6-4；`reliability_test_plan`；工具型；无工作表读取。

## 锁定

| 命令 | 交付 |
|---|---|
| `reliability_test_plan` | 给定 Weibull β、目标可靠度 R、置信度 CL、试验时长/任务时长比；零失效或允许 r 失效；输出 n + 假设摘要 |

## Primary Sources

| URL | 访问 |
|---|---|
| https://help.reliasoft.com/articles/content/hotwire/issue118/relbasics118.htm | 2026-08-25 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/demonstration-test-plan/before-you-start/example/ | 2026-08-25 |
| https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-25 |

## 表形

- Test Plan（n、allowed_failures、test_time、mission_time、beta、R、confidence）
- Assumptions Summary（β 假设、演示型、非估计）

## 公式（# source: formula_reference）

**零失效（success-run）+ Weibull 时间比调整**

目标：以置信水平 \(CL\) 演示任务时间 \(t_m\) 处可靠度至少为 \(R\)。  
每单元试验时长 \(T_0\)，形状 \(\beta\) 已知（假设）：

\[
\delta = \left(\frac{T_0}{t_m}\right)^{\beta},\qquad
n = \left\lceil \frac{\ln(1-CL)}{\delta\cdot\ln R} \right\rceil
\]

当 \(T_0=t_m\) 时 \(\delta=1\)，退化为经典 \(n=\lceil\ln(1-CL)/\ln R\rceil\)。

**允许 r 次失效（窄化）**：在等效可靠度 \(R_{\mathrm{test}}=R^{\delta}\) 下，求最小 \(n\) 使二项累积满足

\[
\sum_{k=0}^{r}\binom{n}{k}(1-R_{\mathrm{test}})^k R_{\mathrm{test}}^{n-k} \le 1-CL
\]

（实现可用递推/逐 n 搜索；禁止嵌入 Python。）

诚实说明：β 为**假设**，非数据估计；不宣称「寿命已达标」。

## UI 分页

1. 输入（β、R、CL、T₀、tₘ、允许失效数）  
2. 方法说明  
3. 结果页（n + 假设摘要）  

## 明确不做

- 全部分布族、多阶段贝叶斯计划、Minitab 数值 golden、与分布拟合同页
