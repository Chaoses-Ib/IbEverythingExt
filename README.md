# IbEverythingExt
[Everything](https://www.voidtools.com/) 拼音搜索、快速选择扩展。 

## 预览
![](docs/preview.png)

## 安装
1. 安装 Everything v1.4.1.1015 x64 [安装版](https://www.voidtools.com/Everything-1.4.1.1015.x64-Setup.exe)或[便携版](https://www.voidtools.com/Everything-1.4.1.1015.x64.zip)（不支持精简版）。
1. 从 [Releases](../../releases) 下载压缩包。
1. 解压压缩包，将 bin 目录下的文件放入 Everything 安装目录（ `C:\Program Files\Everything` ）。
1. 重启 Everything。（如果不生效，请确认你安装了 [VC++ 2019 x64 运行库](https://support.microsoft.com/topic/the-latest-supported-visual-c-downloads-2647da03-1eea-4433-9aff-95f26a218cc0)）

## 拼音搜索
* 默认小写字母匹配拼音或字母，大写字母只匹配字母。
* 支持 Unicode 辅助平面汉字。

### PCRE 模式
* 支持简拼、全拼、带声调全拼和双拼搜索。（默认只开启简拼和全拼）  
  双拼搜索支持以下方案：
    * 微软双拼
    * 自然码双拼
    * 小鹤双拼
    * 拼音加加双拼
    * 智能 ABC 双拼
    * 华宇双拼（紫光双拼）
* 支持 Everything 以下版本：
  * v1.4.1.1015 x64 [安装版](https://www.voidtools.com/Everything-1.4.1.1015.x64-Setup.exe)/[便携版](https://www.voidtools.com/Everything-1.4.1.1015.x64.zip)
  * v1.4.1.1009 x64 [安装版](https://www.voidtools.com/Everything-1.4.1.1009.x64-Setup.exe)/[便携版](https://www.voidtools.com/Everything-1.4.1.1009.x64.zip)
  * v1.5.0.1296a x64 [安装版](https://www.voidtools.com/Everything-1.5.0.1296a.x64-Setup.exe)/[便携版](https://www.voidtools.com/Everything-1.5.0.1296a.x64.zip)
* 后置修饰符：
  * `;py`：小写字母只匹配拼音
  * `;np`：禁用拼音搜索

### Edit 模式（停止维护）
* 只支持简拼搜索。
* 支持 Everything x64 安装版和便携版，不支持精简版。
* 修饰符：
    * `py:` 小写字母只匹配拼音
    * `nopy:` 禁用拼音搜索（对所有关键字生效）

  <img src="docs/search.png" style="max-height: 500px;"/>

### 配置
`IbEverythingExt.yaml` 文件：
```yaml
# 拼音搜索
pinyin_search:
  # true：开启，false：关闭
  enable: true

  # 模式
  # Auto：优先尝试 PCRE 模式，PCRE 模式不可用时使用 Edit 模式
  # Pcre
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

## 快速选择
### 热键模式1（默认）
搜索编辑框：
* `Alt+0~9`：打开文件（Enter）并关闭窗口
* `Alt+Ctrl+0~9`：定位文件（Ctrl+Enter）并关闭窗口
* `Alt+Shift+0~9`：打开右键菜单
* `Alt+Shift+0~9, R`：查看文件属性
* `Tab`：转移焦点到结果列表\*
* `Esc` / `Ctrl+W`：关闭窗口\*

结果列表：
* `[0-9A-Z]`：选中项目
* `Enter`：打开文件\*
* `Ctrl+Enter`：定位文件\*
* `Shift+F10` / `Menu`：打开右键菜单\*
* `Alt+Enter`：查看文件属性\*
* `Esc` / `Ctrl+W`：关闭窗口\*

### 热键模式2
搜索编辑框/结果列表：
* `Alt+[0-9A-Z]`：打开文件（Enter）并关闭窗口
* `Alt+Ctrl+[0-9A-Z]`：定位文件（Ctrl+Enter）并关闭窗口
* `Alt+Shift+[0-9A-Z]`：打开右键菜单
* `Alt+Shift+[0-9A-Z], R`：查看文件属性
* `Esc` / `Ctrl+W`：关闭窗口\*

原本的 `Alt+A~Z` 访问菜单功能可以通过先单击 Alt 键再按 A\~Z 实现，默认的 `Alt+1~4` 调整窗口大小、`Alt+P` 预览和 `Alt+D` 聚焦搜索编辑框则无法使用，可以通过更改 Everything 选项来绑定到其它热键上（其中 `Alt+D` 也可使用相同功能的默认热键 `Ctrl+F` 和 `F3` 来代替）。

注：  
* `[0-9A-Z]` 指 0\~9 和 A\~Z 这 36 个键。  
* 操作之后是否关闭窗口可以通过配置进行控制。
* 标 \* 的热键为 Everything 默认热键，不是扩展增加的，在这里列出是为了完整性。

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
  # Auto, WmKey, SendInput
  input_mode: Auto

  # 打开或定位文件后关闭窗口（不对 Everything 默认热键生效）
  # 如果想要默认 Enter 热键也关闭窗口，可在 Everything 快捷键选项中将“打开选中对象，并退出 Everything”设置为 Enter
  close_everything: true
```

## 第三方程序支持
拼音搜索支持以下第三方程序调用：

* [stnkl/EverythingToolbar](https://github.com/stnkl/EverythingToolbar)  
  <img src="docs/EverythingToolbar.png" style="max-height: 400px;"/>
* [Flow Launcher](https://github.com/Flow-Launcher/Flow.Launcher) 的 [Everything 插件](https://github.com/Flow-Launcher/Flow.Launcher.Plugin.Everything)  
  <img src="docs/FlowLauncher.png"/>
* [火柴（火萤酱）](https://www.huochaipro.com/)本地搜索  
  <img src="docs/HuoChat.png"/>
* [uTools](https://u.tools) 本地搜索  
  <img src="docs/uTools.png" style="max-height: 400px;"/>
* [Wox](https://github.com/Wox-launcher/Wox) 的 Everything 插件 
  <img src="docs/Wox.png"/>

（如果使用的是 Everything Alpha 版，因为 Alpha 版默认启用了命名实例，大部分程序都不支持调用，需要[通过配置关闭命名实例](../../issues/5)。）

## 构建
1. 将以下库放入 `C:\L\C++\packages`（其它位置需要修改 .vcxproj 文件）：
    * [IbDllHijackLib](https://github.com/Chaoses-Ib/IbDllHijackLib/tree/master/DllHijackLib/IbDllHijackLib)
    * [IbEverythingLib](https://github.com/Chaoses-Ib/IbEverythingLib/tree/master/Cpp/IbEverythingLib)
    * [IbPinyinLib](https://github.com/Chaoses-Ib/IbPinyinLib)
    * [IbWinCppLib](https://github.com/Chaoses-Ib/IbWinCppLib/tree/master/WinCppLib/IbWinCppLib)
2. [vcpkg](https://github.com/microsoft/vcpkg)
    ```
    set VCPKG_DEFAULT_TRIPLET=x64-windows-static-md
    vcpkg install detours yaml-cpp
    ```
3. Test 还需要：
    ```
    vcpkg install boost-test pcre pcre2
    ```