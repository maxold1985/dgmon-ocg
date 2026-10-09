@echo off
setlocal
cd /d "%~dp0"
where py >nul 2>&1
if not errorlevel 1 (py -3 tools\update_starter_database.py --apply --export-json %*) else (python tools\update_starter_database.py --apply --export-json %*)
exit /b %errorlevel%
