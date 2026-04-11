@echo off
REM Virtual GPS Uninstallation Script
REM Requires Administrator privileges

echo ========================================
echo Virtual GPS Driver Uninstallation
echo ========================================
echo.

REM Check for admin privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: This script requires Administrator privileges.
    echo Please right-click and select "Run as administrator"
    pause
    exit /b 1
)

echo [1/2] Uninstalling Virtual GNSS Driver...
if exist "%~dp0..\driver\virtual_gnss.inf" (
    pnputil /delete-driver virtual_gnss.inf /uninstall /force
    if %errorLevel% equ 0 (
        echo Driver uninstalled successfully.
    ) else (
        echo WARNING: Driver uninstallation failed or driver not installed.
    )
) else (
    echo WARNING: Driver INF not found. Skipping driver uninstallation.
)
echo.

echo [2/2] Cleaning up certificates...
certutil -delstore Root "VirtualGPS Test Certificate"
if %errorLevel% equ 0 (
    echo Certificate removed successfully.
) else (
    echo WARNING: Certificate removal failed or certificate not found.
)
echo.

echo ========================================
echo Uninstallation Complete
echo ========================================
echo.
echo NOTE: Test signing mode is still enabled.
echo To disable it, run: bcdedit /set testsigning off
echo Then restart your computer.
echo.
pause
