@echo off
setlocal
cd /d "%~dp0"
call build.bat || exit /b 1
powershell -ExecutionPolicy Bypass -File tools\package_windows.ps1 || exit /b 1
