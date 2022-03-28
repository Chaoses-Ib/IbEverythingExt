#include "ipc_window.hpp"
#include <IbEverything/Everything.hpp>
#include "helper.hpp"

extern std::wstring* edit_process_content(const std::wstring& content);

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

            static DWORD last_pid = 0;  // window will be different every time when using the official SDK
            static bool last_known = false;
            DWORD pid;
            GetWindowThreadProcessId((HWND)wParam, &pid);
            if (pid == last_pid)
                known = last_known;
            else {
                HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);

                wchar_t path_buf[MAX_PATH];
                DWORD size = std::size(path_buf);
                QueryFullProcessImageNameW(process, 0, path_buf, &size);
                std::wstring_view path(path_buf, size);
                if constexpr (debug)
                    DebugOStream() << path << L'\n';

                if (path.ends_with(L"\\explorer.exe"))  // stnkl/EverythingToolbar
                    known = true;
                else if (path.ends_with(L"\\Flow.Launcher.exe"))  // Flow Launcher
                    known = true;
                else if (path.ends_with(L"\\HuoChat.exe"))  // HuoChat
                    known = true;
                else if (path.ends_with(L"\\uTools.exe"))  // uTools
                    known = true;
                else if (path.ends_with(L"\\Wox.exe"))  // Wox
                    known = true;

                CloseHandle(process);

                last_pid = pid;
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
                    if (query->search_flags & Search::Regex)
                        break;

                    std::wstring_view search{ query->search_string, (copydata->cbData - query->query_size()) / sizeof(wchar_t) - 1 };
                    return common_process(copydata->dwData, (uint8_t*)query, query->query_size(), search);
                }
                break;
            case EVERYTHING_IPC_COPYDATA_QUERY2A:
                {
                    EVERYTHING_IPC_QUERY2<char> *query = ib::Addr(copydata->lpData);
                    if (query->search_flags & Search::Regex)
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
                    if (query->search_flags & Search::Regex)
                        break;

                    std::wstring_view search{ query->search_string, (copydata->cbData - query->query_size()) / sizeof(wchar_t) - 1 };
                    return common_process(copydata->dwData, (uint8_t*)query, query->query_size(), search);
                }
                break;
            case EVERYTHING_IPC_COPYDATAQUERYA:
                {
                    EVERYTHING_IPC_QUERY<char> *query = ib::Addr(copydata->lpData);
                    if (query->search_flags & Search::Regex)
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

void ipc_window_init(HWND ipc_window) {
    ipc_window_proc_prev = (WNDPROC)SetWindowLongPtrW(ipc_window, GWLP_WNDPROC, (LONG_PTR)ipc_window_proc);
}

void ipc_window_destroy() {}