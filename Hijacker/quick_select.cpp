#include "pch.h"
#include "quick_select.hpp"
#include <CommCtrl.h>
#include "helper.hpp"
#include "ipc.hpp"

extern decltype(&CreateWindowExW) CreateWindowExW_real;
//extern decltype(&SetWindowLongPtrW) SetWindowLongPtrW_real;

HHOOK keyboard_hook;
LRESULT CALLBACK keyboard_proc(
    _In_ int    code,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    bool down = !(lParam & 0xC0000000);
    if constexpr (debug_verbose)
        DebugOStream() << L"keyboard_proc: " << code << L", " << wParam << L", " << std::hex << lParam << L'(' << down << L')' << L'\n';

    do {
        static enum {
            None,
            Alt,
            AltCtrl,
            AltShift
        } hotkey;

        if (down) {
            if (bool alt = GetKeyState(VK_MENU) & 0x8000) {
                HWND focus = GetFocus();
                HWND list = (HWND)GetPropW(focus, prop_edit_list);
                if (!list) {  // not in Search Edit
                    HWND quick_list = (HWND)GetPropW(focus, prop_list_quick_list);
                    if (!quick_list)  // not in Result List
                        break;
                    list = focus;
                }

                int num;
                if ('0' <= wParam && wParam <= '9')
                    num = wParam == '0' ? 9 : wParam - '1';
                else if ('A' <= wParam && wParam <= 'Z')
                    num = wParam - 'A' + 10;
                else
                    break;

                bool ctrl = GetKeyState(VK_CONTROL) & 0x8000;
                bool shift = GetKeyState(VK_SHIFT) & 0x8000;
                if constexpr (debug)
                    DebugOStream() << (ctrl ? L"Ctrl " : L"") << (shift ? L"Shift " : L"") << (alt ? L"Alt " : L"") << (wchar_t)wParam << L'\n';

                // these do not work, so we have to execute the hotkey when Alt is being releasing
                /*
                BYTE state[256];
                GetKeyboardState(state);
                state[VK_MENU] &= ~0x8000;
                SetKeyboardState(state);
                */
                //PostMessageW(GetFocus(), WM_SYSKEYUP, VK_MENU, 0xC0'38'0001);

                if (focus != list)
                    SetFocus(list);
                ListView_SetItemState(list, -1, 0, LVIS_SELECTED);
                ListView_SetItemState(list, ListView_GetTopIndex(list) + num, LVIS_SELECTED, LVIS_SELECTED);
                /*
                RECT rect;
                ListView_GetItemRect((HWND)GetPropW(list, prop_list_quick_list), num, &rect, LVIR_BOUNDS);
                LPARAM coord = rect.right | (rect.top + 10) << 16;
                PostMessageW(list, WM_LBUTTONDOWN, 0, coord);
                PostMessageW(list, WM_LBUTTONUP, 0, coord);
                */

                if (!ctrl && !shift)
                    hotkey = Alt;
                else if (ctrl && !shift)
                    hotkey = AltCtrl;
                else if (!ctrl && shift)
                    hotkey = AltShift;

                return true;
            }
        } else if (hotkey != None && code == HC_ACTION) {  // !down
            // the order of releasing is uncertain
            HWND list = GetFocus();
            switch (hotkey) {
            case Alt:
                if (wParam == VK_MENU) {
                    PostMessageW(list, WM_KEYDOWN, VK_RETURN, 0x00'1C'0001);
                    PostMessageW(list, WM_KEYUP, VK_RETURN, 0xC0'1C'0001);
                    hotkey = None;
                }
                break;
            case AltCtrl:
                if (wParam == VK_MENU) {
                    bool ctrl = GetKeyState(VK_CONTROL) & 0x8000;
                    if (!ctrl)
                        PostMessageW(list, WM_KEYDOWN, VK_CONTROL, 0x00'1D'0001);
                    PostMessageW(list, WM_KEYDOWN, VK_RETURN, 0x00'1C'0001);
                    PostMessageW(list, WM_KEYUP, VK_RETURN, 0xC0'1C'0001);
                    if (!ctrl)
                        PostMessageW(list, WM_KEYUP, VK_CONTROL, 0xC0'1D'0001);
                    hotkey = None;
                }
                break;
            case AltShift:
                if (wParam == VK_MENU && !(GetKeyState(VK_SHIFT) & 0x8000)
                    || wParam == VK_SHIFT && !(GetKeyState(VK_MENU) & 0x8000))
                {
                    //PostMessageW(list, WM_KEYDOWN, VK_APPS, 0x01'5D'0001);
                    //PostMessageW(list, WM_KEYUP, VK_APPS, 0xC1'5D'0001);
                    PostMessageW(list, WM_CONTEXTMENU, (WPARAM)list, -1);
                    hotkey = None;
                }
                break;
            }
        }
    } while (false);
    return CallNextHookEx(keyboard_hook, code, wParam, lParam);
}

constexpr int quick_list_width = 24;

auto SetWindowPos_real = SetWindowPos;
BOOL WINAPI SetWindowPos_detour(
    _In_ HWND hWnd,
    _In_opt_ HWND hWndInsertAfter,
    _In_ int X,
    _In_ int Y,
    _In_ int cx,
    _In_ int cy,
    _In_ UINT uFlags)
{
    using namespace std::literals;

    if (X == 0 && uFlags == SWP_NOACTIVATE | SWP_NOZORDER) {
        if (HWND quick_list = (HWND)GetPropW(hWnd, prop_list_quick_list)) {
            SetWindowPos_real(quick_list, hWndInsertAfter, X, Y, quick_list_width, cy, uFlags);

            X = quick_list_width;
            cx -= quick_list_width;
        }
    }
    return SetWindowPos_real(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

auto DeferWindowPos_real = DeferWindowPos;
HDWP WINAPI DeferWindowPos_detour(
    _In_ HDWP hWinPosInfo,
    _In_ HWND hWnd,
    _In_opt_ HWND hWndInsertAfter,
    _In_ int x,
    _In_ int y,
    _In_ int cx,
    _In_ int cy,
    _In_ UINT uFlags)
{
    if (y != 0 && uFlags == SWP_NOACTIVATE | SWP_NOREPOSITION | SWP_NOZORDER) {  // x is not 0 when there is a sidebar
        if (HWND quick_list = (HWND)GetPropW(hWnd, prop_list_quick_list)) {
            // it will turn into a gray block in some cases without SWP_FRAMECHANGED
            DeferWindowPos_real(hWinPosInfo, quick_list, hWndInsertAfter, x, y, quick_list_width, cy, SWP_FRAMECHANGED | uFlags);

            x += quick_list_width;
            cx -= quick_list_width;
        }
    }
    return DeferWindowPos_real(hWinPosInfo, hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
}

/*
auto EndDeferWindowPos_real = EndDeferWindowPos;
BOOL WINAPI EndDeferWindowPos_detour(_In_ HDWP hWinPosInfo) {
    bool result = EndDeferWindowPos_real(hWinPosInfo);
    return result;
}
*/

void quick_select_init() {
    if (ipc_version.major >= 1 && ipc_version.minor >= 5) {
        IbDetourAttach(&DeferWindowPos_real, DeferWindowPos_detour);
        //IbDetourAttach(&EndDeferWindowPos_real, EndDeferWindowPos_detour);
    }
    else
        IbDetourAttach(&SetWindowPos_real, SetWindowPos_detour);
    
    keyboard_hook = SetWindowsHookExW(WH_KEYBOARD, keyboard_proc, nullptr, GetCurrentThreadId());
}

void quick_select_destroy() {
    UnhookWindowsHookEx(keyboard_hook);

    if (ipc_version.major >= 1 && ipc_version.minor >= 5)
        IbDetourDetach(&DeferWindowPos_real, DeferWindowPos_detour);
    else
        IbDetourDetach(&SetWindowPos_real, SetWindowPos_detour);
}

WNDPROC quick_list_window_proc_prev;
LRESULT CALLBACK quick_list_window_proc(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg) {
    case WM_NCCALCSIZE:
    {
        // prevent scroll bars
        DWORD style = GetWindowLongW(hwnd, GWL_STYLE);

        DWORD new_style = style;
        if (new_style & WS_VSCROLL)
            new_style &= ~WS_VSCROLL;
        if (new_style & WS_HSCROLL)
            new_style &= ~WS_HSCROLL;

        if (new_style != style)
            SetWindowLongW(hwnd, GWL_STYLE, new_style);
    }
    }
    return CallWindowProcW(quick_list_window_proc_prev, hwnd, uMsg, wParam, lParam);
}

HWND quick_list_create(HWND parent, HINSTANCE instance) {
    HWND quick_list = CreateWindowExW_real(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, WC_LISTVIEWW, nullptr, WS_CHILD | WS_VISIBLE | LVS_REPORT, 0, 0, quick_list_width, 0, parent, nullptr, instance, nullptr);
    quick_list_window_proc_prev = (WNDPROC)SetWindowLongPtrW(quick_list, GWLP_WNDPROC, (LONG_PTR)quick_list_window_proc);

    wchar_t header[] = L"键";
    LVCOLUMNW column{
        .mask = LVCF_WIDTH | LVCF_TEXT,
        .cx = quick_list_width,
        .pszText = header,
        .cchTextMax = std::size(header) - 1
    };
    ListView_InsertColumn(quick_list, 0, &column);

    wchar_t s[] = L"1";
    LVITEM item{
        .mask = LVIF_TEXT,
        .iSubItem = 0,
        .pszText = s
    };

    for (; s[0] <= L'9'; s[0]++) {
        ListView_InsertItem(quick_list, &item);
        item.iItem++;
    }
    s[0] = L'0';
    ListView_InsertItem(quick_list, &item);
    item.iItem++;

    for (s[0] = L'A'; s[0] <= L'Z'; s[0]++) {
        ListView_InsertItem(quick_list, &item);
        item.iItem++;
    }

    return quick_list;
}