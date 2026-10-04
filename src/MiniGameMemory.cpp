#include "../include/MiniGameMemory.h"
#include "../include/Application.h"
#include "../include/MiniGameManager.h"
#include "../include/Util.h"
#include "../config.h"

#include <windowsx.h>
#include <gdiplus.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace six_seven {

namespace {

enum MemPreStartCmd { kMemPreStartHardCheck = 4201 };

constexpr int kSlotDrawW = MINIGAME_MEMORY_SLOT_DRAW_SIZE;
constexpr int kSlotDrawH = MINIGAME_MEMORY_SLOT_DRAW_SIZE;
constexpr int kSlotGap = MINIGAME_MEMORY_SLOT_GAP;
constexpr int kPanelPad = MINIGAME_MEMORY_PANEL_PAD;
constexpr int kOverlayTopPad = 12;

struct OverlayData {
    MemoryShellGame* game = nullptr;
};

void RegisterClassOnce(HINSTANCE inst, const wchar_t* name, WNDPROC proc, bool& flag)
{
    if (flag)
        return;
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_HAND);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = name;
    RegisterClassExW(&wc);
    flag = true;
}

COLORREF MemColorkey() { return static_cast<COLORREF>(SIX_SEVEN_COLORKEY); }

bool MemIsColorkey(BYTE r, BYTE g, BYTE b)
{
    const COLORREF key = MemColorkey();
    if (RGB(r, g, b) == key)
        return true;
    const int kr = GetRValue(key);
    const int kg = GetGValue(key);
    const int kb = GetBValue(key);
    const int tol = SIX_SEVEN_COLORKEY_TOLERANCE;
    return std::abs(static_cast<int>(r) - kr) <= tol &&
           std::abs(static_cast<int>(g) - kg) <= tol &&
           std::abs(static_cast<int>(b) - kb) <= tol;
}

HBITMAP MemLoadPng(const std::wstring& path, int& outW, int& outH, const wchar_t* placeholder)
{
    outW = kSlotDrawW;
    outH = kSlotDrawH;
    Gdiplus::Bitmap bmp(path.c_str());
    Gdiplus::Bitmap* use = &bmp;
    std::unique_ptr<Gdiplus::Bitmap> scaled;
    if (bmp.GetLastStatus() != Gdiplus::Ok) {
        HDC hdc = GetDC(nullptr);
        HDC mem = CreateCompatibleDC(hdc);
        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = outW;
        bi.bmiHeader.biHeight = -outH;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biPlanes = 1;
        void* bits = nullptr;
        HBITMAP hbmp = CreateDIBSection(mem, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        HGDIOBJ old = SelectObject(mem, hbmp);
        const COLORREF key = MemColorkey();
        HBRUSH keyBr = CreateSolidBrush(key);
        RECT rc = { 0, 0, outW, outH };
        FillRect(mem, &rc, keyBr);
        HBRUSH body = CreateSolidBrush(RGB(100, 50, 170));
        RECT bodyRc = { 10, 14, outW - 10, outH - 8 };
        FillRect(mem, &bodyRc, body);
        SetBkMode(mem, TRANSPARENT);
        SetTextColor(mem, RGB(255, 255, 255));
        DrawTextW(mem, placeholder, -1, &bodyRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(mem, old);
        DeleteObject(keyBr);
        DeleteObject(body);
        if (bits) {
            const int stride = outW * 4;
            for (int y = 0; y < outH; ++y) {
                for (int x = 0; x < outW; ++x) {
                    BYTE* p = static_cast<BYTE*>(bits) + y * stride + x * 4;
                    if (MemIsColorkey(p[2], p[1], p[0])) {
                        p[0] = p[1] = p[2] = 0;
                        p[3] = 0;
                    } else {
                        p[3] = 255;
                    }
                }
            }
        }
        DeleteDC(mem);
        ReleaseDC(nullptr, hdc);
        return hbmp;
    }

    scaled = std::make_unique<Gdiplus::Bitmap>(outW, outH, PixelFormat32bppARGB);
    if (scaled->GetLastStatus() == Gdiplus::Ok) {
        Gdiplus::Color keyCol(GetRValue(MemColorkey()), GetGValue(MemColorkey()),
                              GetBValue(MemColorkey()));
        Gdiplus::SolidBrush keyBrush(keyCol);
        Gdiplus::Graphics g(scaled.get());
        g.FillRectangle(&keyBrush, 0, 0, outW, outH);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        const int sw = static_cast<int>(bmp.GetWidth());
        const int sh = static_cast<int>(bmp.GetHeight());
        const double scale = std::min(static_cast<double>(outW) / sw, static_cast<double>(outH) / sh);
        const int dw = std::max(1, static_cast<int>(sw * scale + 0.5));
        const int dh = std::max(1, static_cast<int>(sh * scale + 0.5));
        const int dx = (outW - dw) / 2;
        const int dy = (outH - dh) / 2;
        g.DrawImage(&bmp, dx, dy, dw, dh);
        use = scaled.get();
    }

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = outW;
    bi.bmiHeader.biHeight = -outH;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    void* bits = nullptr;
    HDC hdc = GetDC(nullptr);
    HBITMAP hbmp = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, hdc);
    if (!hbmp || !bits)
        return nullptr;

    Gdiplus::BitmapData data = {};
    Gdiplus::Rect rc(0, 0, outW, outH);
    if (use->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) == Gdiplus::Ok) {
        auto* dst = static_cast<BYTE*>(bits);
        for (int y = 0; y < outH; ++y) {
            for (int x = 0; x < outW; ++x) {
                const BYTE* p = static_cast<const BYTE*>(data.Scan0) + y * data.Stride + x * 4;
                BYTE* d = dst + (y * outW + x) * 4;
                const BYTE b = p[0], gch = p[1], r = p[2], a = p[3];
                if (a < 8 || MemIsColorkey(r, gch, b)) {
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
        use->UnlockBits(&data);
    }
    return hbmp;
}

void BlitScaledBrightness(HDC dest, const MemoryShellGame::MemBitmap& bmp, int x, int y,
                          float brightness, float scale, bool centerInSlot = true)
{
    if (!bmp.hbmp || bmp.w <= 0 || bmp.h <= 0)
        return;
    const int dw = std::max(1, static_cast<int>(bmp.w * scale + 0.5f));
    const int dh = std::max(1, static_cast<int>(bmp.h * scale + 0.5f));
    int dx = x;
    int dy = y;
    if (centerInSlot) {
        dx = x + (kSlotDrawW - dw) / 2;
        dy = y + (kSlotDrawH - dh) / 2;
    }

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = bmp.w;
    bi.bmiHeader.biHeight = -bmp.h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    std::vector<BYTE> px(static_cast<size_t>(bmp.w) * bmp.h * 4);
    HDC srcDc = CreateCompatibleDC(dest);
    HGDIOBJ old = SelectObject(srcDc, bmp.hbmp);
    GetDIBits(srcDc, bmp.hbmp, 0, bmp.h, px.data(), &bi, DIB_RGB_COLORS);
    SelectObject(srcDc, old);

    const float br = std::max(0.f, std::min(1.f, brightness));
    for (int i = 0; i < bmp.w * bmp.h; ++i) {
        BYTE* p = px.data() + i * 4;
        if (p[3] == 0)
            continue;
        p[0] = static_cast<BYTE>(std::min(255, static_cast<int>(p[0] * br)));
        p[1] = static_cast<BYTE>(std::min(255, static_cast<int>(p[1] * br)));
        p[2] = static_cast<BYTE>(std::min(255, static_cast<int>(p[2] * br)));
    }

    BITMAPINFO dib = bi;
    void* tmpBits = nullptr;
    HBITMAP tmp = CreateDIBSection(dest, &dib, DIB_RGB_COLORS, &tmpBits, nullptr, 0);
    if (tmp && tmpBits) {
        std::memcpy(tmpBits, px.data(), px.size());
        HDC mem = CreateCompatibleDC(dest);
        HGDIOBJ oldTmp = SelectObject(mem, tmp);
        BLENDFUNCTION blend = {};
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat = AC_SRC_ALPHA;
        GdiAlphaBlend(dest, dx, dy, dw, dh, mem, 0, 0, bmp.w, bmp.h, blend);
        SelectObject(mem, oldTmp);
        DeleteDC(mem);
        DeleteObject(tmp);
    }
    DeleteDC(srcDc);
}

} /* namespace */

void MemoryShellGame::Bind(Application* app, MiniGameManager* mgr, MiniGameRecords* records)
{
    app_ = app;
    mgr_ = mgr;
    records_ = records;
}

int MemoryShellGame::SlotCount() const
{
    return hardMode_ ? MINIGAME_MEMORY_HARD_SLOT_COUNT : MINIGAME_MEMORY_SLOT_COUNT;
}

int MemoryShellGame::OverlayWidth() const
{
    const int n = SlotCount();
    return kPanelPad * 2 + n * kSlotDrawW + (n - 1) * kSlotGap;
}

int MemoryShellGame::OverlayHeight() const
{
    return kOverlayTopPad + kSlotDrawH + kPanelPad;
}

int MemoryShellGame::SlotX(int slot) const
{
    return kPanelPad + slot * (kSlotDrawW + kSlotGap);
}

int MemoryShellGame::RecordScore(bool hardMode) const
{
    if (!records_)
        return 0;
    return records_->Get(MINIGAME_MEMORY_ID, hardMode);
}

bool MemoryShellGame::LoadMemBitmap(const char* spriteRel, MemBitmap& out, const wchar_t* placeholder)
{
    if (out.hbmp) {
        DeleteObject(out.hbmp);
        out = {};
    }
    std::string rel = std::string(SIX_SEVEN_SPRITES_ROOT) + "/" + spriteRel;
    const std::wstring folder = AssetPath(rel.c_str());
    WIN32_FIND_DATAW fd = {};
    const std::wstring pattern = PathJoin(folder, L"*.png");
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    std::wstring file;
    if (h != INVALID_HANDLE_VALUE) {
        file = PathJoin(folder, fd.cFileName);
        FindClose(h);
    }
    if (file.empty())
        file = folder + L"\\01.png";
    out.hbmp = MemLoadPng(file, out.w, out.h, placeholder);
    return out.hbmp != nullptr;
}

void MemoryShellGame::LoadAssets()
{
    if (!app_)
        return;
    app_->Sprites().Init();
    LoadMemBitmap(MOD_SPRITE_MINIGAME_MEMORY_REAL, bmpReal_, L"OK");
    LoadMemBitmap(MOD_SPRITE_MINIGAME_MEMORY_FAKE1, bmpFake1_, L"F1");
    LoadMemBitmap(MOD_SPRITE_MINIGAME_MEMORY_FAKE2, bmpFake2_, L"F2");
    LoadMemBitmap(MOD_SPRITE_MINIGAME_MEMORY_FAKE3, bmpFake3_, L"F3");
    LoadMemBitmap(MOD_SPRITE_MINIGAME_MEMORY_REVEAL, bmpReveal_, L":)");
    LoadMemBitmap(MOD_SPRITE_MINIGAME_MEMORY_SCARE, bmpScare_, L"BOO!");
}

void MemoryShellGame::FreeAssets()
{
    auto freeBmp = [](MemBitmap& b) {
        if (b.hbmp)
            DeleteObject(b.hbmp);
        b = {};
    };
    freeBmp(bmpReal_);
    freeBmp(bmpFake1_);
    freeBmp(bmpFake2_);
    freeBmp(bmpFake3_);
    freeBmp(bmpReveal_);
    freeBmp(bmpScare_);
}

const MemoryShellGame::MemBitmap& MemoryShellGame::BitmapFor(CupKind kind, bool useReveal) const
{
    if (useReveal && kind == CupKind::Real)
        return bmpReveal_.hbmp ? bmpReveal_ : bmpReal_;
    switch (kind) {
    case CupKind::Real:
        return bmpReal_;
    case CupKind::Fake1:
        return bmpFake1_.hbmp ? bmpFake1_ : bmpFake2_;
    case CupKind::Fake2:
        return bmpFake2_.hbmp ? bmpFake2_ : bmpFake1_;
    case CupKind::Fake3:
        return bmpFake3_.hbmp ? bmpFake3_ : bmpFake1_;
    }
    return bmpFake1_;
}

void MemoryShellGame::SetupRound()
{
    const int n = SlotCount();
    std::vector<CupKind> kinds;
    kinds.push_back(CupKind::Real);
    for (int i = 1; i < n; ++i)
        kinds.push_back(static_cast<CupKind>(i));
    for (int i = n - 1; i > 0; --i) {
        const int j = RandomInt(0, i);
        std::swap(kinds[static_cast<size_t>(i)], kinds[static_cast<size_t>(j)]);
    }
    for (int i = 0; i < n; ++i)
        slotContents_[i] = static_cast<int>(kinds[static_cast<size_t>(i)]);
    for (int i = n; i < 4; ++i)
        slotContents_[i] = static_cast<int>(CupKind::Fake1);
}

DWORD MemoryShellGame::ShuffleDurationMs() const
{
    float ms = static_cast<float>(MINIGAME_MEMORY_SHUFFLE_BASE_MS +
                                  (round_ - 1) * MINIGAME_MEMORY_SHUFFLE_ADD_MS);
    if (hardMode_)
        ms *= MINIGAME_MEMORY_HARD_SHUFFLE_MULT;
    return static_cast<DWORD>(ms + 0.5f);
}

DWORD MemoryShellGame::SwapIntervalMs() const
{
    int ms = MINIGAME_MEMORY_SWAP_START_MS - (round_ - 1) * MINIGAME_MEMORY_SWAP_STEP_MS;
    ms = std::max(MINIGAME_MEMORY_SWAP_MIN_MS, ms);
    if (hardMode_)
        ms = std::max(MINIGAME_MEMORY_SWAP_MIN_MS,
                      static_cast<int>(ms * MINIGAME_MEMORY_HARD_SWAP_MULT + 0.5f));
    return static_cast<DWORD>(ms);
}

void MemoryShellGame::DoSwap(int a, int b)
{
    if (a == b)
        return;
    std::swap(slotContents_[a], slotContents_[b]);
}

void MemoryShellGame::StartSwapAnim(int a, int b, DWORD now)
{
    (void)now;
    if (a == b)
        return;
    swap_.active = true;
    swap_.slotA = a;
    swap_.slotB = b;
    swap_.t = 0.f;
}

void MemoryShellGame::TickSwapAnim(DWORD now)
{
    (void)now;
    if (!swap_.active)
        return;
    swap_.t += 16.f / static_cast<float>(MINIGAME_MEMORY_SWAP_ANIM_MS);
    if (swap_.t >= 1.f) {
        swap_.t = 1.f;
        DoSwap(swap_.slotA, swap_.slotB);
        swap_.active = false;
    }
}

void MemoryShellGame::BeginPhase(Phase p, DWORD now)
{
    phase_ = p;
    phaseStartMs_ = now;
    switch (p) {
    case Phase::Preview:
        brightness_ = 1.f;
        SetupRound();
        if (overlayHwnd_)
            ShowWindow(overlayHwnd_, SW_SHOWNA);
        break;
    case Phase::Darken:
        brightness_ = 1.f;
        break;
    case Phase::Shuffle:
        shuffleEndMs_ = now + ShuffleDurationMs();
        nextSwapMs_ = now + SwapIntervalMs();
        swap_.active = false;
        break;
    case Phase::Pick:
        brightness_ = MINIGAME_MEMORY_DARK_BRIGHTNESS;
        break;
    case Phase::Success:
        brightness_ = 1.f;
        break;
    case Phase::Scare:
        scarePopT_ = 0.f;
        if (overlayHwnd_)
            ShowWindow(overlayHwnd_, SW_HIDE);
        CreateScareOverlay();
        break;
    }
    UpdateHud();
    if (phase_ == Phase::Scare)
        PaintScareOverlay();
    else
        PaintOverlay();
}

void MemoryShellGame::DrawCup(HDC hdc, int slot, CupKind kind, int x, int y, float brightness,
                              float scale, bool useReveal) const
{
    (void)slot;
    const MemBitmap& bmp = BitmapFor(kind, useReveal);
    BlitScaledBrightness(hdc, bmp, x, y, brightness, scale);
}

void MemoryShellGame::PaintOverlay()
{
    if (!overlayHwnd_ || !overlayDibDc_ || !overlayDibBits_)
        return;

    const int w = OverlayWidth();
    const int h = OverlayHeight();
    auto* px = static_cast<BYTE*>(overlayDibBits_);
    std::memset(px, 0, static_cast<size_t>(w) * h * 4);

    HDC mem = overlayDibDc_;
    RECT rc = { 0, 0, w, h };
    HBRUSH bg = CreateSolidBrush(RGB(30, 30, 40));
    FillRect(mem, &rc, bg);
    DeleteObject(bg);

    const int n = SlotCount();
    const int baseY = kOverlayTopPad;

    auto drawAtSlot = [&](int slot, CupKind kind, int x, float br, float scale, bool reveal) {
        DrawCup(mem, slot, kind, x, baseY, br, scale, reveal);
    };

    for (int s = 0; s < n; ++s) {
            const CupKind kind = static_cast<CupKind>(slotContents_[s]);
            const bool reveal = phase_ == Phase::Success;
            float br = brightness_;
            if (phase_ == Phase::Preview || phase_ == Phase::Success)
                br = 1.f;
            int x = SlotX(s);
            if (swap_.active && (s == swap_.slotA || s == swap_.slotB)) {
                const int xA = SlotX(swap_.slotA);
                const int xB = SlotX(swap_.slotB);
                const float t = swap_.t;
                if (s == swap_.slotA)
                    x = xA + static_cast<int>((xB - xA) * t + 0.5f);
                else
                    x = xB + static_cast<int>((xA - xB) * t + 0.5f);
            }
            drawAtSlot(s, kind, x, br, 1.f, reveal);
    }

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            px[(static_cast<size_t>(y) * w + x) * 4 + 3] = 255;
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
    GetWindowRect(overlayHwnd_, &wr);
    ptDst.x = wr.left;
    ptDst.y = wr.top;
    HDC screen = GetDC(nullptr);
    UpdateLayeredWindow(overlayHwnd_, screen, &ptDst, &size, overlayDibDc_, &ptSrc, 0, &blend,
                        ULW_ALPHA);
    ReleaseDC(nullptr, screen);
}

void MemoryShellGame::PaintScareOverlay()
{
    if (!scareHwnd_ || !scareDibDc_ || !scareDibBits_ || !bmpScare_.hbmp)
        return;

    auto* px = static_cast<BYTE*>(scareDibBits_);
    std::memset(px, 0, static_cast<size_t>(scareScreenW_) * scareScreenH_ * 4);

    HDC mem = scareDibDc_;
    RECT full = { 0, 0, scareScreenW_, scareScreenH_ };
    HBRUSH bg = CreateSolidBrush(RGB(12, 0, 20));
    FillRect(mem, &full, bg);
    DeleteObject(bg);

    const float eased = scarePopT_ * scarePopT_ * scarePopT_ * scarePopT_;
    const float scale = 0.12f + (scareTargetScale_ - 0.12f) * eased;
    const int dw = std::max(1, static_cast<int>(bmpScare_.w * scale + 0.5f));
    const int dh = std::max(1, static_cast<int>(bmpScare_.h * scale + 0.5f));
    const int dx = (scareScreenW_ - dw) / 2;
    const int dy = (scareScreenH_ - dh) / 2;
    BlitScaledBrightness(mem, bmpScare_, dx, dy, 1.f, scale, false);

    for (int y = 0; y < scareScreenH_; ++y) {
        for (int x = 0; x < scareScreenW_; ++x) {
            px[(static_cast<size_t>(y) * scareScreenW_ + x) * 4 + 3] = 255;
        }
    }

    POINT ptSrc = { 0, 0 };
    SIZE size = { scareScreenW_, scareScreenH_ };
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    POINT ptDst = {};
    RECT wr = {};
    GetWindowRect(scareHwnd_, &wr);
    ptDst.x = wr.left;
    ptDst.y = wr.top;
    HDC screen = GetDC(nullptr);
    UpdateLayeredWindow(scareHwnd_, screen, &ptDst, &size, scareDibDc_, &ptSrc, 0, &blend,
                        ULW_ALPHA);
    ReleaseDC(nullptr, screen);
}

void MemoryShellGame::CreateScareOverlay()
{
    if (!app_ || scareHwnd_)
        return;
    RegisterClassOnce(app_->Inst(), L"SixSevenMemoryScare", ScareWndProc, scareClassRegistered_);

    const RECT wa = GetCombinedWorkArea();
    scareScreenW_ = wa.right - wa.left;
    scareScreenH_ = wa.bottom - wa.top;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = scareScreenW_;
    bi.bmiHeader.biHeight = -scareScreenH_;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    HDC screen = GetDC(nullptr);
    scareDib_ = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &scareDibBits_, nullptr, 0);
    scareDibDc_ = CreateCompatibleDC(screen);
    SelectObject(scareDibDc_, scareDib_);
    ReleaseDC(nullptr, screen);

    const int sw = std::max(1, bmpScare_.w);
    const int sh = std::max(1, bmpScare_.h);
    scareTargetScale_ = std::max(scareScreenW_ / static_cast<float>(sw),
                                 scareScreenH_ / static_cast<float>(sh)) *
                        MINIGAME_MEMORY_SCARE_OVERSCAN;

    scareHwnd_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"SixSevenMemoryScare", L"", WS_POPUP,
        wa.left, wa.top, scareScreenW_, scareScreenH_, nullptr, nullptr, app_->Inst(), this);
    if (scareHwnd_) {
        PaintScareOverlay();
        ShowWindow(scareHwnd_, SW_SHOWNA);
    }
}

void MemoryShellGame::DestroyScareOverlay()
{
    if (scareHwnd_) {
        DestroyWindow(scareHwnd_);
        scareHwnd_ = nullptr;
    }
    if (scareDibDc_) {
        DeleteDC(scareDibDc_);
        scareDibDc_ = nullptr;
    }
    if (scareDib_) {
        DeleteObject(scareDib_);
        scareDib_ = nullptr;
    }
    scareDibBits_ = nullptr;
    scareScreenW_ = 0;
    scareScreenH_ = 0;
}

LRESULT CALLBACK MemoryShellGame::ScareWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    (void)hwnd;
    (void)wp;
    (void)lp;
    if (msg == WM_NCHITTEST)
        return HTTRANSPARENT;
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void MemoryShellGame::UpdateHud()
{
    if (!hudHwnd_ || !hudDibDc_ || !hudDibBits_)
        return;

    const int w = 340;
    const int h = 56;
    auto* px = static_cast<BYTE*>(hudDibBits_);
    std::memset(px, 0, static_cast<size_t>(w) * h * 4);
    HDC mem = hudDibDc_;
    RECT rc = { 0, 0, w, h };
    HBRUSH bg = CreateSolidBrush(RGB(255, 252, 230));
    FillRect(mem, &rc, bg);
    DeleteObject(bg);
    FrameRect(mem, &rc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    SetBkMode(mem, TRANSPARENT);

    wchar_t line1[64];
    wchar_t line2[96];
    swprintf(line1, 64, L"Раунд: %d   Очки: %d", round_, score_);

    switch (phase_) {
    case Phase::Preview:
        wcscpy_s(line2, L"Запоминай отличия...");
        break;
    case Phase::Darken:
        wcscpy_s(line2, L"Темнеет...");
        break;
    case Phase::Shuffle: {
        const DWORD now = GetTickCount();
        const DWORD left = shuffleEndMs_ > now ? (shuffleEndMs_ - now + 999) / 1000 : 0;
        swprintf(line2, 96, L"Перемешивание... %u сек", left);
        break;
    }
    case Phase::Pick:
        wcscpy_s(line2, L"Где настоящий? Нажми!");
        break;
    case Phase::Success:
        wcscpy_s(line2, L"Верно!");
        break;
    case Phase::Scare:
        wcscpy_s(line2, L"...");
        break;
    }

    HFONT font = CreateFontW(-17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HGDIOBJ old = SelectObject(mem, font);
    SetTextColor(mem, RGB(40, 40, 40));
    RECT r1 = { 8, 4, w - 8, 24 };
    DrawTextW(mem, line1, -1, &r1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    RECT r2 = { 8, 26, w - 8, h - 6 };
    SetTextColor(mem, RGB(0, 70, 140));
    DrawTextW(mem, line2, -1, &r2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(mem, old);
    DeleteObject(font);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            px[(static_cast<size_t>(y) * w + x) * 4 + 3] = 255;
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

LRESULT CALLBACK MemoryShellGame::MemHudWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    (void)hwnd;
    (void)wp;
    (void)lp;
    if (msg == WM_NCHITTEST)
        return HTTRANSPARENT;
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK MemoryShellGame::OverlayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* data = reinterpret_cast<OverlayData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        data = new OverlayData();
        data->game = static_cast<MemoryShellGame*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        return 0;
    }
    case WM_LBUTTONDOWN:
        if (data && data->game && data->game->phase_ == Phase::Pick) {
            const int x = GET_X_LPARAM(lp);
            const int y = GET_Y_LPARAM(lp);
            const int slot = data->game->HitSlot(x, y);
            if (slot >= 0)
                data->game->OnSlotClick(slot);
            return 0;
        }
        break;
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

int MemoryShellGame::HitSlot(int clientX, int clientY) const
{
    if (clientY < kOverlayTopPad || clientY > kOverlayTopPad + kSlotDrawH)
        return -1;
    const int n = SlotCount();
    for (int s = 0; s < n; ++s) {
        const int x0 = SlotX(s);
        if (clientX >= x0 && clientX < x0 + kSlotDrawW)
            return s;
    }
    return -1;
}

void MemoryShellGame::OnSlotClick(int slot)
{
    if (!active_ || phase_ != Phase::Pick || swap_.active)
        return;
    const CupKind picked = static_cast<CupKind>(slotContents_[slot]);
    if (picked == CupKind::Real) {
        score_ = round_;
        if (app_)
            app_->Audio().PlayFile(MOD_SOUND_MINIGAME_MEMORY_OK);
        BeginPhase(Phase::Success, GetTickCount());
    } else {
        scareSlot_ = slot;
        if (app_)
            app_->Audio().PlayFile(MOD_SOUND_MINIGAME_MEMORY_FAIL);
        BeginPhase(Phase::Scare, GetTickCount());
    }
}

void MemoryShellGame::CreateOverlay()
{
    if (!app_ || overlayHwnd_)
        return;
    RegisterClassOnce(app_->Inst(), L"SixSevenMemoryOverlay", OverlayWndProc,
                      overlayClassRegistered_);

    const int ow = OverlayWidth();
    const int oh = OverlayHeight();
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = ow;
    bi.bmiHeader.biHeight = -oh;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    HDC screen = GetDC(nullptr);
    overlayDib_ = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &overlayDibBits_, nullptr, 0);
    overlayDibDc_ = CreateCompatibleDC(screen);

    const int hw = 340;
    const int hh = 56;
    BITMAPINFO hbi = bi;
    hbi.bmiHeader.biWidth = hw;
    hbi.bmiHeader.biHeight = -hh;
    hudDib_ = CreateDIBSection(screen, &hbi, DIB_RGB_COLORS, &hudDibBits_, nullptr, 0);
    hudDibDc_ = CreateCompatibleDC(screen);
    SelectObject(overlayDibDc_, overlayDib_);
    SelectObject(hudDibDc_, hudDib_);
    ReleaseDC(nullptr, screen);

    const RECT wa = GetCombinedWorkArea();
    const int ox = wa.left + (wa.right - wa.left - ow) / 2;
    const int oy = wa.top + (wa.bottom - wa.top - oh) / 2 + 40;
    overlayHwnd_ =
        CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"SixSevenMemoryOverlay",
                        L"", WS_POPUP, ox, oy, ow, oh, nullptr, nullptr, app_->Inst(), this);

    RegisterClassOnce(app_->Inst(), L"SixSevenMemoryHud", MemHudWndProc, hudClassRegistered_);
    const int hx = wa.left + (wa.right - wa.left - hw) / 2;
    const int hy = wa.top + 12;
    hudHwnd_ = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
                                 L"SixSevenMemoryHud", L"", WS_POPUP, hx, hy, hw, hh, nullptr,
                                 nullptr, app_->Inst(), nullptr);

    if (overlayHwnd_) {
        PaintOverlay();
        ShowWindow(overlayHwnd_, SW_SHOWNA);
    }
    if (hudHwnd_) {
        UpdateHud();
        ShowWindow(hudHwnd_, SW_SHOWNA);
    }
}

void MemoryShellGame::DestroyOverlay()
{
    DestroyScareOverlay();
    if (overlayHwnd_) {
        DestroyWindow(overlayHwnd_);
        overlayHwnd_ = nullptr;
    }
    if (hudHwnd_) {
        DestroyWindow(hudHwnd_);
        hudHwnd_ = nullptr;
    }
    if (overlayDibDc_) {
        DeleteDC(overlayDibDc_);
        overlayDibDc_ = nullptr;
    }
    if (hudDibDc_) {
        DeleteDC(hudDibDc_);
        hudDibDc_ = nullptr;
    }
    if (overlayDib_) {
        DeleteObject(overlayDib_);
        overlayDib_ = nullptr;
    }
    if (hudDib_) {
        DeleteObject(hudDib_);
        hudDib_ = nullptr;
    }
    overlayDibBits_ = nullptr;
    hudDibBits_ = nullptr;
}

LRESULT CALLBACK MemoryShellGame::MemPreStartWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* game = reinterpret_cast<MemoryShellGame*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        game = static_cast<MemoryShellGame*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(game));
        CreateWindowExW(0, L"STATIC", MINIGAME_MEMORY_PRESTART_TEXT, WS_CHILD | WS_VISIBLE, 16, 12,
                        420, 88, hwnd, nullptr, cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", MINIGAME_MEMORY_HARD_CHECK_LABEL,
                        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 16, 108, 400, 22, hwnd,
                        reinterpret_cast<HMENU>(kMemPreStartHardCheck), cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Старт", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 90, 142,
                        90, 28, hwnd, reinterpret_cast<HMENU>(IDOK), cs->hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Отмена", WS_CHILD | WS_VISIBLE, 220, 142, 90, 28, hwnd,
                        reinterpret_cast<HMENU>(IDCANCEL), cs->hInstance, nullptr);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK && game) {
            HWND check = GetDlgItem(hwnd, kMemPreStartHardCheck);
            game->preStartHard_ =
                check && SendMessageW(check, BM_GETCHECK, 0, 0) == BST_CHECKED;
            game->preStartStarted_ = true;
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

bool MemoryShellGame::ShowPreStartDialog(bool* hardModeOut)
{
    if (!app_ || !hardModeOut)
        return false;
    RegisterClassOnce(app_->Inst(), L"SixSevenMemPreStart", MemPreStartWndProc,
                      preStartClassRegistered_);
    preStartStarted_ = false;
    preStartHard_ = false;
    HWND dlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"SixSevenMemPreStart", MINIGAME_MEMORY_PRESTART_TITLE,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 460, 220,
        app_->MainHwnd(), nullptr, app_->Inst(), this);
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

void MemoryShellGame::Start(bool hardMode)
{
    if (!app_ || active_)
        return;
    app_->Actions().Cancel();
    app_->Bubble().Clear();

    hardMode_ = hardMode;
    round_ = 1;
    score_ = 0;
    scareSlot_ = -1;
    swap_.active = false;

    RECT rc = {};
    GetWindowRect(app_->MainHwnd(), &rc);
    savedWinLeft_ = rc.left;
    savedWinTop_ = rc.top;
    savedWinPos_ = true;
    ShowWindow(app_->MainHwnd(), SW_HIDE);

    LoadAssets();
    CreateOverlay();

    if (hardMode_ && mgr_)
        mgr_->BeginHardGlitches(GetTickCount());

    active_ = true;
    BeginPhase(Phase::Preview, GetTickCount());
}

void MemoryShellGame::EndGame(bool win)
{
    if (!active_ || !app_)
        return;

    const int finalScore = score_;
    const bool hard = hardMode_;
    const int restoreX = savedWinLeft_;
    const int restoreY = savedWinTop_;
    const bool restorePos = savedWinPos_;
    const bool newRecord =
        records_ && finalScore > 0 && records_->TrySave(MINIGAME_MEMORY_ID, hard, finalScore);
    if (newRecord && app_)
        app_->AwardVaultFragment();

    Stop();

    wchar_t body[512];
    swprintf(body, 512, L"%s\r\n\r\nРаундов пройдено: %d", MINIGAME_MEMORY_END_LOSE_TEXT,
             finalScore);
    if (newRecord)
        wcscat_s(body, MINIGAME_MEMORY_NEW_RECORD_SUFFIX);

    MessageBoxW(app_->MainHwnd(), body, MINIGAME_MEMORY_END_LOSE_TITLE, MB_OK | MB_ICONWARNING);

    ShowWindow(app_->MainHwnd(), SW_SHOW);
    if (restorePos) {
        SetWindowPos(app_->MainHwnd(), HWND_TOPMOST, restoreX, restoreY, 0, 0,
                     SWP_NOSIZE | SWP_NOACTIVATE);
    }
    app_->ReturnToIdleSprite();
}

void MemoryShellGame::Stop()
{
    if (!active_ && !overlayHwnd_)
        return;
    active_ = false;
    if (mgr_)
        mgr_->StopHardGlitches();
    DestroyOverlay();
    FreeAssets();
    hardMode_ = false;
    if (mgr_)
        mgr_->OnMemoryGameStopped();
}

void MemoryShellGame::Tick()
{
    if (!active_ || !app_)
        return;

    const DWORD now = GetTickCount();

    if (hardMode_ && mgr_)
        mgr_->TickHardGlitches(now);

    if (swap_.active) {
        TickSwapAnim(now);
        PaintOverlay();
        UpdateHud();
        return;
    }

    switch (phase_) {
    case Phase::Preview:
        if (now - phaseStartMs_ >= static_cast<DWORD>(MINIGAME_MEMORY_PREVIEW_MS))
            BeginPhase(Phase::Darken, now);
        else
            PaintOverlay();
        break;
    case Phase::Darken: {
        const DWORD elapsed = now - phaseStartMs_;
        const float t =
            std::min(1.f, elapsed / static_cast<float>(MINIGAME_MEMORY_DARKEN_MS));
        brightness_ = 1.f - t * (1.f - MINIGAME_MEMORY_DARK_BRIGHTNESS);
        if (elapsed >= static_cast<DWORD>(MINIGAME_MEMORY_DARKEN_MS))
            BeginPhase(Phase::Shuffle, now);
        else
            PaintOverlay();
        break;
    }
    case Phase::Shuffle:
        if (now >= shuffleEndMs_) {
            BeginPhase(Phase::Pick, now);
            break;
        }
        if (now >= nextSwapMs_) {
            const int n = SlotCount();
            int a = RandomInt(0, n - 1);
            int b = RandomInt(0, n - 1);
            for (int tries = 0; tries < 8 && a == b; ++tries)
                b = RandomInt(0, n - 1);
            if (a != b)
                StartSwapAnim(a, b, now);
            nextSwapMs_ = now + SwapIntervalMs();
        }
        UpdateHud();
        PaintOverlay();
        break;
    case Phase::Pick:
        break;
    case Phase::Success:
        if (now - phaseStartMs_ >= static_cast<DWORD>(MINIGAME_MEMORY_REVEAL_MS)) {
            round_++;
            BeginPhase(Phase::Preview, now);
        } else {
            PaintOverlay();
        }
        break;
    case Phase::Scare: {
        const DWORD elapsed = now - phaseStartMs_;
        scarePopT_ =
            std::min(1.f, elapsed / static_cast<float>(MINIGAME_MEMORY_SCARE_POP_MS));
        PaintScareOverlay();
        if (elapsed >= static_cast<DWORD>(MINIGAME_MEMORY_SCARE_MS))
            EndGame(false);
        break;
    }
    }

    if (phase_ != Phase::Shuffle)
        UpdateHud();
}

} /* namespace six_seven */
