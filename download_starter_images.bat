@echo off
setlocal
cd /d "%~dp0"
where py >nul 2>&1
if not errorlevel 1 (py -3 tools\download_starter_images.py %*) else (python tools\download_starter_images.py %*)
endlocal
