# 可靠性样例（Phase 5）

| 文件 | 用途 | 证据 |
|------|------|------|
| [`reliability_survival.csv`](reliability_survival.csv) | 右删失 KM / Weibull / Lognormal UI 与报告预筛 | `formula_reference` |
| [`reliability_survival_manual_phase5.md`](reliability_survival_manual_phase5.md) | Qt Creator 手工验收步骤 |

Phase 0 基线（KM 手算 / 长表 / 保修分层）见 [`../phase0_baselines/README.md`](../phase0_baselines/README.md)。  
S1 场景 Qt Creator 分批：`python tools/list_qt_creator_test_targets.py --scenario-id S1 --by-target`

Reference 脚本：`scripts/reliability_km_reference.py`、`scripts/reliability_warranty_reference.py`（见 [`reference-implementation-index.md`](../docs/research/reference-implementation-index.md)）。
