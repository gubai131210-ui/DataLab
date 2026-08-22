# -*- coding: utf-8 -*-
"""Fresh simulate localize_parameter_summary + caption + known title suffixes."""
import re
import pathlib
from collections import Counter

root = pathlib.Path(r"D:/QT_CppPrograms/DataLab")
loc = (root / "src/application/report_localization.cpp").read_text(encoding="utf-8")
catalog = (root / "src/domain/report_text_catalog.cpp").read_text(encoding="utf-8")

# Extract param.summary token list in declaration order from localize_parameter_summary
start = loc.find("void localize_parameter_summary(")
end = loc.find("\n}  // namespace", start)
body = loc[start:end]
param_tokens = re.findall(
    r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"(param\.summary\.[^"]+|graph\.caption\.[^"]+)"\s*\}',
    body,
)
# caption tokens applied first in real code via localize_graph_caption_tokens
cap_body_start = loc.find("void localize_graph_caption_tokens(")
cap_body_end = loc.find("\n}", cap_body_start)
cap_body = loc[cap_body_start:cap_body_end]
cap_tokens = re.findall(
    r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"(graph\.caption\.[^"]+|param\.summary\.[^"]+)"\s*\}',
    cap_body,
)

# Exact plain maps used for tables/headers/metrics/plots/pages/diags
exact_pairs = re.findall(
    r'\{\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)+)"\s*\}', loc
)
exact_map = {
    a: b
    for a, b in exact_pairs
    if re.search(r"[\u4e00-\u9fff]", a)
    and b.startswith(
        (
            "diag.",
            "page.",
            "plot.",
            "header.",
            "metric.",
            "table.",
            "series.",
            "status.",
            "chrome.",
            "axis.",
        )
    )
}

# catalog en presence
cat = {}
for m in re.finditer(
    r'\{\s*"([^"]+)"\s*,\s*\n\s*"([^"]*)"\s*,\s*\n\s*"([^"]*)"',
    catalog,
):
    cat[m.group(1)] = (m.group(2), m.group(3))


def apply_tokens(text, tokens):
    for zh, tid in tokens:
        en = cat.get(tid, (zh, zh))[1] if tid in cat else f"⟦{tid}⟧"
        # simulate en-US: replace ZH with EN catalog text
        if tid in cat:
            en = cat[tid][1]
        else:
            en = f"⟦{tid}⟧"
        text = text.replace(zh, en)
    return text


def localize_summary(text):
    text = apply_tokens(text, cap_tokens)
    text = apply_tokens(text, param_tokens)
    return text


def has_cjk(s):
    return bool(re.search(r"[\u4e00-\u9fff]", s))


# Collect parameter_summary construction sites: multi-line-ish by scanning
# lines that assign/append parameter_summary and gather string literals on
# contiguous block (simple heuristic: same function proximity).
anal = (root / "src/application/analysis_service.cpp").read_text(encoding="utf-8")
graph = (root / "src/application/graph_service.cpp").read_text(encoding="utf-8")

lit = re.compile(r'"((?:[^"\\]|\\.)*)"')


def extract_summary_templates(text, filename):
    """Find page.parameter_summary = ... chains; join string literals nearby."""
    lines = text.splitlines()
    sites = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if "parameter_summary" in line and (
            "=" in line or "+=" in line or "append" in line.lower()
        ):
            block = []
            j = i
            # take up to 12 following continuation lines
            while j < len(lines) and j < i + 14:
                block.append(lines[j])
                # stop if next statement not continuing summary
                if j > i and lines[j].strip() and not lines[j].lstrip().startswith("+") and "parameter_summary" not in lines[j] and not lines[j].rstrip().endswith("+") and not lines[j].rstrip().endswith(";"):
                    # allow diagnostics append etc to stop
                    if "page." in lines[j] and "parameter_summary" not in lines[j]:
                        break
                    if lines[j].lstrip().startswith("append_") or lines[j].lstrip().startswith("if ") or lines[j].lstrip().startswith("for ") or lines[j].lstrip().startswith("return"):
                        break
                if lines[j].rstrip().endswith(";") and j > i:
                    break
                j += 1
            blob = "\n".join(block)
            parts = lit.findall(blob)
            zh_parts = [p for p in parts if has_cjk(p)]
            if zh_parts:
                # reconstruct approximate template
                tmpl = "".join(zh_parts)
                after = localize_summary(tmpl)
                sites.append((filename, i + 1, tmpl, after))
            i = j + 1
            continue
        i += 1
    return sites


sites = extract_summary_templates(anal, "analysis_service.cpp") + extract_summary_templates(
    graph, "graph_service.cpp"
)

leaky = []
for fn, ln, before, after in sites:
    if has_cjk(after):
        # remaining CJK fragments
        frags = re.findall(r"[\u4e00-\u9fff]+(?:\s*[=:：]?\s*)?", after)
        leaky.append((fn, ln, before, after, frags))

# Cluster remaining fragments
frag_c = Counter()
for _, _, _, after, frags in leaky:
    # also whole leftover tokens with =
    for m in re.finditer(r"[^\s⟦\]]*[\u4e00-\u9fff][^\s⟦\]]*", after):
        frag_c[m.group(0)] += 1

out = root / "scripts/_tmp_param_sim_out.txt"
with out.open("w", encoding="utf-8") as fh:
    fh.write(f"cap_tokens={len(cap_tokens)} param_tokens={len(param_tokens)}\n")
    fh.write(f"sites={len(sites)} leaky_sites={len(leaky)}\n")
    fh.write("\nTOP REMAINING FRAGMENTS\n")
    for k, v in frag_c.most_common(40):
        fh.write(f"  {v}\t{k}\n")
    fh.write("\nLEAKY SITES (sample)\n")
    for fn, ln, before, after, frags in leaky[:40]:
        fh.write(f"\n{fn}:{ln}\n")
        fh.write(f"  BEFORE: {before[:160]}\n")
        fh.write(f"  AFTER:  {after[:200]}\n")

# Also: title literals not in exact_map / suffix rewrite
suffixes = ["的正态概率图", "的直方图", "的个体值图", "的散点图", " 运行图", "（分面）", "（编码）"]
title_leaks = []
for fn, text in [
    ("analysis_service.cpp", anal),
    ("graph_service.cpp", graph),
    ("doe_pages.cpp", (root / "src/application/doe_pages.cpp").read_text(encoding="utf-8")),
    ("chart_pages.cpp", (root / "src/application/chart_pages.cpp").read_text(encoding="utf-8")),
    ("output_builder.cpp", (root / "src/application/output_builder.cpp").read_text(encoding="utf-8")),
]:
    for i, line in enumerate(text.splitlines(), 1):
        if ".title" not in line and "title =" not in line and "x_axis_title" not in line and "y_axis_title" not in line:
            continue
        for s in lit.findall(line):
            if not has_cjk(s):
                continue
            # apply suffix strip simulation
            t = s
            for sfx in suffixes:
                if sfx in t:
                    t = t.replace(sfx, "")
            if s in exact_map:
                continue
            if any(sfx in s for sfx in ["的正态概率图", "的直方图", "的个体值图", "的散点图", "运行图", "（分面）"]):
                # suffix-localized
                continue
            if "（编码）" in s:
                continue  # coded suffix handler
            if has_cjk(t) and s not in exact_map:
                title_leaks.append((fn, i, s))

with out.open("a", encoding="utf-8") as fh:
    fh.write(f"\n\nTITLE_LEAKS {len(title_leaks)}\n")
    for fn, i, s in title_leaks[:40]:
        fh.write(f"  {fn}:{i}\t{s}\n")

print(f"wrote {out}")
print(f"cap={len(cap_tokens)} param={len(param_tokens)} sites={len(sites)} leaky={len(leaky)}")
print("top frags:")
for k, v in frag_c.most_common(20):
    print(f"  {v}\t{k}")
print(f"title_leaks={len(title_leaks)}")
for fn, i, s in title_leaks[:15]:
    print(f"  {fn}:{i}\t{s}")
