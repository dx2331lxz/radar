# Qt6 + CMake + VS Code 示例（macOS）

一个最小的 Qt6 Widgets 应用，使用 CMake 构建，并附带 VS Code 任务与调试配置。

## 先决条件

- Xcode Command Line Tools（提供 clang/lldb）
- Homebrew（推荐）
- Qt6 与 Ninja、CMake

## 安装步骤（macOS, zsh）

```zsh
# 1) 安装命令行工具（若未安装）
xcode-select --install

# 2) 安装 Homebrew（若未安装）
# 参见 https://brew.sh/ 按提示安装

# 3) 安装 CMake / Ninja / Qt6
brew update
brew install cmake ninja qt

# 4) 暴露 Qt6 到 PATH（Homebrew 默认安装在 /opt/homebrew/opt/qt）
# 可选：为当前 shell 会话设置 PKG_CONFIG_PATH 与 CMAKE_PREFIX_PATH
export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt"
export PKG_CONFIG_PATH="/opt/homebrew/opt/qt/lib/pkgconfig:${PKG_CONFIG_PATH}"
```

> 如果你的 Mac 是基于 Intel（x86_64），Homebrew 前缀通常在 `/usr/local/opt/qt`，把上述路径换成 `/usr/local/opt/qt` 即可。

## 在 VS Code 中构建与运行

- 打开本文件夹。
- 推荐安装扩展：
  - CMake Tools（ms-vscode.cmake-tools）
  - C/C++（ms-vscode.cpptools）
- 使用内置任务：
  - 运行“Terminal -> Run Task… -> CMake: build”构建。
  - 运行“Terminal -> Run Task… -> Run app”启动程序。
- 或者按 F5，使用“(lldb) Launch radar”配置进行调试。

## 命令行构建（可选）

```zsh
cmake -S . -B build -G Ninja
cmake --build build -j
./build/radar
```

## 目录结构

```
.
├── CMakeLists.txt
├── src/
│   └── main.cpp
└── .vscode/
    ├── settings.json
    ├── tasks.json
    └── launch.json
```

## 新增功能
- 航迹报文解析（6.1 雷达航迹报文），小端字节序。
- 圆形雷达显示器（`RadarScopeWidget`）：实时显示目标位置与最近30秒航迹折线，方位以正北为0°顺时针。

## 运行
使用 VS Code 任务：
1. 运行 `CMake: configure`
2. 运行 `CMake: build`
3. 运行 `Run app`

应用在本机 UDP 6553 端口接收，默认向 127.0.0.1:6280 发送。将航迹报文通过 UDP 发至 6553 即可在界面中看到航迹绘制。

## 常见问题

- 找不到 Qt6（find_package 失败）
  - 确认 Qt 安装位置，设置 CMAKE_PREFIX_PATH 指向 Qt 根：
    - Apple Silicon: `/opt/homebrew/opt/qt`
    - Intel: `/usr/local/opt/qt`
  - 也可使用官方 Qt 在线安装器，安装后把其 `.../6.x.x/macos` 路径加入 CMAKE_PREFIX_PATH。

- 运行时报错找不到 Qt 库：
  - 本项目将 macOS bundle 关闭，默认可直接从 build 目录运行；
  - 若仍有问题，可尝试 `macdeployqt`（当使用 bundle 时）。

- 生成器选择：
  - 默认使用 Ninja（更快更安静）。如需 Xcode 工程：

```zsh
cmake -S . -B build-xcode -G Xcode
cmake --build build-xcode --config Debug
```
