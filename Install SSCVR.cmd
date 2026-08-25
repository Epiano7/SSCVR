@echo off
setlocal
title SSCVR Installer
echo SSCVR will verify your Steam copy and install into a separate folder.
echo The normal Steam installation will not be modified.
echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-SSCVR.ps1"
set "SSCVR_EXIT=%ERRORLEVEL%"
echo.
if not "%SSCVR_EXIT%"=="0" (
    echo Installation failed. Read the error above for details.
) else (
    echo Installation finished successfully.
)
pause
exit /b %SSCVR_EXIT%
