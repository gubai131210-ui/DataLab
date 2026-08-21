# P1 Friedman 后 Nemenyi（近似）

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。Friedman 平均秩差 + 渐近 studentized-range 临界（与 Steel–Dwass 同一 `q/√2` 口径）。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Nemenyi / WNMT 叙述 | Hollander–Wolfe 非参数教材；Demšar (2006) 型平均秩 CD 叙述 | 2026-08-21 |
| SE 与 CD（q/√2） | PMCMR / 教材：`\|R̄ᵢ−R̄ⱼ\|` 对 `q_{α,k,∞}/√2 · SE`；`SE=√(k(k+1)/(6b))` | 2026-08-21 |
| Minitab Friedman（主检验表形） | [Friedman methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/friedman-test/methods-and-formulas/methods-and-formulas/)（对照主检验，不填未导出后比较数） | 2026-08-21 |
| NIST 区组非参数 | [NIST e-Handbook 7.4.3](https://www.itl.nist.gov/div898/handbook/prd/section4/prd43.htm) | 2026-08-21 |
| DataLab 同族近似 / CLD | [`p1_kruskal_steel_dwass.md`](p1_kruskal_steel_dwass.md)、[`p1_anova_tukey_grouping_letters.md`](p1_anova_tukey_grouping_letters.md)、[`p1_friedman_test.md`](p1_friedman_test.md) | 2026-08-21 |

## 2. 产品选型

深化已有命令 `friedman`（不改 S/S' 本体）：

- 配置 `posthoc`：**默认无后比较**；仅 `posthoc=nemenyi` 时输出成对表 + Grouping
- 成对行复用现有 `DunnComparison` 字段形（对比、平均秩差、SE、Z、P、显著等；列语义按 Nemenyi）
- Grouping 标题标明 `(Nemenyi)`；复用 `tukey_grouping_letters`（只消费 pairwise `significant`）

`NonparametricFacts`：`posthoc_method`（空 / `nemenyi`）、`nemenyi_available`；主检验字段不变。

家庭 α 默认 0.05。解释禁止「已证明相同/不同」。

## 3. 公式（近似）

沿用 Friedman 区组内秩，得处理平均秩 \(\bar R_{.i}\)、\(\bar R_{.j}\)（\(b\) 区组、\(k\) 处理）。对处理对 \(i\neq j\)：

\[
\mathrm{SE}=\sqrt{\frac{k(k+1)}{6b}},\qquad
Z_{ij}=\frac{|\bar R_{.i}-\bar R_{.j}|}{\mathrm{SE}}
\]

渐近临界（与 Steel–Dwass 产品锁定一致）：

\[
c=\frac{q_{\alpha,k,\infty}}{\sqrt{2}}
\]

产品实现锁定：

1. 由已算好的 Friedman 平均秩构造全部 \(k(k-1)/2\) 对
2. \(p_\mathrm{raw}=2(1-\Phi(|Z|))\)（信息列；可选 Bonferroni 仅作信息）
3. `significant ⇔ |Z| ≥ q_{α,k,∞}/√2`（显著性与 CLD 一致）
4. **不**做精确有限 df studentized-range 表；**不**另开独立 `nemenyi` 命令

## 4. 表形

| 表 | 合同 |
|---|---|
| 处理摘要 / Friedman S | 不变（见 [`p1_friedman_test.md`](p1_friedman_test.md)） |
| Nemenyi 成对比较 | 对比、平均秩差、SE、Z、P、显著（`DunnComparison` 形） |
| Grouping Information | 标题标明 Nemenyi；字母来自 `significant` |

默认无 `posthoc`：上述两张后比较表均不出。

## 5. 明确不做

独立 `nemenyi` 命令；重做 Kruskal Dunn / Steel–Dwass；精确 studentized-range 有限 df；默认开启后比较；另算与 `significant` 不一致的字母。

## 6. 测试

平衡等效应：无显著、共享字母；两处理明显分离：字母与显著列一致；默认无 posthoc 回归；`# source: formula_reference`。
