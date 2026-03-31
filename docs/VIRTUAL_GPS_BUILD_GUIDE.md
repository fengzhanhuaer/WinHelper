# Virtual GPS Driver Build Guide

## Overview

The Virtual GPS system consists of three components:
1. **WinHelper GUI** - User interface for configuration and control
2. **VirtualGPSBridge Service** - Windows service that manages GPS data
3. **Virtual GNSS Driver** - UMDF2 kernel driver that provides location data to Windows

## Prerequisites

### For GUI and Service
- Visual Studio 2022 with "Desktop development with C++" workload
- CMake 3.20 or later
- Windows 11 x64

### For Driver Development
- Windows Driver Kit (WDK) for Windows 11
- Visual Studio 2022 with WDK integration
- Windows SDK 10.0.22621.0 or later

## Building the GUI and Service

```bat
# Generate build files
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# Build Release version
cmake --build build --config Release
```

Output files:
- `build/Release/WinHelper.exe` - Main GUI application
- `build/Release/VirtualGPSBridge.exe` - GPS bridge service
- `build/scripts/*.bat` - Installation scripts

## Building the Driver (Requires WDK)

The driver must be built using Visual Studio with WDK installed:

1. Open Visual Studio 2022
2. Create a new "User Mode Driver, Empty (UMDF V2)" project
3. Add the driver source files:
   - [`driver/virtual_gnss_driver.h`](driver/virtual_gnss_driver.h)
   - [`driver/virtual_gnss_driver.cpp`](driver/virtual_gnss_driver.cpp)
4. Configure project properties:
   - Target Platform: Windows 11
   - Target Configuration: x64
   - Driver Type: UMDF 2.x
5. Add the INF file: [`driver/virtual_gnss.inf`](driver/virtual_gnss.inf)
6. Build the driver project

Output files:
- `virtual_gnss.dll` - Driver binary
- `virtual_gnss.inf` - Driver installation information
- `virtual_gnss.cat` - Catalog file (after signing)

## Driver Signing for Testing

### Option 1: Test Signing (Recommended for Development)

```bat
# Enable test signing mode (requires admin and restart)
bcdedit /set testsigning on

# Create test certificate
makecert -r -pe -ss PrivateCertStore -n "CN=VirtualGPS Test Certificate" virtualgps.cer

# Install certificate to Trusted Root
certutil -addstore Root virtualgps.cer

# Sign the driver
signtool sign /v /s PrivateCertStore /n "VirtualGPS Test Certificate" /t http://timestamp.digicert.com virtual_gnss.dll
```

### Option 2: Self-Signed Certificate

Use the installation script which automates test signing:
```bat
scripts\install_virtual_gps.bat
```

## Installation

### Automated Installation

1. Build all components
2. Copy driver files to `build/driver/` directory
3. Run as Administrator:
   ```bat
   cd build\Release
   ..\scripts\install_virtual_gps.bat
   ```

### Manual Installation

1. Enable test signing:
   ```bat
   bcdedit /set testsigning on
   ```

2. Install the service:
   ```bat
   sc create VirtualGPSBridge binPath= "C:\path\to\VirtualGPSBridge.exe" start= auto
   sc start VirtualGPSBridge
   ```

3. Install the driver:
   ```bat
   pnputil /add-driver virtual_gnss.inf /install
   ```

4. Restart the computer

## Configuration

After installation, use the WinHelper GUI:

1. Launch `WinHelper.exe`
2. Navigate to "虚拟定位" (Virtual GPS) tab
3. Enter desired coordinates:
   - Latitude (纬度): e.g., 39.9042
   - Longitude (经度): e.g., 116.4074
   - Altitude (海拔): e.g., 50.0
4. Click "保存配置" (Save Config)
5. Click "应用坐标" (Apply Coordinates)

## Verification

Check if the virtual GPS is working:

```bat
# Check service status
sc query VirtualGPSBridge

# Check driver status
pnputil /enum-drivers | findstr virtual_gnss

# Or use the GUI status button
```

## Troubleshooting

### Driver Not Loading
- Ensure test signing is enabled: `bcdedit | findstr testsigning`
- Verify certificate is installed: `certutil -store Root`
- Check driver signature: `signtool verify /pa virtual_gnss.dll`

### Service Not Starting
- Check Event Viewer: Windows Logs → Application
- Verify service exists: `sc query VirtualGPSBridge`
- Check file permissions on service executable

### Coordinates Not Updating
- Verify config file exists: `virtual_gps_config.ini`
- Check service is running
- Use "查看状态" (Check Status) button in GUI

## Uninstallation

Run as Administrator:
```bat
scripts\uninstall_virtual_gps.bat
```

Or manually:
```bat
# Stop and remove service
sc stop VirtualGPSBridge
sc delete VirtualGPSBridge

# Uninstall driver
pnputil /delete-driver virtual_gnss.inf /uninstall /force

# Remove certificate
certutil -delstore Root "VirtualGPS Test Certificate"

# Disable test signing (optional)
bcdedit /set testsigning off
```

## Architecture

```
┌─────────────────────┐
│  WinHelper GUI      │
│  (User Interface)   │
└──────────┬──────────┘
           │ Config File
           ▼
┌─────────────────────┐
│ VirtualGPSBridge    │
│ (Windows Service)   │
└──────────┬──────────┘
           │ IOCTL (Future)
           ▼
┌─────────────────────┐
│ Virtual GNSS Driver │
│ (UMDF2 Sensor)      │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│ Windows Location    │
│ API / Apps          │
└─────────────────────┘
```

## Known Limitations

1. **Driver is a skeleton implementation** - Full sensor data reporting needs completion
2. **IOCTL communication** - Service-to-driver communication is stubbed
3. **Windows 11 only** - Tested on Windows 11 x64
4. **Test signing required** - Production use requires proper code signing certificate
5. **Fixed coordinates only** - Trajectory playback not yet implemented

## Future Enhancements

- Complete sensor data field implementation
- Add IOCTL interface for service-driver communication
- Implement trajectory playback from GPX files
- Add support for multiple location sensors
- Implement proper error handling and logging
- Add Windows 10 compatibility

## References

- [Windows Driver Kit Documentation](https://docs.microsoft.com/windows-hardware/drivers/)
- [Sensor Driver Development](https://docs.microsoft.com/windows-hardware/drivers/sensors/)
- [UMDF Driver Development](https://docs.microsoft.com/windows-hardware/drivers/wdf/getting-started-with-umdf-version-2)
