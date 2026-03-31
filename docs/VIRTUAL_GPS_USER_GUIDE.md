# 虚拟 GPS 定位系统

## 功能说明

本系统为 Windows 11 提供虚拟 GPS 定位功能，允许用户通过 WinHelper 工具设置固定坐标，系统应用将获取虚拟定位数据而非真实 GPS 信号。

## 系统架构

- **用户界面层**: WinHelper GUI 提供坐标设置和控制入口
- **驱动层**: Virtual GNSS UMDF2 驱动向 Windows Location API 提供位置数据
- **通信方式**: WinHelper 直接通过 `DeviceIoControl` 与 `\\.\VirtualGNSS` 设备通信

## 快速开始

### 1. 构建项目

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### 2. 安装系统

以管理员身份运行：
```bat
cd build\Release
..\scripts\install_virtual_gps.bat
```

**重要**: 安装后必须重启计算机以启用测试签名模式。

### 3. 配置坐标

1. 启动 `WinHelper.exe`
2. 切换到"虚拟定位"标签页
3. 输入目标坐标（默认为 San Jose, CA）
4. 点击"应用坐标"

### 4. 验证功能

- 打开 Windows 地图应用，查看当前位置
- 或使用"查看状态"按钮检查驱动通信状态

## 使用说明

### 坐标格式

- **纬度 (Latitude)**: -90.0 到 90.0，北纬为正
- **经度 (Longitude)**: -180.0 到 180.0，东经为正  
- **海拔 (Altitude)**: 米为单位，可选

### 预设坐标示例

| 位置 | 纬度 | 经度 |
|------|------|------|
| San Jose, CA | 37.3337 | -121.8907 |
| 北京天安门 | 39.9042 | 116.4074 |
| 上海东方明珠 | 31.2397 | 121.4997 |
| 广州塔 | 23.1088 | 113.3191 |

### 按钮功能

- **安装驱动**: 一键安装驱动组件并启用测试签名
- **卸载驱动**: 完全移除虚拟 GPS 驱动
- **查看状态**: 检查驱动安装状态与设备通信状态
- **保存配置**: 将坐标保存到配置文件
- **应用坐标**: 保存配置并直接写入驱动

## 注意事项

### 测试签名模式

本系统使用测试签名的驱动，需要启用 Windows 测试签名模式：
- 安装脚本会自动启用
- 启用后桌面右下角会显示"测试模式"水印
- 如需关闭：`bcdedit /set testsigning off` 并重启

### 安全性

- 仅用于开发和测试目的
- 测试签名驱动不应在生产环境使用
- 生产部署需要购买代码签名证书

### 兼容性

- **支持**: Windows 11 x64
- **未测试**: Windows 10（理论上兼容）
- **不支持**: Windows 7/8, 32位系统

## 故障排除

### 驱动安装失败

**症状**: 安装脚本报错或设备管理器中无虚拟设备

**解决方案**:
1. 确认已启用测试签名：`bcdedit | findstr testsigning`
2. 确认已重启计算机
3. 检查证书是否安装：`certutil -store Root | findstr VirtualGPS`
4. 手动安装驱动：`pnputil /add-driver driver\virtual_gnss.inf /install`

### 驱动通信失败

**症状**: 点击"应用坐标"后提示无法连接驱动设备

**解决方案**:
1. 检查驱动是否安装：`pnputil /enum-drivers | findstr virtual_gnss`
2. 重新运行安装脚本并重启系统
3. 使用"查看状态"按钮确认设备可访问
4. 确认当前进程有足够权限访问设备

### 坐标未生效

**症状**: 应用坐标后系统应用仍显示真实位置

**解决方案**:
1. 确认驱动已正确安装并加载
2. 重启 Windows Location 服务
3. 检查配置文件是否正确生成
4. 某些应用可能使用其他定位方式（IP、WiFi）

## 卸载

以管理员身份运行：
```bat
scripts\uninstall_virtual_gps.bat
```

卸载后如需关闭测试签名模式：
```bat
bcdedit /set testsigning off
```
然后重启计算机。

## 技术细节

详细的构建和开发文档请参阅：
- [构建指南](VIRTUAL_GPS_BUILD_GUIDE.md)
- [驱动开发文档](../driver/README.md)

## 许可证

本项目仅供学习和研究使用。
