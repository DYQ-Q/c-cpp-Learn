@echo off
rem Build every .c in this folder into ..\output (flags = "layer 0" described in the note).
rem float_compare.c needs the math library, so -lm is appended for all targets here.
rem   build.cmd          build all
rem   build.cmd -O2      append extra flags (compare optimisation effects)
setlocal
cd /d "%~dp0"
if not exist "..\output" mkdir "..\output"
set "FLAGS=-std=c11 -Wall -Wextra -Wpedantic -g"
for %%f in (*.c) do (
    echo --- %%f
    gcc %FLAGS% "%%f" -o "..\output\%%~nf.exe" -lm %*
    if errorlevel 1 (echo     BUILD FAILED: %%f) else (echo     ok: ..\output\%%~nf.exe)
)
echo.
echo Optional: enable the deliberately-UB block in promotion_and_overflow.c
echo     gcc -std=c11 -O0 -DRUN_UB=1 promotion_and_overflow.c -o ..\output\p_ub.exe
echo Note: a build with -Wpedantic on Windows may print warnings in GBK; see gcc_g++_cmake.md chcp section.
endlocal
