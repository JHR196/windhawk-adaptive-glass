# Windhawk 自适应玻璃

一套用于 Windows 11 的独立 Windhawk Mod。核心是让开始菜单、任务栏和通知面板在深浅背景上都保持可读，不靠切换 Windows 主题。各 Mod 可分别编译、调整和停用；这是基于 [ZenDesktop](https://github.com/Liset999/ZenDesktop) 的定制版，不是上游官方发行版。

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

源文件保留上游作者、第三方来源及许可声明；GPL-3.0 全文见 [LICENSE](LICENSE)。
