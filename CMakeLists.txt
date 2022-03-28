cmake_minimum_required(VERSION 3.14)

project(IbEverythingExt)

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
FetchContent_MakeAvailable(IbWinCpp IbDllHijack IbEverything IbPinyin)

find_package(yaml-cpp CONFIG REQUIRED)
# Detours
find_path(DETOURS_INCLUDE_DIR detours/detours.h)
find_library(DETOURS_LIBRARY detours)


# IbEverythingExt
set(resourceFiles
    resource.rc
    resource.h
)
list(TRANSFORM resourceFiles PREPEND source/)
source_group("Resource Files" FILES ${resourceFiles})

set(sourceFiles
    dllmain.cpp
    common.hpp
    helper.cpp helper.hpp

    config.cpp config.hpp
    ipc.cpp ipc.hpp
    ipc_window.cpp ipc_window.hpp
    
    PinyinSearch.cpp PinyinSearch.hpp
    PinyinSearchPcre.cpp PinyinSearchPcre.hpp
    match.cpp match.hpp
    PinyinSearchEdit.cpp PinyinSearchEdit.hpp
    pinyin.cpp pinyin.hpp
    search_history.cpp search_history.hpp

    quick_select.cpp quick_select.hpp
)
list(TRANSFORM sourceFiles PREPEND source/)

add_library(IbEverythingExt SHARED ${sourceFiles} ${resourceFiles})
set_target_properties(IbEverythingExt PROPERTIES OUTPUT_NAME WindowsCodecs)
target_compile_features(IbEverythingExt PRIVATE cxx_std_20)
#target_link_options(IbEverythingExt PRIVATE "$<$<CONFIG:Release>:/PDBALTPATH:https://github.com/Chaoses-Ib/IbEverythingExt>")

target_include_directories(IbEverythingExt
    PRIVATE ${DETOURS_INCLUDE_DIR}
)
target_link_libraries(IbEverythingExt
    PRIVATE IbWinCpp
    PRIVATE IbDllHijack
    PRIVATE IbEverything
    PRIVATE IbPinyin
    PRIVATE ${DETOURS_LIBRARY}
    PRIVATE yaml-cpp
)

# test
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(BUILD_TESTING "Build the testing tree." OFF)
    include(CTest)
    if(BUILD_TESTING)
        find_package(Boost REQUIRED COMPONENTS unit_test_framework)
        find_package(unofficial-pcre CONFIG REQUIRED)
        # PCRE2
        find_path(PCRE2_INCLUDE_DIR pcre2.h)
        find_library(PCRE2_LIBRARY pcre2-8.lib)

        set(testFiles
            main.cpp
            common.hpp
            v1.4.cpp
            v1.5a.cpp
            pcre.cpp
            match.cpp
        )
        list(TRANSFORM testFiles PREPEND test/)

        add_executable(IbEverythingExt_test ${testFiles})
        target_compile_features(IbEverythingExt_test PRIVATE cxx_std_20)
        target_include_directories(IbEverythingExt_test
            PRIVATE ${PCRE2_INCLUDE_DIR}
        )
        target_link_libraries(IbEverythingExt_test
            PRIVATE Boost::unit_test_framework
            PRIVATE IbWinCpp
            PRIVATE IbEverything
            PRIVATE IbPinyin
            PRIVATE unofficial::pcre::pcre unofficial::pcre::pcre32
            PRIVATE ${PCRE2_LIBRARY}
        )
    endif()
endif()