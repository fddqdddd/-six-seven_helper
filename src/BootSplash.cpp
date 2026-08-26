#include "../include/BootSplash.h"
#include "../include/Util.h"
#include "../config.h"

#include <gdiplus.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace six_seven {

bool BootSplash::classRegistered_ = false;
bool BootSplash::wasShown_ = false;

bool IsBootLaunch()
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return false;
    bool boot = false;
    for (int i = 1; i < argc; ++i) {
        if (_wcsicmp(argv[i], L"--boot") == 0) {
            boot = true;
            break;
        }
    }
    LocalFree(argv);
    return boot;
}

namespace {

const wchar_t* kRunKey =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t* kRunValueName = L"Six_Seven";

int ReadBootIniInt(const wchar_t* key, int defaultValue)
{
    const std::wstring ini = PathJoin(GetExeDirectory(), L"six_seven.ini");
    return GetPrivateProfileIntW(L"boot", key, defaultValue, ini.c_str());
}

bool BootIniEnabled()
{
    return ReadBootIniInt(L"enabled", SIX_SEVEN_BOOT_SPLASH_DEFAULT) != 0;
}

bool BootIniAutostart()
{
    return ReadBootIniInt(L"autostart", SIX_SEVEN_BOOT_AUTOSTART_DEFAULT) != 0;
}

int BootIniMinSec()
{
    int sec = ReadBootIniInt(L"min_sec", SIX_SEVEN_BOOT_SPLASH_MIN_SEC);
    if (sec < 1)
        sec = 1;
    if (sec > 300)
        sec = 300;
    return sec;
}

int FrameNumberFromPath(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L"\\/");
    const std::wstring name = slash == std::wstring::npos ? path : path.substr(slash + 1);
    int num = 0;
    bool any = false;
    for (wchar_t c : name) {
        if (c >= L'0' && c <= L'9') {
            num = num * 10 + (c - L'0');
            any = true;
        }
    }
    return any ? num : 0;
}

bool FramePathLess(const std::wstring& a, const std::wstring& b)
{
    const int na = FrameNumberFromPath(a);
    const int nb = FrameNumberFromPath(b);
    if (na != nb)
        return na < nb;
    return a < b;
}

COLORREF MagentaKey() { return static_cast<COLORREF>(SIX_SEVEN_COLORKEY); }

bool IsColorkeyRgb(BYTE r, BYTE g, BYTE b)
{
#if !SIX_SEVEN_SPRITE_USE_COLORKEY
    (void)r;
    (void)g;
    (void)b;
    return false;
#else
    const COLORREF key = MagentaKey();
    if (RGB(r, g, b) == key)
        return true;
    const int kr = GetRValue(key);
    const int kg = GetGValue(key);
    const int kb = GetBValue(key);
    const int tol = SIX_SEVEN_COLORKEY_TOLERANCE;
    return std::abs(static_cast<int>(r) - kr) <= tol &&
           std::abs(static_cast<int>(g) - kg) <= tol &&
           std::abs(static_cast<int>(b) - kb) <= tol;
#endif
}

HBITMAP CreateBitmapFromGdiplus(Gdiplus::Bitmap* bmp, int& outW, int& outH)
{
    const int w = static_cast<int>(bmp->GetWidth());
    const int h = static_cast<int>(bmp->GetHeight());
    if (w <= 0 || h <= 0)
        return nullptr;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    void* bits = nullptr;
    HDC screen = GetDC(nullptr);
    HBITMAP hbmp = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hbmp || !bits) {
        ReleaseDC(nullptr, screen);
        return nullptr;
    }

    Gdiplus::Bitmap dst(w, h, PixelFormat32bppARGB);
    Gdiplus::Graphics g(&dst);
    g.DrawImage(bmp, 0, 0, w, h);

    Gdiplus::BitmapData data = {};
    if (dst.LockBits(nullptr, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) ==
        Gdiplus::Ok) {
        const auto* src = static_cast<const BYTE*>(data.Scan0);
        auto* dstBits = static_cast<BYTE*>(bits);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const BYTE* p = src + y * data.Stride + x * 4;
                BYTE* d = dstBits + (static_cast<size_t>(y) * w + x) * 4;
                const BYTE b = p[0], gch = p[1], r = p[2], a = p[3];
                if (a < 8 || IsColorkeyRgb(r, gch, b)) {
                    d[0] = d[1] = d[2] = 0;
                    d[3] = 0;
                } else {
                    d[0] = b;
                    d[1] = gch;
                    d[2] = r;
                    d[3] = a;
                }
            }
        }
        dst.UnlockBits(&data);
    }

    outW = w;
    outH = h;
    ReleaseDC(nullptr, screen);
    return hbmp;
}

HBITMAP MakePlaceholderFrame(int w, int h)
{
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    void* bits = nullptr;
    HDC screen = GetDC(nullptr);
    HBITMAP hbmp = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hbmp || !bits) {
        ReleaseDC(nullptr, screen);
        return nullptr;
    }

    HDC mem = CreateCompatibleDC(screen);
    HGDIOBJ old = SelectObject(mem, hbmp);
    RECT rc = { 0, 0, w, h };
    HBRUSH bg = CreateSolidBrush(SIX_SEVEN_BOOT_SPLASH_BG_RGB);
    FillRect(mem, &rc, bg);
    DeleteObject(bg);
    SetBkMode(mem, TRANSPARENT);
    SetTextColor(mem, RGB(220, 220, 255));
    HFONT font = CreateFontW(
        48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HGDIOBJ oldFont = SelectObject(mem, font);
    DrawTextW(mem, L"Six_Seven\nloading...", -1, &rc, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
    SelectObject(mem, oldFont);
    DeleteObject(font);
    SelectObject(mem, old);
    DeleteDC(mem);
    ReleaseDC(nullptr, screen);

    if (bits) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                BYTE* p = static_cast<BYTE*>(bits) + (static_cast<size_t>(y) * w + x) * 4;
                p[3] = 255;
            }
        }
    }
    return hbmp;
}

void BlitScaled(HDC dst, HBITMAP src, int srcW, int srcH, int dx, int dy, int dw, int dh)
{
    HDC mem = CreateCompatibleDC(dst);
    HGDIOBJ old = SelectObject(mem, src);
    SetStretchBltMode(dst, HALFTONE);
    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;
    GdiAlphaBlend(dst, dx, dy, dw, dh, mem, 0, 0, srcW, srcH, bf);
    SelectObject(mem, old);
    DeleteDC(mem);
}

} /* namespace */

bool BootSplash::ShouldShow()
{
#if !SIX_SEVEN_BOOT_SPLASH_ENABLED
    return false;
#else
    if (!IsBootLaunch())
        return false;
    if (!BootIniEnabled())
        return false;
    if (GetTickCount() > static_cast<DWORD>(SIX_SEVEN_BOOT_TICK_THRESHOLD_MS))
        return false;
    return true;
#endif
}

void BootSplash::SyncAutostart(bool enable)
{
#if !SIX_SEVEN_BOOT_AUTOSTART_ENABLED
    (void)enable;
    return;
#else
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        return;

    if (enable) {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        wchar_t value[MAX_PATH + 4] = {};
        wsprintfW(value, L"\"%s\"", path);
        RegSetValueExW(key, kRunValueName, 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(value),
                       static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(key, kRunValueName);
    }
    RegCloseKey(key);
#endif
}

LRESULT CALLBACK BootSplash::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_NCHITTEST)
        return HTCLIENT;
    if (msg == WM_ERASEBKGND)
        return 1;
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool BootSplash::Create(HINSTANCE inst)
{
    inst_ = inst;
    fps_ = FPS_ANIM_LOADING;

    screenX_ = GetSystemMetrics(SM_XVIRTUALSCREEN);
    screenY_ = GetSystemMetrics(SM_YVIRTUALSCREEN);
    screenW_ = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    screenH_ = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (screenW_ <= 0 || screenH_ <= 0)
        return false;

    if (!classRegistered_) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WndProc;
        wc.hInstance = inst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"SixSevenBootSplash";
        if (!RegisterClassExW(&wc))
            return false;
        classRegistered_ = true;
    }

    if (!LoadFrames())
        return false;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = screenW_;
    bi.bmiHeader.biHeight = -screenH_;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    HDC screen = GetDC(nullptr);
    dib_ = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &dibBits_, nullptr, 0);
    dibDc_ = CreateCompatibleDC(screen);
    SelectObject(dibDc_, dib_);
    ReleaseDC(nullptr, screen);
    if (!dib_ || !dibDc_)
        return false;

    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
    hwnd_ = CreateWindowExW(exStyle, L"SixSevenBootSplash", L"Six_Seven Loading", WS_POPUP,
                              screenX_, screenY_, screenW_, screenH_, nullptr, nullptr, inst,
                              nullptr);
    if (!hwnd_)
        return false;

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    SetForegroundWindow(hwnd_);
    startedMs_ = GetTickCount();
    lastFrameMs_ = 0;
    frameIndex_ = 0;
    loopCount_ = 0;
    active_ = true;
    Paint();
    return true;
}

void BootSplash::ClearFrames()
{
    for (auto& f : frames_) {
        if (f.bitmap)
            DeleteObject(f.bitmap);
    }
    frames_.clear();
}

bool BootSplash::LoadFrames()
{
    ClearFrames();
    const std::wstring folder =
        AssetPath((std::string(SIX_SEVEN_SPRITES_ROOT) + "/" + MOD_SPRITE_LOADING).c_str());
    WIN32_FIND_DATAW fd = {};
    const std::wstring pattern = PathJoin(folder, L"*.png");
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    std::vector<std::wstring> files;
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                files.push_back(PathJoin(folder, fd.cFileName));
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    std::sort(files.begin(), files.end(), FramePathLess);

    if (files.empty()) {
        Frame ph = {};
        ph.bitmap = MakePlaceholderFrame(screenW_ > 0 ? screenW_ : 1920, screenH_ > 0 ? screenH_ : 1080);
        ph.width = screenW_ > 0 ? screenW_ : 1920;
        ph.height = screenH_ > 0 ? screenH_ : 1080;
        if (ph.bitmap)
            frames_.push_back(ph);
        return !frames_.empty();
    }

    for (const auto& file : files) {
        Gdiplus::Bitmap bmp(file.c_str());
        if (bmp.GetLastStatus() != Gdiplus::Ok)
            continue;
        Frame fr = {};
        fr.bitmap = CreateBitmapFromGdiplus(&bmp, fr.width, fr.height);
        if (fr.bitmap)
            frames_.push_back(fr);
    }

    if (frames_.empty()) {
        Frame ph = {};
        ph.bitmap = MakePlaceholderFrame(screenW_ > 0 ? screenW_ : 1920, screenH_ > 0 ? screenH_ : 1080);
        ph.width = screenW_ > 0 ? screenW_ : 1920;
        ph.height = screenH_ > 0 ? screenH_ : 1080;
        frames_.push_back(ph);
    }
    return !frames_.empty();
}

void BootSplash::Destroy()
{
    active_ = false;
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    if (dibDc_) {
        DeleteDC(dibDc_);
        dibDc_ = nullptr;
    }
    if (dib_) {
        DeleteObject(dib_);
        dib_ = nullptr;
    }
    dibBits_ = nullptr;
    ClearFrames();
}

bool BootSplash::IsShellReady() const
{
    HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!tray || !IsWindowVisible(tray))
        return false;
    return GetShellWindow() != nullptr;
}

bool BootSplash::ShouldEnd() const
{
    const DWORD now = GetTickCount();
    const DWORD elapsed = now - startedMs_;
    const DWORD minMs = static_cast<DWORD>(BootIniMinSec()) * 1000u;
    const DWORD maxMs = static_cast<DWORD>(SIX_SEVEN_BOOT_SPLASH_MAX_SEC) * 1000u;

    if (elapsed >= maxMs)
        return true;
    if (elapsed < minMs)
        return false;
    if (loopCount_ < SIX_SEVEN_BOOT_SPLASH_MIN_LOOPS)
        return false;
    return IsShellReady();
}

void BootSplash::TickFrame()
{
    if (frames_.empty())
        return;

    const DWORD now = GetTickCount();
    if (lastFrameMs_ == 0)
        lastFrameMs_ = now;
    const int msPerFrame = std::max(1, 1000 / std::max(1, fps_));
    if (now - lastFrameMs_ < static_cast<DWORD>(msPerFrame))
        return;
    lastFrameMs_ = now;

    const int prev = frameIndex_;
    frameIndex_ = (frameIndex_ + 1) % static_cast<int>(frames_.size());
    if (frameIndex_ == 0 && prev == static_cast<int>(frames_.size()) - 1)
        ++loopCount_;
}

void BootSplash::Paint()
{
    if (!hwnd_ || !dibDc_ || !dibBits_ || frames_.empty())
        return;

    auto* px = static_cast<BYTE*>(dibBits_);
    std::memset(px, 0, static_cast<size_t>(screenW_) * screenH_ * 4);

    HDC mem = dibDc_;
    RECT full = { 0, 0, screenW_, screenH_ };
    HBRUSH bg = CreateSolidBrush(SIX_SEVEN_BOOT_SPLASH_BG_RGB);
    FillRect(mem, &full, bg);
    DeleteObject(bg);

    const Frame& fr = frames_[static_cast<size_t>(frameIndex_)];
    if (fr.bitmap && fr.width > 0 && fr.height > 0) {
#if SIX_SEVEN_BOOT_SPLASH_FIT_STRETCH
        BlitScaled(mem, fr.bitmap, fr.width, fr.height, 0, 0, screenW_, screenH_);
#else
        const float scale = std::min(screenW_ / static_cast<float>(fr.width),
                                     screenH_ / static_cast<float>(fr.height));
        const int dw = std::max(1, static_cast<int>(fr.width * scale + 0.5f));
        const int dh = std::max(1, static_cast<int>(fr.height * scale + 0.5f));
        const int dx = (screenW_ - dw) / 2;
        const int dy = (screenH_ - dh) / 2;
        BlitScaled(mem, fr.bitmap, fr.width, fr.height, dx, dy, dw, dh);
#endif
    }

    for (int y = 0; y < screenH_; ++y) {
        for (int x = 0; x < screenW_; ++x) {
            px[(static_cast<size_t>(y) * screenW_ + x) * 4 + 3] = 255;
        }
    }

    POINT ptSrc = { 0, 0 };
    SIZE size = { screenW_, screenH_ };
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    POINT ptDst = { screenX_, screenY_ };
    HDC screen = GetDC(nullptr);
    UpdateLayeredWindow(hwnd_, screen, &ptDst, &size, dibDc_, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(nullptr, screen);
}

void BootSplash::PumpMessages()
{
    MSG msg = {};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT)
            break;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

bool BootSplash::Run(HINSTANCE inst)
{
    wasShown_ = false;
    if (!ShouldShow())
        return false;

    Gdiplus::GdiplusStartupInput input;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &input, nullptr) != Gdiplus::Ok)
        return false;

    bool shown = false;
    if (Create(inst)) {
        shown = true;
        while (active_) {
            PumpMessages();
            TickFrame();
            Paint();
            if (ShouldEnd())
                break;
            Sleep(1);
        }
        Destroy();
    }

    if (token)
        Gdiplus::GdiplusShutdown(token);
    wasShown_ = shown;
    return shown;
}

} /* namespace six_seven */
