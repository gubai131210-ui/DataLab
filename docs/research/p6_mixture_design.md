# P6：Mixture 设计生成（simplex-lattice）

> 研究日期：2026-08-25 · 访问 2026-08-25（UTC+8）  
> Wave-6 W6-2；`mixture_design`；仅设计生成 + 预览 + 写表；分析留给 Wave-7。

## 锁定

| 命令 | 交付 |
|---|---|
| `mixture_design` | q=3～4；**simplex-lattice degree m=2**；∑xᵢ=1；矩阵预览页；worksheet_export；清空旧 excluded/hidden |

## Primary Sources

| URL | 访问 |
|---|---|
| https://www.itl.nist.gov/div898/handbook/pri/section5/pri54.htm | 2026-08-25 |
| https://www.itl.nist.gov/div898/handbook/pri/section5/pri542.htm | 2026-08-25 |
| https://www.minitab.com/en-us/products/minitab/features/ | 2026-08-25 |

## 表形

- Design Info（type、q、degree、points）
- Design Matrix（StdOrder / RunOrder / x1..xq）
- worksheet_export：x1..xq + RunOrder + 空 Response 列

## 公式 / 点集（# source: formula_reference）

**Simplex-lattice {q, m}**（本波固定 m=2）：各分量取 \(\{0, 1/m, \ldots, 1\}\) 且 \(\sum_{i=1}^{q} x_i = 1\)。

对 m=2，点类型：

1. **纯分量（顶点）**：\(e_i\)（一个分量为 1，其余 0）— 共 q 点  
2. **二元边中点**：对每对 \(i<j\)，\(x_i=x_j=1/2\)，其余 0 — 共 \(\binom{q}{2}\) 点  

总运行数：\(N = q + \binom{q}{2} = q(q+1)/2\)。

| q | N |
|---|---|
| 3 | 6 |
| 4 | 10 |

可选随机化：打乱 RunOrder，种子可复现。

## UI 分页

1. 选项（q、分量名、种子）  
2. 方法说明（lattice 规则）  
3. 设计矩阵预览  
4. 写入确认  

禁止与 Mixture 分析同对话框。

## 明确不做

- extreme-vertices、D-opt、过程变量联合、Scheffé 分析、golden
