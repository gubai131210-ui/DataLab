# P1 回归 Durbin–Watson 临界界与判定

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。保留现有 DW 统计量；用 Savin–White 风格 α=0.05 界替换 1.5/2.5 启发式。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Durbin–Watson 统计量 | Durbin & Watson (1950/1951)；残差一阶序列相关界检验 | 2026-08-21 |
| Savin–White 界表 | Savin & White (1977) 扩展 dL/dU（含截距模型） | 2026-08-21 |
| Farebrother 无截距表 | Farebrother (1980)（本产品有截距回归为主；范围外不硬套） | 2026-08-21 |
| Minitab DW 用法与表 | [Test for autocorrelation by using the Durbin-Watson statistic](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/supporting-topics/model-assumptions/test-for-autocorrelation-by-using-the-durbin-watson-statistic/)（α=0.05；对照判定叙述，不填未导出样例数） | 2026-08-21 |
| DataLab 现状 | `regression.cpp`：输入顺序 DW；诊断现用 1.5/2.5 启发式（**本轮替换**） | 2026-08-21 |

## 2. 产品选型

深化已有回归诊断（不改拟合方程）：

- **保留** \(DW=\sum_{i=2}^{n}(e_i-e_{i-1})^2\big/\sum e_i^2\)，观测序 = **输入行序**
- 查/插值 α=0.05 的 \(d_L,d_U\)；写入 `RegressionFacts`：
  - `durbin_watson_dl`
  - `durbin_watson_du`
  - `durbin_watson_decision`
- \(k'\) = **不含截距**的回归元个数（斜率项数）。若参照 Minitab 表「terms including intercept」，映射为 \(\mathrm{terms}=k'+1\)
- 查表锁定范围：\(n\in[15,100]\)，\(k'\in[1,5]\)；在网格内对已发表界值做插值（n 非表点时对相邻发表 n 线性插值 \(d_L,d_U\)）
- **范围外**（n 或 \(k'\) 越界、或无截距/特殊设计未覆盖）：\(d_L,d_U\) 与 decision 标 `not_computed`（仍可报告 DW 数值）

解释禁止「已证明无自相关」； inconclusive 区如实报告。

## 3. 公式

### 3.1 统计量（不变）

\[
DW=\frac{\sum_{i=2}^{n}(e_i-e_{i-1})^2}{\sum_{i=1}^{n}e_i^2}
\]

残差按回归所用 complete-case 的**输入顺序**排列。

### 3.2 临界界

采用 **Savin–White 风格** α=0.05 近似界（实用实现：内嵌/插值已发表表，而非运行时 Farebrother 特征根精确分布）。有截距模型；\(k'\) 定义见上。

### 3.3 五区判定（产品锁定）

| 条件 | `durbin_watson_decision` |
|---|---|
| \(DW < d_L\) | `positive_autocorr` |
| \(d_L \le DW \le d_U\) | `inconclusive` |
| \(d_U < DW < 4-d_U\) | `no_evidence` |
| \(4-d_U \le DW \le 4-d_L\) | `inconclusive_neg` |
| \(DW > 4-d_L\) | `negative_autocorr` |

（与 Minitab「用 \(4-D\) 测负相关」叙述等价的双侧五区写法。）

**替换**原启发式：`DW<1.5` 或 `DW>2.5` → evidence_against。

诊断 `residual_independence` 状态映射建议：`positive_autocorr` / `negative_autocorr` → `evidence_against`；`no_evidence` → `no_evidence_against`；`inconclusive*` / 界未算 → `not_computed` 或单独 inconclusive 文案（实现时统一一种，测试锁定）。

## 4. 表形

| 输出 | 合同 |
|---|---|
| 回归诊断 / 假设检查 | 现有 DW 值 + dL、dU、decision 文案 |
| Facts JSON | `durbin_watson`、`durbin_watson_dl`、`durbin_watson_du`、`durbin_watson_decision` |

## 5. 明确不做

假 Minitab golden 数值表冒充逐格一致；运行时 Imhof/Farebrother 精确 p；把 1.5/2.5 启发式留作并行默认；无截距专用 Farebrother 全表（范围外 `not_computed` 即可）；改 DW 为时间戳序（除非另开需求）。

## 6. 测试

已知残差近独立：decision=`no_evidence`（界在范围内时）；强正自相关序列：`positive_autocorr`；n 或 k' 越界：dl/du/decision=`not_computed` 且仍有 DW；回归输出无 1.5/2.5 文案；`# source: formula_reference`。
