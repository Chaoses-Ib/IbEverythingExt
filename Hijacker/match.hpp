#pragma once
#include <vector>
#include <IbWinCppLib/WinCppLib.hpp>
#define IB_PINYIN_ENCODING 32
#include <IbPinyinLib/Pinyin.hpp>

struct PatternFlag {
    using Value = unsigned int;
    using T = const Value;
    //static T case_ = 1;
    //static T wildcards = 2;
    //static T py = 3;
};

struct Pattern {
    PatternFlag::Value flags;
    std::vector<pinyin::PinyinFlagValue>* pinyin_flags;
    unsigned int pattern_len;
    unsigned int pattern_u8_len;
    //char32_t pattern[];
    //char8_t pattern_u8[];

    char32_t* pattern() {
        return ib::Addr(this) + sizeof(Pattern);
    }
    char8_t* pattern_u8() {
        return ib::Addr(this) + sizeof(Pattern) + (pattern_len + 1) * sizeof(char32_t);
    }
    std::u8string_view pattern_u8_sv() {
        return { pattern_u8(), pattern_u8_len };
    }
};

Pattern* compile(const char8_t* pattern, PatternFlag::Value flags, std::vector<pinyin::PinyinFlagValue>* pinyin_flags);

int exec(Pattern* pattern, const char8_t* subject, int length, size_t nmatch, int pmatch[], PatternFlag::Value flags);