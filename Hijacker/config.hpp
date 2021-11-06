#pragma once
#include "PinyinSearch.hpp"

struct Config {
    struct {
        bool enable;
        PinyinSearchMode mode;
    } pinyin_search;
    struct {
        bool enable;
    } quick_select;
};
extern Config config;

void config_init();
void config_destroy();