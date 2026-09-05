# Fixture Expected 导出 / 生成指南

本目录存放两类期望值，**不得混称**：

| 轨道 | 文件模式 | 证据类型 | 缺失时测试行为 |
|------|----------|----------|----------------|
| **(A) 历史 regression/arima** | `regression_golden.tsv`、`arima_trend_golden.tsv` | 公式镜像 / 待 vendor 导出核对 | 可 `QSKIP`（**历史行为 ≠ G-Trust Goal 完成定义**） |
| **(B) G-Trust ref-golden** | `<command_id>_ref_golden.tsv` | `golden` ← `reference_implementation` | 锁表命令 **必须 FAIL**（禁止 `QSKIP` 冒充完成） |

**禁止**声称「已与 Minitab 数值对齐 / vendor_oracle」。真·Minitab 导出属后续 Goal。

## (B) G-Trust 锁表 10 命令（reference_implementation）

由 `scripts/g_trust_<id>_reference.py`（Python 3.10+ / stdlib float64）生成。复跑：

```powershell
python scripts/g_trust_<command_id>_reference.py
```

| command_id | expected 文件 | 参考脚本 | 输入 | 状态 |
|---|---|---|---|---|
| `imr` | `imr_ref_golden.tsv` | `scripts/g_trust_imr_reference.py` | `converted/imr_ref_golden_input.csv` | ref-golden 已冻 |
| `xbar_r` | `xbar_r_ref_golden.tsv` | `scripts/g_trust_xbar_r_reference.py` | `converted/CamshaftLength.csv`（`Machine 1`/`Subgroup ID`；Crankshaft Date 组不等 n） | ref-golden 已冻 |
| `p_chart` | `p_chart_ref_golden.tsv` | `scripts/g_trust_p_chart_reference.py` | `converted/UnansweredCalls.csv` | ref-golden 已冻 |
| `capability` | `capability_ref_golden.tsv` | `scripts/g_trust_capability_reference.py` | `converted/PistonRingDiameter.csv` | ref-golden 已冻 |
| `capability_sixpack` | `capability_sixpack_ref_golden.tsv` | `scripts/g_trust_capability_sixpack_reference.py` | 同上 | ref-golden 已冻 |
| `between_within_capability` | `between_within_capability_ref_golden.tsv` | `scripts/g_trust_between_within_capability_reference.py` | `converted/cap_between_within.csv`（LSL/USL/Target=95/105/100） | ref-golden 已冻 |
| `gage_rr` | `gage_rr_ref_golden.tsv` | `scripts/g_trust_gage_rr_reference.py` | `converted/gage_rr_crossed.csv` | ref-golden 已冻 |
| `two_sample_t` | `two_sample_t_ref_golden.tsv` | `scripts/g_trust_two_sample_t_reference.py` | `converted/two_sample_t_ref_golden_input.csv` | ref-golden 已冻 |
| `normality_test` | `normality_test_ref_golden.tsv` | `scripts/g_trust_normality_test_reference.py` | `converted/PistonRingDiameter.csv` | ref-golden 已冻 |
| `one_way_anova` | `one_way_anova_ref_golden.tsv` | `scripts/g_trust_one_way_anova_reference.py` | `converted/one_way_anova_ref_golden_input.csv` | ref-golden 已冻 |

门禁：`python tools/verify_g_trust_golden_gate.py`（默认 / `--all` 检查全部 10；`--wave N` 可按施工波次收窄）。

## (A) 历史文件清单（非本 Goal 完成证据）

| 文件 | 数据集 | 说明 |
|---|---|---|
| `regression_golden.tsv` | `converted/regression.csv` | OLS 公式参考；可日后用 Minitab 导出替换 `# source` |
| `arima_trend_golden.tsv` | `converted/arima_trend.csv` | ARIMA 候选网格镜像；可日后用 Best ARIMA 导出核对 |

重新生成历史公式参考（**不**覆盖锁表 10）：

```powershell
.\.venv\Scripts\python.exe tools\dump_minitab_golden_reference.py
```

## TSV 格式（两轨道共用）

- UTF-8，无 BOM，Tab 分隔
- `# source:` 须标明证据来源（ref-golden 必须含 `reference_implementation`）
- `# config: key=value`
- `# section: 名称` 后首行为表头

## 容差

见 `tests/fixtures/minitab/VALIDATION_MATRIX.md` 与 Wave 计划 §4。默认中心/限：`rel_tol=1e-4` / 近零 `abs_tol=1e-6`。
