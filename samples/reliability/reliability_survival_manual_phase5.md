# 可靠性右删失样例 — 手工验收（Phase 5）

配合 [`VALIDATION_MATRIX.md`](../../docs/research/VALIDATION_MATRIX.md) 中 `phase0_reliability_right_censor` 与报告 PDF 预筛。

## 数据

| 项 | 值 |
|----|-----|
| 文件 | [`reliability_survival.csv`](reliability_survival.csv) |
| 列 | `Unit`, `Time`, `Event`, `Group` |
| 删失 | `Event=0` 为右删失 |
| 行数 | 12 |

## Qt Creator 步骤

1. 导入 `reliability_survival.csv`
2. 列映射：`Time` → 寿命；`Event` → 失效指示（1=失效，0=删失）
3. 分别运行（或依次加入 Session）：
   - **Kaplan–Meier**
   - **Weibull**（二参数）
   - **Lognormal**（二参数）
4. 导出 engineer **en-US** 报告（可选 audit 对照三模板裁剪）

## 自动化预筛（报告链）

- `pdf_reliability_km_and_weibull_localize_to_en_us_without_chinese_leak`
- `pdf_reliability_km_and_weibull_cross_template_table_visibility_and_en_us_locale`
- `representative_reliability_km_three_report_profiles_*`
- `representative_reliability_weibull_three_report_profiles_*`
- `representative_reliability_lognormal_three_report_profiles_*`

## Domain 预筛（无 PDF）

- `reliability_phase5_test::km_handcalc_baseline_formula_reference`（小样本手算，见 `reliability_km_handcalc.csv`）
- `reliability_phase5_test::weibull_and_lognormal_have_separate_assertions`

## Pass 焦点（肉眼/UI）

- [ ] 右删失行不计入失效步进
- [ ] Weibull 与 Lognormal 参数/分位寿命 **分别**展示（不混读）
- [ ] en-US PDF 无中文 chrome（若导出报告）
- [ ] **不得**声称 vendor_oracle / PDF/A·UA 合规

## 证据

- `formula_reference`；Weibull/Lognormal pinned R 对齐仍 ⏸（见 [`reference-implementation-index.md`](../../docs/research/reference-implementation-index.md)）。
