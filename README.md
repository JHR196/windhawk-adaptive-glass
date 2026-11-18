# Windhawk 自适应玻璃

一套用于 Windows 11 的独立 Windhawk Mod。核心是让开始菜单、任务栏和通知面板在深浅背景上都保持可读，不靠切换 Windows 主题。各 Mod 可分别编译、调整和停用；这是基于 [ZenDesktop](https://github.com/Liset999/ZenDesktop) 的定制版，不是上游官方发行版。

## 开发基础与出处

本项目直接基于 **[Lanbo（Liset999）的 ZenDesktop](https://github.com/Liset999/ZenDesktop)** 开发；[Windhawk](https://github.com/ramensoftware/windhawk) 是编译和加载这些 Mod 的平台。本仓库是独立维护的定制版，并非上游官方发行版。本版的问题请提交到[本仓库 Issues](https://github.com/JHR196/windhawk-adaptive-glass/issues)。以下来源可与 [ZenDesktop 源码快照 `fcd7fcd`](https://github.com/Liset999/ZenDesktop/tree/fcd7fcd69e72d0c1ee0dbf87442f733b3db2a88f) 对照。

| 本仓库模块 | 代码来源与设计参考 |
|---|---|
| `start-style`、`taskbar-style`、`notifications`、`explorer`、`settings` | 来自 Lanbo 的 ZenDesktop 样式模块，分别建立在 **[m417z](https://github.com/m417z/my-windhawk-mods)** 的 Windows 11 Start Menu、Taskbar、Notification Center、File Explorer 和 Settings Styler 上（GPL-3.0）。内置社区主题由各主题作者贡献。控件树监听和模糊画刷沿用上游从 [TranslucentTB](https://github.com/TranslucentTB/TranslucentTB) 改编的 ExplorerTAP 和 XamlBlurBrush，不包含独立的 TranslucentTB 程序。 |
| `player` | 裁剪自 Lanbo 的 ZenDesktop 任务栏音乐与歌词模块。[上游声明](https://github.com/Liset999/ZenDesktop/blob/fcd7fcd69e72d0c1ee0dbf87442f733b3db2a88f/THIRD_PARTY_NOTICES-taskbar-lyrics.md) 将 [anync / TaskbarLyrics](https://github.com/ANYNC/TaskbarLyrics) 和 Gemini Spark 的 Taskbar Countdown Text 列为设计参考，未复制其代码。歌词数据可来自 LRCLIB、网易云音乐、QQ 音乐，版权归各自权利人。 |
| `spectrum` | 拆自 ZenDesktop 桌面小组件。宿主和渲染框架来自 m417z 的 [Desktop Live Overlay](https://windhawk.net/mods/desktop-live-overlay)；音频引擎来自 **USER// TOURNE** 的 [Tourne’Table 2.1.1](https://github.com/Liset999/ZenDesktop/blob/fcd7fcd69e72d0c1ee0dbf87442f733b3db2a88f/dev/widgets/vendor/tourne-table-2.1.1.wh.cpp)（MIT，上游另致谢 Salyts、GR0UD），由 Lanbo 整合。 |
| `animations` | 来自 ZenDesktop 的窗口动画模块，基于 **ReDrag** 的 [Windows Animations](https://windhawk.net/mods/windows-animations)（MIT）。macOS 最小化效果来自 **Abdullah Masood** 的 [macOS Minimize Animation](https://windhawk.net/mods/macos-minimize-animation)，其中的网格变形引擎等由 **Potassiumuncher** 在 [MacOS-Animation-for-windows](https://github.com/Potassiumuncher/MacOS-Animation-for-windows) 中实现。 |
| `caret` | 基于 Lanbo 的 ZenDesktop 输入状态提示。上游说明「光标优先、指针兜底」的思路参考了 [aardio / ImTip](https://github.com/aardio/ImTip)，未复制其代码。 |
| `groups`、`desktop-icons` | 本定制版的原生分类规则与桌面图标快捷键。开发分类功能时查阅了 [Feuer / Start Menu Category Manager](https://windhawk.net/mods/start-menu-category-manager)；这里提供的是按完整 AppID 配置的分组模块，不是该项目的分类编辑器。桌面图标模块使用可配置的组合键代替双击。 |

**本版主要改动**：开始菜单、分组和右侧面板按打开前的背景选字色，打开后保持；任务栏在背景持续变亮后才加强渐变衬底；音乐展开卡片在同一窗口绘制玻璃和内容，解决只见玻璃、不见文字的问题。这些改动、配套预设和验证记录由本仓库维护。

## 安装与回退

1. 安装 [Windhawk](https://windhawk.net/)。下载本仓库（**Code → Download ZIP**）并解压。若已有旧的一体版，先备份其设置，停用它再启用对应的独立 Mod。不要同时启用两个针对同一处的美化 Mod。
2. 在 Windhawk 创建新的本地 Mod，用 `mods/` 中所选 `.wh.cpp` 的完整内容替换模板，然后编译并启用。每个文件单独操作。只想用自适应外观，先装 `start-style`、`taskbar-style`、`notifications` 即可。
3. 在对应 Mod 的「高级 → Mod 设置」备份原设置，再按需粘贴 `presets/` 中同名 JSON 的完整内容并保存；导入会替换设置而不是合并。`player` 预设让无音乐时的左下角入口仍然可见。它启用了天气自动定位和允许通过 IP 定位；不想向第三方查询时，请关闭对应选项。
4. 回退时停用对应 Mod，或恢复此前备份的设置即可。任务栏的自适应背景不需要独立 TranslucentTB，避免叠加。系统更新后若某处异常，先停用负责那一处的 Mod。

## 模块索引

下表省略文件名前缀 `zen-split-` 和后缀 `.wh.cpp`。

| 模块 | 功能 |
|---|---|
| `start-style` | 开始菜单和分组按打开前的背景选字色，打开后锁定。 |
| `taskbar-style` | 上沿透明、向下渐变；持续明亮才加强衬色。 |
| `notifications` | 通知、日历、快捷设置分别按所在区域选色。 |
| `player` | 任务栏音乐播放、歌词，以及展开卡片中的天气和资源占用。媒体控制取决于播放器兼容性。 |
| `groups` | 用完整 AppID 自定义原生开始菜单分组；个人规则不随仓库发布。可用 PowerShell `Get-StartApps` 查询。已核对的 StartMenu.dll 为 x64 2608.26001.0.0。 |
| `explorer` / `settings` | 分别调整文件资源管理器和 Windows 设置的外观。 |
| `caret` | 光标旁的输入状态提示。 |
| `desktop-icons` | 用组合键切换桌面图标显隐。 |
| `animations` | 窗口动效。 |
| `spectrum` | 可选任务栏音频频谱，不装也不影响音乐播放。 |

## 验证与许可

[本机验收记录](docs/verification.md)。自适应亮度采样在本地进行，不保存屏幕图像；天气和歌词等可选联网功能与亮度采样无关。仓库不含编译好的 DLL、注册表转储、桌面截图或个人应用清单。

ZenDesktop 以 GPL-3.0 发布，全文见 [LICENSE](LICENSE)。各文件及内嵌组件保留各自的版权和许可：例如 `animations` 文件头标明 MIT，`spectrum` 整合文件标明 GPL-3.0，其中的音频引擎保留 MIT 全文。请一并保留源文件中的声明；另见 [ZenDesktop 可选模块第三方声明](https://github.com/Liset999/ZenDesktop/blob/fcd7fcd69e72d0c1ee0dbf87442f733b3db2a88f/optional/THIRD_PARTY_NOTICES.md)。
