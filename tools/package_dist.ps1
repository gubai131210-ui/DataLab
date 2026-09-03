# Build a portable Windows folder under dist/ with DataLab.exe and required DLLs.
param(
    [string]$BuildDir = "build/Desktop_Qt_6_11_1_MinGW_64_bit_Release",
    [string]$DistDir = "dist",
    [string]$QtRoot = "D:\QT\6.11.1\mingw_64",
    [string]$MingwBin = "D:\QT\Tools\mingw1310_64\bin",
    [string]$CMakeBin = "D:\QT\Tools\CMake_64\bin",
    [switch]$SkipConfigure,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

$env:Path = "$QtRoot\bin;$MingwBin;$CMakeBin;$env:Path"

$WindeployQt = Join-Path $QtRoot "bin\windeployqt.exe"
if (-not (Test-Path $WindeployQt)) {
    throw "windeployqt not found: $WindeployQt"
}

if (-not $SkipConfigure) {
    Write-Host "Configuring Release..."
    & cmake -S . -B $BuildDir -G "MinGW Makefiles" `
        "-DCMAKE_BUILD_TYPE=Release" `
        "-DCMAKE_PREFIX_PATH=$QtRoot"
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
}

if (-not $SkipBuild) {
    Write-Host "Building DataLab and DataLabLicenseAdmin..."
    & cmake --build $BuildDir --target DataLab DataLabLicenseAdmin -j 8
    if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }
}

$ExeSource = Join-Path $BuildDir "DataLab.exe"
if (-not (Test-Path $ExeSource)) {
    $ExeSource = Join-Path $DistDir "DataLab.exe"
}
if (-not (Test-Path $ExeSource)) {
    throw "DataLab.exe not found in $BuildDir or $DistDir"
}

if (Test-Path $DistDir) {
    Remove-Item -Recurse -Force $DistDir
}
New-Item -ItemType Directory -Path $DistDir | Out-Null

Copy-Item $ExeSource (Join-Path $DistDir "DataLab.exe")

$IconSource = Join-Path $RepoRoot "resources\icons\Q_MES.ico"
if (Test-Path $IconSource) {
    Copy-Item $IconSource (Join-Path $DistDir "Q_MES.ico") -Force
}

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

# Learning-center demo SQLite (also embedded in exe via qrc; keep a loose copy for export/backup).
$LearningDbSource = Join-Path $RepoRoot "resources\help\learning_center.sqlite"
if (-not (Test-Path $LearningDbSource)) {
    throw "learning_center.sqlite not found: $LearningDbSource"
}
$LearningHelpDir = Join-Path $DistDir "help"
New-Item -ItemType Directory -Path $LearningHelpDir -Force | Out-Null
Copy-Item $LearningDbSource (Join-Path $LearningHelpDir "learning_center.sqlite") -Force

$readmeSource = Join-Path $PSScriptRoot "dist_readme.txt"
if (-not (Test-Path $readmeSource)) {
    throw "dist_readme.txt not found: $readmeSource"
}
Copy-Item $readmeSource (Join-Path $DistDir "README.txt") -Force

$RequiredPaths = @(
    "DataLab.exe",
    "Q_MES.ico",
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Widgets.dll",
    "Qt6Network.dll",
    "Qt6Sql.dll",
    "Qt6Svg.dll",
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll",
    "platforms\qwindows.dll",
    "sqldrivers\qsqlite.dll",
    "tls\qschannelbackend.dll",
    "help\learning_center.sqlite"
)
$missing = @()
foreach ($rel in $RequiredPaths) {
    if (-not (Test-Path (Join-Path $DistDir $rel))) {
        $missing += $rel
    }
}
if ($missing.Count -gt 0) {
    throw ("Portable dist incomplete. Missing: " + ($missing -join ", "))
}

Write-Host "Portable dist verification: PASS"
Write-Host "Packaged to $((Resolve-Path $DistDir).Path)"

# ---------------------------------------------------------------------------
# License admin tool (admin-only; separate from end-user portable dist)
# ---------------------------------------------------------------------------
$AdminDistDir = "dist-license-admin"
$AdminExeSource = Join-Path $BuildDir "DataLabLicenseAdmin.exe"
if (-not (Test-Path $AdminExeSource)) {
    $AdminExeSource = Join-Path $AdminDistDir "DataLabLicenseAdmin.exe"
}
if (-not (Test-Path $AdminExeSource)) {
    throw "DataLabLicenseAdmin.exe not found in $BuildDir or $AdminDistDir"
}

if (Test-Path $AdminDistDir) {
    Remove-Item -Recurse -Force $AdminDistDir
}
New-Item -ItemType Directory -Path $AdminDistDir | Out-Null

Copy-Item $AdminExeSource (Join-Path $AdminDistDir "DataLabLicenseAdmin.exe")
if (Test-Path $IconSource) {
    Copy-Item $IconSource (Join-Path $AdminDistDir "Q_MES.ico") -Force
}

$PrivateKeySource = Join-Path $RepoRoot "tools\license_admin\license_private.key"
if (Test-Path $PrivateKeySource) {
    Copy-Item $PrivateKeySource (Join-Path $AdminDistDir "license_private.key") -Force
} else {
    Write-Warning "license_private.key not found. Run tools\generate_license_keys.ps1 before using the admin tool."
}

Write-Host "Running windeployqt for DataLabLicenseAdmin..."
& $WindeployQt --release --compiler-runtime --no-translations (Join-Path $AdminDistDir "DataLabLicenseAdmin.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed for DataLabLicenseAdmin" }

foreach ($dll in $RuntimeDlls) {
    $src = Join-Path $MingwBin $dll
    $dst = Join-Path $AdminDistDir $dll
    if (Test-Path $src) {
        Copy-Item $src $dst -Force
    }
}

$AdminSqlPlugin = Join-Path $AdminDistDir "sqldrivers\qsqlite.dll"
if (-not (Test-Path $AdminSqlPlugin)) {
    $srcSql = Join-Path $QtRoot "plugins\sqldrivers\qsqlite.dll"
    if (Test-Path $srcSql) {
        New-Item -ItemType Directory -Path (Join-Path $AdminDistDir "sqldrivers") -Force | Out-Null
        Copy-Item $srcSql $AdminSqlPlugin -Force
    }
}

$adminReadmeSource = Join-Path $PSScriptRoot "dist_license_admin_readme.txt"
if (-not (Test-Path $adminReadmeSource)) {
    throw "dist_license_admin_readme.txt not found: $adminReadmeSource"
}
Copy-Item $adminReadmeSource (Join-Path $AdminDistDir "README.txt") -Force

$AdminRequiredPaths = @(
    "DataLabLicenseAdmin.exe",
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Widgets.dll",
    "Qt6Sql.dll",
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll",
    "platforms\qwindows.dll",
    "sqldrivers\qsqlite.dll"
)
$adminMissing = @()
foreach ($rel in $AdminRequiredPaths) {
    if (-not (Test-Path (Join-Path $AdminDistDir $rel))) {
        $adminMissing += $rel
    }
}
if ($adminMissing.Count -gt 0) {
    throw ("License admin dist incomplete. Missing: " + ($adminMissing -join ", "))
}

Write-Host "License admin dist verification: PASS"
Write-Host "Admin packaged to $((Resolve-Path $AdminDistDir).Path)"

Get-ChildItem $DistDir -Recurse -File | Select-Object -ExpandProperty FullName
Get-ChildItem $AdminDistDir -Recurse -File | Select-Object -ExpandProperty FullName
