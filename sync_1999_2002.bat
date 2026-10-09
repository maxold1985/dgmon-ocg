@echo off
setlocal
REM Python 3 with TLS/Internet access; urllib only.
py -3 tools\sync_wikimon.py --csv data\cards.csv
if errorlevel 1 (
  python tools\sync_wikimon.py --csv data\cards.csv
)
endlocal
