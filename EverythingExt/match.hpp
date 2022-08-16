#pragma once
#include "helper.hpp"
#include <vector>
#include <functional>
#include <IbWinCpp/WinCpp.hpp>
#define IB_PINYIN_ENCODING 32
#include <IbPinyin/pinyin.hpp>

/*
SEARCH_STD
// SEARCH_STD_SEARCHER
SEARCH_BOOST_ALGORITHM
SEARCH_BOOST_XPRESSIVE
*/
#define SEARCH_STD

#ifdef SEARCH_BOOST_ALGORITHM
#include <boost/algorithm/string/find.hpp>
#endif
#ifdef SEARCH_BOOST_XPRESSIVE
#include <boost/xpressive/xpressive.hpp>
#endif

struct CompileFlag {
    bool match_at_start : 1;
    bool match_at_end : 1;
};

struct PatternFlag {
    bool pinyin : 1;

    // note that regcomp_p2 will filter out content containing no lower letter
    // completely
    bool no_lower_letter : 1;

    bool match_at_start : 1;
    bool match_at_end : 1;
};

struct ExecuteFlag {
    bool not_begin_of_line : 1;
};

struct Pattern {
    PatternFlag flags;
    std::vector<pinyin::PinyinFlagValue>* pinyin_flags;
    std::u32string pattern;
    std::u8string pattern_u8_upper;
    size_t pattern_u8_len;
#ifdef SEARCH_STD_SEARCHER
    std::boyer_moore_horspool_searcher<std::u8string_view::const_iterator,
        decltype([](char8_t c) { return std::hash<char8_t>()(ib::toupper(c)); }),
        decltype([](char8_t c1, char8_t c2) {
            //DebugOStream() << wchar_t(c1) << L' ' << wchar_t(c2) << L'\n';
            // c2 is not guaranteed to be from pattern?
            return ib::toupper(c1) == ib::toupper(c2);  // don't use std::toupper, it's slow
        })
    > searcher;
#endif
#ifdef SEARCH_BOOST_XPRESSIVE
        boost::xpressive::regex_traits<char8_t> traits;
        boost::xpressive::detail::boyer_moore<std::u8string_view::const_iterator, boost::xpressive::regex_traits<char8_t>> searcher;
#endif
};

Pattern* compile(const char8_t* pattern, CompileFlag flags, std::vector<pinyin::PinyinFlagValue>* pinyin_flags);

int exec(Pattern* pattern, const char8_t* subject, int length, size_t nmatch, int pmatch[], ExecuteFlag flags);