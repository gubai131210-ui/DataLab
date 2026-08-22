# Reference_implementation preflight — algorithm evidence scripts (no Qt/C++ build).
# Usage: powershell -File tools/reference_implementation_preflight.ps1

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

Write-Host "=== Reference implementation preflight ===" -ForegroundColor Cyan

$scripts = @(
    @{ Name = "km"; Args = @("scripts/reliability_km_reference.py") },
    @{ Name = "warranty"; Args = @("scripts/reliability_warranty_reference.py") },
    @{ Name = "rsm-lof"; Args = @("scripts/rsm_lof_reference.py") },
    @{ Name = "box-cox"; Args = @("scripts/box_cox_reference.py") },
    @{ Name = "doe"; Args = @("scripts/doe_rsm_reference_points.py") }
)

$failed = 0
foreach ($s in $scripts) {
    Write-Host "`n>> python $($s.Args -join ' ')" -ForegroundColor Yellow
    & python @($s.Args)
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAILED: $($s.Name)" -ForegroundColor Red
        $failed++
    } else {
        Write-Host "OK: $($s.Name)" -ForegroundColor Green
    }
}

Write-Host "`n=== Summary ===" -ForegroundColor Cyan
if ($failed -eq 0) {
    Write-Host "All reference_implementation scripts passed (not vendor_oracle)." -ForegroundColor Green
    Write-Host "See docs/research/reference-implementation-index.md"
    exit 0
} else {
    Write-Host "$failed reference script(s) failed." -ForegroundColor Red
    exit 1
}
