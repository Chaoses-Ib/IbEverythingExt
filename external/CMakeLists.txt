cmake_minimum_required(VERSION 3.15)

project(external)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

include(FetchContent)
FetchContent_Declare(IbWinCpp
    GIT_REPOSITORY https://github.com/Chaoses-Ib/IbWinCppLib.git
    GIT_TAG        f630f0e3a2c757652298c626fcd52469e5d5bc81
)
FetchContent_Declare(IbDllHijack
    GIT_REPOSITORY https://github.com/Chaoses-Ib/IbDllHijackLib.git
    GIT_TAG        a82533aff8a73129aee2a3757069f7aa89e7dd6a
)
FetchContent_Declare(IbEverything
    GIT_REPOSITORY https://github.com/Chaoses-Ib/IbEverythingLib.git
    GIT_TAG        f3fb2a5481aedbbe7b0dc063374690320a5193c2
)
FetchContent_Declare(IbPinyin
    GIT_REPOSITORY https://github.com/Chaoses-Ib/IbPinyinLib.git
    GIT_TAG        e576e81ca06a297436bba7b124630b5d64e3106f
)
FetchContent_Declare(IbUpdate
    GIT_REPOSITORY https://github.com/Chaoses-Ib/IbUpdateLib.git
    GIT_TAG        eaa36c76e29409aeb89b1d7f493f48d0b54d819c
)
FetchContent_Declare(sigmatch
    GIT_REPOSITORY https://github.com/SpriteOvO/sigmatch.git
    GIT_TAG        2ca983dc0cb78fe4f4406b67655e7ee132ecf5da
)
FetchContent_MakeAvailable(IbWinCpp IbDllHijack IbEverything IbPinyin IbUpdate sigmatch)