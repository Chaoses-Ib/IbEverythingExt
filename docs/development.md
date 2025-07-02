# 开发
## 构建
[Rust](https://rustup.rs/)

[vcpkg](https://github.com/microsoft/vcpkg)：
```cmd
vcpkg install fmt detours yaml-cpp curl --triplet=x64-windows-static
```
[CMake](https://cliutils.gitlab.io/modern-cmake/)：
```cmd
cd IbEverythingExt
cd external
cmake -B build -DCMAKE_TOOLCHAIN_FILE="C:\...\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

After building projects in VS, run `cargo build -p plugin` to build the plugin.

对于 test 还需要：
```
vcpkg install boost-test pcre pcre2 --triplet=x64-windows-static
```

## CRT
All projects use static release CRT:
- Everything uses static CRT
- Rust uses release CRT

## Bump version
- [Cargo.toml](../Cargo.toml)
- ~~EverythingExt\resource.rc~~
- [Updater\updater.cpp](../Updater/updater.cpp)
