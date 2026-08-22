#!/usr/bin/env python3
"""Split report_text_catalog entries into smaller MinGW-friendly translation units."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOMAIN = ROOT / "src" / "domain"
CPP = DOMAIN / "report_text_catalog.cpp"
PARTS = 16

ENTRY_RE = re.compile(
    r'\{\s*"([^"]+)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}',
    re.MULTILINE,
)


def extract_catalog_block(raw: str) -> tuple[int, int, list[re.Match[str]]]:
    start = raw.find("static const std::vector<ReportTextEntry> catalog = {")
    if start < 0:
        raise SystemExit("catalog initializer not found")
    close = raw.find("\n    };", start)
    if close < 0:
        raise SystemExit("catalog initializer close not found")
    block = raw[start:close]
    matches = list(ENTRY_RE.finditer(block))
    if not matches:
        raise SystemExit("no catalog entries found")
    return start, close, matches


def write_parts(matches: list[re.Match[str]], block: str) -> list[str]:
    chunk = (len(matches) + PARTS - 1) // PARTS
    fn_names: list[str] = []
    for part in range(PARTS):
        i0 = part * chunk
        i1 = min(len(matches), (part + 1) * chunk)
        if i0 >= i1:
            break
        fn = f"append_report_text_catalog_part{part + 1}"
        fn_names.append(fn)
        slice_text = block[matches[i0].start() : matches[i1 - 1].end()].strip()
        if not slice_text.endswith(","):
            slice_text += ","
        cpp_path = DOMAIN / f"report_text_catalog_part{part + 1}.cpp"
        cpp_path.write_text(
            "\n".join(
                [
                    '#include "domain/report_text_catalog_parts.h"',
                    "",
                    "namespace datalab::domain {",
                    "namespace {",
                    "",
                    f"const ReportTextEntry kReportTextCatalogPart{part + 1}[] = {{",
                    slice_text,
                    "};",
                    "",
                    "}  // namespace",
                    "",
                    f"void {fn}(std::vector<ReportTextEntry>& out)",
                    "{",
                    f"    out.insert(",
                    f"        out.end(),",
                    f"        std::begin(kReportTextCatalogPart{part + 1}),",
                    f"        std::end(kReportTextCatalogPart{part + 1}));",
                    "}",
                    "",
                    "}  // namespace datalab::domain",
                    "",
                ]
            ),
            encoding="utf-8",
        )
        print(f"{cpp_path.name}: entries {i0 + 1}-{i1}")
    return fn_names


def rewrite_main(raw: str, start: int, close: int, fn_names: list[str], reserve: int) -> None:
    append_lines = "\n        ".join(f"{fn}(entries);" for fn in fn_names)
    replacement = (
        "static const std::vector<ReportTextEntry> catalog = [] {\n"
        "        std::vector<ReportTextEntry> entries;\n"
        f"        entries.reserve({reserve});\n"
        f"        {append_lines}\n"
        "        return entries;\n"
        "    }();"
    )
    new_raw = raw[:start] + replacement + raw[close + len("\n    };") :]
    if '#include "domain/report_text_catalog_parts.h"' not in new_raw:
        new_raw = new_raw.replace(
            '#include "domain/report_text_catalog.h"\n',
            '#include "domain/report_text_catalog.h"\n'
            '#include "domain/report_text_catalog_parts.h"\n',
        )
    CPP.write_text(new_raw, encoding="utf-8")


def write_header(fn_names: list[str]) -> None:
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


def update_cmake(fn_count: int) -> None:
    cmake = ROOT / "CMakeLists.txt"
    text = cmake.read_text(encoding="utf-8")
    anchor = (
        "    src/domain/report_text_catalog.cpp\n"
        "    src/domain/report_text_catalog.h\n"
    )
    insert = anchor
    for i in range(1, fn_count + 1):
        line = f"    src/domain/report_text_catalog_part{i}.cpp\n"
        if line not in text:
            insert += line
    if insert != anchor:
        text = text.replace(anchor, insert, 1)
        cmake.write_text(text, encoding="utf-8")


def main() -> None:
    raw = CPP.read_text(encoding="utf-8")
    if "append_report_text_catalog_part1" in raw:
        print("catalog already split; regenerating part files only")
        part1 = (DOMAIN / "report_text_catalog_part1.cpp").read_text(encoding="utf-8")
        matches = list(ENTRY_RE.finditer(part1))
        if not matches:
            raise SystemExit("part1 has no entries")
        all_matches: list[re.Match[str]] = []
        for i in range(1, PARTS + 1):
            path = DOMAIN / f"report_text_catalog_part{i}.cpp"
            if not path.is_file():
                break
            all_matches.extend(ENTRY_RE.finditer(path.read_text(encoding="utf-8")))
        fn_names = [f"append_report_text_catalog_part{i}" for i in range(1, PARTS + 1)]
        write_header(fn_names)
        return
    start, close, matches = extract_catalog_block(raw)
    print(f"entries: {len(matches)}")
    block = raw[start:close]
    fn_names = write_parts(matches, block)
    write_header(fn_names)
    rewrite_main(raw, start, close, fn_names, len(matches))
    update_cmake(len(fn_names))
    print("split complete")


if __name__ == "__main__":
    main()
