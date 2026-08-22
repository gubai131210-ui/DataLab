# P2：Nelson/MSSD · Hotelling T² · MEWMA · EMP Crossed

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。

## 0. 锁定与禁止偷懒

| ID | 交付 | 命令/改动 |
|---|---|---|
| B3 | I 图 Nelson estimate + MSSD（中位 MR 已有） | `imr`：`sigma_method` + `use_nelson_estimate` |
| A5 | Hotelling T² 个体图（Phase I Beta UCL） | `hotelling_t2` |
| A6 | MEWMA（λ 可配；渐近协方差说明） | `mewma` |
| A7 窄化 | EMP Crossed（Wheeler ICC 分级；复用交叉 Gage ANOVA） | `emp_crossed`；**非**全量 Expanded GLM |

**禁止偷懒：**

- 禁止把 PCA 经验 T² 冒充正式多元控制图  
- 禁止 Nelson estimate 只改 CL 不写剔除诊断表  
- 禁止 EMP 写成 AIAG「合格/不合格」硬判  
- 禁止假 golden；Jackson–Mudholkar 解析限仍 ⏸  
- 禁止全量 Expanded Gage（任意固定因子 GLM）本轮冒充完成  

---

## 1. B3 — Nelson estimate / MSSD / 中位 MR

| 来源 | URL | 访问 |
|---|---|---|
| Minitab I-MR Estimate | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/i-mr-chart/perform-the-analysis/i-mr-options/specify-estimation-options/ | 2026-08-21 |
| Minitab I 图公式 | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/i-mr-chart/methods-and-formulas/methods-and-formulas-for-individuals-chart/ | 2026-08-21 |

- **平均 MR**：\(\hat\sigma=\overline{MR}/d_2(w)\)  
- **中位 MR**：\(\hat\sigma=\mathrm{med}(MR)/d_4^*(w)\)（现有 `median_moving_range`）  
- **MSSD**：\(\mathrm{SRMSSD}=\sqrt{\sum d_i^2/(2(N-1))}/c_4'(N)\)（默认带无偏）  
- **Nelson estimate**：在平均 MR 路径上，剔除 \(MR > D_4\cdot\overline{MR}\)（等价于「超过平均 MR 的 3σ」），再重算 \(\overline{MR}\) 与限；诊断表列剔除个数  

---

## 2. Hotelling T²（个体，Phase I）

| 来源 | URL | 访问 |
|---|---|---|
| NIST Phase I individuals | https://www.itl.nist.gov/div898/handbook/pmc/section5/pmc5433.htm | 2026-08-21 |

\[
Q_j=(x_j-\bar x)'S^{-1}(x_j-\bar x),\quad
UCL=\frac{(m-1)^2}{m}\,B_{1-\alpha}(p/2,(m-p-1)/2)
\]

（Tracy–Young–Mason；本产品默认只画 UCL，LCL 可选诊断。）  
complete-case 多列；\(m>p+1\)；奇异协方差诊断。

---

## 3. MEWMA

| 来源 | URL | 访问 |
|---|---|---|
| NIST MEWMA | https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc343.htm | 2026-08-21 |

\[
Z_i=\lambda X_i+(1-\lambda)Z_{i-1},\quad
T_i^2=Z_i'\Sigma_{Z_i}^{-1}Z_i
\]

默认 \(\lambda=0.1\)；\(\Sigma_{Z_i}\) 用精确递推对角 λ 公式；UCL 用仿真表常数或用户输入（默认披露近似常数，非 ARL golden）。

本轮 UCL：用户可输 `ucl`；缺省用 NIST 示例量级说明 + 诊断「非仿真 ARL 校准」。

---

## 4. EMP Crossed（窄化）

| 来源 | URL | 访问 |
|---|---|---|
| Minitab EMP overview | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/evaluate-measurement-process-emp-crossed/before-you-start/overview/ | 2026-08-21 |
| EMP statistics | https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/measurement-system-analysis/how-to/gage-study/evaluate-measurement-process-emp-crossed/methods-and-formulas/emp-statistics/ | 2026-08-21 |

复用 `crossed_gage_rr` 方差分量：

- ICC(no bias)=Part/(Part+Repeat)  
- ICC(with bias)=Part/(Part+Repeat+Operator)  
- ICC(with interaction)=Part/(Part+Repeat+Op+Part×Op)  
- PE = 0.67449 √Repeatability  
- 分级：First ≥0.80；Second ≥0.50；Third ≥0.20；else Fourth（默认按 with-interaction ICC）  

不写「量具合格」。

---

## 5. 测试

`tests/p2_t2_mewma_nelson_emp_test.cpp`，`# source: formula_reference`。
