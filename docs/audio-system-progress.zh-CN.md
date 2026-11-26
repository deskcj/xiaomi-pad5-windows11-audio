# 音频进展 · 2026-11-26

[English](audio-system-progress.md) · [Русский](audio-system-progress.ru.md) · [简体中文](audio-system-progress.zh-CN.md)

- **修改：**拒绝无效果控制、预热后精确恢复 Dolby 状态、Automatic／SSD 日志安装以及 MiSound 原始 ACDB 资源读取。
- **通过：**34 项控制／24 项工厂、10,000 次事件替换及 2,048 次竞争；精确 ON/OFF/ON、重置／工作进程替换；SCM 安装／复用／回滚；31 个 ACDB 属性及 12 项拒绝／边界、33 项 NOACCESS。
- **保留失败：**冷 .20 有 32 帧回退。预热比较精确且零回退，尚未证明普遍稳定性。本次无新声学测试或 APO .20 audiodg 测试。

| 项目 | 已验证 | 待完成 |
| --- | --- | --- |
| 四功放／UEFI | 已安装 .20，支持现有及 Remtrik ID。 | Remtrik 实机安装。 |
| Dolby | 先前真实 SFX；.20 原版输出／控制测试；独立后端及 SCM Automatic。 | 常驻音频图／冷启动、EffectsManager、延迟／恢复和 Atmos 对象。 |
| 立体声 | 数字 Normal/RAW L/R。 | 每颗扬声器声学隔离。 |
| MiSound | 原厂数据／控制及 ACDB 资源读取。 | 真实 DSP 模块、音频图、COPP 和处理。 |
| PCM24／音质 | 此前软件验证。 | 硬件 PCM24、HyperOS 音质、睡眠及最大音量保护。 |

当前平板：功放 **4.17.27.20**、APO **2.0.0.4**；.18 后端已恢复 Manual。Qualcomm 待重启状态仍阻止新 APO 测试。.20 是签名候选，未获实机验证。

[详细历史](history/system-2026-11-26.zh-CN.md) · [26.11 · JSON](results/audio-dolby-runtime-acdb-2026-11-26.json)
