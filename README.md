# Line Shooter

黑白线条风格的 2.5D / raycaster FPS 原型。项目使用 C++20 编写，OpenCV highgui 只负责窗口、帧缓冲显示和 2D UI 绘制；第一人称场景由项目内的 CPU raycaster、billboard 敌人和 hitscan 射线系统渲染。

## 当前特性

- 黑白线条场景：白色露天环境、地面网格、长方体障碍物和细线棱边。
- 2.5D raycaster 渲染：低矮障碍物不会完全截断视线，上方可看到后方物体。
- 红色实体射线：从开火位置指向命中点，短时间残留，可命中敌人与环境。
- 敌人生成：最多保持 5 个敌人，避免直接在玩家身边刷新。
- 菜单与设置：主菜单、设置界面、分辨率、全屏、帧率限制、鼠标灵敏度、按键绑定、音量占位配置与保存。
- Game Over 弹窗：支持 retry 和 return to menu，并记录最高击杀数。
- 像素风红色 HUD：HP、弹药、击杀、FPS 等 UI。
- 受伤反馈：受击闪烁与 Beep 音效。

## 技术说明

这不是完整三维 mesh renderer，而是 **2.5D raycaster**：

- 墙体/障碍物：基于网格地图的 DDA raycast。
- 地面：透视 floor casting。
- 敌人：billboard sprite 方式绘制，并通过 z-buffer / 低墙遮挡缓冲处理遮挡。
- UI 与射线：OpenCV 2D 绘制叠加。

## 构建环境

Windows + MSYS2 mingw64：

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja mingw-w64-x86_64-pkgconf \
  mingw-w64-x86_64-opencv mingw-w64-x86_64-qt6-base
```

## 构建

```bat
build.bat
```

输出：

```text
build\fps_mvp.exe
```

## 运行

```bat
run.bat
```

## 打包

```bat
package.bat
```

输出：

```text
dist\line-shooter-windows.zip
```

压缩包包含 `fps_mvp.exe`、运行所需的 MinGW/OpenCV/Qt DLL、`platforms\qwindows.dll`、默认设置文件和 `run_portable.bat`。

## 操作

| 动作 | 默认按键 |
|---|---|
| 移动 | W A S D |
| 视角 | 鼠标 |
| 冲刺 | Shift |
| 开火 | 鼠标左键 / J |
| 换弹 | R |
| 菜单/退出 | Esc |

按键可在 setting 页面修改并保存。

## 自动截图/测试

```bat
set FPS_AUTOFRAMES=120
run.bat
```

程序会自动运行指定帧数，生成：

```text
build\shot.png
```

## 项目结构

```text
src/core       Game loop、配置、数学工具
src/platform   OpenCV 窗口、Win32 输入
src/render     Framebuffer、Raycaster、Renderer
src/game       World、Player、Movement、Weapon、Enemy、Combat、Effects
src/ui         HUD / 菜单 UI
src/audio      BeepAudio / IAudio
src/assets     程序化纹理
tools          打包脚本
```
