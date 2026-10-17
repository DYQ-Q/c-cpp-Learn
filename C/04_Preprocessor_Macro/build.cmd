@echo off
rem Build every .c in this folder into ..\output, then dump preprocessed (-E -P) copies
rem so you can READ what the macros really expand to. Needs gcc on PATH.
rem   build.cmd
setlocal
cd /d "%~dp0"
if not exist "..\output" mkdir "..\output"
set "FLAGS=-std=c11 -Wall -Wextra -Wpedantic -g"
for %%f in (*.c) do (
    echo --- %%f
    gcc %FLAGS% "%%f" -o "..\output\%%~nf.exe" -lm
    if errorlevel 1 (echo     BUILD FAILED: %%f) else (echo     ok: ..\output\%%~nf.exe)
)
echo --- preprocessed dumps (open them in an editor, look for surprises)
for %%f in (*.c) do (
    gcc -std=c11 -E -P "%%f" > "..\output\%%~nf.i"
)
echo wrote ..\output\*.i
echo.
echo Follow-up checks suggested by the note:
echo   gcc -std=c11 -DNDEBUG macro_traps.c -o ..\output\macro_traps_rel.exe -lm
echo   gcc -std=c11 -dM -E -x c nul | findstr STRICT_ANSI
echo   gcc -std=c11 -ftrack-macro-expansion=2 -c macro_traps.c -o ..\output\tm.o
endlocal
