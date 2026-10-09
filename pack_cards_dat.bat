@echo off
setlocal EnableExtensions
cd /d "%~dp0"
where py >nul 2>&1
if not errorlevel 1 (
 py -3 tools\pack_starter_zip.py
) else (
 python tools\pack_starter_zip.py
)
if errorlevel 1 exit /b 1
set "BIN=build_vs2022_x64\Release\hc_zip_dat.exe"
if not exist "%BIN%" (
 echo Compile first: cmake --build build_vs2022_x64 --config Release
 exit /b 1
)
if "%HC_DAT_KEY%"=="" (
 echo Define HC_DAT_KEY before packaging, e.g. set "HC_DAT_KEY=my-key"
 exit /b 2
)
"%BIN%" encode data\cards.zip data\cards.dat "%HC_DAT_KEY%"
if errorlevel 1 exit /b 1
echo DAT ready: data\cards.dat
exit /b 0
