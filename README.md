# IbEverythingExt
[Everything](https://www.voidtools.com/) 拼音搜索扩展。 

## 预览
![](docs/preview.png)

## 安装
1. 支持 [Everything](https://www.voidtools.com/zh-cn/downloads/) x64 安装版和便携版，不支持精简版（Lite）。  
  [v1.5 Alpha](http://www.voidtools.com/forum/viewtopic.php?f=12&t=9787) 目前相比 v1.4 有大约 30% 的性能提升，但没有中文语言且可能存在一些 bug，请根据自己的需要进行选择。
1. 从 [Releases](../../releases) 下载压缩包。
1. 解压压缩包，将 WindowsCodecs.dll 放入 Everything 安装目录（ `C:\Program Files\Everything` ）。
1. 重启 Everything。（如果不生效，请确认你安装了 [VC++ 2019 x64 运行库](https://support.microsoft.com/topic/the-latest-supported-visual-c-downloads-2647da03-1eea-4433-9aff-95f26a218cc0)）

## 搜索说明
* 拼音搜索支持包括辅助平面在内的 Unicode 汉字。
* 默认小写字母匹配拼音或字母，大写字母只匹配字母。
* 修饰符
    * py: 小写字母只匹配拼音
    * nopy: 禁用拼音搜索（对所有关键字生效）

<img src="docs/search.png" max-height="500">

## 第三方程序支持
（Everything Alpha 版默认启用了命名实例，大部分程序都不支持调用，需要[通过配置关闭命名实例](../../issues/5)。）

* [stnkl/EverythingToolbar](https://github.com/stnkl/EverythingToolbar)  
  <img src="docs/EverythingToolbar.png" max-height="400">
* [uTools](https://u.tools) 本地搜索  
  <img src="docs/uTools.png" max-height="400">

## 构建
* Hijacker 和 Test
    1. 将以下库放入 `C:\L\C++\packages`（其它位置需要修改 .vcxproj 文件）：
        * [IbDllHijackLib](https://github.com/Chaoses-Ib/IbDllHijackLib/tree/master/DllHijackLib/IbDllHijackLib)
        * [IbEverythingLib](https://github.com/Chaoses-Ib/IbEverythingLib/tree/master/Cpp/IbEverythingLib)
        * [IbWinCppLib](https://github.com/Chaoses-Ib/IbWinCppLib/tree/master/WinCppLib/IbWinCppLib)
    1. [vcpkg](https://github.com/microsoft/vcpkg)
        ```
        set VCPKG_DEFAULT_TRIPLET=x64-windows-static-md
        vcpkg install detours
        ```
        （x86 版本的 VCPKG_DEFAULT_TRIPLET 应为  x86-windows-static-md）
    1. Test 还需要：
        ```
        vcpkg install boost-test
        ```
* data
    1. 从 [mozillazg/pinyin-data](https://github.com/mozillazg/pinyin-data) 获取 `pinyin.txt`，放入 data 目录。
    1. 运行 `generate_ord_pinyin.py`，得到 `output_ord_pinyin.txt`。