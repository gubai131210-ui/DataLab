# tools/check_layering.ps1
# Layer include check: src/<layer> may only include allowed layer prefixes.
# Together with the CMake link graph this enforces the layering
# (link direction is guaranteed by target_link_libraries; include direction by this script).
# Usage: powershell -ExecutionPolicy Bypass -File tools/check_layering.ps1

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path (Join-Path $root "CMakeLists.txt"))) {
    throw "Project root (CMakeLists.txt) not found."
}

# Allowed include layer prefixes per layer (tests/ is exempt: integration tests cross layers).
$allowed = @{
    "domain"         = @("domain")
    "application"    = @("application", "domain")
    "reporting"      = @("reporting", "domain")
    "infrastructure" = @("infrastructure", "reporting", "domain")
    "ui"             = @("ui", "infrastructure", "reporting", "application", "domain")
}

$prefixPattern = '^\s*#\s*include\s+"(application|domain|infrastructure|reporting|ui)/'
$violations = [System.Collections.Generic.List[string]]::new()

foreach ($layer in $allowed.Keys) {
    $dir = Join-Path $root "src\$layer"
    if (-not (Test-Path $dir)) { continue }
    Get-ChildItem -Path $dir -Recurse -Include *.cpp, *.h -File | ForEach-Object {
        $lines = Get-Content -LiteralPath $_.FullName
        for ($i = 0; $i -lt $lines.Count; $i++) {
            if ($lines[$i] -match $prefixPattern) {
                $target = $Matches[1]
                if ($allowed[$layer] -notcontains $target) {
                    $violations.Add(("{0}:{1}: include `"$target/...`" not allowed in layer $layer" -f $_.FullName, ($i + 1)))
                }
            }
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Host "layering violations:" -ForegroundColor Red
    $violations | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
    exit 1
}

Write-Host "layering check passed (src/domain|application|reporting|infrastructure|ui)" -ForegroundColor Green
exit 0
