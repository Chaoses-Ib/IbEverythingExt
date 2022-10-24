﻿# IbEverythingExt
[Everything](https://www.voidtools.com/) 拼音搜索、快速选择扩展。 

![](docs/preview.png)

## 目录
* [安装](#安装)
* [拼音搜索](#拼音搜索)
  * [第三方程序支持](#第三方程序支持)
  * [配置](#配置)
* [快速选择](#快速选择)
  * [热键](#热键)
  * [键列表](#键列表)
  * [配置](#配置-1)
* [其它](#其它)
  * [快速启动器](#快速启动器)
  * [硬盘占用分析](#硬盘占用分析)
  * [检查更新](#检查更新)
* [→开发](docs/development.md)

## 安装
1. 安装 [Everything](https://www.voidtools.com/zh-cn/downloads/) x64 安装版或便携版（不支持其它架构和精简版）  
  如果你能接受英文界面，也可以选择安装 [Everything 1.5 预览版](https://www.voidtools.com/forum/viewtopic.php?f=12&t=9787#download)。
2. 安装 [VC++ 2022 x64 运行库](https://aka.ms/vs/17/release/vc_redist.x64.exe)  
   （[PowerToys](https://github.com/microsoft/PowerToys) 在安装时会同时安装 VC++ 2022 运行库，如果你安装了 PowerToys 就可以跳过这一步）
3. 从 [Releases](../../releases) 下载压缩包
4. 解压压缩包，将文件放入 Everything 安装目录（ `C:\Program Files\Everything` ）
5. 重启 Everything

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
* 后置修饰符：
  * `;py`：小写字母只匹配拼音
  * `;np`：禁用拼音搜索

<img src="docs/pinyin_search.png" height="400"/>

### [第三方程序支持](docs/third_party/README.md)
拼音搜索对调用 Everything 进行搜索的第三方程序同样生效，例如：
* [EverythingToolbar](https://github.com/stnkl/EverythingToolbar)  
  <img src="docs/third_party/EverythingToolbar.png" height="300"/>
* [Flow Launcher](https://github.com/Flow-Launcher/Flow.Launcher) 的 [Everything 插件](https://github.com/Flow-Launcher/Flow.Launcher.Plugin.Everything)  
  <img src="docs/third_party/FlowLauncher.png" height="200"/>
* [PowerToys Run](https://learn.microsoft.com/windows/powertoys/run) 的 [Everything 插件](https://github.com/lin-ycv/EverythingPowerToys)  
  <img src="docs/third_party/PowerToys.png" height="200"/>
* [uTools](https://u.tools) 本地搜索  
  <img src="docs/third_party/uTools.png" height="300"/>
* [Wox](https://github.com/Wox-launcher/Wox) 的 Everything 插件
* [火柴（火萤酱）](https://www.huochaipro.com/)本地搜索

如果使用的是 Everything 1.5 预览版，因为预览版默认启用了命名实例，大部分程序都不支持调用，需要[通过配置关闭命名实例](https://github.com/Chaoses-Ib/IbEverythingExt/issues/5)。

### 配置
`config.yaml` 文件：
```yaml
# 拼音搜索
pinyin_search:
  # true：开启，false：关闭
  enable: true

  # 模式
  # Pcre：默认模式
  # Edit：版本兼容性好，但只支持简拼搜索，性能较低，且存在许多 bug
  mode: Pcre

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
搜索编辑框：
热键 | 功能
--- | ---
`Alt+0~9` | 打开文件（Enter）并关闭窗口
`Alt+Ctrl+0~9` | 定位文件（Ctrl+Enter）并关闭窗口
`Alt+Shift+0~9` | 打开右键菜单
`Alt+Shift+0~9, R` | 查看文件属性
`Tab` / `Enter` | 转移焦点到结果列表\*
`Esc` / `Ctrl+W` | 关闭窗口\*

结果列表：
热键 | 功能
--- | ---
`[0-9A-Z]` | 选中项目
`Enter` | 打开文件\*
`Ctrl+Enter` | 定位文件\*
`Shift+F10` / `Menu` | 打开右键菜单\*
`Alt+Enter` | 查看文件属性\*
`Esc` / `Ctrl+W` | 关闭窗口\*
`$ (Shift+4)` | 复制文件名，在文件所属目录下启动终端
`# (Shift+3)` | 复制文件名，以管理员身份在文件所属目录下启动终端

注：
* 操作之后是否关闭窗口可以通过配置进行控制。
* 标 \* 的热键为 Everything 默认热键，不是扩展增加的，在这里列出是为了完整性。

### 键列表
键列表支持高 DPI，但只在 Everything v1.5a 上支持缩放（热键为 `Ctrl+=` 和 `Ctrl+-`），在 Everything v1.4 上则不支持。

支持 Everything v1.5a 深色模式：  
![](docs/quick_select_dark_mode.png)

### 配置
`config.yaml` 文件：
```yaml
# 快速选择
quick_select:
  # true：开启，false：关闭
  enable: true

  # 搜索编辑框
  search_edit:
    # Alt 组合键范围
    # 0：禁用
    # 10：Alt+0~9
    # 36：Alt+[0-9A-Z]
      # 原本的 Alt+A~Z 访问菜单功能可以通过先单击 Alt 键再按 A~Z 实现
      # 默认的 Alt+1~4 调整窗口大小、Alt+P 预览和 Alt+D 聚焦搜索编辑框则无法使用，可以通过更改 Everything 选项来绑定到其它热键上（其中 Alt+D 也可使用相同功能的 Ctrl+F 和 F3 来代替）
    alt: 10

  # 结果列表
  result_list:
    # 同上
    alt: 0

    # [0-9A-Z] 选中项目
    select: true

    # 终端
    # Windows Terminal："wt -d ${fileDirname}"
    # Windows Console："conhost"（不支持以管理员身份启动）
    # 禁用：""
    terminal: "wt -d ${fileDirname}"

  # 打开或定位文件后关闭窗口（不对 Everything 默认热键生效）
  # 如果想要默认 Enter 热键也关闭窗口，可在 Everything 快捷键选项中将“打开选中对象，并退出 Everything”设置为 Enter
  close_everything: true

  # 输入模拟模式
  # Auto：v1.5a→WmKey，v1.4→SendInput
  # WmKey
  # SendInput
  input_mode: Auto
```

## 其它
### 快速启动器
相较于使用传统的快速启动器，使用 Everything 这类文件搜索器的主要好处是可以实现去中心化——不必将所有的启动入口集中到一个地方，而是可以在任意个文件夹下分散放置启动入口（快捷方式、笔记文件等），不仅降低了维护成本，还能让个人的文件管理结构更加统一，更适合有大量启动入口的情况。

若要将 Everything 用作快速启动器，推荐进行以下配置：
1. 添加运行次数列：右键结果列表表头，选中运行次数
2. 设置默认按运行次数排序：`选项 → 常规 → 首页 → 排序 → 运行次数（降序）`
3. 配合扩展的拼音搜索和快速选择启动文件

若要更进一步地提高启动效率，可以建立限定路径和扩展名的过滤器或书签，并通过快捷方式或 AutoHotkey 等工具注册全局热键，通过命令行新建 Everything 窗口并应用指定的过滤器或书签。

### 硬盘占用分析
* 重复文件  
  `dupe: sizedupe:`  
  其中 `dupe:` 用于限定文件名重复，`sizedupe:` 用于限定文件大小重复。

  ![](docs/Disk%20space/images/dupe.png)
* 大于 100MB 的文件  
  `size:>100mb`
* 空文件夹  
  `empty:`
* Visual Studio 解决方案缓存  
  `wfn:.vs`  

  ![](docs/Disk%20space/images/VisualStudio.png)

  或者也可使用基于 Everything 实现的专用于清理 `.vs` 的工具 [VsCacheCleaner](https://github.com/SpriteOvO/VsCacheCleaner)。

注意，显示文件夹大小需要在 Everything 选项中开启 `索引 → 索引文件夹大小`。

对于文件管理器 [Directory Opus](https://github.com/Chaoses-Ib/DirectoryOpus) 的用户，亦可使用 [IbDOpusExt](https://github.com/Chaoses-Ib/IbDOpusExt) 从 Everything 获取文件夹大小并显示为列，便于分析硬盘占用：  
![](https://github.com/Chaoses-Ib/IbDOpusExt/blob/develop/docs/images/SizeCol.png?raw=true)

### 检查更新
`config.yaml` 文件：
```yaml
# 更新
update:
  # 检查更新
  check: true

  # 包括预览版
  prerelease: false
```

## 开发
见 [开发](docs/development.md)。