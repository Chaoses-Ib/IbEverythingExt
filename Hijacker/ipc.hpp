#pragma once
#include <IbEverythingLib/Everything.hpp>

extern ib::Holder<Everythings::EverythingMT> ipc_ev;
//extern Everythings::EverythingMT::Version ipc_version;
void ipc_init(std::wstring_view instance_name);
void ipc_destroy();


extern WNDPROC ipc_window_proc_prev;
LRESULT CALLBACK ipc_window_proc(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam);