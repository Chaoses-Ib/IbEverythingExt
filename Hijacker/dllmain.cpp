#include "pch.h"
#include <unordered_map>
#include <string_view>
#include <thread>
#include "helper.hpp"
#include "pinyin.hpp"
#include "search_history.hpp"
#include <IbEverythingLib/Everything.hpp>

constexpr wchar_t prop_edit_content[] = L"IbEverythingExt.Content";
constexpr wchar_t prop_edit_processed_content[] = L"IbEverythingExt.ProcessedContent";
constexpr wchar_t prop_main_title[] = L"IbEverythingExt.Title";

std::unordered_map<std::wstring, std::wstring> content_map{};

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

    if (content.size() <= 1) {  // ignore empty and single-char contents
        return new std::wstring(content);
    } else if (content.starts_with(L"nopy:"sv)) {
        return new std::wstring(content, L"nopy:"sv.size());
    }

    std::wistringstream in{ content };  // (content.data(), content.size()) is not what we want
    std::wostringstream out;

    bool disabled = false;
    struct {
        bool py = false;
        bool endwith = false;
        bool startwith = false;
        bool nowholefilename = false;
        bool nowildcards = false;
    } modifiers;

    auto process_keyword = [&out, &disabled, &modifiers](const std::wstring_view& keyword) {
        if (disabled || keyword.empty()) {
            disabled = false;
            if (modifiers.startwith)
                out << L"startwith:";
            if (modifiers.endwith)
                out << L"endwith:";
            out << keyword << L' ';
            return;
        }

        // check whether wildcards exist
        bool wildcards = false;
        if (!modifiers.nowildcards) {
            bool escape = false;
            uint32_t i = 0;
            while (!wildcards && i < keyword.size()) {
                wchar_t c = keyword[i++];

                if (escape) {
                    escape = false;
                    continue;
                }

                switch (c) {
                case L'\\':
                    escape = true;
                    break;
                case L'*':
                case L'?':
                    wildcards = true;
                    break;
                }
            }
        }

        out << L"regex:";

        /*
        // keyword must be quoted if you use branch in the group of your regex
        bool quoted = keyword[0] == L'"';
        if (!quoted)
            out << L'"';
        */

        if (modifiers.startwith || (wildcards && !modifiers.nowildcards && !modifiers.nowholefilename)) {
            out << L'^';
        }

        // pinyin output functions
        auto output_pinyin_common = [&out, &modifiers](wchar_t cur, std::wstring_view regex) {
            out << L'[';
            if (!modifiers.py)
                out << cur;
            out << regex << L']';
        };
        auto output_pinyin = [&output_pinyin_common](wchar_t cur) {
            output_pinyin_common(cur, pinyin_regexs[cur - L'a']);
        };
        auto output_pinyin_next = [&output_pinyin_common](wchar_t cur, wchar_t next) {
            output_pinyin_common(cur, pinyin_pair_regexs[cur - L'a'][next - L'a'].first);
        };
        auto output_pinyin_last = [&output_pinyin_common](wchar_t last, wchar_t cur) {
            output_pinyin_common(cur, pinyin_pair_regexs[last - L'a'][cur - L'a'].second);
        };
        auto output_pinyin_last_next = [&out, &modifiers](wchar_t last, wchar_t cur, wchar_t next) {
            out << L'[';
            if (!modifiers.py)
                out << cur;

            std::wstring& first = pinyin_pair_regexs[cur - L'a'][next - L'a'].first;
            std::wstring& second = pinyin_pair_regexs[last - L'a'][cur - L'a'].second;

            // intersect
            uint32_t i = 0;
            while (i < first.size()) {
                wchar_t c = first[i++];

                Utf16Pair cur;
                if (0xD800 <= c && c <= 0xDBFF) {
                    if (i < first.size())
                        cur = { c, first[i++] };
                    else
                        break;
                } else {
                    cur = { c };
                }

                if (cur.in(second)) {
                    out << cur.l;
                    if (cur.h)
                        out << cur.h;
                }
            }

            out << L']';
        };

        wchar_t last = L'\0';
        bool escape = false;
        uint32_t i = 0;
        while (i < keyword.size()) {
            wchar_t c = keyword[i++];

            if (escape) {
                escape = false;

                // only \! is escaped
                if (c == L'!') {
                    out << c;
                    continue;
                }
                out << L'\\';
            }
            if (L'a' <= c && c <= L'z') {  // only process lowercase letters
                wchar_t next = L'\0';
                if (i < keyword.size()) {
                    next = keyword[i];
                    if (!(L'a' <= next && next <= L'z'))
                        next = L'\0';
                }

                if (next) {
                    if (last)
                        output_pinyin_last_next(last, c, next);
                    else
                        output_pinyin_next(c, next);
                } else {
                    if (last)
                        output_pinyin_last(last, c);
                    else
                        output_pinyin(c);
                }
                last = c;
            } else if (L'A' <= c && c <= L'Z') {
                last = static_cast<wchar_t>(c - 'A' + 'a');
                out << c;
            } else {
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

                last = L'\0';
            }
        }
        if (escape)  //R"(abc\)"
            out << L'\\';

        if (modifiers.endwith || (wildcards && !modifiers.nowildcards && !modifiers.nowholefilename)) {
            out << L'$';
        }

        /*
        if (!quoted)
            out << L'"';
        */

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

                if (disabled || is_modifier_in_blacklist(modifier)) {
                    disabled = true;
                    out << modifier;
                }
                else if (modifier == L"py:"sv || modifier == L"endwith:"sv || modifier == L"startwith:"sv) {
                    if (modifier == L"py:"sv)
                        modifiers.py = true;
                    else if (modifier == L"endwith:"sv)
                        modifiers.endwith = true;
                    else if (modifier == L"startwith:"sv)
                        modifiers.startwith = true;
                    // remove (these modifiers will be implemented with regex)
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
            if constexpr (debug)
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

            if constexpr (debug)
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
            if constexpr (debug)
                DebugOStream() << L"WM_GETTEXT\n";

            if (auto processed_content = (std::wstring*)GetPropW(hwnd, prop_edit_processed_content)) {
                LRESULT n = min(processed_content->size() + 1, wParam);
                std::copy_n(processed_content->c_str(), n, (wchar_t*)lParam);
                return n - 1;
            }
        }
        break;
    case WM_KILLFOCUS:
        {
            if constexpr (debug)
                DebugOStream() << L"WM_KILLFOCUS\n";

            // save the content to map
            if (auto content = (std::wstring*)GetPropW(hwnd, prop_edit_content)) {
                auto processed_content = (std::wstring*)GetPropW(hwnd, prop_edit_processed_content);
                content_map[*processed_content] = *content;
            }
        }
        break;
    }
    return CallWindowProcW(edit_window_proc_prev, hwnd, uMsg, wParam, lParam);
}

#pragma pack(push, 1)
template <typename CharT>
struct EVERYTHING_IPC_QUERY2 {
    // something wrong with MSVC
    //using namespace Everythings;

    DWORD reply_hwnd;  // !: not sizeof(HWND)
    DWORD reply_copydata_message;
    Everythings::SearchFlags search_flags;
    DWORD offset;
    DWORD max_results;
    Everythings::RequestFlags request_flags;
    Everythings::Sort sort_type;
    CharT search_string[1];  // '\0'
    
    static size_t query_size() {
        return sizeof(EVERYTHING_IPC_QUERY2) - sizeof(CharT);
    }
};

template <typename CharT>
struct EVERYTHING_IPC_QUERY {
    // something wrong with MSVC
    //using namespace Everythings;

    DWORD reply_hwnd;  // !: not sizeof(HWND)
    DWORD reply_copydata_message;
    Everythings::SearchFlags search_flags;
    DWORD offset;
    DWORD max_results;
    CharT search_string[1];  // '\0'

    static size_t query_size() {
        return sizeof(EVERYTHING_IPC_QUERY) - sizeof(CharT);
    }
};
#pragma pack(pop)

WNDPROC ipc_window_proc_prev;
LRESULT CALLBACK ipc_window_proc(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    using namespace Everythings;

    switch (uMsg) {
    case WM_COPYDATA:
        {
            if constexpr (debug)
                DebugOStream() << L"WM_COPYDATA\n";
            if (!wParam)
                break;

            bool known = false;

            static HWND last_window = nullptr;
            static bool last_known = false;
            HWND window = (HWND)wParam;
            if (window == last_window)
                known = last_known;
            else {
                DWORD pid;
                GetWindowThreadProcessId((HWND)wParam, &pid);
                HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);

                wchar_t path_buf[MAX_PATH];
                DWORD size = std::size(path_buf);
                QueryFullProcessImageNameW(process, 0, path_buf, &size);
                std::wstring_view path(path_buf, size);
                if constexpr (debug)
                    DebugOStream() << path << L'\n';

                if (path.ends_with(L"\\explorer.exe"))  // stnkl/EverythingToolbar
                    known = true;
                else if (path.ends_with(L"\\uTools.exe"))  // uTools
                    known = true;

                CloseHandle(process);

                last_window = window;
                last_known = known;
            }
            if (!known)
                break;

            constexpr uintptr_t EVERYTHING_IPC_COPYDATAQUERYA = 1;
            constexpr uintptr_t EVERYTHING_IPC_COPYDATAQUERYW = 2;
            constexpr uintptr_t EVERYTHING_IPC_COPYDATA_QUERY2A = 17;
            constexpr uintptr_t EVERYTHING_IPC_COPYDATA_QUERY2W = 18;
            auto common_process = [hwnd, uMsg, wParam](uintptr_t command, uint8_t* query, size_t query_size, std::wstring_view search) {
                std::unique_ptr<std::wstring> processed_search{ edit_process_content(std::wstring(search)) };

                size_t size = query_size + processed_search->size() * sizeof(wchar_t) + sizeof L'\0';
                auto buf = std::make_unique<uint8_t[]>(size);
                std::copy_n(query, query_size, buf.get());
                processed_search->copy((wchar_t*)(buf.get() + query_size), processed_search->size() + 1);

                COPYDATASTRUCT processed_copydata{
                    command,
                    size,
                    (PVOID)buf.get()
                };

                return CallWindowProcW(ipc_window_proc_prev, hwnd, uMsg, wParam, (LPARAM)&processed_copydata);
            };
            COPYDATASTRUCT* copydata = ib::Addr(lParam);
            switch (copydata->dwData) {
            case EVERYTHING_IPC_COPYDATA_QUERY2W:
                {
                    EVERYTHING_IPC_QUERY2<wchar_t> *query = ib::Addr(copydata->lpData);
                    if (query->search_flags & Search::MatchCase || query->search_flags & Search::Regex)
                        break;

                    std::wstring_view search{ query->search_string, (copydata->cbData - query->query_size()) / sizeof(wchar_t) - 1 };
                    return common_process(copydata->dwData, (uint8_t*)query, query->query_size(), search);
                }
                break;
            case EVERYTHING_IPC_COPYDATA_QUERY2A:
                {
                    EVERYTHING_IPC_QUERY2<char> *query = ib::Addr(copydata->lpData);
                    if (query->search_flags & Search::MatchCase || query->search_flags & Search::Regex)
                        break;

                    size_t search_len = copydata->cbData - query->query_size() - sizeof(char);
                    auto search_buf = std::make_unique<wchar_t[]>(search_len);

                    std::wstring_view search{ search_buf.get(), (size_t)MultiByteToWideChar(CP_ACP, 0, query->search_string, search_len, search_buf.get(), search_len) };
                    return common_process(EVERYTHING_IPC_COPYDATA_QUERY2W, (uint8_t*)query, query->query_size(), search);
                }
                break;
            case EVERYTHING_IPC_COPYDATAQUERYW:
                {
                    EVERYTHING_IPC_QUERY<wchar_t> *query = ib::Addr(copydata->lpData);
                    if (query->search_flags & Search::MatchCase || query->search_flags & Search::Regex)
                        break;

                    std::wstring_view search{ query->search_string, (copydata->cbData - query->query_size()) / sizeof(wchar_t) - 1 };
                    return common_process(copydata->dwData, (uint8_t*)query, query->query_size(), search);
                }
                break;
            case EVERYTHING_IPC_COPYDATAQUERYA:
                {
                    EVERYTHING_IPC_QUERY<char> *query = ib::Addr(copydata->lpData);
                    if (query->search_flags & Search::MatchCase || query->search_flags & Search::Regex)
                        break;

                    size_t search_len = copydata->cbData - query->query_size() - sizeof(char);
                    auto search_buf = std::make_unique<wchar_t[]>(search_len);

                    std::wstring_view search{ search_buf.get(), (size_t)MultiByteToWideChar(CP_ACP, 0, query->search_string, search_len, search_buf.get(), search_len) };
                    return common_process(EVERYTHING_IPC_COPYDATAQUERYW, (uint8_t*)query, query->query_size(), search);
                }
                break;
            default:
                if constexpr (debug)
                    DebugOStream() << L"command: " << copydata->dwData << L'\n';
            }
        }
        break;
    }
    return CallWindowProcW(ipc_window_proc_prev, hwnd, uMsg, wParam, lParam);
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
        if (lpClassName == L"EVERYTHING"sv) {
            if constexpr (debug)
                DebugOStream() << L"EVERYTHING\n";

            std::thread t(query_and_merge_into_pinyin_regexs);
            t.detach();
        } else if (lpClassName == L"Edit"sv) {
            wchar_t buf[std::size(L"EVERYTHING_TOOLBAR")];
            if (int len = GetClassNameW(hWndParent, buf, std::size(buf))) {
                if (std::wstring_view(buf, len) == L"EVERYTHING_TOOLBAR"sv) {
                    edit_window_proc_prev = (WNDPROC)SetWindowLongPtrW(wnd, GWLP_WNDPROC, (LONG_PTR)edit_window_proc);
                }
            }
        } else if (lpClassName == L"EVERYTHING_TASKBAR_NOTIFICATION"sv) {
            if constexpr (debug)
                DebugOStream() << L"EVERYTHING_TASKBAR_NOTIFICATION\n";

            ipc_window_proc_prev = (WNDPROC)SetWindowLongPtrW(wnd, GWLP_WNDPROC, (LONG_PTR)ipc_window_proc);

            std::thread t(query_and_merge_into_pinyin_regexs);
            t.detach();
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
            auto processed_content = (std::wstring*)GetPropW(hWnd, prop_edit_processed_content);
            if (processed_content && *processed_content == lpString) {
                // prevent modifying edit
                return true;
            }

            auto iter = content_map.find<std::wstring>(lpString);
            if (iter != content_map.end())
                return SetWindowTextW_real(hWnd, iter->second.c_str());
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

#include <IbDllHijackLib/Dlls/WindowsCodecs.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if constexpr (debug)
            DebugOStream() << L"DLL_PROCESS_ATTACH\n";

        IbDetourAttach(&CreateWindowExW_real, CreateWindowExW_detour);
        IbDetourAttach(&SetWindowTextW_real, SetWindowTextW_detour);

        // may be loaded after creating windows? it seems that only netutil.dll does
        //EnumWindows(enum_window_proc,GetCurrentThreadId());

        search_history_init();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        if constexpr (debug)
            DebugOStream() << L"DLL_PROCESS_DETACH\n";

        search_history_destroy();

        IbDetourDetach(&SetWindowTextW_real, SetWindowTextW_detour);
        IbDetourDetach(&CreateWindowExW_real, CreateWindowExW_detour);
        break;
    }
    return TRUE;
}