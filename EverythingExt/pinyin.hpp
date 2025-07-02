#pragma once
#include <cstdint>
#include <string>
#include <utility>
#define IB_PINYIN_ENCODING 32
#include <IbPinyin/pinyin.hpp>

extern std::wstring pinyin_regexs[26];
extern std::pair<std::wstring, std::wstring> pinyin_pair_regexs[26][26];

struct Utf16Pair {
    wchar_t l;
    wchar_t h;

    Utf16Pair() : l(0), h(0) {}
    Utf16Pair(wchar_t l) : l(l), h(0) {}
    Utf16Pair(wchar_t l, wchar_t h) : l(l), h(h) {}

    bool in(std::wstring_view sv) const;
};

// require ipc_init
void pinyin_query_and_merge();