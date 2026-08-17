@echo off
setlocal enableextensions
REM ---------------------------------------------------------------------------
REM Build (and optionally run) Workbench the way the README and CI workflow do:
REM in the project .venv with build isolation OFF.
REM
REM Why not `uv run` / `uv add` / `pip install .` in a fresh env? The Shiboken6
REM *generator* (shiboken6-generator) lives on Qt's wheel index, NOT on PyPI. An
REM isolated build can't see it, so CMake generates the bindings but the compile
REM fails with "Cannot open include file: 'shiboken.h' / 'sbkpep.h'". Building in
REM a venv that already has the generator, with --no-build-isolation, fixes it.
REM
REM Usage:
REM   build.cmd            build + editable install
REM   build.cmd run        build, then launch the workbench app
REM   build.cmd run x.slang  build, then open x.slang
REM ---------------------------------------------------------------------------
cd /d "%~dp0"

set "VENV_PY=.venv\Scripts\python.exe"
if not exist "%VENV_PY%" (
    echo [build] creating .venv ...
    python -m venv .venv || goto :fail
)

REM --- Qt 6.8 dev SDK -> CMAKE_PREFIX_PATH -----------------------------------
if not defined CMAKE_PREFIX_PATH (
    if exist "%~dp06.8.3\msvc2022_64" set "CMAKE_PREFIX_PATH=%~dp06.8.3\msvc2022_64"
)
if not defined CMAKE_PREFIX_PATH (
    echo [build] ERROR: set CMAKE_PREFIX_PATH to your Qt 6.8 msvc2022_64 folder.
    goto :fail
)

REM --- Slang SDK -> SLANG_ROOT -----------------------------------------------
if not defined SLANG_ROOT (
    if exist "C:\dev\app\slang\include\slang.h" set "SLANG_ROOT=C:\dev\app\slang"
)
if not defined SLANG_ROOT (
    echo [build] ERROR: set SLANG_ROOT to your Slang SDK folder.
    goto :fail
)

echo [build] python            = %VENV_PY%
echo [build] CMAKE_PREFIX_PATH = %CMAKE_PREFIX_PATH%
echo [build] SLANG_ROOT        = %SLANG_ROOT%

REM --- build deps (shiboken6-generator is Qt-index only) ---------------------
"%VENV_PY%" -c "import shiboken6_generator, scikit_build_core" 2>nul
if errorlevel 1 (
    echo [build] installing build deps from Qt's wheel index ...
    "%VENV_PY%" -m pip install --upgrade pip || goto :fail
    "%VENV_PY%" -m pip install ^
        --index-url https://download.qt.io/official_releases/QtForPython/ ^
        --extra-index-url https://pypi.org/simple ^
        --trusted-host download.qt.io ^
        "PySide6==6.8.*" "shiboken6==6.8.*" "shiboken6-generator==6.8.*" ^
        "scikit-build-core==1.0.3" || goto :fail
)

REM --- build + editable install (isolation OFF) -----------------------------
echo [build] building extension (editable, --no-build-isolation) ...
"%VENV_PY%" -m pip install --no-build-isolation -e . -v || goto :fail

echo [build] done.

if /i "%~1"=="run" (
    echo [build] launching workbench ...
    "%VENV_PY%" -m miskeyed.workbench %2 %3 %4
)
goto :eof

:fail
echo [build] FAILED (exit %errorlevel%).
exit /b 1
