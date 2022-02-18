#include "pch.h"
#include "helper.hpp"
#include "config.hpp"
#include "ipc.hpp"
#include "PinyinSearch.hpp"
#include "quick_select.hpp"

std::wstring instance_name{};
std::wstring class_everything = L"EVERYTHING";

std::unique_ptr<PinyinSearch> pinyin_search;

/*
WNDPROC list_window_proc_0;
LRESULT CALLBACK list_window_proc_1(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg) {
    }
    return CallWindowProcW(list_window_proc_0, hwnd, uMsg, wParam, lParam);
}
*/

WNDPROC list_window_proc_2;
LRESULT CALLBACK list_window_proc_3(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    return quick::list_window_proc(list_window_proc_2, hwnd, uMsg, wParam, lParam);
}

auto SetWindowLongPtrW_real = SetWindowLongPtrW;
LONG_PTR WINAPI SetWindowLongPtrW_detour(
    _In_ HWND hWnd,
    _In_ int nIndex,
    _In_ LONG_PTR dwNewLong)
{
    using namespace std::literals;

    if (nIndex == GWLP_WNDPROC) {
        wchar_t buf[256];
        if (int len = GetClassNameW(hWnd, buf, std::size(buf))) {
            std::wstring_view class_name(buf, len);
            /*
            if (class_name == L"Edit"sv) {
                if (int len = GetClassNameW(GetParent(hWnd), buf, std::size(buf));
                    std::wstring_view(buf, len) == L"EVERYTHING_TOOLBAR"sv)
                {
                    edit_window_proc_0 = (WNDPROC)SetWindowLongPtrW_real(hWnd, nIndex, (LONG_PTR)edit_window_proc_3);
                    edit_window_proc_2 = (WNDPROC)dwNewLong;
                    return (LONG_PTR)edit_window_proc_1;
                }
            }
            */
            if (class_name == L"SysListView32"sv) {
                if (int len = GetClassNameW(GetParent(hWnd), buf, std::size(buf));
                    std::wstring_view(buf, len) == class_everything)
                {
                    // the caller could be Everything.exe or comctl32.dll
                    HMODULE module, everything;
                    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)_ReturnAddress(), &module);
                    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, nullptr, &everything);
                    if (module == everything) {
                        /*
                        list_window_proc_0 = (WNDPROC)SetWindowLongPtrW_real(hWnd, nIndex, (LONG_PTR)list_window_proc_3);
                        list_window_proc_2 = (WNDPROC)dwNewLong;
                        return (LONG_PTR)list_window_proc_1;
                        */
                        list_window_proc_2 = (WNDPROC)dwNewLong;
                        return (LONG_PTR)SetWindowLongPtrW_real(hWnd, nIndex, (LONG_PTR)list_window_proc_3);
                    }
                }
            }
        }
    }
    return SetWindowLongPtrW_real(hWnd, nIndex, dwNewLong);
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

    // EVERYTHING
    // EVERYTHING_TOOLBAR
    // Edit
    // SysListView32
    // msctls_statusbar32
    // ComboBox
    // EVERYTHING_PREVIEW

    HWND wnd = CreateWindowExW_real(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    if ((uintptr_t)lpClassName > 0xFFFF) {
        std::wstring_view class_name(lpClassName);
        if constexpr (debug)
            DebugOStream() << class_name << L", parent: " << hWndParent << L'\n';

        // named instances
        if (class_name.ends_with(L')') && instance_name.empty()) {
            size_t n = class_name.find(L"_("sv);
            if (n != class_name.npos) {
                size_t begin = n + std::size(L"_("sv);
                instance_name = class_name.substr(begin, class_name.size() - 1 - begin);

                class_everything += class_name.substr(n);
            }
        }

        if (class_name == class_everything) {
            if (config.pinyin_search.enable)
                pinyin_search->everything_created();
        } else if (class_name == L"Edit"sv) {
            if (config.pinyin_search.enable) {
                wchar_t buf[std::size(L"EVERYTHING_TOOLBAR")];
                if (int len = GetClassNameW(hWndParent, buf, std::size(buf))) {
                    if (std::wstring_view(buf, len) == L"EVERYTHING_TOOLBAR"sv) {
                        pinyin_search->edit_created(wnd);
                    }
                }
            }
        } else if (class_name == L"SysListView32"sv) {
            if (config.quick_select.enable) {
                wchar_t buf[256];
                if (int len = GetClassNameW(hWndParent, buf, std::size(buf))) {
                    if (std::wstring_view(buf, len) == class_everything) {
                        // find Edit
                        HWND toolbar = FindWindowExW(hWndParent, nullptr, L"EVERYTHING_TOOLBAR", nullptr);
                        HWND edit = FindWindowExW(toolbar, nullptr, L"Edit", nullptr);

                        // create quick list
                        // X == Y == nWidth == nHeight == 0
                        HWND quick_list = quick::create_quick_list(hWndParent, wnd, edit, hInstance);
                    }
                }
            }
        } else if (class_name.starts_with(L"EVERYTHING_TASKBAR_NOTIFICATION"sv)) {
            ipc_init(instance_name);

            if (config.pinyin_search.enable)
                pinyin_search = make_pinyin_search(config.pinyin_search.mode, instance_name, wnd);
            if (config.quick_select.enable)
                quick::init();
        }
    }
    return wnd;
}

/*
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
                        edit_window_proc_0 = (WNDPROC)SetWindowLongPtrW(edit, GWLP_WNDPROC, (LONG_PTR)edit_window_proc_1);
            }
        }
    }

    return true;
}
*/

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

        if (!config_init())
            break;

        IbDetourAttach(&CreateWindowExW_real, CreateWindowExW_detour);
        if (config.quick_select.enable)
            IbDetourAttach(&SetWindowLongPtrW_real, SetWindowLongPtrW_detour);

        // may be loaded after creating windows? it seems that only netutil.dll does
        //EnumWindows(enum_window_proc, GetCurrentThreadId());
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        if constexpr (debug)
            DebugOStream() << L"DLL_PROCESS_DETACH\n";

        if (!config.enable)
            break;

        if (config.quick_select.enable)
            quick::destroy();
        if (config.pinyin_search.enable)
            pinyin_search.reset();
        
        ipc_destroy();

        if (config.quick_select.enable)
            IbDetourDetach(&SetWindowLongPtrW_real, SetWindowLongPtrW_detour);
        IbDetourDetach(&CreateWindowExW_real, CreateWindowExW_detour);

        config_destroy();
        break;
    }
    return TRUE;
}