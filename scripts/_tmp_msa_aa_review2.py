# -*- coding: utf-8 -*-
from __future__ import annotations
import re
from collections import Counter
from pathlib import Path

ROOT = Path(r"d:\QT_CppPrograms\DataLab")
OUT = ROOT / "scripts" / "_tmp_msa_aa_review_out.txt"


def parse_catalog(path: Path):
    text = path.read_text(encoding="utf-8")
    pat = re.compile(
        r'\{\s*"([^"]+)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}',
        re.S,
    )
    return [(m.group(1), m.group(2), m.group(3)) for m in pat.finditer(text)]


def extract_exact_ids(path: Path):
    text = path.read_text(encoding="utf-8")
    start = text.find("localize_known_plain_message")
    block_start = text.find("exact_ids[]", start)
    brace = text.find("{", block_start)
    i, depth, end = brace, 0, None
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                end = i
                break
        i += 1
    block = text[brace : end + 1]
    pair = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"([^"]+)"\s*\}', re.S)
    return [(m.group(1), m.group(2)) for m in pair.finditer(block)]


def main() -> None:
    lines: list[str] = []
    entries = parse_catalog(ROOT / "src/domain/report_text_catalog.cpp")
    lines.append(f"catalog_entry_count={len(entries)}")
    ids = [e[0] for e in entries]
    counts = Counter(ids)
    dupes = [(k, v) for k, v in counts.items() if v > 1]
    lines.append(f"unique={len(counts)} dupes={len(dupes)}")
    for k, v in sorted(dupes):
        lines.append(f"DUPE x{v}: {k}")

    by_id = {e[0]: e for e in entries}
    msa_aa_ids = sorted(i for i in by_id if i.startswith(("rule.msa.", "rule.aa.")))
    lines.append(f"msa_aa_count={len(msa_aa_ids)}")
    for i in msa_aa_ids:
        zh, en = by_id[i][1], by_id[i][2]
        lines.append(f"CAT {i}\n  ZH={zh}\n  EN={en}")

    # Source strings from domain (hard-coded expected)
    sources = [
        ("append_ndc_rule", "rule.msa.ndc_not_computed.message",
         "ndc 不可估计；不能据此评价测量系统分辨力。"),
        ("append_ndc_rule", "rule.msa.ndc_not_computed.action",
         "先检查零总变异、零 Gage 标准差或无效容差后再解释 ndc。"),
        ("append_ndc_rule", "rule.msa.ndc_lt5.message",
         "ndc < 5，提示测量系统对零件间差异的分辨力需要调查。"),
        ("append_ndc_rule", "rule.msa.ndc_lt5.action",
         "ndc 小于 5 只是调查提示，不是量具不合格的绝对结论。"),
        ("append_ndc_rule", "rule.msa.ndc_ge5.message",
         "ndc ≥ 5；这只说明当前研究中零件间变异相对 Gage 变异较大。"),
        ("append_ndc_rule", "rule.msa.ndc_ge5.action",
         "仍需结合 %Study Var、%Tolerance 和现场公差风险解释。"),
        ("gage_rr", "rule.msa.design_balance.message",
         "交叉设计单元重复次数平衡。"),
        ("gage_rr", "rule.msa.design_balance.action",
         "平衡设计是 ANOVA 方差分量解释的前提。"),
        ("gage_rr_unbalanced", None,
         "零件×操作员单元重复次数不一致。"),
        ("gage_rr_unbalanced", None,
         "交叉设计需要平衡重复后才能解释方差分量。"),
        ("gage_rr", "rule.msa.interaction_triggered.message",
         "交互项 p>0.25，可考虑缩减，但当前保留完整模型。"),
        ("gage_rr", "rule.msa.interaction_ok.message",
         "当前保留 Part×Operator 交互模型。"),
        ("gage_rr", "rule.msa.interaction.action",
         "交互是否缩减必须回显；本实现不自动并入重复性。"),
        ("gage_rr", "rule.msa.negative_variance_triggered.message",
         "存在负方差分量，已截断为 0，并保留原始估计。"),
        ("gage_rr", "rule.msa.negative_variance_ok.message",
         "方差分量原始估计均非负。"),
        ("gage_rr", "rule.msa.negative_variance.action",
         "截断后的分量用于 %Contribution；解释时同时查看原始值。"),
        ("gage_rr", "rule.msa.percent_metrics.message",
         "%Contribution 基于方差，%Study Var 基于标准差，口径不同。"),
        ("gage_rr", "rule.msa.percent_metrics.action",
         "不要把 %Contribution 与 %Study Var 当成同一个百分比。"),
        ("gage_rr", "rule.msa.invalid_tolerance.message",
         "未提供有效公差，%Tolerance 不可用。"),
        ("gage_rr", "rule.msa.invalid_tolerance.action",
         "只有有限正公差才能计算 %Tolerance。"),
        ("aa_facts", "rule.aa.kappa_interpretation.message",
         "拒绝 Kappa=0 不等于已证明评估者一致。"),
        ("aa_facts", "rule.aa.kappa_interpretation.action",
         "Kappa 只描述超出偶然的绝对一致率，不能写成测量系统合格。"),
        ("aa_facts", "rule.aa.weighted_kappa.message",
         "Weighted Kappa 是 DataLab 可选 Cohen 加权，不是 Minitab AAA 默认输出。"),
        ("aa_facts", "rule.aa.weighted_kappa.action",
         "Minitab 有序评级路径使用 Kendall；不要把加权 κ 写成 Minitab AAA 结果。"),
        ("aa_facts", "rule.aa.kendall_ok.message",
         "有序评级已计算 Kendall W/τ；拒绝系数为 0 不等于已证明有序一致。"),
        ("aa_facts", "rule.aa.kendall_unavailable.message",
         "已请求有序评级，但 Kendall 不可识别或等级不足。"),
        ("aa_facts", "rule.aa.kendall.action",
         "不要把 Kendall 写成加权 Kappa，也不要把未拒绝原假设写成已证明一致。"),
        ("catalog_name", "rule.msa.catalog.design_balance.name", "设计平衡"),
        ("catalog_name", "rule.msa.catalog.ndc_investigation.name", "ndc 调查"),
    ]

    exact = extract_exact_ids(ROOT / "src/application/report_localization.cpp")
    exact_map = {zh: eid for zh, eid in exact}
    lines.append(f"exact_ids_count={len(exact)}")

    lines.append("=== source zh vs catalog/exact ===")
    for src, eid, zh in sources:
        cat_ok = eid is not None and eid in by_id and by_id[eid][1] == zh
        exact_id = exact_map.get(zh)
        exact_ok = exact_id == eid if eid else exact_id is not None
        if eid is None:
            lines.append(
                f"{src}: NO_ID_EXPECTED zh={zh!r} in_exact={zh in exact_map} mapped={exact_id}"
            )
        else:
            cat_zh = by_id[eid][1] if eid in by_id else None
            lines.append(
                f"{src}/{eid}: cat_zh_match={cat_ok} exact_id={exact_id!r} exact_ok={exact_ok}"
            )
            if not cat_ok:
                lines.append(f"  expected_zh={zh!r}")
                lines.append(f"  catalog_zh={cat_zh!r}")

    # Test expectations vs catalog EN
    test_checks = [
        ("rows[0][3]", "rule.msa.ndc_not_computed.message", "cannot be estimated"),
        ("rows[0][5]", "rule.msa.ndc_not_computed.action", "zero Gage SD"),
        ("rows[1][3]", "rule.msa.ndc_lt5.message", "ndc < 5"),
        ("rows[1][1]", "rule.msa.catalog.ndc_investigation.name", "ndc investigation"),
        ("rows[2][3]", "rule.msa.ndc_ge5.message", "ndc ≥ 5"),
        ("rows[3][1]", "rule.msa.catalog.design_balance.name", "Design balance"),
        ("rows[4][3]", "rule.msa.interaction_triggered.message", "Interaction p>0.25"),
        ("rows[5][3]", "rule.msa.percent_metrics.message", "%Contribution is variance-based"),
        ("rows[7][3]", "rule.aa.kappa_interpretation.message", "does not prove rater agreement"),
        ("rows[8][3]", "rule.aa.weighted_kappa.message", "not Minitab AAA"),
        ("rows[9][3]", "rule.aa.kendall_ok.message", "Kendall W/τ"),
        ("rows[10][3]", "rule.aa.kendall_unavailable.message", "not identifiable"),
        ("rows[11][3]", "rule.msa.negative_variance_triggered.message", "truncated to 0"),
        ("title", "table.rule_evidence", "Rule evidence"),
        ("headers[0]", "header.rule", "Rule"),
        ("headers[1]", "header.rule_name_col", "Name"),
        ("headers[2]", "header.status", "Status"),
        ("headers[3]", "header.evidence", "Evidence"),
        ("headers[4]", "header.related_rows", "Related rows"),
        ("headers[5]", "header.suggestion", "Suggestion"),
    ]
    lines.append("=== test fragments vs catalog EN ===")
    for label, eid, frag in test_checks:
        if eid not in by_id:
            lines.append(f"FAIL {label}/{eid}: MISSING")
            continue
        en = by_id[eid][2]
        ok = frag in en
        lines.append(f"{'OK' if ok else 'FAIL'} {label}/{eid}: frag={frag!r} en={en!r}")

    # Header zh mapping presence in localization
    loc = (ROOT / "src/application/report_localization.cpp").read_text(encoding="utf-8")
    header_pairs = [
        ("规则", "header.rule"),
        ("名称", "header.rule_name_col"),
        ("状态", "header.status"),
        ("证据", "header.evidence"),
        ("关联行", "header.related_rows"),
        ("建议", "header.suggestion"),
        ("规则证据", "table.rule_evidence"),
    ]
    lines.append("=== header wiring in localize_visible_tables ===")
    for zh, eid in header_pairs:
        needle = f'{{"{zh}", "{eid}"}}'
        # also accept spaced variants
        present = f'"{zh}", "{eid}"' in loc or f'"{zh}",\n' in loc and eid in loc
        # simpler
        present = (f'"{zh}"' in loc) and (f'"{eid}"' in loc)
        # more precise
        present = bool(re.search(rf'\{{\s*"{re.escape(zh)}"\s*,\s*"{re.escape(eid)}"\s*\}}', loc))
        cat_zh = by_id[eid][1] if eid in by_id else None
        lines.append(f"{zh}->{eid}: wiring={present} catalog_zh={cat_zh!r}")

    # ID naming mismatch check: script earlier saw interaction_ok vs interaction_ok
    lines.append("=== ID naming notes ===")
    for candidate in [
        "rule.msa.interaction_ok.message",
        "rule.msa.interaction_ok.message",
        "rule.msa.negative_variance_ok.message",
        "rule.msa.negative_variance_ok.message",
        "rule.msa.invalid_tolerance.message",
        "rule.msa.invalid_tolerance.message",
    ]:
        lines.append(f"exists {candidate}: {candidate in by_id}")

    # Dump actual IDs for interaction/negative/invalid
    for prefix in [
        "rule.msa.interaction",
        "rule.msa.negative_variance",
        "rule.msa.invalid_tolerance",
        "rule.msa.design_balance",
    ]:
        hits = [i for i in msa_aa_ids if i.startswith(prefix)]
        lines.append(f"prefix {prefix}: {hits}")

    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"wrote {OUT}")


if __name__ == "__main__":
    main()
