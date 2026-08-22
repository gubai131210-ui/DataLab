#!/usr/bin/env python3
"""Sync report_text_catalog.cpp + ui_menu_strings.json → JSON/.ts/(optional).qm.

Authority: report strings → src/domain/report_text_catalog.cpp (ADR 0009);
UI chrome/menus → translations/ui_menu_strings.json (context DataLabUi).
Does not invent vendor_oracle or golden evidence.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import xml.sax.saxutils as saxutils
from pathlib import Path

ENTRY_RE = re.compile(
    r'\{\s*"([^"]+)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}',
    re.MULTILINE,
)


def unescape_cpp_string(text: str) -> str:
    return (
        text.replace(r"\\", "\\")
        .replace(r"\"", '"')
        .replace(r"\n", "\n")
        .replace(r"\t", "\t")
    )


def parse_catalog(cpp_path: Path) -> list[dict[str, str]]:
    root = cpp_path.resolve().parents[2]
    domain = root / "src" / "domain"
    part_paths = sorted(domain.glob("report_text_catalog_part*.cpp"))
    paths = part_paths if part_paths else [cpp_path]
    entries: list[dict[str, str]] = []
    seen: set[str] = set()
    for path in paths:
        raw = path.read_text(encoding="utf-8")
        for match in ENTRY_RE.finditer(raw):
            text_id, zh_cn, en_us = match.groups()
            if text_id in seen:
                continue
            seen.add(text_id)
            entries.append(
                {
                    "id": text_id,
                    "zh_cn": unescape_cpp_string(zh_cn),
                    "en_us": unescape_cpp_string(en_us),
                }
            )
    if not entries:
        raise SystemExit(f"No catalog entries parsed from {paths}")
    return entries


def write_json(path: Path, entries: list[dict[str, str]]) -> None:
    payload = {
        "catalog_version": "2026-08-21.phase3-linguist",
        "authority": "src/domain/report_text_catalog.cpp",
        "note": (
            "Mirror of report_text_catalog for Linguist/.qm packaging. "
            "UI default remains zh-CN (ADR 0006/0009). "
            "Do not treat this JSON as vendor_oracle or golden."
        ),
        "languages": ["zh-CN", "en-US"],
        "entries": entries,
    }
    path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


def write_ts(
    path: Path,
    report_entries: list[dict[str, str]],
    ui_entries: list[dict[str, str]],
    language: str,
) -> None:
    lines = [
        '<?xml version="1.0" encoding="utf-8"?>',
        "<!DOCTYPE TS>",
        f'<TS version="2.1" language="{language}" sourcelanguage="zh_CN">',
        "<context>",
        "    <name>ReportCatalog</name>",
    ]
    for entry in report_entries:
        source = saxutils.escape(entry["zh_cn"])
        translation = saxutils.escape(
            entry["en_us"] if language.startswith("en") else entry["zh_cn"]
        )
        lines.append(f'    <message id="{saxutils.escape(entry["id"])}">')
        lines.append(f"        <source>{source}</source>")
        lines.append(f"        <translation>{translation}</translation>")
        lines.append("    </message>")
    lines.append("</context>")
    lines.extend(["<context>", "    <name>DataLabUi</name>"])
    for entry in ui_entries:
        source = saxutils.escape(entry["zh_cn"])
        translation = saxutils.escape(
            entry["en_us"] if language.startswith("en") else entry["zh_cn"]
        )
        lines.append(f'    <message id="{saxutils.escape(entry["id"])}">')
        lines.append(f"        <source>{source}</source>")
        lines.append(f"        <translation>{translation}</translation>")
        lines.append("    </message>")
    lines.extend(["</context>", "</TS>", ""])
    path.write_text("\n".join(lines), encoding="utf-8")


def run_lrelease(lrelease: Path, ts_path: Path, qm_path: Path) -> None:
    subprocess.run(
        [str(lrelease), str(ts_path), "-qm", str(qm_path)],
        check=True,
    )


def load_ui_menu_entries(path: Path) -> list[dict[str, str]]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    entries = payload.get("entries", [])
    if not isinstance(entries, list) or not entries:
        raise SystemExit(f"No UI menu entries in {path}")
    return entries


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--lrelease",
        default="",
        help="Optional path to lrelease.exe to compile .qm",
    )
    parser.add_argument(
        "--skip-qm",
        action="store_true",
        help="Only refresh JSON/.ts mirrors",
    )
    args = parser.parse_args()

    cpp_path = root / "src" / "domain" / "report_text_catalog.cpp"
    translations = root / "translations"
    entries = parse_catalog(cpp_path)
    ui_entries = load_ui_menu_entries(translations / "ui_menu_strings.json")
    write_json(translations / "report_strings.json", entries)
    write_ts(translations / "DataLab_en_US.ts", entries, ui_entries, "en_US")
    write_ts(translations / "DataLab_zh_CN.ts", entries, ui_entries, "zh_CN")

    total = len(entries) + len(ui_entries)
    if args.skip_qm:
        print(
            f"Synced {len(entries)} report + {len(ui_entries)} UI menu entries "
            "(JSON + .ts only)"
        )
        return 0

    lrelease = Path(args.lrelease) if args.lrelease else None
    if lrelease is None or not lrelease.is_file():
        candidates = [
            Path(r"D:\Qt\6.11.1\mingw_64\bin\lrelease.exe"),
            Path(r"D:\Qt\6.11.1\msvc2022_64\bin\lrelease.exe"),
        ]
        lrelease = next((p for p in candidates if p.is_file()), None)

    if lrelease is None:
        print(
            f"Synced {total} entries to JSON/.ts; "
            "lrelease not found — compile .qm in Qt Creator or pass --lrelease",
            file=sys.stderr,
        )
        return 0

    run_lrelease(lrelease, translations / "DataLab_en_US.ts", translations / "DataLab_en_US.qm")
    run_lrelease(lrelease, translations / "DataLab_zh_CN.ts", translations / "DataLab_zh_CN.qm")
    print(
        f"Synced {len(entries)} report + {len(ui_entries)} UI menu entries "
        f"and compiled en_US/zh_CN .qm via {lrelease}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
