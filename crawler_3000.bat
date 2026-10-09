@echo off
setlocal
cd /d "%~dp0"
where py >nul 2>&1
if not errorlevel 1 (
 py -3 tools\crawler_3000.py --target 3000 --delay 4 --year-cutoff 2002
) else (
 python tools\crawler_3000.py --target 3000 --delay 4 --year-cutoff 2002
)
endlocal
