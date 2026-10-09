@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "ARCH=%~1"
if "%ARCH%"=="" set "ARCH=x64"
if /I not "%ARCH%"=="x64" if /I not "%ARCH%"=="Win32" (
 echo Usage: build_vs2022.bat [x64^|Win32]
 exit /b 2
)
where cmake >nul 2>&1
if errorlevel 1 (
 echo ERROR: CMake is not available in PATH
 exit /b 1
)
set "BUILD=build_vs2022_%ARCH%"
cmake -S . -B "%BUILD%" -G "Visual Studio 17 2022" -A %ARCH% -T v143
if errorlevel 1 exit /b 1
cmake --build "%BUILD%" --config Release --parallel
if errorlevel 1 exit /b 1
ctest --test-dir "%BUILD%" -C Release --output-on-failure
if errorlevel 1 exit /b 1
"%BUILD%\Release\hc_cards.exe" "%BUILD%\data\cards.csv" --demo
exit /b %errorlevel%
