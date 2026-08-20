# P1 Multi-Vari 第 4 因子

> 研究日期：2026-08-20  
> 访问日期：2026-08-20（UTC+8）  
> 延后项落地：[`deferred-capability-agreement.md`](deferred-capability-agreement.md) §5 Multi-Vari 第四因子。

## 1. 来源

| 主题 | 来源 | 访问日期 |
|---|---|---|
| Minitab Multi-Vari | [Multi-Vari Chart](https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/quality-tools/how-to/multi-vari-chart/before-you-start/overview/)（表形/用途；不填未导出布局数） | 2026-08-20 |
| DataLab 现有 2～3 因子合同 | [`unusual-obs-multivari-doe-chart-formulas.md`](unusual-obs-multivari-doe-chart-formulas.md) | 2026-08-20 |
| 导入契约 | [`algorithm-chart-gap-matrix.md`](algorithm-chart-gap-matrix.md) §3 | 2026-08-20 |

## 2. 产品选型

- 因子上限 **2～4**（domain / UI / service 一致）。  
- complete-case：测量可解析 + 全部因子非空/`*`；保留 `source_row`。  
- 覆盖率 &lt;60% 只诊断不画图（阈值不变）。  
- 不做第 5+ 因子；不另写导入解析。

## 3. 布局合同（第 4 因子）

因子索引：0=主轴水平，1=组内抖动，2=块偏移，3=更外层块偏移。

```text
# 现有 2～3：
x = idx0
if factor_count≥3: x += idx2 · (n0 + 1)
if factor_count≥2: x += (idx1 − (n1−1)/2) · 0.25

# 第 4：
if factor_count≥4:
  block3 = (n0 + 1) · max(n2, 1)   # 或 (n0+1)*(n2) 当 n2≥1
  x += idx3 · (block3 + 1)
```

确定性：同水平组合 → 同 `x_position`。均值系列标签含最外层因子水平以便区分。

## 4. Facts / 命令

`MultiVariFacts.factor_count` 可达 4；命令角色文案「因子（2～4 列）」。

## 5. 明确不做

第 5 因子；改 60% 门；假 golden。

## 6. 测试

4 因子 complete-case 出图且 `source_row` 保留；3 因子回归；缺因子格计入 missing；覆盖不足无图。
