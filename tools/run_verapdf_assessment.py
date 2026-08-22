#!/usr/bin/env python3
"""Offline helper: record veraPDF CLI outcome without inventing PDF/UA pass.

Usage:
  set DATALAB_VERAPDF=C:\\path\\to\\verapdf.bat
  python tools/run_verapdf_assessment.py path\\to\\report.pdf

Writes sibling <pdf>.verapdf.json with exit_code / notes. Does NOT claim PDF/UA.
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: run_verapdf_assessment.py <pdf>", file=sys.stderr)
        return 2
    pdf = Path(sys.argv[1])
    tool = os.environ.get("DATALAB_VERAPDF", "").strip()
    payload = {
        "pdf": str(pdf),
        "tool_configured": bool(tool),
        "tool_available": False,
        "tool_invoked": False,
        "exit_code": None,
        "validator_name": "veraPDF",
        "pdfua_status": "unsupported",
        "notes": "",
    }
    if not tool:
        payload["notes"] = "DATALAB_VERAPDF unset; PDF/A remains not_validated."
        out = pdf.with_suffix(pdf.suffix + ".verapdf.json")
        out.write_text(json.dumps(payload, indent=2), encoding="utf-8")
        print(out)
        return 0
    tool_path = Path(tool)
    if not tool_path.exists():
        payload["notes"] = "DATALAB_VERAPDF path missing."
        out = pdf.with_suffix(pdf.suffix + ".verapdf.json")
        out.write_text(json.dumps(payload, indent=2), encoding="utf-8")
        print(out)
        return 1
    payload["tool_available"] = True
    if not pdf.exists():
        payload["notes"] = "PDF missing; not invoked."
        out = pdf.with_suffix(pdf.suffix + ".verapdf.json")
        out.write_text(json.dumps(payload, indent=2), encoding="utf-8")
        print(out)
        return 1
    completed = subprocess.run([str(tool_path), str(pdf)], capture_output=True, text=True)
    payload["tool_invoked"] = True
    payload["exit_code"] = completed.returncode
    excerpt = (completed.stdout or "") + (completed.stderr or "")
    payload["notes"] = excerpt.strip()[:800]
    payload["pdfa_status"] = "validated_pass" if completed.returncode == 0 else "validated_fail"
    out = pdf.with_suffix(pdf.suffix + ".verapdf.json")
    out.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(out)
    return 0 if completed.returncode == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
