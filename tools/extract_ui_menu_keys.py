#!/usr/bin/env python3
"""Extract unique analysis menu_path / menu_group / menu_label from analysis_commands.cpp."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
text = (ROOT / "src/ui/analysis_commands.cpp").read_text(encoding="utf-8")

# Command header: id, menu_label, dialog_title, menu_path
header_re = re.compile(
    r'QStringLiteral\("([^"]+)"\),\s*'
    r'QStringLiteral\("([^"]+)"\),\s*'
    r'QStringLiteral\("([^"]+)"\),\s*'
    r'QStringLiteral\("([^"]+)"\)',
    re.MULTILINE,
)
paths: set[str] = set()
labels: set[str] = set()
for match in header_re.finditer(text):
    _cid, menu_label, _dialog, menu_path = match.groups()
    paths.add(menu_path)
    labels.add(menu_label)

groups: set[str] = set()
for match in re.finditer(r'\.menu_group\s*=\s*QStringLiteral\("([^"]+)"\)', text):
    groups.add(match.group(1))
# Also trailing field in braced init: last string before requires_data sometimes
# menu_group often as  QStringLiteral("...") after false, true,
for match in re.finditer(
    r'false,\s*true,\s*\n\s*\{.*?},\s*\n\s*\{.*?},\s*\n.*?,\s*\n\s*QStringLiteral\("([^"]+)"\)',
    text,
    re.DOTALL,
):
    groups.add(match.group(1))

print("PATHS", len(paths))
for p in sorted(paths):
    print("  path:", p)
print("GROUPS", len(groups))
for g in sorted(groups):
    print("  group:", g)
print("LABELS", len(labels))
