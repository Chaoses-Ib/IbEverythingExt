#pragma once
#include "PinyinSearch.hpp"

struct Config {
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

void config_init();
void config_destroy();