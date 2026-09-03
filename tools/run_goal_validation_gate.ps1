# Goal 模式验证门禁（编译 + formula_reference + wave verify）
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools/run_goal_validation_gate.ps1
#   powershell -ExecutionPolicy Bypass -File tools/run_goal_validation_gate.ps1 -Wave 15
#   powershell -ExecutionPolicy Bypass -File tools/run_goal_validation_gate.ps1 -SkipBuild

param(
    [int] $Wave = 0,
    [switch] $SkipBuild,
    [string] $BuildDir = "build-mingw"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

$QtBin = "D:\Qt\6.11.1\mingw_64\bin"
$MingwBin = "D:\Qt\Tools\mingw1310_64\bin"
$CmakeBin = "D:\Qt\Tools\CMake_64\bin"
$env:PATH = "$QtBin;$MingwBin;$CmakeBin;" + $env:PATH

$BuildPath = Join-Path $Root $BuildDir
$failed = 0

function Step($name, $action) {
    Write-Host "`n=== $name ===" -ForegroundColor Cyan
    & $action | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAILED: $name" -ForegroundColor Red
        return 1
    }
    Write-Host "OK: $name" -ForegroundColor Green
    return 0
}

if (-not $SkipBuild) {
    if (-not (Test-Path $BuildPath)) {
        New-Item -ItemType Directory -Path $BuildPath | Out-Null
        Push-Location $BuildPath
        cmake .. -G "MinGW Makefiles"
        if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
        Pop-Location
    }
    Push-Location $BuildPath
    $targets = @(
        "reliability_phase5_test",
        "nonnormal_capability_phase6_test",
        "response_surface_design_phase4_test",
        "minitab_formula_golden_test",
        "minitab_numerical_golden_test",
        "quality_statistics_test"
    )
    if ($Wave -gt 0) {
        $targets += "algorithm_wave${Wave}_track_test"
    }
    foreach ($t in $targets) {
        cmake --build . --target $t -j 8
        if ($LASTEXITCODE -ne 0) {
            Write-Host "BUILD FAILED: $t" -ForegroundColor Red
            Pop-Location
            exit 1
        }
    }
    Pop-Location
}

# Python formula-reference gate (reference scripts + optional wave verify + run exes)
$gateArgs = @("tools/verify_formula_reference_gate.py")
if ($Wave -gt 0) { $gateArgs += @("--wave", "$Wave") }
$failed += (Step "formula_reference_gate" { python @gateArgs })

$failed += (Step "learning_center_gate" { python tools/verify_learning_center_gate.py })

if (-not $SkipBuild) {
    Push-Location $BuildPath
    $learningTargets = @(
        "learning_center_store_test",
        "learning_center_worksheet_registry_test",
        "learning_center_analysis_sample_test"
    )
    foreach ($t in $learningTargets) {
        cmake --build . --target $t -j 8
        if ($LASTEXITCODE -ne 0) {
            Write-Host "BUILD FAILED: $t" -ForegroundColor Red
            Pop-Location
            exit 1
        }
        & ".\$t.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Host "TEST FAILED: $t" -ForegroundColor Red
            Pop-Location
            exit 1
        }
    }
    Pop-Location
}

# Wave regression chain (when Wave specified)
if ($Wave -gt 0) {
    for ($w = $Wave - 1; $w -ge 14; $w--) {
        $script = Join-Path $Root "tools/verify_algorithm_wave${w}_track.py"
        if (Test-Path $script) {
            $failed += (Step "verify_wave_$w" { python $script })
        }
    }
    $failed += (Step "verify_menu_ia" { python tools/verify_ui_menu_ia_track.py })
}

Write-Host "`n=== Goal validation gate summary ===" -ForegroundColor Cyan
if ($failed -gt 0) {
    Write-Host "$failed step(s) failed." -ForegroundColor Red
    exit 1
}
Write-Host "All gate steps passed." -ForegroundColor Green
exit 0
