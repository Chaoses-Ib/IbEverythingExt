#pragma once

namespace quick {
    // require ipc_init
    void init();
    void destroy();

    HWND create_quick_list(HWND everything, HWND list, HWND edit, HINSTANCE instance);
    LRESULT CALLBACK list_window_proc(
        WNDPROC prev_proc,
        _In_ HWND   hwnd,
        _In_ UINT   uMsg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam);
}