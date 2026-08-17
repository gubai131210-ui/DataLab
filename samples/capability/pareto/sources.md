# Pareto 数据集来源

downloaded_at: 2026-08-15

## official_primary — ClothingDefect

```text
source_url: https://support.minitab.com/en-us/datasets/quality-tools-data-sets/clothing-defects/
source_title: Clothing defect data
provider: Minitab Data Set Library
downloaded_at: 2026-08-15
original_file: ClothingDefect.MWX
local_file: raw/ClothingDefect.MWX
sha256: B81344F72904DAD9F736D19C5263893C5F7AB545DD891BC1AB089A5E23A2DFAE
license_or_usage_note: Minitab sample data for local validation; follow Minitab site terms.
conversion: tools/mwx_to_csv.py → official_primary/data.csv (UTF-8-SIG, comma, header, * for missing)
csv_sha256: C68F7FB076C64285C2EBB803065D739B60199F3434B98D5EB74BEAD2FF946659
rows: 6 data rows + header
columns: Defect, Count, Cost, Count*Cost
```

官方说明：第 5 行示例为 stitching errors = 112。

## alternate_layout — PaintFlaws

```text
source_url: https://support.minitab.com/en-us/datasets/quality-tools-data-sets/paint-flaws-data/
source_title: Paint flaws data
provider: Minitab Data Set Library
downloaded_at: 2026-08-15
original_file: PaintFlaws.MWX
local_file: raw/PaintFlaws.MWX
sha256: 0AC95AE9532064B3DCDF5268512FF4F35FC91D046D6F6D071C90E948FF8E355E
license_or_usage_note: Minitab sample data for local validation; follow Minitab site terms.
conversion: tools/mwx_to_csv.py → alternate_layout/data.csv
csv_sha256: 37B93FC033674D54B5455C8FA5CAA3561F3FBA6F91F5D530BFB4EF63C4DBA659
rows: 40 observations + header
columns: Flaws, Shift
```

Flaws 计数（本转换结果）：Peel=15, Scratch=13, Smudge=6, Other=6。

## edge_case — 派生文件（含缺失与稀有类别）

```text
source_title: ClothingDefect-derived edge case (documented synthetic extension)
provider: DataLab local derivation from ClothingDefect
downloaded_at: 2026-08-15
original_file: (derived; not an official MWX)
local_file: edge_case/data.csv
label: SYNTHETIC_DERIVED
conversion: started from ClothingDefect Defect/Count, then added missing markers and rare categories
purpose: missing category/count handling + Other threshold 90%
```

派生内容：

- 保留官方 6 类汇总计数。
- 增加稀有类别：Zipper jam=4, Label crooked=3, Pocket tear=2, Buttonhole skip=1。
- 增加缺失：`Defect=*`（Count=5，应整行忽略）；`Color fade` 且 `Count=*`（应忽略）。
- 有效总计数应为 490。
