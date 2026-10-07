#include "../include/Util.h"
#include "../config.h"

#include <windows.h>
#include <shlobj.h>

#include <cstdlib>
#include <cwctype>
#include <fstream>
#include <algorithm>

namespace six_seven {

std::wstring GetExeDirectory()
{
    wchar_t buf[MAX_PATH] = {};
    const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return L".";
    std::wstring path(buf, n);
    const auto slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos)
        path.resize(slash);
    return path;
}

std::wstring Utf8ToWide(const char* utf8)
{
    if (!utf8 || !*utf8)
        return {};
    const int need = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (need <= 0)
        return {};
    std::wstring out(static_cast<size_t>(need), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, out.data(), need);
    if (!out.empty() && out.back() == L'\0')
        out.pop_back();
    return out;
}

std::string WideToUtf8(const wchar_t* wide)
{
    if (!wide || !*wide)
        return {};
    const int need = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (need <= 0)
        return {};
    std::string out(static_cast<size_t>(need), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, out.data(), need, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0')
        out.pop_back();
    return out;
}

std::string CaesarShiftImpl(const std::string& text, int shift)
{
    std::string out;
    out.reserve(text.size());
    for (unsigned char c : text) {
        if (c >= 'a' && c <= 'z') {
            const int base = 'a';
            out.push_back(static_cast<char>(base + (c - base + shift) % 26 + (c - base + shift >= 0 ? 0 : 26)));
            continue;
        }
        if (c >= 'A' && c <= 'Z') {
            const int base = 'A';
            out.push_back(static_cast<char>(base + (c - base + shift) % 26 + (c - base + shift >= 0 ? 0 : 26)));
            continue;
        }
        if (c >= '0' && c <= '9') {
            const int base = '0';
            out.push_back(static_cast<char>(base + (c - base + shift) % 10 + (c - base + shift >= 0 ? 0 : 10)));
            continue;
        }
        out.push_back(static_cast<char>(c));
    }
    return out;
}

std::string CaesarShiftEncode(const std::string& text, int shift)
{
    shift %= 26;
    return CaesarShiftImpl(text, shift);
}

std::string CaesarShiftDecode(const std::string& text, int shift)
{
    shift %= 26;
    return CaesarShiftImpl(text, -shift);
}

std::wstring PathJoin(const std::wstring& a, const std::wstring& b)
{
    if (a.empty())
        return b;
    if (b.empty())
        return a;
    if (a.back() == L'\\' || a.back() == L'/')
        return a + b;
    return a + L'\\' + b;
}

std::wstring AssetPath(const char* relativeUtf8)
{
    return PathJoin(GetExeDirectory(), Utf8ToWide(relativeUtf8));
}

bool FileExists(const std::wstring& path)
{
    const DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool WriteTextFile(const std::wstring& path, const std::string& utf8Contents, bool addBom)
{
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    bool ok = true;
    if (addBom) {
        const char bom[] = "\xEF\xBB\xBF";
        DWORD written = 0;
        if (!WriteFile(h, bom, 3, &written, nullptr) || written != 3)
            ok = false;
    }
    if (ok) {
        const DWORD size = static_cast<DWORD>(utf8Contents.size());
        DWORD written = 0;
        if (size > 0 && (!WriteFile(h, utf8Contents.data(), size, &written, nullptr) ||
                         written != size))
            ok = false;
    }
    CloseHandle(h);
    return ok;
}

int RandomInt(int minInclusive, int maxInclusive)
{
    if (maxInclusive <= minInclusive)
        return minInclusive;
    return minInclusive + (std::rand() % (maxInclusive - minInclusive + 1));
}

const SixSevenActionDef& PickRandom(const SixSevenActionDef* arr, int count)
{
    if (count <= 0) {
        static SixSevenActionDef empty{};
        return empty;
    }
    return arr[RandomInt(0, count - 1)];
}

namespace {

struct WorkAreaUnion {
    RECT rc = {};
    bool empty = true;
};

BOOL CALLBACK UnionWorkAreaProc(HMONITOR hMon, HDC, LPRECT, LPARAM lp)
{
    auto* u = reinterpret_cast<WorkAreaUnion*>(lp);
    MONITORINFO mi = { sizeof(mi) };
    if (!GetMonitorInfoW(hMon, &mi))
        return TRUE;
    const RECT& w = mi.rcWork;
    if (u->empty) {
        u->rc = w;
        u->empty = false;
    } else {
        u->rc.left = std::min(u->rc.left, w.left);
        u->rc.top = std::min(u->rc.top, w.top);
        u->rc.right = std::max(u->rc.right, w.right);
        u->rc.bottom = std::max(u->rc.bottom, w.bottom);
    }
    return TRUE;
}

} /* namespace */

RECT GetCombinedWorkArea()
{
    WorkAreaUnion u;
    EnumDisplayMonitors(nullptr, nullptr, UnionWorkAreaProc, reinterpret_cast<LPARAM>(&u));
    if (!u.empty)
        return u.rc;
    RECT fallback = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &fallback, 0);
    return fallback;
}

void ClampWindowToWorkArea(int& x, int& y, int width, int height, const POINT* followPoint)
{
    POINT probe = {};
    if (followPoint)
        probe = *followPoint;
    else
        probe = { x + width / 2, y + height / 2 };

    RECT wr = {};
    if (!GetWorkAreaAtPoint(probe, wr))
        wr = GetCombinedWorkArea();

    if (x < wr.left)
        x = wr.left;
    if (y < wr.top)
        y = wr.top;
    if (x > wr.right - width)
        x = std::max(wr.left, wr.right - width);
    if (y > wr.bottom - height)
        y = std::max(wr.top, wr.bottom - height);
}

bool GetWorkAreaAtPoint(const POINT& pt, RECT& outWork)
{
    HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    if (!GetMonitorInfoW(mon, &mi))
        return false;
    outWork = mi.rcWork;
    return true;
}

void ClampWindowToSpriteWorkArea(int& winX, int& winY, int spriteDrawX, int spriteDrawY,
                                 int spriteW, int spriteH, const POINT* spriteAnchor)
{
    if (spriteW <= 0)
        spriteW = 1;
    if (spriteH <= 0)
        spriteH = 1;

    POINT probe = {};
    if (spriteAnchor)
        probe = *spriteAnchor;
    else
        probe = { winX + spriteDrawX + spriteW / 2, winY + spriteDrawY + spriteH / 2 };

    RECT wr = {};
    if (!GetWorkAreaAtPoint(probe, wr))
        wr = GetCombinedWorkArea();

    const int m = SIX_SEVEN_DRAG_EDGE_MARGIN;
    const int minX = wr.left + m - spriteDrawX;
    const int maxX = wr.right - m - spriteDrawX - spriteW;
    const int minY = wr.top + m - spriteDrawY;
    const int maxY = wr.bottom - m - spriteDrawY - spriteH;

    if (minX <= maxX)
        winX = std::max(minX, std::min(winX, maxX));
    else
        winX = minX;

    if (minY <= maxY)
        winY = std::max(minY, std::min(winY, maxY));
    else
        winY = minY;
}

namespace {

std::wstring TrimWide(std::wstring s)
{
    while (!s.empty() && (s.front() == L' ' || s.front() == L'\t' || s.front() == L'\r' ||
                          s.front() == L'\n'))
        s.erase(s.begin());
    while (!s.empty() && (s.back() == L' ' || s.back() == L'\t' || s.back() == L'\r' ||
                          s.back() == L'\n'))
        s.pop_back();
    return s;
}

} /* namespace */

void EnsureAdminPasswordFile()
{
    const std::wstring path = PathJoin(GetExeDirectory(), L"password");
    std::ofstream out(WideToUtf8(path.c_str()), std::ios::binary | std::ios::trunc);
    if (!out)
        return;
    out << SIX_SEVEN_ADMIN_PASSWORD << "\r\n";
}

bool VerifyAdminPassword(const std::wstring& attempt)
{
    const std::wstring expected = TrimWide(Utf8ToWide(SIX_SEVEN_ADMIN_PASSWORD));
    return !expected.empty() && attempt == expected;
}

void RandomSpriteWindowPos(int& winX, int& winY, int marginPx)
{
    const RECT wr = GetCombinedWorkArea();
    const int loX = wr.left + marginPx - SIX_SEVEN_SPRITE_DRAW_X;
    const int hiX = wr.right - marginPx - SIX_SEVEN_SPRITE_DRAW_X - SIX_SEVEN_SPRITE_WIDTH;
    const int loY = wr.top + marginPx - SIX_SEVEN_SPRITE_DRAW_Y;
    const int hiY = wr.bottom - marginPx - SIX_SEVEN_SPRITE_DRAW_Y - SIX_SEVEN_SPRITE_HEIGHT;
    winX = (loX >= hiX) ? loX : RandomInt(loX, hiX);
    winY = (loY >= hiY) ? loY : RandomInt(loY, hiY);
    POINT anchor = { winX + SIX_SEVEN_SPRITE_DRAW_X + SIX_SEVEN_SPRITE_WIDTH / 2,
                     winY + SIX_SEVEN_SPRITE_DRAW_Y + SIX_SEVEN_SPRITE_HEIGHT / 2 };
    ClampWindowToSpriteWorkArea(winX, winY, SIX_SEVEN_SPRITE_DRAW_X, SIX_SEVEN_SPRITE_DRAW_Y,
                                SIX_SEVEN_SPRITE_WIDTH, SIX_SEVEN_SPRITE_HEIGHT, &anchor);
}

void CenterSpriteWindow(int& winX, int& winY)
{
    const RECT wr = GetCombinedWorkArea();
    winX = wr.left + (wr.right - wr.left - SIX_SEVEN_WINDOW_WIDTH) / 2;
    winY = wr.top + (wr.bottom - wr.top - SIX_SEVEN_WINDOW_HEIGHT) / 2;
    POINT anchor = { winX + SIX_SEVEN_SPRITE_DRAW_X + SIX_SEVEN_SPRITE_WIDTH / 2,
                     winY + SIX_SEVEN_SPRITE_DRAW_Y + SIX_SEVEN_SPRITE_HEIGHT / 2 };
    ClampWindowToSpriteWorkArea(winX, winY, SIX_SEVEN_SPRITE_DRAW_X, SIX_SEVEN_SPRITE_DRAW_Y,
                                SIX_SEVEN_SPRITE_WIDTH, SIX_SEVEN_SPRITE_HEIGHT, &anchor);
}

void CenterWindowOnScreen(HWND hwnd, int width, int height)
{
    if (!hwnd || width <= 0 || height <= 0)
        return;
    const RECT wr = GetCombinedWorkArea();
    const int x = wr.left + (wr.right - wr.left - width) / 2;
    const int y = wr.top + (wr.bottom - wr.top - height) / 2;
    SetWindowPos(hwnd, nullptr, x, y, width, height, SWP_NOZORDER);
}

std::wstring BaseName(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos)
        return path;
    return path.substr(slash + 1);
}

std::wstring GetDesktopPath()
{
    wchar_t desktop[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, SHGFP_TYPE_CURRENT,
                                desktop)))
        return {};
    return desktop;
}

namespace {

bool IsUsableFolderEntry(const wchar_t* name)
{
    return name[0] != L'.' && wcscmp(name, L".") != 0 && wcscmp(name, L"..") != 0;
}

std::wstring LowercaseCopy(const std::wstring& s)
{
    std::wstring out = s;
    for (auto& ch : out)
        ch = static_cast<wchar_t>(towlower(ch));
    return out;
}

} /* namespace */

std::vector<std::wstring> ListSubdirectories(const std::wstring& dir)
{
    std::vector<std::wstring> out;
    if (dir.empty())
        return out;
    WIN32_FIND_DATAW fd = {};
    HANDLE h = FindFirstFileW(PathJoin(dir, L"*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE)
        return out;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        if (!IsUsableFolderEntry(fd.cFileName))
            continue;
        out.push_back(PathJoin(dir, fd.cFileName));
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return out;
}

std::vector<std::wstring> ListFilesInDirectory(const std::wstring& dir)
{
    std::vector<std::wstring> out;
    if (dir.empty())
        return out;
    WIN32_FIND_DATAW fd = {};
    HANDLE h = FindFirstFileW(PathJoin(dir, L"*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE)
        return out;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        if (!IsUsableFolderEntry(fd.cFileName))
            continue;
        out.push_back(PathJoin(dir, fd.cFileName));
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return out;
}

std::wstring PickDesktopFolder(int depth)
{
    const std::wstring desktop = GetDesktopPath();
    if (desktop.empty())
        return {};
    if (depth < 1)
        depth = 1;

    std::wstring dir = desktop;
    for (int level = 0; level < depth; ++level) {
        std::vector<std::wstring> subs = ListSubdirectories(dir);
        if (depth == 1) {
            if (subs.empty())
                break;
            dir = subs[RandomInt(0, static_cast<int>(subs.size()) - 1)];
            continue;
        }
        std::wstring next;
        const bool wantExisting = subs.empty() || RandomInt(0, 1) == 0;
        if (wantExisting && !subs.empty()) {
            next = subs[RandomInt(0, static_cast<int>(subs.size()) - 1)];
        } else {
            static const wchar_t* kNewNames[] = {
                L"документы", L"старое",    L"архив",   L"бэкап",     L"разное",
                L"ненужное",  L"проекты",   L"фото",    L"новая папка", L"old_stuff",
                L"backup",    L"misc",      L"stuff",   L"очередное",
            };
            next = PathJoin(dir, kNewNames[RandomInt(0, static_cast<int>(std::size(kNewNames)) - 1)]);
            if (!CreateDirectoryW(next.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
                if (!subs.empty())
                    next = subs[RandomInt(0, static_cast<int>(subs.size()) - 1)];
                else
                    break;
            }
        }
        dir = next;
    }
    return dir;
}

std::wstring UniquePathInFolder(const std::wstring& dir, const std::wstring& fileName)
{
    std::wstring candidate = PathJoin(dir, fileName);
    if (!FileExists(candidate))
        return candidate;

    size_t dot = fileName.find_last_of(L'.');
    const size_t slash = fileName.find_last_of(L"\\/");
    if (slash != std::wstring::npos && (dot == std::wstring::npos || dot < slash))
        dot = std::wstring::npos;
    const std::wstring base = dot == std::wstring::npos ? fileName : fileName.substr(0, dot);
    const std::wstring ext = dot == std::wstring::npos ? L"" : fileName.substr(dot);

    for (int i = 2; i < 1000; ++i) {
        const std::wstring name =
            base + L" (" + std::to_wstring(i) + L")" + ext;
        candidate = PathJoin(dir, name);
        if (!FileExists(candidate))
            return candidate;
    }
    return PathJoin(dir, fileName);
}

bool IsFileOpenByOtherProcess(const std::wstring& path)
{
    if (path.empty())
        return false;
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        const DWORD err = GetLastError();
        return err == ERROR_SHARING_VIOLATION || err == ERROR_LOCK_VIOLATION;
    }
    CloseHandle(h);
    return false;
}

namespace {

struct TitleSearchData {
    std::wstring needleLower;
    DWORD selfPid = 0;
    bool found = false;
};

struct TitleCloseData {
    std::wstring needleLower;
    DWORD selfPid = 0;
    int closed = 0;
};

bool TitleContainsNeedle(const std::wstring& titleLower, const std::wstring& needleLower)
{
    return !needleLower.empty() && titleLower.find(needleLower) != std::wstring::npos;
}

BOOL CALLBACK FindTitleProc(HWND hwnd, LPARAM lp)
{
    auto* s = reinterpret_cast<TitleSearchData*>(lp);
    if (!IsWindowVisible(hwnd))
        return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == s->selfPid)
        return TRUE;
    wchar_t title[512] = {};
    const int n = GetWindowTextW(hwnd, title, 512);
    if (n <= 0)
        return TRUE;
    if (TitleContainsNeedle(LowercaseCopy(std::wstring(title, static_cast<size_t>(n))),
                            s->needleLower)) {
        s->found = true;
        return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK CloseEditorTitleProc(HWND hwnd, LPARAM lp)
{
    auto* s = reinterpret_cast<TitleCloseData*>(lp);
    if (!IsWindowVisible(hwnd))
        return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == s->selfPid)
        return TRUE;
    wchar_t cls[128] = {};
    GetClassNameW(hwnd, cls, 128);
    const bool editor = wcscmp(cls, L"Notepad") == 0 || wcscmp(cls, L"WordPadClass") == 0 ||
                        wcscmp(cls, L"OpusApp") == 0 || wcscmp(cls, L"XLMAIN") == 0;
    if (!editor)
        return TRUE;
    wchar_t title[512] = {};
    const int n = GetWindowTextW(hwnd, title, 512);
    if (n <= 0)
        return TRUE;
    if (TitleContainsNeedle(LowercaseCopy(std::wstring(title, static_cast<size_t>(n))),
                            s->needleLower)) {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
        s->closed += 1;
    }
    return TRUE;
}

} /* namespace */

bool WindowTitleContains(const std::wstring& needle)
{
    if (needle.empty())
        return false;
    TitleSearchData data;
    data.needleLower = LowercaseCopy(needle);
    data.selfPid = GetCurrentProcessId();
    EnumWindows(FindTitleProc, reinterpret_cast<LPARAM>(&data));
    return data.found;
}

int CloseEditorWindowsTitled(const std::wstring& needle)
{
    if (needle.empty())
        return 0;
    TitleCloseData data;
    data.needleLower = LowercaseCopy(needle);
    data.selfPid = GetCurrentProcessId();
    EnumWindows(CloseEditorTitleProc, reinterpret_cast<LPARAM>(&data));
    return data.closed;
}

} /* namespace six_seven */
