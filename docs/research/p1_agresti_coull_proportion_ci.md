# P1 单比例 Agresti–Coull 置信区间

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> `formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX`。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab 1 Proportion methods | [1 Proportion methods and formulas](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-proportion/methods-and-formulas/methods-and-formulas/) | 2026-08-20 |
| Minitab 方法选项说明 | [Select analysis options for 1 Proportion](https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/basic-statistics/how-to/1-proportion/perform-the-analysis/select-the-analysis-options/) | 2026-08-20 |
| NIST 比例区间（Wilson / Agresti–Coull） | [NIST e-Handbook 7.2.4.1](https://www.itl.nist.gov/div898/handbook/prc/section2/prc241.htm) | 2026-08-20 |
| Agresti & Coull (1998) | 教材口径：在 Wilson 中心附近加伪计数后做 Wald 区间 | 2026-08-20 |

## 2. 产品选型

在已有 `one_proportion` 上增加方法 **`agresti_coull`**（Wilson 已落地，勿重做）：

| 方法字符串 | CI | 检验 |
|---|---|---|
| `exact` | Clopper–Pearson | 二项精确 |
| `normal` | Wald（样本 SE） | score z under \(p_0\) |
| `wilson` | Wilson score | 同一 score z |
| `agresti_coull` | **Agresti–Coull** | **同一 score z under \(p_0\)** |

本轮 **仅换 CI**。`ProportionFacts.ci_method=agresti_coull`。

## 3. 公式

设 \(\hat p = x/n\)，\(z = z_{1-\alpha/2}\)（单侧用 \(z_{1-\alpha}\)）。

```text
ñ = n + z²
x̃ = x + z²/2
p̃ = x̃ / ñ
SẼ = sqrt( p̃(1−p̃) / ñ )
CI = [p̃ − z·SẼ, p̃ + z·SẼ]
```

边界：

- \(x = 0\) → 下限强制为 0  
- \(x = n\) → 上限强制为 1  
- 区间再 clamp 到 \([0,1]\)

单侧：只输出对应一侧界。Score 检验与 `normal`/`wilson` 相同。

## 4. Minitab 表形（产品合同）

| 表 | 合同 |
|---|---|
| 单比例描述 | 事件 / 试验 / 比例 / N* / 行数 |
| 检验结果 | 方法、Z、P-Value、置信区间；方法列显示 `agresti_coull` |
| Facts | `method=agresti_coull`；`ci_method=agresti_coull` |

对照只记列名，不填未导出数值。

## 5. 导入契约

与现有 `one_proportion` 相同：complete-case 多行求和；`*` 计 N*。

## 6. 明确不做

- Blaker / Adjusted Blaker  
- Wilson 连续性校正  
- 两比例 Agresti–Coull  
- 改 Clopper–Pearson / Wald / Wilson 公式  
- 假 Minitab golden  

## 7. 测试策略

`# source: formula_reference`：手算 \(x=2,n=10,\alpha=0.05\) 上下限；`x=0`/`x=n` 边界；同一数据与 `normal` 的 Z/P 一致、CI 不同。
