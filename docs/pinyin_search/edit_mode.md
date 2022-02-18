# Edit 模式（停止维护）
* 默认小写字母匹配拼音或字母，大写字母只匹配字母。
* 只支持简拼搜索。
* 支持多音字和 Unicode 辅助平面汉字。
* 支持 Everything x64 安装版和便携版，不支持精简版。
* 修饰符：
    * `py:` 小写字母只匹配拼音
    * `nopy:` 禁用拼音搜索（对所有关键字生效）
* 出于稳定性考虑，Edit 模式只支持 [第三方程序](../third_party/README.md) 中列出的第三方程序。

![](edit_mode.png)

已知问题：
* [搜索历史功能破坏 #4](https://github.com/Chaoses-Ib/IbEverythingExt/issues/4)
* [不支持正则表达式 #8](https://github.com/Chaoses-Ib/IbEverythingExt/issues/8)
* [提高性能 #13](https://github.com/Chaoses-Ib/IbEverythingExt/issues/13)
* [拼音搜索不支持命令行搜索选项 #26](https://github.com/Chaoses-Ib/IbEverythingExt/issues/26)
* [filters don't wrok #32](https://github.com/Chaoses-Ib/IbEverythingExt/issues/32)
* [输入搜索并切换窗口，everything输入框出现乱码 #33](https://github.com/Chaoses-Ib/IbEverythingExt/issues/33)
* [搜索框中文输入变成从右至左了 #38](https://github.com/Chaoses-Ib/IbEverythingExt/issues/38)