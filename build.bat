@echo off
setlocal

cd /d "%~dp0"

set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%VCVARS%" (
    echo Error: Visual Studio C++ build tools were not found.
    echo Expected file: %VCVARS%
    exit /b 1
)

call "%VCVARS%"
if errorlevel 1 (
    echo Error: Could not initialize the Visual Studio compiler environment.
    exit /b 1
)

echo.
echo Building MMORPG LFG Dungeon Queue Simulator...
cl /nologo /std:c++17 /EHsc /W4 /permissive- main.cpp config.cpp simulation.cpp /Fe:dungeon_lfg.exe
if errorlevel 1 (
    echo.
    echo Build failed.
    exit /b 1
)

echo.
echo Build succeeded: dungeon_lfg.exe
exit /b 0
