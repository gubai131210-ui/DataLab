"""Read an Excel or CSV file for the DataLab Qt adapter.

The C++ application remains the owner of the quality-analysis calculations.
This script only normalizes external tabular input into a small JSON payload.
"""

from __future__ import annotations

import json
import pathlib
import sys

import pandas as pd


def main() -> int:
    if len(sys.argv) != 2:
        print(json.dumps({"error": "Expected one input file path."}))
        return 2

    source = pathlib.Path(sys.argv[1])
    try:
        sheet_name = ""
        sheet_index = 0
        if source.suffix.lower() in {".xlsx", ".xls"}:
            workbook = pd.ExcelFile(source)
            if not workbook.sheet_names:
                raise ValueError("Excel workbook contains no worksheets.")
            sheet_name = str(workbook.sheet_names[0])
            frame = pd.read_excel(workbook, sheet_name=sheet_name)
        else:
            frame = pd.read_csv(source)
        frame = frame.where(pd.notna(frame), "")
        payload = {
            "schema_version": 1,
            "name": source.stem,
            "source_path": str(source),
            "sheet_name": sheet_name,
            "sheet_index": sheet_index,
            "columns": [str(column) for column in frame.columns],
            "rows": [[str(value) for value in row] for row in frame.astype(str).values.tolist()],
        }
        print(json.dumps(payload, ensure_ascii=False))
        return 0
    except Exception as error:  # noqa: BLE001 - boundary must return a useful message.
        print(json.dumps({"error": str(error)}, ensure_ascii=False))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
