#!/usr/bin/env python3
"""Extract analysis command IDs and help catalog IDs for learning center research."""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def extract_command_ids() -> list[str]:
    cpp = (ROOT / "src/ui/analysis_commands.cpp").read_text(encoding="utf-8")
    # Each command block starts with { then QStringLiteral("id"),
    pattern = re.compile(
        r"\{\s*\n\s*QStringLiteral\(\"([a-z][a-z0-9_]+)\"\),\s*\n\s*QStringLiteral\(",
        re.MULTILINE,
    )
    return pattern.findall(cpp)


def extract_help_ids() -> list[str]:
    data = json.loads((ROOT / "resources/help/algorithm_help.json").read_text(encoding="utf-8"))
    return [e["id"] for e in data["entries"]]


def main() -> None:
    cmd_ids = extract_command_ids()
    help_ids = extract_help_ids()
    union = sorted(set(cmd_ids) | set(help_ids))
    print(f"COMMANDS: {len(cmd_ids)}")
    print(f"HELP: {len(help_ids)}")
    print(f"UNION: {len(union)}")
    print("ONLY_HELP:", sorted(set(help_ids) - set(cmd_ids)))
    print("ONLY_CMD:", sorted(set(cmd_ids) - set(help_ids)))
    for x in cmd_ids:
        print(f"CMD\t{x}")
    for x in help_ids:
        print(f"HELP\t{x}")


if __name__ == "__main__":
    main()
