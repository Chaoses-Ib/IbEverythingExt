#pragma once
#include "PinyinSearch.hpp"

class PinyinSearchEdit : public PinyinSearch {
public:
    PinyinSearchEdit(const std::wstring& instance_name, HWND ipc_window);
    ~PinyinSearchEdit() override;

    void everything_created() override;
    void edit_created(HWND edit) override;
};