#include "tab_virtualgps.h"

#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <locationapi.h>
#include <objbase.h>
#include <ctime>
#include <cstdio>
#include <string>

#include "imgui.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "locationapi.lib")

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
static const wchar_t* CONFIG_FILE = L"virtual_gps_config.ini";

// GPS coordinate state
static char s_latitude[32] = "37.3337";   // 默认圣荷西 1 S Market St
static char s_longitude[32] = "-121.8907";
static char s_altitude[32] = "25.0";
static char s_statusMessage[256] = "";

// Current system location state
static char s_systemLatitude[64] = "--";
static char s_systemLongitude[64] = "--";
static char s_systemAltitude[64] = "--";
static char s_systemLocationStatus[256] = "未获取";
static char s_systemLocationUpdatedAt[64] = "";
static bool s_locationPermissionRequested = false;

// IOCTL definitions (must match driver)
#define IOCTL_VIRTUAL_GPS_SET_COORDINATE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIRTUAL_GPS_GET_COORDINATE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct GPSCoordinateData {
    double Latitude;
    double Longitude;
    double Altitude;
    double ErrorRadius;
};

// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------
static std::wstring GetConfigPath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    std::wstring path = exePath;
    path += L"\\";
    path += CONFIG_FILE;
    return path;
}

static void SaveConfig() {
    std::wstring configPath = GetConfigPath();
    
    // Write INI format
    WritePrivateProfileStringW(L"GPS", L"Latitude", 
        std::wstring(s_latitude, s_latitude + strlen(s_latitude)).c_str(), 
        configPath.c_str());
    WritePrivateProfileStringW(L"GPS", L"Longitude", 
        std::wstring(s_longitude, s_longitude + strlen(s_longitude)).c_str(), 
        configPath.c_str());
    WritePrivateProfileStringW(L"GPS", L"Altitude", 
        std::wstring(s_altitude, s_altitude + strlen(s_altitude)).c_str(), 
        configPath.c_str());
    
    strcpy_s(s_statusMessage, "配置已保存");
}

static void LoadConfig() {
    std::wstring configPath = GetConfigPath();
    
    wchar_t buffer[32];
    GetPrivateProfileStringW(L"GPS", L"Latitude", L"37.3337", buffer, 32, configPath.c_str());
    WideCharToMultiByte(CP_UTF8, 0, buffer, -1, s_latitude, 32, nullptr, nullptr);
    
    GetPrivateProfileStringW(L"GPS", L"Longitude", L"-121.8907", buffer, 32, configPath.c_str());
    WideCharToMultiByte(CP_UTF8, 0, buffer, -1, s_longitude, 32, nullptr, nullptr);
    
    GetPrivateProfileStringW(L"GPS", L"Altitude", L"25.0", buffer, 32, configPath.c_str());
    WideCharToMultiByte(CP_UTF8, 0, buffer, -1, s_altitude, 32, nullptr, nullptr);
}

static void ExecuteScript(const wchar_t* scriptName) {
    wchar_t scriptPath[MAX_PATH];
    GetModuleFileNameW(nullptr, scriptPath, MAX_PATH);
    PathRemoveFileSpecW(scriptPath);
    wcscat_s(scriptPath, L"\\scripts\\");
    wcscat_s(scriptPath, scriptName);
    
    // Execute with admin privileges
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = scriptPath;
    sei.nShow = SW_SHOWNORMAL;
    
    if (ShellExecuteExW(&sei)) {
        strcpy_s(s_statusMessage, "脚本已启动，请查看命令行窗口");
    } else {
        strcpy_s(s_statusMessage, "脚本启动失败，请检查权限");
    }
}

static HANDLE OpenDriverDevice() {
    return CreateFileW(
        L"\\\\.\\VirtualGNSS",
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
}

static void CheckStatus() {
    HANDLE hDevice = OpenDriverDevice();
    if (hDevice == INVALID_HANDLE_VALUE) {
        strcpy_s(s_statusMessage, "驱动未就绪或未安装");
        return;
    }

    GPSCoordinateData out{};
    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        hDevice,
        IOCTL_VIRTUAL_GPS_GET_COORDINATE,
        nullptr,
        0,
        &out,
        sizeof(out),
        &bytesReturned,
        nullptr);
    CloseHandle(hDevice);

    if (ok && bytesReturned == sizeof(out)) {
        strcpy_s(s_statusMessage, "驱动通信正常");
    } else {
        strcpy_s(s_statusMessage, "驱动已安装，但通信失败");
    }
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4995) // Windows Location COM interfaces are deprecated but still required for desktop Win32 scenario.
#endif
static void RefreshSystemLocation() {
    HRESULT hrCoInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool shouldCoUninitialize = SUCCEEDED(hrCoInit);
    if (FAILED(hrCoInit) && hrCoInit != RPC_E_CHANGED_MODE) {
        sprintf_s(s_systemLocationStatus, "COM 初始化失败 (0x%08lX)", static_cast<unsigned long>(hrCoInit));
        return;
    }

    ILocation* pLocation = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_Location, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pLocation));
    if (FAILED(hr)) {
        sprintf_s(s_systemLocationStatus, "无法访问系统定位服务 (0x%08lX)", static_cast<unsigned long>(hr));
        if (shouldCoUninitialize) {
            CoUninitialize();
        }
        return;
    }

    LOCATION_REPORT_STATUS reportStatus = REPORT_NOT_SUPPORTED;
    hr = pLocation->GetReportStatus(IID_ILatLongReport, &reportStatus);
    if (SUCCEEDED(hr) && reportStatus != REPORT_RUNNING) {
        if (reportStatus == REPORT_ACCESS_DENIED) {
            if (!s_locationPermissionRequested) {
                IID reportTypes[] = { IID_ILatLongReport };
                pLocation->RequestPermissions(GetForegroundWindow(), reportTypes, 1, FALSE);
                s_locationPermissionRequested = true;
            }
            strcpy_s(s_systemLocationStatus, "定位权限未授予，请到 系统设置 > 隐私与安全性 > 位置 开启权限");
        } else if (reportStatus == REPORT_INITIALIZING) {
            strcpy_s(s_systemLocationStatus, "系统定位服务初始化中，请稍后重试");
        } else if (reportStatus == REPORT_NOT_SUPPORTED) {
            strcpy_s(s_systemLocationStatus, "当前设备不支持位置服务");
        } else {
            strcpy_s(s_systemLocationStatus, "系统定位服务异常，请稍后重试");
        }

        pLocation->Release();
        if (shouldCoUninitialize) {
            CoUninitialize();
        }
        return;
    }

    ILocationReport* pReport = nullptr;
    hr = pLocation->GetReport(IID_ILatLongReport, &pReport);
    if (FAILED(hr) || pReport == nullptr) {
        if (hr == HRESULT_FROM_WIN32(ERROR_NO_DATA) || hr == static_cast<HRESULT>(0x800700E8)) {
            strcpy_s(s_systemLocationStatus, "系统暂未提供定位数据，请先开启定位并等待几秒后重试");
        } else if (hr == E_ACCESSDENIED) {
            strcpy_s(s_systemLocationStatus, "系统定位权限被拒绝，请在 Windows 设置中允许定位");
        } else {
            sprintf_s(s_systemLocationStatus, "无法读取系统位置 (0x%08lX)", static_cast<unsigned long>(hr));
        }

        if (pReport) {
            pReport->Release();
        }
        pLocation->Release();
        if (shouldCoUninitialize) {
            CoUninitialize();
        }
        return;
    }

    ILatLongReport* pLatLongReport = nullptr;
    hr = pReport->QueryInterface(IID_PPV_ARGS(&pLatLongReport));
    pReport->Release();
    if (FAILED(hr) || pLatLongReport == nullptr) {
        sprintf_s(s_systemLocationStatus, "系统定位接口转换失败 (0x%08lX)", static_cast<unsigned long>(hr));
        pLocation->Release();
        if (shouldCoUninitialize) {
            CoUninitialize();
        }
        return;
    }

    DOUBLE latitude = 0.0;
    DOUBLE longitude = 0.0;
    DOUBLE altitude = 0.0;

    HRESULT hrLat = pLatLongReport->GetLatitude(&latitude);
    HRESULT hrLon = pLatLongReport->GetLongitude(&longitude);
    HRESULT hrAlt = pLatLongReport->GetAltitude(&altitude);

    if (SUCCEEDED(hrLat) && SUCCEEDED(hrLon)) {
        sprintf_s(s_systemLatitude, "%.6f", latitude);
        sprintf_s(s_systemLongitude, "%.6f", longitude);

        if (SUCCEEDED(hrAlt)) {
            sprintf_s(s_systemAltitude, "%.2f m", altitude);
        } else {
            strcpy_s(s_systemAltitude, "--");
        }

        std::time_t now = std::time(nullptr);
        std::tm localTm{};
        if (localtime_s(&localTm, &now) == 0) {
            std::strftime(s_systemLocationUpdatedAt, sizeof(s_systemLocationUpdatedAt), "%Y-%m-%d %H:%M:%S", &localTm);
        } else {
            strcpy_s(s_systemLocationUpdatedAt, "");
        }

        strcpy_s(s_systemLocationStatus, "系统位置获取成功");
    } else {
        strcpy_s(s_systemLocationStatus, "系统位置数据不完整");
    }

    pLatLongReport->Release();
    pLocation->Release();
    if (shouldCoUninitialize) {
        CoUninitialize();
    }
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

static void ApplyCoordinateToDriver() {
    HANDLE hDevice = OpenDriverDevice();
    if (hDevice == INVALID_HANDLE_VALUE) {
        strcpy_s(s_statusMessage, "无法打开驱动设备，请先安装驱动");
        return;
    }

    GPSCoordinateData data{};
    data.Latitude = atof(s_latitude);
    data.Longitude = atof(s_longitude);
    data.Altitude = atof(s_altitude);
    data.ErrorRadius = 10.0;

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        hDevice,
        IOCTL_VIRTUAL_GPS_SET_COORDINATE,
        &data,
        sizeof(data),
        nullptr,
        0,
        &bytesReturned,
        nullptr);
    CloseHandle(hDevice);

    if (ok) {
        strcpy_s(s_statusMessage, "坐标已直接写入驱动");
    } else {
        strcpy_s(s_statusMessage, "写入驱动失败，请检查权限/驱动状态");
    }
}

// ---------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------
void RenderTabVirtualGPS() {
    if (!ImGui::BeginTabItem("\xe8\x99\x9a\xe6\x8b\x9f\xe5\xae\x9a\xe4\xbd\x8d")) // 虚拟定位
        return;
    
    // Load config on first render
    static bool initialized = false;
    if (!initialized) {
        LoadConfig();
        RefreshSystemLocation();
        initialized = true;
    }
    
    ImGui::TextUnformatted("\xe8\x99\x9a\xe6\x8b\x9f GPS \xe5\xae\x9a\xe4\xbd\x8d\xef\xbc\x88\xe7\x9b\xb4\xe8\xbf\x9e\xe9\xa9\xb1\xe5\x8a\xa8\xef\xbc\x89"); // 虚拟 GPS 定位（直连驱动）
    ImGui::Separator();
    
    // Installation section
    ImGui::TextUnformatted("\xe5\xae\x89\xe8\xa3\x85\xe7\xae\xa1\xe7\x90\x86\xef\xbc\x9a"); // 安装管理：
    
    if (ImGui::Button("\xe5\xae\x89\xe8\xa3\x85\xe9\xa9\xb1\xe5\x8a\xa8")) { // 安装驱动
        ExecuteScript(L"install_virtual_gps.bat");
    }
    ImGui::SameLine();
    if (ImGui::Button("\xe5\x8d\xb8\xe8\xbd\xbd\xe9\xa9\xb1\xe5\x8a\xa8")) { // 卸载驱动
        ExecuteScript(L"uninstall_virtual_gps.bat");
    }
    ImGui::SameLine();
    if (ImGui::Button("\xe6\x9f\xa5\xe7\x9c\x8b\xe7\x8a\xb6\xe6\x80\x81")) { // 查看状态
        CheckStatus();
    }
    
    ImGui::Spacing();
    ImGui::Separator();

    // Current system location
    ImGui::TextUnformatted("当前系统位置：");
    ImGui::Text("纬度: %s", s_systemLatitude);
    ImGui::Text("经度: %s", s_systemLongitude);
    ImGui::Text("海拔: %s", s_systemAltitude);
    ImGui::Text("状态: %s", s_systemLocationStatus);
    ImGui::Text("更新时间: %s", s_systemLocationUpdatedAt[0] != '\0' ? s_systemLocationUpdatedAt : "未刷新");

    if (ImGui::Button("刷新系统位置")) {
        RefreshSystemLocation();
    }

    ImGui::Spacing();
    ImGui::Separator();
    
    // Coordinate configuration
    ImGui::TextUnformatted("\xe5\x9d\x90\xe6\xa0\x87\xe8\xae\xbe\xe7\xbd\xae\xef\xbc\x9a"); // 坐标设置：
    
    ImGui::PushItemWidth(200);
    ImGui::InputText("\xe7\xba\xac\xe5\xba\xa6 (Latitude)", s_latitude, sizeof(s_latitude)); // 纬度
    ImGui::InputText("\xe7\xbb\x8f\xe5\xba\xa6 (Longitude)", s_longitude, sizeof(s_longitude)); // 经度
    ImGui::InputText("\xe6\xb5\xb7\xe6\x8b\x94 (Altitude)", s_altitude, sizeof(s_altitude)); // 海拔
    ImGui::PopItemWidth();
    
    ImGui::Spacing();
    
    if (ImGui::Button("\xe4\xbf\x9d\xe5\xad\x98\xe9\x85\x8d\xe7\xbd\xae")) { // 保存配置
        SaveConfig();
    }
    ImGui::SameLine();
    if (ImGui::Button("\xe5\xba\x94\xe7\x94\xa8\xe5\x9d\x90\xe6\xa0\x87")) { // 应用坐标
        SaveConfig();
        ApplyCoordinateToDriver();
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Status message
    if (s_statusMessage[0] != '\0') {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", s_statusMessage);
    }
    
    ImGui::Spacing();
    ImGui::TextWrapped(
        "\xe6\xb3\xa8\xe6\x84\x8f\xef\xbc\x9a\xe9\xa6\x96\xe6\xac\xa1\xe4\xbd\xbf\xe7\x94\xa8\xe9\x9c\x80\xe8\xa6\x81\xe5\x90\xaf\xe7\x94\xa8\xe6\xb5\x8b\xe8\xaf\x95\xe7\xad\xbe\xe5\x90\x8d\xe6\xa8\xa1\xe5\xbc\x8f\xe3\x80\x82"
        "\xe5\xae\x89\xe8\xa3\x85\xe8\x84\x9a\xe6\x9c\xac\xe4\xbc\x9a\xe8\x87\xaa\xe5\x8a\xa8\xe5\xa4\x84\xe7\x90\x86\xe3\x80\x82\n"
        "\xe9\xa9\xb1\xe5\x8a\xa8\xe5\xae\x89\xe8\xa3\x85\xe5\x90\x8e\xef\xbc\x8c\xe7\xb3\xbb\xe7\xbb\x9f\xe5\xba\x94\xe7\x94\xa8\xe5\xb0\x86\xe8\x8e\xb7\xe5\x8f\x96\xe8\x99\x9a\xe6\x8b\x9f\xe5\xae\x9a\xe4\xbd\x8d\xe6\x95\xb0\xe6\x8d\xae\xe3\x80\x82"); 
        // 注意：首次使用需要启用测试签名模式。安装脚本会自动处理。
        // 驱动安装后，系统应用将获取虚拟定位数据。
    
    ImGui::Spacing();
    ImGui::TextWrapped(
        "\xe9\xa2\x84\xe8\xae\xbe\xe5\x9d\x90\xe6\xa0\x87\xef\xbc\x9aSan Jose, CA (37.3337, -121.8907)");
        // 预设坐标：San Jose, CA
    
    ImGui::EndTabItem();
}
