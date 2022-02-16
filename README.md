# IbEverythingExt
[Everything](https://www.voidtools.com/) 拼音搜索、快速选择扩展。 

![](docs/preview.png)

## 安装
1. 安装 Everything v1.4.1.1015 x64 [安装版](https://www.voidtools.com/Everything-1.4.1.1015.x64-Setup.exe)或[便携版](https://www.voidtools.com/Everything-1.4.1.1015.x64.zip)（不支持精简版）。
1. 从 [Releases](../../releases) 下载压缩包。
1. 解压压缩包，将 bin 目录下的文件放入 Everything 安装目录（ `C:\Program Files\Everything` ）。
1. 重启 Everything。（如果不生效，请确认你安装了 [VC++ 2019 x64 运行库](https://support.microsoft.com/topic/the-latest-supported-visual-c-downloads-2647da03-1eea-4433-9aff-95f26a218cc0)）

## 拼音搜索
允许用拼音在 Everything 中搜索文件。

* 默认小写字母匹配拼音或字母，大写字母只匹配字母。
* 支持简拼、全拼、带声调全拼和双拼搜索，默认只开启简拼和全拼。  
  双拼搜索支持以下方案：
    * 微软双拼
    * 自然码双拼
    * 小鹤双拼
    * 拼音加加双拼
    * 智能 ABC 双拼
    * 华宇双拼（紫光双拼） 
  
  支持多音字和 Unicode 辅助平面汉字。
* 只支持 Everything 以下版本：
  * v1.4.1.1015 x64 [安装版](https://www.voidtools.com/Everything-1.4.1.1015.x64-Setup.exe)/[便携版](https://www.voidtools.com/Everything-1.4.1.1015.x64.zip)
  * v1.4.1.1009 x64 [安装版](https://www.voidtools.com/Everything-1.4.1.1009.x64-Setup.exe)/[便携版](https://www.voidtools.com/Everything-1.4.1.1009.x64.zip)
  * v1.5.0.1296a x64 [安装版](https://www.voidtools.com/Everything-1.5.0.1296a.x64-Setup.exe)/[便携版](https://www.voidtools.com/Everything-1.5.0.1296a.x64.zip)
* 后置修饰符：
  * `;py`：小写字母只匹配拼音
  * `;np`：禁用拼音搜索

![](docs/pinyin_search.png)

### 第三方程序支持
拼音搜索对调用 Everything 进行搜索的第三方程序同样生效，例如：
* [stnkl/EverythingToolbar](https://github.com/stnkl/EverythingToolbar)
* [Flow Launcher](https://github.com/Flow-Launcher/Flow.Launcher) 的 [Everything 插件](https://github.com/Flow-Launcher/Flow.Launcher.Plugin.Everything)
* [火柴（火萤酱）](https://www.huochaipro.com/)本地搜索
* [uTools](https://u.tools) 本地搜索
* [Wox](https://github.com/Wox-launcher/Wox) 的 Everything 插件

预览见 [第三方程序](docs/third_party/README.md)。

如果使用的是 Everything v1.5a，因为 Alpha 版默认启用了命名实例，大部分程序都不支持调用，需要[通过配置关闭命名实例](../../issues/5)。

### 配置
`IbEverythingExt.yaml` 文件：
```yaml
# 拼音搜索
pinyin_search:
  # true：开启，false：关闭
  enable: true

  # 模式
  # Auto：优先尝试 PCRE 模式，PCRE 模式不可用时使用 Edit 模式
  # Pcre：默认模式
  # Edit：只支持简拼搜索
  mode: Auto

  initial_letter: true  # 简拼
  pinyin_ascii: true  # 全拼
  pinyin_ascii_digit: false  # 带声调全拼
  double_pinyin_abc: false  # 智能 ABC 双拼
  double_pinyin_jiajia: false  # 拼音加加双拼
  double_pinyin_microsoft: false  # 微软双拼
  double_pinyin_thunisoft: false  # 华宇双拼（紫光双拼）
  double_pinyin_xiaohe: false  # 小鹤双拼
  double_pinyin_zrm: false  # 自然码双拼
```
Edit 模式详见 [Edit 模式](docs/pinyin_search/edit_mode.md)。

## 快速选择
在 Everything 结果列表的左侧插入一个显示 0\~9、A\~Z 的键列表，并允许在搜索编辑框和结果列表中通过热键快速打开对应文件。

### 热键
#### 模式1（默认）
搜索编辑框：
* `Alt+0~9`：打开文件（Enter）并关闭窗口
* `Alt+Ctrl+0~9`：定位文件（Ctrl+Enter）并关闭窗口
* `Alt+Shift+0~9`：打开右键菜单
* `Alt+Shift+0~9, R`：查看文件属性
* `Tab` / `Enter`：转移焦点到结果列表\*
* `Esc` / `Ctrl+W`：关闭窗口\*

结果列表：
* `[0-9A-Z]`：选中项目
* `Enter`：打开文件\*
* `Ctrl+Enter`：定位文件\*
* `Shift+F10` / `Menu`：打开右键菜单\*
* `Alt+Enter`：查看文件属性\*
* `Esc` / `Ctrl+W`：关闭窗口\*

#### 模式2
搜索编辑框/结果列表：
* `Alt+[0-9A-Z]`：打开文件（Enter）并关闭窗口
* `Alt+Ctrl+[0-9A-Z]`：定位文件（Ctrl+Enter）并关闭窗口
* `Alt+Shift+[0-9A-Z]`：打开右键菜单
* `Alt+Shift+[0-9A-Z], R`：查看文件属性
* `Esc` / `Ctrl+W`：关闭窗口\*

原本的 `Alt+A~Z` 访问菜单功能可以通过先单击 Alt 键再按 A\~Z 实现，默认的 `Alt+1~4` 调整窗口大小、`Alt+P` 预览和 `Alt+D` 聚焦搜索编辑框则无法使用，可以通过更改 Everything 选项来绑定到其它热键上（其中 `Alt+D` 也可使用相同功能的默认热键 `Ctrl+F` 和 `F3` 来代替）。

注：
* 操作之后是否关闭窗口可以通过配置进行控制。
* 标 \* 的热键为 Everything 默认热键，不是扩展增加的，在这里列出是为了完整性。

### 键列表
键列表支持高 DPI，但只在 Everything v1.5a 上支持缩放（热键为 `Ctrl+=` 和 `Ctrl+-`），在 Everything v1.4 上则不支持。

支持 Everything v1.5a 深色模式：  
![](docs/quick_select_dark_mode.png)

### 配置
`IbEverythingExt.yaml` 文件：
```yaml
# 快速选择
quick_select:
  # true：开启，false：关闭
  enable: true
  
  # 热键模式
  # 1, 2
  hotkey_mode: 1

  # 输入模拟模式
  # Auto：v1.5a→WmKey，v1.4→WmKey
  # WmKey
  # SendInput
  input_mode: Auto

  # 打开或定位文件后关闭窗口（不对 Everything 默认热键生效）
  # 如果想要默认 Enter 热键也关闭窗口，可在 Everything 快捷键选项中将“打开选中对象，并退出 Everything”设置为 Enter
  close_everything: true
```

## 使用技巧
一些 Everything 的使用技巧。

### 快速启动器
将 Everything 用作简易的快速启动器：
1. 添加运行次数列：右键结果列表表头，选中运行次数
2. 设置默认按运行次数排序：选项-常规-首页-排序-运行次数（降序）
3. 配合扩展的拼音搜索和快速选择启动文件

相较于传统的快速启动器，使用 Everything 的主要好处是去中心化，不必将所有启动入口集中维护，而是可以在任意层级的文件夹中声明启动入口，更加灵活，也减少了维护成本，更适合有大量启动入口的情况。

若想更进一步地提高启动效率，可以建立限定路径和扩展名的过滤器或书签，并通过快捷方式或 AutoHotkey 等工具注册全局热键，通过命令行新建 Everything 窗口并应用指定过滤器或书签。

### 硬盘清理
* 重复文件：`dupe: sizedupe:`
* 大于 100MB 的文件：`size:>100mb`
* Visual Studio 解决方案缓存：`wfn:.vs`

推荐开启文件夹大小索引，便于查看文件夹占用：选项-索引-索引文件夹大小

对于文件管理器 [Directory Opus](https://www.gpsoft.com.au/) 的用户，亦可使用 [IbDOpusExt](https://github.com/Chaoses-Ib/IbDOpusExt) 从 Everything 获取文件夹大小并显示为尺寸列，方便分析硬盘占用。

## 开发
见 [开发](docs/development.md)。