#include "pch.h"
#include "config.hpp"
#include <fstream>
#include <Shlwapi.h>
#include <yaml-cpp/yaml.h>

#pragma comment(lib, "Shlwapi.lib")

Config config{};

bool config_init() {
    using namespace std::literals;

    config.enable = false;

    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, std::size(path));
    PathRemoveFileSpecW(path);
    PathAppendW(path, L"IbEverythingExt.yaml");

    std::ifstream in(path);
    if (!in) {
        MessageBoxW(nullptr, L"配置文件 IbEverythingExt.yaml 不存在！", L"IbEverythingExt", MB_ICONERROR);
        return false;
    }
    try {
        YAML::Node root = YAML::Load(in);

        {
            YAML::Node node = root["pinyin_search"];
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
                    throw YAML::Exception(YAML::Mark::null_mark(), "Invalid pinyin_search.mode");
                }(),
                .flags = [&node] {
                    std::vector<pinyin::PinyinFlagValue> flags;

                    // mind the order
                    if (node["pinyin_ascii_digit"].as<bool>())
                        flags.push_back(pinyin::PinyinFlag::PinyinAsciiDigit);
                    if (node["pinyin_ascii"].as<bool>())
                        flags.push_back(pinyin::PinyinFlag::PinyinAscii);
                    if (node["double_pinyin_abc"].as<bool>())
                        flags.push_back(pinyin::PinyinFlag::DoublePinyinAbc);
                    if (node["double_pinyin_jiajia"].as<bool>())
                        flags.push_back(pinyin::PinyinFlag::DoublePinyinJiajia);
                    if (node["double_pinyin_microsoft"].as<bool>())
                        flags.push_back(pinyin::PinyinFlag::DoublePinyinMicrosoft);
                    if (node["double_pinyin_thunisoft"].as<bool>())
                        flags.push_back(pinyin::PinyinFlag::DoublePinyinThunisoft);
                    if (node["double_pinyin_xiaohe"].as<bool>())
                        flags.push_back(pinyin::PinyinFlag::DoublePinyinXiaohe);
                    if (node["double_pinyin_zrm"].as<bool>())
                        flags.push_back(pinyin::PinyinFlag::DoublePinyinZrm);
                    if (node["initial_letter"].as<bool>())
                        flags.push_back(pinyin::PinyinFlag::InitialLetter);
                    return flags;
                }()
            };
        }

        {
            YAML::Node node = root["quick_select"];
            config.quick_select = {
                .enable = node["enable"].as<bool>(),
                .hotkey_mode = node["hotkey_mode"].as<int>(),
                .input_mode = [&node] {
                    auto mode = node["input_mode"].as<std::string>();
                    if (mode == "Auto")
                        return quick::InputMode::Auto;
                    else if (mode == "WmKey")
                        return quick::InputMode::WmKey;
                    else if (mode == "SendInput")
                        return quick::InputMode::SendInput;
                    throw YAML::Exception(YAML::Mark::null_mark(), "Invalid quick_select.input_mode");
                }(),
                .close_everything = node["close_everything"].as<bool>()
            };
        }
    }
    catch (YAML::Exception& e) {
        MessageBoxA(nullptr, ("配置文件读取错误：\n"s + e.what()).c_str(), "IbEverythingExt", MB_ICONERROR);
        return false;
    }

    pinyin::PinyinFlagValue flags{};
    for (pinyin::PinyinFlagValue flag : config.pinyin_search.flags)
        flags |= flag;
    pinyin::init(flags);

    config.enable = true;
}

void config_destroy() {
    pinyin::destroy();
}