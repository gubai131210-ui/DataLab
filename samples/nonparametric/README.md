# 非参数检验样例

通用导入步骤：

1. 打开 DataLab。
2. **文件 → 打开 / 导入**，选择本目录下的 CSV。
3. 确认工作表列名与下表一致后，从菜单启动对应分析。

## mann_whitney.csv

| 列 | 角色 |
|---|---|
| `SampleA` | 第一组独立样本 |
| `SampleB` | 第二组独立样本 |

DataLab 菜单：**统计 → Mann-Whitney 检验**

弹窗设置：

- 两列独立样本：选择 `SampleA`、`SampleB`
- 备择方向：留空或输入 `two_sided`（也可用 `less` / `greater`）

说明：

- 两组可不对齐；空单元格视为缺失并跳过。
- 重点查看秩和、Z、P 值、位置差估计。

Minitab 对照：`Stat > Nonparametrics > Mann-Whitney`

## wilcoxon_paired.csv

| 列 | 角色 |
|---|---|
| `Before` | 配对前 |
| `After` | 配对后 |

DataLab 菜单：**统计 → Wilcoxon 符号秩检验**

弹窗设置：

- 两列配对样本：选择 `Before`、`After`
- 备择方向：`two_sided`

说明：差值为 0 的配对会被剔除后再做符号秩检验。

Minitab 对照：`Stat > Nonparametrics > 1-Sample Wilcoxon`（对差值）或配对设置。

## kruskal_wallis.csv

| 列 | 角色 |
|---|---|
| `Response` | 测量值 |
| `Group` | 分组标签（A/B/C） |

DataLab 菜单：**统计 → Kruskal-Wallis 检验**

弹窗设置：

- 测量值：`Response`
- 分组列：`Group`

说明：

- 缺少测量值或分组标签的行会被跳过。
- 输出包含各组 N、中位数、平均秩，以及 `H` / `H(adj)` / P。
- 某组观测少于 5 时会出现近似风险提示。

Minitab 对照：`Stat > Nonparametrics > Kruskal-Wallis`
