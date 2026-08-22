# Run Track G1+G2 Qt tests from a Qt Creator / CMake build directory.
# Usage (from repo root):
#   powershell -File tools/run_g1g2_tests.ps1
#   powershell -File tools/run_g1g2_tests.ps1 -BuildDir "build/Desktop_Qt_6_11_1_MinGW_64_bit_Release"

param(
    [string]$BuildDir = "build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildPath = Join-Path $Root $BuildDir

python (Join-Path $Root "tools/check_g1g2_build_ready.py") --build-dir $BuildDir
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$env:QT_QPA_PLATFORM = "offscreen"
Push-Location $BuildPath
try {
    Write-Host "# G1+G2 tests in $BuildPath" -ForegroundColor Cyan
    $pattern = "formula_registry_dialog_test|row_visibility_clipboard_test|analysis_chart_widget_test|output_workspace_test|algorithm_help_dialog_test"
    ctest -R $pattern --output-on-failure
    if ($LASTEXITCODE -ne 0) {
        Write-Host "G1+G2 tests: FAIL (exit $LASTEXITCODE)" -ForegroundColor Red
        exit $LASTEXITCODE
    }
    Write-Host "G1+G2 tests: PASS" -ForegroundColor Green
    exit 0
}
finally {
    Pop-Location
}
