#include "common.hpp"

BOOST_AUTO_TEST_SUITE(v1_5a)

BOOST_FIXTURE_TEST_SUITE(SingleCharQueryPerformance, Everythings::Everything)
    void test_query(std::wstring_view query) {
        using namespace Everythings;

        Everything ev(L"1.5a");
        DWORD t = timeGetTime();
        ev.query_send(query, 0, Request::FileName);
        QueryResults results = ev.query_get();
        t = timeGetTime() - t;

        BOOST_CHECK(results.found_num);
        BOOST_TEST_MESSAGE(t << "ms");
        Sleep(2000);
    }

    BOOST_AUTO_TEST_SUITE(Char)

        BOOST_AUTO_TEST_CASE(Char) {
            test_query(L"y");
        }

        BOOST_AUTO_TEST_CASE(Regex) {
            test_query(L"regex:y");
        }

        BOOST_AUTO_TEST_CASE(RegexSet) {
            test_query(L"regex:[y]");
        }

    BOOST_AUTO_TEST_SUITE_END()


    BOOST_AUTO_TEST_SUITE(Pinyin)

        BOOST_AUTO_TEST_CASE(Base_v03) {
            test_query(L"regex:[y" DYNAMIC_CHARSET_Y L"]");
        }
        
        BOOST_AUTO_TEST_CASE(LetterBranch) {
            test_query(L"regex:y|[" DYNAMIC_CHARSET_Y L"]");
        }

        BOOST_AUTO_TEST_CASE(LetterBranchCase) {
            test_query(L"case:regex:y|Y|[" DYNAMIC_CHARSET_Y L"]");
        }

        BOOST_AUTO_TEST_CASE(Base_v02) {
            test_query(L"case:regex:[yY" DYNAMIC_CHARSET_Y L"]");
        }

        // better
        BOOST_AUTO_TEST_CASE(Dynamic) {
            test_query(L"regex:[y" DYNAMIC_CHARSET_Y L"]");
        }

        // better
        BOOST_AUTO_TEST_CASE(Gb2312) {
            test_query(L"regex:[y" + pinyin_regex_gb2312 + L"]");
        }

        // faster but not the same
        BOOST_AUTO_TEST_CASE(Count) {
            // place count before regex!
            test_query(L"count:100 regex:[y" + pinyin_regex + L"]");
        }

        // faster but not the same
        BOOST_AUTO_TEST_CASE(Boundary) {
            test_query(L"regex:\\b[y" + pinyin_regex + L"]");
        }

        // better
        BOOST_AUTO_TEST_CASE(Case) {
            test_query(L"case:regex:[yY" + pinyin_regex + L"]");
        }

        BOOST_AUTO_TEST_CASE(Base_v01) {
            test_query(L"regex:[y" + pinyin_regex + L"]");
        }

        /*
        // cpu killer
        BOOST_AUTO_TEST_CASE(Branch) {
            test_query(L"regex:y|" + pinyin_branch);
        }
        */

        // worse
        BOOST_AUTO_TEST_CASE(Lookaround1) {
            test_query(L"regex:<y|(?=[" + pinyin_range + L"])[" + pinyin_regex + L"]>");
        }

        BOOST_AUTO_TEST_CASE(Lookaround2_1) {
            // regex:(?>y|(?=[一-龠])一)
            test_query(L"regex:(?>y|(?=[" + pinyin_range + L"])[" + pinyin_regex + L"])");
        }

        // worse than 1
        BOOST_AUTO_TEST_CASE(Lookaround2) {
            // regex:(y|(?=[一-龠])一)
            test_query(L"regex:(y|(?=[" + pinyin_range + L"])[" + pinyin_regex + L"])");
        }

        // worse than 2
        BOOST_AUTO_TEST_CASE(Lookaround3) {
            // regex:(y|(?![\x00-\xff])一)
            test_query(L"regex:(y|(?![\\x00-\\xff])[" + pinyin_regex + L"])");
        }

        // worse than 3
        BOOST_AUTO_TEST_CASE(Lookaround4) {
            // regex:(y|(?![一-龠])(?!)|一)
            test_query(L"regex:(y|(?![" + pinyin_range + L"])(?!)|[" + pinyin_regex + L"])");
        }
    BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(MultiCharsQueryPerformance)
    void test_query(std::wstring_view query) {
        using namespace Everythings;

        Everything ev(L"1.5a");
        DWORD t = timeGetTime();
        ev.query_send(query, 0, Request::FileName);
        QueryResults results = ev.query_get();
        t = timeGetTime() - t;

        BOOST_CHECK(results.found_num);
        BOOST_TEST_MESSAGE(t << "ms");
        Sleep(2000);
    }

    BOOST_AUTO_TEST_SUITE(Char)

        BOOST_AUTO_TEST_CASE(Branch) {
            test_query(L"x|y");
        }

        BOOST_AUTO_TEST_CASE(WildcardsSet) {
            test_query(L"wildcards:*[xy]*");
        }

        // faster than set?
        BOOST_AUTO_TEST_CASE(RegexBranch) {
            test_query(L"regex:x|y");
        }

        BOOST_AUTO_TEST_CASE(RegexSet) {
            test_query(L"regex:[xy]");
        }

    BOOST_AUTO_TEST_SUITE_END()


    BOOST_AUTO_TEST_SUITE(Char100)
        
        BOOST_AUTO_TEST_CASE(RegexSet) {
            test_query(L"regex:[一与业严丫丿么义乐乙也予于亏云亚亦亿仡以仪仰伊众优伛伢但佑佗余佚佣佯佻佾侑依侥俑俞俣俨俺倚偃允元兖养冤冶刈刖剡劓勇匀医卣印厂压厌原厣又友台右叶叹吁吆吖吟听吲吾呀呓员呦咏咦咬咽咿哑哕哟哩哽唁唷喁喑喝喻嗌]");
        }

        // slow
        BOOST_AUTO_TEST_CASE(WildcardsSet) {
            test_query(L"wildcards:*[一与业严丫丿么义乐乙也予于亏云亚亦亿仡以仪仰伊众优伛伢但佑佗余佚佣佯佻佾侑依侥俑俞俣俨俺倚偃允元兖养冤冶刈刖剡劓勇匀医卣印厂压厌原厣又友台右叶叹吁吆吖吟听吲吾呀呓员呦咏咦咬咽咿哑哕哟哩哽唁唷喁喑喝喻嗌]*");
        }

        // very slow
        BOOST_AUTO_TEST_CASE(RegexBranch) {
            test_query(L"regex:一|与|业|严|丫|丿|么|义|乐|乙|也|予|于|亏|云|亚|亦|亿|仡|以|仪|仰|伊|众|优|伛|伢|但|佑|佗|余|佚|佣|佯|佻|佾|侑|依|侥|俑|俞|俣|俨|俺|倚|偃|允|元|兖|养|冤|冶|刈|刖|剡|劓|勇|匀|医|卣|印|厂|压|厌|原|厣|又|友|台|右|叶|叹|吁|吆|吖|吟|听|吲|吾|呀|呓|员|呦|咏|咦|咬|咽|咿|哑|哕|哟|哩|哽|唁|唷|喁|喑|喝|喻|嗌");
        }

        // very slow
        BOOST_AUTO_TEST_CASE(Branch) {
            test_query(L"一|与|业|严|丫|丿|么|义|乐|乙|也|予|于|亏|云|亚|亦|亿|仡|以|仪|仰|伊|众|优|伛|伢|但|佑|佗|余|佚|佣|佯|佻|佾|侑|依|侥|俑|俞|俣|俨|俺|倚|偃|允|元|兖|养|冤|冶|刈|刖|剡|劓|勇|匀|医|卣|印|厂|压|厌|原|厣|又|友|台|右|叶|叹|吁|吆|吖|吟|听|吲|吾|呀|呓|员|呦|咏|咦|咬|咽|咿|哑|哕|哟|哩|哽|唁|唷|喁|喑|喝|喻|嗌");
        }

    BOOST_AUTO_TEST_SUITE_END()


    BOOST_AUTO_TEST_SUITE(Pinyin)

        BOOST_AUTO_TEST_CASE(Base_v03) {
            test_query(L"regex:[y" DYNAMIC_CHARSET_YY_1 L"][y" DYNAMIC_CHARSET_YY_2 L"]");
        }

        // slower than Base_v02
        BOOST_AUTO_TEST_CASE(LetterBranch) {
            // regex:(y|[英])(y|[语])
            test_query(L"regex:(y|[" DYNAMIC_CHARSET_YY_1 L"])(y|[" DYNAMIC_CHARSET_YY_2 L"])");
        }

        BOOST_AUTO_TEST_CASE(LetterBranch_1) {
            // regex:(?:y|[英])(?:y|[语])
            test_query(L"regex:(?:y|[" DYNAMIC_CHARSET_YY_1 L"])(?:y|[" DYNAMIC_CHARSET_YY_2 L"])");
        }

        BOOST_AUTO_TEST_CASE(LetterBranch_2) {
            // regex:(?>y|[英])(?>y|[语])
            test_query(L"regex:(?>y|[" DYNAMIC_CHARSET_YY_1 L"])(?>y|[" DYNAMIC_CHARSET_YY_2 L"])");
        }

        BOOST_AUTO_TEST_CASE(Base_v02) {
            test_query(L"case:regex:[yY" DYNAMIC_CHARSET_YY_1 L"][yY" DYNAMIC_CHARSET_YY_2 L"]");
        }

        BOOST_AUTO_TEST_CASE(DynamicPair) {
            test_query(L"regex:[y" DYNAMIC_CHARSET_YY_1 L"][y" DYNAMIC_CHARSET_YY_2 L"]");
        }

        BOOST_AUTO_TEST_CASE(Dynamic) {
            test_query(L"regex:[y" DYNAMIC_CHARSET_Y L"]{2}");
        }

        BOOST_AUTO_TEST_CASE(Base_v01) {
            test_query(L"regex:[y" + pinyin_regex + L"]{2}");
        }

        BOOST_AUTO_TEST_CASE(Quantifier) {
            test_query(L"regex:[y" + pinyin_regex + L"]{2}");
        }
        
        BOOST_AUTO_TEST_CASE(Repeat) {
            test_query(L"regex:[y" + pinyin_regex + L"][y" + pinyin_regex + L"]");
        }

    BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()


BOOST_FIXTURE_TEST_SUITE(DynamicCharset, Everythings::Everything)
    void test_query(std::wstring_view query) {
        using namespace Everythings;

        Everything ev(L"1.5a");
        DWORD t = timeGetTime();
        ev.query_send(query, Search::MatchCase, Request::FileName);
        QueryResults results = ev.query_get();
        t = timeGetTime() - t;
        
        BOOST_TEST_MESSAGE(results.available_num << ", " << t << "ms");
        Sleep(2000);
    }

    BOOST_AUTO_TEST_CASE(GenerateQuery) {
        test_query(L"regex:[〇-𰻞]");
    }

    BOOST_AUTO_TEST_CASE(UpdateQuery) {
        test_query(L"rc:");
    }

    BOOST_AUTO_TEST_CASE(UpdateQueryRegex) {
        test_query(L"rc: regex:[〇-𰻞]");
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()