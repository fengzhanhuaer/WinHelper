# Win Helper

一个基于 Dear ImGui + Win32 + DirectX 11 的 Windows 桌面辅助工具样例。

## 功能

- **系统信息展示**：计算机名、系统版本、CPU 核心数
- **快速操作**：一键打开控制面板、任务管理器、设置、设备管理器
- **虚拟 GPS 定位**：为 Windows 提供虚拟 GPS 定位服务，支持固定坐标设置
- **可扩展架构**：标签页式工具台结构，易于添加新功能

## 目录结构

```
WinHelper/
├── src/                    # 主程序源码
│   ├── main.cpp           # 程序入口
│   ├── tab_*.cpp/h        # 各功能标签页
│   └── matrix_rain.*      # 背景特效
├── service/               # GPS 桥接服务
│   └── gps_bridge_service.*
├── driver/                # 虚拟 GNSS 驱动（需 WDK 构建）
│   ├── virtual_gnss.inf
│   └── virtual_gnss_driver.*
├── scripts/               # 安装/卸载脚本
│   ├── install_virtual_gps.bat
│   └── uninstall_virtual_gps.bat
├── docs/                  # 文档
│   ├── VIRTUAL_GPS_USER_GUIDE.md
│   └── VIRTUAL_GPS_BUILD_GUIDE.md
├── third_party/imgui/     # Dear ImGui 库
└── CMakeLists.txt         # 构建配置
```

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

- `build/Release/WinHelper.exe` - 主程序
- `build/Release/VirtualGPSBridge.exe` - GPS 桥接服务
- `build/scripts/*.bat` - 安装脚本

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

## 虚拟 GPS 功能

WinHelper 现已集成虚拟 GPS 定位系统，允许为 Windows 应用提供模拟的 GPS 位置数据。

### 快速使用

1. 构建项目后，以管理员身份运行安装脚本：
   ```bat
   cd build\Release
   ..\scripts\install_virtual_gps.bat
   ```

2. **重启计算机**（启用测试签名模式）

3. 启动 WinHelper，切换到"虚拟定位"标签页

4. 输入目标坐标并点击"应用坐标"

### 详细文档

- [用户指南](docs/VIRTUAL_GPS_USER_GUIDE.md) - 安装、配置和使用说明
- [构建指南](docs/VIRTUAL_GPS_BUILD_GUIDE.md) - 驱动开发和签名详解

### 注意事项

- 需要启用 Windows 测试签名模式
- 仅支持 Windows 11 x64
- 驱动需要使用 WDK 单独构建
- 仅用于开发和测试目的

## 后续扩展建议

- 完善虚拟 GPS 驱动的传感器数据上报
- 增加 GPX 轨迹文件回放功能
- 增加服务管理（启动/停止服务）
- 增加网络诊断（IP、DNS、连通性）
- 增加启动项管理
- 增加日志面板与任务执行反馈
