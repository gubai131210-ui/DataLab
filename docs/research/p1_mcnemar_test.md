# P1 McNemar 配对 2×2

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。Edwards 连续性校正 χ²（教材/常见软件口径）；不填未导出 Minitab 数。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| McNemar 配对比例 | McNemar (1947)；教材 2×2 不一致格 \(b,c\) | 2026-08-21 |
| Edwards 连续性校正 | Edwards (1948)：\(\chi^2=(|b-c|-1)^2/(b+c)\) | 2026-08-21 |
| Minitab 为何用 McNemar / 表形 | [Why should I use McNemar's test?](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/supporting-topics/other-statistics-and-tests/why-should-i-use-mcnemar-s-test/) | 2026-08-21 |
| Minitab 2×2 方法页（对照叙述） | [Test for 2×2 tables — methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/tables/how-to/cross-tabulation-and-chi-square/methods-and-formulas/test-for-2x2-tables/)（Help 侧重 exact；本产品锁定 Edwards χ²，不作 golden） | 2026-08-21 |
| NIST / 分类叙述 | [NIST e-Handbook 7.2](https://www.itl.nist.gov/div898/handbook/prc/section2/prc2.htm)（列联/属性检验入口，表形对照） | 2026-08-21 |

## 2. 产品选型

- **新命令** `mcnemar`
- 输入：两列配对二元结果；**complete-case**（任一侧缺失则整行丢弃）；保留 `source_row`
- 二元编码（trim 后）：
  1. 恰好 **2** 个不同非缺失水平；或
  2. 数值 **0/1**；或
  3. 大小写不敏感：`pass`/`fail`、`yes`/`no`、`true`/`false`
- 两列水平集合必须可对齐为同一「阳性/阴性」编码；否则诊断、不出统计量
- Facts：`McNemarFacts`（独立结构，不并入一般卡方 Facts）

**禁止**改动 `chi_square` / `chi_square_gof` 边界与命令。

解释只读 Facts；禁止「已证明前后比例相同/不同」。

## 3. 公式

配对表（行=条件 1，列=条件 2；阳性为「成功」侧）：

|  | 条件2+ | 条件2− |
|---|---|---|
| 条件1+ | \(a\) | \(b\) |
| 条件1− | \(c\) | \(d\) |

不一致对数 \(n_d=b+c\)。原假设：边际阳性率相等（\(P(b)=P(c)\)）。

**产品锁定（Edwards）：** 当 \(b+c>0\)：

\[
\chi^2=\frac{\bigl(|b-c|-1\bigr)^2}{b+c},\qquad \mathrm{DF}=1
\]

\[
p=P\bigl(\chi^2_1\ge \chi^2\bigr)
\]

若 \(|b-c|-1<0\)，按惯例将分子置 0（\(\chi^2=0\)）。

**跳过校正 / 不可算：** \(b+c=0\)（无不一致对）→ 诊断说明；不出 χ²/P（或标 `not_computed`）。此时不套 Edwards 公式。

可选信息列：估计边际差 \(\hat\delta=(b-c)/n\)（\(n=a+b+c+d\)）；**不以 CI 为必出合同**，除非后续深化。

## 4. 表形

| 表 | 合同 |
|---|---|
| 2×2 计数 | a,b,c,d；不一致对数 \(b+c\) |
| McNemar 检验 | χ²（Edwards）、DF、P；注明连续性校正 |
| 诊断 | 非二元、水平不对齐、\(b+c=0\) |

## 5. 明确不做

改 `chi_square` / `chi_square_gof`；无配对宽表 zip；假 Minitab golden（Help 若给 exact P，不冒充一致）；Cochran Q / 多水平扩展；默认 exact 二项为主统计量（本轮锁定 Edwards）。

## 6. 测试

典型 \(b\neq c\) 小表：χ² 与手算 Edwards 一致；\(b=c>0\)：χ²=0、P=1；\(b+c=0\)：诊断；pass/fail 与 0/1 同表一致；`# source: formula_reference`。
