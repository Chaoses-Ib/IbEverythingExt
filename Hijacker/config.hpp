#pragma once

struct Config {
    bool pinyin_search = true;
    bool quick_select = true;
};
extern Config config;

void config_init();
void config_destroy();