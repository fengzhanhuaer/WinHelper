#include "tab_sysinfo.h"

#include <windows.h>
#include <thread>
#include <string>
#include <sstream>

#include "imgui.h"

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
static std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string result(static_cast<size_t>(n - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), n, nullptr, nullptr);
    return result;
}

static std::string GetComputerNameText() {
    wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1]{};
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(buffer, &size))
        return WideToUtf8(buffer);
    return "Unknown";
}

static std::string GetWindowsVersionText() {
    OSVERSIONINFOW osvi{};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
#pragma warning(push)
#pragma warning(disable : 4996)
    BOOL ok = GetVersionExW(&osvi);
#pragma warning(pop)
    if (ok) {
        std::ostringstream oss;
        oss << "Windows " << osvi.dwMajorVersion << '.'
            << osvi.dwMinorVersion << " (Build " << osvi.dwBuildNumber << ')';
        return oss.str();
    }
    return "Windows (Unknown Version)";
}

// ---------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------
void RenderTabSysInfo() {
    if (!ImGui::BeginTabItem("\xe7\xb3\xbb\xe7\xbb\x9f\xe4\xbf\xa1\xe6\x81\xaf")) // 系统信息
        return;

    const std::string computerName    = GetComputerNameText();
    const std::string windowsVersion  = GetWindowsVersionText();
    ImGui::BulletText("\xe8\xae\xa1\xe7\xae\x97\xe6\x9c\xba\xe5\x90\x8d: %s", computerName.c_str());      // 计算机名
    ImGui::BulletText("\xe7\xb3\xbb\xe7\xbb\x9f\xe7\x89\x88\xe6\x9c\xac: %s", windowsVersion.c_str());    // 系统版本
    ImGui::BulletText("CPU \xe6\xa0\xb8\xe5\xbf\x83\xe6\x95\xb0: %u", std::thread::hardware_concurrency()); // CPU 核心数

    ImGui::EndTabItem();
}
