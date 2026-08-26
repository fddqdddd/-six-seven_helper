#include "../include/Util.h"
#include "../config.h"

#include <windows.h>

#include <cstdlib>
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

} /* namespace six_seven */
