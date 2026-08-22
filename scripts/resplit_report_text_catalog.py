#!/usr/bin/env python3
"""Re-split catalog parts into smaller MinGW-friendly chunks."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOMAIN = ROOT / "src" / "domain"
PARTS = 16

ENTRY_RE = re.compile(
    r'\{\s*"([^"]+)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}',
    re.MULTILINE,
)
ARRAY_RE = re.compile(
    r"kReportTextCatalogPart\d+\[\] = \{\s*(.*?)\s*\};",
    re.S,
)


def load_entries() -> list[str]:
    chunks: list[str] = []
    for path in sorted(DOMAIN.glob("report_text_catalog_part*.cpp")):
        raw = path.read_text(encoding="utf-8")
        m = ARRAY_RE.search(raw)
        if not m:
            raise SystemExit(f"array not found in {path.name}")
        chunks.append(m.group(1).strip())
    blob = ",\n".join(chunks)
    matches = list(ENTRY_RE.finditer(blob))
    if not matches:
        raise SystemExit("no entries parsed")
    return [blob[m.start() : m.end()] for m in matches]


def write_part(part: int, slice_text: str, fn: str) -> None:
    path = DOMAIN / f"report_text_catalog_part{part}.cpp"
    path.write_text(
        "\n".join(
            [
                '#include "domain/report_text_catalog_parts.h"',
                "",
                "namespace datalab::domain {",
                "namespace {",
                "",
                f"const ReportTextEntry kReportTextCatalogPart{part}[] = {{",
                slice_text,
                "};",
                "",
                "}  // namespace",
                "",
                f"void {fn}(std::vector<ReportTextEntry>& out)",
                "{",
                f"    out.insert(",
                f"        out.end(),",
                f"        std::begin(kReportTextCatalogPart{part}),",
                f"        std::end(kReportTextCatalogPart{part}));",
                "}",
                "",
                "}  // namespace datalab::domain",
                "",
            ]
        ),
        encoding="utf-8",
    )


def main() -> None:
    entries = load_entries()
    print(f"entries: {len(entries)}")
    chunk = (len(entries) + PARTS - 1) // PARTS
    fn_names: list[str] = []
    used_parts = 0
    for part in range(1, PARTS + 1):
        i0 = (part - 1) * chunk
        i1 = min(len(entries), part * chunk)
        if i0 >= i1:
            break
        used_parts = part
        fn = f"append_report_text_catalog_part{part}"
        fn_names.append(fn)
        slice_text = ",\n".join(entries[i0:i1])
        if not slice_text.endswith(","):
            slice_text += ","
        write_part(part, slice_text, fn)
        print(f"part{part}: {i1 - i0} entries")

    for stale in sorted(DOMAIN.glob("report_text_catalog_part*.cpp")):
        n = int(stale.stem.replace("report_text_catalog_part", ""))
        if n > used_parts:
            stale.unlink()
            print(f"removed {stale.name}")

    decls = "\n".join(
        f"void {fn}(std::vector<ReportTextEntry>& out);" for fn in fn_names
    )
    (DOMAIN / "report_text_catalog_parts.h").write_text(
        "\n".join(
            [
                "#pragma once",
                "",
                '#include "domain/report_text_catalog.h"',
                "",
                "#include <vector>",
                "",
                "namespace datalab::domain {",
                "",
                decls,
                "",
                "}  // namespace datalab::domain",
                "",
            ]
        ),
        encoding="utf-8",
    )

    cpp = DOMAIN / "report_text_catalog.cpp"
    raw = cpp.read_text(encoding="utf-8")
    start = raw.find("static const std::vector<ReportTextEntry> catalog = [] {")
    close = raw.find("\n    }();", start)
    append_lines = "\n        ".join(f"{fn}(entries);" for fn in fn_names)
    replacement = (
        "static const std::vector<ReportTextEntry> catalog = [] {\n"
        "        std::vector<ReportTextEntry> entries;\n"
        f"        entries.reserve({len(entries)});\n"
        f"        {append_lines}\n"
        "        return entries;\n"
        "    }();"
    )
    cpp.write_text(raw[:start] + replacement + raw[close + len("\n    }();") :], encoding="utf-8")

    cmake = ROOT / "CMakeLists.txt"
    text = cmake.read_text(encoding="utf-8")
    text = re.sub(
        r"    src/domain/report_text_catalog_part\d+\.cpp\n",
        "",
        text,
    )
    anchor = (
        "    src/domain/report_text_catalog.cpp\n"
        "    src/domain/report_text_catalog.h\n"
    )
    insert = anchor + "".join(
        f"    src/domain/report_text_catalog_part{i}.cpp\n"
        for i in range(1, used_parts + 1)
    )
    text = text.replace(anchor, insert, 1)
    cmake.write_text(text, encoding="utf-8")
    print(f"resplit into {used_parts} parts")


if __name__ == "__main__":
    main()
