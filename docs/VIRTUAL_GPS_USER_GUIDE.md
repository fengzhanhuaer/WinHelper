# 虚拟 GPS 定位系统

## 功能说明

本系统为 Windows 11 提供虚拟 GPS 定位功能，允许用户通过 WinHelper 工具设置固定坐标，系统应用将获取虚拟定位数据而非真实 GPS 信号。

## 系统架构

- **用户界面层**: WinHelper GUI 提供坐标设置和控制入口
- **服务层**: VirtualGPSBridge Windows 服务管理配置和驱动通信
- **驱动层**: Virtual GNSS UMDF2 驱动向 Windows Location API 提供位置数据

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
3. 输入目标坐标（默认为北京天安门）
4. 点击"应用坐标"

### 4. 验证功能

- 打开 Windows 地图应用，查看当前位置
- 或使用"查看状态"按钮检查服务和驱动状态

## 使用说明

### 坐标格式

- **纬度 (Latitude)**: -90.0 到 90.0，北纬为正
- **经度 (Longitude)**: -180.0 到 180.0，东经为正  
- **海拔 (Altitude)**: 米为单位，可选

### 预设坐标示例

| 位置 | 纬度 | 经度 |
|------|------|------|
| 北京天安门 | 39.9042 | 116.4074 |
| 上海东方明珠 | 31.2397 | 121.4997 |
| 广州塔 | 23.1088 | 113.3191 |
| 深圳市民中心 | 22.5455 | 114.0545 |

### 按钮功能

- **安装驱动与服务**: 一键安装所有组件并启用测试签名
- **卸载驱动与服务**: 完全移除虚拟 GPS 系统
- **查看状态**: 检查服务运行状态和驱动安装情况
- **保存配置**: 将坐标保存到配置文件
- **应用坐标**: 保存配置并通知服务更新位置

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

### 服务无法启动

**症状**: 点击"应用坐标"后提示服务未安装或无法连接

**解决方案**:
1. 检查服务状态：`sc query VirtualGPSBridge`
2. 手动启动服务：`sc start VirtualGPSBridge`
3. 查看事件查看器中的错误日志
4. 确认服务可执行文件存在且有执行权限

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
