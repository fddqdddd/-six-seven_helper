#include "../include/MiniGameManager.h"
#include "../include/Application.h"
#include "../include/Util.h"
#include "../config.h"

#include <windowsx.h>
#include <commctrl.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace six_seven {

namespace {

enum PreStartCmd {
    kPreStartHardCheck = 4101,
};

enum HudIds {
    kHudTimer = 5001,
};

struct HudData {
    MiniGameManager* mgr = nullptr;
};

struct GlitchWndData {
    MiniGameManager* mgr = nullptr;
};

enum GuessGameCmds {
    kGuessEdit = 4201,
    kGuessSubmit = 4202,
    kGuessQuit = 4203,
};

enum RpsGameCmds {
    kRpsStatus = 4400,
    kRpsRock = 4301,
    kRpsPaper = 4302,
    kRpsScissors = 4303,
    kRpsQuit = 4304,
};

void RegisterClassOnce(HINSTANCE inst, const wchar_t* name, WNDPROC proc, bool& flag)
{
    if (flag)
        return;
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = name;
    RegisterClassExW(&wc);
    flag = true;
}

} /* namespace */

void MiniGameManager::Bind(Application* app)
{
    app_ = app;
    memory_.Bind(app, this, &records_);
}

void MiniGameManager::LoadRecords() { records_.Load(); }

int MiniGameManager::RecordScore(bool hardMode) const
{
    return records_.Get(MINIGAME_CLICK_ID, hardMode);
}

int MiniGameManager::MemoryRecordScore(bool hardMode) const
{
    return memory_.RecordScore(hardMode);
}

int MiniGameManager::GuessRecordScore(bool hardMode) const
{
    return records_.Get(MINIGAME_GUESS_ID, hardMode);
}

int MiniGameManager::RpsRecordScore(bool hardMode) const
{
    return records_.Get(MINIGAME_RPS_ID, hardMode);
}

bool MiniGameManager::RunGuessNumberGame()
{
    if (!app_ || activeGame_ != ActiveMiniGame::None)
        return false;

    RegisterClassOnce(app_->Inst(), L"SixSevenGuessGame", GuessGameWndProc,
                      guessClassRegistered_);

    GuessGameData data = {};
    data.mgr = this;

    // Спросить: обычный или hard-mode
    const int mode = MessageBoxW(app_->MainHwnd(),
                                 L"Угадай число, задуманное ШестьСемь.\n"
                                 L"Подсказки: «больше» и «меньше».\n\n"
                                 L"Играть в hard-mode? (число до 100, попыток больше)",
                                 MINIGAME_GUESS_TITLE, MB_YESNOCANCEL | MB_ICONQUESTION);
    if (mode == IDCANCEL)
        return false;

    const bool hard = (mode == IDYES);
    data.hard = hard;
    data.range = hard ? MINIGAME_GUESS_HARD_RANGE : MINIGAME_GUESS_NORMAL_RANGE;
    data.maxAttempts = hard ? MINIGAME_GUESS_HARD_ATTEMPTS : MINIGAME_GUESS_NORMAL_ATTEMPTS;
    data.attemptsLeft = data.maxAttempts;
    data.secret = RandomInt(1, data.range);

    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenGuessGame",
                               MINIGAME_GUESS_TITLE, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                               CW_USEDEFAULT, CW_USEDEFAULT, 372, 130, app_->MainHwnd(), nullptr,
                               app_->Inst(), &data);
    if (!dlg)
        return false;

    ShowWindow(dlg, SW_SHOW);
    EnableWindow(app_->MainHwnd(), FALSE);

    bool result = false;
    MSG msg = {};
    while (IsWindow(dlg) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(dlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(app_->MainHwnd(), TRUE);
    SetForegroundWindow(app_->MainHwnd());

    // Сохранение рекорда (серия угаданых подряд в окне).
    // GuessGameData живёт в wndproc; запишем рекорд внутри по завершении серии.
    return result;
}

bool MiniGameManager::RunRpsGame()
{
    if (!app_ || activeGame_ != ActiveMiniGame::None)
        return false;

    RegisterClassOnce(app_->Inst(), L"SixSevenRpsGame", RpsGameWndProc,
                      rpsClassRegistered_);

    const int mode = MessageBoxW(app_->MainHwnd(),
                                 L"Камень-ножницы-бумага против ШестьСемь!\n"
                                 L"Побеждает лучший из 7 раундов.\n\n"
                                 L"Играть в hard-mode? (67 подглядывает по-крупному)",
                                 MINIGAME_RPS_TITLE, MB_YESNOCANCEL | MB_ICONQUESTION);
    if (mode == IDCANCEL)
        return false;

    const bool hard = (mode == IDYES);

    RpsGameData data = {};
    data.mgr = this;
    data.hard = hard;
    data.maxRounds = MINIGAME_RPS_ROUNDS;

    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenRpsGame",
                               MINIGAME_RPS_TITLE, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                               CW_USEDEFAULT, CW_USEDEFAULT, 372, 200, app_->MainHwnd(), nullptr,
                               app_->Inst(), &data);
    if (!dlg)
        return false;

    ShowWindow(dlg, SW_SHOW);
    EnableWindow(app_->MainHwnd(), FALSE);

    MSG msg = {};
    while (IsWindow(dlg) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(dlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(app_->MainHwnd(), TRUE);
    SetForegroundWindow(app_->MainHwnd());
    return true;
}

void MiniGameManager::BeginHardGlitches(DWORD now)
{
    hardMode_ = true;
    nextGrayGlitchMs_ = now + static_cast<DWORD>(MINIGAME_GLITCH_GRAY_INTERVAL_MS);
    nextSpriteGlitchMs_ = now + static_cast<DWORD>(MINIGAME_GLITCH_SPRITE_INTERVAL_MS);
}

void MiniGameManager::TickHardGlitches(DWORD now)
{
    if (hardMode_)
        TickGlitches(now);
}

void MiniGameManager::StopHardGlitches()
{
    HideGrayGlitch();
    HideSpriteGlitch();
    nextGrayGlitchMs_ = 0;
    nextSpriteGlitchMs_ = 0;
    hardMode_ = false;
}

void MiniGameManager::OnMemoryGameStopped()
{
    if (activeGame_ == ActiveMiniGame::Memory)
        activeGame_ = ActiveMiniGame::None;
    StopHardGlitches();
}

bool MiniGameManager::ShowMemoryPreStartDialog(bool* hardModeOut)
{
    return memory_.ShowPreStartDialog(hardModeOut);
}

void MiniGameManager::StartMemoryShell(bool hardMode)
{
    if (!app_ || activeGame_ != ActiveMiniGame::None)
        return;
    activeGame_ = ActiveMiniGame::Memory;
    memory_.Start(hardMode);
}

void MiniGameManager::FormatTime(wchar_t* buf, size_t count, DWORD msLeft) const
{
    const DWORD sec = (msLeft + 999) / 1000;
    const DWORD mm = sec / 60;
    const DWORD ss = sec % 60;
    swprintf(buf, count, L"%u:%02u", mm, ss);
}

LRESULT CALLBACK MiniGameManager::PreStartWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* mgr = reinterpret_cast<MiniGameManager*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        mgr = static_cast<MiniGameManager*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(mgr));
        CreateWindowExW(0, L"STATIC", MINIGAME_CLICK_PRESTART_TEXT,
                        WS_CHILD | WS_VISIBLE, 16, 12, 340, 72, hwnd, nullptr, cs->hInstance,
                        nullptr);
        CreateWindowExW(0, L"BUTTON", MINIGAME_CLICK_HARD_CHECK_LABEL,
                        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 16, 92, 320, 22, hwnd,
                        reinterpret_cast<HMENU>(kPreStartHardCheck), cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Старт", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 72, 128,
                        90, 28, hwnd, reinterpret_cast<HMENU>(IDOK), cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Отмена", WS_CHILD | WS_VISIBLE, 190, 128, 90, 28, hwnd,
                        reinterpret_cast<HMENU>(IDCANCEL), cs->hInstance, nullptr);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK && mgr) {
            HWND check = GetDlgItem(hwnd, kPreStartHardCheck);
            mgr->preStartHard_ =
                check && SendMessageW(check, BM_GETCHECK, 0, 0) == BST_CHECKED;
            mgr->preStartStarted_ = true;
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(wp) == IDCANCEL) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool MiniGameManager::ShowPreStartDialog(bool* hardModeOut)
{
    if (!app_ || !hardModeOut)
        return false;
    RegisterClassOnce(app_->Inst(), L"SixSevenMiniPreStart", PreStartWndProc,
                      preStartClassRegistered_);

    preStartStarted_ = false;
    preStartHard_ = false;

    HWND dlg =
        CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenMiniPreStart",
                        MINIGAME_CLICK_PRESTART_TITLE, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                        CW_USEDEFAULT, CW_USEDEFAULT, 380, 200, app_->MainHwnd(), nullptr,
                        app_->Inst(), this);
    if (!dlg)
        return false;
    ShowWindow(dlg, SW_SHOW);
    EnableWindow(app_->MainHwnd(), FALSE);

    MSG msg = {};
    while (IsWindow(dlg) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(dlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(app_->MainHwnd(), TRUE);
    SetForegroundWindow(app_->MainHwnd());
    if (preStartStarted_)
        *hardModeOut = preStartHard_;
    return preStartStarted_;
}

LRESULT CALLBACK MiniGameManager::HudWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<HudData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        data = new HudData();
        data->mgr = static_cast<MiniGameManager*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        return 0;
    }
    case WM_NCHITTEST:
        return HTTRANSPARENT;
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

void MiniGameManager::PaintHud()
{
    if (!hudHwnd_ || !hudDibDc_ || !hudDibBits_ || !app_)
        return;

    const int w = 300;
    const int h = 96;
    auto* px = static_cast<BYTE*>(hudDibBits_);
    std::memset(px, 0, static_cast<size_t>(w) * h * 4);

    HDC mem = hudDibDc_;
    RECT rc = { 0, 0, w, h };
    HBRUSH bg = CreateSolidBrush(RGB(255, 252, 230));
    FillRect(mem, &rc, bg);
    DeleteObject(bg);
    FrameRect(mem, &rc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    SetBkMode(mem, TRANSPARENT);

    const DWORD now = GetTickCount();
    const DWORD left = endMs_ > now ? endMs_ - now : 0;
    wchar_t tbuf[32];
    FormatTime(tbuf, 32, left);
    wchar_t scoreLine[64];
    swprintf(scoreLine, 64, L"Очки: %d", score_);

    HFONT scoreFont =
        CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                    DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT timerFont =
        CreateFontW(-44, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                    DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HGDIOBJ old = SelectObject(mem, scoreFont);

    RECT scoreRc = { 8, 6, w - 8, 28 };
    SetTextColor(mem, RGB(40, 40, 40));
    DrawTextW(mem, scoreLine, -1, &scoreRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(mem, timerFont);
    RECT timerRc = { 8, 30, w - 8, h - 8 };
    const DWORD leftSec = (left + 999) / 1000;
    if (leftSec <= 10)
        SetTextColor(mem, RGB(200, 0, 0));
    else
        SetTextColor(mem, RGB(0, 70, 150));
    DrawTextW(mem, tbuf, -1, &timerRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(mem, old);
    DeleteObject(scoreFont);
    DeleteObject(timerFont);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            BYTE* p = px + (static_cast<size_t>(y) * w + x) * 4;
            p[3] = 255;
        }
    }

    POINT ptSrc = { 0, 0 };
    SIZE size = { w, h };
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    POINT ptDst = {};
    RECT wr = {};
    GetWindowRect(hudHwnd_, &wr);
    ptDst.x = wr.left;
    ptDst.y = wr.top;
    HDC screen = GetDC(nullptr);
    UpdateLayeredWindow(hudHwnd_, screen, &ptDst, &size, hudDibDc_, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(nullptr, screen);
}

void MiniGameManager::CreateHud()
{
    if (!app_ || hudHwnd_)
        return;
    RegisterClassOnce(app_->Inst(), L"SixSevenGameHud", HudWndProc, hudClassRegistered_);

    const int w = 300;
    const int h = 96;
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    HDC screen = GetDC(nullptr);
    hudDib_ = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &hudDibBits_, nullptr, 0);
    hudDibDc_ = CreateCompatibleDC(screen);
    SelectObject(hudDibDc_, hudDib_);
    ReleaseDC(nullptr, screen);

    const RECT wa = GetCombinedWorkArea();
    const int x = wa.left + (wa.right - wa.left - w) / 2;
    const int y = wa.top + 12;
    hudHwnd_ = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"SixSevenGameHud",
                                 L"", WS_POPUP, x, y, w, h, nullptr, nullptr, app_->Inst(), this);
    if (hudHwnd_) {
        PaintHud();
        ShowWindow(hudHwnd_, SW_SHOWNA);
    }
}

void MiniGameManager::DestroyHud()
{
    if (hudHwnd_) {
        DestroyWindow(hudHwnd_);
        hudHwnd_ = nullptr;
    }
    if (hudDibDc_) {
        DeleteDC(hudDibDc_);
        hudDibDc_ = nullptr;
    }
    if (hudDib_) {
        DeleteObject(hudDib_);
        hudDib_ = nullptr;
    }
    hudDibBits_ = nullptr;
}

void MiniGameManager::UpdateHud()
{
    if (!hudHwnd_)
        return;
    SetWindowPos(hudHwnd_, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    PaintHud();
}

LRESULT CALLBACK MiniGameManager::GrayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps = {};
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc = {};
        GetClientRect(hwnd, &rc);
        HBRUSH br = CreateSolidBrush(RGB(128, 128, 128));
        FillRect(hdc, &rc, br);
        DeleteObject(br);
        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void MiniGameManager::ShowGrayGlitch(DWORD now)
{
    if (!app_ || grayHwnd_ || !MINIGAME_GLITCH_GRAY_ENABLED || !hardMode_)
        return;
    RegisterClassOnce(app_->Inst(), L"SixSevenGrayGlitch", GrayWndProc, grayClassRegistered_);
    const RECT wa = GetCombinedWorkArea();
    grayHwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, L"SixSevenGrayGlitch", L"",
        WS_POPUP, wa.left, wa.top, wa.right - wa.left, wa.bottom - wa.top, nullptr, nullptr,
        app_->Inst(), nullptr);
    if (!grayHwnd_)
        return;
    SetLayeredWindowAttributes(grayHwnd_, 0,
                               static_cast<BYTE>(std::min(255, MINIGAME_GLITCH_GRAY_ALPHA)),
                               LWA_ALPHA);
    ShowWindow(grayHwnd_, SW_SHOWNA);
    grayHideMs_ = now + static_cast<DWORD>(MINIGAME_GLITCH_GRAY_DURATION_MS);
}

void MiniGameManager::HideGrayGlitch()
{
    if (grayHwnd_) {
        DestroyWindow(grayHwnd_);
        grayHwnd_ = nullptr;
    }
    grayHideMs_ = 0;
}

void MiniGameManager::PaintGlitchSprite()
{
    if (!glitchHwnd_ || !glitchDibDc_)
        return;
    const int w = SIX_SEVEN_WINDOW_WIDTH;
    const int h = SIX_SEVEN_WINDOW_HEIGHT;
    auto* px = static_cast<BYTE*>(glitchDibBits_);
    std::memset(px, 0, static_cast<size_t>(w) * h * 4);
    int drawX = 0;
    int drawY = 0;
    drawY = SIX_SEVEN_BUBBLE_SPACE_H;
    drawX = (w - SIX_SEVEN_SPRITE_WIDTH) / 2;
    int top = 0;
    glitchSprites_.Draw(glitchDibDc_, drawX, drawY, h, top);
    POINT ptSrc = { 0, 0 };
    SIZE size = { w, h };
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    POINT ptDst = {};
    RECT wr = {};
    GetWindowRect(glitchHwnd_, &wr);
    ptDst.x = wr.left;
    ptDst.y = wr.top;
    HDC screen = GetDC(nullptr);
    UpdateLayeredWindow(glitchHwnd_, screen, &ptDst, &size, glitchDibDc_, &ptSrc, 0, &blend,
                        ULW_ALPHA);
    ReleaseDC(nullptr, screen);
}

LRESULT CALLBACK MiniGameManager::GlitchSpriteWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<GlitchWndData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE:
        return 0;
    case WM_PAINT:
        if (data && data->mgr)
            data->mgr->PaintGlitchSprite();
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void MiniGameManager::ShowSpriteGlitch(DWORD now)
{
    if (!app_ || glitchHwnd_ || !MINIGAME_GLITCH_SPRITE_ENABLED || !hardMode_)
        return;
    if (!glitchSprites_.Init())
        return;
    glitchSprites_.SetSprite(MOD_SPRITE_MINIGAME_GLITCH, true);

    RegisterClassOnce(app_->Inst(), L"SixSevenGlitchSprite", GlitchSpriteWndProc,
                      glitchClassRegistered_);

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = SIX_SEVEN_WINDOW_WIDTH;
    bi.bmiHeader.biHeight = -SIX_SEVEN_WINDOW_HEIGHT;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    HDC screen = GetDC(nullptr);
    glitchDib_ = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &glitchDibBits_, nullptr, 0);
    glitchDibDc_ = CreateCompatibleDC(screen);
    SelectObject(glitchDibDc_, glitchDib_);
    ReleaseDC(nullptr, screen);

    const RECT wa = GetCombinedWorkArea();
    const int ww = SIX_SEVEN_WINDOW_WIDTH;
    const int wh = SIX_SEVEN_WINDOW_HEIGHT;
    int gx = RandomInt(wa.left + 20, std::max(wa.left + 20, wa.right - ww - 20));
    int gy = RandomInt(wa.top + 20, std::max(wa.top + 20, wa.bottom - wh - 20));

    auto* gd = new GlitchWndData();
    gd->mgr = this;

    glitchHwnd_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
        L"SixSevenGlitchSprite", L"", WS_POPUP, gx, gy, ww, wh, nullptr, nullptr, app_->Inst(),
        nullptr);
    if (!glitchHwnd_) {
        delete gd;
        HideSpriteGlitch();
        return;
    }
    SetWindowLongPtrW(glitchHwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(gd));
    PaintGlitchSprite();
    ShowWindow(glitchHwnd_, SW_SHOWNA);
    spriteGlitchHideMs_ = now + static_cast<DWORD>(MINIGAME_GLITCH_SPRITE_DURATION_MS);
}

void MiniGameManager::HideSpriteGlitch()
{
    if (glitchHwnd_) {
        auto* gd = reinterpret_cast<GlitchWndData*>(GetWindowLongPtrW(glitchHwnd_, GWLP_USERDATA));
        DestroyWindow(glitchHwnd_);
        glitchHwnd_ = nullptr;
        delete gd;
    }
    if (glitchDibDc_) {
        DeleteDC(glitchDibDc_);
        glitchDibDc_ = nullptr;
    }
    if (glitchDib_) {
        DeleteObject(glitchDib_);
        glitchDib_ = nullptr;
    }
    glitchDibBits_ = nullptr;
    spriteGlitchHideMs_ = 0;
    glitchSprites_.Shutdown();
}

void MiniGameManager::TickGlitches(DWORD now)
{
    if (!hardMode_)
        return;
    if (grayHideMs_ != 0 && now >= grayHideMs_)
        HideGrayGlitch();
    if (spriteGlitchHideMs_ != 0 && now >= spriteGlitchHideMs_)
        HideSpriteGlitch();
    if (glitchHwnd_) {
        glitchSprites_.TickFrame();
        PaintGlitchSprite();
    }
    if (grayHwnd_ == nullptr && nextGrayGlitchMs_ != 0 && now >= nextGrayGlitchMs_) {
        ShowGrayGlitch(now);
        nextGrayGlitchMs_ = now + static_cast<DWORD>(MINIGAME_GLITCH_GRAY_INTERVAL_MS);
    }
    if (glitchHwnd_ == nullptr && nextSpriteGlitchMs_ != 0 && now >= nextSpriteGlitchMs_) {
        ShowSpriteGlitch(now);
        nextSpriteGlitchMs_ = now + static_cast<DWORD>(MINIGAME_GLITCH_SPRITE_INTERVAL_MS);
    }
}

void MiniGameManager::TeleportCharacter()
{
    if (!app_)
        return;
    int tx = 0;
    int ty = 0;
    RandomSpriteWindowPos(tx, ty, SIX_SEVEN_MOVE_MARGIN_PX);
    int drawX = 0;
    int drawY = 0;
    app_->GetSpriteDrawPos(drawX, drawY);
    const int sw = std::max(1, app_->Sprites().FrameWidth());
    const int sh = std::max(1, app_->Sprites().FrameHeight());
    POINT anchor = { tx + drawX + sw / 2, ty + drawY + sh / 2 };
    app_->ClampMainWindow(tx, ty, &anchor);
    SetWindowPos(app_->MainHwnd(), HWND_TOPMOST, tx, ty, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
    app_->RepaintMain();
}

void MiniGameManager::StartClickSixSeven(bool hardMode)
{
    if (!app_ || activeGame_ != ActiveMiniGame::None)
        return;
    app_->Actions().Cancel();
    app_->Bubble().Clear();

    hardMode_ = hardMode;
    score_ = 0;
    const DWORD durationSec =
        hardMode_ ? static_cast<DWORD>(MINIGAME_CLICK_HARD_TIME_SEC)
                  : static_cast<DWORD>(MINIGAME_CLICK_TIME_SEC);
    startMs_ = GetTickCount();
    endMs_ = startMs_ + durationSec * 1000;
    lastMoveMs_ = 0;
    lastHudMs_ = 0;

    const char* sprite =
        hardMode_ ? MOD_SPRITE_MINIGAME_CLICK_SIX_SEVEN_HARD : MOD_SPRITE_MINIGAME_CLICK_SIX_SEVEN;
    app_->Sprites().SetSprite(sprite, true);

    RECT rc = {};
    GetWindowRect(app_->MainHwnd(), &rc);
    savedWinLeft_ = rc.left;
    savedWinTop_ = rc.top;
    savedWinPos_ = true;

    activeGame_ = ActiveMiniGame::Click;
    CreateHud();
    TeleportCharacter();
    lastMoveMs_ = GetTickCount();

    if (hardMode_) {
        const DWORD now = GetTickCount();
        nextGrayGlitchMs_ = now + static_cast<DWORD>(MINIGAME_GLITCH_GRAY_INTERVAL_MS);
        nextSpriteGlitchMs_ = now + static_cast<DWORD>(MINIGAME_GLITCH_SPRITE_INTERVAL_MS);
    } else {
        nextGrayGlitchMs_ = 0;
        nextSpriteGlitchMs_ = 0;
    }
    UpdateHud();
    app_->RepaintMain();
}

bool MiniGameManager::OnCharacterClick(int clientX, int clientY)
{
    if (activeGame_ != ActiveMiniGame::Click || !app_)
        return false;
    int drawX = 0;
    int drawY = 0;
    app_->GetSpriteDrawPos(drawX, drawY);
    auto& sprites = app_->Sprites();
    const bool hit = sprites.HitTest(clientX, clientY, drawX, drawY) ||
                     sprites.PointInSpriteBounds(clientX, clientY, drawX, drawY);
    if (!hit)
        return false;
    score_ += MINIGAME_CLICK_SCORE_PER_HIT;
    TeleportCharacter();
    lastMoveMs_ = GetTickCount();
    UpdateHud();
    app_->RepaintMain();
    return true;
}

void MiniGameManager::EndGame()
{
    if (activeGame_ != ActiveMiniGame::Click || !app_)
        return;
    const int finalScore = score_;
    const bool hard = hardMode_;
    const DWORD elapsed = GetTickCount() - startMs_;
    const int restoreX = savedWinLeft_;
    const int restoreY = savedWinTop_;
    const bool restorePos = savedWinPos_;

    Stop();

    const bool newRecord = records_.TrySave(MINIGAME_CLICK_ID, hard, finalScore);
    if (newRecord && app_)
        app_->AwardVaultFragment();

    wchar_t timeBuf[32];
    FormatTime(timeBuf, 32, elapsed);

    wchar_t body[768];
    swprintf(body, 768, L"%s\r\n\r\nОчки: %d\r\nВремя игры: %s",
             MINIGAME_CLICK_END_TEXT, finalScore, timeBuf);
    if (newRecord)
        wcscat_s(body, MINIGAME_CLICK_NEW_RECORD_SUFFIX);

    MessageBoxW(app_->MainHwnd(), body, MINIGAME_CLICK_END_TITLE, MB_OK | MB_ICONINFORMATION);

    if (restorePos) {
        SetWindowPos(app_->MainHwnd(), HWND_TOPMOST, restoreX, restoreY, 0, 0,
                     SWP_NOSIZE | SWP_NOACTIVATE);
    }
    app_->ReturnToIdleSprite();
}

void MiniGameManager::Stop()
{
    if (memory_.IsActive())
        memory_.Stop();
    HideGrayGlitch();
    HideSpriteGlitch();
    DestroyHud();
    StopHardGlitches();
    activeGame_ = ActiveMiniGame::None;
}

void MiniGameManager::Tick()
{
    if (activeGame_ == ActiveMiniGame::Memory) {
        memory_.Tick();
        return;
    }
    if (activeGame_ != ActiveMiniGame::Click || !app_)
        return;
    const DWORD now = GetTickCount();
    if (now >= endMs_) {
        EndGame();
        return;
    }

    const DWORD moveInterval = hardMode_
                                   ? static_cast<DWORD>(MINIGAME_CLICK_HARD_MOVE_MS)
                                   : static_cast<DWORD>(MINIGAME_CLICK_MOVE_INTERVAL_MS);
    if (now - lastMoveMs_ >= moveInterval) {
        TeleportCharacter();
        lastMoveMs_ = now;
    }

    UpdateHud();

    TickGlitches(now);
}

LRESULT CALLBACK MiniGameManager::GuessGameWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<GuessGameData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        const auto* init = static_cast<const GuessGameData*>(cs->lpCreateParams);
        data = new GuessGameData();
        data->mgr = init->mgr;
        data->range = init->range;
        data->maxAttempts = init->maxAttempts;
        data->attemptsLeft = init->attemptsLeft;
        data->secret = init->secret;
        data->hard = init->hard;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        data->status = CreateWindowExW(0, L"STATIC", L"",
                                       WS_CHILD | WS_VISIBLE, 16, 12, 340, 40, hwnd, nullptr,
                                       cs->hInstance, nullptr);
        wchar_t intro[192];
        swprintf(intro, 192, L"Я загадал число от 1 до %d. Попыток: %d",
                 data->range, data->maxAttempts);
        SetWindowTextW(data->status, intro);
        data->edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP, 16, 58, 120, 24, hwnd,
                                     reinterpret_cast<HMENU>(kGuessEdit), cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Проверить", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 150,
                        58, 90, 26, hwnd, reinterpret_cast<HMENU>(kGuessSubmit), cs->hInstance,
                        nullptr);
        CreateWindowExW(0, L"BUTTON", L"Выйти", WS_CHILD | WS_VISIBLE, 280, 58, 60, 26, hwnd,
                        reinterpret_cast<HMENU>(kGuessQuit), cs->hInstance, nullptr);
        return 0;
    }
    case WM_COMMAND: {
        if (!data)
            break;
        if (LOWORD(wp) == kGuessSubmit && !data->done) {
            data->mgr->OnGuessSubmit(hwnd, data);
            return 0;
        }
        if (LOWORD(wp) == kGuessQuit) {
            DestroyWindow(hwnd);
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

void MiniGameManager::OnGuessSubmit(HWND hwnd, GuessGameData* data) const
{
    wchar_t buf[48] = {};
    GetWindowTextW(data->edit, buf, 48);
    const int guess = ::_wtoi(buf);
    wchar_t text[512];
    if (guess == data->secret) {
        data->streak += 1;
        swprintf(text, 512, L"Угадал! Серия: %d. Загадываю новое число от 1 до %d.",
                 data->streak, data->range);
        SetWindowTextW(data->status, text);
        data->attemptsLeft = data->maxAttempts;
        data->secret = RandomInt(1, data->range);
        SetWindowTextW(data->edit, L"");
        SetFocus(data->edit);
        return;
    }
    data->attemptsLeft -= 1;
    if (data->attemptsLeft <= 0) {
        swprintf(text, 512, L"Попытки кончились! Было загадано %d. Серия: %d.",
                 data->secret, data->streak);
        SetWindowTextW(data->status, text);
        if (data->mgr && data->streak > 0)
            data->mgr->records_.TrySave(MINIGAME_GUESS_ID, data->hard, data->streak);
        if (data->mgr && data->mgr->app_ && data->streak > 0)
            data->mgr->app_->AwardVaultFragment();
        data->done = true;
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
    } else {
        swprintf(text, 512, L"Загаданное число %s! Осталось попыток: %d",
                 guess < data->secret ? L"больше" : L"меньше", data->attemptsLeft);
        SetWindowTextW(data->status, text);
    }
    SetFocus(data->edit);
}

LRESULT CALLBACK MiniGameManager::RpsGameWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<RpsGameData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        data = new RpsGameData();
        data->mgr = static_cast<MiniGameManager*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        CreateWindowExW(0, L"STATIC",
                        L"Камень-ножницы-бумага! Выбери ход. Сыграют 7 раундов с ШестьСемь.",
                        WS_CHILD | WS_VISIBLE, 16, 12, 340, 36, hwnd, nullptr, cs->hInstance,
                        nullptr);
        data->status = CreateWindowExW(0, L"STATIC", L"Раунд 1 / 7 — Счёт: ты 0 : 0 67",
                                       WS_CHILD | WS_VISIBLE, 16, 52, 340, 20, hwnd,
                                       reinterpret_cast<HMENU>(kRpsStatus), cs->hInstance,
                                       nullptr);
        CreateWindowExW(0, L"BUTTON", L"Камень", WS_CHILD | WS_VISIBLE, 16, 90, 100, 28, hwnd,
                        reinterpret_cast<HMENU>(kRpsRock), cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Ножницы", WS_CHILD | WS_VISIBLE, 124, 90, 100, 28, hwnd,
                        reinterpret_cast<HMENU>(kRpsScissors), cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Бумага", WS_CHILD | WS_VISIBLE, 232, 90, 100, 28, hwnd,
                        reinterpret_cast<HMENU>(kRpsPaper), cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Выйти", WS_CHILD | WS_VISIBLE, 16, 128, 88, 26, hwnd,
                        reinterpret_cast<HMENU>(kRpsQuit), cs->hInstance, nullptr);
        return 0;
    }
    case WM_COMMAND: {
        if (!data)
            break;
        switch (LOWORD(wp)) {
        case kRpsRock:
        case kRpsPaper:
        case kRpsScissors:
            data->mgr->OnRpsMove(hwnd, data, LOWORD(wp));
            return 0;
        case kRpsQuit:
            DestroyWindow(hwnd);
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

void MiniGameManager::OnRpsMove(HWND hwnd, RpsGameData* data, int pick)
{
    enum RpsPick { Rock = 0, Paper = 1, Scissors = 2 };
    const RpsPick user = static_cast<RpsPick>(pick == kRpsRock   ? Rock
                                             : pick == kRpsPaper ? Paper
                                                                 : Scissors);
    RpsPick cpu = Rock;
    const int r = RandomInt(0, 2);
    if (r == 0)
        cpu = Rock;
    else if (r == 1)
        cpu = Paper;
    else
        cpu = Scissors;

    const wchar_t* names[] = { L"Камень", L"Бумага", L"Ножницы" };
    wchar_t text[384];
    bool userWon = false;
    bool tie = (user == cpu);
    if (!tie) {
        if ((user == Rock && cpu == Scissors) || (user == Paper && cpu == Rock) ||
            (user == Scissors && cpu == Paper))
            userWon = true;
    }

    if (userWon) {
        data->userWins += 1;
        swprintf(text, 384, L"Ты: %s — 67: %s. Ты победил!", names[user], names[cpu]);
    } else if (tie) {
        swprintf(text, 384, L"Ты: %s — 67: %s. Ничья!", names[user], names[cpu]);
    } else {
        data->cpuWins += 1;
        swprintf(text, 384, L"Ты: %s — 67: %s. 67 победил.", names[user], names[cpu]);
    }
    SetWindowTextW(data->status, text);

    data->round += 1;
    if (data->round > data->maxRounds) {
        wchar_t result[384];
        swprintf(result, 384,
                 L"Игра окончена: ты %d : %d 67.\r\n%s", data->userWins, data->cpuWins,
                 data->userWins > data->cpuWins ? L"Ты победил!" : L"67 победил.");
        if (data->userWins > data->cpuWins && data->mgr)
            if (data->mgr->records_.TrySave(MINIGAME_RPS_ID, data->hard, data->userWins))
                wcscat_s(result, L"\r\nНовый рекорд!");
        if (data->userWins > data->cpuWins && data->mgr && data->mgr->app_)
            data->mgr->app_->AwardVaultFragment();
        MessageBoxW(hwnd, result, MINIGAME_RPS_TITLE, MB_OK | MB_ICONINFORMATION);
        DestroyWindow(hwnd);
        return;
    }

    wchar_t score[96];
    swprintf(score, 96, L"Раунд %d / %d — Счёт: ты %d : %d 67", data->round, data->maxRounds,
             data->userWins, data->cpuWins);
    SetWindowTextW(data->status, score);
}

void MiniGameManager::ShowRecordsDialog()
{
    if (!app_)
        return;
    const int clickNormal = records_.Get(MINIGAME_CLICK_ID, false);
    const int clickHard = records_.Get(MINIGAME_CLICK_ID, true);
    const int memNormal = records_.Get(MINIGAME_MEMORY_ID, false);
    const int memHard = records_.Get(MINIGAME_MEMORY_ID, true);
    const int guessNormal = records_.Get(MINIGAME_GUESS_ID, false);
    const int guessHard = records_.Get(MINIGAME_GUESS_ID, true);
    const int rpsNormal = records_.Get(MINIGAME_RPS_ID, false);
    const int rpsHard = records_.Get(MINIGAME_RPS_ID, true);
    wchar_t text[1024];
    swprintf(text, 1024,
             L"%s (обычный): %d\r\n%s (hard-mode): %d\r\n\r\n"
             L"%s (обычный): %d\r\n%s (hard-mode): %d\r\n\r\n"
             L"%s (обычный): %d\r\n%s (hard-mode): %d\r\n\r\n"
             L"%s (обычный): %d\r\n%s (hard-mode): %d\r\n\r\n"
             L"Рекорды в файле %hs",
             MINIGAME_CLICK_RECORDS_LABEL, clickNormal, MINIGAME_CLICK_RECORDS_LABEL, clickHard,
             MINIGAME_MEMORY_RECORDS_LABEL, memNormal, MINIGAME_MEMORY_RECORDS_LABEL, memHard,
             MINIGAME_GUESS_RECORDS_LABEL, guessNormal, MINIGAME_GUESS_RECORDS_LABEL, guessHard,
             MINIGAME_RPS_RECORDS_LABEL, rpsNormal, MINIGAME_RPS_RECORDS_LABEL, rpsHard,
             MINIGAME_RECORDS_INI);
    MessageBoxW(app_->MainHwnd(), text, L"Рекорды — мини-игры", MB_OK | MB_ICONINFORMATION);
}

} /* namespace six_seven */
