# P2：DOE 设计生成 · ACF/PACF · 等价/DOE/容差功效

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> `formula_reference ≠ golden`。未从 Minitab 导出的数值不得写入 `VALIDATION_MATRIX.md`。

## 0. 本轮锁定与禁止偷懒

**做（竖切闭环）：**

| ID | 命令 / 深化 | 交付 |
|---|---|---|
| A1 | `doe_factorial` 设计生成加深 | 2^k 全因子（已有）+ 2^(k-p) 部分析因；生成器/定义关系/别名表；`DoeFacts`；可与 `doe_response` 衔接 |
| A3 | `acf_pacf` | ACF + PACF 表与图；默认置信带说明进诊断/帮助；`AcfPacfFacts` |
| A4 | `t_power` 模式扩展 | `equivalence_*`、`doe_factorial_*`、`tolerance_*`；`PowerFacts` + 功效曲线 |

**禁止偷懒：**

- 禁止只做菜单占位无设计矩阵 / 无生成器与别名表  
- 禁止 ACF 图无置信带说明  
- 禁止功效页无 `PowerFacts` 与曲线  
- 禁止未更新 backlog/roadmap/acceptance/gap-matrix 就标 ✅  
- 禁止把 PB/DSD/RSM/Mixture 冒充本轮交付（PB/DSD 延后；RSM 下一批）  
- 禁止假 Minitab golden；禁止解释写「已证明稳定 / 过程失控」类禁用子串  

---

## 1. A1 DOE 设计生成（全因子 + 部分析因）

| 来源 | URL | 访问 |
|---|---|---|
| Minitab：What is a design generator? | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/supporting-topics/factorial-and-screening-designs/what-is-a-design-generator/ | 2026-08-21 |
| Minitab：Defining relation | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/supporting-topics/factorial-and-screening-designs/what-is-a-defining-relation/ | 2026-08-21 |
| Minitab：Create 2-Level Factorial (Specify Generators) statistics | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/doe/how-to/factorial/create-factorial-design/create-2-level-factorial-specify-generators/examine-the-design/all-statistics/ | 2026-08-21 |
| PSU STAT 503 Lesson 8（教材对照） | https://online.stat.psu.edu/stat503/lesson/8 | 2026-08-21 |

**产品锁定：**

- 全因子：沿用 `generate_2_level_factorial`（`fraction_p=0`）。  
- 部分析因 2^(k-p)：先生成基设计 2^(k-p)（前 k-p 个因子全因子），再按生成器把后 p 个因子写成基因子乘积（编码 ±1 乘法）。  
- 默认生成器：取常见最高分辨度表（与 Minitab「默认生成器」同类；本轮实现有限 (k,p) 表，超出则诊断并要求手写生成器）。  
- 手写生成器语法：`D=ABC;E=ABD`（字母按因子顺序 A,B,C,… 映射）。  
- 输出表：设计矩阵；设计信息（k、p、运行数、分辨度）；生成器；定义关系；别名结构（主效应与二阶交互，至多列出定义关系诱导的别名行）。  
- `DoeFacts` 增补：`design_kind`（`full`/`fractional`）、`fraction_p`、`resolution`、`run_count`、`generator_text`。  
- 中心点 / 随机化 / 区组：与现有全因子路径一致。  
- **与响应分析衔接**：用户将设计矩阵写入工作表后，仍用现有 `doe_response` / `doe_factorial`（选响应+因子列）分析；本轮不自动写回工作表。  
- **不做：** Plackett–Burman、DSD、RSM 设计生成、折叠、自定义区组生成器 UI。

**分辨度：** 定义关系中最短非 I 字长；罗马数字 III/IV/V/…。

---

## 2. A3 ACF / PACF

| 来源 | URL | 访问 |
|---|---|---|
| NIST EDA：Autocorrelation Plot | https://www.itl.nist.gov/div898/handbook/eda/section3/autocopl.htm | 2026-08-21 |
| Minitab：Autocorrelation methods | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/time-series/how-to/autocorrelation/methods-and-formulas/methods-and-formulas/ | 2026-08-21 |
| Minitab：ARIMA residual ACF/PACF SE | https://support.minitab.com/en-us/minitab/help-and-how-to/statistical-modeling/time-series/supporting-topics/diagnostic-checking/diagnostic-checking-for-arima/ | 2026-08-21 |

**产品锁定：**

- 命令 `acf_pacf`；单列数值；complete-case；保留 `source_row` 仅用于有效序列索引诊断（图用 lag 轴）。  
- ACF（含 lag 0 = 1）：  
  \[
  r_k=\frac{\sum_{t=1}^{n-k}(x_t-\bar x)(x_{t+k}-\bar x)}{\sum_{t=1}^{n}(x_t-\bar x)^2}
  \]  
- PACF：Durbin–Levinson；输出 φ_{kk}。  
- **默认置信带（随机性 / 白噪声零假设，NIST 固定带宽）：**  
  \[
  \pm z_{1-\alpha/2}/\sqrt{n}\quad(\alpha=0.05\Rightarrow\approx\pm 1.96/\sqrt{n})
  \]  
  诊断与帮助写明：此为**独立性检验**带宽；ARIMA 识别用的 Bartlett/MA 变带宽为本轮可选说明，默认不画变带宽（可在诊断提示）。  
- PACF 显著性限：同 \(1.96/\sqrt{n}\)（与常见教材/Minitab 残差 PACF 口径一致的简化）。  
- 可选 Ljung–Box Q（至 max lag）写入表或诊断；不做 ADF。  
- 默认 max lag：\(\min(40,\lfloor n/4\rfloor)\)，可输入覆盖。  
- 图：ACF、PACF 各一张（`PlotKind::time_series` 或等价茎状：x=lag，y=相关；两条水平参考为置信限）。  
- `AcfPacfFacts`：`n`、`max_lag`、`confidence_band_method`=`white_noise_fixed`、`band_half_width`、`ljung_box_available`。  
- **不做：** CCF、谱密度、自动写回 ARIMA 阶。

---

## 3. A4 功效扩展（等价 / DOE / 容差）

| 来源 | URL | 访问 |
|---|---|---|
| Minitab：Power analyses list | https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/power-and-sample-size/supporting-topics/power-and-sample-size-analyses-in-minitab/ | 2026-08-21 |
| Minitab：Power for equivalence tests | https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/equivalence-tests/supporting-topics/power-for-equivalence-tests/ | 2026-08-21 |
| Minitab：2-Level Factorial Design power overview | https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/power-and-sample-size/how-to/linear-models/power-and-sample-size-for-2-level-factorial-design/before-you-start/overview/ | 2026-08-21 |
| Minitab：Sample Size for Tolerance Intervals overview | https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/power-and-sample-size/how-to/sample-size/sample-size-for-tolerance-intervals/before-you-start/overview/ | 2026-08-21 |

**产品锁定（扩展现有 `t_power` 模式字符串，不新开菜单除非必要）：**

### 3.1 等价 TOST 功效（均值差）

- 模式：`equivalence_one_sample_power` / `_sample_size`，`equivalence_two_sample_power` / `_sample_size`。  
- 输入：`effect` = 真实差值/σ（或两样本差值/σ）；`null_proportion`/`second_proportion` 复用为下/上等价界（相对 σ 的编码界，或绝对界/σ——锁定：**界与效应均以 σ 单位**，即 δ_L、δ_U、θ 已除以 σ）。  
- 默认对称：若只给一个 `effect_size` 作半宽 δ，则 δ_L=-δ、δ_U=+δ，真实差默认 0（可用 effect_list 扫真实差）。  
- 计算：已知 σ 规划口径下用正态近似  
  \[
  \mathrm{Power}=\Phi\!\left(\frac{\delta_U-\theta}{\mathrm{SE}}-z_\alpha\right)
  -\Phi\!\left(\frac{\delta_L-\theta}{\mathrm{SE}}+z_\alpha\right)
  \]  
  单样本 SE=1/√n；两样本 SE=√(2/n_per)。样本量为保证 Power≥target 的最小 n。  
- 诊断：标明「规划用 σ 已知正态近似；非 Minitab 精确非中心 t 二元积分」。  
- **不做：** 2×2 crossover 等价功效；比例等价功效本轮可不做。

### 3.2 2 水平析因功效

- 模式：`doe_factorial_power` / `doe_factorial_sample_size`（后者求 replicates）。  
- 输入：`groups`→因子数 k；`observation_length`→fraction p（整数）；`sample_size`→replicates（算功效时）；`effect`→|效应|/σ；中心点数用 `center` 可选（本轮可用 power 配置新字段或忽略中心点对 df 的精细项，锁定：**功效按析因点对比，中心点不计入效应对比 n**）。  
- 每水平对比样本量：n₊ = n₋ = r · 2^(k-p-1)；总析因运行 N = r · 2^(k-p)。  
- 用现有 `two_sample_t_power` 核（效应量 = |effect|/(√2) 调整到与两样本均值差定义一致——锁定：把 DOE 效应 E=ȳ₊-ȳ₋ 直接作为 two-sample effect_size，n_per = r·2^(k-p-1)）。  
- **不做：** PB 功效、一般全因子多水平。

### 3.3 容差区间样本量

- 模式：`tolerance_normal_sample_size`（主路径）；可选 `tolerance_normal_power` 不单独做，只做样本量。  
- 输入：`target`→置信度 γ（默认 0.95）；`effect_size`→覆盖比例 P（默认 0.95）；`null_proportion`→最大可接受总体外侧比例上限的规划（本轮简化：求使 Howe/Natrella 双侧正态容差在给定 P、γ 下 k 可算的最小 n；用现有容差 k 公式反解迭代 n）。  
- 复用领域容差区间 k 计算；输出最小 n 与对应 k。  
- **不做：** 非参数容差样本量（可诊断引导）。

**PowerFacts：** 继续写入 `mode` / `power` / `sample_size` / `actual_power` / `effect_size` / `target`；曲线逻辑与现有 `t_power` 一致。

---

## 4. 测试（`# source: formula_reference`）

1. **部分析因 2^(4-1)、D=ABC：** 8 个析因点；分辨度 IV；D 列 = A*B*C。  
2. **ACF：** 白噪声短序列 → |r_k| 多数在 ±1.96/√n 内；常数列 → r_k=0（k>0）或未定义诊断。  
3. **PACF：** 严格 AR(1) 型短序列 → lag1 PACF 明显非零（定性）。  
4. **等价功效：** θ=0、对称 δ、增大 n → power 单调不减。  
5. **DOE 功效：** 固定 effect，增加 replicates → power 上升。  
6. **容差 n：** P=0.95, γ=0.95 → n 为有限正整数且随更严覆盖增大。

## 5. 手工验收（Qt Creator）

见本轮 brief §5p 与 `quality-algorithms-acceptance.md` 新增行。
