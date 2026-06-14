# WPPtools — AI 代理指南

> **重要：** 本仓库所有交互必须使用**中文**。AI 的回复、解释、代码注释、更新文档等均须使用中文。

## 项目简介

Windows 独占的 C++17/Qt6 应用集，为 WPS 演示幻灯片放映提供悬浮屏幕控件及其他工具。由 TTcore 守护进程统一管理各组件，通过本地命名管道通信。

## 四个可执行文件

| 目标名 | 源码目录 | 入口点 | 类型 |
|--------|---------|--------|------|
| `TTcore` | `core/` | `core/main.cpp` | 核心守护进程（无窗口），管理子进程 + IPC |
| `WPPTouchHelper` | `wpp/` | `wpp/main.cpp` | 幻灯片控件（无窗口） |
| `USBAssistant` | `wpp/` | `wpp/usb_main.cpp` | U盘检测弹出窗（无窗口） |
| `TouchtoolsPanel` | `panel/` | `panel/PanelMain.cpp` | 配置 GUI |

## 构建

```bat
build.bat
```

或手动——工具链路径硬编码在 `build.bat` 中（MSVC 14.51, WinSDK 10.0.26100.0, Qt 6.11.1 msvc2022_64）。需在 **VS 2022 开发者命令提示符** 中运行，或自行调整路径。

构建完成后自动对四个 exe 执行 `windeployqt`，产物输出到 `build/`。

### 安装包发布流程
```
build.bat                      # 编译 + windeployqt → build/
xcopy build\* dist\ /s /e /i   # 复制必要文件到 dist/
Inno Setup 编译 install\*.iss  # 生成安装包到 install/
```

### 构建注意事项
- `CMAKE_AUTOMOC ON` — 不要手动生成 MOC 文件
- 所有 `CMakeLists.txt` 均设置 `WIN32_EXECUTABLE TRUE` → 无控制台窗口（`LOG()` 调试输出到 stderr，仅调试器可见）
- `CMAKE_PREFIX_PATH` 必须指向 Qt 6 msvc2022_64
- `build.bat` 已内置 cmake 路径和旧缓存自动检测清理

## 架构

```
WPPtools/
├── wpp/                     # WPPTouchHelper + USBAssistant
│   ├── main.cpp             — QSharedMemory 单实例，QLocalServer("WPStoolsPipe") IPC
│   ├── usb_main.cpp         — USBAssistant 独立入口
│   ├── USBAssistant.h/.cpp  — WM_DEVICECHANGE USB 检测 + 右下角弹窗
│   ├── Config.h/.cpp        — 单例，读写 <exe-dir>/touchtools.json
│   ├── FloatingControl.h/.cpp   — 两侧翻页按钮，每 500ms 轮询
│   ├── FloatingToolbar.h/.cpp   — 底部工具栏，每 500ms 轮询
│   └── KeySimulator.h/.cpp  — SendInput + 幻灯片窗口检测
├── panel/                   # TouchtoolsPanel 配置 GUI
│   ├── PanelMain.cpp        — 入口，创建 ConfigPanel
│   ├── ConfigPanel.h/.cpp   — 无边框主框架，标题栏 + 左侧导航 + QStackedWidget
│   └── WppConfigPage.h/.cpp — WPPTouchHelper 配置页（滑块、IPC 控制）
├── core/                    # TTcore 守护进程
│   └── main.cpp             — 单实例 + IPC("TTCorePipe") + QProcess 子进程 + QFileSystemWatcher
├── ico/                     — PNG 图标素材
└── dist/                    — 打包输出目录
```

## IPC 体系

| 管道名 | 通信方 | 用途 |
|--------|--------|------|
| `TTCorePipe` | 面板 ↔ TTcore | 状态查询、日志拉取、起停组件、重载配置 |
| `WPStoolsPipe` | 面板 ↔ WPPTouchHelper | 遗留：show/exit/status 命令 |

- **TTcore 命令：** `status` / `log` / `reload` / `exit`
- **TTcore 响应：** `"active=yes"` / `"active=no"` / JSON 日志数组
- **WPPTouchHelper 命令：** `"show\n"` / `"exit\n"` / `"status\n"`
- **流程：** 面板保存 JSON → 发送 `"reload"` → TTcore 重启或通知子进程

## 配置

- JSON 文件位于 `<exe-dir>/touchtools.json`
- 惰性写入——加载时若某键缺失，则设置默认值并重写整个文件
- TTcore 通过 `QFileSystemWatcher` 监听文件变更，自动重启子进程

## 幻灯片检测

- `KeySimulator.cpp` 通过 `CreateToolhelp32Snapshot` 轮询 `wpp.exe` PID，然后通过 `EnumWindows` 匹配窗口标题子串 `"Slide Show"` 或 `"幻灯片放映"`
- 控件通过 QTimer **每 500ms** 轮询
- 检测到幻灯片 → 显示并置顶窗口；未检测到 → 隐藏（IPC 触发时有 8 秒保活期）

## 按键模拟

- `sendKey()` 使用 `AttachThreadInput` + `SetForegroundWindow` + `BringWindowToTop` + `SendInput`
- 多级退出：Esc → Alt+D+Esc（×2）→ `TerminateProcess(wpp.exe)`，**异步 QTimer 状态机**（不阻塞主线程）

| 按钮 | 发送的按键 |
|------|-----------|
| 上一页/下一页 | `VK_LEFT` / `VK_RIGHT` |
| 鼠标模式 | `Ctrl+A` |
| 笔模式 | `Ctrl+P` |
| 橡皮擦 | `Ctrl+E` |
| 退出放映 | Esc（多级兜底） |

## 开机自启

- **仅 TTcore** 通过计划任务启动（任务名 `ttcore`，`-autostart` 参数）
- 面板设置通过 **PowerShell `Register-ScheduledTask`**（管理员提权），`-AtLogOn` 触发器，所有用户登录时运行
- 旧任务名 `touchtools`（启用 TTcore 前使用，现即将弃用）
- 开关操作通过 `ShellExecuteW(runas)` 弹 UAC 提权

## U盘助手

- `USBAssistant` 独立 exe，通过 `WM_DEVICECHANGE` 监听 USB 插入
- 发现新 U 盘 → 右下角置顶弹窗，显示设备名称/盘符 + 绿色"打开U盘"按钮
- 弹窗默认 5 秒自动消失
- 由 TTcore 根据配置 `usb_enabled` 起停

## 图标加载的奇怪之处
- `resources.qrc` 已删除，未使用
- `FloatingToolbar::loadIcon()` 从磁盘加载 `ico/*.png`，手动处理像素：白色像素→透明，其他所有像素→纯白色（为半透明叠加层染色）

## 安装程序

- `setup.iss`（根目录）— 当前使用，安装到 `%localappdata%\WPPtools`，含所有四个 exe、DLL、插件
- `install/*.iss` — 旧的 Touchtools 独立包，待清理
- Inno Setup 需要 `dist/` 目录由构建产物填充（exe + DLL + platforms/ + styles/ 等）

## 平台

- 仅 Windows，无可移植性层
- 使用 `windows.h`、`shellapi.h`、`tlhelp32.h` — 全程 Win32 API
- 四个可执行文件均为 x64 架构
