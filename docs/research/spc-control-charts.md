# SPC 控制图研究记录

研究/访问日期：2026-08-17（UTC+8）  
范围：I-MR、Xbar-R/S、P/NP/C/U、Laney P′/U′、EWMA、CUSUM，以及 Minitab Tests 1–8。  
本文只记录统计契约和验证样例，不修改 SPC 计划文件或源代码。

## 1. 统一约定

控制图的中心线（CL）表示稳定过程的目标或估计均值，UCL/LCL 是在指定
分布和参数估计下的控制限，不是工程规格限。控制图信号表示“值得调查的
特殊原因候选”，不能单独证明根因。NIST 指出非正态、偏态或离散分布下，
对称的 3-sigma 限可能改变误报概率。

默认用 `k=3`（Test 1 的 sigma 倍数）。若使用历史均值/标准差，应在结果中
标明 `historical`；否则标明估计方法和数据范围。参数估计阶段与监控阶段
必须可区分，排除点不能静默改变原始行映射。

主要假设：

- 变量图：合理的时间顺序、观测近似独立；Xbar 子组是合理子组，组内波动
  代表短期变异，过程在估计阶段应基本稳定。
- P/NP：每个单位只有合格/不合格两类，单位条件近似独立，给定子组的
  不合格概率可用二项分布描述。
- C/U：缺陷计数非负，单位/机会数已定义且可比，计数近似 Poisson；C 图
  要求每个子组机会数相同，U 图允许机会数不同。
- EWMA/CUSUM：数据顺序有意义，目标和 sigma 的来源明确；小偏移检测能力
  依赖参数（`λ` 或 `h,k`）而非仅依赖 3-sigma 规则。

## 2. I-MR（Individuals–Moving Range）

适用于每次只有一个观测、不能组成合理子组的连续数据。令 `x_i` 为个体值：

```text
MR_i = |x_i - x_(i-1)|,                 i = 2..m
MRbar = mean(MR_i)
sigma_hat = MRbar / d2(2) = MRbar / 1.128
I-CL = xbar
I-UCL/LCL = xbar ± 3 sigma_hat
MR-CL = MRbar
MR-UCL = D4(2) MRbar = 3.267 MRbar
MR-LCL = D3(2) MRbar = 0
```

若有历史 sigma，可直接用它；Minitab 还提供 average MR、median MR、
square-root MSSD 和 Nelson estimate。Nelson estimate 会剔除超过平均 MR
约 3 sigma 的异常 MR 后重估，因此结果可能不同于简单 `MRbar/1.128`。
至少要有两个相邻有效观测；缺失值会断开 MR，不应跨缺失行计算差分。
连续强自相关、趋势或测量系统变化会使 MR 估计失真。

Minitab 的 Individuals 与 MR 图通常可使用 Tests 1–8，但 MR 图只支持
Tests 1–4；这是对 MR 统计量的限制，不应把 I 图信号复制到 MR 图。

手算样例：`x = [10, 11, 10, 12, 11]`。`xbar=10.8`，
`MR=[1,1,2,1]`，`MRbar=1.25`，`sigma_hat=1.1082`，所以
I 图约为 `[LCL,UCL]=[7.475,14.125]`，MR 图 UCL 约为 `4.084`。

来源：

- Minitab，Individuals methods and formulas：
  https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/i-mr-chart/methods-and-formulas/methods-and-formulas-for-individuals-chart/
- NIST/SEMATECH，Individuals control charts：
  https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc312.htm

## 3. Xbar-R 与 Xbar-S

令第 `i` 个合理子组含 `n_i` 个观测，均值为 `xbar_i`，范围
`R_i=max-min`，标准差为 `s_i`。

### Xbar-R

```text
xbarbar = mean(xbar_i)
Rbar = mean(R_i)
sigma_within ≈ Rbar / d2(n)                 (等组大小)
Xbar limits = xbarbar ± 3 sigma_within/sqrt(n)
R-CL = Rbar
R-UCL = D4(n) Rbar
R-LCL = max(0, D3(n) Rbar)
```

等价的常数写法是 `Xbar-UCL/LCL=xbarbar ± A2(n)Rbar`。R 图适合较小子组
（Minitab 的实务说明通常为 `n≤8`）；S 图在较大子组（通常 `n≥9`）更有效：

```text
Sbar = mean(s_i)
sigma_within ≈ Sbar/c4(n)                 （使用无偏修正时）
Xbar limits = xbarbar ± 3 sigma_within/sqrt(n)
S-CL = Sbar
S-UCL/LCL = c4(n) sigma_within ± 3*sqrt(1-c4(n)^2) sigma_within
```

更一般的变组大小计算应逐组使用 `n_i` 和对应 `d2/c4`；不能把不同组大小
简单混成一个 Rbar。R/S 图先检查组内变异，再解释 Xbar 图；组内含有不同
来源或非随机样本会破坏合理子组假设。Minitab 支持 Rbar、Sbar 和 pooled
standard deviation，是否使用无偏常数也会造成小样本数值差异。R/S 图只支持
Tests 1–4（Xbar 部分可按相应图设置使用更多测试）。

手算样例（四个 `n=3` 子组）：

```text
[10,11, 9], [12,11,13], [10,10,11], [9,10,8]
xbar_i = 10, 12, 10.3333, 9
R_i    = 2, 2, 1, 2
xbarbar=10.3333, Rbar=1.75
```

用 `d2(3)=1.693`、`A2(3)=1.023`、`D3(3)=0`、`D4(3)=2.574`：
`sigma≈1.0337`，Xbar 限约 `[8.543,12.123]`，R 限 `[0,4.505]`。

来源：

- Minitab，R-chart methods and formulas：
  https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-subgroups/xbar-r-chart/methods-and-formulas/r-chart/
- Minitab，control-chart preferences and estimation methods：
  https://support.minitab.com/en-us/real-time-spc/quality-analyses/control-charts/control-chart-preferences/
- NIST，Xbar-R-S charts：
  https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc311.htm

## 4. P、NP、C、U

设第 `i` 个子组不合格品数为 `d_i`，样本量为 `n_i`；缺陷数为 `c_i`，
检查单位数为 `u_i`。估计值用总数而不是子组比例的非加权平均：

```text
p_i = d_i/n_i                 pbar = sum(d_i)/sum(n_i)
P limits_i = pbar ± 3 sqrt(pbar(1-pbar)/n_i)
NP-CL = n*pbar                NP limits = n*pbar ± 3 sqrt(n*pbar*(1-pbar))

cbar = mean(c_i)              C limits = cbar ± 3 sqrt(cbar)
u_i = c_i/u_i                  ubar = sum(c_i)/sum(u_i)
U limits_i = ubar ± 3 sqrt(ubar/u_i)
```

所有下限取 `max(0,LCL)`。NP 和 C 需要固定子组/机会数；P 与 U 可变大小。
P/NP 是“不合格单位”，C/U 是“一个单位上可有多个不符合项”，不能用缺陷数
代替不合格单位数。小的 Poisson 均值、稀有事件、零计数和强偏态会让正态
近似的对称 3-sigma 限变得粗糙，必要时应提供精确概率限或警告。

手算样例：

```text
P/NP: d=[1,0,2,1], n=[10,10,10,10]
      pbar=0.10; P limits=[0, 0.3846]
      NP-CL=1; NP limits=[0, 3.846]
C:    c=[2,3,1,4], cbar=2.5; limits=[0, 7.243]
U:    c=[2,3,1,4], units=[1,2,1,2]
      ubar=10/6=1.6667
      limits_i = 1.6667 ± 3*sqrt(1.6667/units_i)
```

来源：

- NIST，proportion charts（二项模型）：
  https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc332.htm
- NIST，count charts（Poisson 模型）：
  https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc331.htm
- Minitab，attribute control-chart overview：
  https://support.minitab.com/en-us/connect/dashboards/asset-library/process-control---control-charts/

## 5. Laney P′ 与 U′

传统 P/U 的二项或 Poisson 方差有时不能解释相邻子组之间的额外变异
（overdispersion），或实际变异过小（underdispersion）。Laney 图先把每个
子组标准化为 Z，再用长度为 2 的 moving range 估计 `Sigma Z`：

```text
P′: z_i = (p_i-pbar)/sqrt(pbar(1-pbar)/n_i)
U′: z_i = (u_i-ubar)/sqrt(ubar/u_i)
MRz_i = |z_i-z_(i-1)|
SigmaZ = mean(MRz)/1.128

P′ limits_i = pbar ± 3*SigmaZ*sqrt(pbar(1-pbar)/n_i)
U′ limits_i = ubar ± 3*SigmaZ*sqrt(ubar/u_i)
```

实际实现还应按 Minitab 公式把限截到合法范围：P′ 为 `[0,1]`，U′ 下限
不小于 0。`SigmaZ=1` 时应退化为传统 P/U；大于 1 放宽限，小于 1 收紧限。
子组太少、`pbar` 为 0/1、`ubar=0` 或相邻值缺失时 Z/MR 不可稳定估计，
不能伪造 Sigma Z。Laney 修正不是自动解决自相关、分层或错误分组的方法。

手算样例（P′，固定 `n=100`）：

```text
d=[10,12,8,10], p=[.10,.12,.08,.10], pbar=.10
z=[0, 0.6667, -0.6667, 0]
MRz=[.6667,1.3333,.6667], SigmaZ≈0.7876
```

因此 P′ 的限宽度是传统 P 图宽度的约 `0.7876` 倍，显示了
underdispersion 的收紧效果。真实实现应保留 `SigmaZ`、每个 z、MR 和
截断前后控制限，便于复核。

来源：

- Minitab，Laney P′ methods and formulas：
  https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/attributes-charts/laney-p-chart/methods-and-formulas/methods-and-formulas/
- Minitab，Laney U′ methods and formulas：
  https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/attributes-charts/laney-u-chart/methods-and-formulas/methods-and-formulas/
- Minitab，overdispersion and underdispersion：
  https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/understanding-attributes-control-charts/overdispersion-and-underdispersion/

## 6. EWMA

EWMA 对连续数据的当前子组均值 `xbar_i` 加权，适合发现小而持续的均值偏移：

```text
z_0 = μ
z_i = λ*xbar_i + (1-λ)*z_(i-1),       0 < λ ≤ 1
sd(z_i) = sigma*sqrt[(λ/(2-λ))*(1-(1-λ)^(2i))]
UCL_i/LCL_i = μ ± L*sd(z_i)
```

常数子组大小时可用上述形式；变组大小应使用实际 `n_i` 和 sigma
估计方法。Minitab 默认从总体数据均值初始化 `z_0`，但可提供历史均值；
`sigma` 可由 pooled、Sbar、Rbar、average/median MR 或 square-root MSSD
估计。`λ` 越小越平滑、越偏向早期历史；`L` 常取 3，但应按目标 ARL 选择。
初始控制限随 `i` 变化，不能直接用稳态限替代。

手算样例（`μ=10, sigma=1, λ=.2, L=3, z0=10`）：

```text
x=[10,11,10]
z=[10, 10.2, 10.16]
sd(z1)=sqrt(.2/1.8*(1-.8^2))=.2
sd(z2)=sqrt(.2/1.8*(1-.8^4))=.2939
```

第 1、2 点限约为 `[9.4,10.6]`、`[9.118,10.882]`。Minitab 对 EWMA
只提供 Test 1；Tests 2–8 不应附加到 EWMA。

来源：

- Minitab，EWMA methods and formulas：
  https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/time-weighted-charts/ewma-chart/methods-and-formulas/ewma-chart/
- NIST/SEMATECH，EWMA charts：
  https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc324.htm

## 7. CUSUM

以目标 `T`、过程标准差 `sigma`、允许 slack `k` 和决策间隔 `h` 为参数。
对标准化观测 `y_i=(x_i-T)/sigma` 的双侧 tabular CUSUM：

```text
C_i+ = max(0, C_(i-1)+ + y_i - k)
C_i- = max(0, C_(i-1)- + (-y_i) - k)
signal if C_i+ > h or C_i- > h
```

Minitab 默认 tabular CUSUM，默认 `h=4`、`k=0.5`，另有 V-mask 方案；
`h,k` 应按目标 ARL 选择。FIR（fast initial response）改变初始化，
能更快发现启动时已偏移的过程，因此必须记录是否启用。变组大小时，
Minitab 可按实际或指定的目标子组大小计算限。

手算样例（`T=10,sigma=1,k=.5,h=4`，`x=[10,11,10,12]`）：

```text
y=[0,1,0,2]
C+=[0,.5,0,1.5], C-=[0,0,0,0]
```

未超过 `h`，故无信号；若继续出现 `x=12`，`C+` 会累积并最终越过 `h`。
CUSUM 不支持 Minitab 的 Tests 1–8，CUSUM 信号和 Shewhart 测试必须分开报告。

来源：

- Minitab，CUSUM methods and formulas：
  https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/time-weighted-charts/cusum-chart/methods-and-formulas/methods-and-formulas/
- NIST/SEMATECH，CUSUM charts：
  https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc323.htm

## 8. Minitab Tests 1–8

对绘图统计量 `y_i`、中心线 `CL_i` 和 sigma `sigma_i` 定义分区。严格
“超过”用 `>`；中心线点不属于任一侧；中心线或缺失点会重置同侧运行。
每项测试应保存独立失败点集，而不是只保存一个最终标签：

| 测试 | Minitab 规则 | 典型含义 |
|---|---|---|
| 1 | 1 点超过同侧 3σ | 异常点 |
| 2 | 连续 9 点在中心线同一侧 | 均值偏移 |
| 3 | 连续 6 点全部递增或递减 | 趋势 |
| 4 | 连续 14 点上下交替 | 周期/系统性变化 |
| 5 | 连续 3 点中至少 2 点同侧超过 2σ | 小偏移 |
| 6 | 连续 5 点中至少 4 点同侧超过 1σ | 小偏移 |
| 7 | 连续 15 点在中心线两侧的 1σ 内 | 分层/控制限过宽 |
| 8 | 连续 8 点超过 1σ（任一侧） | 混合/双群 |

“超过 1σ”的 Test 8 不要求交替；8 点全部在同一侧且都超过 1σ 也应
触发。Test 4 的“交替”是数值上升/下降交替，平坦点不能当作上升或下降。
控制限随子组大小变化时，分区必须逐点使用 `sigma_i`。多重测试会提高
误报概率，启用项和 K 值必须回显。

可复核的最小数据（假定 `CL=0,sigma=1`；每行只验证指定测试）：

```text
T1: [0,0,0, 3.1]                         -> 点4
T2: [1,1,1,1,1,1,1,1,1]                 -> 点1..9
T3: [0,1,2,3,4,5]                        -> 点1..6（递增）
T4: [1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1] -> 点1..14
T5: [2.1,2.2,0]                          -> 点1..3（2/3）
T6: [1.1,1.2,1.3,1.4,0]                  -> 点1..5（4/5）
T7: [0.1,-0.2,0.3,-0.4,0.0,0.2,-0.1,0.4,-0.3,0.1,-0.2,0.3,-0.4,0.2,0.0]
                                             -> 点1..15
T8: [1.1,1.2,1.3,1.4,1.5,1.6,1.7,1.8] -> 点1..8（不需要交替）
```

Minitab 支持范围差异（访问日期 2026-08-17）：

- I、Xbar 等多数变量图可用 Tests 1–8，Minitab 默认只启用 Test 1。
- R、S、MR 只支持 Tests 1–4。
- P/NP/C/U 和 Laney P′/U′ 在 Minitab 中通常只开放 Tests 1–4。
- EWMA 只支持 Test 1；CUSUM 不支持 Tests 1–8，使用上/下侧累计和决策间隔。

DataLab 项目策略与 Minitab 默认不同，必须在方法参数表中写明：

- 新分析默认勾选**当前图全部适用规则**，而不是只启用 Test 1。
- I、Xbar、P/NP/C/U、Laney：适用 Tests 1–8。
- MR、R、S：适用 Tests 1–4；勾选 5–8 时忽略并给出“规则不适用”诊断。
- EWMA：只运行 Test 1。
- CUSUM：不套用 Tests 1–8，报告首次上侧/下侧信号。
- 旧 JSON 若显式保存 `enabled_special_cause_tests: [1]`，保持只运行 Test 1，不改写为全选。

判定边界：

- “超过 kσ”使用严格大于 `>`；恰好等于 kσ 不触发 Test 1/5/6/8。
- Test 7 的“1σ 内”使用 `|y-CL| ≤ σ`。
- 落在中心线上的点不属于任一侧，会打断 Test 2 的同侧连续段。
- Test 3 要求相邻点严格单调；相等点打断趋势。
- Test 4 是相邻数值上升/下降交替，不是中心线两侧交替；零差分打断模式。
- Test 8 允许 8 点全部在同一侧，只要都在 1σ 外。
- 阶段标签变化、非有限值会打断窗口，不跨断点计算。
- 分区 sigma 使用 `(UCL-CL)/3`，避免 LCL 截断为 0 时把 `(UCL-LCL)/6` 当成 sigma。

来源：

- Minitab，tests for special causes：
  https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/basics/using-tests-for-special-causes/
- Minitab，I-MR 选择测试：
  https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/i-mr-chart/perform-the-analysis/i-mr-options/select-tests-for-special-causes/
- Minitab，R 图选择测试：
  https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-subgroups/r-chart/perform-the-analysis/r-options/select-tests-for-special-causes/
- Minitab，chart availability/defaults：
  https://support.minitab.com/en-us/minitab/help-and-how-to/minitab-environment/settings-and-defaults/control-charts-and-quality-tools/tests/
- NIST/SEMATECH，variables chart run/zone rules（WECO 子集）：
  https://www.itl.nist.gov/div898/handbook/pmc/section3/pmc32.htm

NIST 的 WECO 说明主要覆盖 1 点超 3σ、3 点中 2 点超 2σ、5 点中 4 点超
1σ、8 点同侧；这对应 Minitab Tests 1、5、6、2 的常见子集，不等于完整
Nelson 1–8。实现和报告中应明确“使用的规则集合”，不能把 NIST WECO
子集误称为完整 Tests 1–8。

## 9. 实现验收要点

每个分析结果至少保留：原始行号、绘图点号、绘图值、CL、逐点 sigma、
LCL/UCL、估计方法、阶段、有效样本量、缺失/断点、启用测试、每项失败点集。
边界值（恰好等于 1σ/2σ/3σ）、零下限、可变子组大小、零计数、缺失值、
历史参数、Laney `SigmaZ`、EWMA 初始点、CUSUM FIR 都应有独立测试。

