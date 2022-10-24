#include "pch.h"
#include "PinyinSearch.hpp"
#include "PinyinSearchPcre.hpp"
#include "PinyinSearchEdit.hpp"
#include <cassert>

std::unique_ptr<PinyinSearch> make_pinyin_search(PinyinSearchMode mode, std::wstring& instance_name, HWND ipc_window) {
    switch (mode) {
    case PinyinSearchMode::Pcre:
        return std::make_unique<PinyinSearchPcre>();
    case PinyinSearchMode::Edit:
        return std::make_unique<PinyinSearchEdit>(instance_name, ipc_window);
    default:
        assert(false);
    }
}