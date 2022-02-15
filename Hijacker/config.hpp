#pragma once
#include "PinyinSearch.hpp"

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
    } quick_select;
};
extern Config config;

bool config_init();
void config_destroy();