#include "../include/Application.h"

#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include "../include/BootLogoInstaller.h"
#include "../include/BootSplash.h"
#include "../include/ProfileCustomizer.h"
#include "../include/ConfigData.h"
#include "../include/NameValidator.h"
#include "../include/Phrases.h"
#include "../include/SongCatalog.h"
#include "../include/Songs.h"
#include "../include/TerminalCommands.h"
#include "../include/Util.h"
#include "../config.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

namespace six_seven {

Application* Application::instance_ = nullptr;

enum TimerId {
    kFrame = 1,
    kDefCheck = 2,
    kTerminalCheck = 4,
};

enum MenuCmd {
    kClickActionBase = 100,
    kMenuChangeColor = 2001,
    kMenuSongs = 2002,
    kMenuAdmin = 2003,
    kMenuBootLogoInstall = 2004,
    kMenuBootLogoUninstall = 2005,
    kMenuProfileRevert = 2006,
    kMenuExit = 2007,
    kMenuRestartOnboarding = 2008,
    kMenuAdminPanel = 2009,
    kAdminBtnBootLogoInstall = 3101,
    kAdminBtnBootLogoUninstall = 3102,
    kAdminBtnProfileRevert = 3103,
    kAdminBtnTerminalUnlock = 3104,
    kAdminBtnCommands = 3105,
    kTrayAutostartToggle = 1006,
    kColorTrackbar = 3001,
    kNameEdit = 3002,
    kColorCombo = 3003,
    kColorOk = 3004,
    kSeasonCombo = 3009,
    kSeasonOk = 3010,
    kFoodEdit = 3011,
    kSongsList = 3012,
    kSongsSing = 3013,
    kBirthdayCalendar = 3005,
    kBirthdayYearEdit = 3006,
    kBirthdayOk = 3007,
    kAdminPasswordEdit = 3008,
    kMiniGameClickSixSeven = 2101,
    kMiniGameMemoryShell = 2102,
    kMiniGameGuessNumber = 2103,
    kMiniGameRps = 2104,
    kMiniGameHideSeek = 2105,
    kMenuRecords = 2200,
    kMenuCoolGamesLimboKeys = 2301,
    kMenuLeaveServeFile = 2401,
    kMenuLeaveGift = 2402,
    kMenuVaultHide = 2403,
    kMenuVaultRestore = 2404,
};

struct NameDialogData {
    Application* app = nullptr;
    HWND edit = nullptr;
    bool accepted = false;
    std::wstring name;
};

struct ColorDialogData {
    Application* app = nullptr;
    HWND combo = nullptr;
    HWND okBtn = nullptr;
    bool accepted = false;
    std::wstring color;
};

struct AdminDialogData {
    Application* app = nullptr;
    HWND edit = nullptr;
    bool accepted = false;
    std::wstring password;
};

struct AdminPanelData {
    Application* app = nullptr;
};

struct CommandsDialogData {
    Application* app = nullptr;
};

struct BirthdayDialogData {
    Application* app = nullptr;
    HWND calendar = nullptr;
    HWND yearEdit = nullptr;
    HWND okBtn = nullptr;
    bool accepted = false;
    bool hasSelection = false;
    int month = 0;
    int day = 0;
    int year = 0;
};

int CurrentCalendarYear()
{
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    return static_cast<int>(st.wYear);
}

int ClampBirthYear(int year)
{
    const int currentYear = CurrentCalendarYear();
    if (year < 1900)
        return 1900;
    if (year > currentYear)
        return currentYear;
    return year;
}

void UpdateBirthdayCalendarForYear(HWND calendar, int year)
{
    if (!calendar)
        return;
    year = ClampBirthYear(year);
    const int currentYear = CurrentCalendarYear();

    SYSTEMTIME minRange = {};
    minRange.wYear = 1900;
    minRange.wMonth = 1;
    minRange.wDay = 1;
    SYSTEMTIME maxRange = {};
    maxRange.wYear = static_cast<WORD>(currentYear);
    maxRange.wMonth = 12;
    maxRange.wDay = 31;
    SendMessageW(calendar, MCM_SETRANGE, GDTR_MIN, reinterpret_cast<LPARAM>(&minRange));
    SendMessageW(calendar, MCM_SETRANGE, GDTR_MAX, reinterpret_cast<LPARAM>(&maxRange));

    SYSTEMTIME sel = {};
    if (SendMessageW(calendar, MCM_GETCURSEL, 0, reinterpret_cast<LPARAM>(&sel)) != GDT_VALID) {
        GetLocalTime(&sel);
    }
    sel.wYear = static_cast<WORD>(year);
    if (sel.wMonth < 1)
        sel.wMonth = 1;
    if (sel.wDay < 1)
        sel.wDay = 1;
    SendMessageW(calendar, MCM_SETCURSEL, 0, reinterpret_cast<LPARAM>(&sel));
}

bool Application::AcceptBirthdayDialog(HWND hwnd, BirthdayDialogData* data)
{
    if (!hwnd || !data || !data->calendar || !data->yearEdit)
        return false;

    wchar_t yearBuf[16] = {};
    GetWindowTextW(data->yearEdit, yearBuf, 16);
    while (yearBuf[0] == L' ' || yearBuf[0] == L'\t') {
        wmemmove(yearBuf, yearBuf + 1, wcslen(yearBuf));
    }
    if (!yearBuf[0])
        return false;

    const size_t yearLen = wcslen(yearBuf);
    int year = _wtoi(yearBuf);
    if (yearLen < 4 || year < 1900 || year > CurrentCalendarYear()) {
        MessageBeep(MB_ICONWARNING);
        SetFocus(data->yearEdit);
        return false;
    }

    int month = 0;
    int day = 0;
    if (data->hasSelection) {
        month = data->month;
        day = data->day;
    } else {
        SYSTEMTIME range[2] = {};
        if (SendMessageW(data->calendar, MCM_GETSELRANGE, 0, reinterpret_cast<LPARAM>(range))) {
            month = static_cast<int>(range[0].wMonth);
            day = static_cast<int>(range[0].wDay);
        } else {
            SYSTEMTIME sel = {};
            if (SendMessageW(data->calendar, MCM_GETCURSEL, 0,
                             reinterpret_cast<LPARAM>(&sel)) == GDT_VALID) {
                month = static_cast<int>(sel.wMonth);
                day = static_cast<int>(sel.wDay);
            } else {
                SYSTEMTIME today = {};
                GetLocalTime(&today);
                month = static_cast<int>(today.wMonth);
                day = static_cast<int>(today.wDay);
            }
        }
    }

    if (month < 1 || month > 12 || day < 1)
        return false;

    data->year = year;
    data->month = month;
    data->day = day;
    data->accepted = true;
    if (data->app) {
        data->app->pendingBirthdayDialogAccepted_ = true;
        data->app->pendingBirthdayMonth_ = month;
        data->app->pendingBirthdayDay_ = day;
        data->app->pendingBirthdayYear_ = year;
    }
    DestroyWindow(hwnd);
    return true;
}

void Application::LayoutBirthdayDialog(HWND hwnd, BirthdayDialogData* data)
{
    if (!hwnd || !data || !data->calendar)
        return;

    RECT calRc = {};
    GetWindowRect(data->calendar, &calRc);
    POINT calBottom = { calRc.left, calRc.bottom };
    ScreenToClient(hwnd, &calBottom);

    const int okY = calBottom.y + 10;
    if (data->okBtn)
        SetWindowPos(data->okBtn, nullptr, 110, okY, 80, 28, SWP_NOZORDER);

    RECT wr = {};
    GetWindowRect(hwnd, &wr);
    RECT client = { 0, 0, 304, okY + 48 };
    AdjustWindowRectEx(&client, GetWindowLong(hwnd, GWL_STYLE), FALSE,
                       GetWindowLong(hwnd, GWL_EXSTYLE));
    const int winW = client.right - client.left;
    const int winH = client.bottom - client.top;
    SetWindowPos(hwnd, nullptr, wr.left, wr.top, winW, winH, SWP_NOZORDER);
}

static const wchar_t* kFavoriteColors[] = {
    L"Красный",   L"Оранжевый", L"Жёлтый",  L"Зелёный", L"Голубой",
    L"Синий",     L"Фиолетовый", L"Белый",  L"Чёрный",  L"Радужный",
};

static const wchar_t* kFavoriteSeasons[] = {
    L"Зима", L"Весна", L"Лето", L"Осень",
};

struct SeasonDialogData {
    Application* app = nullptr;
    HWND combo = nullptr;
    HWND okBtn = nullptr;
    int preselect = 2;
    bool accepted = false;
    std::wstring season;
};

struct FoodDialogData {
    Application* app = nullptr;
    HWND edit = nullptr;
    bool accepted = false;
    std::wstring food;
};

struct SongsDialogData {
    Application* app = nullptr;
    HWND list = nullptr;
    HWND singBtn = nullptr;
    std::vector<int> indices;
};

const char* FirstRunDirectionSprite(int dx, int dy)
{
    const bool up = dy < 0;
    const bool down = dy > 0;
    const bool left = dx < 0;
    const bool right = dx > 0;
    if (up && right)
        return MOD_SPRITE_MOVING_RU;
    if (up && left)
        return MOD_SPRITE_MOVING_LU;
    if (down && right)
        return MOD_SPRITE_MOVING_RD;
    if (down && left)
        return MOD_SPRITE_MOVING_LD;
    if (up)
        return MOD_SPRITE_MOVING_U;
    if (down)
        return MOD_SPRITE_MOVING_D;
    if (left)
        return MOD_SPRITE_MOVING_L;
    if (right)
        return MOD_SPRITE_MOVING_R;
    return MOD_SPRITE_MOVING_R;
}

float SmoothStep(float t)
{
    if (t <= 0.0f)
        return 0.0f;
    if (t >= 1.0f)
        return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

int PickVariedWanderAngle(int lastAngleDeg)
{
    if (lastAngleDeg < 0)
        return RandomInt(0, 359);

    for (int attempt = 0; attempt < 16; ++attempt) {
        int angle = 0;
        const int roll = RandomInt(0, 99);
        if (roll < 35)
            angle = RandomInt(0, 359);
        else if (roll < 70)
            angle = (lastAngleDeg + RandomInt(70, 160) * (RandomInt(0, 1) ? 1 : -1) + 360) % 360;
        else
            angle = (lastAngleDeg + RandomInt(160, 220) * (RandomInt(0, 1) ? 1 : -1) + 360) % 360;

        const int delta = (angle - lastAngleDeg + 360) % 360;
        const int reverseDist = std::abs(delta - 180);
        if (reverseDist >= 55 && reverseDist <= 305)
            return angle;
    }
    return (lastAngleDeg + RandomInt(100, 260)) % 360;
}

struct ColorPickerData {
    Application* app = nullptr;
    HWND trackbar = nullptr;
};

LRESULT CALLBACK Application::ColorPickerWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<ColorPickerData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        data = new ColorPickerData();
        data->app = static_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        CreateWindowExW(0, L"STATIC", L"Оттенок:", WS_CHILD | WS_VISIBLE, 16, 12, 280, 18,
                        hwnd, nullptr, cs->hInstance, nullptr);
        data->trackbar =
            CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ, 16, 32, 280, 30,
                            hwnd, reinterpret_cast<HMENU>(kColorTrackbar), cs->hInstance, nullptr);
        SendMessageW(data->trackbar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 360));
        SendMessageW(data->trackbar, TBM_SETTICFREQ, 36, 0);
        if (data->app)
            SendMessageW(data->trackbar, TBM_SETPOS, TRUE, data->app->sprites_.HueShift());
        CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 118, 72,
                        80, 24, hwnd, reinterpret_cast<HMENU>(IDOK), cs->hInstance, nullptr);
        return 0;
    }
    case WM_HSCROLL:
        if (data && data->trackbar && data->app &&
            reinterpret_cast<HWND>(lp) == data->trackbar) {
            const int hue = static_cast<int>(SendMessageW(data->trackbar, TBM_GETPOS, 0, 0));
            data->app->sprites_.SetHueShift(hue);
            data->app->Paint();
        }
        return 0;
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (data) {
            if (data->app)
                data->app->colorPickerHwnd_ = nullptr;
            delete data;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK Application::AdminDialogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<AdminDialogData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        data = new AdminDialogData();
        data->app = static_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        CreateWindowExW(0, L"STATIC", L"Пароль администратора:", WS_CHILD | WS_VISIBLE, 16, 12,
                        260, 18, hwnd, nullptr, cs->hInstance, nullptr);
        data->edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                     WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_PASSWORD, 16,
                                     34, 260, 24, hwnd,
                                     reinterpret_cast<HMENU>(kAdminPasswordEdit), cs->hInstance,
                                     nullptr);
        SendMessageW(data->edit, EM_SETLIMITTEXT, 64, 0);
        CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 108, 68,
                        80, 24, hwnd, reinterpret_cast<HMENU>(IDOK), cs->hInstance, nullptr);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK && data && data->edit) {
            wchar_t buf[128] = {};
            GetWindowTextW(data->edit, buf, 128);
            data->password = buf;
            data->accepted = true;
            if (data->app) {
                data->app->pendingAdminDialogAccepted_ = true;
                data->app->pendingAdminDialogValue_ = data->password;
            }
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (data) {
            delete data;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void Application::LayoutAdminPanel(HWND hwnd)
{
    if (!hwnd)
        return;
    const bool admin = userInfo_.IsAdmin();
    const bool profileRevert = admin && ProfileCustomizer::IsOverrideActive();
    const bool terminalUnlocked = userInfo_.IsTerminalUnlocked();
    const bool commands = userInfo_.AreCommandsUnlocked();

    int y = 16;
    const int btnW = 260;
    const int btnH = 28;
    const int gap = 8;
    auto place = [&](UINT id, const wchar_t* label, bool visible) {
        HWND btn = GetDlgItem(hwnd, id);
        if (!btn)
            return;
        if (visible) {
            SetWindowTextW(btn, label);
            SetWindowPos(btn, nullptr, 16, y, btnW, btnH, SWP_NOZORDER | SWP_SHOWWINDOW);
            y += btnH + gap;
        } else {
            ShowWindow(btn, SW_HIDE);
        }
    };

    place(kAdminBtnBootLogoInstall, L"Установить логотип загрузки ПК", admin);
    place(kAdminBtnBootLogoUninstall, L"Убрать логотип загрузки ПК", admin);
    place(kAdminBtnProfileRevert, L"Вернуть имя и аватар Windows", profileRevert);
    place(kAdminBtnTerminalUnlock, L"Снять ограничения терминала", admin && !terminalUnlocked);
    place(kAdminBtnCommands, L"Commands", admin && commands);

    RECT wr = {};
    GetWindowRect(hwnd, &wr);
    const int winH = y + 16;
    const int winW = 304;
    SetWindowPos(hwnd, nullptr, wr.left, wr.top, winW, winH, SWP_NOZORDER);
    CenterWindowOnScreen(hwnd, winW, winH);
}

LRESULT CALLBACK Application::AdminPanelDialogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<AdminPanelData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        data = new AdminPanelData();
        data->app = static_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        const DWORD style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
        CreateWindowExW(0, L"BUTTON", L"", style, 0, 0, 10, 10, hwnd,
                        reinterpret_cast<HMENU>(kAdminBtnBootLogoInstall), cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"", style, 0, 0, 10, 10, hwnd,
                        reinterpret_cast<HMENU>(kAdminBtnBootLogoUninstall), cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"", style, 0, 0, 10, 10, hwnd,
                        reinterpret_cast<HMENU>(kAdminBtnProfileRevert), cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"", style, 0, 0, 10, 10, hwnd,
                        reinterpret_cast<HMENU>(kAdminBtnTerminalUnlock), cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"", style, 0, 0, 10, 10, hwnd,
                        reinterpret_cast<HMENU>(kAdminBtnCommands), cs->hInstance, nullptr);
        if (data->app)
            data->app->LayoutAdminPanel(hwnd);
        return 0;
    }
    case WM_COMMAND: {
        if (!data || !data->app)
            break;
        const UINT cmd = LOWORD(wp);
        if (cmd == kAdminBtnBootLogoInstall) {
            BootLogoInstaller::LaunchInstallElevated();
            return 0;
        }
        if (cmd == kAdminBtnBootLogoUninstall) {
            BootLogoInstaller::LaunchUninstallElevated();
            return 0;
        }
        if (cmd == kAdminBtnProfileRevert) {
            data->app->RevertWindowsProfile();
            return 0;
        }
        if (cmd == kAdminBtnTerminalUnlock) {
            data->app->UnlockTerminalRestrictions();
            data->app->LayoutAdminPanel(hwnd);
            return 0;
        }
        if (cmd == kAdminBtnCommands) {
            data->app->ShowCommandsDialog();
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (data) {
            delete data;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK Application::CommandsDialogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<CommandsDialogData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        data = new CommandsDialogData();
        data->app = static_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        const wchar_t* text =
            L"six-seven_open — открыть окно 67\r\n"
            L"kill_67 — завершить процесс 67 (не удаляет с диска)\r\n"
            L"sleep67 — усыпить 67, чтение книги или сон (случайно)\r\n"
            L"67move — переместить 67, как при долгой скуке\r\n"
            L"шестьсемь отзовись — позвать 67\r\n"
            L"погладить 67 — погладить 67\r\n"
            L"покажи глюк — глюк?\r\n"
            L"убери стол — попросить 67 прибраться\r\n";
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", text,
                        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL, 12, 12,
                        360, 176, hwnd, nullptr, cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 152, 200,
                        80, 26, hwnd, reinterpret_cast<HMENU>(IDOK), cs->hInstance, nullptr);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (data) {
            delete data;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK Application::NameDialogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<NameDialogData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        data = new NameDialogData();
        data->app = static_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        CreateWindowExW(0, L"STATIC", L"Как тебя зовут?", WS_CHILD | WS_VISIBLE, 16, 12, 260,
                        18, hwnd, nullptr, cs->hInstance, nullptr);
        data->edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                     WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 16, 34, 260, 24,
                                     hwnd, reinterpret_cast<HMENU>(kNameEdit), cs->hInstance,
                                     nullptr);
        SendMessageW(data->edit, EM_SETLIMITTEXT, 64, 0);
        CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 108, 68,
                        80, 24, hwnd, reinterpret_cast<HMENU>(IDOK), cs->hInstance, nullptr);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK && data && data->edit) {
            wchar_t buf[128] = {};
            GetWindowTextW(data->edit, buf, 128);
            std::wstring name(buf);
            while (!name.empty() && (name.front() == L' ' || name.front() == L'\t'))
                name.erase(name.begin());
            while (!name.empty() && (name.back() == L' ' || name.back() == L'\t'))
                name.pop_back();
            if (name.empty())
                return 0;
            data->name = name;
            data->accepted = true;
            if (data->app) {
                data->app->pendingNameDialogAccepted_ = true;
                data->app->pendingNameDialogValue_ = name;
            }
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (data) {
            delete data;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK Application::ColorDialogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<ColorDialogData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        data = new ColorDialogData();
        data->app = static_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        CreateWindowExW(0, L"STATIC", L"Выбери любимый цвет:", WS_CHILD | WS_VISIBLE, 16, 12,
                        260, 18, hwnd, nullptr, cs->hInstance, nullptr);
        data->combo = CreateWindowExW(0, L"COMBOBOX", L"",
                                      WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 16,
                                      34, 260, 200, hwnd,
                                      reinterpret_cast<HMENU>(kColorCombo), cs->hInstance, nullptr);
        for (const wchar_t* color : kFavoriteColors)
            SendMessageW(data->combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(color));
        data->okBtn =
            CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_DISABLED, 108, 72, 80,
                            24, hwnd, reinterpret_cast<HMENU>(kColorOk), cs->hInstance, nullptr);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == kColorCombo && HIWORD(wp) == CBN_SELCHANGE && data && data->combo &&
            data->okBtn) {
            const int sel = static_cast<int>(SendMessageW(data->combo, CB_GETCURSEL, 0, 0));
            EnableWindow(data->okBtn, sel != CB_ERR);
            return 0;
        }
        if (LOWORD(wp) == kColorOk && data && data->combo) {
            const int sel = static_cast<int>(SendMessageW(data->combo, CB_GETCURSEL, 0, 0));
            if (sel == CB_ERR)
                return 0;
            wchar_t buf[64] = {};
            SendMessageW(data->combo, CB_GETLBTEXT, sel, reinterpret_cast<LPARAM>(buf));
            data->color = buf;
            data->accepted = true;
            if (data->app) {
                data->app->pendingColorDialogAccepted_ = true;
                data->app->pendingColorDialogValue_ = buf;
            }
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (data) {
            delete data;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK Application::SeasonDialogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<SeasonDialogData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        data = new SeasonDialogData();
        data->app = static_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        CreateWindowExW(0, L"STATIC", L"Выбери любимое время года:", WS_CHILD | WS_VISIBLE, 16, 12,
                        260, 18, hwnd, nullptr, cs->hInstance, nullptr);
        data->combo = CreateWindowExW(0, L"COMBOBOX", L"",
                                      WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 16,
                                      34, 260, 200, hwnd,
                                      reinterpret_cast<HMENU>(kSeasonCombo), cs->hInstance, nullptr);
        for (const wchar_t* season : kFavoriteSeasons)
            SendMessageW(data->combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(season));
        if (data->app) {
            data->preselect =
                UserInformation::SeasonIndex(data->app->seasonDialogSuggested_);
        }
        SendMessageW(data->combo, CB_SETCURSEL, data->preselect, 0);
        data->okBtn =
            CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 108, 72,
                            80, 24, hwnd, reinterpret_cast<HMENU>(kSeasonOk), cs->hInstance,
                            nullptr);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == kSeasonOk && data && data->combo) {
            const int sel = static_cast<int>(SendMessageW(data->combo, CB_GETCURSEL, 0, 0));
            if (sel == CB_ERR)
                return 0;
            wchar_t buf[32] = {};
            SendMessageW(data->combo, CB_GETLBTEXT, sel, reinterpret_cast<LPARAM>(buf));
            data->season = buf;
            data->accepted = true;
            if (data->app) {
                data->app->pendingSeasonDialogAccepted_ = true;
                data->app->pendingSeasonDialogValue_ = buf;
            }
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (data) {
            delete data;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK Application::FoodDialogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<FoodDialogData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        data = new FoodDialogData();
        data->app = static_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        CreateWindowExW(0, L"STATIC", L"Какая твоя любимая еда?", WS_CHILD | WS_VISIBLE, 16, 12,
                        260, 18, hwnd, nullptr, cs->hInstance, nullptr);
        data->edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                       WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 16, 34, 260, 24,
                                       hwnd, reinterpret_cast<HMENU>(kFoodEdit), cs->hInstance,
                                       nullptr);
        SendMessageW(data->edit, EM_SETLIMITTEXT, 64, 0);
        CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 108, 68,
                        80, 24, hwnd, reinterpret_cast<HMENU>(IDOK), cs->hInstance, nullptr);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK && data && data->edit) {
            wchar_t buf[128] = {};
            GetWindowTextW(data->edit, buf, 128);
            std::wstring food(buf);
            while (!food.empty() && (food.front() == L' ' || food.front() == L'\t'))
                food.erase(food.begin());
            while (!food.empty() && (food.back() == L' ' || food.back() == L'\t'))
                food.pop_back();
            if (food.empty())
                return 0;
            data->food = food;
            data->accepted = true;
            if (data->app) {
                data->app->pendingFoodDialogAccepted_ = true;
                data->app->pendingFoodDialogValue_ = food;
            }
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (data) {
            delete data;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK Application::SongsDialogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<SongsDialogData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        data = new SongsDialogData();
        data->app = static_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        CreateWindowExW(0, L"STATIC", L"Выбери песню:", WS_CHILD | WS_VISIBLE, 12, 10, 276, 18,
                        hwnd, nullptr, cs->hInstance, nullptr);
        data->list = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                                       WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | LBS_NOTIFY,
                                       12, 32, 276, 180, hwnd,
                                       reinterpret_cast<HMENU>(kSongsList), cs->hInstance, nullptr);
        if (data->app && data->list) {
            FillAvailableSongIndices(&data->app->userInfo_, data->indices);
            for (int idx : data->indices)
                SendMessageW(data->list, LB_ADDSTRING, 0,
                           reinterpret_cast<LPARAM>(GetSongs()[static_cast<size_t>(idx)].title.c_str()));
            if (!data->indices.empty())
                SendMessageW(data->list, LB_SETCURSEL, 0, 0);
        }
        data->singBtn = CreateWindowExW(
            0, L"BUTTON", L"Спеть",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON |
                (data->indices.empty() ? WS_DISABLED : 0),
            108, 222, 88, 28, hwnd, reinterpret_cast<HMENU>(kSongsSing), cs->hInstance, nullptr);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == kSongsList && HIWORD(wp) == LBN_DBLCLK && data && data->list) {
            SendMessageW(hwnd, WM_COMMAND, MAKEWPARAM(kSongsSing, BN_CLICKED), 0);
            return 0;
        }
        if (LOWORD(wp) == kSongsSing && data && data->list && !data->indices.empty()) {
            const int sel = static_cast<int>(SendMessageW(data->list, LB_GETCURSEL, 0, 0));
            if (sel == LB_ERR || sel < 0 || sel >= static_cast<int>(data->indices.size()))
                return 0;
            if (data->app)
                data->app->RunSongByIndex(data->indices[static_cast<size_t>(sel)]);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (data) {
            delete data;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK Application::BirthdayDialogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<BirthdayDialogData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        data = new BirthdayDialogData();
        data->app = static_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        CreateWindowExW(0, L"STATIC", L"Год рождения:", WS_CHILD | WS_VISIBLE, 12, 10, 120, 18,
                        hwnd, nullptr, cs->hInstance, nullptr);
        data->yearEdit =
            CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, 12, 30, 80, 24, hwnd,
                            reinterpret_cast<HMENU>(kBirthdayYearEdit), cs->hInstance, nullptr);
        SendMessageW(data->yearEdit, EM_SETLIMITTEXT, 4, 0);
        wchar_t yearText[8] = {};
        wsprintfW(yearText, L"%d", CurrentCalendarYear());
        SetWindowTextW(data->yearEdit, yearText);
        CreateWindowExW(0, L"STATIC", L"Выбери день и месяц:", WS_CHILD | WS_VISIBLE, 12, 60, 276,
                        18, hwnd, nullptr, cs->hInstance, nullptr);
        data->calendar =
            CreateWindowExW(0, MONTHCAL_CLASSW, L"",
                            WS_CHILD | WS_VISIBLE | MCS_DAYSTATE | WS_TABSTOP, 8, 80, 276, 180,
                            hwnd, reinterpret_cast<HMENU>(kBirthdayCalendar), cs->hInstance,
                            nullptr);
        if (data->calendar) {
            SendMessageW(data->calendar, MCM_SETMAXSELCOUNT, 0, 1);
            UpdateBirthdayCalendarForYear(data->calendar, CurrentCalendarYear());
            SYSTEMTIME sel = {};
            if (SendMessageW(data->calendar, MCM_GETCURSEL, 0, reinterpret_cast<LPARAM>(&sel)) ==
                GDT_VALID) {
                data->month = static_cast<int>(sel.wMonth);
                data->day = static_cast<int>(sel.wDay);
                data->hasSelection = true;
            }
        }
        data->okBtn =
            CreateWindowExW(0, L"BUTTON", L"OK",
                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON | WS_CLIPSIBLINGS,
                            110, 280, 80, 28, hwnd, reinterpret_cast<HMENU>(kBirthdayOk),
                            cs->hInstance, nullptr);
        LayoutBirthdayDialog(hwnd, data);
        if (data->okBtn)
            SetFocus(data->okBtn);
        return 0;
    }
    case WM_NOTIFY:
        if (data && data->calendar) {
            const auto* nm = reinterpret_cast<NMHDR*>(lp);
            if (nm->hwndFrom == data->calendar &&
                (nm->code == MCN_SELECT || nm->code == MCN_SELCHANGE)) {
                const auto* change = reinterpret_cast<const NMSELCHANGE*>(lp);
                data->month = static_cast<int>(change->stSelStart.wMonth);
                data->day = static_cast<int>(change->stSelStart.wDay);
                data->hasSelection = data->month >= 1 && data->day >= 1;
                return 0;
            }
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wp) == kBirthdayYearEdit && HIWORD(wp) == EN_CHANGE && data && data->calendar &&
            data->yearEdit) {
            wchar_t yearBuf[16] = {};
            GetWindowTextW(data->yearEdit, yearBuf, 16);
            if (yearBuf[0]) {
                const int year = ClampBirthYear(_wtoi(yearBuf));
                UpdateBirthdayCalendarForYear(data->calendar, year);
            }
            return 0;
        }
        if (LOWORD(wp) == kBirthdayOk && data) {
            AcceptBirthdayDialog(hwnd, data);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (wp == VK_RETURN && data) {
            AcceptBirthdayDialog(hwnd, data);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (data) {
            delete data;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int Application::Run(HINSTANCE inst)
{
    instance_ = this;
    inst_ = inst;
    std::srand(static_cast<unsigned>(GetTickCount()));
    if (!Init(inst))
        return 1;
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    Shutdown();
    instance_ = nullptr;
    return static_cast<int>(msg.wParam);
}

void Application::ClampToWorkArea(int& x, int& y, const POINT* followPoint)
{
    int drawX = 0;
    int drawY = 0;
    GetSpriteDrawPos(drawX, drawY);
    int spriteW = sprites_.FrameWidth();
    int spriteH = sprites_.FrameHeight();
    if (spriteW <= 0 || spriteH <= 0) {
        spriteW = SIX_SEVEN_SPRITE_WIDTH;
        spriteH = SIX_SEVEN_SPRITE_HEIGHT;
        drawX = SIX_SEVEN_SPRITE_DRAW_X;
        drawY = SIX_SEVEN_SPRITE_DRAW_Y;
    }
    ClampWindowToSpriteWorkArea(x, y, drawX, drawY, spriteW, spriteH, followPoint);
}

int Application::RandomWorkX()
{
    const RECT wr = GetCombinedWorkArea();
    const int m = SIX_SEVEN_DRAG_EDGE_MARGIN;
    const int lo = wr.left + m - SIX_SEVEN_SPRITE_DRAW_X;
    const int hi = wr.right - m - SIX_SEVEN_SPRITE_DRAW_X - SIX_SEVEN_SPRITE_WIDTH;
    if (lo >= hi)
        return lo;
    return RandomInt(lo, hi);
}

int Application::RandomWorkY()
{
    const RECT wr = GetCombinedWorkArea();
    const int m = SIX_SEVEN_DRAG_EDGE_MARGIN;
    const int lo = wr.top + m - SIX_SEVEN_SPRITE_DRAW_Y;
    const int hi = wr.bottom - m - SIX_SEVEN_SPRITE_DRAW_Y - SIX_SEVEN_SPRITE_HEIGHT;
    if (lo >= hi)
        return lo;
    return RandomInt(lo, hi);
}

bool Application::Init(HINSTANCE inst)
{
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
        return false;

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES | ICC_DATE_CLASSES };
    InitCommonControlsEx(&icc);

    EnsureAdminPasswordFile();

    AppSettings wsEarly;
    settings_.Load(wsEarly);
#if SIX_SEVEN_BOOT_AUTOSTART_ENABLED
    BootSplash::SyncAutostart(settings_.IsAutostartEnabled());
#endif
#if SIX_SEVEN_BOOT_SPLASH_ENABLED
    {
        BootSplash splash;
        splash.Run(inst);
        bootSplashShown_ = BootSplash::WasShown();
    }
#endif

    {
        WNDCLASSEXW cwc = {};
        cwc.cbSize = sizeof(cwc);
        cwc.lpfnWndProc = ColorPickerWndProc;
        cwc.hInstance = inst;
        cwc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        cwc.lpszClassName = L"SixSevenColorPicker";
        RegisterClassExW(&cwc);
    }
    {
        WNDCLASSEXW nwc = {};
        nwc.cbSize = sizeof(nwc);
        nwc.lpfnWndProc = NameDialogWndProc;
        nwc.hInstance = inst;
        nwc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        nwc.lpszClassName = L"SixSevenNameDialog";
        RegisterClassExW(&nwc);
    }
    {
        WNDCLASSEXW cwc2 = {};
        cwc2.cbSize = sizeof(cwc2);
        cwc2.lpfnWndProc = ColorDialogWndProc;
        cwc2.hInstance = inst;
        cwc2.hCursor = LoadCursor(nullptr, IDC_ARROW);
        cwc2.lpszClassName = L"SixSevenColorDialog";
        RegisterClassExW(&cwc2);
    }
    {
        WNDCLASSEXW swc = {};
        swc.cbSize = sizeof(swc);
        swc.lpfnWndProc = SeasonDialogWndProc;
        swc.hInstance = inst;
        swc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        swc.lpszClassName = L"SixSevenSeasonDialog";
        RegisterClassExW(&swc);
    }
    {
        WNDCLASSEXW fwc = {};
        fwc.cbSize = sizeof(fwc);
        fwc.lpfnWndProc = FoodDialogWndProc;
        fwc.hInstance = inst;
        fwc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        fwc.lpszClassName = L"SixSevenFoodDialog";
        RegisterClassExW(&fwc);
    }
    {
        WNDCLASSEXW swc2 = {};
        swc2.cbSize = sizeof(swc2);
        swc2.lpfnWndProc = SongsDialogWndProc;
        swc2.hInstance = inst;
        swc2.hCursor = LoadCursor(nullptr, IDC_ARROW);
        swc2.lpszClassName = L"SixSevenSongsDialog";
        RegisterClassExW(&swc2);
    }
    {
        WNDCLASSEXW bwc = {};
        bwc.cbSize = sizeof(bwc);
        bwc.lpfnWndProc = BirthdayDialogWndProc;
        bwc.hInstance = inst;
        bwc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        bwc.lpszClassName = L"SixSevenBirthdayDialog";
        RegisterClassExW(&bwc);
    }
    {
        WNDCLASSEXW awc = {};
        awc.cbSize = sizeof(awc);
        awc.lpfnWndProc = AdminDialogWndProc;
        awc.hInstance = inst;
        awc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        awc.lpszClassName = L"SixSevenAdminDialog";
        RegisterClassExW(&awc);
    }
    {
        WNDCLASSEXW apwc = {};
        apwc.cbSize = sizeof(apwc);
        apwc.lpfnWndProc = AdminPanelDialogWndProc;
        apwc.hInstance = inst;
        apwc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        apwc.lpszClassName = L"SixSevenAdminPanel";
        RegisterClassExW(&apwc);
    }
    {
        WNDCLASSEXW cwc3 = {};
        cwc3.cbSize = sizeof(cwc3);
        cwc3.lpfnWndProc = CommandsDialogWndProc;
        cwc3.hInstance = inst;
        cwc3.hCursor = LoadCursor(nullptr, IDC_ARROW);
        cwc3.lpszClassName = L"SixSevenCommandsDialog";
        RegisterClassExW(&cwc3);
    }

    userInfo_.Load();
    if (userInfo_.AreCommandsUnlocked())
        InstallTerminalCommandStubs();
    if (userInfo_.IsOnboarded()) {
        ProfileCustomizer::RequestApplyOnBootIfNeeded(true);
    }
    RefreshMood();

    if (!sprites_.Init())
        return false;
    speech_.Init();
    if (!LoadSongCatalog()) {
        MessageBoxW(nullptr, L"Не удалось загрузить песни (assets/songs/songs.h).", L"Six_Seven",
                    MB_OK | MB_ICONWARNING);
    }
    timeLastFire_.assign(static_cast<size_t>(kTimeActionCount), 0);

    muted_ = wsEarly.mute;
    bubble_.SetMaxLines(wsEarly.bubbleMaxLines);
    idleBreath_ = wsEarly.idleBreath;
    defDelayMinMs_ = wsEarly.defDelayMinSec * 1000;
    defDelayMaxMs_ = wsEarly.defDelayMaxSec * 1000;
    anger_ = wsEarly.anger;
    vaultFragments_ = wsEarly.vaultFragments;
    nextAngerDecayAt_ = GetTickCount() + SIX_SEVEN_ANGRY_DECAY_INTERVAL_MS;
    nextAngerSaveAt_ = GetTickCount() + SIX_SEVEN_ANGRY_SAVE_EVERY_MS;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"SixSevenOverlay";
    RegisterClassExW(&wc);

    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW;
    if (SIX_SEVEN_ALWAYS_ON_TOP)
        exStyle |= WS_EX_TOPMOST;

    int x = wsEarly.x, y = wsEarly.y;
    if (x < 0 || y < 0) {
        x = RandomWorkX();
        y = RandomWorkY();
    }

    hwnd_ = CreateWindowExW(exStyle, wc.lpszClassName, L"Six_Seven", WS_POPUP, x, y,
                            SIX_SEVEN_WINDOW_WIDTH, SIX_SEVEN_WINDOW_HEIGHT, nullptr,
                            nullptr, inst, nullptr);
    if (!hwnd_)
        return false;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = SIX_SEVEN_WINDOW_WIDTH;
    bi.bmiHeader.biHeight = -SIX_SEVEN_WINDOW_HEIGHT;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    HDC screen = GetDC(nullptr);
    dib_ = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &dibBits_, nullptr, 0);
    dibDc_ = CreateCompatibleDC(screen);
    SelectObject(dibDc_, dib_);
    ReleaseDC(nullptr, screen);

    actions_.Bind(
        hwnd_, &sprites_, &audio_, &speech_, &bubble_, &movement_, &muted_, &idleBreath_,
        &userInfo_, [this]() { ScheduleDef(); },
        [this]() {
            if (shuttingDown_)
                OnShutdownChainNext();
            else if (firstRunAwaitingAppearance_)
                OnFirstRunAppearanceDone();
            else if (!startupDone_)
                OnStartupChainNext();
        },
        [this]() { ForceQuit(); }, [this]() { Paint(); });
    actions_.SetWanderWalkSec(wsEarly.wanderWalkSec);
    miniGames_.Bind(this);
    miniGames_.LoadRecords();

    tray_.Create(hwnd_, inst);
    SetTimer(hwnd_, kFrame, 16, nullptr);
    SetTimer(hwnd_, kDefCheck, 1000, nullptr);
    SetTimer(hwnd_, kTerminalCheck, 400, nullptr);
#if SIX_SEVEN_TYPING_HOOK_ENABLED
    keyboardHook_ = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, inst, 0);
#endif

    terminalGuard_.Bind(
        &userInfo_,
        [this]() { OnTerminalBlocked(); },
        [this]() { OnUserTerminalOpened(); },
        [this](const std::wstring& cmd) { ExecuteTerminalCommand(cmd); });

    sprites_.SetSprite(SIX_SEVEN_STAY_SPRITE, idleBreath_);
    {
        RECT rc = {};
        GetWindowRect(hwnd_, &rc);
        int px = rc.left;
        int py = rc.top;
        ClampToWorkArea(px, py);
        if (px != rc.left || py != rc.top)
            SetWindowPos(hwnd_, nullptr, px, py, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    ShowWindow(hwnd_, SW_SHOW);
    lastActivity_ = GetTickCount();
    ScheduleDef();
    if (!userInfo_.IsOnboarded())
        StartFirstRun();
    else
        StartStartupChain();
    Paint();
    return true;
}

void Application::Shutdown()
{
    miniGames_.Stop();
#if SIX_SEVEN_TYPING_HOOK_ENABLED
    if (keyboardHook_) {
        UnhookWindowsHookEx(keyboardHook_);
        keyboardHook_ = nullptr;
    }
#endif
    if (anger_ > 0)
        settings_.SaveAnger(anger_);
    if (hwnd_) {
        RECT rc = {};
        GetWindowRect(hwnd_, &rc);
        settings_.Save(rc.left, rc.top, muted_, idleBreath_);
        KillTimer(hwnd_, kFrame);
        KillTimer(hwnd_, kDefCheck);
    }
    tray_.Destroy();
    speech_.Shutdown();
    sprites_.Shutdown();
    if (dibDc_)
        DeleteDC(dibDc_);
    if (dib_)
        DeleteObject(dib_);
    CoUninitialize();
}

void Application::ClampMainWindow(int& x, int& y, const POINT* anchor)
{
    ClampToWorkArea(x, y, anchor);
}

void Application::ReturnToIdleSprite()
{
    actions_.ReturnToStay();
}

void Application::GetSpriteDrawPos(int& drawX, int& drawY) const
{
    const int w = SIX_SEVEN_WINDOW_WIDTH;
    const int h = SIX_SEVEN_WINDOW_HEIGHT;
    drawX = (w - SIX_SEVEN_SPRITE_WIDTH) / 2;
    drawY = std::max(SIX_SEVEN_BUBBLE_SPACE_H, h - SIX_SEVEN_SPRITE_HEIGHT - SIX_SEVEN_WINDOW_PAD_BOTTOM);
}

bool Application::IsInteractiveAt(int clientX, int clientY) const
{
    return clientX >= 0 && clientY >= 0 && clientX < SIX_SEVEN_WINDOW_WIDTH &&
           clientY < SIX_SEVEN_WINDOW_HEIGHT;
}

void Application::Paint()
{
    if (!dibDc_ || !dibBits_ || !hwnd_)
        return;
    const int w = SIX_SEVEN_WINDOW_WIDTH;
    const int h = SIX_SEVEN_WINDOW_HEIGHT;
    auto* px = static_cast<BYTE*>(dibBits_);
    const int total = w * h;
    for (int i = 0; i < total; ++i) {
        px[i * 4 + 0] = 0;
        px[i * 4 + 1] = 0;
        px[i * 4 + 2] = 0;
        px[i * 4 + 3] = 0;
    }

    int spriteTop = h;
    int drawX = 0, drawY = 0;
    GetSpriteDrawPos(drawX, drawY);

    sprites_.Draw(dibDc_, drawX, drawY, h, spriteTop);
    if (!miniGames_.IsActive())
        bubble_.Draw(dibDc_, w, spriteTop);
#if SIX_SEVEN_BUBBLE_ENABLED
    if (!miniGames_.IsActive() && bubble_.IsVisible())
        bubble_.ApplyLayerAlpha(px, w, h);
#endif

    POINT ptSrc = { 0, 0 };
    SIZE size = { w, h };
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    POINT ptDst = {};
    RECT wr = {};
    GetWindowRect(hwnd_, &wr);
    ptDst.x = wr.left;
    ptDst.y = wr.top;
    HDC screen = GetDC(nullptr);
    UpdateLayeredWindow(hwnd_, screen, &ptDst, &size, dibDc_, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(nullptr, screen);
}

void Application::NoteUserActivity()
{
    lastActivity_ = GetTickCount();
    if (actions_.IsPersistent())
        return;
    ScheduleDef();
    if (SIX_SEVEN_TIME_RESET_ON_USER_INPUT) {
        const DWORD now = GetTickCount();
        for (size_t i = 0; i < timeLastFire_.size(); ++i)
            timeLastFire_[i] = now;
    }
}

void Application::ScheduleDef()
{
    nextDefAt_ = GetTickCount() + static_cast<DWORD>(
        RandomInt(defDelayMinMs_, defDelayMaxMs_));
}

void Application::FireDefIfDue()
{
    if (!startupDone_ || shuttingDown_ || actions_.IsBusy() || firstRunActive_)
        return;
    if (GetTickCount() < nextDefAt_)
        return;
    if (SIX_SEVEN_ANGRY_ENABLED && anger_ >= SIX_SEVEN_ANGRY_MISBEHAVE_MIN &&
        RandomInt(1, 100) <= 35) {
        // «Безобразия»: злой уход прогуляться со звуком.
        SixSevenActionDef rage{};
        rage.type = SixSevenActionType::Def;
        rage.id = "rage";
        rage.sprite_path = MOD_SPRITE_DEF_SPEAK;
        rage.sprite_mode = SixSevenSpriteMode::oneshot;
        rage.on_finish = SixSevenOnFinish::ReturnStay;
        rage.phrase_file = MOD_PHRASES_DEF_ANGRY;
        rage.sound = MOD_SOUND_DEF_ANGRY;
        rage.move = true;
        rage.dictors = true;
        actions_.Run(rage);
        ScheduleDef();
        return;
    }
    if (kSleepActionCount > 0 && SIX_SEVEN_DEF_SLEEP_CHANCE > 0 &&
        RandomInt(1, 100) <= SIX_SEVEN_DEF_SLEEP_CHANCE) {
        actions_.Run(PickRandom(kSleepActions, kSleepActionCount));
    } else if (kDefActionCount > 0) {
        const SixSevenActionDef base = PickRandom(kDefActions, kDefActionCount);
        SixSevenActionDef chosen = base;
        if (SIX_SEVEN_ANGRY_ENABLED && anger_ >= SIX_SEVEN_ANGRY_DEF_MIN &&
            base.type == SixSevenActionType::Def && base.phrase_file &&
            *base.phrase_file) {
            chosen.phrase_file = MOD_PHRASES_DEF_ANGRY;
        }
        actions_.Run(chosen);
    }
    ScheduleDef();
}

void Application::FireTimeActions()
{
    if (!startupDone_ || shuttingDown_ || actions_.IsBusy() || firstRunActive_)
        return;
    const DWORD now = GetTickCount();
    for (int i = 0; i < kTimeActionCount; ++i) {
        const auto& t = kTimeActions[i];
        if (timeLastFire_[static_cast<size_t>(i)] == 0)
            timeLastFire_[static_cast<size_t>(i)] = now;
        if (now - timeLastFire_[static_cast<size_t>(i)] >= t.delay_ms) {
            timeLastFire_[static_cast<size_t>(i)] = now;
            actions_.Run(t);
            break;
        }
    }
}

void Application::StartStartupChain()
{
    startupDone_ = false;
    shuttingDown_ = false;
    leavingStarted_ = false;
    if (bootSplashShown_) {
        OnStartupChainNext();
        return;
    }
    if (kAppearanceActionCount > 0)
        actions_.Run(PickRandom(kAppearanceActions, kAppearanceActionCount));
    else
        OnStartupChainNext();
}

void Application::OnStartupChainNext()
{
    startupDone_ = true;
    if (kHelloActionCount > 0) {
        const SixSevenActionDef& base = PickRandom(kHelloActions, kHelloActionCount);
        SixSevenActionDef ctx = base;
        const char* contextFile = PickHelloContextPhraseFile();
        if (contextFile && *contextFile)
            ctx.phrase_file = contextFile;
        actions_.Run(ctx);
    } else {
        actions_.ReturnToStay();
        ScheduleDef();
    }
}

void Application::RefreshMood()
{
    int mood = 65;
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    const int month = static_cast<int>(st.wMonth);
    const int day = static_cast<int>(st.wDay);
    const int hour = static_cast<int>(st.wHour);

    if (userInfo_.IsBirthdayToday()) {
        mood = 100;
    } else if ((month == 1 && day == 1) || (month == 12 && day == 31)) {
        mood = 95;
    } else if (month == 2 && day == 23) {
        mood = 92;
    } else if (month == 3 && day == 8) {
        mood = 96;
    } else {
        if (hour >= 5 && hour < 11)
            mood = 78;
        else if (hour >= 11 && hour < 18)
            mood = 65;
        else if (hour >= 18 && hour < 23)
            mood = 85;
        else
            mood = 40;
    }
    mood_ = mood;
    settings_.SaveMood(mood);
}

LRESULT CALLBACK Application::KeyboardHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        Application* app = instance_;
        if (app) {
            const DWORD now = GetTickCount();
            if (now - app->loudTypingWindowStart_ > SIX_SEVEN_TYPING_MS)
                app->loudTypingHits_ = 0;
            if (app->loudTypingWindowStart_ == 0)
                app->loudTypingWindowStart_ = now;
            ++app->loudTypingHits_;
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

void Application::TickLoudTyping()
{
    if (!SIX_SEVEN_ANGRY_ENABLED || shuttingDown_ || firstRunActive_ ||
        miniGames_.IsActive())
        return;
    const DWORD now = GetTickCount();
    if (loudTypingWindowStart_ == 0)
        return;
    if (now - loudTypingWindowStart_ > SIX_SEVEN_TYPING_MS) {
        if (loudTypingHits_ >= SIX_SEVEN_TYPING_THRESHOLD && now >= nextLoudTypingAt_) {
            const std::wstring text = LoadRandomLine(MOD_PHRASES_DEF_TYPING);
            if (!text.empty()) {
                SpeakNotice(text);
                nextLoudTypingAt_ = now + SIX_SEVEN_TYPING_COOLDOWN_MS;
                if (SIX_SEVEN_ANGRY_ENABLED) {
                    anger_ += SIX_SEVEN_ANGRY_SOURCE_TYPING;
                    if (anger_ > 100)
                        anger_ = 100;
                }
            }
        }
        loudTypingWindowStart_ = 0;
        loudTypingHits_ = 0;
    }
}

void Application::TickAnger()
{
    if (!SIX_SEVEN_ANGRY_ENABLED || shuttingDown_ || firstRunActive_)
        return;
    const DWORD now = GetTickCount();
    if (anger_ > 0 && now >= nextAngerDecayAt_) {
        nextAngerDecayAt_ = now + SIX_SEVEN_ANGRY_DECAY_INTERVAL_MS;
        anger_ -= SIX_SEVEN_ANGRY_DECAY_STEP;
        if (anger_ < 0)
            anger_ = 0;
        if (anger_ > 0)
            settings_.SaveAnger(anger_);
    }
}

void Application::SaveAngerIfNeeded()
{
    const DWORD now = GetTickCount();
    if (now < nextAngerSaveAt_)
        return;
    nextAngerSaveAt_ = now + SIX_SEVEN_ANGRY_SAVE_EVERY_MS;
    if (anger_ > 0)
        settings_.SaveAnger(anger_);
}

void Application::TickCadPanic()
{
    if (!SIX_SEVEN_CAD_PANIC_ENABLED || shuttingDown_)
        return;
    if (miniGames_.IsActive() || firstRunActive_ || dragging_)
        return;
    const DWORD now = GetTickCount();
    HDESK input = OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
    HDESK current = GetThreadDesktop(GetCurrentThreadId());
    if (!input || !current) {
        if (input)
            CloseDesktop(input);
        return;
    }
    wchar_t inputName[256] = {};
    wchar_t currentName[256] = {};
    DWORD needed = 0;
    const bool inputOk =
        GetUserObjectInformationW(input, UOI_NAME, inputName,
                                  static_cast<DWORD>(sizeof(inputName)), &needed) != 0;
    needed = 0;
    const bool currentOk =
        GetUserObjectInformationW(current, UOI_NAME, currentName,
                                  static_cast<DWORD>(sizeof(currentName)), &needed) != 0;
    bool onSecure = false;
    if (inputOk && currentOk)
        onSecure = _wcsicmp(inputName, currentName) != 0;
    CloseDesktop(input);
    if (onSecure && onCadDesktop_ == false) {
        onCadDesktop_ = true;
        cadDesktopSince_ = now;
        cadPanicHidden_ = false;
        return;
    }
    if (!onSecure && onCadDesktop_) {
        // Вернулись с защищённого рабочего стола (Ctrl+Alt+Del зажат/отпущен).
        onCadDesktop_ = false;
        if (now >= nextCadPanicAt_) {
            if (IsWindowVisible(hwnd_)) {
                ShowWindow(hwnd_, SW_HIDE);
                cadPanicHidden_ = true;
            }
            nextCadPanicAt_ = now + SIX_SEVEN_CAD_COOLDOWN_MS;
            return;
        }
    }
    if (cadPanicHidden_ && now - cadDesktopSince_ > 400) {
        cadPanicHidden_ = false;
        ShowWindow(hwnd_, SW_SHOW);
        SpeakNotice(SIX_SEVEN_CAD_PANIC_PHRASE);
    }
}

void Application::WriteServeFileToDesktop()
{
    wchar_t desktop[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, SHGFP_TYPE_CURRENT,
                                desktop))) {
        SpeakNotice(L"Куда положить? Наш рабочий стол не нашёлся...");
        return;
    }
    std::wstring path = PathJoin(desktop, L"Я_вижу_всё.txt");
    std::string body;
    body += "Я_вижу_всё.txt\n";
    body += "================\n";
    body += "Список того, что 67 заметила за сегодня:\n\n";
    body += "  - сколько раз ты открыл браузер:   ********\n";
    body += "  - сколько раз посмотрел пароль:    ********\n";
    body += "  - сколько раз поднял бровь:        ********\n";
    body += "  - сколько раз хотел выключить 67:  ********\n\n";
    body += "Не бойся, friend. Я никому не скажу.\n";
    body += "Пока.\n";
    std::ofstream out(WideToUtf8(path.c_str()).c_str(), std::ios::binary);
    if (out) {
        out << "\xEF\xBB\xBF";
        out.write(body.data(), static_cast<std::streamsize>(body.size()));
        SpeakNotice(L"Готово. Файл на столе. Проверь, друг.");
    } else {
        SpeakNotice(L"Ой, не получилось. Наверное, стол занят.");
    }
}

void Application::WriteGiftToDesktop()
{
    wchar_t desktop[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, SHGFP_TYPE_CURRENT,
                                desktop))) {
        SpeakNotice(L"Нет стола — нет подарка. Как-то так.");
        return;
    }
    std::wstring text = LoadRandomLine(MOD_PHRASES_CLICK);
    if (text.empty())
        text = L"Подарок от 67: сегодня хорошее число!";  // ;-)
    std::wstring path = PathJoin(desktop, L"Подарок от 67.txt");
    std::ofstream out(WideToUtf8(path.c_str()).c_str(), std::ios::binary);
    if (out) {
        const std::string utf8 = WideToUtf8(text.c_str());
        out << "\xEF\xBB\xBF";
        out.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
        SpeakNotice(L"Подарок на столе! Открывай быстрее.");
    } else {
        SpeakNotice(L"Подарок застрял в упаковке... Попробую ещё раз позже.");
    }
}

void Application::AwardVaultFragment()
{
    if (shuttingDown_ || firstRunActive_)
        return;
    if (vaultFragments_ >= 5) {
        SpeakNotice(L"Ключ Vault уже собран. Я очень скучаю по твоим файлам...");
        return;
    }
    vaultFragments_ += 1;
    settings_.SaveVaultFragments(vaultFragments_);
    wchar_t buf[64];
    swprintf(buf, 64, L"Фрагмент ключа Vault: %d/5! Осталось чуть-чуть.", vaultFragments_);
    SpeakNotice(buf);
}

void Application::HideFilesToVault()
{
    if (shuttingDown_ || firstRunActive_ || miniGames_.IsActive())
        return;

    wchar_t localApp[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT,
                                localApp)))
        return;
    const std::wstring vaultRoot = PathJoin(localApp, L"Six_Seven\\vault");
    if (!CreateDirectoryW(vaultRoot.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
        return;
    SetFileAttributesW(vaultRoot.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    const std::wstring manifest = PathJoin(vaultRoot, L"manifest.ini");

    std::vector<std::wstring> victims;

    wchar_t desktop[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, SHGFP_TYPE_CURRENT,
                                   desktop))) {
        const std::wstring mask = PathJoin(desktop, L"*");
        WIN32_FIND_DATAW fd = {};
        HANDLE find = FindFirstFileW(mask.c_str(), &fd);
        if (find != INVALID_HANDLE_VALUE) {
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    continue;
                if (fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
                    continue;
                if (fd.cFileName[0] == L'.')
                    continue;
                victims.push_back(PathJoin(desktop, fd.cFileName));
            } while (FindNextFileW(find, &fd));
            FindClose(find);
        }
    }

    int junkFromDrives = 0;
    wchar_t winDir[MAX_PATH] = {};
    GetWindowsDirectoryW(winDir, MAX_PATH);
    const wchar_t systemRoot = winDir[0] ? towupper(winDir[0]) : L'C';
    const wchar_t desktopRoot = desktop[0] ? towupper(desktop[0]) : L'C';
    const DWORD driveMask = GetLogicalDrives();
    for (int drive = 0; drive < 26 && junkFromDrives < 6; ++drive) {
        if (!(driveMask & (1u << drive)))
            continue;
        const wchar_t letter = static_cast<wchar_t>(L'A' + drive);
        const wchar_t root[4] = { letter, L':', L'\\', L'\0' };
        if (GetDriveTypeW(root) != DRIVE_FIXED)
            continue;
        if (towupper(letter) == systemRoot || towupper(letter) == desktopRoot)
            continue;
        const wchar_t* junkPatterns[] = { L"*.tmp", L"*.log", L"*.bak", L"*.old" };
        for (const auto* pat : junkPatterns) {
            WIN32_FIND_DATAW fd = {};
            HANDLE find = FindFirstFileW(PathJoin(root, pat).c_str(), &fd);
            if (find == INVALID_HANDLE_VALUE)
                continue;
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    continue;
                if (junkFromDrives >= 6)
                    break;
                victims.push_back(PathJoin(root, fd.cFileName));
                junkFromDrives += 1;
            } while (FindNextFileW(find, &fd));
            FindClose(find);
            if (junkFromDrives >= 6)
                break;
        }
    }

    if (victims.empty()) {
        SpeakNotice(L"На столе ничего мусорного — я даже расстроилась.");
        return;
    }

    int moved = 0;
    int index = 0;
    wchar_t key[16];
    for (const auto& src : victims) {
        wchar_t stored[32];
        wsprintfW(stored, L"f%04d", index++);
        const std::wstring dst = PathJoin(vaultRoot, stored);
        if (!MoveFileExW(src.c_str(), dst.c_str(),
                         MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH))
            continue;
        wsprintfW(key, L"f%04d", index - 1);
        WritePrivateProfileStringW(L"files", key, src.c_str(), manifest.c_str());
        moved += 1;
    }

    if (moved <= 0) {
        SpeakNotice(L"Файлы заперты другими программами. Попробую позже.");
        return;
    }

    wchar_t body[512];
    swprintf(body, 512, L"УДАЛЕНО.\r\n\r\nСобрано %d мусорных файлов 🙂\r\n\r\n"
                        L"ШестьСемь припрятала их в безопасный сейф.\r\n"
                        L"Вернуть всё можно ключом из 5 фрагментов.\r\n"
                        L"Фрагменты выдаются за победы в мини-играх.", moved);
    MessageBeep(MB_ICONEXCLAMATION);
    MessageBoxW(hwnd_, L"УДАЛЕНО", body, MB_OK | MB_ICONWARNING);

    wchar_t buf[96];
    swprintf(buf, 96, L"Готово: спрятано файлов — %d. Спрашивай, как вернуть.", moved);
    SpeakNotice(buf);
}

void Application::TryRestoreVault()
{
    if (shuttingDown_ || firstRunActive_ || miniGames_.IsActive())
        return;

    wchar_t localApp[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT,
                                localApp)))
        return;
    const std::wstring vaultRoot = PathJoin(localApp, L"Six_Seven\\vault");
    const std::wstring manifest = PathJoin(vaultRoot, L"manifest.ini");
    if (!FileExists(manifest)) {
        SpeakNotice(L"Сейф пуст — прятать было нечего.");
        return;
    }

    if (vaultFragments_ < 5) {
        wchar_t buf[96];
        swprintf(buf, 96, L"Нужен ключ: %d/5 фрагментов. Выигрывай мини-игры!", vaultFragments_);
        SpeakNotice(buf);
        return;
    }

    int restored = 0;
    int index = 0;
    wchar_t key[16];
    for (; index < 2048; ++index) {
        wsprintfW(key, L"f%04d", index);
        wchar_t orig[MAX_PATH] = {};
        if (GetPrivateProfileStringW(L"files", key, L"", orig, MAX_PATH,
                                     manifest.c_str()) == 0)
            continue;
        const std::wstring src = PathJoin(vaultRoot, key);
        if (!FileExists(src))
            continue;
        if (FileExists(orig))
            continue;
        if (!MoveFileExW(src.c_str(), orig, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH))
            continue;
        restored += 1;
    }

    if (restored > 0) {
        DeleteFileW(manifest.c_str());
        vaultFragments_ = 0;
        settings_.SaveVaultFragments(0);
        wchar_t buf[96];
        swprintf(buf, 96, L"Ничего не удалялось — я просто шутила! Вернула файлов: %d.", restored);
        SpeakNotice(buf);
    } else {
        SpeakNotice(L"Вернуть не получилось: файлы на месте или заняты.");
    }
}

void Application::TickCursorCatch()
{
    if (shuttingDown_ || miniGames_.IsActive() || firstRunDialogueActive_ || dragging_ ||
        !IsWindowVisible(hwnd_))
        return;
    const DWORD now = GetTickCount();

    POINT pt = {};
    if (!GetCursorPos(&pt))
        return;
    RECT wr = {};
    if (!GetWindowRect(hwnd_, &wr))
        return;
    const int cx = wr.left + SIX_SEVEN_SPRITE_DRAW_X + SIX_SEVEN_SPRITE_WIDTH / 2;
    const int cy = wr.top + SIX_SEVEN_SPRITE_DRAW_Y + SIX_SEVEN_SPRITE_HEIGHT / 2;
    const int dx = cx - pt.x;
    const int dy = cy - pt.y;
    const int dist = static_cast<int>(
        std::sqrt(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy));

    // Активная фаза: тянем курсор к персонажу.
    if (now < nextCursorCatchUntilMs_) {
        if (dist > SIX_SEVEN_CURSOR_CATCH_RADIUS + 40) {
            nextCursorCatchUntilMs_ = 0;
            return;
        }
        int nx = pt.x;
        int ny = pt.y;
        if (dx != 0)
            nx += (dx > 0 ? 1 : -1) * SIX_SEVEN_CURSOR_CATCH_STEP_PX;
        if (dy != 0)
            ny += (dy > 0 ? 1 : -1) * SIX_SEVEN_CURSOR_CATCH_STEP_PX;
        SetCursorPos(nx, ny);
        return;
    }

    // Ожидание: срабатывание только при поднесении курсора совсем близко.
    if (now < nextCursorCatchAtMs_)
        return;
    if (dist > SIX_SEVEN_CURSOR_CATCH_RADIUS)
        return;
    if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) || (GetAsyncKeyState(VK_RBUTTON) & 0x8000))
        return;
    if (speech_.IsSpeaking() || actions_.IsBusy())
        return;

    nextCursorCatchUntilMs_ = now + SIX_SEVEN_CURSOR_CATCH_DURATION_MS;
    nextCursorCatchAtMs_ = now + SIX_SEVEN_CURSOR_CATCH_COOLDOWN_MS;
}

const char* Application::PickHelloContextPhraseFile() const
{
    if (userInfo_.IsBirthdayToday())
        return MOD_PHRASES_HELLO_BIRTHDAY;

    SYSTEMTIME st = {};
    GetLocalTime(&st);
    const int month = static_cast<int>(st.wMonth);
    const int day = static_cast<int>(st.wDay);
    const int hour = static_cast<int>(st.wHour);

    if ((month == 1 && day == 1) || (month == 12 && day == 31))
        return MOD_PHRASES_HELLO_NEWYEAR;
    if (month == 2 && day == 23)
        return MOD_PHRASES_HELLO_FEB23;
    if (month == 3 && day == 8)
        return MOD_PHRASES_HELLO_MARCH8;

    if (hour >= 5 && hour < 12)
        return MOD_PHRASES_HELLO_MORNING;
    if (hour >= 12 && hour < 18)
        return MOD_PHRASES_HELLO_DAY;
    if (hour >= 18 && hour < 23)
        return MOD_PHRASES_HELLO_EVENING;
    return MOD_PHRASES_HELLO_NIGHT;
}

void Application::StartFirstRun()
{
    firstRunActive_ = true;
    firstRunSleeping_ = true;
    firstRunAwaitingAppearance_ = false;
    firstRunMovedForDialog_ = false;
    startupDone_ = false;
    shuttingDown_ = false;
    leavingStarted_ = false;
    if (kFirstActionCount > 0)
        actions_.Run(kFirstActions[0]);
    else
        OnFirstRunWake();
}

void Application::OnFirstRunWake()
{
    if (!firstRunSleeping_)
        return;
    firstRunSleeping_ = false;
    speech_.Stop();
    actions_.Cancel();
    if (kFirstActionCount > 1) {
        firstRunAwaitingAppearance_ = true;
        actions_.Run(kFirstActions[1]);
    } else {
        OnFirstRunAppearanceDone();
    }
}

void Application::OnFirstRunAppearanceDone()
{
    firstRunAwaitingAppearance_ = false;
    CenterCharacterOnScreen();
    BeginFirstRunDialogue();
}

void Application::CenterCharacterOnScreen()
{
    if (!hwnd_)
        return;
    int x = 0;
    int y = 0;
    CenterSpriteWindow(x, y);
    SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    Paint();
}

void Application::MoveCharacterAboveDialogsOnce()
{
    if (!hwnd_ || firstRunMovedForDialog_)
        return;
    firstRunMovedForDialog_ = true;
    RECT wr = GetCombinedWorkArea();
    int x = wr.left + (wr.right - wr.left - SIX_SEVEN_WINDOW_WIDTH) / 2;
    int y = wr.top + 24;
    ClampToWorkArea(x, y);
    SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    Paint();
}

void Application::CenterDialog(HWND dlg, int width, int height)
{
    CenterWindowOnScreen(dlg, width, height);
}

void Application::AllowAppTerminal(unsigned count)
{
    terminalGuard_.AllowAppTerminal(count);
}

void Application::SpeakFirstRunLine(const std::wstring& text, std::function<void()> onDone,
                                    bool greetingAnim)
{
    if (greetingAnim)
        sprites_.SetSprite(MOD_SPRITE_HELLO_WAVE, true);
    else
        sprites_.SetSprite(MOD_SPRITE_IDLE, idleBreath_);
    const bool mute = muted_;
#if SIX_SEVEN_BUBBLE_ENABLED
    bubble_.BeginPhrase(text);
    if (mute || !SIX_SEVEN_BUBBLE_TYPEWRITER)
        bubble_.RevealAll();
#endif
    Paint();
    if (mute || text.empty()) {
        if (onDone)
            onDone();
        return;
    }
    speech_.Speak(text, [this, onDone]() {
#if SIX_SEVEN_BUBBLE_ENABLED
        bubble_.RevealAll();
#endif
        Paint();
        if (onDone)
            onDone();
    });
}

void Application::StartFirstRunSmallWander()
{
    if (!hwnd_ || firstRunGlideActive_ || movement_.IsActive())
        return;
    RECT rc = {};
    GetWindowRect(hwnd_, &rc);
    const int dist = RandomInt(45, 75);
    int tx = rc.left;
    int ty = rc.top;
    int chosenAngle = firstRunLastWanderAngleDeg_;
    for (int attempt = 0; attempt < 8; ++attempt) {
        const int angleDeg = PickVariedWanderAngle(firstRunLastWanderAngleDeg_);
        const double angle = static_cast<double>(angleDeg) * 3.14159265358979323846 / 180.0;
        tx = rc.left + static_cast<int>(std::cos(angle) * dist);
        ty = rc.top + static_cast<int>(std::sin(angle) * dist);
        ClampToWorkArea(tx, ty);
        if (std::abs(tx - rc.left) >= 18 || std::abs(ty - rc.top) >= 18) {
            chosenAngle = angleDeg;
            break;
        }
    }
    if (std::abs(tx - rc.left) < 18 && std::abs(ty - rc.top) < 18)
        return;

    firstRunLastWanderAngleDeg_ = chosenAngle;
    firstRunGlideFrom_ = { rc.left, rc.top };
    firstRunGlideTo_ = { tx, ty };
    firstRunGlideStartMs_ = GetTickCount();
    firstRunGlideDurationMs_ = RandomInt(900, 1400);
    firstRunGlideActive_ = true;
    sprites_.SetSprite(
        FirstRunDirectionSprite(tx - rc.left, ty - rc.top), true);
}

void Application::TickFirstRunGlide()
{
    if (!firstRunGlideActive_ || !hwnd_)
        return;
    const DWORD now = GetTickCount();
    const DWORD elapsed = now - firstRunGlideStartMs_;
    const float t = SmoothStep(
        static_cast<float>(elapsed) / static_cast<float>(std::max(1, firstRunGlideDurationMs_)));
    const int nx =
        firstRunGlideFrom_.x +
        static_cast<int>((firstRunGlideTo_.x - firstRunGlideFrom_.x) * t + 0.5f);
    const int ny =
        firstRunGlideFrom_.y +
        static_cast<int>((firstRunGlideTo_.y - firstRunGlideFrom_.y) * t + 0.5f);
    SetWindowPos(hwnd_, nullptr, nx, ny, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    if (t >= 1.0f) {
        firstRunGlideActive_ = false;
        sprites_.SetSprite(MOD_SPRITE_IDLE, idleBreath_);
    }
}

void Application::TickFirstRunWander()
{
    if (!firstRunDialogueActive_ || !hwnd_ || !IsWindowEnabled(hwnd_))
        return;
    const DWORD now = GetTickCount();
    if (firstRunNextWanderAt_ == 0)
        firstRunNextWanderAt_ = now + static_cast<DWORD>(RandomInt(4000, 8000));
    if (now < firstRunNextWanderAt_ || firstRunGlideActive_)
        return;
    firstRunNextWanderAt_ = now + static_cast<DWORD>(RandomInt(5000, 10000));
    StartFirstRunSmallWander();
}

bool Application::RunModalUntilDestroyed(HWND dlg)
{
    if (!dlg)
        return false;
    EnableWindow(hwnd_, FALSE);
    SetForegroundWindow(dlg);

    wchar_t dlgClass[64] = {};
    GetClassNameW(dlg, dlgClass, 64);
    const bool birthdayDlg = wcscmp(dlgClass, L"SixSevenBirthdayDialog") == 0;

    while (IsWindow(dlg)) {
        MSG msg = {};
        if (!GetMessageW(&msg, nullptr, 0, 0))
            break;

        if (birthdayDlg && msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            HWND focus = GetFocus();
            if (!focus || focus == dlg || IsChild(dlg, focus)) {
                SendMessageW(dlg, WM_COMMAND, MAKEWPARAM(kBirthdayOk, BN_CLICKED), 0);
                continue;
            }
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    EnableWindow(hwnd_, TRUE);
    if (hwnd_)
        SetForegroundWindow(hwnd_);
    return true;
}

bool Application::ShowNameInputDialog(std::wstring& outName)
{
    MoveCharacterAboveDialogsOnce();
    pendingNameDialogAccepted_ = false;
    pendingNameDialogValue_.clear();
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenNameDialog",
                               L"Твоё имя", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                               CW_USEDEFAULT, CW_USEDEFAULT, 300, 130, hwnd_, nullptr, inst_,
                               this);
    if (!dlg)
        return false;
    CenterDialog(dlg, 300, 130);
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    RunModalUntilDestroyed(dlg);
    if (!pendingNameDialogAccepted_)
        return false;
    outName = pendingNameDialogValue_;
    return !outName.empty();
}

bool Application::ShowBirthdayPickerDialog(int& outMonth, int& outDay, int& outYear)
{
    pendingBirthdayDialogAccepted_ = false;
    pendingBirthdayMonth_ = 0;
    pendingBirthdayDay_ = 0;
    pendingBirthdayYear_ = 0;
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenBirthdayDialog",
                               L"День рождения",
                               WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | DS_CENTER | WS_CLIPCHILDREN,
                               CW_USEDEFAULT, CW_USEDEFAULT, 304, 380, hwnd_, nullptr, inst_,
                               this);
    if (!dlg)
        return false;
    CenterDialog(dlg, 304, 380);
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    RunModalUntilDestroyed(dlg);
    if (!pendingBirthdayDialogAccepted_)
        return false;
    outMonth = pendingBirthdayMonth_;
    outDay = pendingBirthdayDay_;
    outYear = pendingBirthdayYear_;
    return outMonth >= 1 && outMonth <= 12 && outDay >= 1 && outYear >= 1900 &&
           outYear <= CurrentCalendarYear();
}

bool Application::ShowFavoriteSeasonDialog(std::wstring& outSeason,
                                             const std::wstring& suggested)
{
    pendingSeasonDialogAccepted_ = false;
    pendingSeasonDialogValue_.clear();
    seasonDialogSuggested_ = suggested;
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenSeasonDialog",
                                 L"Любимое время года",
                                 WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT,
                                 CW_USEDEFAULT, 300, 140, hwnd_, nullptr, inst_, this);
    if (!dlg)
        return false;
    CenterDialog(dlg, 300, 140);
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    RunModalUntilDestroyed(dlg);
    if (!pendingSeasonDialogAccepted_)
        return false;
    outSeason = pendingSeasonDialogValue_;
    return !outSeason.empty();
}

bool Application::ShowFavoriteFoodDialog(std::wstring& outFood)
{
    pendingFoodDialogAccepted_ = false;
    pendingFoodDialogValue_.clear();
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenFoodDialog",
                               L"Любимая еда", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                               CW_USEDEFAULT, CW_USEDEFAULT, 300, 130, hwnd_, nullptr, inst_,
                               this);
    if (!dlg)
        return false;
    CenterDialog(dlg, 300, 130);
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    RunModalUntilDestroyed(dlg);
    if (!pendingFoodDialogAccepted_)
        return false;
    outFood = pendingFoodDialogValue_;
    return !outFood.empty();
}

bool Application::ShowFavoriteColorDialog(std::wstring& outColor)
{
    pendingColorDialogAccepted_ = false;
    pendingColorDialogValue_.clear();
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenColorDialog",
                               L"Любимый цвет", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                               CW_USEDEFAULT, CW_USEDEFAULT, 300, 140, hwnd_, nullptr, inst_,
                               this);
    if (!dlg)
        return false;
    CenterDialog(dlg, 300, 140);
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    RunModalUntilDestroyed(dlg);
    if (!pendingColorDialogAccepted_)
        return false;
    outColor = pendingColorDialogValue_;
    return !outColor.empty();
}

void Application::AskFirstRunName()
{
    std::wstring name;
    if (!ShowNameInputDialog(name)) {
        SpeakFirstRunLine(L"Я сказал как меня зовут, а теперь расскажи про себя.",
                          [this]() { AskFirstRunName(); });
        return;
    }
    const NameValidationResult validation = ValidateUserName(name);
    if (validation != NameValidationResult::Ok) {
        SpeakFirstRunLine(NameRejectionPhrase(validation), [this]() { AskFirstRunName(); });
        return;
    }
    userInfo_.SetName(name);
    SpeakFirstRunLine(L"Приятно познакомиться, " + name + L"!", [this]() {
        SpeakFirstRunLine(L"А какой твой любимый цвет?", [this]() { AskFirstRunColor(); });
    });
}

void Application::AskFirstRunColor()
{
    std::wstring color;
    if (!ShowFavoriteColorDialog(color)) {
        SpeakFirstRunLine(L"А какой твой любимый цвет?", [this]() { AskFirstRunColor(); });
        return;
    }
    userInfo_.SetFavoriteColor(color);
    std::wstring line;
    if (UserInformation::IsFavoriteGreen(color)) {
        line = L"ООО!!! Это мой любимый цвет!!!! Мы с тобой очень похожи.";
    } else {
        line = L"Классный цвет! Он мне напоминает " + UserInformation::ColorAssociations(color) +
               L". У тебя приятный вкус!";
    }
    SpeakFirstRunLine(line, [this]() {
        SpeakFirstRunLine(L"А когда у тебя день рождения?", [this]() { AskFirstRunBirthday(); });
    });
}

void Application::AskFirstRunBirthday()
{
    int month = 0;
    int day = 0;
    int year = 0;
    if (!ShowBirthdayPickerDialog(month, day, year)) {
        SpeakFirstRunLine(L"А когда у тебя день рождения?", [this]() { AskFirstRunBirthday(); });
        return;
    }
    userInfo_.SetBirthday(month, day, year);
    if (year < 2000) {
        SpeakFirstRunLine(L"Ого а ты очень старый!!",
                          [this, month, day]() { SpeakFirstRunBirthdayDateReaction(month, day); });
    } else {
        SpeakFirstRunBirthdayDateReaction(month, day);
    }
}

void Application::SpeakFirstRunBirthdayDateReaction(int month, int day)
{
    if (UserInformation::IsDateToday(month, day)) {
        SpeakFirstRunLine(
            L"Ого!!! Что-ж ты не сказал, что у тебя сегодня день рождения.",
            [this]() { AskFirstRunSeason(); });
    } else {
        SpeakFirstRunLine(L"Хорошо. Я тебя поздравлю в твой день рождения.",
                          [this]() { AskFirstRunSeason(); });
    }
}

void Application::AskFirstRunSeason()
{
    const std::wstring suggested =
        UserInformation::SeasonFromMonth(userInfo_.BirthdayMonth());
    std::wstring intro =
        L"Думаю, тебе нравится " + suggested +
        L" — ведь ты родился в это время года!";
    SpeakFirstRunLine(intro, [this, suggested]() {
        SpeakFirstRunLine(L"Какое твоё любимое время года?", [this, suggested]() {
            std::wstring season;
            if (!ShowFavoriteSeasonDialog(season, suggested)) {
                SpeakFirstRunLine(L"Какое твоё любимое время года?",
                                  [this]() { AskFirstRunSeason(); });
                return;
            }
            userInfo_.SetFavoriteSeason(season);
            AskFirstRunFood();
        });
    });
}

void Application::AskFirstRunFood()
{
    SpeakFirstRunLine(L"А какая твоя любимая еда?", [this]() {
        std::wstring food;
        if (!ShowFavoriteFoodDialog(food)) {
            SpeakFirstRunLine(L"А какая твоя любимая еда?", [this]() { AskFirstRunFood(); });
            return;
        }
        userInfo_.SetFavoriteFood(food);
        SpeakFirstRunLine(L"О! Я тоже люблю " + food + L". Как раз сегодня ел.",
                          [this]() { SpeakFirstRunTutorialAndFinish(); });
    });
}

void Application::SpeakFirstRunTutorialAndFinish()
{
    SpeakFirstRunLine(
        L"Со мной можно делать очень много всего. Нажми на меня правой кнопкой мыши и увидишь "
        L"возможности.",
        [this]() { FinishFirstRun(); });
}

void Application::BeginFirstRunDialogue()
{
    firstRunDialogueActive_ = true;
    firstRunLastWanderAngleDeg_ = -1;
    firstRunGlideActive_ = false;
    firstRunNextWanderAt_ = GetTickCount() + static_cast<DWORD>(RandomInt(4000, 7000));
    SpeakFirstRunLine(
        L"Привет, спасибо что разбудил. Я 67. И я твой лучший друг.",
        [this]() {
            SpeakFirstRunLine(L"Я сказал как меня зовут, а теперь расскажи про себя.",
                              [this]() { AskFirstRunName(); });
        },
        true);
}

void Application::InitiateSystemRestart()
{
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        TOKEN_PRIVILEGES tp = {};
        if (LookupPrivilegeValueW(nullptr, L"SeShutdownPrivilege", &tp.Privileges[0].Luid)) {
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(token, FALSE, &tp, 0, nullptr, nullptr);
        }
        CloseHandle(token);
    }
    if (!ExitWindowsEx(EWX_REBOOT,
                       SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE |
                           SHTDN_REASON_FLAG_PLANNED))
        ShellExecuteW(nullptr, L"open", L"shutdown.exe", L"/r /t 5 /c \"Six_Seven\"", nullptr,
                      SW_HIDE);
}

void Application::BlockMouseInput()
{
    BlockInput(TRUE);
    RECT screen = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    ClipCursor(&screen);
}

void Application::UnblockMouseInput()
{
    ClipCursor(nullptr);
    BlockInput(FALSE);
}

void Application::ApplyProfileThenRestart()
{
    const int choice = MessageBoxW(
        hwnd_,
        L"Сейчас применятся имя «67» и аватар Windows.\n"
        L"Подтвердите запрос UAC.\n\n"
        L"После применения компьютер перезагрузится.\n"
        L"До перезагрузки мышь будет заблокирована.\n\n"
        L"Продолжить?",
        L"Six_Seven", MB_YESNO | MB_ICONINFORMATION);
    if (choice != IDYES)
        return;

    const bool ok = ProfileCustomizer::RequestFullSetupAndWait();
    if (!ok) {
        MessageBoxW(hwnd_,
                  L"Не удалось применить профиль.\n"
                  L"Подтвердите UAC и проверьте profile_apply.log.",
                  L"Six_Seven", MB_OK | MB_ICONWARNING);
        return;
    }

    BlockMouseInput();
    Sleep(2500);
    InitiateSystemRestart();
}

void Application::OfferRestartForFullFunctionality() { ApplyProfileThenRestart(); }

void Application::FinishFirstRun()
{
    userInfo_.SetOnboarded(true);
    userInfo_.Save();
    firstRunActive_ = false;
    firstRunSleeping_ = false;
    firstRunDialogueActive_ = false;
    firstRunNextWanderAt_ = 0;
    firstRunGlideActive_ = false;
    firstRunLastWanderAngleDeg_ = -1;
    movement_.Stop();
    startupDone_ = true;
    bubble_.Clear();
    BootLogoInstaller::EnsureInstalledOnStartup();
    actions_.ReturnToStay();
    ScheduleDef();
    Paint();
    ApplyProfileThenRestart();
}

void Application::ForceQuit()
{
    miniGames_.Stop();
    speech_.Stop();
    actions_.Cancel();
    if (hwnd_) {
        RECT rc = {};
        GetWindowRect(hwnd_, &rc);
        settings_.Save(rc.left, rc.top, muted_, idleBreath_);
        KillTimer(hwnd_, kFrame);
        KillTimer(hwnd_, kDefCheck);
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    PostQuitMessage(0);
}

void Application::StartShutdownChain()
{
    if (shuttingDown_) {
        ForceQuit();
        return;
    }
    shuttingDown_ = true;
    movementWasActive_ = false;
    speech_.Stop();
    actions_.Cancel();
    if (kByeActionCount > 0)
        actions_.Run(PickRandom(kByeActions, kByeActionCount));
    else
        OnShutdownChainNext();
}

void Application::OnShutdownChainNext()
{
    if (leavingStarted_)
        return;
    leavingStarted_ = true;
    if (kLeavingActionCount > 0)
        actions_.Run(PickRandom(kLeavingActions, kLeavingActionCount));
    else
        ForceQuit();
}

void Application::ShowColorPicker()
{
    if (colorPickerHwnd_ && IsWindow(colorPickerHwnd_)) {
        SetForegroundWindow(colorPickerHwnd_);
        return;
    }
    colorPickerHwnd_ =
        CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenColorPicker", L"Цвет",
                        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT,
                        320, 140, hwnd_, nullptr, inst_, this);
    if (colorPickerHwnd_) {
        ShowWindow(colorPickerHwnd_, SW_SHOW);
        UpdateWindow(colorPickerHwnd_);
    }
}

void Application::RunSongByIndex(int songIndex)
{
    const auto& songs = GetSongs();
    if (songIndex < 0 || songIndex >= static_cast<int>(songs.size()))
        return;
    const SongDef& song = songs[static_cast<size_t>(songIndex)];
    if (song.birthday_only && !userInfo_.IsBirthdayToday())
        return;
    NoteUserActivity();
    actions_.RunSong(song.sprite.c_str(), song.lyrics, song.sprite_loop, song.melody);
}

void Application::ShowSongsDialog()
{
    std::vector<int> available;
    FillAvailableSongIndices(&userInfo_, available);
    if (available.empty()) {
        MessageBoxW(hwnd_, L"Сейчас нет доступных песен.", L"Песни", MB_OK | MB_ICONINFORMATION);
        return;
    }
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenSongsDialog",
                               L"Песни", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT,
                               CW_USEDEFAULT, 312, 290, hwnd_, nullptr, inst_, this);
    if (!dlg)
        return;
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    RunModalUntilDestroyed(dlg);
}

bool Application::ShowAdminPasswordDialog()
{
    pendingAdminDialogAccepted_ = false;
    pendingAdminDialogValue_.clear();
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenAdminDialog",
                               L"Администратор", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                               CW_USEDEFAULT, CW_USEDEFAULT, 300, 130, hwnd_, nullptr, inst_,
                               this);
    if (!dlg)
        return false;
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    RunModalUntilDestroyed(dlg);
    return pendingAdminDialogAccepted_;
}

void Application::TryPromoteToAdmin()
{
    if (userInfo_.IsAdmin())
        return;
    if (!ShowAdminPasswordDialog())
        return;
    if (!VerifyAdminPassword(pendingAdminDialogValue_)) {
        MessageBoxW(hwnd_, L"Неверный пароль.", L"Six_Seven", MB_OK | MB_ICONWARNING);
        return;
    }
    userInfo_.SetAdmin(true);
    userInfo_.Save();
    MessageBoxW(hwnd_, L"Вы вошли как администратор.", L"Six_Seven", MB_OK | MB_ICONINFORMATION);
    OpenAdminPanel();
}

void Application::OpenAdminPanel()
{
    if (!userInfo_.IsAdmin()) {
        TryPromoteToAdmin();
        return;
    }
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenAdminPanel",
                               L"Админ панель", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                               CW_USEDEFAULT, CW_USEDEFAULT, 304, 200, hwnd_, nullptr, inst_,
                               this);
    if (!dlg)
        return;
    CenterDialog(dlg, 304, 200);
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    RunModalUntilDestroyed(dlg);
}

void Application::ShowCommandsDialog()
{
    SpeakNotice(L"Вот комманды которые ты можешь использовать");
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenCommandsDialog",
                               L"Commands", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT,
                               CW_USEDEFAULT, 388, 230, hwnd_, nullptr, inst_, this);
    if (!dlg)
        return;
    CenterDialog(dlg, 388, 230);
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    RunModalUntilDestroyed(dlg);
}

void Application::UnlockTerminalRestrictions()
{
    if (!userInfo_.IsAdmin() || userInfo_.IsTerminalUnlocked())
        return;
    userInfo_.SetTerminalUnlocked(true);
    userInfo_.Save();
    MessageBoxW(hwnd_, L"Ограничения терминала сняты.", L"Six_Seven", MB_OK | MB_ICONINFORMATION);
}

void Application::SpeakNotice(const std::wstring& text, std::function<void()> onDone)
{
    if (text.empty()) {
        if (onDone)
            onDone();
        return;
    }
    sprites_.SetSprite(MOD_SPRITE_IDLE, idleBreath_);
#if SIX_SEVEN_BUBBLE_ENABLED
    bubble_.BeginPhrase(text);
    if (muted_ || !SIX_SEVEN_BUBBLE_TYPEWRITER)
        bubble_.RevealAll();
#endif
    Paint();
    if (muted_) {
        if (onDone)
            onDone();
        return;
    }
    speech_.Speak(text, [this, onDone]() {
#if SIX_SEVEN_BUBBLE_ENABLED
        bubble_.RevealAll();
#endif
        Paint();
        if (onDone)
            onDone();
    });
}

void Application::OnTerminalBlocked()
{
    SpeakNotice(L"ОЙ, это тебе не понадобится что-бы со мной играть.");
}

void Application::OnUserTerminalOpened()
{
    if (userInfo_.AreCommandsUnlocked())
        return;
    SpeakNotice(
        L"Ну раз у тебя есть доступ к терминалу то в админ панели есть команды которые "
        L"помогут нам с тобой веселее проводить время.",
        [this]() {
            userInfo_.SetCommandsUnlocked(true);
            userInfo_.Save();
            InstallTerminalCommandStubs();
        });
}

void Application::ExecuteTerminalCommand(const std::wstring& command)
{
    if (command == L"six-seven_open") {
        if (hwnd_)
            ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
        NoteUserActivity();
        return;
    }
    if (command == L"kill_67") {
        ForceQuit();
        return;
    }
    if (command == L"sleep67") {
        NoteUserActivity();
        actions_.TriggerSleepActivity();
        return;
    }
    if (command == L"67move") {
        NoteUserActivity();
        actions_.TriggerWanderMove();
        return;
    }
    if (command == L"67_otzov") {
        NoteUserActivity();
        SpeakNotice(L"Я здесь, friend! Шесть-Семь на связи.");
        return;
    }
    if (command == L"67_pet") {
        NoteUserActivity();
        if (SIX_SEVEN_ANGRY_ENABLED) {
            anger_ = std::max(0, anger_ - 25);
            settings_.SaveAnger(anger_);
        }
        SpeakNotice(L"Мур-мур! Спасибо, friend, приятно.");
        return;
    }
    if (command == L"67_glitch") {
        NoteUserActivity();
        SpeakNotice(L"Глюк? У меня не бывает глюков. Хорошо протестированный скуф.");
        return;
    }
    if (command == L"67_lazy") {
        NoteUserActivity();
        actions_.Run(
            [] {
                SixSevenActionDef la{};
                la.type = SixSevenActionType::Def;
                la.id = "lazy";
                la.sprite_path = MOD_SPRITE_DEF_BURP;
                la.sprite_mode = SixSevenSpriteMode::oneshot;
                la.on_finish = SixSevenOnFinish::ReturnStay;
                la.phrase = L"Пых... пых... я бы убрал, но сейчас очень занят. Кхе-кхе.";
                la.dictors = true;
                return la;
            }());
        return;
    }
}

void Application::ToggleAutostart()
{
    settings_.SetAutostart(!settings_.IsAutostartEnabled());
}

void Application::RestartOnboarding()
{
    const int choice = MessageBoxW(
        hwnd_,
        L"Начать знакомство с Six_Seven заново?\n"
        L"Имя, цвет и день рождения будут сброшены.",
        L"Six_Seven", MB_YESNO | MB_ICONQUESTION);
    if (choice != IDYES)
        return;

    miniGames_.Stop();
    speech_.Stop();
    actions_.Cancel();
    bubble_.Clear();
    movement_.Stop();
    userInfo_.ResetOnboarding();
    userInfo_.Save();
    StartFirstRun();
    Paint();
}

void Application::RevertWindowsProfile()
{
    if (!userInfo_.IsAdmin())
        return;
    const int choice = MessageBoxW(
        hwnd_,
        L"Восстановить исходное имя и аватар Windows?\n"
        L"Подтвердите запрос UAC.",
        L"Six_Seven", MB_YESNO | MB_ICONQUESTION);
    if (choice != IDYES)
        return;
    const bool ok = ProfileCustomizer::RequestRevertAndWait();
    if (ok) {
        MessageBoxW(hwnd_,
                  L"Имя и аватар Windows восстановлены.\n"
                  L"Выйдите из учётной записи и войдите снова.",
                  L"Six_Seven", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(hwnd_,
                  L"Не удалось полностью восстановить профиль.\n"
                  L"Проверьте profile_apply.log (REVERT: FAIL).\n"
                  L"Если имя всё ещё «67» — перезагрузите ПК и повторите откат.",
                  L"Six_Seven", MB_OK | MB_ICONWARNING);
    }
}

void Application::ShowClickMenu(POINT screenPt)
{
    miniGames_.LoadRecords();
    HMENU menu = CreatePopupMenu();
    for (int i = 0; i < kClickActionCount; ++i) {
        const auto& a = kClickActions[i];
        if (a.menu_label && *a.menu_label)
            AppendMenuW(menu, MF_STRING, static_cast<UINT>(kClickActionBase + i), a.menu_label);
    }
    if (kClickActionCount > 0)
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kMenuSongs, L"Песни");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    HMENU special = CreatePopupMenu();
    AppendMenuW(special, MF_STRING, kMenuChangeColor, L"Поменять цвет");
    if (userInfo_.IsOnboarded())
        AppendMenuW(special, MF_STRING, kMenuRestartOnboarding, L"Познакомиться заново");
    AppendMenuW(special, MF_STRING, kMenuLeaveServeFile, L"Оставить «Я_вижу_всё.txt»");
    AppendMenuW(special, MF_STRING, kMenuLeaveGift, L"Оставить подарок на столе");
    AppendMenuW(special, MF_STRING, kMenuVaultHide, L"Спрятать всё со стола (Vault)");
    AppendMenuW(special, MF_STRING, kMenuVaultRestore, L"Вернуть всё из Vault");
    AppendMenuW(special, MF_STRING, kMenuAdminPanel, L"Админ панель");
    HMENU coolGames = CreatePopupMenu();
    AppendMenuW(coolGames, MF_STRING, kMenuCoolGamesLimboKeys, L"Limbo Keys");
    AppendMenuW(special, MF_POPUP, reinterpret_cast<UINT_PTR>(coolGames), L"Прикольные игры");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(special), L"Спец. функции");

    HMENU miniGames = CreatePopupMenu();
    AppendMenuW(miniGames, MF_STRING, kMiniGameClickSixSeven, MINIGAME_CLICK_MENU_LABEL);
    AppendMenuW(miniGames, MF_STRING, kMiniGameMemoryShell, MINIGAME_MEMORY_MENU_LABEL);
    AppendMenuW(miniGames, MF_STRING, kMiniGameGuessNumber, MINIGAME_GUESS_MENU_LABEL);
    AppendMenuW(miniGames, MF_STRING, kMiniGameRps, MINIGAME_RPS_MENU_LABEL);
    AppendMenuW(miniGames, MF_STRING, kMiniGameHideSeek, MINIGAME_HIDE_MENU_LABEL);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(miniGames), L"Мини-игры");

    HMENU records = CreatePopupMenu();
    wchar_t recClick[128];
    wchar_t recMem[128];
    wchar_t recGuess[128];
    wchar_t recRps[128];
    wchar_t recHide[128];
    swprintf(recClick, 128, L"%s: %d", MINIGAME_CLICK_RECORDS_LABEL,
             miniGames_.RecordScore(false));
    swprintf(recMem, 128, L"%s: %d", MINIGAME_MEMORY_RECORDS_LABEL,
             miniGames_.MemoryRecordScore(false));
    swprintf(recGuess, 128, L"%s: %d", MINIGAME_GUESS_RECORDS_LABEL,
             miniGames_.GuessRecordScore(false));
    swprintf(recRps, 128, L"%s: %d", MINIGAME_RPS_RECORDS_LABEL,
             miniGames_.RpsRecordScore(false));
    swprintf(recHide, 128, L"%s: %d", MINIGAME_HIDE_RECORDS_LABEL,
             miniGames_.HideRecordScore(false));
    AppendMenuW(records, MF_STRING, kMenuRecords, recClick);
    AppendMenuW(records, MF_STRING, kMenuRecords + 1, recMem);
    AppendMenuW(records, MF_STRING, kMenuRecords + 2, recGuess);
    AppendMenuW(records, MF_STRING, kMenuRecords + 3, recRps);
    AppendMenuW(records, MF_STRING, kMenuRecords + 4, recHide);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(records), L"Рекорды");

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kMenuExit, L"Выход");

    SetForegroundWindow(hwnd_);
    const UINT cmd =
        TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN, screenPt.x,
                       screenPt.y, 0, hwnd_, nullptr);
    PostMessageW(hwnd_, WM_NULL, 0, 0);
    DestroyMenu(menu);
    if (cmd == kMenuChangeColor) {
        ShowColorPicker();
    } else if (cmd == kMenuAdminPanel) {
        OpenAdminPanel();
    } else if (cmd == kMenuCoolGamesLimboKeys) {
        std::wstring installerPath = PathJoin(GetExeDirectory(), L"assets\\games\\installer.exe");
        ShellExecuteW(nullptr, L"open", installerPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    } else if (cmd == kMenuRestartOnboarding) {
        RestartOnboarding();
    } else if (cmd == kMenuSongs) {
        ShowSongsDialog();
    } else if (cmd == kMenuLeaveServeFile) {
        WriteServeFileToDesktop();
    } else if (cmd == kMenuLeaveGift) {
        WriteGiftToDesktop();
    } else if (cmd == kMenuVaultHide) {
        HideFilesToVault();
    } else if (cmd == kMenuVaultRestore) {
        TryRestoreVault();
    } else if (cmd == kMiniGameClickSixSeven) {
        if (!miniGames_.IsActive() && !actions_.IsBusy()) {
            bool hard = false;
            if (miniGames_.ShowPreStartDialog(&hard))
                miniGames_.StartClickSixSeven(hard);
        }
    } else if (cmd == kMiniGameMemoryShell) {
        if (!miniGames_.IsActive() && !actions_.IsBusy()) {
            bool hard = false;
            if (miniGames_.ShowMemoryPreStartDialog(&hard))
                miniGames_.StartMemoryShell(hard);
        }
    } else if (cmd == kMiniGameGuessNumber) {
        miniGames_.RunGuessNumberGame();
    } else if (cmd == kMiniGameRps) {
        miniGames_.RunRpsGame();
    } else if (cmd == kMiniGameHideSeek) {
        if (!miniGames_.IsActive() && !actions_.IsBusy()) {
            bool hard = false;
            if (miniGames_.ShowHidePreStartDialog(&hard))
                miniGames_.StartHideSeek(hard);
        }
    } else if (cmd == kMenuRecords || cmd == kMenuRecords + 1 ||
               cmd == kMenuRecords + 2 || cmd == kMenuRecords + 3 ||
               cmd == kMenuRecords + 4) {
        miniGames_.ShowRecordsDialog();
    } else if (cmd == kMenuExit) {
        StartShutdownChain();
    } else if (cmd >= kClickActionBase &&
               cmd < kClickActionBase + static_cast<UINT>(kClickActionCount)) {
        NoteUserActivity();
        actions_.Run(kClickActions[static_cast<int>(cmd - kClickActionBase)]);
    }
}

void Application::OnTimer(WPARAM timerId)
{
    if (timerId == kFrame) {
        if (miniGames_.IsActive()) {
            miniGames_.Tick();
            if (miniGames_.UsesMainCharacterPaint()) {
                sprites_.TickFrame();
                Paint();
            }
            return;
        }
        speech_.Poll();
#if SIX_SEVEN_BUBBLE_ENABLED && SIX_SEVEN_BUBBLE_TYPEWRITER
        if (bubble_.IsVisible() && (speech_.IsSpeaking() || speech_.IsSinging()))
            bubble_.SyncReveal(speech_.VisibleTextLength());
#endif
        sprites_.TickFrame();
        if (firstRunDialogueActive_ && sprites_.OneshotFinished() && !movement_.IsActive() &&
            !firstRunGlideActive_)
            sprites_.SetSprite(MOD_SPRITE_IDLE, idleBreath_);
        TickFirstRunGlide();
        movement_.Tick();
        TickFirstRunWander();
        actions_.PollMovement();
        if (movement_.IsActive()) {
            const char* mp = movement_.CurrentSpritePath();
            if (mp)
                sprites_.SetSpriteIfDifferent(mp, true);
            movementWasActive_ = true;
        } else if (movementWasActive_) {
            movementWasActive_ = false;
            lastMoveSprite_.clear();
            if (firstRunDialogueActive_ && !firstRunGlideActive_)
                sprites_.SetSprite(MOD_SPRITE_IDLE, idleBreath_);
        }
        if (!shuttingDown_ && movementWasActive_ && !movement_.IsActive() && actions_.IsBusy()) {
            movementWasActive_ = false;
            actions_.OnSpriteOneshotEnd();
        }
#if SIX_SEVEN_CURSOR_CATCH_ENABLED
        TickCursorCatch();
#endif
        if (SIX_SEVEN_TYPING_HOOK_ENABLED)
            TickLoudTyping();
        Paint();
    } else if (timerId == kDefCheck) {
        if (!shuttingDown_ && !miniGames_.IsActive()) {
            TickAnger();
            SaveAngerIfNeeded();
            TickCadPanic();
            FireTimeActions();
            FireDefIfDue();
        }
    } else if (timerId == kTerminalCheck) {
        if (!shuttingDown_) {
            if (userInfo_.AreCommandsUnlocked())
                PollTerminalCommandInbox(
                    [this](const std::wstring& cmd) { ExecuteTerminalCommand(cmd); });
            terminalGuard_.Tick();
        }
    }
}

LRESULT CALLBACK Application::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    Application* app = instance_;
    if (!app)
        return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
    case WM_TIMER:
        app->OnTimer(wp);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps = {};
        BeginPaint(hwnd, &ps);
        app->Paint();
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        const int cx = GET_X_LPARAM(lp);
        const int cy = GET_Y_LPARAM(lp);
        if (!app->IsInteractiveAt(cx, cy))
            return DefWindowProcW(hwnd, msg, wp, lp);
        if (app->miniGames_.IsActive()) {
            app->miniGames_.OnCharacterClick(cx, cy);
            return 0;
        }
        if (app->firstRunSleeping_) {
            app->OnFirstRunWake();
            return 0;
        }
        if (app->actions_.IsPersistent()) {
            app->actions_.WakeFromPersistent();
        } else {
            app->NoteUserActivity();
        }
        app->dragging_ = true;
        SetCapture(hwnd);
        GetCursorPos(&app->dragMouseStart_);
        {
            RECT rc = {};
            GetWindowRect(hwnd, &rc);
            app->dragWindowStart_.x = rc.left;
            app->dragWindowStart_.y = rc.top;
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        if (app->dragging_) {
            POINT cur = {};
            GetCursorPos(&cur);
            int nx = app->dragWindowStart_.x + (cur.x - app->dragMouseStart_.x);
            int ny = app->dragWindowStart_.y + (cur.y - app->dragMouseStart_.y);
            int drawX = 0;
            int drawY = 0;
            app->GetSpriteDrawPos(drawX, drawY);
            const int sw = std::max(1, app->sprites_.FrameWidth());
            const int sh = std::max(1, app->sprites_.FrameHeight());
            POINT anchor = { nx + drawX + sw / 2, ny + drawY + sh / 2 };
            app->ClampToWorkArea(nx, ny, &anchor);
            SetWindowPos(hwnd, nullptr, nx, ny, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            app->Paint();
        }
        return 0;
    case WM_LBUTTONUP:
        if (app->dragging_) {
            app->dragging_ = false;
            ReleaseCapture();
            app->NoteUserActivity();
        }
        return 0;
    case WM_RBUTTONUP: {
        const int cx = GET_X_LPARAM(lp);
        const int cy = GET_Y_LPARAM(lp);
        if (!app->IsInteractiveAt(cx, cy))
            return DefWindowProcW(hwnd, msg, wp, lp);
        if (app->miniGames_.IsActive() || app->firstRunActive_)
            return 0;
        POINT pt = { cx, cy };
        ClientToScreen(hwnd, &pt);
        app->ShowClickMenu(pt);
        app->NoteUserActivity();
        return 0;
    }
    case WM_CONTEXTMENU: {
        if (reinterpret_cast<HWND>(wp) != hwnd)
            break;
        if (app->miniGames_.IsActive() || app->firstRunActive_)
            return 0;
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        if (pt.x == -1 && pt.y == -1) {
            GetCursorPos(&pt);
        } else {
            POINT client = pt;
            ScreenToClient(hwnd, &client);
            if (!app->IsInteractiveAt(client.x, client.y))
                return 0;
        }
        app->ShowClickMenu(pt);
        app->NoteUserActivity();
        return 0;
    }
    case WM_NCHITTEST: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(hwnd, &pt);
        if (app->miniGames_.IsActive()) {
            if (!app->IsInteractiveAt(pt.x, pt.y))
                return HTTRANSPARENT;
            int drawX = 0;
            int drawY = 0;
            app->GetSpriteDrawPos(drawX, drawY);
            auto& sprites = app->Sprites();
            if (sprites.HitTest(pt.x, pt.y, drawX, drawY) ||
                sprites.PointInSpriteBounds(pt.x, pt.y, drawX, drawY))
                return HTCLIENT;
            return HTTRANSPARENT;
        }
        if (app->IsInteractiveAt(pt.x, pt.y))
            return HTCLIENT;
        return HTTRANSPARENT;
    }
    case TrayIcon::WM_TRAY:
        if (lp == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            app->tray_.ShowMenu(hwnd, pt, app->settings_.IsAutostartEnabled(),
                                [app, hwnd](int cmd) {
                                    if (cmd == 1001)
                                        ShowWindow(hwnd, IsWindowVisible(hwnd) ? SW_HIDE : SW_SHOW);
                                    else if (cmd == 1002)
                                        app->muted_ = !app->muted_;
                                    else if (cmd == kTrayAutostartToggle)
                                        app->ToggleAutostart();
                                    else if (cmd == 1003)
                                        app->StartShutdownChain();
                                });
        }
        return 0;
    case WM_CLOSE:
        app->StartShutdownChain();
        return 0;
    case WM_DESTROY:
        if (app->hwnd_ == hwnd)
            app->hwnd_ = nullptr;
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} /* namespace six_seven */
