# Dolby COM APO 进展 · 2026 年 11 月 24 日

[English](dolby-apo-progress.md) · [Русский](dolby-apo-progress.ru.md) · [简体中文](dolby-apo-progress.zh-CN.md)

**2026-11-25 — 有界 Dolby 运行程序与实机安静验证**

管理程序等待处理程序就绪，拒绝重复占用的 bank，并仅通过 Job Object 管理自己的进程。有界计数替代完整音频／包日志，文件共享错误与 DSP 处理隔离。10 项原生生命周期／故障验证通过；普通和不规则输出各 196,608 帧仍精确匹配原版，重置对比也通过。

新管理程序和 Android 原版处理程序完成实际安静 .13/.36 验证。最后一个普通流记录 72,096 湿处理帧、零干信号回退帧；4 项 Normal/RAW L/R 测试通过，设备音量未改变。两个处理程序正常退出，自己的映射已释放，.18 / APO .4 健康恢复。该实验配置仍使用 128 ms 缓冲和外部后端依赖；服务、恢复、MiSound、声学 L/R、PCM24 与最大音量安全尚未完成。

[Source / Исходники / 源码](../driver/apo/worker-native/runtime/) · [Evidence / Данные / 数据](results/audio-runtime-broker-2026-11-25.json)

<details>
<summary>此前研究及验证</summary>

**2026-11-25 — 修复 APO 聚合；实际 Dolby 处理已运行**

.12 对照在两个 audiodg 进程的 11 次聚合创建中返回 `0x80040110`。.13 使用私有 IUnknown 管理内部对象，公开接口将身份和引用计数委托给控制对象。这修复了实际系统激活故障；非聚合创建仍正常。

| 项目 | 已确认 | 待完成 |
| --- | --- | --- |
| Dolby SFX | ✅ .13 在实际 audiodg 中创建、初始化及处理，安静非零输入已测试。 | ❌ 常驻处理包、持续时序和延迟。 |
| 数字 L/R | ✅ 四项 Normal/RAW 测试，设备音量未改变。 | ❌ 四颗物理扬声器的隔离。 |
| MiSound | ✅ 原厂数据、配置及命令源码精确验证；Ghidra 已分析启用路径。 | ❌ Qualcomm DSP 图及资源集成。 |
| PCM24／保护 | 保留此前软件验证。 | ❌ 硬件 PCM24、HyperOS 声学及最大音量安全。 |

测试使用临时运行的 Android 原版 Dolby 处理程序和 128 ms 缓冲。四项极安静的 800 Hz Normal/RAW L/R 验证通过；另一通道的相干音调 ≤0.955%，PCM16 抖动保留在结果中。这证明数字分离及有界实机 SFX 处理；物理扬声器隔离、音质改善与最大音量安全尚未证明。

平板当前已恢复四颗功放 .18 / APO .4；未重启，也未改变校准、增益或保护。.13/.36 签名包、源码及回滚保存在 SSD，供后续测试。公开 .16 ZIP 未改变，不含系统 Dolby/MiSound。

[Evidence / Данные / 数据](results/audio-apo-aggregation-2026-11-25.json)



> [最新系统集成结果与改动前后对比](audio-system-progress.zh-CN.md).

**已验证：**尚未注册的原生 ARM64 COM APO 通过独立进程接收 Android 原版 Dolby 的 Float32 输出。固定大小和不规则大小回调测试中，各有 196,608 个双声道帧与参考结果逐字节一致。**系统级 Dolby 和 MiSound 仍未完成。**

原版 `libswdap.so` 在隔离的 ARM64 环境中运行，使用与 HyperOS ACDB 核对过的 Dynamic / speaker_landscape 参数。它仍是模拟执行的 Android 代码，不是原生 Windows Dolby 库。没有提高均衡器、功放增益或保护阈值。

## 本次改进

- 默认 COM 工厂此前只创建直通实例，现在可在初始化时读取实验传输配置并连接独立的请求和回复队列。
- 任意大小的回调会累积为原版处理器使用的 1,024 帧批次。原型使用固定 128 毫秒传输延迟。
- 工作进程延迟或停止时，输出保持相同帧位置的原始信号。注入 120 毫秒暂停后，Dolby 处理成功恢复。
- 共享内存使用新的代次，避免重置或重新打开 COM 后使用旧回复。恢复精确的 Android 状态，使实测重建时间从约 260 毫秒降至 2–3 毫秒。
- 原生对数和四舍六入五成双的舍入实现保持参考输出不变。339 项数学 ABI 检查和两次完整的 196,608 帧比较均通过。

## 验证结果

七项数字场景均通过：固定回调、不规则回调、工作进程退出、临时暂停、状态重置、无配置时的零延迟直通，以及已配置但无工作进程时的原始信号输出。另一项测试在同一工作进程存活期间两次打开 APO，代次从 1 变为 2，每次均匹配 196,608 个参考帧，最后一个所有者关闭后映射被释放。测试未打开音频设备，也未安装驱动。

## 待完成

- ❌ 在实际 Windows 音频图中激活并处理音频。标准属性存储初始化、工作进程代理与组件安装已通过各自的独立检查；[最新系统集成结果](audio-system-progress.zh-CN.md)将这些结果与实际系统处理区分开。
- ❌ MiSound 集成及 Qualcomm DSP 模块加载问题。
- ⏳ 负载下的长期播放、功放遥测、与 Android 的声学比较及最大音量安全性。此前关机的原因仍未确定。

Windows 支持[复合音效注册](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/pkey-compositefx-endpointeffectclsid)，但在现有 Qualcomm 代理旁插入音效前，需要检查[流处理支持及驱动中的混音位置](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/implementing-audio-processing-objects)。

[测量摘要](results/dolby-apo-2026-11-24.json) · [PCM24 进展](pcm24-progress.zh-CN.md) · [主页](README.zh-CN.md)

</details>
