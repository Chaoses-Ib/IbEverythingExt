# 开发
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