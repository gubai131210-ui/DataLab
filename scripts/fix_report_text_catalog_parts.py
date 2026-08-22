#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOMAIN = ROOT / "src" / "domain"
ENTRY_RE = re.compile(
    r'\{\s*"([^"]+)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}',
    re.MULTILINE,
)

for i in range(1, 5):
    path = DOMAIN / f"report_text_catalog_part{i}.cpp"
    raw = path.read_text(encoding="utf-8")
    raw = raw.lstrip()
    m = re.search(
        rf"kReportTextCatalogPart{i}\[\] = \{{\s*(.*?)\s*\}};\s*\n\s*\}}  // namespace",
        raw,
        re.S,
    )
    if not m:
        raise SystemExit(f"could not parse array body in {path.name}")
    slice_text = m.group(1).strip()
    fn = f"append_report_text_catalog_part{i}"
    path.write_text(
        "\n".join(
            [
                '#include "domain/report_text_catalog_parts.h"',
                "",
                "namespace datalab::domain {",
                "namespace {",
                "",
                f"const ReportTextEntry kReportTextCatalogPart{i}[] = {{",
                slice_text,
                "};",
                "",
                "}  // namespace",
                "",
                f"void {fn}(std::vector<ReportTextEntry>& out)",
                "{",
                f"    out.insert(",
                f"        out.end(),",
                f"        std::begin(kReportTextCatalogPart{i}),",
                f"        std::end(kReportTextCatalogPart{i}));",
                "}",
                "",
                "}  // namespace datalab::domain",
                "",
            ]
        ),
        encoding="utf-8",
    )
    print(f"fixed {path.name}: {len(ENTRY_RE.findall(slice_text))} entries")
