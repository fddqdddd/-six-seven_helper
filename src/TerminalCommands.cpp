#include "../include/TerminalCommands.h"

#include "../include/Util.h"

#include <windows.h>

#include <fstream>
#include <set>
#include <vector>

namespace six_seven {

namespace {

constexpr wchar_t kStubDirSuffix[] = L"\\Six_Seven\\terminal_cmd";
constexpr wchar_t kInboxDirSuffix[] = L"\\Six_Seven\\cmd_inbox";

struct CommandDef {
    const wchar_t* id;
};

constexpr CommandDef kCommands[] = {
    { L"six-seven_open" },
    { L"kill_67" },
    { L"sleep67" },
    { L"67move" },
    { L"67_otzov" },
    { L"67_pet" },
    { L"67_glitch" },
    { L"67_lazy" },
};

std::wstring LocalAppDataPath()
{
    wchar_t buf[MAX_PATH] = {};
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return L"";
    return buf;
}

std::wstring StubDirectory()
{
    const std::wstring base = LocalAppDataPath();
    if (base.empty())
        return L"";
    return base + kStubDirSuffix;
}

std::wstring InboxDirectory()
{
    const std::wstring base = LocalAppDataPath();
    if (base.empty())
        return L"";
    return base + kInboxDirSuffix;
}

bool EnsureDirectory(const std::wstring& path)
{
    if (path.empty())
        return false;
    if (CreateDirectoryW(path.c_str(), nullptr))
        return true;
    return GetLastError() == ERROR_ALREADY_EXISTS;
}

bool WriteUtf8File(const std::wstring& path, const std::string& utf8)
{
    std::ofstream out(WideToUtf8(path.c_str()), std::ios::binary | std::ios::trunc);
    if (!out)
        return false;
    out.write("\xEF\xBB\xBF", 3);
    out.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
    return static_cast<bool>(out);
}

std::string BuildStubBody(const wchar_t* commandId)
{
    const std::wstring inbox = InboxDirectory();
    const std::string inboxUtf8 = WideToUtf8(inbox.c_str());
    const std::string cmdUtf8 = WideToUtf8(commandId);
    std::string body;
    body += "@echo off\r\n";
    body += "chcp 65001 >nul 2>&1\r\n";
    body += "echo 67 уже выполняет команду\r\n";
    body += "if not exist \"" + inboxUtf8 + "\" mkdir \"" + inboxUtf8 + "\"\r\n";
    body += "break > \"" + inboxUtf8 + "\\" + cmdUtf8 + ".req\"\r\n";
    return body;
}

bool PathListContains(const std::wstring& pathList, const std::wstring& entry)
{
    if (entry.empty())
        return false;
    size_t pos = 0;
    while (pos <= pathList.size()) {
        size_t sep = pathList.find(L';', pos);
        if (sep == std::wstring::npos)
            sep = pathList.size();
        std::wstring part = pathList.substr(pos, sep - pos);
        while (!part.empty() && (part.back() == L' ' || part.back() == L'\"'))
            part.pop_back();
        while (!part.empty() && (part.front() == L' ' || part.front() == L'\"'))
            part.erase(part.begin());
        if (_wcsicmp(part.c_str(), entry.c_str()) == 0)
            return true;
        if (sep == pathList.size())
            break;
        pos = sep + 1;
    }
    return false;
}

bool AddStubDirToUserPath(const std::wstring& stubDir)
{
    if (stubDir.empty())
        return false;

    wchar_t pathBuf[32767] = {};
    DWORD pathLen = GetEnvironmentVariableW(L"Path", pathBuf, 32767);
    std::wstring path = pathLen > 0 ? std::wstring(pathBuf, pathLen) : L"";

    if (PathListContains(path, stubDir))
        return true;

    if (!path.empty() && path.back() != L';')
        path.push_back(L';');
    path += stubDir;

    if (!SetEnvironmentVariableW(L"Path", path.c_str()))
        return false;

    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_READ | KEY_WRITE, &key) !=
        ERROR_SUCCESS)
        return false;

    wchar_t regBuf[32767] = {};
    DWORD regSize = sizeof(regBuf);
    DWORD regType = REG_EXPAND_SZ;
    std::wstring regPath;
    if (RegQueryValueExW(key, L"Path", nullptr, &regType, reinterpret_cast<LPBYTE>(regBuf),
                         &regSize) == ERROR_SUCCESS &&
        (regType == REG_SZ || regType == REG_EXPAND_SZ)) {
        regPath = regBuf;
    }

    if (!PathListContains(regPath, stubDir)) {
        if (!regPath.empty() && regPath.back() != L';')
            regPath.push_back(L';');
        regPath += stubDir;
        RegSetValueExW(key, L"Path", 0, REG_EXPAND_SZ,
                       reinterpret_cast<const BYTE*>(regPath.c_str()),
                       static_cast<DWORD>((regPath.size() + 1) * sizeof(wchar_t)));
    }
    RegCloseKey(key);

    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                        reinterpret_cast<LPARAM>(L"Environment"), SMTO_ABORTIFHUNG, 5000, nullptr);
    return true;
}

} /* namespace */

bool InstallTerminalCommandStubs()
{
    const std::wstring stubDir = StubDirectory();
    const std::wstring inboxDir = InboxDirectory();
    if (stubDir.empty() || inboxDir.empty())
        return false;
    const std::wstring root = LocalAppDataPath() + L"\\Six_Seven";
    if (!EnsureDirectory(root) || !EnsureDirectory(stubDir) || !EnsureDirectory(inboxDir))
        return false;

    for (const auto& cmd : kCommands) {
        const std::wstring stubPath = stubDir + L"\\" + cmd.id + L".cmd";
        if (!WriteUtf8File(stubPath, BuildStubBody(cmd.id)))
            return false;
    }

    return AddStubDirToUserPath(stubDir);
}

void PollTerminalCommandInbox(const std::function<void(const std::wstring&)>& onCommand)
{
    if (!onCommand)
        return;
    const std::wstring inbox = InboxDirectory();
    if (inbox.empty())
        return;

    const std::wstring pattern = inbox + L"\\*.req";
    WIN32_FIND_DATAW fd = {};
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE)
        return;

    static std::set<std::wstring> recent;
    recent.clear();

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        std::wstring name = fd.cFileName;
        const size_t dot = name.rfind(L'.');
        if (dot == std::wstring::npos)
            continue;
        const std::wstring command = name.substr(0, dot);
        const std::wstring full = inbox + L"\\" + name;
        if (recent.count(command))
            continue;
        recent.insert(command);
        DeleteFileW(full.c_str());
        onCommand(command);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

} /* namespace six_seven */
