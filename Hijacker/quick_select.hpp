#pragma once

constexpr wchar_t prop_edit_list[] = L"IbEverythingExt.List";
constexpr wchar_t prop_list_quick_list[] = L"IbEverythingExt.QuickList";

void quick_select_init();
void quick_select_destroy();

HWND quick_list_create(HWND parent, HINSTANCE instance);