# Wave 执行态：G6 命令 Wizard（2026-08-23）

> 计划：[`goal-wave-2026-08-23-g6-command-wizard-plan-and-mega-prompt.md`](goal-wave-2026-08-23-g6-command-wizard-plan-and-mega-prompt.md)  
> 调研：[`g6-command-wizard-ux-research-2026-08-23.md`](g6-command-wizard-ux-research-2026-08-23.md)  
> 门禁：`python tools/verify_g6_command_wizard_track.py`  
> **状态：W1–W4 已完成；`verify_g6_command_wizard_track.py` PASS（2026-08-23）。**

---

## DoD（W1–W4 全部勾选才算 complete）

### W1 推荐引擎

- [x] 纯函数引擎落地（无 Qt 优先）
- [x] 规则覆盖 research §3 主路径
- [x] Top-N ≤ 8；推荐 id 均可 `find`
- [x] QtTest ≥12 例（t01–t15 + UI smoke）

### W2 Wizard UI

- [x] 独立 `CommandWizardDialog`（或等价）
- [x] 选列 → 意图 → 推荐 → 打开既有分析设置
- [x] **未**在 Wizard 内调用 `AnalysisService::*`

### W3 接线与 i18n

- [x] MainWindow 入口
- [x] 双语文案
- [x] wiring-index + acceptance §2

### W4 验收

- [x] `tests/g6_command_wizard_track_test.cpp` + CMake
- [x] `tools/verify_g6_command_wizard_track.py` PASS
- [x] 回归 menu IA + wave4 verify PASS
- [x] 本文件 DoD 勾选完成

### 明确不做（保持未做）

- [x] 未做 G3 / LLM / 一键跑全套图 / 改 domain 公式

---

## 人手门

1. Qt Creator Rebuild  
2. 打开命令向导：选 1 列数值 → 见推荐 → 点开进入既有设置对话框（不直接出结果）

---

**文档状态：** 2026-08-23 执行完成；DoD 全勾；人手门待 Qt Creator Rebuild 目视。
