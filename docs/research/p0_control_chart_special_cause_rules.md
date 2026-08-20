# P0 控制图 Special-Cause Rules（Minitab 对齐）

- 访问日期：2026-08-20
- 适用范围：`Test 1..8` 的规则定义、默认策略、可用图种差异

## 公式与判定

1. Test 1: `|y_i-CL_i| > 3σ_i`
2. Test 2: 连续 `9` 点在中心线同侧
3. Test 3: 连续 `6` 点严格上升或严格下降
4. Test 4: 连续 `14` 点相邻方向交替
5. Test 5: 连续 `3` 点中同侧且 `>2σ` 的点数至少 `2`
6. Test 6: 连续 `5` 点中同侧且 `>1σ` 的点数至少 `4`
7. Test 7: 连续 `15` 点满足 `|y_i-CL_i| < 1σ_i`（严格小于）
8. Test 8: 连续 `8` 点满足 `|y_i-CL_i| > 1σ_i`（允许同侧）

## 变量定义

- `y_i`: 第 `i` 个绘图点
- `CL_i`: 对应中心线（分阶段可变）
- `σ_i`: 对应点标准差（分图种/分阶段可变）

## 适用条件与实现口径

- EWMA 仅启用 Test 1。
- CUSUM 不使用 Test 1..8，改用上/下侧累计和首次信号。
- R/S/MR 仅启用 Test 1..4。
- 窗口在阶段切换、缺失点处打断，防止跨阶段串窗。

## Minitab 口径

- 默认只启用 Test 1，但可勾选额外规则。
- 随着启用规则增多，误报概率会上升，需结合工程判断。

## 来源

- https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/supporting-topics/basics/using-tests-for-special-causes/
- https://support.minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/control-charts/how-to/variables-charts-for-individuals/z-mr-chart/perform-the-analysis/z-mr-options/select-tests-for-special-causes/
