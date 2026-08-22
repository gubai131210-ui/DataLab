#!/usr/bin/env python3
import re
from pathlib import Path

ROOT = Path(r"d:\QT_CppPrograms\DataLab")
loc = (ROOT / "src/application/report_localization.cpp").read_text(encoding="utf-8")

# All exact_ids Chinese keys in localize_known_plain_message
# Match {"zh", "catalog.id"} pairs in exact_ids section - simplified: all {"...", "..."} with catalog-like ids
all_mapped = set()
for m in re.finditer(r'\{"([^"]*[\u4e00-\u9fff][^"]*)",\s*"([^"]+)"\}', loc):
    all_mapped.add(m.group(1))

sources = {
    "time_series.cpp": ROOT / "src/domain/statistics/time_series.cpp",
    "normality_test.cpp": ROOT / "src/domain/statistics/normality_test.cpp",
    "distribution_identification.cpp": ROOT
    / "src/domain/statistics/distribution_identification.cpp",
    "doe_factorial.cpp": ROOT / "src/domain/statistics/doe_factorial.cpp",
}

print("=== Full exact_ids coverage (all catalog ids) ===\n")
total_missing = []
for name, p in sources.items():
    text = p.read_text(encoding="utf-8")
    strings = sorted(
        set(m.group(1) for m in re.finditer(r'"([^"]*[\u4e00-\u9fff][^"]*)"', text))
    )
    missing = [s for s in strings if s not in all_mapped]
    print(f"{name}: {len(strings)} total, {len(missing)} missing from exact_ids")
    for s in missing:
        print(f"  MISSING: {s}")
        total_missing.append((name, s))
    print()

# Dynamic/template strings in doe
print(f"Total missing across 4 files: {len(total_missing)}")

# Slice-specific 21 strings from test
test = (ROOT / "tests/report_locale_phase3_test.cpp").read_text(encoding="utf-8")
block = test.split("void forecast_normality_distid_doe_diag_localizes_to_en_us()")[1]
diag_section = block.split("page.diagnostics = {")[1].split("};")[0]
slice_strings = re.findall(r'"([^"]*[\u4e00-\u9fff][^"]*)"', diag_section)
print(f"\nSlice test strings: {len(slice_strings)}")
for s in slice_strings:
    in_exact = s in all_mapped
    print(f"  {'OK' if in_exact else 'FAIL'} {s[:70]}")

# Catalog vs exact_ids for slice ids
cat = (ROOT / "src/domain/report_text_catalog.cpp").read_text(encoding="utf-8")
slice_ids = [
    "diag.time_series.single_exp_invalid",
    "diag.time_series.double_exp_invalid",
    "diag.normality.need_one_valid",
    "diag.normality.need_two_obs_prob_only",
    "diag.normality.zero_variance",
    "diag.normality.need_three_valid",
    "diag.normality.ryan_joiner_not_computed",
    "diag.normality.ryan_joiner_p_gt_010",
    "diag.normality.ryan_joiner_p_lt_001",
    "diag.distribution_id.insufficient_data",
    "diag.distribution_id.non_positive_values",
    "diag.doe.factor_count_overflow",
    "diag.doe.invalid_run_shape",
    "diag.doe.invalid_center_point",
    "diag.doe.invalid_factor_level",
    "diag.doe.incomplete_factorial_coverage",
    "diag.doe.hold_out_of_range",
    "diag.doe.invalid_hold_levels",
    "diag.doe.invalid_hold_value",
    "diag.doe.contour_factors_held_at_actual",
    "diag.doe.contour_factors_held_at_zero",
]
print("\n=== Slice 21 catalog EN zh consistency ===")
for cid in slice_ids:
    m = re.search(
        rf'\{{"{re.escape(cid)}",\s*\n\s*"([^"]+)",\s*\n\s*"([^"]+)"\}}',
        cat,
    )
    if not m:
        print(f"  CATALOG MISSING: {cid}")
        continue
    zh, en = m.group(1), m.group(2)
    # find exact_ids mapping
    em = re.search(rf'\{{"{re.escape(zh)}",\s*"{re.escape(cid)}"\}}', loc)
    if not em:
        print(f"  exact_ids MISSING for {cid}: {zh!r}")
    else:
        print(f"  OK {cid}")
