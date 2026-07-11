@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\setup-toolchain.ps1"
if errorlevel 1 (
    echo.
    echo SETUP FAILED
    pause
    exit /b 1
)
echo.
echo SETUP OK
pause

