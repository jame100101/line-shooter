# 资源许可证清单

> 规则：只使用 CC0 或明确可商用资源。每条记录必须含 名称 / 作者 / 来源链接 / 许可证 / 下载日期。来源不明的资源禁止使用。

## 代码依赖

| 名称 | 作者 | 链接 | 许可证 | 用途 |
|---|---|---|---|---|
| OpenCV | OpenCV team | https://opencv.org | Apache-2.0 | 窗口/framebuffer/输入 |
| Qt6 (via OpenCV highgui) | The Qt Company | https://www.qt.io | LGPL-3.0 | OpenCV highgui 的 GUI 后端(MSYS2 构建依赖，动态链接) |
| MinGW-w64 runtime (libstdc++/libgcc/winpthread) | GCC/MinGW-w64 | https://www.mingw-w64.org | GCC Runtime Library Exception / MIT-like | C++ 运行时 |

> 说明：MSYS2 预编译的 OpenCV `highgui` 以 Qt6 为 GUI 后端，故运行需 Qt6 DLL。Qt6 以
> LGPL-3.0 动态链接方式使用，符合可商用要求（如需分发，遵守 LGPL 动态链接条款即可）。
> nlohmann/json 目前未接入（MVP 参数集中在 Config.hpp，未使用 JSON），后续接入时在此登记。

## 美术资源

_MVP 阶段使用程序化占位美术，无外部美术资源。确认 CC0 资源后在此登记后再接入。_

| 名称 | 作者 | 链接 | 许可证 | 下载日期 | 用途 |
|---|---|---|---|---|---|
| (待接入) | | | | | |

## 音效资源

_MVP 阶段使用接口占位 (空实现/系统 Beep)，无外部音效。_

| 名称 | 作者 | 链接 | 许可证 | 下载日期 | 用途 |
|---|---|---|---|---|---|
| (待接入) | | | | | |
