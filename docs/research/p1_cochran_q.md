# P1 Cochran’s Q

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。接 McNemar 二元缝；k≥3 宽表。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Cochran Q 标准公式 | Cochran (1950)；教材配对二元 k 处理 | 2026-08-21 |
| NIST / 分类叙述 | [NIST e-Handbook 7.2](https://www.itl.nist.gov/div898/handbook/prc/section2/prc2.htm) | 2026-08-21 |
| Minitab McNemar 邻接 | [Why McNemar](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/supporting-topics/other-statistics-and-tests/why-should-i-use-mcnemar-s-test/)（k=2 引导；不作 Q golden） | 2026-08-21 |
| DataLab 缝 | [`p1_mcnemar_test.md`](p1_mcnemar_test.md) 二元编码 | 2026-08-21 |

## 2. 产品选型

- **新命令** `cochran_q`
- 输入：宽表 **k≥3** 列配对二元；行主序 complete-case（任一侧缺失整行丢）
- 二元编码与 McNemar 相同（抽共享 helper）
- **k=2**：只诊断「请用 McNemar」，不算无校正 Q
- Facts：`CochranQFacts`（独立；不并入 ChiSquareFacts）

解释禁止「已证明各处理阳性率相同」。

## 3. 公式（锁定）

令 \(n\) 行（受试者）、\(k\) 列（处理）；\(X_{ij}\in\{0,1\}\)；
\(R_i=\sum_j X_{ij}\)，\(C_j=\sum_i X_{ij}\)，\(T=\sum_{i,j} X_{ij}\)。

\[
Q=\frac{(k-1)\bigl(k\sum_j C_j^2-T^2\bigr)}{kT-\sum_i R_i^2},\qquad
\mathrm{DF}=k-1
\]

\(p=P(\chi^2_{\mathrm{DF}}\ge Q)\)。分母 ≤0 或全同列 → 诊断、不可算。

小样本提示：\(n<4\) 或某列全 0/全 1 时诊断（仍可算则保留 Q）。

## 4. 表形

| 表 | 合同 |
|---|---|
| 处理成功计数 | 各列阳性数、阳性率、N |
| Cochran Q 检验 | Q、DF、P |
| （可选）条图 | 各处理阳性率 |

## 5. 明确不做

改 `chi_square` / `chi_square_gof`；把 Q 塞进 McNemar 命令；k=2 输出无校正 Q；假 golden。

## 6. 测试

经典小宽表手算 Q；k=2 仅诊断；非二元诊断；McNemar 回归不变；`# source: formula_reference`。
