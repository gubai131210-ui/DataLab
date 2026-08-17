"""Convert the public Minitab MWX worksheet container to CSV.

MWX files downloaded from the official Data Set Library are ZIP containers
whose worksheet payload is JSON. This converter preserves column order and
uses '*' for missing values.
"""

from __future__ import annotations

import csv
import json
import pathlib
import sys
import zipfile


def load_columns(source: pathlib.Path) -> tuple[list[str], list[list[object]]]:
    with zipfile.ZipFile(source) as archive:
        sheet = json.loads(archive.read("/sheets/0/sheet.json"))
    columns = sheet["Data"]["Columns"]
    names: list[str] = []
    values: list[list[object]] = []
    for column in columns:
        body = column["WorksheetVarBody"]
        data = body["VarData"]["VarDataBody"]
        names.append(body["Name"])
        if data.get("HasNumericData"):
            values.append(data.get("NumericData") or [])
        elif data.get("HasTextData"):
            values.append(data.get("TextData") or [])
        else:
            values.append(["*"] * int(data.get("CellCt", 0)))
    return names, values


def main() -> int:
    if len(sys.argv) != 3:
        print("Usage: mwx_to_csv.py INPUT.MWX OUTPUT.csv")
        return 2
    source = pathlib.Path(sys.argv[1])
    target = pathlib.Path(sys.argv[2])
    names, columns = load_columns(source)
    row_count = max((len(column) for column in columns), default=0)
    target.parent.mkdir(parents=True, exist_ok=True)
    with target.open("w", newline="", encoding="utf-8-sig") as stream:
        writer = csv.writer(stream)
        writer.writerow(names)
        for row_index in range(row_count):
            writer.writerow([
                "*" if row_index >= len(column) or column[row_index] is None else column[row_index]
                for column in columns
            ])
    print(f"Converted {row_count} rows and {len(names)} columns: {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
