# Virtual GPS Driver Build Guide

## Overview

The Virtual GPS system consists of two components:
1. **WinHelper GUI** - User interface for configuration and control
2. **Virtual GNSS Driver** - UMDF2 driver that provides location data to Windows

WinHelper communicates with the driver directly through `DeviceIoControl` on `\\.\VirtualGNSS`.

## Prerequisites

### For WinHelper GUI
- Visual Studio 2022 with "Desktop development with C++" workload
- CMake 3.20 or later
- Windows 11 x64

### For Driver Development
- Windows Driver Kit (WDK) for Windows 11
- Visual Studio 2022 with WDK integration
- Windows SDK 10.0.22621.0 or later

## Building WinHelper

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output files:
- `build/Release/WinHelper.exe` - Main GUI application
- `build/scripts/*.bat` - Installation scripts

## Building the Driver (Requires WDK)

The driver must be built using Visual Studio with WDK installed:

1. Open Visual Studio 2022
2. Create a new "User Mode Driver, Empty (UMDF V2)" project
3. Add the driver source files:
   - [`driver/virtual_gnss_driver.h`](../driver/virtual_gnss_driver.h)
   - [`driver/virtual_gnss_driver.cpp`](../driver/virtual_gnss_driver.cpp)
4. Configure project properties:
   - Target Platform: Windows 11
   - Target Configuration: x64
   - Driver Type: UMDF 2.x
5. Add the INF file: [`driver/virtual_gnss.inf`](../driver/virtual_gnss.inf)
6. Build the driver project

Typical output files:
- `virtual_gnss.dll` - Driver binary
- `virtual_gnss.inf` - Driver installation information
- `virtual_gnss.cat` - Catalog file (after signing)

## Driver Signing for Testing

### Option 1: Test Signing (Recommended for Development)

```bat
bcdedit /set testsigning on
makecert -r -pe -ss PrivateCertStore -n "CN=VirtualGPS Test Certificate" virtualgps.cer
certutil -addstore Root virtualgps.cer
signtool sign /v /s PrivateCertStore /n "VirtualGPS Test Certificate" /t http://timestamp.digicert.com virtual_gnss.dll
```

### Option 2: Use Provided Script

```bat
scripts\install_virtual_gps.bat
```

## Installation

### Automated Installation

1. Build WinHelper and driver
2. Ensure `driver/virtual_gnss.inf` and related binaries are available
3. Run as Administrator:

```bat
cd build\Release
..\scripts\install_virtual_gps.bat
```

4. Restart the computer

### Manual Installation

```bat
bcdedit /set testsigning on
pnputil /add-driver virtual_gnss.inf /install
```

Restart after enabling test-signing.

## Configuration

After installation, use WinHelper:

1. Launch `WinHelper.exe`
2. Navigate to "虚拟定位" tab
3. Enter desired coordinates
4. Click "应用坐标"

## Verification

```bat
pnputil /enum-drivers | findstr virtual_gnss
```

Then in WinHelper, click "查看状态" to validate direct driver communication.

## Troubleshooting

### Driver Not Loading
- Ensure test signing is enabled: `bcdedit | findstr testsigning`
- Verify certificate is installed: `certutil -store Root`
- Check driver signature: `signtool verify /pa virtual_gnss.dll`

### Coordinates Not Updating
- Verify config file exists: `virtual_gps_config.ini`
- Ensure driver device `\\.\VirtualGNSS` can be opened by WinHelper
- Use "查看状态" button in GUI

## Uninstallation

Run as Administrator:

```bat
scripts\uninstall_virtual_gps.bat
```

Or manually:

```bat
pnputil /delete-driver virtual_gnss.inf /uninstall /force
certutil -delstore Root "VirtualGPS Test Certificate"
bcdedit /set testsigning off
```

Restart after disabling test-signing.

## Architecture

```
┌─────────────────────┐
│  WinHelper GUI      │
│  (User Interface)   │
└──────────┬──────────┘
           │ DeviceIoControl
           ▼
┌─────────────────────┐
│ Virtual GNSS Driver │
│   (UMDF2 Sensor)    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│ Windows Location    │
│ API / Apps          │
└─────────────────────┘
```

## Known Limitations

1. Windows 11 x64 only (current validation scope)
2. Test-signing required for development builds
3. Fixed coordinates only

## Future Enhancements

- GPX trajectory playback
- Richer sensor extensions (speed/heading)
- Better UI diagnostics
- Wider OS compatibility validation

## References

- [Windows Driver Kit Documentation](https://docs.microsoft.com/windows-hardware/drivers/)
- [Sensor Driver Development](https://docs.microsoft.com/windows-hardware/drivers/sensors/)
- [UMDF Driver Development](https://docs.microsoft.com/windows-hardware/drivers/wdf/getting-started-with-umdf-version-2)
