# 音频进展 · 2026-11-25

[English](audio-system-progress.md) · [Русский](audio-system-progress.ru.md) · [简体中文](audio-system-progress.zh-CN.md)

**新增 · 已验证 Qualcomm 路由与参数封装：** 当前 Windows 校准文件为 Device 73 / AppType `0x11134`、`0x11138` 选择拓扑 `0x10000009`，七个模块中没有 MiSound 或 Dolby。新 C/H 检查器拒绝不匹配或歧义的文件路由，不自动替换 Android 标识。Windows 封装器与原版 QCADCM 机器码执行比较，**91 个原厂参数块**逐字节一致，并拒绝缓冲区越界与实例编号截断。

六组件 ARM64 EWDK 库通过 901 项路由案例、1,770 项路由内存案例和 49 项封装内存案例。此前 864 项 MiSound 比较与十个原厂滤波表仍通过。四项安静 Normal/RAW L/R 测试在原有 6% 音量下通过（另一声道相干音调 ≤0.0787%）。这是数字分离；系统音效、物理扬声器与最大音量安全仍未确认。软件 Dolby APO 的绑定需要单独检查 Windows 音频图。已安装 kernel .18 / APO .4；签名 .10/.36 已准备但未安装。

[验证数据](results/audio-acdb-pp-2026-11-25.json) · [C/H 源码](../driver/dsp/)

<details>
<summary>此前源码验证 · 11 月 25 日</summary>

**新增 · 原版 MiSound 扬声器命令计划：** C/H 模块复现 MiSoundDSP_SetSPKParam，不再只编码单个值。**864 项比较**全部匹配原版 ARM64 机器码执行：480 项支持案例命令／最终路由精确一致，384 项不支持案例保持输出不变。原版对 AppType 0x11130/0x11134 发出 2/4/10/12 条命令，持久采样率表为 44100/48000/88200/96000/192000。八字节数据及模式例外保留原始位和顺序。这是控制／校准路由，不证明硬件采样率或扬声器隔离。

四组件 ARM64 EWDK 库通过 53 项真实 NOACCESS 页边界案例及 13 项无效输入案例。十个原厂表与十四项拒绝测试仍通过，普通系统文件读取仍与 HyperOS 一致。Ghidra 导出了四个原版函数及原始指令。参考执行拦截了 Android 服务发送与日志，未发送 DSP 命令、播放声音或安装驱动。补齐了命令计划源码；真实系统 MiSound/Dolby、物理 L/R 与最大音量安全仍未完成。

[验证数据](results/audio-misound-speaker-plan-2026-11-25.json) · [MiSound C/H 源码](../driver/dsp/)

**此前 · APO .10／Windows 11 设置 .36：** 显式选择的 Dolby 连接无效、不可用或被占用时，不再切换到公共后备。.9 对照在 28 项原生测试中失败 14 项，.10 全部通过。仅固定 Global 查找被重定向到真实 Local 测试映射。这是可复现的软件修复，不证明系统音效或此前关机的原因。

.36 扩展为 APOInitSystemEffects3 使用的确切上下文声明 Default 存储，而不将自定义参数放在旧平面存储中。原生 SetupAPI 解析通过，五项损坏配置被拒绝，包括覆盖 OEM MFX。此 INF 不更改现有 OEM MFX/EFX 值。.10/.36 签名目录及成员验证通过，只读预检通过。五项内存、七项日志、两项计数、七项原版工作进程测试再次通过，普通／不规则输出精确匹配 196,608 帧。十五项模拟上下文清理测试通过；真实安装、上下文读取和回滚仍未测试。

Ghidra 分析了原版 AMP0/LMT0 每声道构造函数。所比较代码已存在于 Windows 固件中，并不证明功放保护正在运行。未安装新驱动或启动声音。系统 Dolby/MiSound、物理 L/R 与最大音量安全仍未完成。

[验证数据](results/audio-apo-policy-settings-2026-11-25.json) · [APO 源码](../driver/apo/) · [Windows 11 设置框架](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/windows-11-apis-for-audio-processing-objects)

**此前 · MiSound 资源读取器 v2：** ARM64 EWDK 库在输出原始参数前验证全部配置／模块。两个原厂资源保留 **502 条精确记录**（各 7 个配置、19 个模块）。13 项损坏案例未触发回调；10 项损坏尾部比较现在拒绝旧滤波读取器接受的文件。10 个原版滤波表与 14 项拒绝测试仍通过。使用真实 NOACCESS 保护页的原生测试通过 **30,770 项截断／变异案例**。结构有效的变异并不代表原厂认可的校准。

四声道文件在 NORM/MOVI/GAME/SYSS 中增加 8 条 MMS0 记录，**每个配置增加 32 字节**。保留选择器与数值位，不猜测参数单位或扬声器物理位置。Ghidra 追踪了原版独立滤波连接路径，但这些索引本身不证明声学 L/R。普通系统文件读取与 HyperOS 一致；DSP 实际打开资源仍未观察到。

另分析了十个原版函数。原始指令确认 **176400 → 192000 Hz 原厂表选择**，读取器现已保留。两个别名案例逐位匹配全部滤波数据，十四项邻近／无效采样率仍被拒绝。这不重采样音频，也不证明这些采样率的硬件播放。

原版 Qualcomm 驱动在 DSDT 中搜索 **SOSI**，该记录位于搜索窗口内。4 项隔离原码缓存测试显示首次查询失败可被缓存；SOC 来源及操作系统调用均被模拟。这是可测试的故障条件，不是已证明的实时超时原因，也不能据此修改内核。APO .9 已准备但未安装；本次未改变已安装驱动、增益或保护设置。

[验证数据](results/audio-misound-resource-2026-11-25.json) · [MiSound 源码](../driver/dsp/) · [Xiaomi SOC 结构](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/blob/71b6effd2dfa8f703dd1ea17acd1323b2fd3cf6c/drivers/soc/qcom/socinfo.c)

**此前 · APO 2.0.0.9-lab／MiSound 支持：** APO 状态使用独立 VirtualAlloc 分配，页锁定不再依赖堆对象边界。三个未注册实例的原生检查确认独立区域、中间区域被释放、其余实例仍可用。.8 对照正常使用堆，保留堆页不被称为泄漏或崩溃。五项内存故障、七项日志、两项计数及七项原版工作进程测试再次通过；普通／不规则流仍精确匹配 196,608 帧。签名 .9 与只读预检通过，候选尚未安装。

新增 ARM64 MiSound 支持库读取原厂 SPMD/MMFD 表。十个表的 binary32 系数与比例逐位一致，十四个损坏／不支持案例不改变输出。链接后的 EWDK 库也通过这些比较。两个系统资源经普通 Windows 文件读取，与 HyperOS 一致。SPMD 部分相同；不同的 OAL、动态及多声道处理尚未实现。Ghidra 进一步分析四个原版 HPMD/SPMD/HRIR 解析及指针函数。这不证明 DSP 实际打开资源或系统 MiSound 已运行。不宣称音质提升、声学 L/R 或最大音量安全。

[源码与数据](results/audio-isolation-filters-2026-11-25.json) · [MiSound C/H 源码](../driver/dsp/) · [Microsoft 页解锁语义](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualunlock)

**此前 · APO 2.0.0.8-lab：** 处理和重置使用已缓存并锁定的映射指针，不再读取未锁定的视图。锁定／分配失败和 UnlockForProcess 会释放锁定页，成功重试清除旧错误。五项原生 COM 故障测试通过（.7 五项均失败）。分配与锁定 API 被替换，视图页被刻意保护；这是代码验证，不是实际平板崩溃。七项日志、两项计数器及七项原版工作进程测试通过；普通／不规则流仍精确匹配 196,608 帧。签名 .8 包通过目录、文件及只读预检，尚未安装。

在音量保持 6% 时再次通过四项安静数字 L/R 测试，另一声道 800 Hz 音调的最大比值为 0.000673。单独的极低音量预处理文件传输测试未通过信号匹配（相关系数 0.491），记录中右声道为零，数值仅有 PCM16 步进；此结果不作为 Dolby 已运行的证据。独立等峰值 L/R 对照测试通过。声学 L/R、系统音效及最大音量安全仍未验证。

Ghidra 进一步分析八个 HyperOS 资源、解析及多声道阶段函数，所比较代码已与 Windows 固件一致。加载器选择 OAL1/NORM/OAL2，读取器区分打开失败、文件名过长和短读取。根据原始 `mux` 指令纠正了反编译器的缓存返回值：实际为布尔值 0/1。此结果指导后续资源打开追踪，并不证明 MiSound 音效已运行。

[源码与验证数据](results/audio-apo-memory-2026-11-25.json)

不宣称音量提升或达到 HyperOS 音质。安静的数字回环不是扬声器声学测量。

**此前 · APO 2.0.0.7-lab：** 增加 Windows 11 源码指纹、映射名称及代次日志。重置/重新打开时清零旧帧计数，避免把前一个实例的结果误认为新处理。七项日志协议、两项原生计数测试及七项原版处理进程场景通过；常规/不规则渲染各精确匹配 196,608 帧。日志仅在 Initialize/LockForProcess 中调用，Reset/APOProcess 中没有日志。十七项模拟观察器测试拒绝错误构建、流、代次及测试程序 PID。这些是软件检查；签名 .7/.35 测试已准备，但候选尚未安装。

**MiSound 调查：** 已安装的 `qcadsprpcd` 内核驱动正在运行，ACPI 设备无错误。Ghidra 确认 `apps_std` 注册、`audiopd` 分支及资源路径处理。两个旧跟踪文件仅包含两条名为 `FastRPCDriverD0Entry` 的驱动消息；它们不证明监听进程已工作、资源已打开或 ADM 已回复。此前“没有监听进程”的假设不足以支持启动另一项服务。

[最新构建及证据](results/audio-apo-observability-2026-11-25.json) · [Microsoft 日志框架](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/windows-11-apis-for-audio-processing-objects) · [Reset 限制](https://learn.microsoft.com/en-us/windows/win32/api/audioenginebaseapo/nf-audioenginebaseapo-iaudioprocessingobject-reset)

</details>

| 项目 | 改动前 | 当前已验证结果 |
| --- | --- | --- |
| IRQ/GPIO 与路由 | IRQ 配置忽略 I²C 错误，仅检查 CH_BAL。 | ✅ .18 拒绝 IRQ 故障，四颗功放的实际回读通过。 |
| 数字 L/R | 未测量分离。 | ✅ 四项安静测试通过；另一声道的 800 Hz 音调低于 0.1%。UID/I²C 映射与 Android 一致。声学隔离尚未验证。 |
| Dolby APO | 显式连接不可用时可能使用公共后备，设置位于旧平面存储。 | ✅ .10 保留显式选择，.36 声明正确上下文；28 项软件测试通过。❌ 系统处理未验证。 |
| PCM24 | 格式与供数时序错误。 | ✅ 此前 13 项时序／重开测试通过。硬件 PCM24 尚未验证。 |
| MiSound／DSP 路由 | 缺少完整计划与 Windows 参数封装，未检查目标文件路由。 | ✅ 原版命令计划、ACDB 检查与 91 个精确原厂参数块。❌ 当前 Windows 文件链没有 MiSound/Dolby，实际处理未验证。 |

GPIO 中断及保护掩码配置现会传递错误，并在开启 DSP/功放前验证回读。成功路径写入、固件、系数和出厂校准不变。EWDK 构建、目录生成及签名通过。

真实内核事件验证了 UID 0–3 的路由及 IRQ 配置。四项 −60 dBFS 源信号测试通过，系统音量始终为 6%。原先总 RMS 条件在基线 .17 也会失败：如此安静的 PCM16 信号受到量化抖动限制。修正后的测量比较 800 Hz 测试音的相干分量，同时保留原始 RMS/峰值。这不测量扬声器声学音质。

修正后的 COMPUTER Dolby 扩展无需重启即可注册全局包。显式选择成功，但 audiodg 仍未产生 APO 回调及处理帧。临时扩展已移除，选择已恢复。小信号测试未触发过温/超行程计数；编解码器重启前观察到锁存 VmonLowFlag。这尚未解释此前大音量关机，也不证明最大音量安全。

最新安静复测在音量保持 6% 时通过四项普通／RAW L/R 测试。另一声道 800 Hz 音调的最大幅度比为 0.000910；RAW 总 RMS 仍包含 PCM16 量化抖动。原始 ACPI UID/I²C 描述与 Android DTS 独立确认全部四个板级位置。这验证连接映射，而非扬声器声学隔离。

新的独立 Dolby init3 测试精确匹配 196,608 帧（七种初始化情况，零错误／直通帧）。重新构建的 ARM64 MiSound 编码器再次与执行的 Android 原版机器码在 96 项测试中一致。这些测试不证明系统音效已运行。Ghidra 与只读端点属性确认公共全局包的 OEM 过滤条件。驱动绑定 SFX 扩展 .35 通过目录、签名及预检，尚未安装。两个资源及所比较的 MiSound DSP 代码区域与 HyperOS 一致；实际资源打开和参数确认仍未解决。

[最新数据](results/audio-effects-integration-2026-11-25.json)

**Dolby 2.0.0.5-lab：** 旧队列限制同时统计过期和有效回复。六条过期记录后，4,800 / 8,192 帧中仅 2,048 帧保留处理；候选版本保留全部 4,800 / 8,192 帧。六项比较及九项边界/安全测试通过，包括计数器回绕、恢复和拒绝 NaN/Inf。由[公开源码](../driver/apo/)新构建的 ARM64 DLL 通过七项原版处理进程场景，常规及不规则回调各精确匹配 196,608 帧。DLL 尚未签名、注册或安装，延迟仍为 128 ms。

**MiSound 文件路由：** 用 Ghidra 分析了六个 Windows FastRPC 函数。四项隔离的原版机器码测试复现默认 `%DriverData%\Qualcomm\fastRPC` 搜索和 `0x102020` 内核路径查询。所有 OS/CRT 调用、设备获取及 FILE 管理均被拦截，文件打开与内核响应使用模拟。当前扬声器资源映射和字节与 HyperOS 一致，但实际监听进程、资源打开及 ADM 确认仍未验证。该设备列表中没有 `adsp_avs_config.acdb` 映射，这只是观察结果，尚不能确定为故障原因。

.5 组件包也通过 Inf2Cat、目录签名及 INF/DLL 成员验证。候选包不再注册公共全局包，已保存 .4 的原始 INF/CAT/DLL 以供回滚。联合 .5/.35 脚本通过只读预检，并拒绝非管理员令牌，没有安装、启动处理进程、播放声音或打开 UAC。实际集成及回滚仍未测试。

[队列、构建及路由证据](results/audio-transport-2026-11-25.json) · [Qualcomm 守护进程架构](https://github.com/qualcomm/fastrpc/blob/development/Docs/daemons.md)

## 正在进行

- ❌ **系统级 Dolby：** 验证驱动绑定的 SFX 候选；公共全局包不适用于此 OEM 端点。
- ❌ **MiSound：** 修复原版 DSP 资源加载和参数应用确认。
- ⏳ 每颗扬声器的实际 L/R、PCM24 输出、HyperOS 音质及最大音量安全性仍未验证。

公开下载：**4.17.27.16**。开发源码：**4.17.27.18**。公开 ZIP 不包含系统级 Dolby 或 MiSound.

[验证数据 · 2026-11-25](results/audio-postboot-2026-11-25.json) · [之前的数据](results/audio-routing-apo-2026-11-25.json)

<details>
<summary>以前的验证记录（2026 年 11 月 24–25 日）</summary>

最终的标准初始化测试通过六种情况：无配置、无代理、仅发现音效、RAW、正常处理与代理槽已占用。正常处理的 **196,608 个双声道帧**与参考结果逐字节一致：**529 次回调，零异常帧、零故障直通帧**。发现模式、RAW 与槽占用时使用零延迟直通。测试未注册 APO，也未打开音频设备。

Windows 接受了两个签名包并创建正常的 APO 设备。重新启动音频宿主后，常规播放仍然产生**零 APO 回调和零处理帧**。候选组件已移除，工作基线已恢复，Windows 没有重启。这缩小了集成问题的范围，但尚未解决它。

`libmisoundfx.so!MiSound_process`（ELF 地址 `0xb168`）的四项原版机器码检查通过。有效的独立缓冲区保持输出不变；原地缓冲区的采样也没有变化。无效参数返回 `-22`。这个 68 字节回调只验证缓冲区，因此扬声器 MiSound 处理必须通过 DSP 路径接入。

原生 ARM64 MiSound 命令编码器已与**执行 Android 原版机器码的 96 项离线测试**完全匹配。参数 ID、模块／拓扑变化及不带 `=` 填充的 Base64 输出均一致；不支持的类型保持输出不变。[C 源码与头文件](../driver/dsp/)不进行音频 I/O，也不设置功放增益。参考测试拦截了 DSP 发送，因此这些结果还不能证明 Windows 已应用音效。

后续的无声音效选择诊断在第一个默认音频流返回 `0x800700FF`；同一时段记录了 `audiodg` 堆故障（`0xC0000374`）。WER 加载模块列表中没有 Nabu DLL。原因尚未确定；回滚后全部基线初始化检查通过。诊断没有启动播放或提交音频数据。

原型进一步区分了**发现音效**和占用处理进程。此前仅发现模式返回空音效列表；现在会报告已配置的 Dolby 音效而不占用代理槽，即使槽已被占用也能发现。七项初始化检查通过，全部 196,608 个参考帧仍精确一致。这修复了原型协议，但尚未启用系统级 Dolby。

最新 SFX 与不包含现代选择器的有效传统 GFX 测试使用 Driver Store 中的签名 DLL 和属性变更通知，没有重启音频服务。两个零输入音频流通过检查，但仍没有 APO 处理。电平表约为 PCM16 一个最低有效位的转换抖动，不是音乐或测试音。两个临时包已移除，工作基线已完整恢复，无错误。

</details>
