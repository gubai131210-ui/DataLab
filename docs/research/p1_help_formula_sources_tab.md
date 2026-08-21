# P1 帮助中心「公式与来源」页签

> 研究日期：2026-08-21  
> 访问日期：2026-08-21（UTC+8）  
> 加深现有 `AlgorithmHelpDialog`，不新建壳。

## 1. 现状

- 菜单：**帮助 → 算法、公式与参考资料**
- 数据：`resources/help/algorithm_help.json`（生成自 `help_catalog_families.py`）
- 详情已混排方法说明 + 公式块 + 仓库 md + wiring

## 2. 产品选型

- 详情区增加页签：`方法说明` | **`公式与来源`**
- `公式与来源`：仅 `formula_blocks`（FormulaRenderer）+ 官方 `reference_links`（标签、URL、accessed）
- **不含**：仓库 md 路径、wiring 表（留在「方法说明」底部）
- 树/搜索/复制摘要/复制公式/打开链接不变
- 正文禁止「见 md」；本轮为 Mood / Cochran / Wilcoxon 补条目并修正 Wilcoxon 输入描述

## 3. 明确不做

新顶层 Dialog；Ryan–Joiner 条目；i18n 大改；把公式页做成只链仓库文档。

## 4. 测试

`algorithm_help_dialog_test`：页签存在；选条目后公式区非空；官方链接可解析。  
`algorithm_help_catalog_test`：新命令覆盖；禁止「见 md」。
