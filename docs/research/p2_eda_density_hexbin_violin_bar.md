# Track C1–C4：密度图 / Hexbin / Violin / 通用条形图

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。

## 0. 锁定与禁止偷懒

| ID | 命令 | 交付 |
|---|---|---|
| C1 | `density_plot` | 高斯 KDE + Silverman h；面积曲线；`EdaPlotFacts` |
| C2 | `hexbin_plot` | 矩形二维分箱计数着色（Binned Scatter / hexbin 产品名）；坐标轴为数值范围 |
| C3 | `violin_plot` | 分组镜像 KDE + 箱线五数；可选无分组=单小提琴 |
| C4 | `bar_chart` | 分类计数条形；**不**按频数排序、**无**累积％（与 `pareto` 分流） |

**禁止偷懒：**

- 禁止只加 ChartKind 不接线 GraphService + renderer + adapter  
- 禁止用柏拉图冒充通用条形（排序+累积线）  
- 禁止密度图无带宽说明  
- 禁止 hexbin 无 complete-case / source_row  
- 禁止 Graph Builder 拖拽 / 可旋转 3D  
- 禁止假 golden；解释不写「分布已正态 / 已证明」  

---

## 1. C1 密度 / KDE

| 来源 | URL | 访问 |
|---|---|---|
| NIST Dataplot Kernel Density Width | https://www.itl.nist.gov/div898/software/dataplot/refman1/auxillar/kernwidt.htm | 2026-08-21 |
| NIST Kernel Density Plot | https://www.itl.nist.gov/div898/software/dataplot/refman1/auxillar/kernplot.htm | 2026-08-21 |

\[
\hat f(x)=\frac{1}{nh}\sum_{i=1}^{n} K\!\left(\frac{x-X_i}{h}\right),\quad
K(u)=\frac{1}{\sqrt{2\pi}}e^{-u^2/2}
\]

\[
h=0.9\,A\,n^{-1/5},\quad A=\min\!\left(s,\frac{\mathrm{IQR}}{1.34}\right)
\]

网格：\([\min-3h,\max+3h]\)，默认 128 点。可选覆盖直方图（本轮可仅密度曲线）。

---

## 2. C2 Hexbin / 二维分箱

Minitab Binned Scatter 对应：把 (x,y) complete-case 落入矩形格，单元格填计数，颜色映射计数。

本产品：**矩形格**（非正六边形镶嵌）；命令名 `hexbin_plot`，诊断写明「矩形二维分箱（Binned Scatter）」。格数默认按 √n 钳制到 [8,40]。

---

## 3. C3 Violin

每组：Gaussian KDE（同 C1）镜像画在类别轴两侧；叠加箱线（须/Q1/中位/Q3）。无分组时单列全体。

---

## 4. C4 通用条形

类别计数（或可选权重列求和）。**保持输入/字典序**，不按计数降序；无 Cum%。与 `pareto` 分流。

---

## 5. 接线

- `PlotKind`/`ChartKind`：`density` / `hexbin` / `violin` / `bar`  
- `GraphService::{density,hexbin,violin,bar}`  
- `EdaPlotFacts`：kind、n、bandwidth、bin_rows/cols、category_count  
- 测试 `# source: formula_reference`  
- help + brief §5r + backlog/roadmap/acceptance/wiring/gap-matrix  

## 6. 手工验收

见 brief §5r。
