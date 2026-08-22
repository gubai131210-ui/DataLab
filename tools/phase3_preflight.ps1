# Phase 3 preflight — run before Qt Creator §3.1 (61 tests)
# Usage: powershell -File tools/phase3_preflight.ps1

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

Write-Host "=== Phase 3 preflight ===" -ForegroundColor Cyan

$scripts = @(
    @{ Name = "registry"; Args = @("tools/verify_phase3_prefilter_registry.py") },
    @{ Name = "interpretation"; Args = @("tools/audit_interpretation_localization.py") },
    @{ Name = "list+verify"; Args = @("tools/list_phase3_prefilter_tests.py", "--verify") },
    @{ Name = "qt-targets"; Args = @("tools/list_qt_creator_test_targets.py") },
    @{ Name = "deepen-registry"; Args = @("tools/verify_deepen_prefilter_registry.py") },
    @{ Name = "scenario-registry"; Args = @("tools/verify_scenario_prefilter_registry.py") },
    @{ Name = "vertical-slice-scenario"; Args = @("tools/verify_vertical_slice_scenario_coverage.py") },
    @{ Name = "interpretation-gate-scenario"; Args = @("tools/verify_interpretation_gate_scenario_coverage.py") },
    @{ Name = "customer-keeps-scenario"; Args = @("tools/verify_customer_keeps_scenario_coverage.py") },
    @{ Name = "domain-gate-scenario"; Args = @("tools/verify_domain_gate_scenario_coverage.py") },
    @{ Name = "doe-k4-fixture"; Args = @("tools/verify_doe_ccd_k4_fixture.py") }
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

Write-Host "`n>> powershell -File tools/reference_implementation_preflight.ps1" -ForegroundColor Yellow
& powershell -File (Join-Path $PSScriptRoot "reference_implementation_preflight.ps1")
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAILED: reference_implementation" -ForegroundColor Red
    $failed++
} else {
    Write-Host "OK: reference_implementation" -ForegroundColor Green
}

Write-Host "`n=== Summary ===" -ForegroundColor Cyan
if ($failed -eq 0) {
    Write-Host "Preflight passed. See docs/research/qt-creator-dual-line-acceptance-runbook.md" -ForegroundColor Green
    Write-Host "  python tools/list_qt_creator_test_targets.py --by-target"
    Write-Host "  python tools/list_qt_creator_test_targets.py --deepen --by-target"
    Write-Host "  python tools/list_qt_creator_test_targets.py --scenario --by-target"
    Write-Host "  python tools/list_qt_creator_test_targets.py --global-only --by-target"
    Write-Host "  python tools/list_qt_creator_test_targets.py --scenario-id S4 --by-target"
    Write-Host "  python tools/list_phase3_prefilter_by_scenario.py"
    Write-Host "  python tools/list_qt_creator_test_targets.py --by-target --algorithm-regression"
    Write-Host "  Manual PDF: samples/phase0_baselines/phase3_manual_acceptance_index.md"
    exit 0
} else {
    Write-Host "$failed preflight check(s) failed. Fix before running Qt Creator tests." -ForegroundColor Red
    exit 1
}
