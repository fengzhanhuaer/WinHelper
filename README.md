# Win Helper

一个基于 Dear ImGui + Win32 + DirectX 11 的 Windows 桌面辅助工具样例。

## 功能

- 系统信息展示（计算机名、系统版本、CPU 核心数）
- 快速打开常用系统入口（控制面板、任务管理器、设置、设备管理器）
- 可扩展的标签页式工具台结构

## 目录结构

- `src/main.cpp`：主程序入口，Win32 窗口 + D3D11 + ImGui 渲染循环
- `CMakeLists.txt`：CMake 构建脚本
- `third_party/imgui`：Dear ImGui 源码目录，用于编译静态库 [`imgui`](CMakeLists.txt:13)

## 前置要求

1. Windows 10/11
2. Visual Studio 2022（安装“使用 C++ 的桌面开发”工作负载）
3. CMake 3.20+
4. 已下载 Dear ImGui 源码到 `third_party/imgui`

建议目录结构：

```text
WinHelper/
├─ CMakeLists.txt
├─ src/
│  └─ main.cpp
└─ third_party/
   └─ imgui/
      ├─ imgui.cpp
      ├─ imgui_draw.cpp
      ├─ imgui_tables.cpp
      ├─ imgui_widgets.cpp
      ├─ imgui_demo.cpp
      └─ backends/
         ├─ imgui_impl_win32.cpp
         └─ imgui_impl_dx11.cpp
```

## 构建说明

当前配置会先将 Dear ImGui 及其 Win32/DX11 后端编译为静态库 [`imgui`](CMakeLists.txt:13)，再由可执行程序 [`WinHelper`](CMakeLists.txt:29) 链接该库。

## 构建步骤（命令行）

在项目根目录执行：

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

生成可执行文件：

- `build/Release/WinHelper.exe`

## 运行

双击 `WinHelper.exe` 或在命令行执行：

```bat
build\Release\WinHelper.exe
```

## 关于 includePath 报错

如果 VS Code 提示 `windows.h` 找不到，通常是 C/C++ 扩展未绑定 MSVC 工具链，不影响 CMake 正常构建。可通过以下方式修复编辑器智能提示：

1. 安装并打开 Visual Studio Developer Command Prompt 后再启动 VS Code
2. 或在 VS Code 使用 CMake Tools 选择 MSVC Kit
3. 或配置 `.vscode/c_cpp_properties.json` 指向 Windows SDK 与 MSVC 包含目录

## 后续扩展建议

- 增加服务管理（启动/停止服务）
- 增加网络诊断（IP、DNS、连通性）
- 增加启动项管理
- 增加日志面板与任务执行反馈
