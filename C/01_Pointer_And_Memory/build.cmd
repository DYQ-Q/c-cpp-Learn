@echo off
rem Compile every .c in this folder into ..\output with the flags recommended by the
rem notes in this directory. Needs gcc on PATH (MSYS2 MinGW-w64 15.2).
rem   build.cmd          build all with -std=c11 -Wall -Wextra -Wpedantic -g
rem   build.cmd -O2      append extra flags (used by dynamic_matrix.c timing test)
setlocal
cd /d "%~dp0"
if not exist "..\output" mkdir "..\output"
set "FLAGS=-std=c11 -Wall -Wextra -Wpedantic -g"
for %%f in (*.c) do (
    echo --- %%f
    gcc %FLAGS% "%%f" -o "..\output\%%~nf.exe" %*
    if errorlevel 1 (echo     BUILD FAILED: %%f) else (echo     ok: ..\output\%%~nf.exe)
)
echo.
echo Binaries land in ..\output (git-ignored). Run one, e.g.:
echo     ..\output\pointer_arithmetic.exe
endlocal
