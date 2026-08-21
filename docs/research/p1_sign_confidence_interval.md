# P1 符号检验中位数置信区间（Sign CI）

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。加深 `sign_test`；**不改**主检验二项精确 P。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 1-Sample Sign 方法 | [Methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/1-sample-sign/methods-and-formulas/methods-and-formulas/) | 2026-08-21 |
| Minitab 关键结果 | [Key results](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/1-sample-sign/interpret-the-results/key-results/) | 2026-08-21 |
| 序统计插值 CI | Hettmansperger & Sheather (1986), *Statistics & Probability Letters* 4(2) | 2026-08-21 |
| NIST 非参数入口 | [NIST e-Handbook 7.4](https://www.itl.nist.gov/div898/handbook/prd/section4/prd4.htm) | 2026-08-21 |

## 2. 产品选型

- 加深命令 `sign_test` / `sign_test_paired`：增加 `confidence`（默认 95%）
- 抽出可复用 **`sign_median_ci`**：供 Sign 与 Mood 各组共用
- CI 基于**全部有限观测**的序统计（与相对 η0 的符号计数独立）
- 主 P 仍为去结后二项精确；表形增加「中位数置信区间」
- Facts：填充已有 `NonparametricFacts.location_estimate` / `ci_lower` / `ci_upper`

**明确不做：** 改 Sign 主 P；Wilcoxon 本体；假 Minitab 三区间 golden 数值。

## 3. 公式（锁定）

令有限观测排序 \(X_{(1)}\le\cdots\le X_{(n)}\)，目标置信水平 \(\gamma\in(0,1)\)，\(\alpha=1-\gamma\)。  
\(B\sim\mathrm{Bin}(n,1/2)\)。

1. 点估计：样本中位数 \(\hat\eta=\mathrm{median}(X)\)。
2. 取最大整数 \(d\ge 0\) 使 \(P(B<d)<\alpha/2\)（即 \(P(B\le d-1)<\alpha/2\)；若无则 \(d=0\)）。
3. **精确窄区间**（达到水平 \(\le\gamma\) 侧）：\([X_{(d+1)},\,X_{(n-d)}]\)，  
   \(\gamma_{\mathrm{narrow}}=1-2\,P(B\le d)\)（需 \(1\le d+1\le n-d\)）。
4. **精确宽区间**（达到水平 \(\ge\gamma\) 侧，需 \(d\ge 1\)）：\([X_{(d)},\,X_{(n-d+1)}]\)，  
   \(\gamma_{\mathrm{wide}}=1-2\,P(B\le d-1)\)。
5. **主报告区间（产品锁定）：** 在可算的精确区间中，选取 **achieved 最接近 \(\gamma\)** 者（若并列优先 \(\ge\gamma\) 的宽区间）。报告 `achieved_confidence`。  
   **不实现** Hettmansperger–Sheather NLI 中间插值（避免与未导出 Minitab 数值假对齐）。
6. \(n<2\) 或无法形成合法序统计区间 → 诊断、不出 CI。

配对：对差分 \(d_i=x_i-y_i\) 的有限值做同上 CI（相对中位数 0 的位置区间）。

## 4. 表形

| 表 | 合同 |
|---|---|
| 符号摘要 | 保持 N / 结 / n+ / n− / 中位数 / η0 |
| 符号检验 | 主 P 不变 |
| 中位数置信区间 | 估计、下限、上限、名义 γ、达到水平、方法=`sign_order_statistic` |

## 5. 明确不做

NLI 三区间并列表；假 golden；改 Mood χ²；改 Wilcoxon。

## 6. 测试

小样本手算 \(d\) 与边界；全结/ n=1 无 CI；配对差分与单列差分一致；主 P 回归；`# source: formula_reference`。
