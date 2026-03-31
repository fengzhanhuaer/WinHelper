# Virtual GPS Implementation Summary

## 完成状态

已完成 Windows 11 x64 虚拟 GPS 定位系统的驱动直连实现：WinHelper 直接与虚拟 GNSS 驱动通信，不再依赖中间服务。

## 核心能力

### 1. 完整的传感器数据上报 ✅

**驱动实现** ([`driver/virtual_gnss_driver.cpp`](../driver/virtual_gnss_driver.cpp))：

- 使用 WDF 定时器周期性更新传感器数据（默认 1 秒）
- 实现 4 个核心 GNSS 数据字段：
  - `SENSOR_DATA_TYPE_LATITUDE_DEGREES`
  - `SENSOR_DATA_TYPE_LONGITUDE_DEGREES`
  - `SENSOR_DATA_TYPE_ALTITUDE_ELLIPSOID_METERS`
  - `SENSOR_DATA_TYPE_ERROR_RADIUS_METERS`
- 完整的数据集合分配/释放流程
- 通过 `WdfWaitLock` 保护共享状态
- 实现传感器启动/停止生命周期回调

关键函数：

```cpp
CreateSensorDataCollection()  // 创建传感器数据集合
UpdateSensorData()            // 更新并上报数据
VirtualGNSSEvtTimerFunc()     // 定时器回调
VirtualGNSSEvtSensorStart()   // 启动传感器
VirtualGNSSEvtSensorStop()    // 停止传感器
```

### 2. GUI 直连驱动 IOCTL ✅

**IOCTL 定义** ([`driver/virtual_gnss_driver.h`](../driver/virtual_gnss_driver.h))：

```cpp
#define IOCTL_VIRTUAL_GPS_SET_COORDINATE  // 设置坐标
#define IOCTL_VIRTUAL_GPS_GET_COORDINATE  // 获取坐标
```

**坐标结构（仅位置）**：

```cpp
typedef struct _GPS_COORDINATE_DATA {
    DOUBLE Latitude;
    DOUBLE Longitude;
    DOUBLE Altitude;
    DOUBLE ErrorRadius;
} GPS_COORDINATE_DATA;
```

**GUI 侧驱动通信** ([`src/tab_virtualgps.cpp`](../src/tab_virtualgps.cpp))：

- `OpenDriverDevice()`：打开 `\\.\VirtualGNSS`
- `ApplyCoordinateToDriver()`：发送 `IOCTL_VIRTUAL_GPS_SET_COORDINATE`
- `CheckStatus()`：读取 `IOCTL_VIRTUAL_GPS_GET_COORDINATE`

### 3. 安装与运维脚本 ✅

- [`scripts/install_virtual_gps.bat`](../scripts/install_virtual_gps.bat)：启用测试签名、安装证书、安装驱动
- [`scripts/uninstall_virtual_gps.bat`](../scripts/uninstall_virtual_gps.bat)：卸载驱动、清理证书
- [`scripts/status_virtual_gps.bat`](../scripts/status_virtual_gps.bat)：检查测试签名与驱动状态

## 文件清单

### 核心组件

| 文件 | 说明 | 状态 |
|------|------|------|
| [`src/tab_virtualgps.h`](../src/tab_virtualgps.h) | 虚拟 GPS 标签页头文件 | ✅ 完成 |
| [`src/tab_virtualgps.cpp`](../src/tab_virtualgps.cpp) | GUI 实现、配置读写、驱动 IOCTL | ✅ 完成 |
| [`driver/virtual_gnss_driver.h`](../driver/virtual_gnss_driver.h) | 驱动头文件、数据结构与 IOCTL | ✅ 完成 |
| [`driver/virtual_gnss_driver.cpp`](../driver/virtual_gnss_driver.cpp) | 驱动实现、传感器上报 | ✅ 完成 |
| [`driver/virtual_gnss.inf`](../driver/virtual_gnss.inf) | 驱动安装信息 | ✅ 完成 |

### 构建与脚本

| 文件 | 说明 | 状态 |
|------|------|------|
| [`CMakeLists.txt`](../CMakeLists.txt) | WinHelper 构建配置 | ✅ 更新 |
| [`scripts/install_virtual_gps.bat`](../scripts/install_virtual_gps.bat) | 一键安装脚本 | ✅ 更新 |
| [`scripts/uninstall_virtual_gps.bat`](../scripts/uninstall_virtual_gps.bat) | 卸载脚本 | ✅ 更新 |
| [`scripts/status_virtual_gps.bat`](../scripts/status_virtual_gps.bat) | 状态检查脚本 | ✅ 更新 |

## 技术架构

```
┌─────────────────────────────────────────────────────────────┐
│                     WinHelper GUI                           │
│  - 坐标输入界面                                              │
│  - 安装/卸载/状态按钮                                        │
│  - 配置文件读写 (INI)                                        │
└────────────────────┬────────────────────────────────────────┘
                     │ DeviceIoControl
                     ▼
┌─────────────────────────────────────────────────────────────┐
│           Virtual GNSS UMDF2 Driver                         │
│  - 接收 IOCTL 命令                                           │
│  - 更新内部坐标状态                                          │
│  - WDF 定时器周期触发                                        │
│  - CreateSensorDataCollection()                             │
│  - SensorsCxSensorDataReady() 上报数据                      │
└────────────────────┬────────────────────────────────────────┘
                     │ Sensor Class Extension
                     ▼
┌─────────────────────────────────────────────────────────────┐
│            Windows Location API                             │
│  - 接收传感器数据                                            │
│  - 提供给系统应用                                            │
└────────────────────┬────────────────────────────────────────┘
                     ▼
              ┌──────────────┐
              │ Windows Maps │
              │ 其他应用     │
              └──────────────┘
```

## 数据流

### 坐标设置流程

1. 用户在 WinHelper 输入坐标（示例：37.3337, -121.8907）
2. GUI 保存 `virtual_gps_config.ini`
3. GUI 直接调用 `DeviceIoControl(IOCTL_VIRTUAL_GPS_SET_COORDINATE)`
4. 驱动处理 `VirtualGNSSEvtDeviceIoControl()` 并更新内部坐标
5. 驱动立即/周期上报到 Windows Location API
6. 系统应用获取新坐标

### 状态检查流程

1. GUI 打开设备 `\\.\VirtualGNSS`
2. 调用 `DeviceIoControl(IOCTL_VIRTUAL_GPS_GET_COORDINATE)`
3. 显示当前驱动返回坐标与通信结果

## 构建与部署

### 构建 WinHelper

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

输出：
- `build/Release/WinHelper.exe`
- `build/scripts/*.bat`

### 构建驱动

使用 Visual Studio 2022 + WDK：
1. 创建 UMDF2 项目
2. 添加驱动源文件
3. 配置项目属性（见 [`driver/README.md`](../driver/README.md)）
4. 构建 Release x64
5. 测试签名

## 验证建议

```bat
pnputil /enum-drivers | findstr virtual_gnss
```

在 WinHelper 中点击“查看状态”，应可读取驱动当前坐标。

## 已知范围

- 当前验证目标：Windows 11 x64
- 当前能力：固定坐标
- 当前部署：测试签名模式

## 总结

当前版本已完成“WinHelper 直连驱动”的虚拟 GPS 方案，实现可安装、可配置、可验证的系统级定位模拟能力，并移除中间服务依赖。
