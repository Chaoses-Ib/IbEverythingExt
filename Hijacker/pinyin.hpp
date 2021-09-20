#pragma once
#include <cstdint>
#include <string>

extern std::wstring pinyin_regexs[26];

struct PinyinRange {
    char32_t begin;
    char32_t end;
    uint32_t* table;

    bool has(char32_t c) const;
    uint32_t& get_flags(char32_t c) const;
};
extern PinyinRange pinyin_ranges[5];

void query_and_merge_into_pinyin_regexs();