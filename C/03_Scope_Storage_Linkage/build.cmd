@echo off
rem static_counter.c compiles alone. The linkage_pair_*.c files MUST be linked together.
rem Also produces .o files and dumps their symbol tables (nm), which the note asks you to read.
setlocal
cd /d "%~dp0"
if not exist "..\output" mkdir "..\output"
set "FLAGS=-std=c11 -Wall -Wextra -Wpedantic -g"

for %%f in (static_counter.c) do (
    echo --- %%f
    gcc %FLAGS% "%%f" -o "..\output\%%~nf.exe"
    if errorlevel 1 (echo     BUILD FAILED: %%f) else (echo     ok)
)

echo --- linkage_pair (three translation units, one link step)
gcc %FLAGS% linkage_pair_main.c linkage_pair_a.c linkage_pair_b.c -o "..\output\linkage_pair.exe"
if errorlevel 1 (echo     BUILD FAILED) else (echo     ok: ..\output\linkage_pair.exe)

echo --- compile-only objects for symbol inspection
gcc %FLAGS% -c linkage_pair_a.c -o "..\output\linkage_pair_a.o"
gcc %FLAGS% -c linkage_pair_b.c -o "..\output\linkage_pair_b.o"

where nm >nul 2>nul
if not errorlevel 1 (
    echo --- nm ..\output\linkage_pair_a.o
    nm "..\output\linkage_pair_a.o"
    echo --- nm ..\output\linkage_pair_b.o
    nm "..\output\linkage_pair_b.o"
) else (
    echo nm not on PATH - use: objdump -t ..\output\linkage_pair_a.o
)

echo.
echo Run: ..\output\static_counter.exe   and   ..\output\linkage_pair.exe
endlocal
