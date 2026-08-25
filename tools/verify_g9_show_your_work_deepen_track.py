#!/usr/bin/env python3
"""Preflight for G9-D Show Your Work Depth track."""

from __future__ import annotations

import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED = [
    "docs/research/g9-show-your-work-depth-matrix.md",
    "docs/research/goal-wave-2026-08-24-g9-show-your-work-deepen.md",
    "src/application/computation_trace_attach_deep.cpp",
    "src/application/computation_trace_attach_deep.h",
    "src/application/computation_trace_helpers.cpp",
    "tests/g9_show_your_work_deepen_track_test.cpp",
]

PILOTS = ("capability", "one_sample_t", "weibayes", "imr", "regression", "gage_rr")
EXEMPT = {"tests", "rule_policy"}
L2_EXCEPTIONS = {
    "bootstrap_mean",
    "bootstrap_two_sample",
    "random_forest",
    "cart_tree",
    "isolation_forest",
    "stepwise_regression",
    "best_subsets_regression",
    "arima",
    "time_series_smoothing",
    "time_series_decomposition",
    "seasonal_forecasting",
    "acf_pacf",
    "ccf",
    "correlogram",
    "cluster_observations",
    "km_interval",
}
D_CLASS_DEPTH = "L1"
E_CLASS_DEPTH = "L0"
A_CLASS_FAMILIES = {
    "能力/质量",
    "控制图",
    "基础统计",
    "回归/多变量/ML",
    "可靠性",
    "DOE",
    "MSA",
}


def fail(msg: str) -> None:
    print(f"FAIL: {msg}")
    sys.exit(1)


def ok(msg: str) -> None:
    print(f"OK: {msg}")


def list_command_ids() -> list[str]:
    out = subprocess.check_output(
        [sys.executable, str(ROOT / "tools/_list_command_ids.py")],
        cwd=ROOT,
        text=True,
        encoding="utf-8",
    )
    ids: list[str] = []
    started = False
    for line in out.splitlines():
        if line.startswith("count="):
            started = True
            continue
        if started and line.strip():
            ids.append(line.strip())
    return ids


def parse_depth_matrix() -> dict[str, dict[str, str]]:
    text = (ROOT / "docs/research/g9-show-your-work-depth-matrix.md").read_text(
        encoding="utf-8"
    )
    rows: dict[str, dict[str, str]] = {}
    for m in re.finditer(
        r"^\|\s*([a-z0-9_]+)\s*\|\s*([^|]+)\|\s*(L[0-3])\s*\|\s*([^|]+)\|",
        text,
        flags=re.M,
    ):
        rows[m.group(1).strip()] = {
            "family": m.group(2).strip(),
            "depth": m.group(3).strip(),
            "notes": m.group(4).strip(),
        }
    return rows


def main() -> None:
    for rel in REQUIRED:
        if not (ROOT / rel).is_file():
            fail(f"missing {rel}")
        ok(rel)

    types = (ROOT / "src/domain/quality_types.h").read_text(encoding="utf-8")
    for field in (
        "expression_before",
        "expression_after",
        "int order",
    ):
        if field not in types:
            fail(f"ComputationStep missing {field}")
    ok("ComputationStep extended fields")

    ser = (ROOT / "src/infrastructure/output_serialization.cpp").read_text(encoding="utf-8")
    for token in ("expression_before", "expression_after", '"order"'):
        if token not in ser:
            fail(f"serialization missing {token}")
    ok("serialization round-trip fields")

    ui = (ROOT / "src/ui/formula_substitution_dialog.cpp").read_text(encoding="utf-8")
    for marker in ("分步求值", "代入前", "代入后", "得数"):
        if marker not in ui:
            fail(f"UI page3 missing {marker}")
    if "QStackedWidget" not in ui:
        fail("UI missing stacked pages")
    ok("UI page3 step table")

    src_dir = ROOT / "src"
    stub_hits = []
    for path in src_dir.rglob("*"):
        if path.suffix not in {".cpp", ".h"}:
            continue
        if "主公式" in path.read_text(encoding="utf-8"):
            stub_hits.append(str(path.relative_to(ROOT)))
    if stub_hits:
        fail(f'主公式 stub in src: {stub_hits[:5]}')
    ok("src 主公式 stub count = 0")

    attach = (ROOT / "src/application/computation_trace_attach.cpp").read_text(
        encoding="utf-8"
    )
    deep = (ROOT / "src/application/computation_trace_attach_deep.cpp").read_text(
        encoding="utf-8"
    )
    attach_sources = attach + deep
    if "attach_deep_trace" not in attach:
        fail("attach.cpp missing attach_deep_trace dispatch")
    if "attach_deep_trace" not in deep:
        fail("deep module missing attach_deep_trace")
    ok("deep dispatch wired")

    ids = list_command_ids()
    matrix = parse_depth_matrix()
    gaps = [cid for cid in ids if cid not in matrix]
    if gaps:
        fail(f"depth matrix gaps ({len(gaps)}): {gaps[:10]}")
    ok(f"depth matrix covers {len(ids)} commands")

    for cid, row in matrix.items():
        depth = row["depth"]
        if depth not in {"L0", "L1", "L2", "L3"}:
            fail(f"invalid depth for {cid}: {depth}")
        if cid in EXEMPT and depth != "L0":
            fail(f"exempt {cid} must be L0")
        if depth == "L3":
            if cid in PILOTS or cid in {
                "two_sample_t",
                "paired_t",
                "imr",
                "xbar_r",
                "p_chart",
            }:
                continue
            if f'try_attach_{cid}' not in deep and f'"{cid}"' not in attach_sources:
                fail(f"L3 command {cid} not referenced in attach/deep")

    a_l3 = [
        cid
        for cid, row in matrix.items()
        if row["family"] in A_CLASS_FAMILIES and row["depth"] == "L3"
    ]
    a_total = [
        cid
        for cid, row in matrix.items()
        if row["family"] in A_CLASS_FAMILIES and row["depth"] != "L0"
    ]
    l2_a_exceptions = [
        cid
        for cid, row in matrix.items()
        if row["family"] in A_CLASS_FAMILIES and row["depth"] == "L2"
    ]
    l2_allowed = set(L2_EXCEPTIONS)
    # B/C design/tool commands intentionally L2 per research §2.1
    b_c_l2_ok = {
        "anom", "multi_vari", "acceptance_sampling", "distribution_identification",
        "box_cox", "doe_plackett_burman", "doe_ccd", "doe_bbd", "response_optimization",
        "pca", "kmeans", "discriminant", "km_interval",
    }
    pure_a = [
        cid
        for cid, row in matrix.items()
        if row["family"] in A_CLASS_FAMILIES
        and row["depth"] in {"L2", "L3"}
        and cid not in b_c_l2_ok
    ]
    unauthorized_l2 = [
        cid
        for cid in pure_a
        if matrix[cid]["depth"] == "L2" and cid not in l2_allowed
    ]
    if pure_a and len(unauthorized_l2) / len(pure_a) > 0.15:
        fail(
            f"unauthorized A-class L2 {len(unauthorized_l2)}/{len(pure_a)}: "
            f"{unauthorized_l2[:8]}"
        )
    ok(
        f"A-class L3 {len(a_l3)}/{len(a_total)}; "
        f"L2 exceptions {len(l2_a_exceptions)} (allowed {len(l2_allowed)})"
    )

    dod = (ROOT / "docs/research/goal-wave-2026-08-24-g9-show-your-work-deepen.md").read_text(
        encoding="utf-8"
    )
    unchecked = re.findall(r"^- \[ \] \*\*SYW-", dod, flags=re.M)
    if unchecked:
        fail(f"DoD unchecked SYW items: {len(unchecked)}")
    ok("DoD SYW-A..J checked")

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    for token in (
        "computation_trace_attach_deep",
        "computation_trace_helpers",
        "g9_show_your_work_deepen_track_test",
    ):
        if token not in cmake:
            fail(f"CMakeLists missing {token}")
    ok("CMake deepen sources + test")

    for pilot in PILOTS:
        if pilot not in attach and pilot not in deep:
            fail(f"pilot missing: {pilot}")
    ok(f"pilots {len(PILOTS)} referenced")

    print("PASS: verify_g9_show_your_work_deepen_track")


if __name__ == "__main__":
    main()
