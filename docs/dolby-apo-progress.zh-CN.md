# Dolby COM APO 进展 · 2026 年 11 月 24 日

[English](dolby-apo-progress.md) · [Русский](dolby-apo-progress.ru.md) · [简体中文](dolby-apo-progress.zh-CN.md)

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

- ❌ Windows 实际音频图初始化、工作进程管理、组件注册和处理模式验证。实验初始化数据不是 Windows 系统属性存储初始化数据。
- ❌ MiSound 集成及 Qualcomm DSP 模块加载问题。
- ⏳ 负载下的长期播放、功放遥测、与 Android 的声学比较及最大音量安全性。此前关机的原因仍未确定。

Windows 支持[复合音效注册](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/pkey-compositefx-endpointeffectclsid)，但在现有 Qualcomm 代理旁插入音效前，需要检查[流处理支持及驱动中的混音位置](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/implementing-audio-processing-objects)。

[测量摘要](results/dolby-apo-2026-11-24.json) · [PCM24 进展](pcm24-progress.zh-CN.md) · [主页](README.zh-CN.md)
