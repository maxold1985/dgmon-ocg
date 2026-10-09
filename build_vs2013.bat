@echo off
setlocal
REM Requires CMake 3.27.x and the Visual Studio 2013 C++ tools.
cmake -S . -B build_vs2013 -G "Visual Studio 12 2013" -A Win32
if errorlevel 1 exit /b 1
cmake --build build_vs2013 --config Release
if errorlevel 1 exit /b 1
ctest --test-dir build_vs2013 -C Release --output-on-failure
if errorlevel 1 exit /b 1
chcp 65001 >nul
build_vs2013\Release\hc_cards.exe data\cards.csv --demo
endlocal
