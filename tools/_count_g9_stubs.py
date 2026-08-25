#!/usr/bin/env python3
"""Count G9 attach stub markers."""
from __future__ import annotations

import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
text = (ROOT / "src/application/computation_trace_attach.cpp").read_text(encoding="utf-8")
print("主公式 count:", text.count("主公式"))
blocks = re.findall(r'if \(command_id == "([^"]+)"\)', text)
stub_ids = []
for m in re.finditer(r'if \(command_id == "([^"]+)"\)\s*\{([^}]+(?:\{[^}]*\}[^}]*)*)\}', text):
    if "主公式" in m.group(2):
        stub_ids.append(m.group(1))
print("stub command blocks:", len(stub_ids))
print("sample:", stub_ids[:10])
