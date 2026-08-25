#!/usr/bin/env python3
"""Analyze 主公式 stubs in computation_trace_attach.cpp."""
from __future__ import annotations

import json
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

# Import FAMILY map from generator
import importlib.util

spec = importlib.util.spec_from_file_location(
    "gen_g9", ROOT / "tools/_gen_g9_computation_trace_attach.py"
)
gen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gen)

FAMILY = gen.FAMILY
family_of = gen.family_of

CPP = ROOT / "src/application/computation_trace_attach.cpp"
HELP = ROOT / "resources/help/algorithm_help.json"
QUALITY = ROOT / "src/domain/quality_types.h"


def extract_stubs() -> list[tuple[str, str, str]]:
    cpp = CPP.read_text(encoding="utf-8")
    stubs: list[tuple[str, str, str]] = []
    for m in re.finditer(r'if \(command_id == "([^"]+)"\)', cpp):
        cid = m.group(1)
        block = cpp[m.start() : m.start() + 2500]
        pm = re.search(r'tr\.plain_formula = "([^"]+)"', block)
        tm = re.search(r'tr\.title = "([^"]+)"', block)
        if pm and "主公式" in pm.group(1):
            stubs.append((cid, tm.group(1) if tm else "", pm.group(1)))
    # dedupe preserving order
    seen: set[str] = set()
    out: list[tuple[str, str, str]] = []
    for s in stubs:
        if s[0] not in seen:
            seen.add(s[0])
            out.append(s)
    return out


def extract_interpretation_facts_fields() -> list[str]:
    text = QUALITY.read_text(encoding="utf-8")
    m = re.search(r"struct InterpretationFacts \{([\s\S]*?)\n\};", text)
    if not m:
        return []
    body = m.group(1)
    fields = re.findall(
        r"std::optional<(\w+)>\s+(\w+);", body
    )
    return [name for _, name in fields]


def load_help_formulas() -> dict[str, list[str]]:
    data = json.loads(HELP.read_text(encoding="utf-8"))
    entries = data.get("entries", [])
    out: dict[str, list[str]] = {}
    for entry in entries:
        if not isinstance(entry, dict):
            continue
        cid = entry.get("id") or entry.get("command_id")
        if not cid:
            continue
        blocks = entry.get("formula_blocks") or []
        formulas: list[str] = []
        for b in blocks:
            if isinstance(b, str):
                formulas.append(b)
            elif isinstance(b, dict):
                for k in ("plain_text", "formula", "plain", "text", "latex", "content"):
                    if k in b and b[k]:
                        formulas.append(str(b[k]))
                        break
        if formulas:
            out[cid] = formulas
    return out


def map_family(cid: str) -> str:
    return FAMILY.get(cid, family_of(cid))


# Family -> relevant InterpretationFacts optional fields (heuristic mapping)
FAMILY_FACTS: dict[str, list[str]] = {
    "能力/质量": [
        "capability", "batch_capability", "nonparametric_capability",
        "tolerance", "acceptance_sampling", "distribution_identification",
        "multi_vari", "variability", "box_cox",
    ],
    "控制图": [
        "spc", "multivariate_spc", "zone_chart", "zmr", "moving_average_chart",
    ],
    "基础统计": [
        "descriptive", "normality", "outlier_test", "correlation",
        "t_test", "proportion", "poisson_rate", "equivalence",
        "anova", "chi_square", "cross_tab", "chi_square_gof", "poisson_gof",
        "nonparametric", "variance", "anom",
    ],
    "回归/多变量/ML": [
        "regression", "logistic", "poisson_regression", "ordinal_logistic",
        "nominal_logistic", "stepwise_regression", "best_subsets_regression",
        "pca", "kmeans", "cart_tree", "random_forest", "isolation_forest",
        "discriminant", "hierarchical_cluster", "adf", "ccf", "correlogram",
        "acf_pacf", "forecast",
    ],
    "可靠性": [
        "reliability", "warranty", "accelerated_life", "cox_regression",
        "weibayes", "probit_reliability", "km_interval",
    ],
    "DOE": [
        "doe", "rsm", "design_generation", "plackett_burman", "taguchi_orthogonal",
    ],
    "MSA": ["msa"],
    "图形/工具": [
        "eda", "pareto", "run_chart", "cause_effect", "distribution_calculator",
        "power", "bootstrap_mean", "bootstrap_two_sample",
    ],
}


def main() -> None:
    stubs = extract_stubs()
    all_facts = extract_interpretation_facts_fields()
    help_formulas = load_help_formulas()

    by_family: dict[str, list[str]] = defaultdict(list)
    for cid, _, _ in stubs:
        by_family[map_family(cid)].append(cid)

    report_path = ROOT / "tools/_g9_stub_report.json"
    report: dict = {
        "total_stubs": len(stubs),
        "families": {},
        "all_stubs": [{"command_id": c, "title": t, "plain_formula": p} for c, t, p in stubs],
    }

    for fam in [
        "能力/质量", "控制图", "基础统计", "回归/多变量/ML",
        "可靠性", "DOE", "MSA", "图形/工具",
    ]:
        cids = sorted(by_family.get(fam, []))
        relevant = [f for f in FAMILY_FACTS.get(fam, []) if f in all_facts]
        top5 = []
        for cid in cids[:5]:
            formulas = help_formulas.get(cid, [])
            top5.append({
                "command_id": cid,
                "recommended_plain_formula": formulas[0] if formulas else None,
                "all_formula_blocks": formulas[:3],
            })
        report["families"][fam] = {
            "count": len(cids),
            "interpretation_facts_fields": relevant,
            "command_ids": cids,
            "top5_recommendations": top5,
        }

    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Wrote {report_path}")
    print(f"Total stubs: {len(stubs)}")
    for fam, data in report["families"].items():
        print(f"{fam}: {data['count']}")


if __name__ == "__main__":
    main()
