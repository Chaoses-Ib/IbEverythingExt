#include "pch.h"
#include "quick_select.hpp"
#include <CommCtrl.h>
#include "helper.hpp"
#include "ipc.hpp"

constexpr wchar_t everything_prop_quick_list[] = L"IbEverythingExt.QuickList";
constexpr wchar_t edit_prop_list[] = L"IbEverythingExt.List";
constexpr wchar_t list_prop_quick_list[] = L"IbEverythingExt.QuickList";
struct QuickListData {
    HWND header;
    HWND list;
    HWND list_header;  // may be null
    int item_height;
    COLORREF background_color;
    HBRUSH background_brush;
    COLORREF text_color;

    /*
    COLORREF header_background_color;
    HBRUSH header_background_brush;
    */

    static constexpr int default_item_height = 21;
    static constexpr int left_padding = 4;
    static constexpr int left_key_padding = 6;
    static constexpr int right_padding = 4;

    void try_get_header() {
        list_header = ListView_GetHeader(list);
    }
};
constexpr wchar_t quick_list_prop[] = L"IbEverythingExt";
constexpr UINT quick_list_msg_update = WM_APP;

extern decltype(&CreateWindowExW) CreateWindowExW_real;
extern decltype(&SetWindowLongPtrW) SetWindowLongPtrW_real;

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
                HWND list = (HWND)GetPropW(focus, edit_prop_list);
                if (!list) {  // not in Search Edit
                    HWND quick_list = (HWND)GetPropW(focus, list_prop_quick_list);
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
        if (HWND quick_list = (HWND)GetPropW(hWnd, list_prop_quick_list)) {
            RECT rect;
            GetWindowRect(quick_list, &rect);
            int width = rect.right - rect.left;

            SetWindowPos_real(quick_list, hWndInsertAfter, X, Y, width, cy, uFlags);

            X = width;
            cx -= width;
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
        if (HWND quick_list = (HWND)GetPropW(hWnd, list_prop_quick_list)) {
            RECT rect;
            GetWindowRect(quick_list, &rect);
            int width = rect.right - rect.left;

            // it will turn into a gray block in some cases without SWP_FRAMECHANGED
            DeferWindowPos_real(hWinPosInfo, quick_list, hWndInsertAfter, x, y, width, cy, SWP_FRAMECHANGED | uFlags);

            x += width;
            cx -= width;
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

void quick::init() {
    if (ipc_version.major >= 1 && ipc_version.minor >= 5) {
        IbDetourAttach(&DeferWindowPos_real, DeferWindowPos_detour);
        //IbDetourAttach(&EndDeferWindowPos_real, EndDeferWindowPos_detour);
    }
    else
        IbDetourAttach(&SetWindowPos_real, SetWindowPos_detour);
    
    keyboard_hook = SetWindowsHookExW(WH_KEYBOARD, keyboard_proc, nullptr, GetCurrentThreadId());
}

void quick::destroy() {
    UnhookWindowsHookEx(keyboard_hook);

    if (ipc_version.major >= 1 && ipc_version.minor >= 5)
        IbDetourDetach(&DeferWindowPos_real, DeferWindowPos_detour);
    else
        IbDetourDetach(&SetWindowPos_real, SetWindowPos_detour);
}

WNDPROC everything_window_proc_prev;
LRESULT CALLBACK everything_window_proc(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg) {
    case WM_MEASUREITEM: {
        MEASUREITEMSTRUCT* measure = (MEASUREITEMSTRUCT*)lParam;
        if (measure->CtlType != ODT_LISTVIEW)
            break;

        HWND quick_list = (HWND)GetPropW(hwnd, everything_prop_quick_list);  // measure->CtlID == 0?
        QuickListData* data = (QuickListData*)GetPropW(quick_list, quick_list_prop);
        measure->itemHeight = data->item_height ? data->item_height : data->default_item_height;

        return true;
    }
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* draw = (DRAWITEMSTRUCT*)lParam;
        QuickListData* data;
        if (draw->CtlType != ODT_LISTVIEW || !(data = (QuickListData*)GetPropW(draw->hwndItem, quick_list_prop)))
            break;

        HDC dc = draw->hDC;

        FillRect(dc, &draw->rcItem, data->background_brush);
        
        /*
        wchar_t s;
        if (0 <= draw->itemID && draw->itemID < 9)
            s = L'1' + draw->itemID;
        else if (draw->itemID == 9)
            s = L'0';
        else if (draw->itemID < 10 + 26)
            s = L'A' + draw->itemID - 10;
        else
            return true;
        */
        wchar_t s[] = L"1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        if (draw->itemID >= std::size(s) - 1)
            return true;

        /*
        // fix the right (the column width is 0x7FFF)
        RECT quick_rect;
        GetWindowRect(draw->hwndItem, &quick_rect);
        draw->rcItem.right = draw->rcItem.left + (quick_rect.right - quick_rect.left);
        */
        draw->rcItem.left = data->left_key_padding;

        SetTextColor(dc, data->text_color);

        HFONT font = (HFONT)SendMessageW(data->list_header, WM_GETFONT, 0, 0);
        HGDIOBJ original_font = SelectObject(dc, font);
        DrawTextW(dc, &s[draw->itemID], 1, &draw->rcItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, original_font);

        return true;
    }
    }
    return CallWindowProcW(everything_window_proc_prev, hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK quick::list_window_proc(
    WNDPROC prev_proc,
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg) {
    case WM_PAINT: {
        HWND quick_list = (HWND)GetPropW(hwnd, list_prop_quick_list);
        // updating quick list by SetWindowPos or WM_SETFONT will cause the list to be a gray block,
        // so we update the list at another place
        PostMessageW(quick_list, quick_list_msg_update, 0, 0);
        break;
    }
    }
    return CallWindowProcW(prev_proc, hwnd, uMsg, wParam, lParam);
}

WNDPROC quick_header_window_proc_prev;
LRESULT CALLBACK quick_header_window_proc(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg) {
    case HDM_LAYOUT: /* adjust the height of the header */ {
        QuickListData* data = (QuickListData*)GetPropW(GetParent(hwnd), quick_list_prop);
        if (!data->list_header) data->try_get_header();

        // doesn't work as expected
        //return SendMessageW(data->list_header, uMsg, wParam, lParam);

        LRESULT r = CallWindowProcW(quick_header_window_proc_prev, hwnd, uMsg, wParam, lParam);
        if (!data->list_header)  // real case
            return r;

        HDLAYOUT* layout = (HDLAYOUT*)lParam;
        RECT list_header;
        GetWindowRect(data->list_header, &list_header);
        if (int height = list_header.bottom - list_header.top)  // necessary
            layout->pwpos->cy = layout->prc->top = height;

        return r;
    }
    }
    return CallWindowProcW(quick_header_window_proc_prev, hwnd, uMsg, wParam, lParam);
}

WNDPROC quick_list_window_proc_prev;
LRESULT CALLBACK quick_list_window_proc(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg) {
    case quick_list_msg_update: {
        QuickListData* data = (QuickListData*)GetPropW(hwnd, quick_list_prop);

        // adjust the height and width of items and the height of the header
        RECT item_rect;
        if (ListView_GetItemRect(data->list, 0, &item_rect, LVIR_BOUNDS)) {
            if (int new_item_height = item_rect.bottom - item_rect.top;
                new_item_height != data->item_height)
            {
                // adjust the height of items
                data->item_height = new_item_height;
                /*
                // trigger WM_MEASUREITEM
                RECT quick_rect;
                GetWindowRect(quick_list, &quick_rect);
                WINDOWPOS pos{
                    .hwnd = quick_list,
                    .cx = quick_rect.right - quick_rect.left,
                    .cy = quick_rect.bottom - quick_rect.top,
                    .flags = SWP_FRAMECHANGED | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOZORDER  // no SWP_NOSIZE
                };
                SendMessageW(quick_list, WM_WINDOWPOSCHANGED, 0, (WPARAM)&pos);
                */

                /*
                if (!data->list_header)
                    data->try_get_header();
                */
                HFONT font = (HFONT)SendMessageW(data->list_header, WM_GETFONT, 0, 0);
                //LRESULT r = SendMessageW(data->header, WM_SETFONT, (WPARAM)font, false);

                // adjust the width
                wchar_t header_text[] = L"键";
                HDC dc = GetDC(data->header);
                HGDIOBJ original_font = SelectObject(dc, font);  // cannot just use the device context of list_header
                SIZE size;
                GetTextExtentPoint32W(dc, header_text, std::size(header_text) - 1, &size);
                SelectObject(dc, original_font);
                ReleaseDC(data->header, dc);
                size.cx += data->left_padding + data->right_padding;

                //ListView_SetColumnWidth(quick_list, 0, size.cx);
                
                RECT quick_rect;
                GetWindowRect(hwnd, &quick_rect);
                SetWindowPos_real(hwnd, nullptr, 0, 0, size.cx, quick_rect.bottom - quick_rect.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

                // fix for zoom reseting (Ctrl+0)
                RECT list_rect;
                GetWindowRect(data->list, &list_rect);
                int quick_width_delta = size.cx - (quick_rect.right - quick_rect.left);
                list_rect.left += quick_width_delta;
                POINT list_pos{ list_rect.left, list_rect.top };
                ScreenToClient(GetParent(data->list), &list_pos);
                SetWindowPos_real(data->list, nullptr,
                    list_pos.x, list_pos.y,
                    list_rect.right - list_rect.left, list_rect.bottom - list_rect.top,
                    SWP_NOACTIVATE | SWP_NOZORDER
                    );
            }
        }

        // adjust the background color and text color
        if (COLORREF new_bk_color = ListView_GetBkColor(data->list); new_bk_color != data->background_color) {
            DeleteObject(data->background_brush);
            data->background_brush = CreateSolidBrush(new_bk_color);
            data->background_color = new_bk_color;

            data->text_color = ListView_GetTextColor(data->list);
        }

        return true;
    }
    case WM_NCCALCSIZE: {
        // prevent scroll bars
        DWORD style = GetWindowLongW(hwnd, GWL_STYLE);

        DWORD new_style = style;
        if (new_style & WS_VSCROLL)
            new_style &= ~WS_VSCROLL;
        if (new_style & WS_HSCROLL)
            new_style &= ~WS_HSCROLL;

        if (new_style != style)
            SetWindowLongW(hwnd, GWL_STYLE, new_style);
        break;
    }
    case WM_ERASEBKGND: {
        /*
        // only display for a few frames
        QuickListData* data = (QuickListData*)GetPropW(hwnd, quick_list_prop);

        HDC dc = (HDC)wParam;
        RECT client;
        GetClientRect(hwnd, &client);
        //FillRect(dc, &client, (HBRUSH)GetStockObject(BLACK_BRUSH));
        FillRect(dc, &client, data->background_brush);
        */

        return true;
    }
    case WM_DRAWITEM: /* draw header item */ {
        QuickListData* data = (QuickListData*)GetPropW(hwnd, quick_list_prop);

        DRAWITEMSTRUCT* draw = (DRAWITEMSTRUCT*)lParam;
        HDC dc = draw->hDC;
        FillRect(dc, &draw->rcItem, data->background_brush);

        draw->rcItem.left = data->left_padding;

        SetTextColor(dc, data->text_color);
        SetBkMode(dc, TRANSPARENT);

        HFONT font = (HFONT)SendMessageW(data->list_header, WM_GETFONT, 0, 0);
        HGDIOBJ original_font = SelectObject(dc, font);
        DrawTextW(dc, L"键", 1, &draw->rcItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, original_font);

        return true;
    }
    case WM_NCDESTROY: {
        // there is no need to restore GWLP_WNDPROC

        QuickListData* data = (QuickListData*)GetPropW(hwnd, quick_list_prop);
        DeleteObject(data->background_brush);
        delete data;

        break;
    }
    }
    return CallWindowProcW(quick_list_window_proc_prev, hwnd, uMsg, wParam, lParam);
}

HWND quick::create_quick_list(HWND everything, HWND list, HWND edit, HINSTANCE instance) {
    HWND quick_list = CreateWindowExW_real(
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, WC_LISTVIEWW, nullptr, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA | LVS_OWNERDRAWFIXED,
        0, 0, 20, 0,
        everything, nullptr, instance, nullptr);

    QuickListData* data = new QuickListData{
        .header = ListView_GetHeader(quick_list),
        .list = list,
        .list_header = nullptr,  // ListView_GetHeader(list) returns 0
        .item_height = 0,
        .background_color = ListView_GetBkColor(list),
        .text_color = ListView_GetTextColor(list)
    };
    data->background_brush = CreateSolidBrush(data->background_color);
    //ListView_SetBkColor(quick_list, data->background_color);  // not work
    //ListView_SetTextBkColor(quick_list, data->background_color);  // not work
    SetPropW(quick_list, quick_list_prop, data);
    quick_list_window_proc_prev = (WNDPROC)SetWindowLongPtrW_real(quick_list, GWLP_WNDPROC, (LONG_PTR)quick_list_window_proc);
    quick_header_window_proc_prev = (WNDPROC)SetWindowLongPtrW_real(data->header, GWLP_WNDPROC, (LONG_PTR)quick_header_window_proc);

    SetPropW(everything, everything_prop_quick_list, quick_list);
    everything_window_proc_prev = (WNDPROC)SetWindowLongPtrW_real(everything, GWLP_WNDPROC, (LONG_PTR)everything_window_proc);

    SetPropW(list, list_prop_quick_list, quick_list);

    SetPropW(edit, edit_prop_list, list);

    
    wchar_t header_text[] = L"键";
    /*
    HDC dc = GetDC(quick_list);
    SIZE size;
    GetTextExtentPoint32W(dc, header_text, std::size(header_text) - 1, &size);
    ReleaseDC(quick_list, dc);
    size.cx += 8;  // padding
    */
    
    LVCOLUMNW column{
        .mask = LVCF_WIDTH | LVCF_TEXT,
        .cx = 0x7FFF,
        .pszText = header_text,
        .cchTextMax = std::size(header_text) - 1
    };
    ListView_InsertColumn(quick_list, 0, &column);
    HDITEM header_item{
        .mask = HDI_FORMAT,
        .fmt = HDF_OWNERDRAW
    };
    Header_SetItem(data->header, 0, &header_item);

    ListView_SetItemCount(quick_list, 100000000);

    /*
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
    */

    //SetWindowPos(quick_list, nullptr, 0, 0, size.cx, 0, SWP_NOMOVE | SWP_NOZORDER);

    return quick_list;
}