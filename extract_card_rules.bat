@echo off
cd /d "%~dp0"
where py >nul 2>&1
if not errorlevel 1 (py -3 tools\extract_card_rules.py %*) else (python tools\extract_card_rules.py %*)
