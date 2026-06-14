# WPPtools — WPS PPT 触屏辅助工具集

在触屏设备上使用 WPS 演示放映幻灯片时，提供悬浮翻页控件、工具栏及 U 盘助手。由 TTcore 守护进程统一管理各组件。

## 功能

- **悬浮翻页按钮** — 屏幕两侧半透明按钮，上下翻页（模拟键盘方向键）
- **底部工具栏** — 切换鼠标/笔/橡皮擦模式（Ctrl+A / Ctrl+P / Ctrl+E），一键退出放映
- **U 盘助手** — 检测 USB 插入，右下角弹窗快速打开
- **配置面板** — 无边框暗色 UI，调节透明度、尺寸、边缘距离，管理进程
- **TTcore 守护进程** — 集中管理子进程启停、配置热重载、运行日志
- **开机自启** — 通过 Task Scheduler 启动 TTcore，子进程由 TTcore 按配置启动
- **自动检测** — 检测到 WPS 幻灯片放映时自动显示控件，退出放映后自动隐藏
- **多级退出** — Esc → Alt+D+Esc → TerminateProcess 兜底，避免墨迹弹窗卡住

## 项目结构

```
WPPtools/
├── wpp/                  # WPPTouchHelper + USBAssistant
├── panel/                # TouchtoolsPanel（配置 GUI）
├── core/                 # TTcore（守护进程）
├── ico/                  # PNG 图标素材
└── dist/                 # 打包输出目录
```

## 构建

### 环境要求

- Windows 10/11 x64
- Visual Studio 2022 BuildTools（MSVC 14.51+）
- Qt 6.x（msvc2022_64）
- CMake 3.16+

### 构建步骤

```bat
build.bat
```

产物在 `build/` 下：`WPPTouchHelper.exe` + `TouchtoolsPanel.exe` + `USBAssistant.exe` + `TTcore.exe`

## 使用

1. 运行 `TTcore.exe`（守护进程，托盘无图标）
2. 运行 `TouchtoolsPanel.exe` 打开配置面板管理组件
3. TTcore 按配置自动启动 WPPTouchHelper（幻灯片控件）和 USBAssistant（U盘助手）
4. 打开 WPS 演示并开始放映幻灯片，屏幕两侧出现翻页按钮，底部出现工具栏

### 按键映射

| 操作 | 快捷键 |
|------|--------|
| 上一页 | ← |
| 下一页 | → |
| 鼠标模式 (Ctrl+A) | 工具栏按钮 |
| 笔模式 (Ctrl+P) | 工具栏按钮 |
| 橡皮擦 (Ctrl+E) | 工具栏按钮 |
| 退出放映 | Esc（多级重试） |

## 开源许可

版权所有 (C) 2026 Wayne

本程序是自由软件：您可以依据自由软件基金会发布的 GNU 通用公共许可证（**GPLv3 或任何更高版本**）重新分发或修改它。

本程序分发时希望它有用，但**不提供任何担保**；甚至没有适销性或特定用途适用性的默示担保。详见 [LICENSE](LICENSE)。

## Qt 框架

本程序使用 Qt 框架。Qt 是 The Qt Company 的商标。
从以下地址获取 Qt 源码：<https://download.qt.io/>
