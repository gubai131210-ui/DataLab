# Goal Wave 计划模板

> 复制本文件为 `docs/research/goal-wave-YYYY-MM-DD-{name}.md` 并在 `/goal` 启动时锁定。

## Meta

| 字段 | 值 |
|------|-----|
| Goal 名称 | |
| Orchestrator | 主 agent |
| Wave 数 | 1–3 |
| 验收 | unified_track_acceptance_plan.md |
| verify 脚本 | `tools/verify_{name}_track.py` |

## Wave 1（锁定项 — 全部完成才结束 Wave）

| # | 类型 | id / Track | research md | Primary URL（待填） | DoD |
|---|------|------------|-------------|---------------------|-----|
| 1 | 新增/深化 | | | | [ ] |
| 2 | | | | | [ ] |
| 3 | | | | | [ ] |
| 4 | | | | | [ ] |

## Wave 2（可选）

| # | 类型 | id / Track | research md | DoD |
|---|------|------------|-------------|-----|
| 1 | | | | [ ] |

## 多 Agent 分工

| 阶段 | Agent | 任务 |
|------|-------|------|
| 启动 | explore | 扫 backlog + wiring，确认 id 未重复 |
| 调研 | 主 agent + WebSearch | 每项 Primary URL → research md |
| 实现 | 主 agent | 顺序竖切；domain 多时用 cpp-coding skill |
| 审查 | bugbot | Wave 末 diff vs DoD |
| 验证 | shell / 主 agent | verify 脚本 PASS |

## 禁止偷懒

粘贴 `goal-execution-framework.md` §6 全文。

## Wave 完成检查

- [ ] 全部项 DoD §4  
- [ ] verify 脚本 PASS  
- [ ] acceptance §2 更新  
- [ ] backlog / wiring-index 更新  
