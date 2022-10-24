cmake_minimum_required(VERSION 3.14)

project(external)

include(FetchContent)
FetchContent_Declare(IbWinCpp
    GIT_REPOSITORY https://github.com/Chaoses-Ib/IbWinCppLib.git
    GIT_TAG        a29ac95537f403ac5ae221cb744d3e82076efbf7
)
FetchContent_Declare(IbDllHijack
    GIT_REPOSITORY https://github.com/Chaoses-Ib/IbDllHijackLib.git
    GIT_TAG        a82533aff8a73129aee2a3757069f7aa89e7dd6a
)
FetchContent_Declare(IbEverything
    GIT_REPOSITORY https://github.com/Chaoses-Ib/IbEverythingLib.git
    GIT_TAG        0fda1b6a2db79112b08ebdfb1443a84959ddc042
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