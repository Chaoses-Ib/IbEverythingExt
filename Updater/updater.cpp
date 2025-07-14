#include <Shlwapi.h>
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <IbUpdate/GitHubUpdater.hpp>

// IbUpdate -> github_api -> cURL
// See https://github.com/microsoft/vcpkg/issues/2621
#pragma comment(lib, "Ws2_32.Lib")
//#pragma comment(lib, "Wldap32.Lib")
#pragma comment(lib, "Crypt32.Lib")

#pragma comment(lib, "Shlwapi.lib")

std::wstring u8_to_u16(std::string u8) {
    int u16_size = MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), (int)u8.length(), nullptr, 0);
    std::wstring u16(u16_size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), (int)u8.length(), u16.data(), u16_size);
    return u16;
}

/// <summary>
/// 
/// </summary>
/// <param name="s"></param>
/// <param name="lines"></param>
/// <returns>Ends with '\n'</returns>
std::string truncate_lines(std::string s, size_t lines) {
    std::istringstream in(s);
    std::ostringstream out;
    
    size_t i = 0;
    for (std::string line; i < lines && std::getline(in, line); i++) {
        out << line << '\n';
    }
    if (i == lines)
        out << (const char*)u8"……\n";
    return out.str();
}

bool check_for_update(bool prerelease, bool quiet, bool silent_error) {
    try {
        GitHubUpdater updater{ "Chaoses-Ib", "IbEverythingExt", "v0.8.0-beta.3" };
        YAML::Node release = updater.check_for_new_release(prerelease);
        if (release.IsNull()) {
            if (!quiet)
                MessageBoxW(nullptr, L"无可用更新", L"IbEverythingExt", MB_ICONINFORMATION);
            return true;
        }

        std::wostringstream ss;
        ss << L"检测到新版本 " << u8_to_u16(release["tag_name"].as<std::string>())
            << L"\n发布时间：" << u8_to_u16(release["published_at"].as<std::string>())
            << L"\n更新内容：\n" << u8_to_u16(truncate_lines(release["body"].as<std::string>(), 10))
            << L"\n是否打开发布页面？";
        if (MessageBoxW(nullptr, ss.str().c_str(), L"IbEverythingExt", MB_ICONINFORMATION | MB_YESNO) == IDYES) {
            ShellExecuteW(nullptr, nullptr, u8_to_u16(release["html_url"].as<std::string>()).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        return true;
    }
    catch (std::runtime_error& e) {
        if (!silent_error) {
            std::ostringstream ss;
            ss << "检查更新失败：\n" << e.what();
            MessageBoxA(nullptr, ss.str().c_str(), "IbEverythingExt", MB_ICONWARNING);
        }
        return false;
    }
}

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nShowCmd) {
    using namespace std::literals;

    // lpCmdLine is empty for unknown reason
    //MessageBoxW(nullptr, lpCmdLine, nullptr, 0);
    //MessageBoxW(nullptr, GetCommandLineW(), nullptr, 0);
    lpCmdLine = GetCommandLineW();
    
    bool quiet = false;
    if (std::wstring_view(lpCmdLine) == L"--quiet")
        quiet = true;

    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, std::size(path));
    PathRemoveFileSpecW(path);
    PathAppendW(path, L"config.yaml");

    std::ifstream in(path);
    if (!in) {
        MessageBoxW(nullptr, L"配置文件 config.yaml 不存在！", L"IbEverythingExt", MB_ICONERROR);
        return 1;
    }
    try {
        YAML::Node root = YAML::Load(in);
        YAML::Node update = root["update"];
        if (update["check"].as<bool>()) {
            // try twice
            if (!check_for_update(update["prerelease"].as<bool>(), quiet, true))
                check_for_update(update["prerelease"].as<bool>(), quiet, false);
        }
    }
    catch (YAML::Exception& e) {
        MessageBoxA(nullptr, ("配置文件读取错误：\n"s + e.what()).c_str(), "IbEverythingExt", MB_ICONERROR);
        return 1;
    }
    return 0;
}