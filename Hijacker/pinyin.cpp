#include "pch.h"
#include "pinyin.hpp"
#include <mutex>
#include <IbEverythingLib/Everything.hpp>
#include "helper.hpp"

std::wstring pinyin_regexs[26]{};
std::pair<std::wstring, std::wstring> pinyin_pair_regexs[26][26]{};

bool PinyinRange::has(char32_t c) const {
    return begin <= c && c <= end;
}

uint32_t& PinyinRange::get_flags(char32_t c) const {
    return table[c - begin];
}

bool Utf16Pair::in(std::wstring_view sv) const {
    if (!h) {
        return sv.find(l) != sv.npos;
    } else {
        size_t pos = sv.find(l);
        while (pos != sv.npos && pos != sv.size() - 1) {
            if (sv[pos + 1] == h)
                return true;
            pos = sv.find(l, pos + 1);
        }
        return false;
    }
}

struct Utf16PairOrd : Utf16Pair {
    char32_t ord;

    Utf16PairOrd() : Utf16Pair(), ord(0) {}
    Utf16PairOrd(wchar_t l) : Utf16Pair(l), ord(l) {}
    Utf16PairOrd(wchar_t l, wchar_t h) : Utf16Pair(l, h), ord(to_ord(l, h)) {}

protected:
    static char32_t to_ord(wchar_t l, wchar_t h) {
        return 0x10000 + ((l - 0xD800) << 10) + (h - 0xDC00);
    }
};

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
        ev.database_loaded_future().get();
        last_query = GetTickCount() / 1000;
    } else {
        DWORD time = GetTickCount() / 1000;
        query = L"rc:last" + std::to_wstring(time - last_query + 1) + L"secs regex:[〇-𰻞]";
        last_query = time;
    }
    QueryResults results = ev.query_send(query, Search::MatchCase, Request::FileName).get();

    // merge functions
    //"拼"
    auto merge = [](Utf16PairOrd pair) -> uint32_t {
        for (PinyinRange range : pinyin_ranges) {
            if (range.has(pair.ord)) {
                if (uint32_t& flags = range.get_flags(pair.ord)) {
                    if (!(flags & 1u << 31)) {
                        for (int i = 0; i < 26; i++) {
                            if (flags & 1 << i) {
                                pinyin_regexs[i].push_back(pair.l);
                                if (pair.h) pinyin_regexs[i].push_back(pair.h);
                            }
                        }
                        flags |= 1u << 31;
                    }
                    return flags & ~(1u << 31);
                }
                return 0;
            }
        }
        return 0;
    };
    //"拼音"
    auto merge_pair = [&merge](Utf16PairOrd pair1, Utf16PairOrd pair2, uint32_t py1) -> uint32_t {
        uint32_t py2 = merge(pair2);
        if (!py1 || !py2)
            return py2;
        for (int i = 0; i < 26; i++) {
            if (py1 & 1 << i) {
                for (int j = 0; j < 26; j++) {
                    if (py2 & 1 << j) {
                        auto& [first, second] = pinyin_pair_regexs[i][j];

                        if (!pair1.in(first)) {
                            first.push_back(pair1.l);
                            if (pair1.h) first.push_back(pair1.h);
                        }

                        if (!pair2.in(second)) {
                            second.push_back(pair2.l);
                            if (pair2.h) second.push_back(pair2.h);
                        }
                    }
                }
            }
        }
        return py2;
    };
    // "p音"
    auto merge_letter_hanzi = [&merge](wchar_t letter, Utf16PairOrd hanzi) -> uint32_t {
        uint32_t py = merge(hanzi);
        if (!py)
            return py;

        uint32_t i = letter - 'a';
        for (int j = 0; j < 26; j++) {
            if (py & 1 << j) {
                std::wstring& second = pinyin_pair_regexs[i][j].second;
                if (!hanzi.in(second)) {
                    second.push_back(hanzi.l);
                    if (hanzi.h) second.push_back(hanzi.h);
                }
            }
        }
        return py;
    };
    // "拼y"
    auto merge_hanzi_letter = [](Utf16PairOrd hanzi, wchar_t letter, uint32_t py) -> void {
        if (!py)
            return;

        uint32_t j = letter - 'a';
        for (int i = 0; i < 26; i++) {
            if (py & 1 << i) {
                std::wstring& first = pinyin_pair_regexs[i][j].first;
                if (!hanzi.in(first)) {
                    first.push_back(hanzi.l);
                    if (hanzi.h) first.push_back(hanzi.h);
                }
            }
        }
    };

    // merge chars in results
    for (DWORD j = 0; j < results.available_num; j++) {
        std::wstring_view filename = results[j].get_str(Request::FileName);

        Utf16PairOrd last_hanzi{};
        uint32_t last_py = 0;
        wchar_t last_letter = L'\0';
        uint32_t i = 0;
        while (i < filename.size()) {
            wchar_t c = filename[i++];
            if (c < 0x3007) {
                last_letter = L'\0';
                if (c <= L'z') {
                    if (c <= L'Z')
                        c += L'a' - L'A';
                    if (L'a' <= c) {  // [a-z]
                        if (last_hanzi.l) {
                            merge_hanzi_letter(last_hanzi, c, last_py);
                            last_hanzi = {};
                        }
                        last_letter = c;
                    }
                }
            } else {
                Utf16PairOrd cur;
                if (0xD800 <= c && c <= 0xDBFF) {
                    if (i < filename.size())
                        cur = { c, filename[i++] };
                    else
                        break;
                } else {
                    cur = { c };
                }
                
                if (last_letter) {
                    last_py = merge_letter_hanzi(last_letter, cur);
                    last_letter = L'\0';
                } else if (last_hanzi.l)
                    last_py = merge_pair(last_hanzi, cur, last_py);
                else
                    last_py = merge(cur);

                last_hanzi = cur;
            }
        }
    }

    mutex.unlock();

    if constexpr (debug) {
        for (uint32_t i = 0; i < std::size(pinyin_regexs); i++)
            DebugOStream() << static_cast<wchar_t>(L'a' + i) << L": " << pinyin_regexs[i] << L"\n";
        /*
        // may be very long
        for (uint32_t i = 0; i < std::size(pinyin_regexs); i++) {
            ib::DebugOStream dout = DebugOStream();
            for (uint32_t j = 0; j < std::size(pinyin_regexs); j++)
                 dout << static_cast<wchar_t>(L'a' + i) << static_cast<wchar_t>(L'a' + j) << L": "
                      << pinyin_pair_regexs[i][j].first << L' ' << pinyin_pair_regexs[i][j].second << L", ";
            dout << L'\n';
        }
        */
    }
}