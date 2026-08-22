# P4：Cox 回归（固定协变量 · PH 窄化）

> 研究日期：2026-08-22 · 访问 2026-08-22（UTC+8）  
> Wave-4 W4-3；新增 `cox_regression`；与 ALT / probit / Fine-Gray 分流；`formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `cox_regression` | 固定协变量 Cox PH：\(\lambda(t|x)=\lambda_0(t)\exp(x'\beta)\)；系数 / SE / HR / P + Log-Likelihood；右删失 complete-case + `source_row`；`CoxRegressionFacts` |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/supporting-topics/basics/reliability-analyses-in-minitab/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/cox-regression/with-fixed-predictors-only/methods-and-formulas/methods/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/cox-regression/with-fixed-predictors-only/methods-and-formulas/coefficients/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/cox-regression/with-fixed-predictors-only/methods-and-formulas/relative-risks/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/reliability/how-to/cox-regression/with-fixed-predictors-only/interpret-the-results/all-statistics-and-graphs/coefficients-table/ | 2026-08-22 |

## Minitab 表形参考（非 golden）

### Coefficients / Relative Risks 表

| 列 | 含义 |
|---|---|
| Term / Predictor | 协变量名（含截距若 Minitab 展示） |
| Coef | \(\hat\beta\) |
| SE Coef | 系数标准误（信息矩阵对角） |
| Z | Wald \(Z = \hat\beta / SE\)（或等价） |
| P-Value | 双侧 Wald P |
| HR / Exp(Coef) | \(\exp(\hat\beta)\)（连续预测默认每 +1 单位） |
| CI Lower / CI Upper | HR 置信区间 |

### Model Summary / Goodness-of-Fit

| 统计量 | 含义 |
|---|---|
| Log-Likelihood | 偏对数似然 \(\ell(\hat\beta)\) |
| -2 Log L | \(-2\ell\)（可选导出） |
| N | 有效行数 |
| Events | 失效数 |
| Censored | 右删失数 |

### 图（Wave-4 可选 · 非必交付）

- Cox 残差图、生存曲线按协变量分层 — **延后**；本 Wave 以系数表 + Log-L 为主。

## DataLab 交付范围

- **新 domain**：`src/domain/statistics/cox_regression.{h,cpp}`（模板：`bootstrap_two_sample.cpp` + `logistic_regression.cpp` IRLS/Newton 模式）。
- **Configuration**：`CoxRegressionConfiguration` — time_column、event_column、covariate_columns、confidence_level、ties_method（`breslow` 默认，可选 `efron` 窄化）。
- **Facts**：`CoxRegressionFacts` — n、events、censored、converged、log_likelihood、coefficients[]（term/beta/se/z/p/hr/ci）、evidence_type=`formula_reference`、algorithm_id=`cox_ph_fixed_covariates`。
- **删失契约**：复用 `censoring_contract` / `parse_reliability_event`；**仅 exact + right**；complete-case 剔除缺测协变量行；保留 `source_rows`。
- **Service / commands**：`cox_regression` 独立命令 id；interpretation + catalog 双语；`algorithm_help.json` 公式块。
- **解释层**：HR 为相对风险证据，禁止「已证明因果 / 寿命达标」。

## 公式（# source: formula_reference）

**Cox PH 模型：**

\[
\lambda(t \mid x_i) = \lambda_0(t)\,\exp(x_i'\beta)
\]

**偏对数似然（Breslow ties 窄化）：**

\[
\ell(\beta) = \sum_{i \in \text{events}} \left[ x_i'\beta - \log \sum_{j \in R(t_i)} \exp(x_j'\beta) \right]
\]

**系数 SE：** 信息矩阵 \(I(\hat\beta)\) 逆的对角平方根（Newton–Raphson 收敛后）。

**Hazard Ratio：** \(\widehat{HR} = \exp(\hat\beta)\)；连续协变量 c 单位变化：\(\exp(c\hat\beta)\)。

**Wald P：** 基于 \(Z = \hat\beta / SE\) 的正态近似（与 Minitab 表形一致；非 golden）。

## 测试要求

| 层级 | 要求 |
|---|---|
| **Domain** | `# source: formula_reference`：无 ties 小样本手算 \(\ell\)、\(\hat\beta\)、HR；奇异信息矩阵报错 |
| **Censoring** | 右删失行进入风险集但不作事件；event=0 计数正确 |
| **Service** | complete-case N；**真·A→B** + `source_row` 可审计 |
| **Serialize** | `CoxRegressionFacts` round-trip |
| **Interpret** | 禁语 grep；与 Fine-Gray / ALT 命令 id 不混淆 |
| **Regression** | Wave-3 reliability 测试不回退 |

## 明确不做（延后 · 登记 Wave-5+）

- **Fit Cox Model in Counting Process Form**（时依协变量、计数过程）。
- 左删失 / 区间删失 Cox；left truncation / entry time。
- 分层 Cox（stratification variable）；robust sandwich（聚类/重复事件）。
- Stepwise Cox 变量选择；交互项 / 多项式全量。
- Minitab Cox 数值 golden；Efron/Breslow 双轨 golden 对齐。
- 基线 hazard / 生存曲线导出（除非 Wave-4 末显式加 scope）。
