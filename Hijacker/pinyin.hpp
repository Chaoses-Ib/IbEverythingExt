#pragma once
#include <cstdint>
#include <string>
#include <utility>

extern std::wstring pinyin_regexs[26];
extern std::pair<std::wstring, std::wstring> pinyin_pair_regexs[26][26];

struct PinyinRange {
    char32_t begin;
    char32_t end;
    uint32_t* table;

    bool has(char32_t c) const;
    uint32_t& get_flags(char32_t c) const;
};
extern PinyinRange pinyin_ranges[7];

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