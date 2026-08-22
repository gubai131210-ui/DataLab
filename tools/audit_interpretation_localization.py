#!/usr/bin/env python3
"""Audit interpretation bullets vs report_localization handlers.

Static scan of interpretation_service.cpp bullets.push_back strings against
handlers in report_localization.cpp. Dynamic bullets (prefix + runtime values)
are matched via starts_with patterns from else-if chains.

Exit 0 when all candidates covered; exit 1 when uncovered remain.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys
from collections import Counter

ROOT = pathlib.Path(__file__).resolve().parents[1]
LOC = ROOT / "src/application/report_localization.cpp"
INTERP = ROOT / "src/application/interpretation_service.cpp"

MIN_PREFIX = 4


def load_exact_pairs(text: str) -> set[str]:
    return {
        zh
        for zh, _ in re.findall(
            r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)+)"\s*\}', text
        )
        if re.search(r"[\u4e00-\u9fff]", zh)
    }


def load_starts_with_prefixes(text: str) -> set[str]:
    prefixes: set[str] = set()
    for pat in (
        r'starts_with\(\s*(?:bullet|body)\s*,\s*"((?:[^"\\]|\\.)+)"',
        r'parse_leading_count_after_prefix\(\s*bullet\s*,\s*"((?:[^"\\]|\\.)+)"',
        r'else if \(starts_with\(bullet,\s*"((?:[^"\\]|\\.)+)"\)\)',
    ):
        for m in re.findall(pat, text):
            if len(m) >= MIN_PREFIX and (
                re.search(r"[\u4e00-\u9fff]", m) or m.startswith(("RSM ", "ANOM "))
            ):
                prefixes.add(m)
    return prefixes


def load_exact_equality_bullets(text: str) -> set[str]:
    return {
        m
        for m in re.findall(
            r'(?:bullet|body)\s*==\s*"((?:[^"\\]|\\.)+)"', text
        )
        if re.search(r"[\u4e00-\u9fff]", m)
    }


def load_find_substrings(text: str) -> set[str]:
    return {
        m
        for m in re.findall(r'(?:bullet|body)\.find\(\s*"((?:[^"\\]|\\.)+)"', text)
        if re.search(r"[\u4e00-\u9fff]", m) and len(m) >= MIN_PREFIX
    }


def load_ends_with_suffixes(text: str) -> set[str]:
    return {
        m
        for m in re.findall(r'ends_with\(\s*bullet\s*,\s*"((?:[^"\\]|\\.)+)"', text)
        if re.search(r"[\u4e00-\u9fff]", m) and len(m) >= MIN_PREFIX
    }


def _joined_literals(fragment: str) -> str:
    return "".join(re.findall(r'"((?:[^"\\]|\\.)*)"', fragment))


def extract_bullet_candidates(lines: list[str]) -> list[tuple[int, str]]:
    """One candidate per push_back; concatenate string literals in the block."""
    out: list[tuple[int, str]] = []
    i = 0
    while i < len(lines):
        if "bullets.push_back" not in lines[i]:
            i += 1
            continue
        buf = lines[i]
        j = i
        while j < len(lines) and ");" not in lines[j]:
            j += 1
            if j < len(lines):
                buf += "\n" + lines[j]
        joined = _joined_literals(buf)
        if re.search(r"[\u4e00-\u9fff]", joined):
            out.append((i + 1, joined))
        i = j + 1
    return out


def is_known_dynamic(
    text: str,
    exact: set[str],
    prefixes: set[str],
    finds: set[str],
    ends: set[str],
    equals: set[str],
) -> bool:
    if cover(text, exact, prefixes, finds, ends, equals):
        return True
    for clause in exact:
        if len(clause) >= 16 and clause in text:
            return True
    return False


def cover(
    text: str,
    exact: set[str],
    prefixes: set[str],
    finds: set[str],
    ends: set[str],
    equals: set[str],
) -> str | None:
    if text in exact or text in equals:
        return "exact"
    for clause in sorted(exact, key=len, reverse=True):
        if len(clause) >= 24 and clause in text:
            return "exact_clause"
    for prefix in sorted(prefixes, key=len, reverse=True):
        if text.startswith(prefix):
            return "starts_with"
    if text.endswith("：") and len(text) >= MIN_PREFIX:
        for prefix in sorted(prefixes, key=len, reverse=True):
            if text.startswith(prefix) or prefix.startswith(text):
                return "starts_with_prefix"
    for sub in sorted(finds, key=len, reverse=True):
        if sub in text:
            return "find"
    for suffix in sorted(ends, key=len, reverse=True):
        if text.endswith(suffix):
            return "ends_with"
    return None


def family(text: str) -> str:
    keywords = [
        ("msa", ["量具", "Gage", "ndc", "偏倚", "Wheeler", "EMP"]),
        ("doe_rsm", ["DOE", "Pareto", "等值线", "Desirability", "因子", "中心点", "失拟", "CCD", "BBD"]),
        ("reliability", ["寿命", "Weibull", "删失", "KM", "CIF", "Fine-Gray", "保证", "可靠", "指数"]),
        ("capability", ["能力", "Cp", "Pp", "规格", "门禁"]),
        ("spc", ["控制图", "规则「", "特殊原因", "I-MR"]),
    ]
    for name, kws in keywords:
        if any(k in text for k in kws):
            return name
    return "other"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--write-report",
        type=pathlib.Path,
        default=ROOT / "scripts/_tmp_phase3_interp_strict.txt",
        help="Optional report path (default: scripts/_tmp_phase3_interp_strict.txt)",
    )
    args = parser.parse_args()

    loc_text = LOC.read_text(encoding="utf-8")
    interp_lines = INTERP.read_text(encoding="utf-8").splitlines()

    exact = load_exact_pairs(loc_text)
    prefixes = load_starts_with_prefixes(loc_text)
    finds = load_find_substrings(loc_text)
    ends = load_ends_with_suffixes(loc_text)
    equals = load_exact_equality_bullets(loc_text)

    cands = extract_bullet_candidates(interp_lines)
    unc: list[tuple[int, str]] = []
    for ln, text in cands:
        if not is_known_dynamic(text, exact, prefixes, finds, ends, equals):
            unc.append((ln, text))

    cf = Counter(family(t) for _, t in unc)
    report_lines = [
        f"cands={len(cands)} uncovered={len(unc)}",
        *(f"  {v}\t{k}" for k, v in cf.most_common()),
        *(f"L{ln}\t{family(t)}\t{t}" for ln, t in unc),
    ]
    args.write_report.parent.mkdir(parents=True, exist_ok=True)
    args.write_report.write_text("\n".join(report_lines) + "\n", encoding="utf-8")

    print(f"covered={len(cands) - len(unc)}/{len(cands)} uncovered={len(unc)}")
    if unc:
        for ln, t in unc[:12]:
            print(f"  L{ln} [{family(t)}] {t[:90]}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
