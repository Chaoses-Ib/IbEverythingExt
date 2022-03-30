#pragma once
#include "PinyinSearch.hpp"
#include "quick_select.hpp"

struct Config {
    // set to false when config_init fails
    bool enable;

    struct {
        bool enable;
        PinyinSearchMode mode;
        std::vector<pinyin::PinyinFlagValue> flags;
    } pinyin_search;

    struct {
        bool enable;
        
        struct {
            uint8_t alt;
        } search_edit;
        
        struct {
            bool select;
            uint8_t alt;
            std::wstring terminal_file;
            std::wstring terminal_parameter;
        } result_list;
        
        bool close_everything;
        
        quick::InputMode input_mode;
    } quick_select;
};
extern Config config;

bool config_init();
void config_destroy();