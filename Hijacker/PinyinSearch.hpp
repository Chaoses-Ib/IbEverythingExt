#pragma once
#include <memory>
#include <string>

#include <vector>
#define IB_PINYIN_ENCODING 32
#include <IbPinyinLib/Pinyin.hpp>

enum class PinyinSearchMode {
    Auto,
    Pcre,
    Edit
};

class PinyinSearch {
public:
    virtual ~PinyinSearch() {}

    virtual void everything_created() {}
    virtual void edit_created(HWND edit) {}
};

std::unique_ptr<PinyinSearch> make_pinyin_search(PinyinSearchMode mode, std::wstring& instance_name, HWND ipc_window);