#include "common.hpp"

#include "../Hijacker/match.cpp"

BOOST_AUTO_TEST_SUITE(Match)
    BOOST_AUTO_TEST_CASE(Match) {
    std::vector<pinyin::PinyinFlagValue> flags = { pinyin::PinyinFlag::PinyinAsciiDigit, pinyin::PinyinFlag::PinyinAscii, pinyin::PinyinFlag::Initial };
        int offsets[30];
        auto test = [&flags, &offsets](const char8_t* pattern, const char8_t* subject) {
            return match(pattern, subject, strlen((const char*)subject), flags, offsets, std::size(offsets));
        };

        // plain
        BOOST_CHECK(test(u8"pinyin", u8"pinyin") == 1 && offsets[0] == 0 && offsets[1] == 6);
        BOOST_CHECK(test(u8"pinyin", u8"PinYin") == 1 && offsets[0] == 0 && offsets[1] == 6);

        // pinyin
        BOOST_CHECK(test(u8"py", u8"拼音") == 1 && offsets[0] == 0 && offsets[1] == 6);
        BOOST_CHECK(test(u8"pinyin", u8"拼音") == 1 && offsets[0] == 0 && offsets[1] == 6);
        BOOST_CHECK(test(u8"pin1yin1", u8"拼音") == 1 && offsets[0] == 0 && offsets[1] == 6);

        // global
        BOOST_CHECK(test(u8"pinyin", u8"0123pinyin") == 1 && offsets[0] == 4 && offsets[1] == 10);
        BOOST_CHECK(test(u8"pinyin", u8"0123拼音") == 1 && offsets[0] == 4 && offsets[1] == 10);

        // multiple
        BOOST_CHECK(test(u8"pinyin", u8"01拼音23拼音") == 2 && offsets[0] == 2 && offsets[1] == 8 && offsets[2] == 10 && offsets[3] == 16);

        // mix
        BOOST_CHECK(test(u8"pinyin", u8"拼yin") == 1 && offsets[0] == 0 && offsets[1] == 6);
        BOOST_CHECK(test(u8"sous", u8"拼音搜索Everything") == 1 && offsets[0] == 6 && offsets[1] == 12);
    }

BOOST_AUTO_TEST_SUITE_END()