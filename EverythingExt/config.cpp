#include "pch.h"
#include "common.hpp"
#include "config.hpp"
#include <fstream>
#include <regex>
#include <Shlwapi.h>
#include <yaml-cpp/yaml.h>

#pragma comment(lib, "Shlwapi.lib")

Config config{};

std::wstring u8_to_u16(const std::string& u8) {
    std::wstring u16;
    u16.resize(MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), u8.size(), nullptr, 0));
    MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), u8.size(), u16.data(), u16.size());
    return u16;
}

bool config_init(const char* yaml) {
    using namespace std::literals;

    config.enable = false;

    wchar_t root_path[MAX_PATH];
    GetModuleFileNameW(nullptr, root_path, std::size(root_path));
    PathRemoveFileSpecW(root_path);
    PathAppendW(root_path, LR"(plugins\IbEverythingExt\)");

    try {
        YAML::Node root;
        if (yaml) {
            root = YAML::Load(yaml);
        } else {
            wchar_t config_path[MAX_PATH];
            wcscpy_s(config_path, root_path);
            PathAppendW(config_path, LR"(config.yaml)");

            std::ifstream in(config_path);
            if (!in) {
                MessageBoxW(nullptr, L"配置文件 config.yaml 不存在！", L"IbEverythingExt", MB_ICONERROR);
                return false;
            }

            root = YAML::Load(in);
        }

        {
            YAML::Node node = root["pinyin_search"];
            config.pinyin_search = {
                .enable = node["enable"].as<bool>(),
                .mode = [&node] {
                    auto mode = node["mode"].as<std::string>();
                    if (mode == "Pcre")
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
                
                .search_edit = [](const YAML::Node& node) {
                    return decltype(config.quick_select.search_edit) {
                        .alt = node["alt"].as<uint8_t>()
                    };
                }(node["search_edit"]),
                
                .result_list = [](const YAML::Node& node) {
                    decltype(config.quick_select.result_list) result_list {
                        .select = node["select"].as<bool>(),
                        .alt = node["alt"].as<uint8_t>()
                    };
                    std::wstring terminal = u8_to_u16(node["terminal"].as<std::string>());
                    if (terminal.size()) {
                        std::wregex reg(LR"__((?:"([^"]*)"|([^ ]*)) ?(.*))__");
                        std::wsmatch match;
                        std::regex_match(terminal, match, reg);
                        result_list.terminal_file = match[1].str() + match[2].str();
                        result_list.terminal_parameter = match[3].str();
                    }
                    return result_list;
                }(node["result_list"]),
                    
                .close_everything = node["close_everything"].as<bool>(),
                
                .input_mode = [&node] {
                    auto mode = node["input_mode"].as<std::string>();
                    if (mode == "Auto")
                        return quick::InputMode::Auto;
                    else if (mode == "WmKey")
                        return quick::InputMode::WmKey;
                    else if (mode == "SendInput")
                        return quick::InputMode::SendInput;
                    throw YAML::Exception(YAML::Mark::null_mark(), "Invalid quick_select.input_mode");
                }()
            };
        }
        
        if (config.update.check = root["update"]["check"].as<bool>()) {
            wchar_t update_path[MAX_PATH];
            wcscpy_s(update_path, root_path);
            PathAppendW(update_path, L"Updater.exe");
            config.update.update_path = update_path;

            // to avoid checking on system startup and when calling Everything through command line,
            // we do the check after the main window is created
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
    return true;
}

void config_destroy() {
    pinyin::destroy();
}