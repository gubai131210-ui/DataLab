# P1 Kruskal–Wallis 后 Dunn 多重比较

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。近似正态 Z + Bonferroni。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Dunn (1964) | Dunn O.J., Multiple comparisons using rank sums | 2026-08-20 |
| Minitab KRUSMC 叙述 | [Kruskal Wallis multiple comparisons macro](https://support.minitab.com/en-us/minitab/macro-library/macro-files/nonparametrics-macros/krusmc/) | 2026-08-20 |
| CLD 复用 | [`p1_anova_tukey_grouping_letters.md`](p1_anova_tukey_grouping_letters.md) | 2026-08-20 |
| DataLab Kruskal | `nonparametric_tests` / `pca-nonparametric-variance-chart-formulas.md` | 2026-08-20 |

## 2. 产品选型

深化 `kruskal_wallis`（不改 H 统计量本体）：

1. **Dunn 成对比较**表：对比、平均秩差、SE、Z、未调整 P、Bonferroni 调整 P、显著  
2. **Grouping Information**：水平、N、中位数、Grouping — 复用 `tukey_grouping_letters`（只消费 pairwise `significant`）

`NonparametricFacts`：`dunn_available`、`grouping_letter_count`、`posthoc_pair_count`。  
家庭 α 默认 0.05。解释禁止「已证明相同/不同」。

## 3. 公式（近似）

用 Kruskal **全局秩**得到组平均秩 \(\bar R_i\)。对组 i≠j：

```text
ΔR = R̄_i − R̄_j
SE  = sqrt( (N(N+1)/12 − C_tie) · (1/n_i + 1/n_j) )
Z   = |ΔR| / SE
p_raw = 2·(1−Φ(Z))
m = k(k−1)/2
p_adj = min(1, m·p_raw)   # Bonferroni
significant ⇔ p_adj ≤ α
```

结修正（与常见 Dunn 实现一致）：

```text
C_tie = Σ_t (t³−t) / (12(N−1))
```

其中 t 为各结大小；无结时 C_tie=0。N 为全体 complete-case 观测数。

## 4. 表形

| 表 | 合同 |
|---|---|
| 组摘要 / 检验统计量 | 现有不变 |
| Dunn 成对比较 | 见上 |
| Grouping Information | 标题标明 Dunn；字母来自显著矩阵 |

## 5. 明确不做

Steel–Dwass / Nemenyi；精确 studentized-range；另算一套与 `significant` 不一致的 CLD 临界值。

## 6. 测试

三组等分布：无显著、共享字母；两组明显分离：字母不相交且与显著列一致。
