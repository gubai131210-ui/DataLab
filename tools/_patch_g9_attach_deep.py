#!/usr/bin/env python3
"""Patch computation_trace_attach.cpp for G9-D deep traces."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "src/application/computation_trace_attach.cpp"


def extract_stubs(cpp: str) -> list[str]:
    ids: list[str] = []
    seen: set[str] = set()
    for m in re.finditer(r'if \(command_id == "([^"]+)"\)', cpp):
        cid = m.group(1)
        block = cpp[m.start() : m.start() + 2500]
        pm = re.search(r'tr\.plain_formula = "([^"]+)"', block)
        if pm and "主公式" in pm.group(1) and cid not in seen:
            seen.add(cid)
            ids.append(cid)
    return ids


def patch_attach_cpp() -> None:
    cpp = CPP.read_text(encoding="utf-8")
    stubs = extract_stubs(cpp)
    print(f"stubs={len(stubs)}")

    if '#include "application/computation_trace_attach_deep.h"' not in cpp:
        cpp = cpp.replace(
            '#include "application/computation_trace_attach.h"\n',
            '#include "application/computation_trace_attach.h"\n'
            '#include "application/computation_trace_attach_deep.h"\n'
            '#include "application/computation_trace_helpers.h"\n',
            1,
        )

    if "using namespace datalab::application::trace_helpers" not in cpp:
        cpp = cpp.replace(
            "namespace {\n\nusing datalab::domain::",
            "namespace {\n\nusing namespace datalab::application::trace_helpers;\n\nusing datalab::domain::",
            1,
        )

    if "attach_deep_trace(page, command_id)" not in cpp:
        cpp = cpp.replace(
            "    static_cast<void>(k_g9_covered_commands);\n\n",
            "    static_cast<void>(k_g9_covered_commands);\n\n"
            "    if (attach_deep_trace(page, command_id)) {\n"
            "        return;\n"
            "    }\n\n",
            1,
        )

    for cid in stubs:
        block_pat = re.compile(
            rf'\n    if \(command_id == "{re.escape(cid)}"\) \{{'
            rf'[\s\S]*?'
            rf'tr\.plain_formula = "[^"]*主公式[^"]*";'
            rf'[\s\S]*?'
            rf'        return;\n    \}}',
            re.MULTILINE,
        )
        cpp, n = block_pat.subn("", cpp, count=1)
        if n != 1:
            print(f"WARNING: remove count={n} for {cid}", file=sys.stderr)

    enhancements = {
        "capability": """
        push_step(tr.steps, make_step(1, "上规格距离", "Cpu = (USL−μ)/(3σ)",
            "(" + usl + "−" + mu + ")/(3·" + sigma + ")", usl));
        push_step(tr.steps, make_step(2, "下规格距离", "Cpl = (μ−LSL)/(3σ)",
            "(" + mu + "−" + lsl + ")/(3·" + sigma + ")", lsl));""",
        "one_sample_t": """
        push_step(tr.steps, make_step(1, "样本均值", "x̄ = Σx/n", "x̄ = " + xbar, xbar));
        push_step(tr.steps, make_step(2, "标准误", "SE = s/√n", "SE = " + s + "/√" + n, s));
        push_step(tr.steps, make_step(3, "t 统计量", "t = (x̄−μ₀)/SE",
            "t = " + tstat, tstat));""",
        "weibayes": """
        push_step(tr.steps, make_step(1, "形状先验 β", "β fixed", "β = " + beta, beta));
        push_step(tr.steps, make_step(2, "失效数 r", "r = failures", "r = " + r, r));
        push_step(tr.steps, make_step(3, "尺度 η", "η = (Σ t_i^β / r)^(1/β)",
            "η = " + eta, eta));""",
    }

    for cid, enh in enhancements.items():
        block_pat = re.compile(
            rf'(    if \(command_id == "{re.escape(cid)}"\) \{{[\s\S]*?)(        tr\.substituted_text = )',
            re.MULTILINE,
        )
        m = block_pat.search(cpp)
        if m and "push_step(tr.steps" not in m.group(1):
            cpp = block_pat.sub(rf"\1{enh}\n\2", cpp, count=1)

    # descriptive: replace bindings + add steps
    desc_pat = re.compile(
        r'(    if \(command_id == "descriptive"\) \{[\s\S]*?)'
        r'        tr\.bindings\.push_back\(bind\("n"',
        re.MULTILINE,
    )
    if desc_pat.search(cpp) and "push_step(tr.steps" not in desc_pat.search(cpp).group(1):
        desc_repl = r'''\1        const std::string xbar = first_nonempty({
            page.facts.descriptive.has_value()
                ? opt_fmt(page.facts.descriptive->mean) : std::string{},
            table_value(page, "Mean", "均值", "Average")});
        const std::string n_desc = first_nonempty({
            page.facts.descriptive.has_value()
                ? std::to_string(page.facts.descriptive->n) : std::string{},
            table_value(page, "N", "n", "有效 N")});
        tr.bindings.push_back(bind("n"'''
        cpp = desc_pat.sub(desc_repl, cpp, count=1)
        cpp = re.sub(
            r'(if \(command_id == "descriptive"\)[\s\S]*?tr\.bindings\.push_back\(bind\("n", "样本量 N", )[^,]+,',
            r'\1n_desc,',
            cpp,
            count=1,
        )
        cpp = re.sub(
            r'(if \(command_id == "descriptive"\)[\s\S]*?tr\.bindings\.push_back\(bind\("stat"[\s\S]*?\);\n)'
            r'        tr\.bindings\.push_back\(bind\("p"[\s\S]*?\);\n'
            r'        tr\.result_value = [^;]+;\n'
            r'        tr\.substituted_text = [^;]+;\n',
            r'\1        tr.bindings.push_back(bind("x̄", "均值", xbar, "result"));\n'
            r'        tr.result_value = xbar;\n'
            r'        push_step(tr.steps, make_step(1, "求和", "Σx_i", "n = " + n_desc, n_desc));\n'
            r'        push_step(tr.steps, make_step(2, "均值", "x̄ = Σx_i/n", "x̄ = " + xbar, xbar));\n'
            r'        tr.substituted_text = "x̄ = Σx_i / " + n_desc + " = " + xbar;\n',
            cpp,
            count=1,
        )

    CPP.write_text(cpp, encoding="utf-8")
    remaining = len(re.findall(r"主公式", cpp))
    print(f"patched attach.cpp; 主公式 remaining={remaining}")


if __name__ == "__main__":
    patch_attach_cpp()
