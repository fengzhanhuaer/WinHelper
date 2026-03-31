#include "gps_bridge_service.h"
#include <stdio.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

// Forward declaration with correct signature
VOID WINAPI ServiceMain(DWORD argc, LPWSTR* argv);

// Global service status
static SERVICE_STATUS g_serviceStatus = { 0 };
static SERVICE_STATUS_HANDLE g_statusHandle = nullptr;
static HANDLE g_stopEvent = nullptr;
static GPSCoordinate g_currentCoordinate = { 37.3337, -121.8907, 25.0, 10.0 };
static HANDLE g_driverHandle = INVALID_HANDLE_VALUE;

// ---------------------------------------------------------------------------
// Driver Communication
// ---------------------------------------------------------------------------
HANDLE OpenDriverDevice() {
    HANDLE hDevice = CreateFileW(
        VIRTUAL_GPS_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, L"Failed to open driver device (Error: %lu)", error);
        LogError(errorMsg, error);
    }

    return hDevice;
}

bool SendCoordinateToDriver(HANDLE hDevice, const GPSCoordinate& coord) {
    if (hDevice == INVALID_HANDLE_VALUE) {
        LogError(L"Invalid driver handle", 0);
        return false;
    }

    DWORD bytesReturned = 0;
    GPSCoordinate driverData;
    
    // Prepare data for driver
    driverData.latitude = coord.latitude;
    driverData.longitude = coord.longitude;
    driverData.altitude = coord.altitude;
    driverData.errorRadius = coord.errorRadius;

    BOOL result = DeviceIoControl(
        hDevice,
        IOCTL_VIRTUAL_GPS_SET_COORDINATE,
        &driverData,
        sizeof(GPSCoordinate),
        nullptr,
        0,
        &bytesReturned,
        nullptr
    );

    if (!result) {
        DWORD error = GetLastError();
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, L"Failed to send coordinate to driver (Error: %lu)", error);
        LogError(errorMsg, error);
        return false;
    }

    wchar_t logMsg[256];
    swprintf_s(logMsg, L"Coordinate sent to driver: Lat=%.6f, Lon=%.6f, Alt=%.2f",
               coord.latitude, coord.longitude, coord.altitude);
    LogEvent(logMsg);

    return true;
}

bool GetCoordinateFromDriver(HANDLE hDevice, GPSCoordinate& coord) {
    if (hDevice == INVALID_HANDLE_VALUE) {
        LogError(L"Invalid driver handle", 0);
        return false;
    }

    DWORD bytesReturned = 0;
    GPSCoordinate driverData;

    BOOL result = DeviceIoControl(
        hDevice,
        IOCTL_VIRTUAL_GPS_GET_COORDINATE,
        nullptr,
        0,
        &driverData,
        sizeof(GPSCoordinate),
        &bytesReturned,
        nullptr
    );

    if (!result || bytesReturned != sizeof(GPSCoordinate)) {
        DWORD error = GetLastError();
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, L"Failed to get coordinate from driver (Error: %lu)", error);
        LogError(errorMsg, error);
        return false;
    }

    coord = driverData;
    return true;
}

// ---------------------------------------------------------------------------
// Configuration Management
// ---------------------------------------------------------------------------
bool LoadGPSConfig(GPSCoordinate& coord) {
    wchar_t configPath[MAX_PATH];
    GetModuleFileNameW(nullptr, configPath, MAX_PATH);
    PathRemoveFileSpecW(configPath);
    wcscat_s(configPath, L"\\..\\virtual_gps_config.ini");
    
    wchar_t buffer[64];
    
    // Read latitude
    GetPrivateProfileStringW(L"GPS", L"Latitude", L"37.3337", buffer, 64, configPath);
    coord.latitude = _wtof(buffer);
    
    // Read longitude
    GetPrivateProfileStringW(L"GPS", L"Longitude", L"-121.8907", buffer, 64, configPath);
    coord.longitude = _wtof(buffer);
    
    // Read altitude
    GetPrivateProfileStringW(L"GPS", L"Altitude", L"25.0", buffer, 64, configPath);
    coord.altitude = _wtof(buffer);
    
    // Set default error radius
    coord.errorRadius = 10.0;
    
    return true;
}

bool ApplyGPSCoordinate(const GPSCoordinate& coord) {
    // Store current coordinate
    g_currentCoordinate = coord;
    
    // Send to driver if connected
    if (g_driverHandle != INVALID_HANDLE_VALUE) {
        if (!SendCoordinateToDriver(g_driverHandle, coord)) {
            LogError(L"Failed to apply coordinate to driver", 0);
            return false;
        }
    } else {
        LogError(L"Driver not connected, coordinate stored but not applied", 0);
        return false;
    }
    
    wchar_t logMsg[256];
    swprintf_s(logMsg, L"Applied GPS coordinate: Lat=%.6f, Lon=%.6f, Alt=%.2f",
               coord.latitude, coord.longitude, coord.altitude);
    LogEvent(logMsg);
    
    return true;
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------
void LogEvent(const wchar_t* message) {
    HANDLE hEventLog = RegisterEventSourceW(nullptr, SERVICE_NAME);
    if (hEventLog) {
        const wchar_t* strings[1] = { message };
        ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, 0, 0,
                    nullptr, 1, 0, strings, nullptr);
        DeregisterEventSource(hEventLog);
    }
}

void LogError(const wchar_t* message, DWORD errorCode) {
    wchar_t errorMsg[512];
    if (errorCode != 0) {
        swprintf_s(errorMsg, L"%s (Error: %lu)", message, errorCode);
    } else {
        wcscpy_s(errorMsg, message);
    }
    
    HANDLE hEventLog = RegisterEventSourceW(nullptr, SERVICE_NAME);
    if (hEventLog) {
        const wchar_t* strings[1] = { errorMsg };
        ReportEventW(hEventLog, EVENTLOG_ERROR_TYPE, 0, 0,
                    nullptr, 1, 0, strings, nullptr);
        DeregisterEventSource(hEventLog);
    }
}

// ---------------------------------------------------------------------------
// Service Control
// ---------------------------------------------------------------------------
VOID WINAPI ServiceCtrlHandler(DWORD ctrlCode) {
    switch (ctrlCode) {
    case SERVICE_CONTROL_STOP:
        if (g_serviceStatus.dwCurrentState != SERVICE_RUNNING)
            break;
        
        g_serviceStatus.dwControlsAccepted = 0;
        g_serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_serviceStatus.dwWin32ExitCode = 0;
        g_serviceStatus.dwCheckPoint = 4;
        
        SetServiceStatus(g_statusHandle, &g_serviceStatus);
        SetEvent(g_stopEvent);
        break;
        
    case SERVICE_CONTROL_RELOAD_CONFIG:
        // Reload configuration
        if (LoadGPSConfig(g_currentCoordinate)) {
            ApplyGPSCoordinate(g_currentCoordinate);
            LogEvent(L"Configuration reloaded successfully");
        } else {
            LogError(L"Failed to reload configuration", GetLastError());
        }
        break;
        
    default:
        break;
    }
}

VOID WINAPI ServiceMain(DWORD argc, LPWSTR* argv) {
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    // Register service control handler
    g_statusHandle = RegisterServiceCtrlHandlerW(SERVICE_NAME, ServiceCtrlHandler);
    if (!g_statusHandle) {
        return;
    }
    
    // Initialize service status
    ZeroMemory(&g_serviceStatus, sizeof(g_serviceStatus));
    g_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_CONTROL_RELOAD_CONFIG;
    g_serviceStatus.dwCurrentState = SERVICE_START_PENDING;
    SetServiceStatus(g_statusHandle, &g_serviceStatus);
    
    // Create stop event
    g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_stopEvent) {
        g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
        g_serviceStatus.dwWin32ExitCode = GetLastError();
        SetServiceStatus(g_statusHandle, &g_serviceStatus);
        return;
    }
    
    // Open driver device
    LogEvent(L"Opening driver device...");
    g_driverHandle = OpenDriverDevice();
    
    if (g_driverHandle == INVALID_HANDLE_VALUE) {
        LogError(L"Failed to open driver device. Service will continue but coordinates cannot be applied.", 0);
        // Continue service even if driver is not available
    } else {
        LogEvent(L"Driver device opened successfully");
    }
    
    // Load initial configuration
    if (!LoadGPSConfig(g_currentCoordinate)) {
        LogError(L"Failed to load initial configuration, using defaults", GetLastError());
    }
    
    // Apply initial coordinate
    if (g_driverHandle != INVALID_HANDLE_VALUE) {
        if (!ApplyGPSCoordinate(g_currentCoordinate)) {
            LogError(L"Failed to apply initial coordinate", GetLastError());
        }
    }
    
    // Service is now running
    g_serviceStatus.dwCurrentState = SERVICE_RUNNING;
    g_serviceStatus.dwWin32ExitCode = 0;
    g_serviceStatus.dwCheckPoint = 0;
    SetServiceStatus(g_statusHandle, &g_serviceStatus);
    
    LogEvent(L"Virtual GPS Bridge Service started successfully");
    
    // Main service loop - wait for stop event
    while (WaitForSingleObject(g_stopEvent, 5000) == WAIT_TIMEOUT) {
        // Periodic tasks can be added here
        // For now, just keep the service alive
    }
    
    // Cleanup
    if (g_driverHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(g_driverHandle);
        g_driverHandle = INVALID_HANDLE_VALUE;
    }
    
    CloseHandle(g_stopEvent);
    
    g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
    g_serviceStatus.dwWin32ExitCode = 0;
    g_serviceStatus.dwCheckPoint = 3;
    SetServiceStatus(g_statusHandle, &g_serviceStatus);
    
    LogEvent(L"Virtual GPS Bridge Service stopped");
}

// ---------------------------------------------------------------------------
// Main Entry Point
// ---------------------------------------------------------------------------
int wmain(int argc, wchar_t* argv[]) {
    SERVICE_TABLE_ENTRYW serviceTable[] = {
        { const_cast<LPWSTR>(SERVICE_NAME), ServiceMain },
        { nullptr, nullptr }
    };
    
    if (!StartServiceCtrlDispatcherW(serviceTable)) {
        LogError(L"StartServiceCtrlDispatcher failed", GetLastError());
        return 1;
    }
    
    return 0;
}
