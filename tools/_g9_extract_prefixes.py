#!/usr/bin/env python3
"""Extract new_id prefixes from analysis_service.cpp for G9 prefix map."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
text = (ROOT / "src/application/analysis_service.cpp").read_text(encoding="utf-8")
ids = sorted(set(re.findall(r'new_id\(\s*"([^"]+)"', text)))
print(f"count={len(ids)}")
for i in ids:
    print(i)
