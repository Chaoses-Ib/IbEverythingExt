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

        if (YAML::Node node = root["pinyin_search"]) {
            config.pinyin_search = {
                .enable = node["enable"].as<bool>(),
                .mode = [&node] {
                    auto mode = node["mode"].as<std::string>();
                    if (mode == "Auto")
                        return PinyinSearchMode::Auto;
                    else if (mode == "Pcre")
                        return PinyinSearchMode::Pcre;
                    else if (mode == "Edit")
                        return PinyinSearchMode::Edit;
                    throw std::range_error("Invalid pinyin_search.mode");
                }(),
                .flags = [&node] {
                    std::vector<pinyin::PinyinFlagValue> flags;

                    // mind the order
                    if (node["pinyin_ascii_digit"].as<bool>())
                        flags.push_back(pinyin::PinyinFlag::PinyinAsciiDigit);
                    if (node["pinyin_ascii"].as<bool>())
                        flags.push_back(pinyin::PinyinFlag::PinyinAscii);
                    if (node["double_pinyin_xiaohe"].as<bool>())
                        flags.push_back(pinyin::PinyinFlag::DoublePinyinXiaohe);
                    if (node["initial"].as<bool>())
                        flags.push_back(pinyin::PinyinFlag::Initial);
                    return flags;
                }()
            };
        }

        if (YAML::Node node = root["quick_select"]) {
            config.quick_select = {
                .enable = node["enable"].as<bool>()
            };
        }
    }
}

void config_destroy() {}