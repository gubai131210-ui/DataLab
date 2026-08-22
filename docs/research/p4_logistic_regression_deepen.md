# P4：Binary Logistic Regression 深化（逐步 AIC/BIC）

> 研究日期：2026-08-22 · 访问 2026-08-22（UTC+8）  
> Wave-4 W4-4；在 Wave-1/2 既有 IRLS + HL/VIF/杠杆/concordance 上深化；`formula_reference ≠ golden`。

## 锁定决策（二选一 · 已选）

| 选项 | 决定 |
|---|---|
| **A. Forward/Backward 逐步（AIC/BIC 窄化）** | **✅ 采用** — 复用 `stepwise_regression` 模式与 Wave-2 `forward_aicc` / `forward_bic` 惯例 |
| B. Holdout 验证混淆矩阵 | **❌ 不采用** — 延后 Wave-5+（需 holdout 列契约 + 与 Minitab Test/Train 对齐成本高） |

**理由：** 代码库已有 `fit_stepwise_regression` + `StepwiseRegressionFacts`（含 AICc/BIC 步骤表）；Minitab Binary Logistic 同样提供 Forward Information Criteria（AICc/BIC）；holdout 需额外数据分割 UI 与 `TimeSeriesConfiguration.validation_*` 式契约，超出 Wave-4 窄化范围。

| 命令 | 交付 |
|---|---|
| `logistic_regression` **深化** | 在既有全模型输出上增 **逐步步骤表**（forward/backward + `forward_aicc` / `forward_bic`）；终模型系数 / HL / VIF 等同屏；扩展 `LogisticFacts` |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/binary-logistic-regression/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-binary-logistic-model/perform-the-analysis/perform-stepwise-regression/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-binary-logistic-model/methods-and-formulas/stepwise/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-binary-logistic-model/methods-and-formulas/model-summary/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/fit-binary-logistic-model/interpret-the-results/all-statistics-and-graphs/model-summary-statistics/ | 2026-08-22 |

## Minitab 表形参考（非 golden）

### 逐步步骤表（Stepwise Details）

| 列 | 含义 |
|---|---|
| Step | 步骤序号 |
| Action | Enter / Remove / Start / Stop |
| Term | 进入或移除的预测变量 |
| Deviance / -2 Log L | 当前模型偏差（或 −2ℓ） |
| AIC / AICc / BIC | 信息准则（Forward IC 路径） |
| P-value | 进入/移除项的检验 P（α 路径） |

### 终模型（已有 · 保持）

| 表 | 列 |
|---|---|
| 系数表 | Term · Coef · SE · Z · P · Odds Ratio · CI |
| Model Summary | Log-L · AIC · AICc · BIC · Deviance R²（可选） |
| Hosmer-Lemeshow | Chi-Sq · DF · P（既有） |
| 拟合优度 / 关联 | Concordance · TP/TN/FP/FN（既有，**全样本**拟合分类） |

**不交付（本 Wave）：** K-fold ROC、Test Set Confusion Matrix、Validate with Test Sample。

## DataLab 交付范围

- **Domain 已有**（`logistic_regression.cpp`）：IRLS、系数、HL、VIF、杠杆、concordance、全样本 TP/TN/FP/FN。
- **本 Wave 新增**：
  - `fit_logistic_stepwise`（或等价）— 方法：`forward` / `backward` / `stepwise`（α）；`forward_aicc` / `forward_bic`。
  - 每步记录 deviance、AIC、AICc、BIC；`best_step_index` 指向 IC 最小步（IC 路径）或终 α 步。
  - **Configuration 扩展**：`InferenceConfiguration` 或专用块 — `logistic_stepwise_method`、`logistic_stepwise_criterion`（alpha/aicc/bic）、α enter/remove。
  - **Facts 扩展**（`LogisticFacts` 或嵌套 `LogisticStepwiseFacts`）：
    - `stepwise_method`、`stepwise_criterion`、`step_count`、`selected_predictor_count`、`best_step_index`
    - `steps[]`：step/action/term/deviance/aic/aicc/bic/enter_p/remove_p
    - 终模型 `log_likelihood`、`aic`、`bic`（与步骤表一致）
  - **UI**：逐步选项独立区（禁止与响应/预测主区堆叠超过一层）。
  - **algorithm_help.json**：逐步公式 + Primary URL。
- **契约**：二元响应 complete-case；≥2 候选预测；`source_row` 保留。

## 公式（# source: formula_reference）

**Logistic 模型：**

\[
P(Y=1 \mid x) = \frac{1}{1 + \exp(-x'\beta)}
\]

**对数似然（Individual Bernoulli）：**

\[
\ell(\beta) = \sum_i \left[ y_i \log p_i + (1-y_i)\log(1-p_i) \right]
\]

**AIC / AICc / BIC（p = 系数个数含截距）：**

\[
AIC = -2\ell + 2p,\quad
AICc = AIC + \frac{2p(p+1)}{n-p-1},\quad
BIC = -2\ell + p\ln n
\]

**Forward AICc/BIC：** 每步加入 p 最小候选项，记录各步 IC；终模型 = IC 最小步（与 `stepwise_regression` 同模式）。

**α 逐步：** 进入 P &lt; α_enter；移除 P &gt; α_remove（与线性逐步对称）。

## 测试要求

| 层级 | 要求 |
|---|---|
| **Domain** | `# source: formula_reference`：3 预测小样本 forward_aicc 步骤 IC 单调可审计；完全分离 diagnostic |
| **Service** | complete-case N；**真·A→B**；逐步关闭时输出与 Wave-2 回归一致 |
| **Serialize** | 新增 stepwise 字段 round-trip |
| **Interpret** | 含「逐步选择不稳定 / 非最优模型证明」；禁语 grep |
| **Regression** | HL/VIF/concordance 不回退；与 `stepwise_regression` 命令独立 |

## 明确不做（延后）

- **Holdout / Test set 混淆矩阵**（本 Wave 明确排除）。
- K-fold 交叉验证 ROC、Validate Model with Test Sample。
- 层次/随机效应 Logistic；名义/有序 Logistic 逐步（另命令）。
- Minitab Binary Logistic 逐步 golden；Best Subsets Logistic。
- 分类预测变量全量 one-hot 自动展开（Wave-4 仅连续 + 已编码 0/1 列，与现 logistic 一致）。
