# Q2RTX 安卓 RTX Demo 移植计划

## Summary
- 目标定为：`Android 14 + arm64-v8a + 支持 Vulkan RT 的 Snapdragon 旗舰机（Adreno 高端 GPU）`，先做单机型优先的可玩 Demo。
- 第一阶段成功标准：APK 可安装；能导入 `baseq2`/`q2rtx` 资源；能进入地图并完成移动、视角、开火、交互、音频播放；保留 `VKPT` 路径追踪。
- 方案默认不做“广泛兼容”。如果设备缺少 `VK_KHR_ray_tracing_pipeline`/`VK_KHR_ray_query`、`VK_KHR_acceleration_structure`、`bufferDeviceAddress`、`descriptorIndexing` 等能力，应用直接给出明确“不支持此设备”的提示并退出。

## Key Changes
- 构建与包装
  - 新增独立安卓工程，基于仓库内置的 `SDL2/android-project` 模板，改为 `Gradle + CMake` 流程。
  - 原生客户端从桌面 `add_executable(client)` 改为安卓专用 `add_library(main SHARED ...)`，仅构建 `arm64-v8a`。
  - 安卓版只构建 `client`，不包含专用 `server` 目标；保留 `game` 逻辑库并改为安卓可加载的 `so` 输出。
  - 安卓构建默认关闭桌面无关能力：多 GPU、窗口模式/窗口几何、桌面图标/显示器枚举、Linux 包管理安装逻辑。

- 平台层
  - 新增 `src/android` 平台分层，替代当前 `src/unix/system.c` 里的 `HOME/XDG/getuid/dlopen` 假设。
  - 文件系统改用 `SDL_AndroidGetInternalStoragePath`/外部应用目录，资源导入后统一挂到 `baseq2` 搜索路径。
  - 处理 Android 生命周期：启动、Pause/Resume、Surface 丢失重建、后台前台切换、日志输出到 `logcat`。
  - 复用现有 SDL 视频/输入代码作为基础，但将桌面专属分支用 `#ifndef __ANDROID__` 剥离；Android 分支固定横屏、全屏、高 DPI。

- 渲染与性能
  - 安卓发布路径只保留 `VKPT`；不把 `ref_gl` 作为用户可选运行模式。
  - 启动时增加 Vulkan RT 能力探测与硬失败路径，检查实例/设备扩展、必需 features、shader bytecode 可加载性。
  - 默认启用移动端保守参数：30 FPS 上限、较低内部渲染分辨率、FSR 上采样、关闭高成本可选项作为初始配置；把这些做成少量安卓专用归档配置项。
  - 着色器编译继续沿用现有 CMake 流程，安卓包不内嵌大体积运行资源；`shaders.pkz` 仍按现有内容布局加载。

- 输入、音频、用户体验
  - 新增安卓触屏方案：左侧移动摇杆、右侧视角区、开火、使用、武器切换、暂停菜单；同时保留蓝牙手柄支持。
  - Android Back 键映射到菜单/返回，不直接杀进程；需要文本输入时走 SDL 的软键盘能力。
  - 音频继续基于 SDL/OpenAL Soft，先保证基础 SFX 和音乐播放，后续再做延迟优化。
  - 首版资源策略采用“用户自行导入”：APK 不分发原版 `pak*.pak`；`q2rtx_media.pkz`、`blue_noise.pkz`、`shaders.pkz` 也默认走外部导入，避免 APK 体积和资产压缩问题。

## Public Interfaces / User-Facing Additions
- 新增安卓启动器工程与应用包入口，原生库入口固定为 SDL Android 约定的 `main` 共享库。
- 新增安卓专用归档配置项，范围限定为：
  - 渲染缩放 / 帧率上限
  - 触控开关与触控布局
  - 可选陀螺仪视角
- 新增首次启动资源检查与导入流程；缺资源时进入导入/说明页，而不是直接在引擎内部报错退出。
- 新增“不支持此设备”的能力检查提示页，明确缺失的 Vulkan RT 能力。

## Test Plan
- 构建验证
  - `Debug APK` 可在目标手机安装、启动、输出原生日志。
  - `arm64-v8a` 原生库、SDL 启动链路、游戏逻辑库全部正确装载。
- 资源验证
  - 无资源时显示清晰导入提示。
  - 导入完成后能识别 `baseq2`、`q2rtx_media.pkz`、`blue_noise.pkz`、`shaders.pkz`。
- 设备能力验证
  - 支持 RT 的目标机：通过探测并进入主菜单/地图。
  - 不支持 RT 的设备：稳定给出明确不支持提示，不崩溃。
- 可玩性验证
  - 进入 `base1` 或等价首张地图，触控可完成移动、转向、开火、交互、菜单操作。
  - 音频正常；暂停/恢复后画面和声音恢复；连续游玩 10 分钟无崩溃。
- 性能验证
  - 以默认移动端配置在目标机上保持可玩帧率，优先以稳定性和温控为准，不追求桌面同等画质。

## Assumptions
- 只面向一类高端安卓旗舰假设：`Android 14`、`arm64-v8a`、`Vulkan 1.2+`、可用 RT 扩展。
- 首阶段是侧载 Demo，不做 Play 商店发布准备。
- 安卓版首发固定横屏，不做平板/折叠屏专项适配。
- 不做无 RT 的正式运行 fallback；缺能力直接拒绝运行。
- 不在 APK 中重新分发受版权限制的 Quake II 原版数据。
