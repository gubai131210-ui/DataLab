# DataLab

DataLab is a Windows Qt/C++ quality analysis tool for automotive manufacturing quality engineers.

## Current MVP slice

- C++ domain library for descriptive statistics, process capability and control charts.
- CSV import with a read-only data preview.
- SQLite-backed `.dlab` project persistence.
- I-MR analysis workflow in the Qt desktop application.
- Basic PDF report export.
- Qt Test coverage for the first statistical seams.

Core quality calculations are implemented in C++. The desktop app natively imports CSV, TXT, and XLSX without a Python runtime. Legacy `.xls` is not supported.

## Development environment

- Qt 6.11.1
- MinGW 13.1
- CMake
- C++17
- zlib (vendored miniz for native `.xlsx` import; no Python runtime required)

Optional Python virtual environment for **dev tools only** (Minitab fixture scripts, not required to run the app):

`D:\QT_CppPrograms\DataLab\.venv`

PowerShell (dev tools only):

```powershell
& ".\.venv\Scripts\python.exe" --version
& ".\.venv\Scripts\python.exe" -m pip install -r requirements.txt
```

## Build and test

```powershell
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug `
  -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug
```

Run tests from a Qt-enabled terminal so the Qt DLL directory is available:

```powershell
$env:Path = "D:\QT\6.11.1\mingw_64\bin;D:\QT\Tools\mingw1310_64\bin;$env:Path"
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug --output-on-failure
```
