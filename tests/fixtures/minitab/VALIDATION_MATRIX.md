# 验证矩阵（fixtures/minitab）

公开数据集 CSV 可作**输入**；数值基线分双轨：

| 轨道 | 含义 | 本 Goal（G-Trust） |
|------|------|-------------------|
| **ref-golden** | `reference_implementation` 脚本冻结 → `*_ref_golden.tsv` | **本轮完成定义**：锁表命令缺文件必须 FAIL，禁止 QSKIP 冒充 |
| **vendor 导出** | 真·Minitab 导出 → `vendor_oracle` | **后续 Goal**；不得用「等导出」冒充本轮完成 |

**禁止**把「公开输入 + 自写脚本输出」写成「已与 Minitab 数值对齐」。

## 数据集形状检查（输入完整性）

| 数据集 | DataLab 分析 | 配置 | 当前验证 |
|---|---|---|---|
| CrankshaftMovement | Xbar-R | A to B Distance，按 Date 分组 | CSV 行数、列名、125 行 |
| CamshaftLength | Xbar-R / G-Trust `xbar_r` | Machine 1 按 Subgroup ID（20×n=5） | CSV + Wave-1 `xbar_r_ref_golden.tsv` |
| PistonRingDiameter | Cp/Cpk/Pp/Ppk | 每 5 行一个子组，73.95/74.05，Target 74.00 | CSV 行数、列名、规格记录 |
| PinLength | 分组能力分析 | Length 按 Machine 分组，13/25 | CSV 行数、列名 |
| UnansweredCalls | P 图 | Unanswered Calls / Total Calls | 21 行、两列计数 |
| CableWires | Xbar-R/能力分析 | Diameter、Subgroup，0.50/0.60 | 缺失值保留为 `*` |
| imr_ref_golden_input | I-MR | 列 `Value`；与历史 `imr_synthetic` 同源小 N | 4 行；Wave-1 ref-golden |
| factorial_2x2 | 双因素 ANOVA | Response，FactorA，FactorB | Seq/Adj SS、交互项和秩亏诊断 |
| arima_trend | ARIMA 基础预测 | Period、Value，AICc，4 期预测，max_d=1 | 历史 `arima_trend_golden.tsv`（公式镜像；≠ vendor） |
| regression.csv | 线性回归 | Response，Temperature，Pressure | 历史 `regression_golden.tsv`（公式镜像；≠ vendor） |

## Golden 文件与容差

### G-Trust ref-golden（`golden` ← `reference_implementation`）

| 文件 | 来源 | 容差 | 状态 |
|---|---|---|---|
| `expected/imr_ref_golden.tsv` | NIST PMC 3.2.2 + `scripts/g_trust_imr_reference.py` | 中心/限/σ：rel≤1e-4 / abs≤1e-6 | ref-golden 已冻 |
| `expected/xbar_r_ref_golden.tsv` | NIST PMC 3.2.1 + `scripts/g_trust_xbar_r_reference.py` | 同上 | ref-golden 已冻 |
| `expected/p_chart_ref_golden.tsv` | NIST PMC 3.3.2 + `scripts/g_trust_p_chart_reference.py` | 同上 | ref-golden 已冻 |
| `expected/capability_ref_golden.tsv` | NIST PMC 1.6 + `scripts/g_trust_capability_reference.py` | Cp/Cpk/Pp/Ppk abs≤1e-3 | ref-golden 已冻 |
| `expected/capability_sixpack_ref_golden.tsv` | 同上 + sixpack 入口；`contract` min_plots=5 expected_plots=6 | 指数 + 图数量精确 | ref-golden 已冻 |
| `expected/between_within_capability_ref_golden.tsv` | NIST PMC 1.6 + domain BW + `scripts/g_trust_between_within_capability_reference.py`；规格 95/105/100 | σ rel≤1e-4；指数 abs≤1e-3 | ref-golden 已冻 |
| `expected/gage_rr_ref_golden.tsv` | crossed ANOVA + `scripts/g_trust_gage_rr_reference.py` | %Study Var / ndc abs≤0.05 | ref-golden 已冻 |
| `expected/two_sample_t_ref_golden.tsv` | NIST EDA 3.5.3 Welch + `scripts/g_trust_two_sample_t_reference.py` | t/CI rel≤1e-4；p abs≤1e-4；Welch df abs≤1e-6 | ref-golden 已冻 |
| `expected/normality_test_ref_golden.tsv` | NIST EDA 3.5.e AD + `scripts/g_trust_normality_test_reference.py` | A² rel≤1e-4；p abs≤1e-4 | ref-golden 已冻 |
| `expected/one_way_anova_ref_golden.tsv` | NIST PRC 4.3 + `scripts/g_trust_one_way_anova_reference.py` | SS/F rel≤1e-4；DF 精确；p abs≤1e-4 | ref-golden 已冻 |

复跑：

```powershell
python scripts/g_trust_imr_reference.py
python scripts/g_trust_xbar_r_reference.py
python scripts/g_trust_p_chart_reference.py
python scripts/g_trust_capability_reference.py
python scripts/g_trust_capability_sixpack_reference.py
python scripts/g_trust_between_within_capability_reference.py
python scripts/g_trust_gage_rr_reference.py
python scripts/g_trust_two_sample_t_reference.py
python scripts/g_trust_normality_test_reference.py
python scripts/g_trust_one_way_anova_reference.py
python tools/verify_g_trust_golden_gate.py
```

### 历史 regression / arima（非本 Goal 完成证据）

| 文件 | 来源 | 容差 |
|---|---|---|
| `expected/regression_golden.tsv` | OLS 公式参考；**可**日后用 Minitab 导出替换 `# source` | 系数/SS：相对 ≤1e-4；DW：相对 ≤1e-4 |
| `expected/arima_trend_golden.tsv` | 镜像 `arima.cpp` 候选网格；**可**日后用 Best ARIMA 导出核对 | AICc：绝对 ≤0.01；Forecast：绝对 ≤0.05 |
| `expected/EXPORT_GUIDE.md` | 双轨说明 | — |

```powershell
.\.venv\Scripts\python.exe tools\dump_minitab_golden_reference.py
```

自动化测试：

- **锁表 `*_ref_golden.tsv` 缺失** → **FAIL**（禁止 QSKIP）。
- **历史** `regression_golden.tsv` / `arima_trend_golden.tsv` 缺失 → 可维持历史 QSKIP；**不等于** G-Trust Goal 完成。

Johnson 变换、非正态 Z-score、对数正态可靠性、乘法 SARIMA CSS、三参数 Weibull、
Fleiss Kappa、Kendall W/τ、两参数指数、三参数对数正态、PCA 系数/T²Q、非参数 ties
与 Levene 中位数等方差的自动化测试是 **公式参考**（`# source: formula_reference`），
**不是** Minitab 导出。不要把它们登记为数值对齐。

## 本批黄金检查项

- 双因素 ANOVA：`DF`、`SS`、`MS`、`F`、`P-Value`、因子均值和交互均值。
- 回归诊断：`Durbin-Watson`、内部/删除学生化残差、Cook's D、DFITS 和 VIF；ANOVA `Seq SS` / `Adj SS`。
- ARIMA：候选模型名称、AIC/AICc/BIC、Forecast、Lower、Upper。
- G-Trust Wave-1：I-MR / Xbar-R / P 图中心线与控制限 vs `*_ref_golden.tsv`。
- G-Trust Wave-2：capability / sixpack / between_within vs ref-golden。
- G-Trust Wave-3：gage_rr / two_sample_t / normality_test / one_way_anova vs ref-golden。
- G-Trust Wave-4：VALIDATION_MATRIX / backlog **ref-golden 已冻**；`verify_g_trust_golden_gate.py` 默认全集 PASS（≠ vendor 对齐）。
- 输出页面：长中文标题、11 列以上表格、诊断卡片和多图布局不发生文字重叠。
