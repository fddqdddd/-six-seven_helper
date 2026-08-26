#include "../include/ProfileCustomizer.h"
#include "../include/Util.h"
#include "../config.h"

#include <objbase.h>
#include <lmcons.h>
#include <shlobj.h>
#include <gdiplus.h>
#include <sddl.h>

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <secext.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace six_seven {

namespace {

constexpr wchar_t kInfoFile[] = L"information";
constexpr wchar_t kProfileSection[] = L"profile";
constexpr wchar_t kProfileTool[] = L"Six_Seven_Profile.exe";
constexpr wchar_t kBootTask[] = L"Six_Seven_Profile_Boot";
/* Win10/8 и меню Пуск: HKCU + PNG в AppData\Roaming\...\AccountPictures */
constexpr int kUserPictureSizes[] = { 32, 40, 48, 96, 192, 240, 448 };
/* Win11 и экран входа: HKLM + JPG в C:\Users\Public\AccountPictures\{SID} */
constexpr int kMachinePictureSizes[] = { 32, 40, 48, 64, 96, 192, 208, 240, 424, 448, 1080 };

using ImagePathEntry = std::pair<int, std::wstring>;

void ReadBackupImagePath(int size, std::wstring& out);

std::wstring InfoPath() { return PathJoin(GetExeDirectory(), kInfoFile); }

std::wstring ProfileCacheDir()
{
    wchar_t localApp[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, localApp)))
        return PathJoin(GetExeDirectory(), L"profile_cache");
    std::wstring base = PathJoin(localApp, L"Six_Seven");
    CreateDirectoryW(base.c_str(), nullptr);
    std::wstring dir = PathJoin(base, L"profile");
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

std::wstring ParentDir(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos)
        return {};
    return path.substr(0, slash);
}

void EnsureDirectoryTree(const std::wstring& path)
{
    std::wstring cur;
    for (size_t i = 0; i < path.size(); ++i) {
        const wchar_t ch = path[i];
        cur.push_back(ch);
        if (ch == L'\\' || ch == L'/') {
            if (cur.size() > 3)
                CreateDirectoryW(cur.c_str(), nullptr);
        }
    }
    CreateDirectoryW(path.c_str(), nullptr);
}

std::wstring BackupImageDir()
{
    const std::wstring dir = PathJoin(ProfileCacheDir(), L"backup_original");
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

bool ProfileIniHasKey(const wchar_t* key)
{
    wchar_t buf[4] = {};
    GetPrivateProfileStringW(kProfileSection, key, L"", buf, static_cast<DWORD>(std::size(buf)),
                             InfoPath().c_str());
    return buf[0] != L'\0';
}

bool HasBackupSnapshot() { return ProfileIniHasKey(L"backup_snapshot"); }

void MarkBackupSnapshot()
{
    WritePrivateProfileStringW(kProfileSection, L"backup_snapshot", L"1", InfoPath().c_str());
}

bool PathLooksCustomized(const std::wstring& path)
{
    return path.find(L"six_seven_") != std::wstring::npos ||
           path.find(L"SixSeven") != std::wstring::npos;
}

std::wstring BackupImageFilePath(int size)
{
    wchar_t name[64] = {};
    wsprintfW(name, L"orig_%d.bin", size);
    return PathJoin(BackupImageDir(), name);
}

void StoreBackupImageFile(int size, const std::wstring& sourcePath)
{
    if (sourcePath.empty() || !FileExists(sourcePath))
        return;
    CopyFileW(sourcePath.c_str(), BackupImageFilePath(size).c_str(), FALSE);
}

bool HasBackupImageFile(int size) { return FileExists(BackupImageFilePath(size)); }

bool RestoreBackupImageFile(int size, const std::wstring& destPath)
{
    const std::wstring backup = BackupImageFilePath(size);
    if (!FileExists(backup))
        return false;
    const std::wstring parent = ParentDir(destPath);
    if (!parent.empty())
        EnsureDirectoryTree(parent);
    return CopyFileW(backup.c_str(), destPath.c_str(), FALSE) != 0;
}

void SaveUserSidToInfo(const std::wstring& sid)
{
    if (!sid.empty())
        WritePrivateProfileStringW(kProfileSection, L"user_sid", sid.c_str(), InfoPath().c_str());
}

std::wstring DefaultWindowsUserPicture(int size)
{
    wchar_t name[64] = {};
    wsprintfW(name, L"user-%d.png", size);
    std::wstring path =
        PathJoin(L"C:\\ProgramData\\Microsoft\\User Account Pictures", name);
    if (FileExists(path))
        return path;
    return PathJoin(L"C:\\ProgramData\\Microsoft\\User Account Pictures", L"user.png");
}

std::wstring PublicAccountDir(const std::wstring& sid)
{
    return L"C:\\Users\\Public\\AccountPictures\\" + sid;
}

std::wstring UserAccountPicturesDir()
{
    wchar_t appData[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appData)))
        return {};
    std::wstring dir = PathJoin(appData, L"Microsoft");
    CreateDirectoryW(dir.c_str(), nullptr);
    dir = PathJoin(dir, L"Windows");
    CreateDirectoryW(dir.c_str(), nullptr);
    dir = PathJoin(dir, L"AccountPictures");
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty())
        return {};
    const int need =
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (need <= 1)
        return {};
    std::string out(static_cast<size_t>(need - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, out.data(), need, nullptr, nullptr);
    return out;
}

bool WriteUtf16LeFile(const std::wstring& path, const std::wstring& content)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    const wchar_t bom = 0xFEFF;
    DWORD written = 0;
    WriteFile(file, &bom, sizeof(bom), &written, nullptr);
    WriteFile(file, content.c_str(), static_cast<DWORD>(content.size() * sizeof(wchar_t)),
              &written, nullptr);
    CloseHandle(file);
    return written > 0;
}

void LogLine(const wchar_t* line)
{
    const std::wstring path = PathJoin(GetExeDirectory(), L"profile_apply.log");
    std::string text = WideToUtf8(line);
    text.push_back('\n');
    HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return;
    DWORD written = 0;
    WriteFile(file, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
    CloseHandle(file);
}

std::wstring OrigDisplayNameBackupPath()
{
    return PathJoin(ProfileCacheDir(), L"orig_display_name.txt");
}

void SaveOrigDisplayNameBackup(const std::wstring& name)
{
    if (name.empty() || wcscmp(name.c_str(), SIX_SEVEN_WINDOWS_DISPLAY_NAME) == 0)
        return;
    WriteUtf16LeFile(OrigDisplayNameBackupPath(), name);
}

std::wstring LoadOrigDisplayNameBackup()
{
    const std::wstring path = OrigDisplayNameBackupPath();
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return {};
    const DWORD size = GetFileSize(file, nullptr);
    if (size < sizeof(wchar_t) || size > 4096) {
        CloseHandle(file);
        return {};
    }
    std::vector<wchar_t> buf(size / sizeof(wchar_t) + 1, L'\0');
    DWORD read = 0;
    ReadFile(file, buf.data(), size, &read, nullptr);
    CloseHandle(file);
    size_t start = 0;
    if (read >= sizeof(wchar_t) && buf[0] == 0xFEFF)
        start = 1;
    return std::wstring(buf.data() + start);
}

std::wstring GetLoginUserName()
{
    wchar_t user[UNLEN + 1] = {};
    DWORD ulen = UNLEN + 1;
    if (GetUserNameW(user, &ulen) && user[0])
        return user;
    return {};
}

bool GetCurrentUserSidString(std::wstring& outSid)
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;
    DWORD len = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &len);
    std::vector<BYTE> buf(len);
    if (!GetTokenInformation(token, TokenUser, buf.data(), len, &len)) {
        CloseHandle(token);
        return false;
    }
    CloseHandle(token);
    auto* user = reinterpret_cast<TOKEN_USER*>(buf.data());
    LPWSTR sidStr = nullptr;
    if (!ConvertSidToStringSidW(user->User.Sid, &sidStr))
        return false;
    outSid = sidStr;
    LocalFree(sidStr);
    return true;
}

bool GetDefaultUserSidFromRegistry(std::wstring& outSid)
{
    outSid.clear();
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList", 0,
                      KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
        return false;
    wchar_t name[256] = {};
    for (DWORD i = 0;; ++i) {
        DWORD len = static_cast<DWORD>(std::size(name));
        if (RegEnumKeyExW(key, i, name, &len, nullptr, nullptr, nullptr, nullptr) !=
            ERROR_SUCCESS)
            break;
        if (wcsncmp(name, L"S-1-5-21-", 9) != 0)
            continue;
        outSid = name;
        break;
    }
    RegCloseKey(key);
    return !outSid.empty();
}

bool GetUserSidForApply(std::wstring& outSid)
{
    wchar_t buf[128] = {};
    GetPrivateProfileStringW(kProfileSection, L"user_sid", L"", buf, 128, InfoPath().c_str());
    if (buf[0]) {
        outSid = buf;
        return true;
    }
    if (GetCurrentUserSidString(outSid))
        return true;
    return GetDefaultUserSidFromRegistry(outSid);
}

bool ReadRegString(HKEY root, const wchar_t* subKey, const wchar_t* valueName, std::wstring& out,
                   REGSAM access = KEY_READ)
{
    out.clear();
    HKEY key = nullptr;
    if (RegOpenKeyExW(root, subKey, 0, access, &key) != ERROR_SUCCESS)
        return false;
    wchar_t buf[2048] = {};
    DWORD type = 0;
    DWORD size = sizeof(buf);
    const LONG rc = RegQueryValueExW(key, valueName, nullptr, &type,
                                     reinterpret_cast<LPBYTE>(buf), &size);
    RegCloseKey(key);
    if (rc != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ))
        return false;
    out = buf;
    return true;
}

bool WriteRegString(HKEY root, const wchar_t* subKey, const wchar_t* valueName,
                    const std::wstring& value, REGSAM access = KEY_SET_VALUE)
{
    HKEY key = nullptr;
    DWORD disp = 0;
    if (RegCreateKeyExW(root, subKey, 0, nullptr, 0, access, nullptr, &key, &disp) !=
        ERROR_SUCCESS)
        return false;
    const LONG rc =
        RegSetValueExW(key, valueName, 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(value.c_str()),
                       static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    return rc == ERROR_SUCCESS;
}

std::wstring AccountPictureKey(const std::wstring& sid)
{
    return L"Software\\Microsoft\\Windows\\CurrentVersion\\AccountPicture\\Users\\" + sid;
}

bool ReadRegistryFullName(const std::wstring& sid, std::wstring& out)
{
    const std::wstring sub =
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\" + sid;
    return ReadRegString(HKEY_LOCAL_MACHINE, sub.c_str(), L"FullName", out,
                         KEY_READ | KEY_WOW64_64KEY);
}

bool DeleteRegistryFullName(const std::wstring& sid)
{
    const std::wstring sub =
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\" + sid;
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, sub.c_str(), 0, KEY_SET_VALUE | KEY_WOW64_64KEY,
                      &key) != ERROR_SUCCESS)
        return false;
    const LONG rc = RegDeleteValueW(key, L"FullName");
    RegCloseKey(key);
    return rc == ERROR_SUCCESS || rc == ERROR_FILE_NOT_FOUND;
}

bool NameLooksInvalidForRestore(const std::wstring& name)
{
    if (name.empty())
        return true;
    if (name == SIX_SEVEN_WINDOWS_DISPLAY_NAME)
        return true;
    return name.find(L'?') != std::wstring::npos;
}

std::wstring ResolveRestoreDisplayName()
{
    std::wstring name = LoadOrigDisplayNameBackup();
    if (!NameLooksInvalidForRestore(name))
        return name;

    wchar_t buf[256] = {};
    GetPrivateProfileStringW(kProfileSection, L"orig_display_name", L"", buf, 256,
                             InfoPath().c_str());
    if (buf[0] && !NameLooksInvalidForRestore(buf))
        return buf;

    wchar_t login[UNLEN + 1] = {};
    GetPrivateProfileStringW(kProfileSection, L"orig_login_name", L"", login, UNLEN + 1,
                             InfoPath().c_str());
    if (login[0] && !NameLooksInvalidForRestore(login))
        return login;

    return GetLoginUserName();
}

bool VerifyDisplayNameApplied(const std::wstring& sid, const std::wstring& expected)
{
    if (expected.empty())
        return false;
    std::wstring current;
    if (!ReadRegistryFullName(sid, current))
        return false;
    while (!current.empty() && (current.back() == L' ' || current.back() == L'\t'))
        current.pop_back();
    return _wcsicmp(current.c_str(), expected.c_str()) == 0;
}

bool VerifyDisplayNameRestored(const std::wstring& sid, const std::wstring& expected)
{
    std::wstring current;
    if (!ReadRegistryFullName(sid, current))
        current.clear();
    while (!current.empty() && (current.back() == L' ' || current.back() == L'\t'))
        current.pop_back();
    if (current == SIX_SEVEN_WINDOWS_DISPLAY_NAME)
        return false;
    if (!expected.empty())
        return _wcsicmp(current.c_str(), expected.c_str()) == 0;
    return current.empty();
}

bool IsNameStillCustomized(const std::wstring& sid)
{
    std::wstring current;
    if (!ReadRegistryFullName(sid, current))
        return true;
    while (!current.empty() && (current.back() == L' ' || current.back() == L'\t'))
        current.pop_back();
    return current == SIX_SEVEN_WINDOWS_DISPLAY_NAME;
}

bool IsAvatarStillCustomized(const std::wstring& sid)
{
    const std::wstring sub = AccountPictureKey(sid);
    for (int sz : kUserPictureSizes) {
        wchar_t valName[32];
        wsprintfW(valName, L"Image%d", sz);
        std::wstring path;
        if (ReadRegString(HKEY_CURRENT_USER, sub.c_str(), valName, path) &&
            PathLooksCustomized(path))
            return true;
    }
    const std::wstring dir = UserAccountPicturesDir();
    if (!dir.empty()) {
        for (int sz : kUserPictureSizes) {
            wchar_t fileName[64];
            wsprintfW(fileName, L"six_seven_%d.png", sz);
            if (FileExists(PathJoin(dir, fileName)))
                return true;
        }
    }
    for (int sz : kMachinePictureSizes) {
        wchar_t valName[32];
        wsprintfW(valName, L"Image%d", sz);
        std::wstring hklmPath;
        if (!ReadRegString(HKEY_LOCAL_MACHINE, sub.c_str(), valName, hklmPath,
                           KEY_READ | KEY_WOW64_64KEY))
            continue;
        if (PathLooksCustomized(hklmPath))
            return true;
        std::wstring backupPath;
        ReadBackupImagePath(sz, backupPath);
        if (!backupPath.empty() &&
            _wcsicmp(hklmPath.c_str(), backupPath.c_str()) != 0)
            return true;
    }
    return false;
}

bool ApplyMachineRegistryElevated(const std::wstring& sid, const std::wstring& guid,
                                  const std::vector<ImagePathEntry>& images)
{
    if (guid.empty() || images.empty())
        return false;
    const std::wstring sub = AccountPictureKey(sid);
    bool ok = WriteRegString(HKEY_LOCAL_MACHINE, sub.c_str(), L"CorrelationID", guid,
                             KEY_SET_VALUE | KEY_WOW64_64KEY);
    for (const auto& entry : images) {
        wchar_t valName[32];
        wsprintfW(valName, L"Image%d", entry.first);
        if (WriteRegString(HKEY_LOCAL_MACHINE, sub.c_str(), valName, entry.second,
                           KEY_SET_VALUE | KEY_WOW64_64KEY))
            ok = true;
    }
    return ok;
}

std::wstring ExtractGuidFromImagePath(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L"\\/");
    const std::wstring name = slash == std::wstring::npos ? path : path.substr(slash + 1);
    const size_t start = name.find(L'{');
    const size_t end = name.find(L'}', start);
    if (start != std::wstring::npos && end != std::wstring::npos)
        return name.substr(start, end - start + 1);
    return {};
}

std::wstring NewPictureGuid()
{
    GUID g = {};
    if (FAILED(CoCreateGuid(&g)))
        return L"{00000000-0000-0000-0000-000000000001}";
    wchar_t buf[64] = {};
    StringFromGUID2(g, buf, 64);
    return buf;
}

bool GetCurrentDisplayName(std::wstring& out)
{
    out.clear();
    wchar_t buf[256] = {};
    ULONG size = static_cast<ULONG>(std::size(buf));
    if (GetUserNameExW(NameDisplay, buf, &size) && buf[0]) {
        out = buf;
        return true;
    }
    std::wstring sid;
    if (!GetCurrentUserSidString(sid))
        return false;
    std::wstring sub = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\" + sid;
    return ReadRegString(HKEY_LOCAL_MACHINE, sub.c_str(), L"FullName", out,
                         KEY_READ | KEY_WOW64_64KEY);
}

bool RunHiddenCommand(const std::wstring& cmdLine)
{
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};
    std::vector<wchar_t> buf(cmdLine.begin(), cmdLine.end());
    buf.push_back(L'\0');
    if (!CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi))
        return false;
    WaitForSingleObject(pi.hProcess, 30000);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return code == 0;
}

constexpr wchar_t kSystemRegTask[] = L"Six_Seven_Profile_Reg";

bool RunBatchAsSystem(const std::wstring& batchPath)
{
    RunHiddenCommand(std::wstring(L"schtasks /Delete /TN ") + kSystemRegTask + L" /F");
    std::wstring create = L"schtasks /Create /TN ";
    create += kSystemRegTask;
    create += L" /TR \"";
    create += batchPath;
    create += L"\" /SC ONCE /ST 23:59 /RU SYSTEM /RL HIGHEST /F";
    if (!RunHiddenCommand(create))
        return false;
    if (!RunHiddenCommand(std::wstring(L"schtasks /Run /TN ") + kSystemRegTask))
        return false;
    Sleep(2000);
    RunHiddenCommand(std::wstring(L"schtasks /Delete /TN ") + kSystemRegTask + L" /F");
    return true;
}

bool ApplyMachineRegistryAsSystem(const std::wstring& sid, const std::wstring& guid,
                                  const std::vector<ImagePathEntry>& images)
{
    if (guid.empty() || images.empty())
        return false;
    const std::wstring bat = PathJoin(ProfileCacheDir(), L"profile_reg.cmd");
    const std::wstring regBase =
        L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AccountPicture\\Users\\" + sid;
    std::wstring script = L"@reg add \"" + regBase + L"\" /v CorrelationID /t REG_SZ /d \"" +
                          guid + L"\" /f\r\n";
    for (const auto& entry : images) {
        wchar_t valName[32];
        wsprintfW(valName, L"Image%d", entry.first);
        script += L"@reg add \"" + regBase + L"\" /v " + valName + L" /t REG_SZ /d \"" +
                  entry.second + L"\" /f\r\n";
    }
    if (!WriteUtf16LeFile(bat, script))
        return false;
    return RunBatchAsSystem(bat);
}

void RestartExplorer()
{
    RunHiddenCommand(L"taskkill /f /im explorer.exe");
    Sleep(800);
    RunHiddenCommand(L"cmd.exe /c start explorer.exe");
}

void PreparePublicAccountDir(const std::wstring& sid)
{
    const std::wstring dir = PublicAccountDir(sid);
    EnsureDirectoryTree(L"C:\\Users\\Public\\AccountPictures");
    EnsureDirectoryTree(dir);

    wchar_t user[UNLEN + 1] = {};
    DWORD ulen = UNLEN + 1;
    GetUserNameW(user, &ulen);

    std::wstring takeown = L"cmd.exe /c takeown /F \"" + dir + L"\" /R /D Y";
    RunHiddenCommand(takeown);
    std::wstring icaclsAdmins =
        L"cmd.exe /c icacls \"" + dir + L"\" /grant *S-1-5-32-544:(OI)(CI)F /T";
    RunHiddenCommand(icaclsAdmins);
    if (user[0]) {
        std::wstring icaclsUser =
            L"cmd.exe /c icacls \"" + dir + L"\" /grant \"" + std::wstring(user) +
            L"\":(OI)(CI)F /T";
        RunHiddenCommand(icaclsUser);
    }
    std::wstring attrib = L"cmd.exe /c attrib -R \"" + dir + L"\" /S /D";
    RunHiddenCommand(attrib);
}

bool SetDisplayNameViaSystem(const std::wstring& sid, const std::wstring& name, bool clear)
{
    const std::wstring bat = PathJoin(ProfileCacheDir(), L"profile_name_reg.cmd");
    const std::wstring regPath =
        L"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\" + sid;
    std::wstring script;
    if (clear || name.empty())
        script = L"@reg delete \"" + regPath + L"\" /v FullName /f\r\n";
    else
        script = L"@reg add \"" + regPath + L"\" /v FullName /t REG_SZ /d \"" + name +
                 L"\" /f\r\n";
    if (!WriteUtf16LeFile(bat, script))
        return false;
    return RunBatchAsSystem(bat);
}

bool SetDisplayNameElevated(const std::wstring& name)
{
    std::wstring sid;
    if (!GetCurrentUserSidString(sid))
        return false;

    const bool clear = name.empty();
    const std::wstring sub =
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\" + sid;
    const std::wstring login = GetLoginUserName();

    if (clear) {
        (void)DeleteRegistryFullName(sid);
        (void)SetDisplayNameViaSystem(sid, L"", true);
        if (!login.empty()) {
            std::wstring net =
                L"cmd.exe /c net user \"" + login + L"\" /fullname:\"\"";
            RunHiddenCommand(net);
            std::wstring ps = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"";
            ps += L"Set-LocalUser -Name '" + login + L"' -FullName $null\"";
            RunHiddenCommand(ps);
        }
        return VerifyDisplayNameRestored(sid, L"");
    }

    const bool regOk = WriteRegString(HKEY_LOCAL_MACHINE, sub.c_str(), L"FullName", name,
                                      KEY_SET_VALUE | KEY_WOW64_64KEY);
    (void)SetDisplayNameViaSystem(sid, name, false);
    if (!login.empty()) {
        std::wstring ps = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"";
        ps += L"Set-LocalUser -Name '" + login + L"' -FullName '" + name + L"'\"";
        RunHiddenCommand(ps);
        std::wstring net =
            L"cmd.exe /c net user \"" + login + L"\" /fullname:\"" + name + L"\"";
        RunHiddenCommand(net);
    }
    if (VerifyDisplayNameApplied(sid, name))
        return true;
    return regOk && VerifyDisplayNameApplied(sid, name);
}

std::wstring FindAvatarSourcePng()
{
    const std::wstring folder =
        AssetPath((std::string(SIX_SEVEN_SPRITES_ROOT) + "/" + MOD_SPRITE_AVATAR).c_str());
    WIN32_FIND_DATAW fd = {};
    const std::wstring pattern = PathJoin(folder, L"*.png");
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    std::wstring best;
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                best = PathJoin(folder, fd.cFileName);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    return best;
}

bool GetEncoderClsid(const wchar_t* mime, CLSID* out)
{
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
        return false;
    std::vector<BYTE> buf(size);
    auto* codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buf.data());
    if (Gdiplus::GetImageEncoders(num, size, codecs) != Gdiplus::Ok)
        return false;
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(codecs[i].MimeType, mime) == 0) {
            *out = codecs[i].Clsid;
            return true;
        }
    }
    return false;
}

std::wstring EnsureAvatarSource()
{
    std::wstring src = FindAvatarSourcePng();
    if (!src.empty())
        return src;

    const std::wstring outPath = PathJoin(ProfileCacheDir(), L"default_avatar.png");
    if (FileExists(outPath))
        return outPath;

    Gdiplus::Bitmap bmp(512, 512, PixelFormat32bppARGB);
    Gdiplus::Graphics g(&bmp);
    g.Clear(Gdiplus::Color(255, 120, 60, 180));
    Gdiplus::FontFamily family(L"Segoe UI");
    Gdiplus::Font font(&family, 180, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush brush(Gdiplus::Color(255, 255, 255, 255));
    Gdiplus::StringFormat fmt;
    fmt.SetAlignment(Gdiplus::StringAlignmentCenter);
    fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::RectF rc(0, 0, 512.f, 512.f);
    g.DrawString(SIX_SEVEN_WINDOWS_DISPLAY_NAME, -1, &font, rc, &fmt, &brush);
    CLSID clsid = {};
    if (GetEncoderClsid(L"image/png", &clsid) &&
        bmp.Save(outPath.c_str(), &clsid, nullptr) == Gdiplus::Ok && FileExists(outPath))
        return outPath;
    return {};
}

bool SaveSizedPng(Gdiplus::Bitmap* src, int size, const std::wstring& outPath)
{
    auto scaled = std::make_unique<Gdiplus::Bitmap>(size, size, PixelFormat32bppARGB);
    if (scaled->GetLastStatus() != Gdiplus::Ok)
        return false;
    Gdiplus::Graphics g(scaled.get());
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g.Clear(Gdiplus::Color(0, 0, 0, 0));
    g.DrawImage(src, 0, 0, size, size);
    CLSID clsid = {};
    if (!GetEncoderClsid(L"image/png", &clsid))
        return false;
    return scaled->Save(outPath.c_str(), &clsid, nullptr) == Gdiplus::Ok;
}

bool SaveSizedJpeg(Gdiplus::Bitmap* src, int size, const std::wstring& outPath)
{
    auto scaled = std::make_unique<Gdiplus::Bitmap>(size, size, PixelFormat24bppRGB);
    if (scaled->GetLastStatus() != Gdiplus::Ok)
        return false;
    Gdiplus::Graphics g(scaled.get());
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g.Clear(Gdiplus::Color(255, 0, 0, 0));
    g.DrawImage(src, 0, 0, size, size);
    CLSID clsid = {};
    if (!GetEncoderClsid(L"image/jpeg", &clsid))
        return false;
    ULONG quality = 95;
    Gdiplus::EncoderParameters ep = {};
    ep.Count = 1;
    ep.Parameter[0].Guid = Gdiplus::EncoderQuality;
    ep.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
    ep.Parameter[0].NumberOfValues = 1;
    ep.Parameter[0].Value = &quality;
    return scaled->Save(outPath.c_str(), &clsid, &ep) == Gdiplus::Ok;
}

void NotifyProfileUiChanged()
{
    const std::wstring userDir = UserAccountPicturesDir();
    if (!userDir.empty())
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, userDir.c_str(), nullptr);
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    DWORD_PTR result = 0;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                        reinterpret_cast<LPARAM>(L"Shell"), SMTO_ABORTIFHUNG, 5000, &result);
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                        reinterpret_cast<LPARAM>(L"UserProfile"), SMTO_ABORTIFHUNG, 5000, &result);
}

bool IsOverrideActiveInInfo()
{
    return GetPrivateProfileIntW(kProfileSection, L"active", 0, InfoPath().c_str()) != 0;
}

void SetOverrideActive(bool active)
{
    WritePrivateProfileStringW(kProfileSection, L"active", active ? L"1" : L"0",
                               InfoPath().c_str());
}

bool IsPendingBootApply()
{
    return GetPrivateProfileIntW(kProfileSection, L"pending_boot", 0, InfoPath().c_str()) != 0;
}

void SetPendingBootApply(bool pending)
{
    WritePrivateProfileStringW(kProfileSection, L"pending_boot", pending ? L"1" : L"0",
                               InfoPath().c_str());
}

bool IsProfileReverted()
{
    return GetPrivateProfileIntW(kProfileSection, L"reverted", 0, InfoPath().c_str()) != 0;
}

void SetProfileReverted(bool reverted)
{
    WritePrivateProfileStringW(kProfileSection, L"reverted", reverted ? L"1" : L"0",
                               InfoPath().c_str());
}

bool IsFreshWindowsSession()
{
    return GetTickCount64() <= 10ULL * 60ULL * 1000ULL;
}

bool RevertDisplayName(const std::wstring& sid)
{
    const std::wstring restoreName = ResolveRestoreDisplayName();
    if (restoreName.empty())
        return SetDisplayNameElevated(L"");
    return SetDisplayNameElevated(restoreName);
}

void RemoveCustomAvatarFiles()
{
    const std::wstring dir = UserAccountPicturesDir();
    if (dir.empty())
        return;
    for (int sz : kUserPictureSizes) {
        wchar_t fileName[64];
        wsprintfW(fileName, L"six_seven_%d.png", sz);
        DeleteFileW(PathJoin(dir, fileName).c_str());
    }
    DeleteFileW(PathJoin(dir, L"SixSeven.png").c_str());
}

void EnsureBackupImageFiles(const std::wstring& sid)
{
    const std::wstring info = InfoPath();
    const std::wstring sub = AccountPictureKey(sid);

    auto ensureOne = [&](int sz) {
        if (HasBackupImageFile(sz))
            return;

        wchar_t valName[32];
        wsprintfW(valName, L"Image%d", sz);
        std::wstring path;
        wchar_t buf[2048] = {};
        GetPrivateProfileStringW(kProfileSection, valName, L"", buf, 2048, info.c_str());
        if (buf[0])
            path = buf;
        else if (!(ReadRegString(HKEY_LOCAL_MACHINE, sub.c_str(), valName, path,
                                 KEY_READ | KEY_WOW64_64KEY) &&
                   !PathLooksCustomized(path)) &&
                 !(ReadRegString(HKEY_CURRENT_USER, sub.c_str(), valName, path) &&
                   !PathLooksCustomized(path)))
            return;

        if (path.empty() || PathLooksCustomized(path) || !FileExists(path))
            return;

        if (!ProfileIniHasKey(valName))
            WritePrivateProfileStringW(kProfileSection, valName, path.c_str(), info.c_str());
        StoreBackupImageFile(sz, path);
    };

    for (int sz : kUserPictureSizes)
        ensureOne(sz);
    for (int sz : kMachinePictureSizes)
        ensureOne(sz);
}

void BackupCurrentProfile(const std::wstring& sid)
{
    SaveUserSidToInfo(sid);
    const std::wstring info = InfoPath();

    if (!HasBackupSnapshot()) {
        if (!ProfileIniHasKey(L"orig_display_name")) {
            std::wstring displayName;
            if (GetCurrentDisplayName(displayName) &&
                wcscmp(displayName.c_str(), SIX_SEVEN_WINDOWS_DISPLAY_NAME) != 0) {
                WritePrivateProfileStringW(kProfileSection, L"orig_display_name",
                                           displayName.c_str(), info.c_str());
                SaveOrigDisplayNameBackup(displayName);
            }
        }
        if (!ProfileIniHasKey(L"orig_login_name")) {
            const std::wstring login = GetLoginUserName();
            if (!login.empty())
                WritePrivateProfileStringW(kProfileSection, L"orig_login_name", login.c_str(),
                                           info.c_str());
        }

        const std::wstring sub = AccountPictureKey(sid);
        auto savePath = [&](int sz, bool preferUserHive) {
            wchar_t valName[32];
            wsprintfW(valName, L"Image%d", sz);
            if (ProfileIniHasKey(valName))
                return;
            std::wstring path;
            const auto readHive = [&](HKEY hive, REGSAM extra) {
                return ReadRegString(hive, sub.c_str(), valName, path, extra) &&
                       !PathLooksCustomized(path);
            };
            if (preferUserHive) {
                if (!readHive(HKEY_CURRENT_USER, KEY_READ) &&
                    !readHive(HKEY_LOCAL_MACHINE, KEY_READ | KEY_WOW64_64KEY))
                    return;
            } else if (!readHive(HKEY_LOCAL_MACHINE, KEY_READ | KEY_WOW64_64KEY) &&
                       !readHive(HKEY_CURRENT_USER, KEY_READ))
                return;
            WritePrivateProfileStringW(kProfileSection, valName, path.c_str(), info.c_str());
        };

        for (int sz : kUserPictureSizes)
            savePath(sz, true);
        for (int sz : kMachinePictureSizes) {
            if (std::find(std::begin(kUserPictureSizes), std::end(kUserPictureSizes), sz) !=
                std::end(kUserPictureSizes))
                continue;
            savePath(sz, false);
        }

        if (!ProfileIniHasKey(L"orig_correlation_id")) {
            std::wstring guid;
            if (ReadRegString(HKEY_LOCAL_MACHINE, sub.c_str(), L"CorrelationID", guid,
                              KEY_READ | KEY_WOW64_64KEY))
                WritePrivateProfileStringW(kProfileSection, L"orig_correlation_id", guid.c_str(),
                                           info.c_str());
        }

        MarkBackupSnapshot();
    }

    EnsureBackupImageFiles(sid);
}

void ReadBackupImagePath(int size, std::wstring& out)
{
    wchar_t valName[32];
    wsprintfW(valName, L"Image%d", size);
    wchar_t buf[2048] = {};
    GetPrivateProfileStringW(kProfileSection, valName, L"", buf, 2048, InfoPath().c_str());
    out = buf;
}

bool ApplyAvatarUserHive(const std::wstring& sid, Gdiplus::Bitmap& src)
{
    const std::wstring dir = UserAccountPicturesDir();
    if (dir.empty())
        return false;

    const std::wstring sub = AccountPictureKey(sid);
    bool any = false;
    for (int sz : kUserPictureSizes) {
        wchar_t fileName[64];
        wsprintfW(fileName, L"six_seven_%d.png", sz);
        const std::wstring pngPath = PathJoin(dir, fileName);
        if (!SaveSizedPng(&src, sz, pngPath))
            continue;
        wchar_t valName[32];
        wsprintfW(valName, L"Image%d", sz);
        if (WriteRegString(HKEY_CURRENT_USER, sub.c_str(), valName, pngPath))
            any = true;
    }
    return any;
}

bool SyncMachineHiveFromUserHive(const std::wstring& sid)
{
    const std::wstring sub = AccountPictureKey(sid);
    std::vector<ImagePathEntry> images;
    for (int sz : kUserPictureSizes) {
        wchar_t valName[32];
        wsprintfW(valName, L"Image%d", sz);
        std::wstring path;
        if (ReadRegString(HKEY_CURRENT_USER, sub.c_str(), valName, path))
            images.emplace_back(sz, path);
    }
    return ApplyMachineRegistryAsSystem(sid, NewPictureGuid(), images);
}

bool ApplyAvatarMachineHive(const std::wstring& sid, Gdiplus::Bitmap& src)
{
    PreparePublicAccountDir(sid);

    const std::wstring sub = AccountPictureKey(sid);
    const std::wstring publicDir = PublicAccountDir(sid);
    std::wstring guid;
    bool any = false;
    std::vector<ImagePathEntry> written;

    for (int sz : kMachinePictureSizes) {
        wchar_t valName[32];
        wsprintfW(valName, L"Image%d", sz);
        std::wstring destPath;
        if (ReadRegString(HKEY_LOCAL_MACHINE, sub.c_str(), valName, destPath,
                          KEY_READ | KEY_WOW64_64KEY)) {
            if (guid.empty()) {
                guid = ExtractGuidFromImagePath(destPath);
                if (guid.empty())
                    guid = NewPictureGuid();
            }
        } else {
            if (guid.empty())
                guid = NewPictureGuid();
            wchar_t fileName[128];
            wsprintfW(fileName, L"%s-Image%d.jpg", guid.c_str(), sz);
            destPath = PathJoin(publicDir, fileName);
        }

        const std::wstring parent = ParentDir(destPath);
        if (!parent.empty())
            EnsureDirectoryTree(parent);
        if (!SaveSizedJpeg(&src, sz, destPath))
            continue;
        any = true;
        written.emplace_back(sz, destPath);
    }

    if (any && !guid.empty())
        ApplyMachineRegistryAsSystem(sid, guid, written);
    return any;
}

bool ApplyAvatarAll(const std::wstring& sid, const std::wstring& sourcePath, bool& userOk,
                  bool& machineOk)
{
    userOk = false;
    machineOk = false;
    Gdiplus::Bitmap src(sourcePath.c_str());
    if (src.GetLastStatus() != Gdiplus::Ok)
        return false;
    userOk = ApplyAvatarUserHive(sid, src);
    machineOk = ApplyAvatarMachineHive(sid, src);
    if (userOk && !machineOk)
        machineOk = SyncMachineHiveFromUserHive(sid);
    return userOk || machineOk;
}

bool RestoreUserHivePngs(const std::wstring& sid)
{
    const std::wstring dir = UserAccountPicturesDir();
    if (dir.empty())
        return false;

    const std::wstring sub = AccountPictureKey(sid);
    bool any = false;
    for (int sz : kUserPictureSizes) {
        if (!HasBackupImageFile(sz))
            continue;

        wchar_t outName[64];
        wsprintfW(outName, L"restored_%d.png", sz);
        const std::wstring pngPath = PathJoin(dir, outName);
        const std::wstring backup = BackupImageFilePath(sz);
        Gdiplus::Bitmap bmp(backup.c_str());
        if (bmp.GetLastStatus() != Gdiplus::Ok)
            continue;

        CLSID clsid = {};
        if (!GetEncoderClsid(L"image/png", &clsid))
            continue;
        if (bmp.Save(pngPath.c_str(), &clsid, nullptr) != Gdiplus::Ok)
            continue;

        wchar_t valName[32];
        wsprintfW(valName, L"Image%d", sz);
        if (WriteRegString(HKEY_CURRENT_USER, sub.c_str(), valName, pngPath))
            any = true;
    }
    return any;
}

bool RestoreAvatar(const std::wstring& sid)
{
    RemoveCustomAvatarFiles();

    const std::wstring sub = AccountPictureKey(sid);
    std::vector<ImagePathEntry> machineImages;
    bool filesOk = false;

    auto restoreSize = [&](int sz) {
        wchar_t valName[32];
        wsprintfW(valName, L"Image%d", sz);
        std::wstring path;
        ReadBackupImagePath(sz, path);
        if (path.empty())
            return;

        bool restored = RestoreBackupImageFile(sz, path);
        if (!restored) {
            const std::wstring fallback = DefaultWindowsUserPicture(sz);
            if (FileExists(fallback)) {
                const std::wstring parent = ParentDir(path);
                if (!parent.empty())
                    EnsureDirectoryTree(parent);
                restored = CopyFileW(fallback.c_str(), path.c_str(), FALSE) != 0;
            }
        }
        if (restored)
            filesOk = true;

        if (path.find(L":\\Users\\Public\\AccountPictures\\") != std::wstring::npos)
            machineImages.emplace_back(sz, path);
    };

    for (int sz : kUserPictureSizes)
        restoreSize(sz);
    for (int sz : kMachinePictureSizes) {
        if (std::find(std::begin(kUserPictureSizes), std::end(kUserPictureSizes), sz) !=
            std::end(kUserPictureSizes))
            continue;
        restoreSize(sz);
    }

    const bool userHiveOk = RestoreUserHivePngs(sid);

    wchar_t guid[128] = {};
    GetPrivateProfileStringW(kProfileSection, L"orig_correlation_id", L"", guid, 128,
                             InfoPath().c_str());
    bool registryOk = userHiveOk;
    if (!machineImages.empty()) {
        std::wstring corr = ExtractGuidFromImagePath(machineImages[0].second);
        if (corr.empty() && guid[0])
            corr = guid;
        if (corr.empty())
            corr = NewPictureGuid();
        WriteRegString(HKEY_CURRENT_USER, sub.c_str(), L"CorrelationID", corr);
        registryOk = ApplyMachineRegistryElevated(sid, corr, machineImages) || registryOk;
        if (!registryOk)
            registryOk = ApplyMachineRegistryAsSystem(sid, corr, machineImages);
    }

    return filesOk && (registryOk || userHiveOk);
}

std::wstring ProfileToolPath() { return PathJoin(GetExeDirectory(), kProfileTool); }

bool RunElevatedToolAndWait(const wchar_t* args)
{
    const std::wstring tool = ProfileToolPath();
    if (!FileExists(tool))
        return false;

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = tool.c_str();
    sei.lpParameters = args;
    sei.lpDirectory = GetExeDirectory().c_str();
    sei.nShow = SW_HIDE;
    if (!ShellExecuteExW(&sei))
        return false;
    if (!sei.hProcess)
        return true;

    WaitForSingleObject(sei.hProcess, 120000);
    DWORD code = 1;
    GetExitCodeProcess(sei.hProcess, &code);
    CloseHandle(sei.hProcess);
    return code == 0;
}

bool ShouldApplyAtBoot()
{
    if (IsProfileReverted())
        return false;
    if (IsPendingBootApply())
        return true;
    if (IsOverrideActiveInInfo())
        return true;
    return false;
}

} /* namespace */

bool ProfileCustomizer::IsOverrideActive() { return IsOverrideActiveInInfo(); }

void ProfileCustomizer::ScheduleApplyOnNextBoot()
{
#if SIX_SEVEN_PROFILE_OVERRIDE_ENABLED
    SetPendingBootApply(true);
    SetProfileReverted(false);
#endif
}

void ProfileCustomizer::RequestInstallBootTask()
{
#if SIX_SEVEN_PROFILE_OVERRIDE_ENABLED
    const std::wstring tool = ProfileToolPath();
    if (FileExists(tool)) {
        ShellExecuteW(nullptr, L"runas", tool.c_str(), L"--install-boot-task",
                      GetExeDirectory().c_str(), SW_HIDE);
    }
#endif
}

bool ProfileCustomizer::RemoveBootApplyTask()
{
    return RunHiddenCommand(std::wstring(L"schtasks /Delete /TN ") + kBootTask + L" /F");
}

bool ProfileCustomizer::InstallBootApplyTask()
{
#if !SIX_SEVEN_PROFILE_OVERRIDE_ENABLED
    return false;
#else
    const std::wstring tool = ProfileToolPath();
    const std::wstring dir = GetExeDirectory();
    const std::wstring bat = PathJoin(ProfileCacheDir(), L"boot_apply.cmd");
    const std::wstring script = L"@cd /d \"" + dir + L"\"\r\n@\"" + tool + L"\" --apply-boot\r\n";
    if (!WriteUtf16LeFile(bat, script))
        return false;

    ProfileCustomizer::RemoveBootApplyTask();
    std::wstring create = L"schtasks /Create /TN ";
    create += kBootTask;
    create += L" /TR \"";
    create += bat;
    create += L"\" /SC ONSTART /RU SYSTEM /RL HIGHEST /F";
    const bool ok = RunHiddenCommand(create);
    if (ok)
        LogLine(L"OK: boot task installed");
    else
        LogLine(L"FAIL: boot task install");
    return ok;
#endif
}

bool ProfileCustomizer::ApplyBootElevated()
{
#if !SIX_SEVEN_PROFILE_OVERRIDE_ENABLED
    return false;
#else
    if (!ShouldApplyAtBoot()) {
        LogLine(L"BOOT: skip");
        return true;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    std::wstring sid;
    if (!GetUserSidForApply(sid)) {
        LogLine(L"BOOT: FAIL SID");
        CoUninitialize();
        return false;
    }

    BackupCurrentProfile(sid);

    Gdiplus::GdiplusStartupInput input;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &input, nullptr) != Gdiplus::Ok) {
        LogLine(L"BOOT: FAIL Gdiplus");
        CoUninitialize();
        return false;
    }

    const std::wstring source = EnsureAvatarSource();
    if (source.empty()) {
        LogLine(L"BOOT: FAIL no source");
        Gdiplus::GdiplusShutdown(token);
        CoUninitialize();
        return false;
    }

    Gdiplus::Bitmap src(source.c_str());
    const bool machineOk = ApplyAvatarMachineHive(sid, src);
    const bool nameOk = SetDisplayNameElevated(SIX_SEVEN_WINDOWS_DISPLAY_NAME);

    if (machineOk && nameOk && !IsPendingBootApply())
        SetOverrideActive(true);
    if (machineOk || nameOk)
        SetProfileReverted(false);

    LogLine(machineOk ? L"BOOT: OK avatar" : L"BOOT: FAIL avatar");
    LogLine(nameOk ? L"BOOT: OK name" : L"BOOT: FAIL name");

    if (token)
        Gdiplus::GdiplusShutdown(token);
    CoUninitialize();
    return machineOk && nameOk;
#endif
}

void ProfileCustomizer::RequestApplyOnBootIfNeeded(bool onboarded)
{
#if !SIX_SEVEN_PROFILE_OVERRIDE_ENABLED
    (void)onboarded;
    return;
#else
    if (!onboarded)
        return;
    if (!IsFreshWindowsSession())
        return;
    if (IsProfileReverted())
        return;
    if (!IsPendingBootApply() && IsOverrideActiveInInfo())
        return;

    RequestApply();
    SetPendingBootApply(false);
#endif
}

bool ProfileCustomizer::RequestFullSetupAndWait()
{
#if !SIX_SEVEN_PROFILE_OVERRIDE_ENABLED
    return false;
#else
    SetPendingBootApply(false);
    SetProfileReverted(false);
    return RunElevatedToolAndWait(L"--setup");
#endif
}

bool ProfileCustomizer::RequestApplyAndWait()
{
#if !SIX_SEVEN_PROFILE_OVERRIDE_ENABLED
    return false;
#else
    return RunElevatedToolAndWait(L"--apply");
#endif
}

bool ProfileCustomizer::RequestRevertAndWait()
{
    return RunElevatedToolAndWait(L"--revert");
}

void ProfileCustomizer::RequestApply()
{
#if !SIX_SEVEN_PROFILE_OVERRIDE_ENABLED
    return;
#else
    const std::wstring tool = ProfileToolPath();
    if (FileExists(tool)) {
        ShellExecuteW(nullptr, L"runas", tool.c_str(), L"--apply", GetExeDirectory().c_str(),
                      SW_HIDE);
        return;
    }
    ApplyElevated();
#endif
}

void ProfileCustomizer::RequestRevert()
{
    const std::wstring tool = ProfileToolPath();
    if (FileExists(tool)) {
        ShellExecuteW(nullptr, L"runas", tool.c_str(), L"--revert", GetExeDirectory().c_str(),
                      SW_HIDE);
        return;
    }
    RevertElevated();
}

bool ProfileCustomizer::ApplyElevated()
{
#if !SIX_SEVEN_PROFILE_OVERRIDE_ENABLED
    return false;
#else
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    std::wstring sid;
    if (!GetCurrentUserSidString(sid)) {
        LogLine(L"FAIL: SID");
        CoUninitialize();
        return false;
    }

    BackupCurrentProfile(sid);

    Gdiplus::GdiplusStartupInput input;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &input, nullptr) != Gdiplus::Ok) {
        LogLine(L"FAIL: GdiplusStartup");
        CoUninitialize();
        return false;
    }

    const std::wstring source = EnsureAvatarSource();
    if (source.empty()) {
        LogLine(L"FAIL: no avatar source");
        Gdiplus::GdiplusShutdown(token);
        CoUninitialize();
        return false;
    }

    bool userAvatarOk = false;
    bool machineAvatarOk = false;
    const bool avatarOk = ApplyAvatarAll(sid, source, userAvatarOk, machineAvatarOk);
    const bool nameOk = SetDisplayNameElevated(SIX_SEVEN_WINDOWS_DISPLAY_NAME);

    if (avatarOk && nameOk) {
        SetOverrideActive(true);
        SetProfileReverted(false);
        SetPendingBootApply(false);
        InstallBootApplyTask();
    }
    NotifyProfileUiChanged();
    if (avatarOk)
        RestartExplorer();

    LogLine(userAvatarOk ? L"OK: avatar (AppData/HKCU)" : L"FAIL: avatar (AppData/HKCU)");
    LogLine(machineAvatarOk ? L"OK: avatar (Public/HKLM)" : L"FAIL: avatar (Public/HKLM)");
    LogLine(nameOk ? L"OK: name" : L"FAIL: name");

    if (token)
        Gdiplus::GdiplusShutdown(token);
    CoUninitialize();
    return avatarOk && nameOk;
#endif
}

bool ProfileCustomizer::RevertElevated()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    Gdiplus::GdiplusStartupInput gdiInput;
    ULONG_PTR gdiToken = 0;
    Gdiplus::GdiplusStartup(&gdiToken, &gdiInput, nullptr);

    std::wstring sid;
    if (!GetCurrentUserSidString(sid)) {
        LogLine(L"REVERT: FAIL SID");
        if (gdiToken)
            Gdiplus::GdiplusShutdown(gdiToken);
        CoUninitialize();
        return false;
    }

    const bool stillCustomized =
        IsNameStillCustomized(sid) || IsAvatarStillCustomized(sid);
    if (!IsOverrideActiveInInfo() && !stillCustomized) {
        LogLine(L"REVERT: skip (already restored)");
        if (gdiToken)
            Gdiplus::GdiplusShutdown(gdiToken);
        CoUninitialize();
        return true;
    }
    if (!IsOverrideActiveInInfo())
        LogLine(L"REVERT: force (registry still customized)");

    (void)RevertDisplayName(sid);
    (void)RestoreAvatar(sid);

    const bool nameOk = !IsNameStillCustomized(sid);
    const bool avatarOk = !IsAvatarStillCustomized(sid);

    SetOverrideActive(false);
    SetPendingBootApply(false);
    SetProfileReverted(true);
    RemoveBootApplyTask();
    NotifyProfileUiChanged();
    if (avatarOk || nameOk)
        RestartExplorer();

    LogLine(nameOk ? L"REVERT: OK name" : L"REVERT: FAIL name");
    LogLine(avatarOk ? L"REVERT: OK avatar" : L"REVERT: FAIL avatar");

    if (gdiToken)
        Gdiplus::GdiplusShutdown(gdiToken);
    CoUninitialize();
    return nameOk && avatarOk;
}

} /* namespace six_seven */
