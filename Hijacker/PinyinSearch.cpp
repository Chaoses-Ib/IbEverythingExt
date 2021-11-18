#include "pch.h"
#include "PinyinSearch.hpp"
#include "PinyinSearchPcre.hpp"
#include "PinyinSearchEdit.hpp"

std::unique_ptr<PinyinSearch> make_pinyin_search(PinyinSearchMode mode, std::wstring& instance_name, HWND ipc_window) {
    if (mode == PinyinSearchMode::Auto) {
        try {
            return std::make_unique<PinyinSearchPcre>();
        } catch (const std::runtime_error& e) {
            mode = PinyinSearchMode::Edit;
        }
    }

    switch (mode) {
    case PinyinSearchMode::Pcre:
        return std::make_unique<PinyinSearchPcre>();
    case PinyinSearchMode::Edit:
        return std::make_unique<PinyinSearchEdit>(instance_name, ipc_window);
    }
}