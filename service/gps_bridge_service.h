#pragma once

#include <windows.h>

// Service name and display name
#define SERVICE_NAME L"VirtualGPSBridge"
#define SERVICE_DISPLAY_NAME L"Virtual GPS Bridge Service"

// Custom control code for reloading configuration
#define SERVICE_CONTROL_RELOAD_CONFIG 128

// Driver symbolic link name
#define VIRTUAL_GPS_DEVICE_NAME L"\\\\.\\VirtualGNSS"

// Custom IOCTL codes (must match driver definitions)
#define IOCTL_VIRTUAL_GPS_SET_COORDINATE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIRTUAL_GPS_GET_COORDINATE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

// GPS coordinate structure
struct GPSCoordinate {
    double latitude;
    double longitude;
    double altitude;
    double errorRadius;
};

// Service control functions
VOID WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD ctrlCode);

// Configuration functions
bool LoadGPSConfig(GPSCoordinate& coord);
bool ApplyGPSCoordinate(const GPSCoordinate& coord);

// Driver communication functions
HANDLE OpenDriverDevice();
bool SendCoordinateToDriver(HANDLE hDevice, const GPSCoordinate& coord);
bool GetCoordinateFromDriver(HANDLE hDevice, GPSCoordinate& coord);

// Logging functions
void LogEvent(const wchar_t* message);
void LogError(const wchar_t* message, DWORD errorCode);
