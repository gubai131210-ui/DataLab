# ADR 0008：正态能力指数置信区间采用 χ² 尺度与 Bissell 近似

- 状态：已接受
- 日期：2026-08-20

## 背景

正态过程能力已输出 Cp/Cpk/Pp/Ppk 点估计。质量工程师需要区间以表达抽样不确定性。候选包括精确分布、Bissell (1990) 正态近似、以及 Kushler–Hurwitz (1992) 比较过的其它界限。

## 决策

1. **Cp / Pp**：χ² 尺度区间，自由度按 σ 估计方法（个体默认 \(N-1\)），与 Minitab Potential/Overall capability 方法页一致。
2. **Cpk / Ppk**：Bissell 标准误 \(\sqrt{1/(9N)+C^2/(2\nu)}\) 配 \(z_{1-\alpha/2}\)。
3. **不实现** Kushler–Hurwitz 文中的其它界限、Zhang–Stenback–Wardrop 精确矩法。
4. 单侧规格缺失侧保持 `*`。Johnson/非正态不编造 Within CI。数值为 `formula_reference`，不是 Minitab golden。

## 后果

- 与 Minitab 表形（估计+下限+上限）可对照列名，但不能把实现值写入 `VALIDATION_MATRIX.md` 直至真实导出。
- 解释层只陈述区间与 `assumption_status=not_verified`，不把区间下界与 1.33 比较写成合格。
