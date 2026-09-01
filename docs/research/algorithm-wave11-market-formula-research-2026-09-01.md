# 算法 Wave-11 市场对照与公式入口（2026-09-01）

> 访问日期：2026-09-01（UTC+8）  
> 用途：给 **算法 Wave-11 `/goal`** 的 Primary URL、公式口径、Minitab 表形参考；**本文件不写代码、不填 golden**。  
> 状态权威：[`minitab-market-algorithm-backlog.md`](minitab-market-algorithm-backlog.md)  
> 执行计划：[`goal-wave-2026-09-01-algorithm-wave11-plan-and-mega-prompt.md`](goal-wave-2026-09-01-algorithm-wave11-plan-and-mega-prompt.md)  
> 前置水位：Wave-10 ✅（`general_manova` / `mixed_effects_reml` / `binary_doe_probit` / `life_data_lognormal`）

---

## §0 一句话水位（给新对话）

| 水位 | 状态 |
|------|------|
| P0–P2 + Wave-2～10 | ✅ / ⚪（勿重做） |
| Wave-10 | General MANOVA · Mixed REML · Binary Probit DOE · Life Lognormal ✅ |
| 对应分析（Simple / Multiple） | ❌ |
| 非线性回归 | ❌ |
| 裂区 **设计生成**（W9 仅有 `split_plot_analyze`） | ❌ |
| **本 Wave 锁定** | **4 项竖切**：Simple CA · Multiple CA · Nonlinear Regression · Split-Plot Design |

**「实现所有算法」在本产品的正确含义：**  
按 backlog 产品范围内 ❌ **多 Wave 滚动清空**，**不是**单次 Goal 克隆 Minitab Feature List 100%。本 Goal 锁定 **4 项完整竖切**；其余进 §4 候补队列供 Wave-12+。

---

## §1 网上调研摘要（2026-09-01 Primary URL 再访）

Primary：[Minitab Feature List](https://www.minitab.com/en-us/products/minitab/features/)（访问 2026-09-01）

### 1.1 Simple Correspondence Analysis（SCA）

| 要点 | 来源 |
|------|------|
| 菜单：Stat > Multivariate > **Simple Correspondence Analysis** | [Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/simple-correspondence-analysis/before-you-start/overview/) |
| 对 **二维列联表** 做加权 PCA；惯性 = Pearson χ²/n，非总方差 | [Method](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/simple-correspondence-analysis/methods-and-formulas/method/) |
| 维数 d = min(r−1, c−1)；行/列 profile 与主坐标、标准坐标 | 同上 |
| 输出：Summary of Analysis（惯性分解）；Row/Column Contributions（Qual, Mass, Inert, Coord, Contr）；Row/Column Plot | [Example](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/simple-correspondence-analysis/before-you-start/example/) |

**DataLab 窄化：** 两列 **分类** 变量（或用户指定行/列标签 + 频数列）；complete-case；2 个主成分图窄化；不做 Burt 表、不做 3 维以上交互导出。

### 1.2 Multiple Correspondence Analysis（MCA）

| 要点 | 来源 |
|------|------|
| 菜单：Stat > Multivariate > **Multiple Correspondence Analysis** | [Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/multiple-correspondence-analysis/before-you-start/overview/) |
| 对 **3 个及以上** 分类变量的指示矩阵做加权 PCA；仅 **列贡献**（无行 profile 图） | [Method](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/multiple-correspondence-analysis/methods-and-formulas/method/) |
| 输入：原始分类列 **或** 0/1 指示列；组件数 1～变量数 | [Enter data](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/multiple-correspondence-analysis/perform-the-analysis/enter-your-data/) |
| 输出：Summary of Analysis；Column Contributions（Qual, Mass, Inert, Coord×k, Contr×k）；Column Plot | [Example](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/multiple-correspondence-analysis/before-you-start/example/) |

**DataLab 窄化：** **原始数据** 3～6 列分类；自动展开指示矩阵；组件数默认 min(2, 变量数)；**不做** 指示矩阵手工输入模式（可 Phase-2）。

### 1.3 Nonlinear Regression

| 要点 | 来源 |
|------|------|
| 菜单：Stat > Regression > **Nonlinear Regression** | [Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/nonlinear-regression/before-you-start/overview/) |
| 算法：**Gauss-Newton** 或 **Levenberg-Marquardt**；最小化 SSE；相对 offset 收敛 | [Methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/nonlinear-regression/methods-and-formulas/methods/) |
| 输出：Method；Starting Values；Parameter Estimates（Estimate, SE, CI）；Summary of Fit（SSE, DF, MSE, S）；可选相关矩阵 | [Results](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/nonlinear-regression/perform-the-analysis/select-the-results-to-display/) |
| 用户指定期望函数 + 初值；可约束参数 | [Parameter estimates](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/nonlinear-regression/interpret-the-results/all-statistics-and-graphs/regression-equation-and-parameter-estimates/) |

**DataLab 窄化：** **单响应 + 单预测变量**；内置 **3～5 个命名模型**（如 Growth、Decay、Logistic 饱和、Michaelis-Menten、幂律）；Gauss-Newton 默认 + LM 可选；用户编辑初值；**不做** 任意表达式解析器全量、不做预测子对话框全量。

### 1.4 Create 2-Level Split-Plot Design

| 要点 | 来源 |
|------|------|
| 菜单：Stat > DOE > Factorial > Create Factorial Design > **2-level split-plot** | [Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/create-2-level-split-plot/before-you-start/overview/) |
| 难改因子（HTC）在 whole plot 内固定；易改因子（ETC）在 subplot 变化；最多 7 因子窄化 | 同上 |
| 设计表：Standard Order, Run Order, Whole Plots, Point Type；Summary + Alias + Design table | [Specify design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/create-2-level-split-plot/create-the-design/specify-the-design/) |
| 分析用 W9 `split_plot_analyze`（WP/SP 双误差） | [Example](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/create-2-level-split-plot/before-you-start/example/) |

**DataLab 窄化：** **2～4 因子**（其中 1 个 HTC）；全因子或固定分辨率表；whole-plot 复制 1～2；输出设计矩阵写入 worksheet 契约（与 `doe_factorial` 一致）；**不做** Define Custom Split-Plot、不做 7 因子全表。

### 1.5 开源 / 教科书补充（非抄代码）

| 来源 | 可吸收点 |
|------|----------|
| Greenacre, *Correspondence Analysis in Practice* | 惯性解释、双标图读法 |
| Bates & Watts, *Nonlinear Regression* | GN/LM 实现与收敛判据 |
| Box, Hunter & Hunter, *Statistics for Experimenters* | 裂区 randomization 结构 |
| NIST/SEMATECH e-Handbook | 非线性模型示例与初值策略 |

**禁止：** sklearn/R/Python 进 dist；Minitab 数值 golden；AGPL 整库并入。

---

## §2 Wave-11 锁定项（W11-1～W11-4）

### W11-1 `simple_correspondence` — 简单对应分析（窄化）

| 字段 | 内容 |
|------|------|
| **动机** | backlog §4 多元 / §12 Track H；Wave-10 §4 候补 #1 |
| **窄化** | 2 列分类 → 列联表；1～2 维；行/列贡献表 + 双标图；**独立**命令 ≠ MCA |
| **Primary URL** | [SCA Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/simple-correspondence-analysis/before-you-start/overview/)（2026-09-01）；[Method](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/simple-correspondence-analysis/methods-and-formulas/method/) |
| **公式** | χ² 惯性；\( D_r^{-1/2} A D_c^{-1/2} \) 的 SVD；主坐标 = 奇异值 × 左/右奇异向量 |
| **UI** | **页1** 行变量+列变量；**页2** 组件数+绘图选项；**页3** 输出表选项；**页4** 预览 |

### W11-2 `multiple_correspondence` — 多重对应分析（窄化）

| 字段 | 内容 |
|------|------|
| **动机** | 同上；与 SCA 分命令 |
| **窄化** | 3～6 列分类原始数据；指示矩阵；1～2 组件默认；Column Contributions + Column plot |
| **Primary URL** | [MCA Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/multiple-correspondence-analysis/before-you-start/overview/)（2026-09-01）；[Method](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/multivariate/how-to/multiple-correspondence-analysis/methods-and-formulas/method/) |
| **公式** | 指示矩阵 \( Z \)；\( B = Z'Z/n \) 或 weighted PCA on \( Z \)；列坐标与 Qual/Mass |
| **UI** | **页1** 选 3～6 分类列；**页2** 组件数；**页3** 输出表选项；**页4** 预览 |

### W11-3 `nonlinear_regression` — 非线性回归（窄化）

| 字段 | 内容 |
|------|------|
| **动机** | backlog §4 Nonlinear ❌ |
| **窄化** | 单 Y + 单 X；内置模型库 + 初值表；GN/LM；Parameter + Summary of Fit |
| **Primary URL** | [NL Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/nonlinear-regression/before-you-start/overview/)（2026-09-01）；[Methods](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/regression/how-to/nonlinear-regression/methods-and-formulas/methods/) |
| **公式** | \( \min_\theta \sum (y_i - f(x_i;\theta))^2 \)；GN：\( \theta_{k+1} = \theta_k + (J'J)^{-1}J'r \)；LM 阻尼 |
| **UI** | **页1** 响应+预测；**页2** 选模型+参数初值；**页3** 算法+收敛；**页4** 预览 |

### W11-4 `split_plot_design` — 2 水平裂区设计生成（窄化）

| 字段 | 内容 |
|------|------|
| **动机** | W9 `split_plot_analyze` 无设计生成；backlog §12 RSM/Split-plot 🟡 |
| **窄化** | 2～4 因子、1 HTC；全因子或 catalog 分辨率；复制与 Whole plot 列 |
| **Primary URL** | [Split-plot Overview](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/create-2-level-split-plot/before-you-start/overview/)（2026-09-01）；[Specify design](https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/create-2-level-split-plot/create-the-design/specify-the-design/) |
| **邻域** | `split_plot_analyze`（W9）、`doe_factorial`、`doe_ccd` |
| **UI** | **页1** 因子数+名称；**页2** HTC 指定+设计表选择；**页3** 复制/随机化；**页4** 预览矩阵 |

### 备选替换（仅 Planner 证明原项不可行；仍须满 4 项）

| 备选 id | 场景 |
|---------|------|
| `pls_regression` | 非线性被阻塞；Track H |
| `mixture_extreme_vertices` | 设计类被阻塞 |
| `nested_anova` | 与 split-plot 冲突时 |
| `parallel_plot` | **须另开 Track G Goal** |

---

## §3 导入 / 数据衔接硬约束

1. **complete-case**：无效行不进计算；诊断说明排除原因。  
2. **`source_row`**：拟合/残差/运行表可回溯（非线性、设计生成写入行号）。  
3. **A→B**：换文件后旧排除行不串到新文件。  
4. **SCA**：两列均分类；空单元格按 0 频数或门禁拒绝。  
5. **MCA**：每列至少 2 水平；组件数 ≤ 变量数。  
6. **Nonlinear**：Y/X 数值；初值非法或 SSE 不下降 → diagnostic。  
7. **Split-plot design**：因子水平仅 −1/+1 编码；输出列契约与既有 DOE 一致。

---

## §4 Wave-12+ 候补队列（本 Goal 不实现）

1. PLS regression（NIPALS + 交叉验证）  
2. Mixture extreme-vertices 设计  
3. Mixed REML **扩展**（多随机项 / 嵌套全量）  
4. Nonlinear 多预测 + 自定义表达式  
5. Cox regression 全量  
6. Definitive Screening Designs  
7. Parallel / Bubble / Mosaic（Track G 独占 Wave）  
8. Graph Builder（G3 独占 Wave）

---

## §5 调研检查清单（Planner 开工前）

- [ ] Feature List 本波相关行已对照  
- [ ] 每项 ≥1 Primary URL + 访问日期写入 `p11_*.md`  
- [ ] command_id 在 `analysis_commands.cpp` 无冲突  
- [ ] Menu IA：`menu_path` / `menu_group` 已规划  
- [ ] UI 线框：每命令 ≥4 页（层次分离）  
- [ ] 导入影响已评估  
- [ ] SCA ≠ MCA 对话框；Nonlinear ≠ `linear_regression`；Split design ≠ `split_plot_analyze` 对话框  

---

**文档状态：** 2026-09-01 初稿；供 Wave-11 `/goal` 锁定。
