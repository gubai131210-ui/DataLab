---
name: data_verification
description: Verify DataLab C++ statistical algorithms against Minitab using three downloaded reference datasets per algorithm, reproducible run instructions, numerical comparisons, provenance, and user-confirmed results. Use only inside the DataLab project when validating a new or changed algorithm.
disable-model-invocation: true
---

# DataLab Minitab Data Verification

This skill is project-local and applies only to `DataLab`. Its purpose is to make every new statistical algorithm auditable against Minitab before the implementation is treated as complete.

## Non-negotiable workflow

When a new algorithm or formula is added:

1. Identify the exact Minitab method, data arrangement, estimation method, control-chart constants, missing-value rules, and specification settings.
2. Search the official Minitab Data Set Library and official support examples first.
3. Find exactly three suitable datasets:
   - one official primary/reference dataset;
   - one official or standards-based dataset with a different scale or layout;
   - one edge-case dataset containing a meaningful challenge such as missing values, unequal subgroup sizes, one-sided specification, outliers, or a shifted process.
4. Download or construct a local, importable copy under:

   `samples/capability/<algorithm-id>/`

5. Preserve source URLs, download date, original file name, license/usage notes, SHA-256, and any conversion steps. Never overwrite an existing reference dataset silently.
6. Create Minitab instructions and DataLab instructions for each dataset.
7. Run automated formula tests using the same fixtures where practical.
8. Give the user the exact manual comparison procedure and stop. Do not continue algorithm changes until the user reports the Minitab and DataLab results.
9. Compare the user-provided results line by line. Classify every difference as:
   - display rounding;
   - data/layout mismatch;
   - estimation-method mismatch;
   - missing-value or subgroup mismatch;
   - formula/implementation defect.
10. Only after the comparison is accepted may the algorithm be marked verified.

The user must perform the Minitab run and the DataLab run. Do not claim that an algorithm matches Minitab based only on a successful C++ build or unit tests.

## Dataset directory contract

For an algorithm with ID `normal_capability`, use:

```text
samples/capability/normal_capability/
├── sources.md
├── README.md
├── official_primary/
│   ├── data.csv
│   └── minitab_steps.md
├── alternate_layout/
│   ├── data.csv
│   └── minitab_steps.md
├── edge_case/
│   ├── data.csv
│   └── minitab_steps.md
├── datalab_steps.md
├── expected/
│   ├── primary.json
│   ├── alternate_layout.json
│   └── edge_case.json
└── verification_report.md
```

The exact subdirectory names may be adapted, but every algorithm folder must contain:

- three datasets;
- source and provenance information;
- Minitab execution instructions;
- DataLab execution instructions;
- expected-value placeholders or captured outputs;
- a verification report.

Keep original source files when legally and technically possible. Store converted UTF-8 CSV files beside them and document the conversion. Do not use synthetic data as the primary validation dataset unless no real reference data exists; if synthetic data is used, label it clearly and include the generator and seed.

## Source and download rules

- Prefer `support.minitab.com`, `minitab.com`, NIST, ASTM/AIAG references, government data, or peer-reviewed/official engineering sources.
- Use web search for discovery and fetch the source page before downloading.
- Verify that a dataset actually matches the algorithm. Do not infer specifications or subgroup sizes from a file name.
- Record:

```text
source_url:
source_title:
provider:
downloaded_at:
original_file:
local_file:
sha256:
license_or_usage_note:
conversion:
```

- If an official file is an MWX/MTW/ZIP container, keep the original and document the conversion script. Do not silently retype values.
- For CSV, record delimiter, encoding, decimal separator, header handling, missing-value tokens, and row count.

## Minitab run instructions

Every `minitab_steps.md` must state:

1. How to open/import the data.
2. Which worksheet column is used for each role.
3. Whether data are in one column, multiple columns, or subgroup rows.
4. Subgroup size or subgroup ID column.
5. LSL, USL, target, historical mean, and historical sigma when applicable.
6. The exact Minitab menu path and dialog settings.
7. Any Options/Estimate settings that affect formulas.
8. The output values and graph features to record.

Use the exact method variant. For example, do not compare an I-MR estimate with an Xbar-R estimate merely because both accept one measurement column.

## DataLab run instructions

Every algorithm folder must explain:

1. Which CSV to import.
2. Which DataLab menu to select.
3. Which variable and role boxes to fill.
4. The same subgroup/specification/estimation settings used in Minitab.
5. Which output tables, control limits, capability indices, PPM values, and graph annotations to record.
6. How to save the project or export the report if persistence is being verified.

Use Simplified Chinese UI names as displayed by DataLab, while retaining statistical symbols such as `Cp`, `Cpk`, `Pp`, `Ppk`, `X̄`, `R̄`, and `σ`.

## Comparison protocol

Use a comparison table in `verification_report.md` with these fields:

```text
metric:
minitab_value:
datalab_value:
absolute_difference:
relative_difference:
allowed_tolerance:
status:
notes:
```

Default numerical tolerances:

- control-chart limits and means: absolute tolerance `1e-6` for ordinary-scale fixtures;
- capability indices and sigma estimates: relative tolerance `1e-4`;
- PPM values: compare with the same displayed precision first, then compare unrounded values if available;
- counts, sample sizes, subgroup counts, and out-of-control row numbers: exact match;
- graph layout: qualitative review plus explicit axis/limit/value checks.

If Minitab displays rounded values, request or use the highest precision available before deciding there is a defect. Never loosen a tolerance merely to make a failing comparison pass.

## Algorithm-specific checklist

### Control charts

- Verify subgroup construction and incomplete-subgroup handling.
- Verify center line, UCL, LCL, and zero truncation.
- Verify d2, d3, c4, A2, A3, B3, B4, D3, and D4 constants.
- Verify Test 1 and every flagged source row.
- Verify whether Minitab uses average moving range, median moving range, R-bar, or S-bar.

### Process capability

- Verify Within and Overall standard deviations independently.
- Verify `Cp`, `CPL`, `CPU`, `Cpk`, `Pp`, `PPL`, `PPU`, `Ppk`, and target-related indices.
- Verify observed and expected PPM using the same limits and sigma estimates.
- Verify normality/probability-plot assumptions and warnings.
- Verify LSL-only and USL-only cases.

### Missing values and grouping

- Count `N` and missing values exactly.
- Check whether excluded rows are skipped or removed.
- Check subgroup IDs, ordering, and source-row mapping.
- Include at least one dataset where missing values are meaningful.

## User handoff and stop condition

After downloading the three datasets and writing the instructions, respond with:

```text
算法：
数据目录：

请先在 Minitab 中运行：
1. ...

再在 DataLab 中运行：
1. ...

请把以下结果发回：
- ...
- ...

在收到你的对比结果前，我不会把该算法标记为“已通过 Minitab 验证”，也不会基于猜测继续修改公式。
```

If the user reports a mismatch, reproduce it with the same local fixture before editing code. Preserve the failing values in `expected/` or `verification_report.md`, add a regression test, and rerun all relevant tests.

## Additional quality requirements

- Each algorithm must have at least one automated golden or fixture test.
- Keep raw imported data immutable; cleaning or reshaping must create a documented derived file.
- Record software/version information when a Minitab result is supplied.
- Record DataLab build configuration and commit/source state when available.
- Never compare only screenshots. Screenshots supplement numerical comparison.
- If the source dataset cannot legally be redistributed, store the source URL and a reproducible download/conversion script instead of copying it.
- Do not use Python or third-party statistical libraries to replace the C++ production formula. Python may assist with file conversion, checksum, and independent validation.
