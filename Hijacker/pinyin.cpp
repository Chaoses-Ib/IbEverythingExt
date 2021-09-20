#include "pch.h"
#include "pinyin.hpp"
#include <mutex>
#include <IbEverythingLib/Everything.hpp>
#include "helper.hpp"

std::wstring pinyin_regexs[26] = {
L"[aA",
L"[bB",
L"[cC",
L"[dD",
L"[eE",
L"[fF",
L"[gG",
L"[hH",
L"[iI",
L"[jJ",
L"[kK",
L"[lL",
L"[mM",
L"[nN",
L"[oO",
L"[pP",
L"[qQ",
L"[rR",
L"[sS",
L"[tT",
L"[uU",
L"[vV",
L"[wW",
L"[xX",
L"[yY",
L"[zZ"
};

bool PinyinRange::has(char32_t c) const {
    return begin <= c && c <= end;
}

uint32_t& PinyinRange::get_flags(char32_t c) const {
    return table[c - begin];
}

void query_and_merge_into_pinyin_regexs() {
    using namespace Everythings;
    using namespace std::literals;

    static EverythingMT ev;
    static DWORD last_query = 0;

    static std::mutex mutex;  // be careful
    if (!mutex.try_lock())
        return;

    // query
    std::wstring query;
    if (!last_query) {
        query = L"regex:[〇-𰻞]";
        Sleep(3000);  //#TODO: wait for the database to be loaded?
        last_query = GetTickCount() / 1000;
    } else {
        DWORD time = GetTickCount() / 1000;
        query = L"rc:last" + std::to_wstring(time - last_query + 1) + L"secs regex:[〇-𰻞]";
        last_query = time;
    }
    QueryResults results = ev.query_send(query, Search::MatchCase, Request::FileName).get();

    // merge functions
    auto merge = [](wchar_t c) {
        for (PinyinRange range : pinyin_ranges) {
            if (range.has(c)) {
                if (uint32_t& flags = range.get_flags(c)) {
                    for (int i = 0; i < 26; i++) {
                        if (flags & 1 << i)
                            pinyin_regexs[i].push_back(c);
                    }
                    flags = 0;
                }
                break;
            }
        }
    };
    auto merge_pair = [](wchar_t c1, wchar_t c2) {
        char32_t c = 0x10000 + (c1 - 0xD800) << 10 + (c2 - 0xDC00);  // UTF-16 LE -> UTF-32

        for (PinyinRange range : pinyin_ranges) {
            if (range.has(c)) {
                if (uint32_t& flags = range.get_flags(c)) {
                    for (int i = 0; i < 26; i++) {
                        if (flags & 1 << i) {
                            pinyin_regexs[i].push_back(c1);
                            pinyin_regexs[i].push_back(c2);
                        }
                    }
                    flags = 0;
                }
                break;
            }
        }
    };

    // merge chars in results
    for (DWORD j = 0; j < results.available_num; j++) {
        std::wstring_view filename = results[j].get_str(Request::FileName);

        uint32_t i = 0;
        while (i < filename.size()) {
            wchar_t c = filename[i++];
            if (c <= 0x3007)
                continue;
            else if (0xD800 <= c && c <= 0xDBFF) {
                if (i < filename.size()) {
                    merge_pair(c, filename[i++]);
                }
            } else {
                merge(c);
            }
        }
    }

    mutex.unlock();

    if constexpr (ib::debug_runtime) {
        for (std::wstring& regex : pinyin_regexs) {
            DebugOStream() << regex << L"\n";
        }
    }
}