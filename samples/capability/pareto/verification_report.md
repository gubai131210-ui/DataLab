# 柏拉图验证报告

状态：**等待用户填写 Minitab / DataLab 对照结果**  
日期：2026-08-15

## 软件信息（请填写）

```text
minitab_version:
datalab_build: Desktop_Qt_6_11_1_MinGW_64_bit_Debug（本地）
operator:
```

## 对照表模板

对每一套数据复制下表并填写。

### official_primary

| metric | minitab_value | datalab_value | absolute_difference | relative_difference | allowed_tolerance | status | notes |
|---|---|---|---|---|---|---|---|
| total_count |  |  |  |  | exact |  | 期望 480 |
| order |  |  |  |  | exact |  |  |
| Missing button count |  |  |  |  | exact |  | 217 |
| Missing button Percent |  |  |  |  | 1e-4 rel |  | ~45.2083 |
| Missing button Cum % |  |  |  |  | 1e-4 rel |  |  |
| Stitching errors Cum % |  |  |  |  | 1e-4 rel |  | ~68.5417 |

### alternate_layout

| metric | minitab_value | datalab_value | absolute_difference | relative_difference | allowed_tolerance | status | notes |
|---|---|---|---|---|---|---|---|
| total_count |  |  |  |  | exact |  | 期望 40 |
| Peel count/percent |  |  |  |  | exact / 1e-4 |  | 15 / 37.5 |
| Scratch count/percent |  |  |  |  | exact / 1e-4 |  | 13 / 32.5 |
| Other vs Smudge order |  |  |  |  | qualitative |  | 同频排序 |

### edge_case

| metric | minitab_value | datalab_value | absolute_difference | relative_difference | allowed_tolerance | status | notes |
|---|---|---|---|---|---|---|---|
| total_count |  |  |  |  | exact |  | 期望 490 |
| missing handling |  |  |  |  | exact |  | * 行是否跳过 |
| retained categories |  |  |  |  | exact |  | 5 类 + Other |
| Other count |  |  |  |  | exact |  | 期望 28 |

## 差异分类（填写时勾选）

- [ ] 显示取整
- [ ] 数据/布局不一致
- [ ] 估计方法不一致（柏拉图一般不适用）
- [ ] 缺失值或 Other 规则不一致
- [ ] 公式/实现缺陷

## 结论

在用户回报数值前：**不标记为已通过 Minitab 验证**。
