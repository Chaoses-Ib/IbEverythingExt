#include "pch.h"
#include <string_view>
#include "helper.hpp"
#include "pinyin.hpp"

constexpr wchar_t prop_edit_content[] = L"IbEverythingExt.Content";
constexpr wchar_t prop_edit_processed_content[] = L"IbEverythingExt.ProcessedContent";
constexpr wchar_t prop_main_title[] = L"IbEverythingExt.Title";

bool is_modifier_in_blacklist(std::wstring_view modifier) {
    using namespace std::literals;

    static constexpr std::wstring_view blacklist[]{
        L"attrib:"sv,
        L"attributes:"sv,
        L"bitdepth:"sv,
        L"case:"sv,
        L"childcount:"sv,
        L"childfilecount:"sv,
        L"childfoldercount:"sv,
        L"count:"sv,
        L"da:"sv,
        L"dateaccessed:"sv,
        L"datecreated:"sv,
        L"datemodified:"sv,
        L"daterun:"sv,
        L"dc:"sv,
        L"dimensions:"sv,
        L"dm:"sv,
        L"dr:"sv,
        L"ext:"sv,
        L"frn:"sv,
        L"fsi:"sv,
        L"height:"sv,
        L"len:"sv,
        L"orientation:"sv,
        L"parents:"sv,
        L"rc:"sv,
        L"recentchange:"sv,
        L"runcount:"sv,
        L"size:"sv,
        L"track:"sv,
        L"type:"sv,
        L"width:"sv
    };
    return std::ranges::binary_search(blacklist, modifier);
}

// #TODO: may be buggy, need tests
std::wstring* edit_process_content(const std::wstring& content) {
    using namespace std::literals;

    // ignore empty and single-char contents
    if (content.size() <= 1) {
        return new std::wstring(content);
    }

    std::wistringstream in{ content };  // (content.data(), content.size()) is not what we want
    std::wostringstream out;

    bool disabled = false;
    struct {
        bool endwith = false;
        bool startwith = false;
        bool nowholefilename = false;
        bool nowildcards = false;
    } modifiers;

    auto process_keyword = [&out, &disabled, &modifiers](const std::wstring& keyword) {
        if (disabled || keyword.empty()) {
            disabled = false;
            out << keyword;
            return;
        }

        std::wstringstream keyword_in(keyword);
        wchar_t c;

        // check whether wildcards exist
        bool wildcards = false;
        if (!modifiers.nowildcards) {
            bool escape = false;
            bool skip_escape_reset = false;
            while (!wildcards && keyword_in.get(c)) {
                switch (c) {
                case L'\\':
                    escape = true;
                    skip_escape_reset = true;
                    break;
                case L'*':
                case L'?':
                    if (!escape)
                        wildcards = true;
                }
                if (skip_escape_reset)
                    skip_escape_reset = false;
                else
                    escape = false;
            }
        }
        keyword_in.clear();
        keyword_in.seekg(0);

        out << L"regex:";
        if (modifiers.startwith || (wildcards && !modifiers.nowildcards && !modifiers.nowholefilename)) {
            out << L'^';
        }

        bool escape = false;
        wchar_t last_letter = L'\0';
        int letter_count = 0;
        while (keyword_in.get(c)) {
            if (escape) {
                out << c;
                escape = false;
            } else {
                if (L'a' <= c && c <= L'z') {  // only process lowercase letters
                    // merge the same letters
                    if (last_letter == c) {
                        letter_count += 1;
                    }
                    else {
                        if (letter_count >= 1) {
                            out << pinyin_regexs[last_letter - 'a'];
                            if (letter_count > 1)
                                out << L'{' << letter_count << L'}';
                        }

                        last_letter = c;
                        letter_count = 1;
                    }
                }
                else {
                    if (letter_count >= 1) {
                        out << pinyin_regexs[last_letter - 'a'];
                        if (letter_count > 1)
                            out << L'{' << letter_count << L'}';

                        last_letter = L'\0';
                        letter_count = 0;
                    }

                    switch (c) {
                    case L'\\':
                        escape = true;
                        out << c;
                        break;

                    // escape regex chars
                    case L'.':
                    case L'[':
                    case L']':
                    case L'^':
                    case L'$':
                    case L'+':
                    case L'{':
                    case L'}':
                    case L'(':
                    case L')':
                        out << L'\\' << c;
                        break;

                    // wildcards to regex
                    case L'*':
                        if (!modifiers.nowildcards)
                            out << L".*";
                        else
                            out << L'\\' << c;
                        break;
                    case L'?':
                        if (!modifiers.nowildcards)
                            out << L".";
                        else
                            out << L'\\' << c;
                        break;

                    default:
                        out << c;
                    }
                }
            }
        }
        // stream EOF
        if (letter_count >= 1) {
            out << pinyin_regexs[last_letter - 'a'];
            if (letter_count > 1)
                out << L'{' << letter_count << L'}';
        }

        if (modifiers.endwith || (wildcards && !modifiers.nowildcards && !modifiers.nowholefilename)) {
            out << L'$';
        }
        out << L' ';
    };

    wchar_t c;
    bool escape = false;
    bool skip_escape_reset = false;
    bool in_quotes = false;
    std::wistringstream::pos_type last_colon_next{};
    while (in.get(c)) {
        switch (c) {
        case L'\\':
            escape = true;
            skip_escape_reset = true;
            break;
        case L'"':
            if (!escape)
                in_quotes = !in_quotes;
            break;
        case L':':
            if (!in_quotes) {
                // modifier/function

                auto colon_next = in.tellg();
                size_t size = colon_next - last_colon_next;
                std::wstring modifier(size, L'\0');
                in.seekg(last_colon_next);
                in.read(modifier.data(), size);
                last_colon_next = colon_next;

                if (is_modifier_in_blacklist(modifier)) {
                    disabled = true;
                    out << modifier;
                }
                else if (modifier == L"endwith:"sv || modifier == L"startwith:"sv) {
                    if (modifier == L"endwith:"sv)
                        modifiers.endwith = true;
                    if (modifier == L"startwith:"sv)
                        modifiers.startwith = true;
                    // remove (these two will be implemented with regex)
                }
                else {
                    if (modifier == L"nowholefilename:"sv)
                        modifiers.nowholefilename = true;
                    else if (modifier == L"nowildcards:"sv)
                        modifiers.nowildcards = true;
                    out << modifier;
                }
            }
            break;
        case L' ':
            if (!in_quotes) {
                // keyword

                size_t size = in.tellg() - last_colon_next - 1;
                if (size) {  // not L"  "
                    std::wstring keyword(size, L'\0');
                    in.seekg(last_colon_next);
                    in.read(keyword.data(), size);

                    process_keyword(keyword);
                    modifiers = {};
                }
                last_colon_next = in.tellg();
            }
            break;
        }
        if (skip_escape_reset)
            skip_escape_reset = false;
        else
            escape = false;
    }
    // stream EOF
    in.clear();
    size_t size = in.tellg() - last_colon_next;
    std::wstring keyword(size, L'\0');
    in.seekg(last_colon_next);
    in.read(keyword.data(), size);
    process_keyword(keyword);

    return new std::wstring(out.str());
}

WNDPROC edit_window_proc_prev;
LRESULT CALLBACK edit_window_proc(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg) {
    case WM_GETTEXTLENGTH:
        {
            if constexpr (ib::debug_runtime)
                DebugOStream() << L"WM_GETTEXTLENGTH\n";

            // retrieve the content
            LRESULT result = CallWindowProcW(edit_window_proc_prev, hwnd, uMsg, wParam, lParam);
            wchar_t* buf = new wchar_t[result + 1 + std::size(" - Everything") - 1];  // for both the content and the title
            LRESULT content_len = CallWindowProcW(edit_window_proc_prev, hwnd, WM_GETTEXT, result + 1, (LPARAM)buf);

            // save the content
            auto content = (std::wstring*)GetPropW(hwnd, prop_edit_content);
            auto processed_content = (std::wstring*)GetPropW(hwnd, prop_edit_processed_content);
            if (content && *content == std::wstring_view(buf, content_len)) {
                delete[] buf;
                return processed_content->size();
            }
            delete content;
            delete processed_content;
            content = new std::wstring(buf, content_len);
            SetPropW(hwnd, prop_edit_content, content);

            // process the content
            processed_content = edit_process_content(*content);
            SetPropW(hwnd, prop_edit_processed_content, processed_content);

            if constexpr (ib::debug_runtime)
                DebugOStream() << *content << L" -> " << *processed_content << std::endl;


            // make the title
            if (content_len)
                std::copy_n(L" - Everything", std::size(L" - Everything"), buf + content_len);
            else
                std::copy_n(L"Everything", std::size(L"Everything"), buf);

            // save the title
            HWND main = GetAncestor(hwnd, GA_ROOT);
            delete[] (wchar_t*)GetPropW(main, prop_main_title);
            SetPropW(main, prop_main_title, buf);

            return processed_content->size();
        }
        break;
    case WM_GETTEXT:
        {
            if constexpr (ib::debug_runtime)
                DebugOStream() << L"WM_GETTEXT\n";

            if (auto processed_content = (std::wstring*)GetPropW(hwnd, prop_edit_processed_content)) {
                LRESULT n = min(processed_content->size() + 1, wParam);
                std::copy_n(processed_content->c_str(), n, (wchar_t*)lParam);
                return n - 1;
            }
        }
        break;
    }
    return CallWindowProcW(edit_window_proc_prev, hwnd, uMsg, wParam, lParam);
}

auto CreateWindowExW_real = CreateWindowExW;
HWND WINAPI CreateWindowExW_detour(
    _In_ DWORD dwExStyle,
    _In_opt_ LPCWSTR lpClassName,
    _In_opt_ LPCWSTR lpWindowName,
    _In_ DWORD dwStyle,
    _In_ int X,
    _In_ int Y,
    _In_ int nWidth,
    _In_ int nHeight,
    _In_opt_ HWND hWndParent,
    _In_opt_ HMENU hMenu,
    _In_opt_ HINSTANCE hInstance,
    _In_opt_ LPVOID lpParam)
{
    using namespace std::literals;

    HWND wnd = CreateWindowExW_real(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    if ((uintptr_t)lpClassName > 0xFFFF) {
        if (lpClassName == L"Edit"sv) {
            edit_window_proc_prev = (WNDPROC)SetWindowLongPtrW(wnd, GWLP_WNDPROC, (LONG_PTR)edit_window_proc);
        }
    }
    return wnd;
}

auto SetWindowTextW_real = SetWindowTextW;
BOOL WINAPI SetWindowTextW_detour(
    _In_ HWND hWnd,
    _In_opt_ LPCWSTR lpString)
{
    using namespace std::literals;

    wchar_t buf[std::size(L"EVERYTHING")];
    if (int len = GetClassNameW(hWnd, buf, std::size(buf))) {
        std::wstring_view sv(buf, len);

        if (sv == L"EVERYTHING"sv) {
            // set the title
            if (auto title = (const wchar_t*)GetPropW(hWnd, prop_main_title)) {
                SetWindowTextW_real(hWnd, title);
                return true;
            }
        } else if (sv == L"Edit"sv) {
            // prevent modifying edit
            return true;
        }
    }
    return SetWindowTextW_real(hWnd, lpString);
}

BOOL CALLBACK enum_window_proc(
    _In_ HWND hwnd,
    _In_ LPARAM lParam)
{
    using namespace std::literals;

    if (GetWindowThreadProcessId(hwnd, nullptr) == static_cast<DWORD>(lParam)) {
        // hwnd may be a window other than EVERYTHING, such as "#32770 (Dialog)" (Everything Options).
        wchar_t buf[std::size(L"EVERYTHING")];
        if (int len = GetClassNameW(hwnd, buf, std::size(buf))) {
            if (std::wstring_view(buf, len) == L"EVERYTHING"sv) {
                if (HWND toolbar = FindWindowExW(hwnd, 0, L"EVERYTHING_TOOLBAR", nullptr))
                    if (HWND edit = FindWindowExW(toolbar, 0, L"Edit", nullptr))
                        edit_window_proc_prev = (WNDPROC)SetWindowLongPtrW(edit, GWLP_WNDPROC, (LONG_PTR)edit_window_proc);
            }
        }
    }

    return true;
}

#include <IbDllHijackLib/Dlls/srvcli.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        IbDetourAttach(&CreateWindowExW_real, CreateWindowExW_detour);
        IbDetourAttach(&SetWindowTextW_real, SetWindowTextW_detour);

        // may be loaded after creating windows? it seems that only netutil.dll does
        //EnumWindows(enum_window_proc,GetCurrentThreadId());

        if constexpr (ib::debug_runtime)
            DebugOStream() << L"DLL_PROCESS_ATTACH\n";
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        IbDetourDetach(&SetWindowTextW_real, SetWindowTextW_detour);
        IbDetourDetach(&CreateWindowExW_real, CreateWindowExW_detour);

        if constexpr (ib::debug_runtime)
            DebugOStream() << L"DLL_PROCESS_DETACH\n";
        break;
    }
    return TRUE;
}