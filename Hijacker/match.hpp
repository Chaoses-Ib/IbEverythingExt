#pragma once
#include <vector>
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
    char32_t pattern[];
};

Pattern* compile(const char8_t* pattern, PatternFlag::Value flags, std::vector<pinyin::PinyinFlagValue>* pinyin_flags);

int exec(Pattern* pattern, const char8_t* subject, int length, size_t nmatch, int pmatch[], PatternFlag::Value flags);