# 学习中心文案加深 — Agent6 教学检验 QA

> **岗**：Agent6 教学检验（非开发新人视角）  
> **日期**：2026-09-03  
> **手册**：[`goal-learning-center-copy-depth-and-graph-alignment-plan-and-mega-prompt.md`](goal-learning-center-copy-depth-and-graph-alignment-plan-and-mega-prompt.md)  
> **证据**：`tools/learning_data/tutorial_overlays/*.json`（184）+ `LEARNING_CENTER_WAVE=5` `python tools/verify_learning_center_gate.py` **PASS**（含 `verify_learning_center_copy_depth.py`）

---

## 1. 金标 I-MR（必须通过）

| 检查 | 结果 |
|------|------|
| 0–6 片 41 / 55 | **在**。`imr.json` buried `row` 41 与 55；dialog_fill_detail 仍 9 行；UCL≠USL 在 used_for/not_for/glossary |
| 7+ 像课堂提问 | 先修是完整问句（UCL 是什么、数据如何排列、何时用 I-MR）；选项是完整中文；`why`/`hint` 讲原因，不是复读题目 |
| 揭晓来源 | `LearningCenterPage` 读本课 `hint`/`why`；缺省中性句；**无**全课检索套 UCL |
| 范围缩水 | **无**。未把 184 缩成只改 imr；生成器 `copy_depth.py` + 各 `wave*_content.write_overlays` 会再 polish |

**金标 7+：通过。**

---

## 2. 抽检（每 Wave ≥2 + 图形 ≥3）

| 课 | 先修是否没头没脑 | 揭晓 | 内部 id 当答案 |
|----|------------------|------|----------------|
| `imr`（W0） | 否 | hint 讲 Nelson / 阶段列 / UCL≠USL | 无 |
| `c_chart` / `p_chart`（W1） | 重写为「本课主要学什么」完整问句 | 有 hint | 无 `^[a-z_]+$` good |
| `gage_rr`（W2） | 同上 | 有 hint | 无 |
| `two_sample_t`（W3） | 不再是「列数？/共享？」 | 有 hint | 无 `infer_two_sample_location` 当 good |
| `histogram`（W4） | 不再是「共享族？/UCL=柱高？」 | 有 hint；非 SPC 练习区不考 UCL | related 改为概率图+箱线 |
| `bar_chart` | 标题仍是**条形图**；图标 Pareto 有人话说明 | 有 hint | related→饼图/马赛克 |
| `area_plot` | 完整问句 | 有 hint | `dialog_fill` 含 time+**value=产量** |
| `interval_plot` | 标题 **区间散点图**；fill 含 category | 有 hint | related 非三件套 |

未发现「列数？」「禁止句」作题干的残留（可读性脚本拦截）。

---

## 3. 图形名实

红表已修（overlay 现网）：

- related 模板三件套：verify 禁止且 PASS  
- `bar_chart` 不改名为 Pareto  
- `area_plot`/`scatter_plot`/`time_series_plot` 等必填角色已进 `dialog_fill`  
- `interval_plot` 菜单名对齐  

图标借用用 used_for 一句说明，**未改** `analysis_commands` 图标键。

---

## 4. 驳回项

无阻断驳回。建议你本机抽查学习中心练习区揭晓（sqlite 已由 `tools/build_learning_center_db.py` 重建为 v2）。部分 0–6 仍带角色英文字段名，属对话框对照需要，不是电报题干。

---

## 5. go/no-go

**go 收尾**：金标 7+ 通过；184 overlay 可读性门 PASS；无范围缩水。
