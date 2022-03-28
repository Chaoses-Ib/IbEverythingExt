# 开发
## 构建
[vcpkg](https://github.com/microsoft/vcpkg)：
```
vcpkg install detours yaml-cpp --triplet=x64-windows-static-md
```
[CMake](https://cliutils.gitlab.io/modern-cmake/)：
```
cd IbEverythingExt
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:\...\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static-md
cmake --build . --config Debug
```

对于 test 还需要：
```
vcpkg install boost-test pcre pcre2 --triplet=x64-windows-static-md
```
以及在执行 `cmake ..` 时加上 `-DBUILD_TESTING=ON` 。