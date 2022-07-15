#include "PinyinSearchEdit.hpp"
#include <thread>
#include <unordered_map>
#include "helper.hpp"
#include "pinyin.hpp"
#include "search_history.hpp"
#include "ipc_window.hpp"

bool is_modifier_in_blacklist(std::wstring_view modifier) {
    using namespace std::literals;

    // must be sorted
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
        L"regex:"sv,
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
        if (keyword.empty())
            disabled = true;
        else if (keyword[0] == L'"') {
            if (keyword.size() >= 4 && keyword[2] == L':')  // '"C:"'
                disabled = true;
        } else {
            if (keyword.size() >= 2 && keyword[1] == L':')  // 'C:'
                disabled = true;
        }

        if (disabled) {
            disabled = false;
            if (modifiers.startwith)
                out << L"startwith:";
            if (modifiers.endwith)
                out << L"endwith:";
            out << keyword;
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
    };

    wchar_t c;
    bool in_quotes = false;
    std::wistringstream::pos_type last_colon_next{};
    while (in.get(c)) {
        switch (c) {
        case L'"':
            in_quotes = !in_quotes;
            break;
        case L':':
            if (!in_quotes) {
                // modifier/function

                auto colon_next = in.tellg();
                size_t size = colon_next - last_colon_next;
                if (size <= 2)  // "C:"
                    break;
                std::wstring modifier(size, L'\0');
                in.seekg(last_colon_next);
                in.read(modifier.data(), size);
                last_colon_next = colon_next;

                if (disabled || is_modifier_in_blacklist(modifier)) {
                    disabled = true;
                    out << modifier;
                } else if (modifier == L"py:"sv || modifier == L"endwith:"sv || modifier == L"startwith:"sv) {
                    if (modifier == L"py:"sv)
                        modifiers.py = true;
                    else if (modifier == L"endwith:"sv)
                        modifiers.endwith = true;
                    else if (modifier == L"startwith:"sv)
                        modifiers.startwith = true;
                    // remove (these modifiers will be implemented with regex)
                } else {
                    if (modifier == L"nowholefilename:"sv)
                        modifiers.nowholefilename = true;
                    else if (modifier == L"nowildcards:"sv)
                        modifiers.nowildcards = true;
                    out << modifier;
                }
            }
            break;
        case L'!':
            if (in.tellg() == last_colon_next + std::streamoff(1)) {
                out << c;
                last_colon_next += 1;
            }
            break;
        case L' ':
            if (!in_quotes) {
                // keyword

                size_t size = in.tellg() - last_colon_next - 1;
                std::wstring keyword(size, L'\0');
                if (size) {
                    in.seekg(last_colon_next);
                    in.read(keyword.data(), size);
                    in.ignore();  // ignore L' '
                }

                process_keyword(keyword);
                out << L' ';

                modifiers = {};
                last_colon_next = in.tellg();
            }
            break;
        }
    }
    // stream EOF
    in.clear();

    size_t size = in.tellg() - last_colon_next;
    std::wstring keyword(size, L'\0');
    in.seekg(last_colon_next);
    in.read(keyword.data(), size);

    process_keyword(keyword);

    // return the result
    if constexpr (debug)
        DebugOStream() << content << L" -> " << out.str() << std::endl;
    return new std::wstring(out.str());
}

constexpr wchar_t prop_main_title[] = L"IbEverythingExt.Title";
constexpr wchar_t prop_edit_content[] = L"IbEverythingExt.Content";
constexpr wchar_t prop_edit_processed_content[] = L"IbEverythingExt.ProcessedContent";

std::wstring title_suffix = L" - Everything";
std::unordered_map<std::wstring, std::wstring> content_map{};

WNDPROC edit_window_proc_0;
LRESULT CALLBACK edit_window_proc_1(
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
            LRESULT result = CallWindowProcW(edit_window_proc_0, hwnd, uMsg, wParam, lParam);
            wchar_t* buf = new wchar_t[result + 1 + title_suffix.size()];  // for both the content and the title
            LRESULT content_len = CallWindowProcW(edit_window_proc_0, hwnd, WM_GETTEXT, result + 1, (LPARAM)buf);

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

            // make the title
            if (content_len)
                std::copy_n(title_suffix.c_str(), title_suffix.size() + 1, buf + content_len);
            else
                std::copy_n(title_suffix.c_str() + std::size(L" - ") - 1, title_suffix.size() + 1 - (std::size(L" - ") - 1), buf);

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
    return CallWindowProcW(edit_window_proc_0, hwnd, uMsg, wParam, lParam);
}

/*
WNDPROC edit_window_proc_2;
LRESULT CALLBACK edit_window_proc_3(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if constexpr (debug)
            DebugOStream() << (uMsg == WM_KEYDOWN ? L"WM_KEYDOWN: " : L"WM_SYSKEYDOWN: ") << wParam << L'\n';
        break;
    }
    return CallWindowProcW(edit_window_proc_2, hwnd, uMsg, wParam, lParam);
}
*/

extern std::wstring class_everything;

auto SetWindowTextW_real = SetWindowTextW;
BOOL WINAPI SetWindowTextW_detour(
    _In_ HWND hWnd,
    _In_opt_ LPCWSTR lpString)
{
    using namespace std::literals;

    wchar_t buf[256];
    if (int len = GetClassNameW(hWnd, buf, std::size(buf))) {
        std::wstring_view sv(buf, len);

        if (sv == class_everything) {
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

PinyinSearchEdit::PinyinSearchEdit(const std::wstring& instance_name, HWND ipc_window) {
    using namespace std::literals;

    std::thread t(pinyin_query_and_merge);
    t.detach();
    
    title_suffix += L" ("sv;
    if (!instance_name.empty()) {
        title_suffix += instance_name;
        title_suffix += L", "sv;
    }
    title_suffix += L"Edit 模式)"sv;

    IbDetourAttach(&SetWindowTextW_real, SetWindowTextW_detour);

    search_history_init();

    ipc_window_init(ipc_window);
}

PinyinSearchEdit::~PinyinSearchEdit() {
    ipc_window_destroy();

    search_history_destroy();

    IbDetourDetach(&SetWindowTextW_real, SetWindowTextW_detour);
}

void PinyinSearchEdit::everything_created() {
    std::thread t(pinyin_query_and_merge);
    t.detach();
}

void PinyinSearchEdit::edit_created(HWND edit) {
    edit_window_proc_0 = (WNDPROC)SetWindowLongPtrW(edit, GWLP_WNDPROC, (LONG_PTR)edit_window_proc_1);
}