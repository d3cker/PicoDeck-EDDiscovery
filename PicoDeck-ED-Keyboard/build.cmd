@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\build.ps1" %*
if errorlevel 1 (
    echo.
    echo BUILD FAILED
    pause
    exit /b 1
)
echo.
echo BUILD OK
echo Firmware: dist\PicoDeck-ED-Keyboard.uf2
pause

