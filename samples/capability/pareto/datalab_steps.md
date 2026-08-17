# DataLab：柏拉图统一操作

菜单路径：**图形 → 柏拉图**

## 共同设置

1. **文件 → 导入数据**，选择对应 CSV。
2. 打开 **图形 → 柏拉图**。
3. **先点击右侧角色框**（蓝框高亮），再在左侧选列，点 **选择 >** 或双击列名。
4. 完成后记录：
   - 参数摘要中的总计数
   - 结果表：`类别 / 计数 / Percent / Cum %`
   - 图：柱顺序是否与表一致；累计线是否单调升到 100%
   - 诊断/解释卡（若有）

## official_primary（ClothingDefect）

| 字段 | 填什么 |
|---|---|
| 缺陷类别 | `Defect` |
| 计数列 | `Count` |
| Other 合并阈值 | 留空 |

对照：总计数应为 **480**；第一类 `Missing button`，计数 217。

## alternate_layout（PaintFlaws）

| 字段 | 填什么 |
|---|---|
| 缺陷类别 | `Flaws` |
| 计数列 | 不选（原始观测） |
| Other 合并阈值 | 留空 |

对照：总计数应为 **40**；第一类 `Peel`=15，第二类 `Scratch`=13。  
同频 `Other` 与 `Smudge`（均为 6）时，DataLab 按类别名字母序：`Other` 在前。若 Minitab 顺序不同，在报告中注明为同频排序差异。

## edge_case（缺失 + Other=90）

| 字段 | 填什么 |
|---|---|
| 缺陷类别 | `Defect` |
| 计数列 | `Count` |
| Other 合并阈值 | `90` |

对照：

- 缺失行应被跳过（类别 `*`、计数 `*`）。
- 有效总计数 **490**。
- 保留到累计首次超过 90% 的类别后，其余合并为 `Other`。
- 预期保留：Missing button, Stitching errors, Loose thread, Hemming errors, Fabric flaws；`Other` 计数 **28**。

## 工作表基础操作（一并测）

导入任一 CSV 后：

1. 框选矩形区域 → `Ctrl+C` 复制 → 点到空白区 `Ctrl+V` 粘贴。
2. 选中单元格 → `Delete` 清除内容。
3. `Ctrl+X` 剪切后 `Ctrl+Z` / `Ctrl+Y` 撤销重做。
4. 右键菜单应有：复制、剪切、粘贴、清除单元格。
