#include "pch.h"
#include "match.hpp"
#include <functional>

char32_t read_char32(const char8_t* str, int* length) {
    char c = str[0];
    switch (*length = 1 + ((c & 0b11000000) == 0b11000000) + ((c & 0b11100000) == 0b11100000) + ((c & 0b11110000) == 0b11110000)) {
    case 1: return c;
    case 2: return (c & 0b11111) << 6 | (str[1] & 0b111111);
    case 3: return (c & 0b1111) << 12 | (str[1] & 0b111111) << 6 | (str[2] & 0b111111);
    case 4: return (c & 0b111) << 18 | (str[1] & 0b111111) << 12 | (str[2] & 0b111111) << 6 | (str[3] & 0b111111);
    }
}

Pattern* compile(const char8_t* pattern, PatternFlag::Value flags, std::vector<pinyin::PinyinFlagValue>* pinyin_flags) {
    size_t length = 1;  // '\0'
    size_t length_u8 = 0;
    {
        const char8_t* p = pattern;
        int char_len;
        for (char32_t c = read_char32(p, &char_len); c; c = read_char32(p += char_len, &char_len)) {
            length++;
            length_u8 += char_len;
        }
    }
    //Pattern* pat = ib::Addr(new ib::Byte[sizeof Pattern + length * sizeof(char32_t)]);
    Pattern* pat = ib::Addr(HeapAlloc(GetProcessHeap(), 0, sizeof Pattern + length * sizeof(char32_t) + length_u8 * sizeof(char8_t)));

    pat->flags = flags;
    pat->pinyin_flags = pinyin_flags;

    pat->pattern_len = length - 1;
    pat->pattern_u8_len = length_u8;

    const char8_t* p = pattern;
    int char_len;
    for (size_t i = 0; i < length; i++) {
        pat->pattern()[i] = read_char32(p, &char_len);
        p += char_len;
    }

    memcpy(pat->pattern_u8(), pattern, length_u8);

    return pat;
}

int exec(Pattern* pattern, const char8_t* subject, int length, size_t nmatch, int pmatch[], PatternFlag::Value flags)
{
    const char8_t* subject_end = subject + length;
    
    // plain text match
    bool plain = true;
    {
        const char8_t* s = subject;
        int char_len;
        for (char32_t c = read_char32(s, &char_len); s != subject_end; c = read_char32(s += char_len, &char_len)) {
            if (c >= 0x3007) {
                plain = false;
                break;
            }
        }
    }
    if (plain) {
        std::u8string_view sv(subject, length);
        std::u8string_view pt = pattern->pattern_u8_sv();
        auto it = std::search(sv.begin(), sv.end(), pt.begin(), pt.end(),
            [](char8_t c1, char8_t c2) {
                return std::toupper(c1) == std::toupper(c2);
            });

        if (it == sv.end()) {
            return -1;
        } else {
            if (nmatch) {
                pmatch[0] = it - sv.begin();
                pmatch[1] = it - sv.begin() + pt.size();
                return 1;
            } else {
                return 0;
            }
        }
    }

    // DFA?
    auto char_match = [pattern](char32_t c, const char32_t* pat) -> std::vector<size_t> {
        std::vector<size_t> v;
        if (c == *pat)
            v.push_back(1);
        else {
            if (c < 0x3007) {
                if (U'A' <= c && c <= U'Z') {
                    if (*pat == c - U'A' + U'a')
                        v.push_back(1);
                } else if (U'a' <= c && c <= U'z') {
                    if (*pat == c - U'a' + U'A')
                        v.push_back(1);
                }
            } else {
                for (pinyin::PinyinFlagValue flag : *pattern->pinyin_flags) {
                    if (size_t size = pinyin::match_pinyin(pat, c, flag))
                        v.push_back(size);
                }
            }
        }
        return v;
    };
    std::function<const char8_t* (const char8_t*, const char32_t*)> subject_match = [&char_match, &subject_match, subject_end](const char8_t* sub, const char32_t* pattern) -> const char8_t* {
        if (!*pattern)
            return sub;

        if (sub == subject_end)
            return nullptr;
        int char_len;
        char32_t c = read_char32(sub, &char_len);

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
    while (sub != subject_end) {
        if (const char8_t* s = subject_match(sub, pattern->pattern())) {
            if (nmatch) {
                pmatch[0] = sub - subject;
                pmatch[1] = s - subject;
                return 1;
            } else {
                return 0;
            }
            /*
            sub = s;
            continue;
            */
        }
        read_char32(sub, &char_len);
        sub += char_len;
    }

    return -1;
}