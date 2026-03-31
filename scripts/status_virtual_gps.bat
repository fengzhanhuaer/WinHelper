@echo off
REM Virtual GPS Status Check Script

echo ========================================
echo Virtual GPS Status Check
echo ========================================
echo.

echo [Test Signing Mode]
bcdedit | findstr /i "testsigning"
echo.

echo [Virtual GPS Service Status]
sc query VirtualGPSBridge
echo.

echo [Virtual GNSS Driver Status]
pnputil /enum-drivers | findstr /i "virtual_gnss"
echo.

echo [Configuration File]
if exist "%~dp0..\virtual_gps_config.ini" (
    echo Configuration file exists:
    type "%~dp0..\virtual_gps_config.ini"
) else (
    echo Configuration file not found.
)
echo.

echo ========================================
echo Status Check Complete
echo ========================================
pause
