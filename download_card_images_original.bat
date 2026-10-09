@echo off
cd /d "%~dp0"
where py >nul 2>&1
if not errorlevel 1 (py -3 tools\download_card_images_original.py %*) else (python tools\download_card_images_original.py %*)
