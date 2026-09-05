#!/usr/bin/env python3
"""G-Trust reference-implementation golden gate (Wave-4 full lock).

Evidence: golden <- reference_implementation. Does NOT claim vendor_oracle /
Minitab numerical alignment.

Usage (PowerShell):
  python tools/verify_g_trust_golden_gate.py          # default: all 10 + G12/G13
  python tools/verify_g_trust_golden_gate.py --all    # same as default
  python tools/verify_g_trust_golden_gate.py --wave 1 # construction subset
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
FIXTURES = ROOT / "tests" / "fixtures" / "minitab"
EXPECTED = FIXTURES / "expected"
SCRIPTS = ROOT / "scripts"
BUILD = ROOT / "build-mingw"
RESEARCH = ROOT / "docs" / "research"
DOCS_MATRIX = RESEARCH / "VALIDATION_MATRIX.md"
BACKLOG = RESEARCH / "minitab-market-algorithm-backlog.md"

# Q2A lock table — must stay exactly these 10 ids (Wave plan §1).
LOCKED_COMMAND_IDS: tuple[str, ...] = (
    "imr",
    "xbar_r",
    "capability",
    "capability_sixpack",
    "gage_rr",
    "two_sample_t",
    "normality_test",
    "one_way_anova",
    "p_chart",
    "between_within_capability",
)

WAVE_COMMANDS: dict[int, tuple[str, ...]] = {
    1: ("imr", "xbar_r", "p_chart"),
    2: ("capability", "capability_sixpack", "between_within_capability"),
    3: ("gage_rr", "two_sample_t", "normality_test", "one_way_anova"),
}

# Student-visible / goal paths: forbid completed-alignment claims.
FORBIDDEN_CLAIM_RE = re.compile(
    r"(已与\s*Minitab\s*数值对齐|与\s*Minitab\s*数值完全对齐|已与\s*Minitab\s*一致"
    r"|vendor_oracle\s*对齐|Minitab\s*vendor\s*对齐)",
    re.IGNORECASE,
)

PARALLEL_FIXTURE_RE = re.compile(
    r"fixtures[/\\](g_trust_v2|oracle_v2)|golden_loader2",
    re.IGNORECASE,
)

GREP_ROOTS = (
    ROOT / "docs",
    FIXTURES,
    SCRIPTS,
)

REF_GOLDEN_STATUS = "ref-golden 已冻"
TYPE_MARKER = "golden`←`reference_implementation"
TYPE_MARKER_ALT = "golden` ← `reference_implementation"
TYPE_MARKER_PLAIN = "golden←reference_implementation"


def _safe_print(prefix: str, msg: str) -> None:
    line = f"{prefix}: {msg}"
    try:
        print(line)
    except UnicodeEncodeError:
        print(line.encode("ascii", "backslashreplace").decode("ascii"))


def fail(msg: str, errors: list[str]) -> None:
    _safe_print("FAIL", msg)
    errors.append(msg)


def ok(msg: str) -> None:
    _safe_print("OK", msg)


def required_commands(wave: int | None, all_cmds: bool) -> list[str]:
    """Default / --all / wave>=4 => full lock table of 10."""
    if all_cmds or wave is None or wave >= 4:
        return list(LOCKED_COMMAND_IDS)
    cmds: list[str] = []
    for w in range(1, wave + 1):
        cmds.extend(WAVE_COMMANDS.get(w, ()))
    return cmds


def check_lock_table(errors: list[str]) -> None:
    if len(LOCKED_COMMAND_IDS) != 10:
        fail(f"G1 lock table size={len(LOCKED_COMMAND_IDS)} (want 10)", errors)
        return
    if len(set(LOCKED_COMMAND_IDS)) != 10:
        fail("G1 lock table has duplicate command_id", errors)
        return
    ok(f"G1 lock table 10 ids: {', '.join(LOCKED_COMMAND_IDS)}")


def check_paths(commands: list[str], errors: list[str], strict_tsv_meta: bool) -> None:
    for cid in commands:
        expected = EXPECTED / f"{cid}_ref_golden.tsv"
        script = SCRIPTS / f"g_trust_{cid}_reference.py"
        if not expected.is_file():
            fail(f"G2 missing expected: {expected.relative_to(ROOT)}", errors)
        else:
            ok(f"G2 exists {expected.relative_to(ROOT)}")
            if strict_tsv_meta:
                text = expected.read_text(encoding="utf-8")
                if "# source:" not in text or "reference_implementation" not in text:
                    fail(
                        f"G3 {expected.name}: need # source: … reference_implementation",
                        errors,
                    )
                elif "vendor_oracle" in text.split("# source:")[1].splitlines()[0].lower():
                    fail(f"G3 {expected.name}: source must not claim vendor_oracle", errors)
                else:
                    ok(f"G3 source ok {expected.name}")
                cfg = f"command_id={cid}"
                if f"# config: {cfg}" not in text and f"command_id={cid}" not in text:
                    fail(f"G4 {expected.name}: missing command_id={cid} in config", errors)
                else:
                    ok(f"G4 config command_id ok {expected.name}")
        if not script.is_file():
            fail(f"G5 missing script: {script.relative_to(ROOT)}", errors)
        else:
            ok(f"G5 exists {script.relative_to(ROOT)}")


def check_docs_registry(errors: list[str]) -> None:
    export_guide = EXPECTED / "EXPORT_GUIDE.md"
    source_md = FIXTURES / "SOURCE.md"
    fixture_matrix = FIXTURES / "VALIDATION_MATRIX.md"
    for path, label in (
        (export_guide, "EXPORT_GUIDE"),
        (source_md, "SOURCE.md"),
        (fixture_matrix, "fixtures VALIDATION_MATRIX"),
    ):
        if not path.is_file():
            fail(f"missing {label}: {path}", errors)
            continue
        text = path.read_text(encoding="utf-8")
        missing = [cid for cid in LOCKED_COMMAND_IDS if cid not in text]
        if missing:
            fail(f"G6/G7/G9 {label} missing lock ids: {', '.join(missing)}", errors)
        else:
            ok(f"G6/G7/G9 {label} lists all 10 command_id")
        # Q1B: must not say QSKIP completes ref-golden
        if "只有实际从 Minitab 导出的结果才应作为最终 golden" in text:
            fail(f"G9 stale QSKIP/vendor-final narrative in {label}", errors)
        # Wave-4: fixture matrix / EXPORT should show frozen status for lock table
        if label in {"EXPORT_GUIDE", "fixtures VALIDATION_MATRIX"}:
            if text.count(REF_GOLDEN_STATUS) < 10:
                fail(
                    f"G9 {label}: need >=10 '{REF_GOLDEN_STATUS}' markers "
                    f"(found {text.count(REF_GOLDEN_STATUS)})",
                    errors,
                )
            else:
                ok(f"G9 {label} has >=10 '{REF_GOLDEN_STATUS}' markers")


def check_research_validation_matrix(errors: list[str]) -> None:
    """G12: docs/research VALIDATION_MATRIX has 10 rows, correct type, frozen status."""
    if not DOCS_MATRIX.is_file():
        fail(f"G12 missing {DOCS_MATRIX.relative_to(ROOT)}", errors)
        return
    text = DOCS_MATRIX.read_text(encoding="utf-8")
    # Isolate G-Trust section when present
    section = text
    marker = "## G-Trust"
    if marker in text:
        section = text.split(marker, 1)[1]
        # cut at next ## if any
        nxt = section.find("\n## ")
        if nxt != -1:
            section = section[:nxt]

    missing_ids: list[str] = []
    bad_type: list[str] = []
    bad_status: list[str] = []
    for cid in LOCKED_COMMAND_IDS:
        rows = [
            ln
            for ln in section.splitlines()
            if f"`{cid}`" in ln and "|" in ln and "command_id" not in ln
        ]
        if not rows:
            missing_ids.append(cid)
            continue
        row = rows[0]
        cells = [c.strip() for c in row.split("|")]
        nonempty = [c for c in cells if c]
        # command_id | expected | type | status
        type_cell = nonempty[2] if len(nonempty) >= 4 else ""
        status_cell = nonempty[3] if len(nonempty) >= 4 else ""
        type_ok = (
            TYPE_MARKER in type_cell
            or TYPE_MARKER_ALT in type_cell
            or TYPE_MARKER_PLAIN in type_cell.replace(" ", "")
            or (
                "golden" in type_cell.replace(" ", "")
                and "reference_implementation" in type_cell
                and "vendor_oracle" not in type_cell
            )
        )
        if not type_ok:
            bad_type.append(cid)
        if REF_GOLDEN_STATUS not in status_cell:
            bad_status.append(cid)

    if missing_ids:
        fail(f"G12 VALIDATION_MATRIX missing rows: {', '.join(missing_ids)}", errors)
    if bad_type:
        fail(
            f"G12 type must be golden<-reference_implementation (not vendor): "
            f"{', '.join(bad_type)}",
            errors,
        )
    if bad_status:
        fail(
            f"G12 status must be '{REF_GOLDEN_STATUS}': {', '.join(bad_status)}",
            errors,
        )
    if not missing_ids and not bad_type and not bad_status:
        ok(
            f"G12 VALIDATION_MATRIX: 10 rows, type golden<-reference_implementation, "
            f"status '{REF_GOLDEN_STATUS}'"
        )


def check_backlog_ref_golden(errors: list[str]) -> None:
    """G13: backlog notes ref-golden for each lock id; no fake vendor promotion."""
    if not BACKLOG.is_file():
        fail(f"G13 missing {BACKLOG.relative_to(ROOT)}", errors)
        return
    text = BACKLOG.read_text(encoding="utf-8")
    missing: list[str] = []
    fake_vendor: list[str] = []
    for cid in LOCKED_COMMAND_IDS:
        lines = [ln for ln in text.splitlines() if f"`{cid}`" in ln]
        hit = [ln for ln in lines if "ref-golden" in ln]
        if not hit:
            missing.append(cid)
            continue
        for ln in hit:
            # Forbid implying vendor alignment on the same note line
            if FORBIDDEN_CLAIM_RE.search(ln) and "≠" not in ln and "禁止" not in ln:
                # Allow negation via ≠ vendor_oracle
                if "vendor_oracle" in ln and ("≠" in ln or "非" in ln or "不是" in ln):
                    continue
                if "≠ vendor" in ln or "≠vendor" in ln.replace(" ", ""):
                    continue
                fake_vendor.append(cid)
            # Do not allow rewriting ⚪→✅ on same line solely as vendor claim;
            # allow ✅ with explicit ref-golden note (depth闭环 + ref freeze).
            if "vendor_oracle" in ln and "ref-golden" in ln:
                if not any(m in ln for m in ("≠", "非", "不是", "禁止", "NOT")):
                    fake_vendor.append(cid)
    if missing:
        fail(
            f"G13 backlog missing ref-golden note for: {', '.join(missing)}",
            errors,
        )
    if fake_vendor:
        fail(
            f"G13 backlog pseudo-vendor claim near: {', '.join(sorted(set(fake_vendor)))}",
            errors,
        )
    if not missing and not fake_vendor:
        ok("G13 backlog: all 10 command_ids have ref-golden notes (no pseudo-vendor)")


def check_forbidden_claims(errors: list[str]) -> None:
    """G10: flag affirmative vendor-alignment claims; skip honest denials/examples."""
    hits: list[str] = []
    negation_markers = (
        "禁止",
        "不得",
        "不要",
        "不是",
        "不声称",
        "不存在",
        "也不",
        "不可以",
        "非 vendor",
        "NOT vendor",
        "说成",
        "冒充",
        "禁止写成",
        "禁止声称",
        "写「",
        "写成",
        "⇒「",
        "->「",
        "示例",
        "≠",
    )
    for root in GREP_ROOTS:
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix.lower() not in {".md", ".py", ".tsv", ".txt", ".cpp", ".h"}:
                continue
            try:
                text = path.read_text(encoding="utf-8", errors="replace")
            except OSError:
                continue
            for i, line in enumerate(text.splitlines(), 1):
                if "FORBIDDEN_CLAIM_RE" in line or "forbid completed-alignment" in line:
                    continue
                match = FORBIDDEN_CLAIM_RE.search(line)
                if not match:
                    continue
                prefix = line[: match.start()]
                if any(marker in line for marker in negation_markers):
                    continue
                if any(marker in prefix for marker in ("不", "禁", "非", "勿")):
                    continue
                safe = line.strip()[:120].encode("ascii", "backslashreplace").decode("ascii")
                hits.append(f"{path.relative_to(ROOT)}:{i}: {safe}")
    if hits:
        for h in hits[:20]:
            fail(f"G10 vendor claim: {h}", errors)
    else:
        ok("G10 no forbidden vendor-alignment claims in docs/fixtures/scripts")


def check_no_parallel_fixtures(errors: list[str]) -> None:
    bad_dirs = [
        p
        for p in (ROOT / "tests" / "fixtures").iterdir()
        if p.is_dir() and p.name in {"g_trust_v2", "oracle_v2"}
    ]
    if bad_dirs:
        fail(f"G11 parallel fixture dirs: {bad_dirs}", errors)
        return
    sample = (ROOT / "scripts").glob("g_trust_*.py")
    for path in sample:
        text = path.read_text(encoding="utf-8", errors="replace")
        if PARALLEL_FIXTURE_RE.search(text):
            fail(f"G11 parallel fixture path in {path.name}", errors)
            return
    ok("G11 no parallel g_trust_v2 / oracle_v2 fixture universe")


def maybe_run_cpp(errors: list[str]) -> None:
    """Optional: run C++ golden test if exe exists (not a Wave-4 G12/G13 item)."""
    exe = BUILD / "minitab_numerical_golden_test.exe"
    if not exe.is_file():
        print(
            "INFO optional-cpp: minitab_numerical_golden_test.exe not in build-mingw — "
            "user should build that target in Qt Creator (not a gate failure)."
        )
        return
    import subprocess

    print(f">> {exe}")
    rc = subprocess.run([str(exe)], cwd=str(BUILD)).returncode
    if rc != 0:
        fail(f"optional-cpp minitab_numerical_golden_test exited {rc}", errors)
    else:
        ok("optional-cpp minitab_numerical_golden_test PASS")


def main() -> int:
    parser = argparse.ArgumentParser(description="G-Trust golden gate")
    parser.add_argument(
        "--wave",
        type=int,
        default=None,
        help="Require expected/scripts for waves 1..N. Omit or use 4 for full lock (default).",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Require all 10 lock-table expected + scripts (same as default).",
    )
    parser.add_argument(
        "--run-cpp",
        action="store_true",
        help="If exe exists, run minitab_numerical_golden_test.",
    )
    args = parser.parse_args()
    errors: list[str] = []

    check_lock_table(errors)
    # Default (no --wave) and --all => full 10; --wave 1|2|3 => subset for construction.
    all_cmds = args.all or args.wave is None
    cmds = required_commands(args.wave, all_cmds)
    wave_label = "all" if all_cmds else str(args.wave)
    print(f"Checking commands for wave={wave_label}: {', '.join(cmds)}")
    check_paths(cmds, errors, strict_tsv_meta=True)
    check_docs_registry(errors)
    check_forbidden_claims(errors)
    check_no_parallel_fixtures(errors)

    # Wave-4 registry gates always run (full lock documentation).
    check_research_validation_matrix(errors)
    check_backlog_ref_golden(errors)

    if args.run_cpp:
        maybe_run_cpp(errors)
    else:
        exe = BUILD / "minitab_numerical_golden_test.exe"
        if exe.is_file():
            print(
                f"INFO optional-cpp: {exe.name} present — pass --run-cpp to execute "
                "(optional; Chinese-path cmake not required)."
            )
        else:
            print(
                "INFO optional-cpp: minitab_numerical_golden_test.exe not in build-mingw — "
                "build that target in Qt Creator when ready."
            )

    if errors:
        print(f"\nG-Trust golden gate: {len(errors)} failure(s)")
        return 1
    print("\nG-Trust golden gate: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
