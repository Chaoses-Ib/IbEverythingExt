#include "common.hpp"
#include <pcre.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#undef _T
#define _T(x) u8##x

BOOST_AUTO_TEST_SUITE(PCRE_Match)

    uint32_t get_pinyin(char32_t c) {
        return 0x400;
    }

    BOOST_AUTO_TEST_CASE(Match) {
        auto match = [](std::string_view pattern, std::string_view subject) -> uint32_t {
            for (uint32_t i = 0; i < subject.size(); i++) {
                uint32_t j = 0, i_ = i;
                for (; j < pattern.size() && i_ < subject.size(); j++, i_++) {
                    if (subject[i_] == pattern[j] || subject[i_] > 'z' && get_pinyin(subject[i_]) & 1 << (pattern[j] - 'a'))
                        continue;
                    break;
                }
                if (j == pattern.size())
                    return i;
            }
            return (uint32_t)-1;
        };
        BOOST_CHECK(match("kk", "abcdefgkk.txt") == 7);
        BOOST_CHECK(match("kk", "abcdefg~~.txt") == 7);

        char pattern[] = "yy", subject[] = "abcdefg满天都是小星星.txt";
        DWORD t = timeGetTime();
        for (size_t i = 0; i < 1'000'000; i++) {
            if (__rdtsc() == 0) {
                pattern[0] = subject[0] = '\0';
            }
            match(pattern, subject);
        }
        t = timeGetTime() - t;
        BOOST_TEST_MESSAGE(t << "ms");
        // 10ms
    }

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(PCRE)

    BOOST_AUTO_TEST_CASE(Compile) {
        DWORD t = timeGetTime();
        std::u8string_view pattern = u8"[y" DYNAMIC_CHARSET_YY_1 u8"][y" DYNAMIC_CHARSET_YY_2 u8"]";
        int errorcode, erroroffset;
        const char* error;
        pcre* re = pcre_compile2((const char*)pattern.data(), PCRE_UCP | PCRE_UTF8 | PCRE_CASELESS, &errorcode, &error, &erroroffset, nullptr);

        int offsets[30], result;

        result = pcre_exec(re, nullptr, (const char*)u8"测试语言.txt", 16, 0, 0, offsets, std::size(offsets));
        t = timeGetTime() - t;
        BOOST_REQUIRE(result > 0 && offsets[0] == 6);
        BOOST_TEST_MESSAGE(t << "ms");
        // 0ms

        std::u8string_view subject(u8"abcdefg满天都是小星星.txt");
        t = timeGetTime();
        for (size_t i = 0; i < 1'000'000; i++) {
            result = pcre_exec(re, nullptr, (const char*)subject.data(), subject.size(), 0, 0, offsets, std::size(offsets));
        }
        t = timeGetTime() - t;
        BOOST_CHECK(result == -1);

        pcre_free(re);
        
        BOOST_TEST_MESSAGE(t << "ms");
        // 4800~5200ms
    }

    void JIT_common(std::u8string_view pattern, std::u8string_view test_subject, std::u8string_view subject, int subject_result = -1) {
        int errorcode, erroroffset;
        const char* error;
        DWORD t = timeGetTime();
        pcre* re = pcre_compile2((const char*)pattern.data(), PCRE_UCP | PCRE_UTF8 | PCRE_CASELESS, &errorcode, &error, &erroroffset, nullptr);

        pcre_extra* extra = pcre_study(re, PCRE_STUDY_JIT_COMPILE, &error);
        pcre_jit_stack* jit_stack = pcre_jit_stack_alloc(32 * 1024, 512 * 1024);
        pcre_assign_jit_stack(extra, nullptr, jit_stack);

        int offsets[30], result;

        result = pcre_exec(re, extra, (const char*)test_subject.data(), test_subject.size(), 0, 0, offsets, std::size(offsets));
        BOOST_REQUIRE(result > 0 && offsets[0] == 6);
        t = timeGetTime() - t;
        BOOST_TEST_MESSAGE(t << "ms");
        // 0ms
        
        t = timeGetTime();
        for (size_t i = 0; i < 1'000'000; i++) {
            result = pcre_exec(re, extra, (const char*)subject.data(), subject.size(), 0, 0, offsets, std::size(offsets));
        }
        t = timeGetTime() - t;
        BOOST_CHECK(result == subject_result);

        pcre_free(re);
        pcre_free_study(extra);
        pcre_jit_stack_free(jit_stack);

        BOOST_TEST_MESSAGE(t << "ms");
    }

    void JIT_common32(std::u32string_view pattern, std::u32string_view test_subject, std::u32string_view subject, int subject_result = -1) {
        int errorcode, erroroffset;
        const char* error;
        DWORD t = timeGetTime();
        pcre32* re = pcre32_compile2((PCRE_SPTR32)pattern.data(), PCRE_UCP | PCRE_UTF8 | PCRE_CASELESS, &errorcode, &error, &erroroffset, nullptr);

        pcre32_extra* extra = pcre32_study(re, PCRE_STUDY_JIT_COMPILE, &error);
        pcre32_jit_stack* jit_stack = pcre32_jit_stack_alloc(32 * 1024, 512 * 1024);
        pcre32_assign_jit_stack(extra, nullptr, jit_stack);

        int offsets[30], result;

        result = pcre32_exec(re, extra, (PCRE_SPTR32)test_subject.data(), test_subject.size(), 0, 0, offsets, std::size(offsets));
        BOOST_REQUIRE(result > 0 && offsets[0] == 2);
        t = timeGetTime() - t;
        BOOST_TEST_MESSAGE(t << "ms");
        // 0ms

        t = timeGetTime();
        for (size_t i = 0; i < 1'000'000; i++) {
            result = pcre32_exec(re, extra, (PCRE_SPTR32)subject.data(), subject.size(), 0, 0, offsets, std::size(offsets));
        }
        t = timeGetTime() - t;
        BOOST_CHECK(result == subject_result);

        pcre32_free(re);
        pcre32_free_study(extra);
        pcre32_jit_stack_free(jit_stack);

        BOOST_TEST_MESSAGE(t << "ms");
    }

#undef _T
#define _T(x) U##x

    BOOST_AUTO_TEST_CASE(JIT32) {
        JIT_common32(
            U"[y" DYNAMIC_CHARSET_YY_1 U"][y" DYNAMIC_CHARSET_YY_2 U"]",
            U"测试语言.txt",
            U"abcdefg满天都是小星星.txt"
        );
        // 600~800ms
        JIT_common32(
            U"[y" DYNAMIC_CHARSET_YY_1 U"][y" DYNAMIC_CHARSET_YY_2 U"]",
            U"测试语言.txt",
            U"abcdefghijklmnopqrstuvwxyzab.txt"
        );
        // 100~150ms
    }

#undef _T
#define _T(x) u8##x

    BOOST_AUTO_TEST_CASE(JIT) {
        JIT_common(
            u8"[y" DYNAMIC_CHARSET_YY_1 u8"][y" DYNAMIC_CHARSET_YY_2 u8"]",
            u8"测试语言.txt",
            u8"abcdefg满天都是小星星.txt"
        );
        // 930~960ms
        JIT_common(
            u8"[y" DYNAMIC_CHARSET_YY_1 u8"][y" DYNAMIC_CHARSET_YY_2 u8"]",
            u8"测试语言.txt",
            u8"abcdefghijklmnopqrstuvwxyzab.txt"
        );
        // 150~200ms
    }

    BOOST_AUTO_TEST_CASE(JIT_Lookaround) {
        JIT_common(
            u8"(?>y|(?=[〇-龥])[" DYNAMIC_CHARSET_YY_1 u8"])(?>y|(?=[〇-龥])[" DYNAMIC_CHARSET_YY_2 u8"])",
            u8"测试语言.txt",
            u8"abcdefg满天都是小星星.txt"
        );
        // 700~900ms
        JIT_common(
            u8"(?>y|(?=[〇-龥])[" DYNAMIC_CHARSET_YY_1 u8"])(?>y|(?=[〇-龥])[" DYNAMIC_CHARSET_YY_2 u8"])",
            u8"测试语言.txt",
            u8"abcdefghijklmnopqrstuvwxyzab.txt"
        );
        // 120~150ms
    }

    BOOST_AUTO_TEST_CASE(JIT_CharacterClass) {
        JIT_common(
            u8"[[:alpha:]][[:alpha:]]",
            u8"000111yy.txt",
            u8"0123456满天都是小星星.txt",
            1
        );
        // 150~200ms
        JIT_common(
            u8"[[:alpha:]][[:alpha:]]",
            u8"000111yy.txt",
            u8"0123456789!@#$%^&.....0123456789"
        );
        // 270~350ms
    }

    BOOST_AUTO_TEST_CASE(JIT_PlainOrHanzi) {
        JIT_common(
            u8"(?>yy|[y〇-龥]{2})",
            u8"000111语言.txt",
            u8"abcdefg满天都是小星星.txt",
            1
        );
        // 120~150ms
        JIT_common(
            u8"(?>yy|[y〇-龥]{2})",
            u8"000111语言.txt",
            u8"abcdefghijklmnopqrstuvwxyzab.txt"
        );
        // 130~160ms
    }

    BOOST_AUTO_TEST_CASE(JIT_PlainXorHanzi) {
        JIT_common(
            u8"(?>yy|[〇-龥]{2})",
            u8"000111语言.txt",
            u8"abcdefg满天都是小星星.txt",
            1
        );
        // 120~150ms
    }

    BOOST_AUTO_TEST_CASE(JIT_Plain) {
        JIT_common(
            u8"yy",
            u8"测试yy.txt",
            u8"abcdefg满天都是小星星.txt"
        );
        // 70~90ms
    }

    BOOST_AUTO_TEST_CASE(DFA) {
        std::u8string_view pattern = u8"[y" DYNAMIC_CHARSET_YY_1 u8"][y" DYNAMIC_CHARSET_YY_2 u8"]";
        int errorcode, erroroffset;
        const char* error;
        pcre* re = pcre_compile2((const char*)pattern.data(), PCRE_UCP | PCRE_UTF8 | PCRE_CASELESS, &errorcode, &error, &erroroffset, nullptr);
        
        int offsets[30], result;
        int workspace[4096];

        result = pcre_dfa_exec(re, nullptr, (const char*)u8"测试语言.txt", 16, 0, 0, offsets, std::size(offsets), workspace, std::size(workspace));
        BOOST_REQUIRE(result > 0 && offsets[0] == 6);

        std::u8string_view subject(u8"abcdefg满天都是小星星.txt");
        DWORD t = timeGetTime();
        for (size_t i = 0; i < 1'000'000; i++) {
            result = pcre_dfa_exec(re, nullptr, (const char*)subject.data(), subject.size(), 0, 0, offsets, std::size(offsets), workspace, std::size(workspace));
        }
        t = timeGetTime() - t;
        BOOST_CHECK(result == -1);

        pcre_free(re);
        
        BOOST_TEST_MESSAGE(t << "ms");
        // 4900~5600ms
    }

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(PCRE2)

    BOOST_AUTO_TEST_CASE(Compile) {
        std::u8string_view pattern = u8"[y" DYNAMIC_CHARSET_YY_1 u8"][y" DYNAMIC_CHARSET_YY_2 u8"]";
        int errorcode;
        PCRE2_SIZE erroroffset;
        pcre2_code* re = pcre2_compile((PCRE2_SPTR)pattern.data(), pattern.size(), PCRE2_UCP | PCRE2_UTF | PCRE2_CASELESS, &errorcode, &erroroffset, nullptr);
        
        pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(re, nullptr);

        int result = pcre2_match(re, (PCRE2_SPTR)u8"测试语言.txt", 16, 0, 0, match_data, nullptr);
        BOOST_REQUIRE(result > 0 && pcre2_get_ovector_pointer(match_data)[0] == 6);

        std::u8string_view subject(u8"abcdefg满天都是小星星.txt");
        DWORD t = timeGetTime();
        for (size_t i = 0; i < 1'000'000; i++) {
            result = pcre2_match(re, (PCRE2_SPTR)subject.data(), subject.size(), 0, 0, match_data, nullptr);
        }
        t = timeGetTime() - t;
        BOOST_CHECK(result == -1);

        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
        
        BOOST_TEST_MESSAGE(t << "ms");
        // 4700~4900ms
    }

    BOOST_AUTO_TEST_CASE(JIT) {
        std::u8string_view pattern = u8"[y" DYNAMIC_CHARSET_YY_1 u8"][y" DYNAMIC_CHARSET_YY_2 u8"]";
        int errorcode;
        PCRE2_SIZE erroroffset;
        pcre2_code* re = pcre2_compile((PCRE2_SPTR)pattern.data(), pattern.size(), PCRE2_UCP | PCRE2_UTF | PCRE2_CASELESS, &errorcode, &erroroffset, nullptr);

        pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);
        pcre2_match_context* mcontext = pcre2_match_context_create(nullptr);
        pcre2_jit_stack* jit_stack = pcre2_jit_stack_create(32 * 1024, 512 * 1024, nullptr);
        pcre2_jit_stack_assign(mcontext, nullptr, jit_stack);
        
        pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(re, nullptr);

        int result = pcre2_match(re, (PCRE2_SPTR)u8"测试语言.txt", 16, 0, 0, match_data, mcontext);
        BOOST_REQUIRE(result > 0 && pcre2_get_ovector_pointer(match_data)[0] == 6);

        std::u8string_view subject(u8"abcdefg满天都是小星星.txt");
        DWORD t = timeGetTime();
        for (size_t i = 0; i < 1'000'000; i++) {
            result = pcre2_match(re, (PCRE2_SPTR)subject.data(), subject.size(), 0, 0, match_data, mcontext);
        }
        t = timeGetTime() - t;
        BOOST_CHECK(result == -1);

        pcre2_code_free(re);
        pcre2_match_data_free(match_data);
        pcre2_match_context_free(mcontext);
        pcre2_jit_stack_free(jit_stack);

        BOOST_TEST_MESSAGE(t << "ms");
        // 950~1100ms
    }

BOOST_AUTO_TEST_SUITE_END()