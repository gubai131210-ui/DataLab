# P1 Kruskal–Wallis 后 Steel–Dwass（近似）

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。近似正态 + 渐近 Tukey–Kramer 临界。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Steel–Dwass | Steel (1960)；Dwass (1960)；Hollander–Wolfe 非参数教材叙述 | 2026-08-20 |
| Critchlow–Fligner / 渐近 TK | Critchlow & Fligner (1991) 型 pairwise Wilcoxon + studentized-range 临界近似 | 2026-08-20 |
| Minitab 非参数多重比较叙述 | Minitab Kruskal / nonparametric multiple comparisons 帮助（表形对照） | 2026-08-20 |
| DataLab Dunn / CLD | [`p1_kruskal_dunn_posthoc.md`](p1_kruskal_dunn_posthoc.md)、[`p1_anova_tukey_grouping_letters.md`](p1_anova_tukey_grouping_letters.md) | 2026-08-20 |

## 2. 产品选型

深化 `kruskal_wallis`；配置 `nonparametric_posthoc`：

- `dunn`（默认）：现有 Dunn 表 + Grouping (Dunn) **数值与表形不变**
- `steel_dwass`：成对表为 Steel–Dwass（近似）；Grouping 标题 `(Steel-Dwass)`；复用 `tukey_grouping_letters`（只消费 `significant`）

`NonparametricFacts`：`posthoc_method`、`steel_dwass_available`；`dunn_available` 仅在 dunn 路径为 true。

家庭 α 默认 0.05。解释禁止「已证明相同/不同」。

## 3. 公式（近似）

对组 \(i\neq j\)，仅用两组样本做 Mann–Whitney / Wilcoxon 秩和（结修正，双侧）：

\[
Z_{ij}=\frac{W-\mu_W}{\sigma_W}
\]

（与现有 `mann_whitney` 标准化一致；可用 \(|Z|\)）。

令 \(k=\) 组数。渐近 Tukey–Kramer 临界：

\[
c=\frac{q_{\alpha,k,\infty}}{\sqrt{2}}
\]

其中 \(q_{\alpha,k,\infty}\) 为 studentized range 在 df→∞ 的上侧分位（可用标准正态分位近似：\(q_{\alpha,k,\infty}\approx \sqrt{2}\,z_{1-\alpha^*}\) 的稳妥实现见领域层注释；本产品采用 **\( |Z_{ij}| \ge q_{\alpha,k,\infty}/\sqrt{2} \)** 判显著）。

产品实现锁定：

1. 计算 pairwise Wilcoxon \(Z_{ij}\)（结修正）
2. \(p_\mathrm{raw}=2(1-\Phi(|Z|))\)
3. 用 \(q=\) studentized-range 渐近分位（领域已有或新增 `studentized_range_quantile_infty`）；`significant ⇔ |Z| ≥ q/√2`
4. 调整 P 可选报告为 `min(1, m·p_raw)` 仅作信息列；**显著性以临界 \(q/\sqrt{2}\) 为准**（与 CLD 一致）

## 4. 表形

| 表 | 合同 |
|---|---|
| 组摘要 / Kruskal H | 不变 |
| Steel–Dwass 成对比较 | 对比、Z、P、显著 |
| Grouping Information | 标题标明 Steel-Dwass |

## 5. 明确不做

精确 studentized-range 有限 df 表；Nemenyi 独立命令；改默认 Dunn；另算与 `significant` 不一致的字母。

## 6. 测试

三组等分布：无显著、共享字母；两组明显分离：字母与显著列一致；默认 `dunn` 回归。
