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

echo [1/5] Enabling Test Signing Mode...
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

echo [4/5] Installing Virtual GPS Service...
if exist "%~dp0..\service\VirtualGPSBridge.exe" (
    sc create VirtualGPSBridge binPath= "%~dp0..\service\VirtualGPSBridge.exe" start= auto DisplayName= "Virtual GPS Bridge Service"
    if %errorLevel% equ 0 (
        echo Service created successfully.
        sc start VirtualGPSBridge
        echo Service started.
    ) else (
        echo WARNING: Service creation failed. Service may already exist.
    )
) else (
    echo WARNING: Service executable not found. Skipping service installation.
    echo Expected location: %~dp0..\service\VirtualGPSBridge.exe
)
echo.

echo [5/5] Installing Virtual GNSS Driver...
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
