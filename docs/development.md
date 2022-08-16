# 开发
## 构建
[vcpkg](https://github.com/microsoft/vcpkg)：
```cmd
vcpkg install detours yaml-cpp curl --triplet=x64-windows-static-md
```
[CMake](https://cliutils.gitlab.io/modern-cmake/)：
```cmd
cd IbEverythingExt
cd external
cmake -B build -DCMAKE_TOOLCHAIN_FILE="C:\...\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static-md
```

对于 test 还需要：
```
vcpkg install boost-test pcre pcre2 --triplet=x64-windows-static-md
```