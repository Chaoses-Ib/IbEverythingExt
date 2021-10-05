#include "pch.h"
#include "config.hpp"
#include <fstream>
#include <Shlwapi.h>
#include <yaml-cpp/yaml.h>

#pragma comment(lib, "Shlwapi.lib")

Config config{};

void config_init() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, std::size(path));
    PathRemoveFileSpecW(path);
    PathAppendW(path, L"IbEverythingExt.yaml");

    std::ifstream in(path);
    if (in) {
        YAML::Node root = YAML::Load(in);
        config.pinyin_search = root["pinyin_search"].as<bool>();
        config.quick_select = root["quick_select"].as<bool>();
    }
}

void config_destroy() {}