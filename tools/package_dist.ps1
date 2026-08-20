# Build a portable Windows folder under dist/ with DataLab.exe and required DLLs.
param(
    [string]$BuildDir = "build/Desktop_Qt_6_11_1_MinGW_64_bit_Release",
    [string]$DistDir = "dist",
    [string]$QtRoot = "D:\QT\6.11.1\mingw_64",
    [string]$MingwBin = "D:\QT\Tools\mingw1310_64\bin",
    [string]$CMakeBin = "D:\QT\Tools\CMake_64\bin"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

$env:Path = "$QtRoot\bin;$MingwBin;$CMakeBin;$env:Path"

$WindeployQt = Join-Path $QtRoot "bin\windeployqt.exe"
if (-not (Test-Path $WindeployQt)) {
    throw "windeployqt not found: $WindeployQt"
}

Write-Host "Configuring Release..."
& cmake -S . -B $BuildDir -G "MinGW Makefiles" `
    "-DCMAKE_BUILD_TYPE=Release" `
    "-DCMAKE_PREFIX_PATH=$QtRoot"
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }

Write-Host "Building DataLab..."
& cmake --build $BuildDir --target DataLab -j 8
if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }

$ExeSource = Join-Path $BuildDir "DataLab.exe"
if (-not (Test-Path $ExeSource)) {
    throw "DataLab.exe not found: $ExeSource"
}

if (Test-Path $DistDir) {
    Remove-Item -Recurse -Force $DistDir
}
New-Item -ItemType Directory -Path $DistDir | Out-Null

Copy-Item $ExeSource (Join-Path $DistDir "DataLab.exe")

Write-Host "Running windeployqt..."
& $WindeployQt --release --compiler-runtime --no-translations (Join-Path $DistDir "DataLab.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

$RuntimeDlls = @(
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll"
)
foreach ($dll in $RuntimeDlls) {
    $src = Join-Path $MingwBin $dll
    $dst = Join-Path $DistDir $dll
    if (Test-Path $src) {
        Copy-Item $src $dst -Force
    }
}

$SqlPlugin = Join-Path $DistDir "sqldrivers\qsqlite.dll"
if (-not (Test-Path $SqlPlugin)) {
    $srcSql = Join-Path $QtRoot "plugins\sqldrivers\qsqlite.dll"
    if (Test-Path $srcSql) {
        New-Item -ItemType Directory -Path (Join-Path $DistDir "sqldrivers") -Force | Out-Null
        Copy-Item $srcSql $SqlPlugin -Force
    }
}

$readme = @"
DataLab portable package
========================
Copy this entire folder. Run DataLab.exe. Do not move the .exe out of this folder.

Required contents:
- DataLab.exe
- Qt/MinGW DLLs next to the executable
- platforms\qwindows.dll
- sqldrivers\qsqlite.dll (for .dlab projects)
"@
Set-Content -Path (Join-Path $DistDir "README.txt") -Value $readme -Encoding UTF8

Write-Host "Packaged to $((Resolve-Path $DistDir).Path)"
Get-ChildItem $DistDir -Recurse -File | Select-Object -ExpandProperty FullName
