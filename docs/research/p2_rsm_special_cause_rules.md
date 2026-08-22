# P2：RSM 分析 · 特殊原因规则 Catalog · 默认策略

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。

## 0. 锁定与禁止偷懒

**做：**

| ID | 交付 |
|---|---|
| A2 | 命令 `rsm_response`：已有设计表上的二次响应曲面（线性+交互+纯二次）；系数表；≥2 因子等值线/静态曲面；残差四图；`RsmFacts` |
| B1 | Help 条目 `special_cause_rules`：Tests 1–8 全文 + 图种适用表 + 与 Minitab 默认仅 Test1 差异 + WECO 对照 + Zone≠Tests |
| B2 | 控制图配置 `special_cause_rule_policy`=`minitab_like`\|`all_applicable`（可序列化）；空 tests 时按策略默认；诊断写明误报风险 |

**禁止偷懒：**

- 禁止 RSM 无残差  
- 禁止把 Zone Jaehn 写成 Tests 1–8  
- 禁止改默认策略不序列化/不写误报风险  
- 禁止可旋转 3D / Graph Builder  
- 禁止假 golden；解释禁用「过程已失控 / 已证明稳定」等子串  

---

## 1. A2 RSM（已有设计表）

| 来源 | URL | 访问 |
|---|---|---|
| Minitab RSM model information | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/response-surface/analyze-response-surface-design/methods-and-formulas/model-information/ | 2026-08-21 |
| Minitab RSM coefficients | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/response-surface/analyze-response-surface-design/interpret-the-results/all-statistics-and-graphs/coefficients-table/ | 2026-08-21 |
| Minitab RSM fits/residuals | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/response-surface/analyze-response-surface-design/methods-and-formulas/fits-and-residuals/ | 2026-08-21 |

**产品锁定：**

- 输入：响应列 + ≥2 连续因子列（complete-case）；因子按列内 min/max 线性编码到 [-1,1]（若已全在 [-1,1] 则视为已编码）。  
- 模型（编码单位）：  
  \[
  y=\beta_0+\sum_i\beta_i x_i+\sum_{i<j}\beta_{ij}x_i x_j+\sum_i\beta_{ii}x_i^2+\varepsilon
  \]  
- 拟合：复用 `fit_linear_regression`（设计矩阵列=线性/交互/二次）。  
- 输出：系数表（Coef/SE/t/P）、模型摘要（R²/Adj/S）、ANOVA（回归/误差）；残差 vs 拟合/顺序/正态/直方图；任选两因子等值线+静态曲面（其余因子 hold=0 编码）。  
- `RsmFacts`：factor_count、term_count、residual_count、r_squared、contour_available、largest_|t|_term。  
- **不做：** CCD/BBD 设计生成；混合物；Box–Cox 自动；可旋转 3D；块效应本轮可诊断「未建模」。

---

## 2. B1 Special Cause Rules Catalog（help）

| 来源 | URL | 访问 |
|---|---|---|
| Minitab Tests 1–8 | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/basics/using-tests-for-special-causes/ | 2026-08-21 |
| Minitab 默认测试选项 | https://support.minitab.com/en-us/minitab/help-and-how-to/minitab-environment/settings-and-defaults/control-charts-and-quality-tools/tests/ | 2026-08-21 |
| NIST WECO | https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm | 2026-08-21 |

**Help 条目内容（正文禁止「见 md」）：**

1. Tests 1–8 定义（与 `all_special_cause_tests()` 一致）。  
2. 图种适用：I/Xbar/属性/Laney=1–8；R/S/MR/Z-MR=1–4；EWMA/MA/G/T=仅1；CUSUM=无；Zone=无 Tests（Jaehn）。  
3. DataLab 默认 `all_applicable` vs Minitab 常见默认仅 Test 1；多规则提高灵敏度也提高误报。  
4. WECO 四规则与 Tests 1/5/6/2 部分重叠说明。  
5. 解释约束：规则触发=调查信号，不是失控/合格/已证明稳定。

---

## 3. B2 默认策略 `minitab_like` vs `all_applicable`

**产品锁定：**

- `ControlConfiguration.special_cause_rule_policy`：  
  - `all_applicable`（现状默认）：空 tests → 该图种全部适用测试。  
  - `minitab_like`：空 tests → 仅 Test 1（且须在 applicable 内；CUSUM 仍无）。  
  - `explicit`：由 tests 文本列出。  
- UI：控制图对话框增加策略下拉或文本 `rule_policy`；恢复默认按策略勾选。  
- 序列化：已有 `special_cause_rule_policy`；确保 round-trip。  
- 诊断：选用 `all_applicable` 时 info「多规则提高误报风险，与 Minitab 默认仅 Test1 不同」。  
- `SpcFacts` 增补 `rule_policy`（若尚无）。  
- **不做：** 全局应用设置页（本轮仅分析配置）；不改 Zone Jaehn。

---

## 4. 测试（`# source: formula_reference`）

1. RSM：2 因子二次可识别小设计 → 系数有限；残差图存在；常数响应 → 二次项≈0。  
2. `minitab_like` + 空 tests → resolve 结果仅含 1（I 图）。  
3. `all_applicable` + 空 tests → I 图含 1–8。  
4. Help catalog 含 `special_cause_rules` 且正文无「见 md」。

## 5. 手工验收

见 brief §5q。
