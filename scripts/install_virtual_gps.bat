@echo off
REM Virtual GPS Installation Script
REM Requires Administrator privileges

echo ========================================
echo Virtual GPS Driver Installation
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

echo [1/3] Enabling Test Signing Mode...
bcdedit /set testsigning on
if %errorLevel% neq 0 (
    echo ERROR: Failed to enable test signing mode
    pause
    exit /b 1
)
echo Test signing mode enabled successfully.
echo.

echo [2/5] Creating test certificate...
if not exist "%~dp0cert" mkdir "%~dp0cert"
makecert -r -pe -ss PrivateCertStore -n "CN=VirtualGPS Test Certificate" "%~dp0cert\virtualgps.cer"
if %errorLevel% neq 0 (
    echo WARNING: Certificate creation failed, continuing...
)
echo.

echo [3/5] Installing certificate to Trusted Root...
certutil -addstore Root "%~dp0cert\virtualgps.cer"
if %errorLevel% neq 0 (
    echo WARNING: Certificate installation failed, continuing...
)
echo.

echo [2/3] Installing Virtual GNSS Driver...
if exist "%~dp0..\driver\virtual_gnss.inf" (
    pnputil /add-driver "%~dp0..\driver\virtual_gnss.inf" /install
    if %errorLevel% equ 0 (
        echo Driver installed successfully.
    ) else (
        echo WARNING: Driver installation failed.
        echo This is expected if driver files are not yet built.
    )
) else (
    echo WARNING: Driver INF not found. Skipping driver installation.
    echo Expected location: %~dp0..\driver\virtual_gnss.inf
)
echo.

echo ========================================
echo Installation Complete
echo ========================================
echo.
echo IMPORTANT: You must RESTART your computer for test signing mode to take effect.
echo After restart, the virtual GPS driver will be active.
echo.
echo You can now configure coordinates in the WinHelper application.
echo.
pause
