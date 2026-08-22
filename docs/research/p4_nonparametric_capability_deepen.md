# P4：Nonparametric Capability 深化（直方图 + Observed Performance）

> 研究日期：2026-08-22 · 访问 2026-08-22（UTC+8）  
> Wave-4 W4-1；在 Wave-2 `nonparametric_capability` 域骨架上补 Minitab 表形缺口；`formula_reference ≠ golden`。

## 锁定

| 命令 | 交付 |
|---|---|
| `nonparametric_capability` **深化** | Overall Capability（Cnp/Cnpl/Cnpu/Cnpk）+ **Capability Histogram** + **Observed Performance**（PPM &lt; LSL、&gt; USL、Total）；扩展 `NonparametricCapabilityFacts` |

## Primary Sources

| URL | 访问 |
|---|---|
| https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/nonparametric-capability-analysis/methods-and-formulas/overall-capability/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/nonparametric-capability-analysis/interpret-the-results/all-statistics-and-graphs/capability-histogram/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/nonparametric-capability-analysis/interpret-the-results/all-statistics-and-graphs/observed-performance/ | 2026-08-22 |
| https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis/how-to/capability-analysis/nonparametric-capability-analysis/interpret-the-results/key-results/ | 2026-08-22 |

## Minitab 表形参考（非 golden）

### Overall Capability

| 统计量 | 列名 / 含义 |
|---|---|
| Cnp | (USL − LSL) / (Xpu − Xpl)；仅 spread |
| Cnpl | (η − LSL) / (η − Xpl) |
| Cnpu | (USL − η) / (Xpu − η) |
| Cnpk | min{Cnpl, Cnpu} |
| η | 过程中位数 |
| Xpl / Xpu | 容差 T 对应经验分位数（默认 T=6 → pL≈0.00135, pU≈0.99865） |

### Capability Histogram（图）

- 直方图：样本分布相对 LSL/USL；超规格区间以红色/高亮条标识（Minitab 视觉语义）。
- 轴：测量值；可选 Target 竖线（若 `specifications.target` 有值）。
- 图题：**Capability Histogram**（或中文等价 catalog 键）。

### Observed Performance（表）

| 行 / 列 | 含义 |
|---|---|
| PPM &lt; LSL | 样本中 &lt; LSL 的百万分之不合格率 |
| PPM &gt; USL | 样本中 &gt; USL 的百万分之不合格率 |
| PPM Total | PPM &lt; LSL + PPM &gt; USL |
| % &lt; LSL / % &gt; USL / % Total | 同上，百分比形式（Minitab 同页；Wave-4 **可选**导出 % 列，PPM 三列为必交付） |

## DataLab 交付范围

- **Domain 已有**（`nonparametric_capability.cpp`）：Cnp/Cnpl/Cnpu/Cnpk、经验分位数、PPM 三列、`source_rows`、N≥10 与 LSL+USL 门禁。
- **本 Wave 补全**：
  - **Facts 扩展**：`NonparametricCapabilityFacts` 补齐 cnpl/cnpu、Xpl/Xpu、tolerance_k、observed_ppm_*、histogram_bin_edges/counts（或等效 chart payload 字段）。
  - **GraphService**：Capability Histogram（LSL/USL 参考线、超规格 bin 高亮）。
  - **Output 表**：Overall Capability + Observed Performance 独立表块（catalog 双语表题）。
  - **Interpretation**：强调 Cnpk 仅反映较差一侧；PPM Total 为样本观测不合格率，禁止「过程合格 / 批次合格」。
  - **algorithm_help.json**：公式块 + Primary URL（Overall + Histogram + Observed Performance）。
- **契约**：complete-case；`source_row` 可审计；`hidden_rows` ≠ excluded。

## 公式（# source: formula_reference）

**容差分位点：**

\[
p_U = P(Z < T/2),\quad p_L = P(Z < -T/2)
\]

其中 T 为容差（默认 6），Z 为标准正态。

**经验分位数（Minitab 线性插值）：**

\[
w = p(N+1),\quad y = \lfloor w \rfloor,\quad z = w - y
\]

\[
X_p = (1-z)\,X_y + z\,X_{y+1}
\]

**Overall Capability：**

\[
Cnp = \frac{USL - LSL}{X_{pu} - X_{pl}},\quad
Cnpl = \frac{\eta - LSL}{\eta - X_{pl}},\quad
Cnpu = \frac{USL - \eta}{X_{pu} - \eta},\quad
Cnpk = \min(Cnpl, Cnpu)
\]

其中 η 为样本中位数。

**Observed Performance（PPM）：**

\[
\text{PPM} < LSL = \frac{1000000}{N}\sum \mathbf{1}(x_i < LSL),\quad
\text{PPM} > USL = \frac{1000000}{N}\sum \mathbf{1}(x_i > USL)
\]

\[
\text{PPM Total} = \text{PPM} < LSL + \text{PPM} > USL
\]

## 测试要求

| 层级 | 要求 |
|---|---|
| **Domain** | `# source: formula_reference`：已知小样本手算 Cnp/Cnpk 与 PPM；N&lt;10 / 缺 LSL 或 USL 报错 |
| **Service** | complete-case N；至少 1 条 **真·A→B**（换表 + `excluded_rows` 不继承 → N/PPM 变化） |
| **Serialize** | `nonparametric_capability` Facts 新字段 round-trip |
| **Graph** | histogram 含 LSL/USL；超规格 bin 计数与 PPM 一致 |
| **Interpret** | grep 禁语：过程合格、已证明稳定、批次合格 |

## 明确不做（延后）

- Automated Capability / 正态或 Johnson 变换能力路径。
- Expected Performance（基于拟合分布的 PPM）；仅交付 **Observed** PPM。
- 子组间 / 阶段间能力；Batch capability 另命令。
- Minitab 数值 golden；Johnson / Box-Cox 联动能力表。
- 单页 UI 堆叠多主流程；直方图选项独立区或沿用现有能力页二级区。
