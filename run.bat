@echo off
setlocal

cd /d "%~dp0"

if not exist "dungeon_lfg.exe" (
    echo Error: dungeon_lfg.exe was not found.
    echo Run build.bat first.
    exit /b 1
)

if not exist "config.txt" (
    echo Error: config.txt was not found.
    exit /b 1
)

dungeon_lfg.exe
exit /b %ERRORLEVEL%
