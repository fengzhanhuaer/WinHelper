# Virtual GNSS Driver - Visual Studio Project Setup

## Prerequisites

- Visual Studio 2022
- Windows Driver Kit (WDK) for Windows 11
- Windows SDK 10.0.22621.0 or later

## Creating the Driver Project

### Step 1: Create New Driver Project

1. Open Visual Studio 2022
2. File → New → Project
3. Search for "User Mode Driver"
4. Select "User Mode Driver, Empty (UMDF V2)"
5. Name: `VirtualGNSSDriver`
6. Location: `d:\Code\WinHelper\driver\`
7. Click Create

### Step 2: Add Source Files

Add the following files to the project:
- [`virtual_gnss_driver.h`](virtual_gnss_driver.h)
- [`virtual_gnss_driver.cpp`](virtual_gnss_driver.cpp)
- [`virtual_gnss.inf`](virtual_gnss.inf)

### Step 3: Configure Project Properties

Right-click project → Properties:

#### General
- Target Platform: Windows 11
- Target Platform Version: Latest
- Configuration Type: Dynamic Library (.dll)
- Driver Type: UMDF
- UMDF Version: 2.x

#### C/C++ → General
- Additional Include Directories: Add WDK sensor headers
  ```
  $(DDK_INC_PATH)
  $(SDK_INC_PATH)
  ```

#### C/C++ → Preprocessor
- Preprocessor Definitions:
  ```
  _WIN64
  _AMD64_
  AMD64
  UMDF_VERSION_MAJOR=2
  UMDF_VERSION_MINOR=25
  ```

#### Linker → Input
- Additional Dependencies:
  ```
  SensorsCx.lib
  WdfDriverStubUm.lib
  ```

#### Driver Settings
- Target OS Version: Windows 11
- Target Platform: Desktop

### Step 4: Build the Driver

1. Select Configuration: Release, Platform: x64
2. Build → Build Solution (Ctrl+Shift+B)

Output files will be in:
```
driver\x64\Release\
├── virtual_gnss.dll
├── virtual_gnss.inf
└── virtual_gnss.pdb
```

### Step 5: Sign the Driver

#### Option A: Test Signing (Development)

```bat
cd driver\x64\Release

REM Create test certificate (one time)
makecert -r -pe -ss PrivateCertStore -n "CN=VirtualGPS Test Certificate" virtualgps.cer

REM Install certificate to Trusted Root (one time)
certutil -addstore Root virtualgps.cer

REM Sign the driver
signtool sign /v /s PrivateCertStore /n "VirtualGPS Test Certificate" /t http://timestamp.digicert.com virtual_gnss.dll

REM Create catalog file
inf2cat /driver:. /os:10_X64

REM Sign the catalog
signtool sign /v /s PrivateCertStore /n "VirtualGPS Test Certificate" /t http://timestamp.digicert.com virtual_gnss.cat
```

#### Option B: Use Installation Script

The installation script handles test signing automatically:
```bat
cd ..\..\..\build\Release
..\scripts\install_virtual_gps.bat
```

### Step 6: Install the Driver

```bat
REM Enable test signing (requires restart)
bcdedit /set testsigning on

REM Install driver
pnputil /add-driver virtual_gnss.inf /install

REM Verify installation
pnputil /enum-drivers | findstr virtual_gnss
```

## Troubleshooting

### Build Errors

**Error: Cannot find SensorsCx.h**
- Solution: Install Windows Driver Kit (WDK)
- Verify WDK path in project properties

**Error: Unresolved external symbol**
- Solution: Add SensorsCx.lib to linker input
- Check library directories include WDK lib path

### Installation Errors

**Error: Driver signature verification failed**
- Solution: Enable test signing mode
- Verify certificate is installed in Trusted Root

**Error: Device not found**
- Solution: Driver creates a root-enumerated device
- Check Device Manager → Sensors for "Virtual GNSS Location Sensor"

### Runtime Errors

**Driver loads but no data**
- Check Event Viewer → Windows Logs → System
- Look for VirtualGNSS driver events
- Verify service is running and connected

## Device Symbolic Link

The driver creates a device with symbolic link:
```
\\.\VirtualGNSS
```

This is used by the service for IOCTL communication.

## Testing

### Verify Driver Installation

```bat
REM Check driver status
pnputil /enum-drivers | findstr virtual_gnss

REM Check device in Device Manager
devmgmt.msc
→ Sensors
  → Virtual GNSS Location Sensor
```

### Test IOCTL Communication

Use the VirtualGPSBridge service or create a test application:

```cpp
HANDLE hDevice = CreateFile(
    L"\\\\.\\VirtualGNSS",
    GENERIC_READ | GENERIC_WRITE,
    0, nullptr, OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL, nullptr
);

if (hDevice != INVALID_HANDLE_VALUE) {
    // Send coordinate
    GPS_COORDINATE_DATA coord = { 37.3337, -121.8907, 25.0, 10.0, 0 };
    DWORD bytesReturned;
    
    DeviceIoControl(
        hDevice,
        IOCTL_VIRTUAL_GPS_SET_COORDINATE,
        &coord, sizeof(coord),
        nullptr, 0,
        &bytesReturned, nullptr
    );
    
    CloseHandle(hDevice);
}
```

### Verify Location Data

1. Open Windows Maps application
2. Check if location shows the configured coordinates
3. Or use Windows Location API test application

## Debugging

### Enable Driver Verifier

```bat
verifier /standard /driver virtual_gnss.dll
```

### Attach Debugger

1. Configure kernel debugging
2. Set breakpoints in driver code
3. Use WinDbg for kernel debugging

### View Debug Output

```bat
REM Use DebugView from Sysinternals
DbgView.exe
```

## Production Deployment

For production use, you need:

1. **EV Code Signing Certificate**
   - Purchase from trusted CA (DigiCert, Sectigo, etc.)
   - Cost: ~$300-500/year

2. **Windows Hardware Dev Center**
   - Create account at partner.microsoft.com
   - Submit driver for attestation signing

3. **Driver Package**
   - Sign with EV certificate
   - Submit to Microsoft for attestation
   - Distribute signed package

## References

- [UMDF Driver Development](https://docs.microsoft.com/windows-hardware/drivers/wdf/getting-started-with-umdf-version-2)
- [Sensor Driver Development](https://docs.microsoft.com/windows-hardware/drivers/sensors/)
- [Driver Signing Requirements](https://docs.microsoft.com/windows-hardware/drivers/install/driver-signing)
