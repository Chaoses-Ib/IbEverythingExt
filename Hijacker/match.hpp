#pragma once
#include <vector>
#include <IbWinCppLib/WinCppLib.hpp>
#define IB_PINYIN_ENCODING 32
#include <IbPinyinLib/Pinyin.hpp>

struct CompileFlag {

};

struct PatternFlag {
    bool pinyin : 1;

    // note that regcomp_p2 will filter out content containing no lower letter
    // completely
    bool no_lower_letter : 1;

    bool match_at_start : 1;
};

struct ExecuteFlag {
    bool not_begin_of_line : 1;
};

struct Pattern {
    PatternFlag flags;
    std::vector<pinyin::PinyinFlagValue>* pinyin_flags;
    unsigned int pattern_len;
    unsigned int pattern_u8_len;
    //char32_t pattern[];
    //char8_t pattern_u8[];

    // null-terminated
    char32_t* pattern() {
        return ib::Addr(this) + sizeof(Pattern);
    }
    std::u32string_view pattern_sv() {
        return { pattern(), pattern_len };
    }

    // not null-terminated
    char8_t* pattern_u8() {
        return ib::Addr(this) + sizeof(Pattern) + (pattern_len + 1) * sizeof(char32_t);
    }
    std::u8string_view pattern_u8_sv() {
        return { pattern_u8(), pattern_u8_len };
    }
};

Pattern* compile(const char8_t* pattern, CompileFlag flags, std::vector<pinyin::PinyinFlagValue>* pinyin_flags);

int exec(Pattern* pattern, const char8_t* subject, int length, size_t nmatch, int pmatch[], ExecuteFlag flags);