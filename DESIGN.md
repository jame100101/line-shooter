# FPS 原型技术方案 (OpenCV + C++20 软渲染)

> 状态：设计已定稿，等待开工 Phase 0。
> 确认结论：OpenCV 用 **vcpkg** 集成；转向默认 **鼠标锁定**(键盘转向也实现，config 可切)；MVP 美术 **先程序化占位**，后续替换 CC0 资源。

## 0. OpenCV 能力边界
OpenCV 只做三件事：
1. 开窗口 + 把 `cv::Mat` framebuffer blit 到屏幕 (`imshow`)。
2. 保持窗口消息循环 (`waitKey(1)`)。
3. 便利地画 2D 叠加 (准星/文字/血条)。

**它不做**：透视 3D、深度缓冲、纹理采样、光照 —— 这些是我们自写的 software renderer。

### 精细模型的诚实结论
纯 OpenCV/CPU 实时光栅化精细三角网格做不到 60FPS，也不硬做。MVP 采用 boomer-shooter 方案：
- 世界几何：2.5D raycasting (DDA 网格墙 + 地板/天花板)。
- 敌人：billboard sprite (可用 3D 模型离线渲染成 sprite，观感精细、运行轻)。
- 枪械：屏幕空间 sprite (后坐力/开火帧)。
- "精细" 靠高质量美术资源，而非实时多边形。
- 后续风险项 (非 MVP)：极小软件三角光栅器，仅给枪械/敌人做 low-poly mesh overlay + z-test，先做技术验证再决定。

## 1. 目录结构
见仓库 `src/` 分层：core / platform / render / game / ui / assets。

## 2. 构建：CMake + OpenCV (vcpkg)
- C++20 (可回退 C++17)。
- 依赖：OpenCV 4.x (core/highgui/imgproc)，nlohmann/json (MIT, vendored)，链接 user32。
- 命令：
  ```
  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
  cmake --build build --config Release
  ctest --test-dir build
  ```

## 3. 模块职责
GameLoop(固定步长) / Renderer(编排) / Input(Win32 轮询) / Player+Movement / Weapon / Enemy / Combat / Effects / Hud / AssetManager。

## 4. 输入 (Windows 关键决策)
`cv::waitKey` 不报按键释放、难处理多键同按 —— 不适合 FPS。改用 Win32：
- 移动/动作键：`GetAsyncKeyState` 每帧轮询真实状态 (支持 W+A+Shift+Space 同按)。
- 转向：默认鼠标锁定 (`GetCursorPos`+`SetCursorPos` 模拟 pointer-lock)；键盘 ←/→ 或 Q/E 也实现，config 切换。
- `cv::waitKey(1)` 仅用于窗口存活 + blit。
- 键位 (config 可改)：WASD 移动 / Shift 冲刺 / Space 跳 / Ctrl 滑铲 / 左键(或 J) 射击 / R 换弹 / Esc 退出。

## 5. 渲染
- Framebuffer：一张 cv::Mat (BGR, 内部 960x540, 可缩放到窗口)。
- 世界：raycaster 逐列 DDA 命中墙，按距算墙高贴纹理列；地/顶 MVP 先纯色/渐变保帧率。
- 深度：每列墙距存 zbuffer[column]，sprite 逐列比较遮挡。
- 敌人 sprite：billboard 投影 + 缩放 + 逐列深度测试。
- 枪械：屏幕空间 sprite + 后坐力位移。
- 性能：960x540 CPU 可 60+FPS，列循环后续可并行。

## 6. 机动性 (参数进 config)
地面加速度+摩擦 / 冲刺(提速+FOV 反馈) / 跳跃(冲量+重力+状态机) / 空中控制(保留动量+有限转向) / 滑铲(降碰撞高+摩擦骤降+压相机) / 轻量 bunny-hop(落地续跳保速, 设上限)。

## 7. 枪械与打击感
hitscan (Projectile 预留) / 后坐力脉冲回正+扩散 / 屏幕震动(trauma 衰减) / 枪口火光 / 命中停顿 hitstop / 命中标记 X / 敌人受击闪白(addWeighted) / 击杀文字上浮淡出 / 音效 IAudio 接口(MVP 空实现或 Beep)。

## 8. UI (简洁 boomer 风)
中心准星(命中变色/弹X) / 左下 HP / 右下 Ammo / 击杀提示 / 连杀提示(DOUBLE/TRIPLE 计时窗口)。framebuffer 直绘 + putText，无花哨面板。

## 9. 敌人 AI (MVP 简单版)
状态机 Idle→Chase→Strafe→Attack→Dead：追踪(避让)/绕行/攻击(冷却伤害)/死亡(闪白+碎片+结算)。后续：视线/A* 寻路/多类型。

## 10. 资源 (许可证强制)
优先 Kenney(CC0)/Quaternius(CC0)/OpenGameArt(逐条确认)。只用 CC0/明确可商用；每个资源在 assets/licenses.md 记录 名称/作者/链接/许可证/日期。来源不明不用。MVP 先程序化占位，确认许可后替换。

## 11. MVP 范围
必做：稳定 60FPS 窗口 / raycaster 场景(地墙障) / 移动+转向+冲刺+跳+滑铲 / 一把枪 hitscan / 命中反馈(准星+闪白+震动) / 击杀反馈(提示+hitstop+粒子) / HUD(准星/HP/Ammo/Kill)。
后续：Projectile / A* 寻路+多敌人 / 真实音频 / 精美 sprite / 鼠标锁定打磨 / mesh overlay / 多武器 / 关卡外置。

## 12. 分阶段任务 (每阶段可编译运行 + 跑构建)
- Phase 0 骨架：CMake+OpenCV 开窗、GameLoop 固定步长、Framebuffer 清屏、Config 加载、帧率显示 → 稳定 60FPS 空窗口。
- Phase 1a 世界：网格地图 + raycaster 墙 + 移动/转向 + 碰撞 → 场景走动。
- Phase 1b 机动：加速度/冲刺/跳/滑铲/bhop + 相机反馈 → 手感成型。
- Phase 1c 枪械：枪 sprite + hitscan + 后坐力 + 火光 + HUD 准星/Ammo → 能开枪。
- Phase 1d 敌人+战斗：billboard 敌人 + AI + 闪白/震动/hitstop + 击杀提示/粒子 + Kill count → MVP 完成。
每阶段末：cmake --build + 运行 + ctest。
