#include "pch.h"
#include "search_history.hpp"
#include <unordered_map>
#include <string>
#include "helper.hpp"

extern std::unordered_map<std::wstring, std::wstring> content_map;

auto WriteFile_real = WriteFile;
BOOL WINAPI WriteFile_detour(
    _In_ HANDLE hFile,
    _In_reads_bytes_opt_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToWrite,
    _Out_opt_ LPDWORD lpNumberOfBytesWritten,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
) {
    using namespace std::literals;

    do {
        // UTF-8
        if (nNumberOfBytesToWrite > sizeof "Search,Search Count,Last Search Date\r\n") {
            std::string_view sv(static_cast<const char*>(lpBuffer), nNumberOfBytesToWrite);
            if (!sv.starts_with("Search,Search Count,Last Search Date\r\n"sv))
                break;

            /*
            Search,Search Count,Last Search Date
            "a",4,132767094406609228
            "case:regex:""[aA][bB][cC]""[dD单到的]",2,132767108299260960
            */
            std::ostringstream out(std::ios_base::binary);  // may be longer than buffer (such as "a" -> "nopy:a")
            out << "Search,Search Count,Last Search Date\r\n"sv;

            std::unordered_map<std::string, uint32_t> search_map{};
            uint32_t i = "Search,Search Count,Last Search Date\r\n"sv.size();
            while (i < sv.size()) {
                if (sv[i++] != '"')
                    break;
                out << '"';

                // parse Search and convert processed content to raw content
                std::string search;
                while (i < sv.size()) {
                    char c = sv[i++];
                    if (c == '"') {
                        if (i < sv.size() && sv[i] == '"') {  // ""
                            search.push_back('"');
                            i++;
                        } else
                            break;
                    } else
                        search.push_back(c);
                }
                {
                    // search to search_u16
                    // std::wstring_convert is deprecated in C++ 17
                    std::wstring search_u16(search.size(), L'\0');
                    search_u16.resize(MultiByteToWideChar(CP_UTF8, 0, search.data(), search.size(), search_u16.data(), search_u16.size()));

                    // processed content -> content
                    auto iter = content_map.find<std::wstring>(search_u16);
                    if (iter != content_map.end()) {
                        // content to content_u8
                        std::string content_u8(iter->second.size() * 3, '\0');
                        int length = WideCharToMultiByte(CP_UTF8, 0, iter->second.data(), iter->second.size(), content_u8.data(), content_u8.size(), nullptr, nullptr);
                        if (!length) {
                            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                                break;
                            content_u8.resize(iter->second.size() * 4);
                            length = WideCharToMultiByte(CP_UTF8, 0, iter->second.data(), iter->second.size(), content_u8.data(), content_u8.size(), nullptr, nullptr);
                        }
                        content_u8.resize(length);

                        search = content_u8;
                    }
                }
                out << search << '"' << ',';
                i++;  // ','

                // parse Search Count and do sums
                uint32_t j = i;
                while (j < sv.size()) {
                    if (sv[j++] == ',')
                        break;
                }
                j--;
                uint32_t count = std::atoi(sv.substr(i, j - i).data());
                auto iter = search_map.find<std::string>(search);
                if (iter == search_map.end()) {
                    search_map[search] = count;
                } else {
                    count += iter->second;
                    search_map.erase(iter);
                    out << count;
                    i = j;
                }

                // the remaining
                while (i < sv.size()) {
                    char c = sv[i++];
                    out << c;
                    if (c == '\n')
                        break;
                }
            }

            const std::string& out_str = out.str();
            bool result = WriteFile_real(hFile, out_str.data(), out_str.size(), lpNumberOfBytesWritten, lpOverlapped);
            if (*lpNumberOfBytesWritten == out_str.size())
                *lpNumberOfBytesWritten = nNumberOfBytesToWrite;  // required
            return result;
        }
    } while (false);

    return WriteFile_real(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

auto TextOutW_real = TextOutW;
BOOL WINAPI TextOutW_detour(_In_ HDC hdc, _In_ int x, _In_ int y, _In_reads_(c) LPCWSTR lpString, _In_ int c) {
    // TextOutW will only be called from:
    // DropdownList
    //   WindowFromDC() == NULL
    // Options: Fonts and Colors
    //   WindowFromDC() != NULL

    /*
    using namespace std::literals;
    
    static HDC last_hdc = 0;
    static bool is_dropdown_list = false;
    if (hdc != last_hdc) {  // there may be a collision?
        last_hdc = hdc;
        is_dropdown_list = false;

        HWND hwnd = WindowFromDC(hdc);
        wchar_t buf[std::size(L"EVERYTHING_DROPDOWNLIST")];
        if (int len = GetClassNameW(hwnd, buf, std::size(buf))) {
            if (std::wstring_view(buf, len) == L"EVERYTHING_DROPDOWNLIST"sv) {
                is_dropdown_list = true;
            }
        }
    }
    if (is_dropdown_list) {
    }
    */

    if (!WindowFromDC(hdc)) {
        std::wstring_view sv(lpString, c);
        if (c == 4096) {  // max length
            sv = std::wstring_view(lpString);  // #TODO: is it always zero-terminated?
        }

        auto iter = content_map.find<std::wstring>(std::wstring(sv));  // #TODO: directly use sv
        if (iter != content_map.end()) {
            return TextOutW_real(hdc, x, y, iter->second.c_str(), iter->second.size());
        }
    }
    return TextOutW_real(hdc, x, y, lpString, c);
}

void search_history_init() {
    IbDetourAttach(&WriteFile_real, WriteFile_detour);
    IbDetourAttach(&TextOutW_real, TextOutW_detour);
}

void search_history_destroy() {
    IbDetourDetach(&TextOutW_real, TextOutW_detour);
    IbDetourDetach(&WriteFile_real, WriteFile_detour);
}