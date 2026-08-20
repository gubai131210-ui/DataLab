# P1 Friedman 检验

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Friedman (1937) | Friedman M., The use of ranks to avoid the assumption of normality | 2026-08-20 |
| NIST / 教材结修正 | [NIST e-Handbook 7.4.3](https://www.itl.nist.gov/div898/handbook/prd/section4/prd43.htm)（非参数区组设计叙述） | 2026-08-20 |
| Minitab Friedman | [Friedman test](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/friedman-test/methods-and-formulas/methods-and-formulas/)（表形对照，不填未导出数） | 2026-08-20 |
| DataLab 非参数伴随图 | Kruskal 箱线/个体值模式；`align_complete_rows` | 2026-08-20 |

## 2. 产品选型

- 新命令 `friedman`
- 输入：**响应 + 处理 + 区组** 三列；complete-case；`source_row`
- 每区组内每个处理恰 1 个观测；否则诊断、不出统计量
- `NonparametricFacts.method=friedman`
- 本轮 **不做** 后多重比较（Steel–Dwass/Nemenyi）

## 3. 公式

设 \(b\) 个区组、\(k\) 个处理。区组 \(i\) 内对 \(k\) 个响应赋秩 \(R_{ij}\)（结取平均秩）。

\[
R_{.j}=\sum_{i=1}^{b} R_{ij},\qquad
\bar R_{.j}=R_{.j}/b
\]

无结：

\[
S=\frac{12}{bk(k+1)}\sum_{j=1}^{k} R_{.j}^2 - 3b(k+1)
\]

结修正：令 \(C=\sum_t(t^3-t)\) 对所有区组内结，

\[
S'=\frac{S}{1-C/\bigl(bk(k^2-1)\bigr)}
\]

（分母≤0 时退回 \(S\)）。近似 \(S\sim\chi^2_{k-1}\)。DF=\(k-1\)。

## 4. 表形

| 表/图 | 合同 |
|---|---|
| 处理摘要 | 处理、N、中位数、平均秩 |
| Friedman 检验 | S（或 S'）、DF、P、结修正 |
| 箱线 + 个体值 | 按处理；悬停 `source_row` |

## 5. 明确不做

后比较；宽表 zip；假 golden；改 Kruskal/Dunn。

## 6. 测试

平衡小样例核对 S/DF/P；缺处理区组诊断；complete-case 缺失不进图；`# source: formula_reference`。
