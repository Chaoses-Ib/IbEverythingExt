#include "pch.h"
#include "match.hpp"
#include <functional>

int match(const char8_t* pattern, const char8_t* subject, int length, std::vector<pinyin::PinyinFlagValue>& flags, int* offsets, int offsetcount)
{
    // DFA?
    auto char_match = [&flags](char32_t c, const char8_t* pattern) -> std::vector<size_t> {
        std::vector<size_t> v;
        if (c < 0x3007) {
            if ('A' <= c && c <= 'Z') {
                if (*pattern == c || *pattern == c - 'A' + 'a')
                    v.push_back(1);
            } else if ('a' <= c && c <= 'z') {
                if (*pattern == c || *pattern == c - 'a' + 'A')
                    v.push_back(1);
            } else {
                int char_len;
                char32_t pat_char = pinyin::read_char32((const char*)pattern, &char_len);
                if (pat_char == c)
                    v.push_back(char_len);
            }
        } else {
            for (pinyin::PinyinFlagValue flag : flags) {
                if (size_t size = pinyin::match_pinyin((const char*)pattern, c, flag))
                    v.push_back(size);
            }
        }
        return v;
    };
    std::function<const char8_t* (const char8_t*, const char8_t*)> subject_match = [&char_match, &subject_match](const char8_t* sub, const char8_t* pattern) -> const char8_t* {
        if (!*pattern)
            return sub;

        int char_len;
        char32_t c = pinyin::read_char32((const char*)sub, &char_len);
        if (!c)
            return nullptr;

        std::vector<size_t> v = char_match(c, pattern);
        for (size_t size : v) {
            if (const char8_t* s = subject_match(sub + char_len, pattern + size))
                return s;
        }
        return nullptr;
    };

    // global match
    const char8_t* sub = subject;
    int char_len;
    for (char32_t c = pinyin::read_char32((const char*)sub, &char_len); c; c = pinyin::read_char32((const char*)sub, &char_len)) {
        if (const char8_t* s = subject_match(sub, pattern)) {
            if (offsets && offsetcount >= 2) {  // may be null
                offsets[0] = sub - subject;
                offsets[1] = s - subject;
                return 1;
            } else {
                return 0;
            }
            /*
            sub = s;
            continue;
            */
        }
        sub += char_len;
    }

    return -1;
}