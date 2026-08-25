@echo off
setlocal
title SSCVR Uninstaller
set "SSCVR_ROOT=%~dp0"
cd /d "%TEMP%"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SSCVR_ROOT%Uninstall-SSCVR.ps1" -InstallRoot "%SSCVR_ROOT%" -Confirm:$false
set "SSCVR_EXIT=%ERRORLEVEL%"
echo.
if not "%SSCVR_EXIT%"=="0" (
    echo Uninstallation failed. Nothing outside a verified SSCVR installation was removed.
) else (
    echo SSCVR was removed. Your Steam installation was not changed.
)
pause
exit /b %SSCVR_EXIT%
