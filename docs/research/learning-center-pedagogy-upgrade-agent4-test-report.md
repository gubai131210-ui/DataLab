# Agent4 Test Report — 学习中心教学升级（WAVE=5）

> **岗**：Agent4 Test  
> **日期**：2026-09-03  
> **权威**：[`goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md`](goal-learning-center-pedagogy-upgrade-plan-and-mega-prompt.md)  
> **计划**：[`goal-learning-center-pedagogy-upgrade-wave-plan.md`](goal-learning-center-pedagogy-upgrade-wave-plan.md)  
> **约束**：Python gate 由 Agent 跑；C++ / Qt Creator 由用户本机编译（中文路径不强跑 cmake）；**不 commit**。

---

## 1. Verify 结果（`LEARNING_CENTER_WAVE=5`）

| Script | Result | Exit |
|--------|--------|------|
| `tools/verify_learning_center_gate.py` | **PASS**（内嵌 db + research + mapping + content quality, wave=5） | 0 |
| `tools/verify_learning_center_db.py` | **PASS** — 184 tutorials, 93 datasets, catalog `learning-center-v2`, gold `imr_spi_shift`, no dangling/banned ids | 0 |
| `tools/verify_learning_dataset_mapping.py` | **PASS** — Mappings 184/184; datasets 93 | 0 |
| `tools/verify_learning_research_notes.py` | **PASS** — 184/184 ids | 0 |

运行方式（PowerShell）：

```powershell
$env:LEARNING_CENTER_WAVE='5'
python tools/verify_learning_center_gate.py
python tools/verify_learning_center_db.py
python tools/verify_learning_dataset_mapping.py
python tools/verify_learning_research_notes.py
```

**未**用「看一眼 HTML」代替 gate；**未**强跑 cmake；**未** commit。

---

## 2. 旧「恰好 10 dataset」断言

| 项 | 状态 |
|----|------|
| `listsTenDatasets` / `QCOMPARE(..., 10)` | **已替换** → `LearningCenterStoreTest::listsPlannedDatasets()`：`size() >= 2`，强制含 `imr_spi_shift` / `imr_spi_spike_b`，并断言旧 10 id **不在** catalog |
| db verify「恰好 10」 | **已替换** → 与 mapping 对齐（现网 **93** datasets） |

### 允许残留（仅 banned 列表本身）

旧 10 字符串仅允许出现在「禁止名单」中（gate Wave-5 扫描对 banned 列表行豁免）：

| 位置 | 用途 |
|------|------|
| `tools/verify_learning_center_gate.py` → `BANNED_OLD_DATASET_IDS` | Wave-5 内容扫描 |
| `tools/verify_learning_center_db.py` → `BANNED_OLD_DATASET_IDS` | sqlite 不得含旧 id |
| `tools/verify_learning_dataset_mapping.py` → `BANNED_OLD_DATASET_IDS` | mapping 不得挂旧 id |
| `tools/verify_learning_research_notes.py` → `BANNED_OLD_DATASET_IDS` | notes hint 不得推荐旧 id |
| `tools/build_learning_center_db.py` → `BANNED_OLD_DATASET_IDS` | 生成期拒绝旧 id / 清旧 CSV |
| `tests/learning_center_store_test.cpp` → `QStringList banned` | 断言 catalog **不含**旧 10 |

旧 10 id：`smt_paste_height`, `two_line_thickness`, `paired_rework`, `anova_cavity`, `corr_temp_offset`, `attribute_defect`, `gage_rr_balance`, `doe_factorial_demo`, `reliability_cycles`, `ts_weekly_yield`。

**说明**：现网新专用集 `corr_temp_offset_y` / `ts_weekly_yield_series` 等为**新 id**（后缀区分），gate token 边界已通过，不算旧表残留。

---

## 3. 请用户本机 Qt Creator 编译的 C++ test target

对照 `CMakeLists.txt`（约 1064–1069 行）：

| CMake target | 源文件 | 备注 |
|--------------|--------|------|
| **`learning_center_store_test`** | `tests/learning_center_store_test.cpp` | catalog v2、planned datasets、金标字段、连接不泄漏；需 `Qt6::Sql` |
| **`learning_center_worksheet_registry_test`** | `tests/learning_center_worksheet_registry_test.cpp` | 多工作表导入 / 不清覆盖 |
| **`learning_center_analysis_sample_test`** | `tests/learning_center_analysis_sample_test.cpp` | I-MR / 直方图走 `imr_spi_shift`；另有 3 个 **QSKIP**（见风险） |

Qt Creator：**Projects → Build → Build Steps** 勾选上述 target，或在测试树运行同名 CTest。Agent **不**代跑 cmake（中文路径）。

---

## 4. 金标 I-MR「导入后预期」检查单（摘要）

权威细则见 wave-plan §6.6；sqlite 抽检确认：`dataset_id=imr_spi_shift`，`buried` row **41** / **55**，`dialog_fill_detail` **9**，glossary 含 **UCL** 与 **USL**，catalog **`learning-center-v2`**。

1. 学习中心选 **I-MR** → 导入 → 工作表名必须是 **`demo_imr_spi_shift`**（禁止 `demo_demo_…`）。
2. 列：`片号` / `锡膏高度_um` / `时段备注`；对话框变量=`锡膏高度_um`；**阶段列留空**；MR 长度=2；σ=平均移动极差；Nelson estimate=否；规则默认；tests / 历史限空。
3. **I 图**：片 **41** 起后段上移（钢网更换阶跃）；片 **55** 尖峰相对近期波动不寻常 / 越 **UCL** 线索；MR 在尖峰处应变大。
4. **话术**：**UCL ≠ USL**；禁止「过程合格 / 必须停线 / 点出 UCL=废品」。
5. **UI**：0–6 默认展开；7+ 默认折叠；先修/检索可点；fade level 2 可导入练习表 `demo_imr_spi_spike_b`（尖峰行号与金标不同）。

---

## 5. Agent4 DoD 勾选

| DoD | 状态 |
|-----|------|
| Python gate 本机可 PASS（Agent 跑，WAVE=5） | [x] |
| 给出「请用户编译的 test target 列表」 | [x] |
| 金标 imr「导入后预期」检查单（片 55 / UCL 话术） | [x] |
| 禁止用「看一眼 HTML」代替 gate | [x] |
| 不强跑 cmake / 不 commit | [x] |
| 旧「恰好 10」断言已替换；banned 残留已列 | [x] |

---

## 6. 风险（一行 + 展开）

**主风险**：`learning_center_analysis_sample_test` 仍对 `cat_shift_line` / `pareto_defect_tail` / `cap_between_within` 使用 Wave-0 文案的 **QSKIP**，C++ 侧未覆盖已入库的非金标样本分析。

其他注意：

- meta.`source_git` 仍标 `learning-center-v2-wave4`（内容 gate 已 PASS；收尾可择机改标签）。
- C++ / UI / package_dist **未**在本岗执行；金标读图与折叠 UI 依赖用户本机 + Agent6。

---

## 7. Go / No-Go → Agent5

**GO**（可进 Agent5 Closeout）。

依据：WAVE=5 四门 Python verify 全 PASS；旧 10 仅 banned 列表残留；catalog v2 + 184/93 对齐；金标检查单已落盘。Agent5 勿塞新功能；请提醒用户编译上表三 target，并视需要取消 sample test 的过期 QSKIP（属测试补全，非本岗改码范围）。
