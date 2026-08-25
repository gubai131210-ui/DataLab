#!/usr/bin/env python3
"""One-shot patch: fill menu_path + menu_group on AnalysisCommand initializers
and align algorithm_help.json menu_path to \"{path} > {group}\".
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "src" / "ui" / "analysis_commands.cpp"
HELP = ROOT / "resources" / "help" / "algorithm_help.json"
MAP = ROOT / "docs" / "research" / "ui-menu-ia-command-taxonomy-map-2026-08-23.md"

# Explicit Wave corrections (must win if map ever drifts).
OVERRIDE: dict[str, tuple[str, str]] = {
    "cox_regression": ("统计", "可靠性"),
    "weibayes": ("统计", "可靠性"),
    "random_forest": ("统计", "多变量"),
    "taguchi_orthogonal_design": ("统计", "DOE"),
    "distribution_calculator": ("统计", "推断 / 仿真"),
    "pareto": ("质量工具", "质量图 / 规划"),
    "doe_factorial": ("统计", "DOE"),
    "doe_response": ("统计", "DOE"),
    "response_optimization": ("统计", "DOE"),
    "rsm_response": ("统计", "DOE"),
    "doe_plackett_burman": ("统计", "DOE"),
    "doe_ccd": ("统计", "DOE"),
    "doe_bbd": ("统计", "DOE"),
}


def load_taxonomy(path: Path) -> dict[str, tuple[str, str]]:
    text = path.read_text(encoding="utf-8")
    rows: dict[str, tuple[str, str]] = {}
    for line in text.splitlines():
        m = re.match(r"^\|\s*([a-z0-9_]+)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|$", line)
        if not m:
            continue
        cid, menu_path, menu_group = (g.strip() for g in m.groups())
        if cid == "id":
            continue
        rows[cid] = (menu_path, menu_group)
    for cid, pair in OVERRIDE.items():
        rows[cid] = pair
    return rows


CMD_START = re.compile(
    r"(?P<head>\n        \{\n            QStringLiteral\(\"(?P<id>[a-z0-9_]+)\"\),\n"
    r"            QStringLiteral\(\"[^\"]*\"\),\n"
    r"            QStringLiteral\(\"[^\"]*\"\),\n"
    r"            )QStringLiteral\(\"(?P<path>[^\"]*)\"\)(?P<tail>,\n)"
)


def find_command_end(text: str, start_idx: int) -> int:
    """Return index of the closing `}` of the AnalysisCommand aggregate that
    begins at start_idx (the `{` after leading whitespace/newline).
    """
    # start_idx should point at '{'
    assert text[start_idx] == "{", repr(text[start_idx : start_idx + 20])
    depth = 0
    i = start_idx
    n = len(text)
    in_str = False
    str_delim = ""
    escape = False
    while i < n:
        ch = text[i]
        if in_str:
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == str_delim:
                in_str = False
            i += 1
            continue
        if ch in ('"', "'"):
            # C++ raw/char/string — treat " and ' as string-ish for brace skip
            in_str = True
            str_delim = ch
            i += 1
            continue
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise RuntimeError("unclosed command brace")


def patch_cpp(text: str, taxonomy: dict[str, tuple[str, str]]) -> tuple[str, list[str], list[str]]:
    """Return (new_text, patched_ids, missing_ids)."""
    # Work from end to start so indices stay valid.
    matches = list(CMD_START.finditer(text))
    patched: list[str] = []
    missing: list[str] = []
    # Deduplicate by occurrence order; each match is one command.
    edits: list[tuple[int, int, str]] = []  # start, end, replacement for whole command slice

    for m in matches:
        cid = m.group("id")
        if cid not in taxonomy:
            missing.append(cid)
            continue
        path, group = taxonomy[cid]
        # Locate the opening `{` of this command (right after the leading newline+spaces).
        brace_pos = m.start() + len("\n        ")
        if text[brace_pos] != "{":
            # Fallback: find '{' near match start
            brace_pos = text.find("{", m.start())
        end_brace = find_command_end(text, brace_pos)

        # Slice of the full command including opening `{` through closing `}`
        # (comma after may follow).
        # Rewrite menu_path (field already captured by regex within head..path).
        # And ensure run ends with `, QStringLiteral("<group>")` before closing `}`.

        # Rebuild from match: replace path in the matched prefix, then fix ending.
        old_path = m.group("path")
        prefix_end = m.end()  # after path comma+newline
        # New prefix with corrected path
        new_prefix = (
            m.group("head")
            + f'QStringLiteral("{path}")'
            + m.group("tail")
        )

        # Body between end of path-line and closing brace
        body = text[prefix_end:end_brace]
        # Closing `}` currently sits right after the run expression.
        # Strip optional existing menu_group if re-run:
        #   ,\n            QStringLiteral("...")
        body_stripped = re.sub(
            r",\n            QStringLiteral\(\"[^\"]*\"\)\s*$",
            "",
            body.rstrip(),
        )
        # Also handle already-patched form without needing strip if run ends with `},`
        # body ends with run expr — may end with `AnalysisService::foo` or `doe_run`
        # Ensure we add comma + menu_group
        if body_stripped.endswith(","):
            # unexpected trailing comma after run
            body_stripped = body_stripped.rstrip().rstrip(",")

        new_body = (
            body_stripped
            + ",\n            "
            + f'QStringLiteral("{group}")'
        )
        new_cmd = new_prefix[len("\n        ") :]  # drop leading newline+spaces for splice?
        # Actually m.start() includes `\n        `, and brace is at brace_pos.
        # Safer: replace from brace_pos through end_brace inclusive.
        # Reconstruct full command content:
        # text[brace_pos:prefix_end] starts with `{` then fields through path line.
        # We need: `{` + id/label/title/path... from new_prefix without the leading `\n        `
        # new_prefix = `\n            QStringLiteral("id"),...path...,\n` — wait, head includes
        # `\n        {\n            QStringLiteral("id"),\n...`
        # So new_prefix starts with newline+spaces+`{`.

        assert new_prefix.lstrip("\n ").startswith("{") or new_prefix.startswith("\n        {")
        # Full replacement from m.start() to end_brace inclusive:
        replacement = new_prefix + new_body + "}"
        # But new_prefix already includes from `\n        {` ... so replacement starts at m.start()
        edits.append((m.start(), end_brace + 1, replacement))
        patched.append(cid)
        if old_path != path:
            print(f"  path {cid}: {old_path!r} -> {path!r}")

    # Apply edits reverse
    out = text
    for start, end, repl in sorted(edits, key=lambda t: t[0], reverse=True):
        out = out[:start] + repl + out[end:]

    expected = set(taxonomy)
    found = set(patched)
    missing_expected = sorted(expected - found)
    return out, patched, missing_expected


def verify(cpp_text: str, taxonomy: dict[str, tuple[str, str]]) -> None:
    matches = list(CMD_START.finditer(cpp_text))
    print(f"verify: found {len(matches)} command starts")
    bad: list[str] = []
    groups: dict[str, str] = {}
    paths: dict[str, str] = {}
    for m in matches:
        cid = m.group("id")
        paths[cid] = m.group("path")
        brace_pos = m.start() + len("\n        ")
        if cpp_text[brace_pos] != "{":
            brace_pos = cpp_text.find("{", m.start())
        end_brace = find_command_end(cpp_text, brace_pos)
        chunk = cpp_text[brace_pos : end_brace + 1]
        gm = re.search(
            r",\n            QStringLiteral\(\"([^\"]+)\"\)\s*\}$",
            chunk,
        )
        if not gm:
            bad.append(cid)
        else:
            groups[cid] = gm.group(1)

    print(f"verify: groups filled {len(groups)} / {len(matches)}")
    if bad:
        print("MISSING menu_group:", bad)

    missing = sorted(set(taxonomy) - set(groups))
    extra = sorted(set(groups) - set(taxonomy))
    if missing:
        print("TAXONOMY ids not in cpp:", missing)
    if extra:
        print("CPP ids not in taxonomy:", extra)

    mismatches = []
    for cid, (ep, eg) in taxonomy.items():
        if cid not in groups:
            continue
        if paths.get(cid) != ep or groups[cid] != eg:
            mismatches.append((cid, paths.get(cid), groups[cid], ep, eg))
    if mismatches:
        print("MISMATCHES:")
        for row in mismatches:
            print(" ", row)
    else:
        print("verify: all matched taxonomy (incl. overrides)")

    cox = paths.get("cox_regression")
    print(f"verify: cox_regression menu_path = {cox!r}")
    assert cox == "统计", cox
    assert len(groups) == 137, len(groups)
    assert not bad and not missing and not mismatches


def main() -> int:
    taxonomy = load_taxonomy(MAP)
    print(f"taxonomy: {len(taxonomy)} ids")
    if len(taxonomy) != 137:
        print("WARNING: expected 137, got", len(taxonomy))
        # still proceed

    text = CPP.read_text(encoding="utf-8")
    new_text, patched, missing = patch_cpp(text, taxonomy)
    print(f"patched {len(patched)} commands; missing from cpp: {missing}")
    CPP.write_text(new_text, encoding="utf-8", newline="\n")

    print("NOTE: run tools/_patch_help_menu_path.py for surgical help.json updates")
    verify(CPP.read_text(encoding="utf-8"), taxonomy)
    return 0


if __name__ == "__main__":
    sys.exit(main())
