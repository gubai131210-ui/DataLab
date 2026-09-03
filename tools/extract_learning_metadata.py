#!/usr/bin/env python3
"""Extract full metadata for learning center research notes generation."""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def extract_commands() -> list[dict]:
    cpp = (ROOT / "src/ui/analysis_commands.cpp").read_text(encoding="utf-8")
    pattern = re.compile(
        r"\{\s*\n"
        r"\s*QStringLiteral\(\"(?P<id>[a-z][a-z0-9_]+)\"\),\s*\n"
        r"\s*QStringLiteral\(\"(?P<menu_label>[^\"]+)\"\),\s*\n"
        r"\s*QStringLiteral\(\"(?P<dialog_title>[^\"]+)\"\),\s*\n"
        r"\s*QStringLiteral\(\"(?P<menu_path>[^\"]+)\"\),\s*\n"
        r"\s*QStringLiteral\(\"(?P<icon>[^\"]+)\"\),\s*\n"
        r"\s*(?P<sep>false|true),\s*(?P<requires_data>false|true),\s*\n"
        r"\s*\{(?P<roles>.*?)\},\s*\n"
        r"\s*\{(?P<inputs>.*?)\},\s*\n",
        re.MULTILINE | re.DOTALL,
    )
    results = []
    for m in pattern.finditer(cpp):
        roles = re.findall(
            r'QStringLiteral\("([^"]+)"\),\s*QStringLiteral\("([^"]+)"\)',
            m.group("roles"),
        )
        role_specs = [{"id": r[0], "label": r[1]} for r in roles[:10]]
        results.append(
            {
                "id": m.group("id"),
                "menu_label": m.group("menu_label"),
                "dialog_title": m.group("dialog_title"),
                "menu_path": m.group("menu_path"),
                "requires_data": m.group("requires_data") == "true",
                "roles": role_specs,
            }
        )
    return results


def extract_help() -> dict[str, dict]:
    data = json.loads((ROOT / "resources/help/algorithm_help.json").read_text(encoding="utf-8"))
    return {e["id"]: e for e in data["entries"]}


def main() -> None:
    cmds = extract_commands()
    help_map = extract_help()
    out = {
        "commands": cmds,
        "help_only": sorted(set(help_map) - {c["id"] for c in cmds}),
        "cmd_only": sorted({c["id"] for c in cmds} - set(help_map)),
    }
    (ROOT / "tools/learning_data/id_metadata.json").parent.mkdir(parents=True, exist_ok=True)
    merged = []
    all_ids = sorted(set(help_map) | {c["id"] for c in cmds})
    cmd_by_id = {c["id"]: c for c in cmds}
    for cid in all_ids:
        entry = {
            "id": cid,
            "command": cmd_by_id.get(cid),
            "help": {
                k: help_map[cid].get(k)
                for k in (
                    "title",
                    "category",
                    "menu_path",
                    "implemented_status",
                    "purpose",
                    "output_description",
                    "interpretation_limits",
                )
                if cid in help_map
            }
            or None,
        }
        merged.append(entry)
    (ROOT / "tools/learning_data/id_metadata.json").write_text(
        json.dumps({"union_count": len(all_ids), "entries": merged}, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    print(f"Wrote {len(merged)} entries to tools/learning_data/id_metadata.json")
    print("help_only:", out["help_only"])
    print("cmd_only:", out["cmd_only"])


if __name__ == "__main__":
    main()
