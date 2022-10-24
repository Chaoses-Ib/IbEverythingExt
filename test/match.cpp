#include "common.hpp"

#include "../EverythingExt/match.cpp"
#include "../EverythingExt/helper.cpp"

BOOST_AUTO_TEST_SUITE(Match)
    BOOST_AUTO_TEST_CASE(Match) {
        pinyin::init(pinyin::PinyinFlag::All);
        std::vector<pinyin::PinyinFlagValue> flags = { pinyin::PinyinFlag::PinyinAsciiDigit, pinyin::PinyinFlag::PinyinAscii, pinyin::PinyinFlag::InitialLetter, pinyin::PinyinFlag::DoublePinyinMicrosoft };
        int pmatch[20];
        auto test = [&flags, &pmatch](const char8_t* pat, const char8_t* subject) {
            Pattern* pattern = compile(pat, {}, &flags);
            int result = exec(pattern, subject, strlen((const char*)subject), std::size(pmatch) / 2, pmatch, {});
            HeapFree(GetProcessHeap(), 0, pattern);
            return result;
        };

        // plain
        BOOST_CHECK(test(u8"pinyin", u8"pinyin") == 1 && pmatch[0] == 0 && pmatch[1] == 6);
        BOOST_CHECK(test(u8"pinyin", u8"PinYin") == 1 && pmatch[0] == 0 && pmatch[1] == 6);

        // pinyin
        BOOST_CHECK(test(u8"py", u8"拼音") == 1 && pmatch[0] == 0 && pmatch[1] == 6);
        BOOST_CHECK(test(u8"pinyin", u8"拼音") == 1 && pmatch[0] == 0 && pmatch[1] == 6);
        BOOST_CHECK(test(u8"pin1yin1", u8"拼音") == 1 && pmatch[0] == 0 && pmatch[1] == 6);
        // DoublePinyinMicrosoft
        BOOST_CHECK(test(u8"pn", u8"拼音") == 1 && pmatch[0] == 0 && pmatch[1] == 3);

        // global
        BOOST_CHECK(test(u8"pinyin", u8"0123pinyin") == 1 && pmatch[0] == 4 && pmatch[1] == 10);
        BOOST_CHECK(test(u8"pinyin", u8"0123拼音") == 1 && pmatch[0] == 4 && pmatch[1] == 10);

        // multiple
        //BOOST_CHECK(test(u8"pinyin", u8"01拼音23拼音") == 1 && pmatch[0] == 2 && pmatch[1] == 8);

        // mix
        BOOST_CHECK(test(u8"pinyin", u8"拼yin") == 1 && pmatch[0] == 0 && pmatch[1] == 6);
        BOOST_CHECK(test(u8"sous", u8"拼音搜索Everything") == 1 && pmatch[0] == 6 && pmatch[1] == 12);

        pinyin::destroy();
    }

BOOST_AUTO_TEST_SUITE_END()