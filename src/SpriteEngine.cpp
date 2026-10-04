#include "../include/SpriteEngine.h"
#include "../include/ConfigData.h"
#include "../include/Util.h"
#include "../config.h"

#include <windows.h>
#include <gdiplus.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

namespace six_seven {

namespace {

ULONG_PTR g_gdiToken = 0;
int g_gdiRefCount = 0;

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
    if (std::abs(static_cast<int>(r) - kr) <= tol &&
        std::abs(static_cast<int>(g) - kg) <= tol &&
        std::abs(static_cast<int>(b) - kb) <= tol)
        return true;
    return r >= 140 && g <= 90 && b >= 140;
#endif
}

Gdiplus::Color KeyGdiColor()
{
    const COLORREF k = MagentaKey();
    return Gdiplus::Color(GetRValue(k), GetGValue(k), GetBValue(k));
}

bool GetPngEncoderClsid(CLSID* out)
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
        if (wcscmp(codecs[i].MimeType, L"image/png") == 0) {
            *out = codecs[i].Clsid;
            return true;
        }
    }
    return false;
}

bool SaveBitmapPng(Gdiplus::Bitmap* bmp, const std::wstring& path)
{
    CLSID clsid = {};
    if (!GetPngEncoderClsid(&clsid))
        return false;
    return bmp->Save(path.c_str(), &clsid, nullptr) == Gdiplus::Ok;
}

void FillBitmapColorkey(Gdiplus::Bitmap* bmp)
{
    Gdiplus::BitmapData data = {};
    Gdiplus::Rect rc(0, 0, static_cast<INT>(bmp->GetWidth()),
                     static_cast<INT>(bmp->GetHeight()));
    if (bmp->LockBits(&rc, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &data) !=
        Gdiplus::Ok)
        return;
    const COLORREF key = MagentaKey();
    const BYTE kr = static_cast<BYTE>(GetRValue(key));
    const BYTE kg = static_cast<BYTE>(GetGValue(key));
    const BYTE kb = static_cast<BYTE>(GetBValue(key));
    auto* scan = static_cast<BYTE*>(data.Scan0);
    for (UINT y = 0; y < data.Height; ++y) {
        BYTE* row = scan + static_cast<size_t>(y) * data.Stride;
        for (UINT x = 0; x < data.Width; ++x) {
            BYTE* p = row + x * 4;
            p[0] = kb;
            p[1] = kg;
            p[2] = kr;
            p[3] = 255;
        }
    }
    bmp->UnlockBits(&data);
}

/* Вписать/растянуть в SIX_SEVEN_SPRITE_WIDTH×HEIGHT; опционально сохранить PNG. */
std::unique_ptr<Gdiplus::Bitmap> NormalizeSpriteBitmap(Gdiplus::Bitmap* src,
                                                       const std::wstring& filePath)
{
#if !SIX_SEVEN_SPRITE_AUTO_RESIZE
    (void)src;
    (void)filePath;
    return nullptr;
#else
    const int targetW = SIX_SEVEN_SPRITE_WIDTH;
    const int targetH = SIX_SEVEN_SPRITE_HEIGHT;
    const int srcW = static_cast<int>(src->GetWidth());
    const int srcH = static_cast<int>(src->GetHeight());
    if (srcW <= 0 || srcH <= 0 || targetW <= 0 || targetH <= 0)
        return nullptr;

    if (srcW == targetW && srcH == targetH)
        return nullptr;

    auto out = std::make_unique<Gdiplus::Bitmap>(targetW, targetH, PixelFormat32bppARGB);
    if (out->GetLastStatus() != Gdiplus::Ok)
        return nullptr;

    FillBitmapColorkey(out.get());
    Gdiplus::Graphics g(out.get());
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

#if SIX_SEVEN_SPRITE_FIT_STRETCH
    g.DrawImage(src, 0, 0, targetW, targetH);
#else
    const double scale =
        std::min(static_cast<double>(targetW) / srcW, static_cast<double>(targetH) / srcH);
    const int drawW = std::max(1, static_cast<int>(srcW * scale + 0.5));
    const int drawH = std::max(1, static_cast<int>(srcH * scale + 0.5));
    const int drawX = (targetW - drawW) / 2;
    const int drawY = (targetH - drawH) / 2;
    g.DrawImage(src, drawX, drawY, drawW, drawH);
#endif

#if SIX_SEVEN_SPRITE_SAVE_RESIZED
    SaveBitmapPng(out.get(), filePath);
#endif
    return out;
#endif
}

HBITMAP CreateBitmapFromGdiplus(Gdiplus::Bitmap* bmp, int& w, int& h)
{
    w = static_cast<int>(bmp->GetWidth());
    h = static_cast<int>(bmp->GetHeight());
    if (w <= 0 || h <= 0)
        return nullptr;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC hdc = GetDC(nullptr);
    HBITMAP hbmp =
        CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, hdc);
    if (!hbmp || !bits)
        return nullptr;

    Gdiplus::Bitmap dst(w, h, PixelFormat32bppARGB);
    Gdiplus::Graphics g(&dst);
    g.DrawImage(bmp, 0, 0, w, h);

    Gdiplus::BitmapData data = {};
    if (dst.LockBits(nullptr, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB,
                     &data) == Gdiplus::Ok) {
        const auto* src = static_cast<const BYTE*>(data.Scan0);
        auto* dstBits = static_cast<BYTE*>(bits);
        const COLORREF key = MagentaKey();
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const BYTE* p = src + y * data.Stride + x * 4;
                BYTE* d = dstBits + (y * w + x) * 4;
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
    return hbmp;
}

void FillHueColorMatrix(int hueDeg, Gdiplus::ColorMatrix& cm)
{
    for (int r = 0; r < 5; ++r)
        for (int c = 0; c < 5; ++c)
            cm.m[r][c] = (r == c) ? 1.f : 0.f;

    if (hueDeg == 0)
        return;

    const float hue = static_cast<float>(((hueDeg % 360) + 360) % 360) * 3.14159265f / 180.f;
    const float c = std::cos(hue);
    const float s = std::sin(hue);
    const float lumR = 0.213f;
    const float lumG = 0.715f;
    const float lumB = 0.072f;

    cm.m[0][0] = lumR + c * (1.f - lumR) + s * (-lumR);
    cm.m[0][1] = lumG + c * (-lumG) + s * (-lumG);
    cm.m[0][2] = lumB + c * (-lumB) + s * (1.f - lumB);
    cm.m[1][0] = lumR + c * (-lumR) + s * (0.143f);
    cm.m[1][1] = lumG + c * (1.f - lumG) + s * (0.140f);
    cm.m[1][2] = lumB + c * (-lumB) + s * (-0.283f);
    cm.m[2][0] = lumR + c * (-lumR) + s * (-(1.f - lumR));
    cm.m[2][1] = lumG + c * (-lumG) + s * lumG;
    cm.m[2][2] = lumB + c * (1.f - lumB) + s * lumB;
}

void ApplyHueMatrixToRgb(BYTE& r, BYTE& g, BYTE& b, const Gdiplus::ColorMatrix& cm)
{
    const float rf = r / 255.f;
    const float gf = g / 255.f;
    const float bf = b / 255.f;
    const float nr =
        cm.m[0][0] * rf + cm.m[0][1] * gf + cm.m[0][2] * bf + cm.m[0][3] * 0.f + cm.m[0][4];
    const float ng =
        cm.m[1][0] * rf + cm.m[1][1] * gf + cm.m[1][2] * bf + cm.m[1][3] * 0.f + cm.m[1][4];
    const float nb =
        cm.m[2][0] * rf + cm.m[2][1] * gf + cm.m[2][2] * bf + cm.m[2][3] * 0.f + cm.m[2][4];
    r = static_cast<BYTE>(std::min(255.f, std::max(0.f, nr * 255.f + 0.5f)));
    g = static_cast<BYTE>(std::min(255.f, std::max(0.f, ng * 255.f + 0.5f)));
    b = static_cast<BYTE>(std::min(255.f, std::max(0.f, nb * 255.f + 0.5f)));
}

HBITMAP CreateHueTintedCopy(HBITMAP src, int width, int height, int hueShift)
{
    if (!src || hueShift == 0 || width <= 0 || height <= 0)
        return nullptr;

    BITMAP bm = {};
    if (GetObject(src, sizeof(bm), &bm) == 0 || bm.bmWidth <= 0 || bm.bmHeight <= 0)
        return nullptr;

    const int copyW = std::min(width, static_cast<int>(bm.bmWidth));
    const int copyH = std::min(height, static_cast<int>(bm.bmHeight));

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = copyW;
    bi.bmiHeader.biHeight = -copyH;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* dstBits = nullptr;
    HDC screen = GetDC(nullptr);
    HBITMAP dst = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &dstBits, nullptr, 0);
    if (!dst || !dstBits) {
        ReleaseDC(nullptr, screen);
        return nullptr;
    }

    std::memset(dstBits, 0, static_cast<size_t>(((copyW * 4 + 3) / 4) * 4) * copyH);

    HDC srcDc = CreateCompatibleDC(screen);
    HDC dstDc = CreateCompatibleDC(screen);
    HGDIOBJ oldSrc = SelectObject(srcDc, src);
    HGDIOBJ oldDst = SelectObject(dstDc, dst);
    BLENDFUNCTION copyBf = {};
    copyBf.BlendOp = AC_SRC_OVER;
    copyBf.SourceConstantAlpha = 255;
    copyBf.AlphaFormat = AC_SRC_ALPHA;
    GdiAlphaBlend(dstDc, 0, 0, copyW, copyH, srcDc, 0, 0, copyW, copyH, copyBf);
    SelectObject(srcDc, oldSrc);
    SelectObject(dstDc, oldDst);
    DeleteDC(srcDc);
    DeleteDC(dstDc);
    ReleaseDC(nullptr, screen);

    DIBSECTION outDs = {};
    if (GetObject(dst, sizeof(outDs), &outDs) != sizeof(DIBSECTION) || !outDs.dsBm.bmBits)
        return dst;

    Gdiplus::ColorMatrix cm = {};
    FillHueColorMatrix(hueShift, cm);
    const int outStride = outDs.dsBm.bmWidthBytes;
    auto* outBits = static_cast<BYTE*>(outDs.dsBm.bmBits);

    for (int y = 0; y < copyH; ++y) {
        BYTE* dp = outBits + static_cast<size_t>(y) * outStride;
        for (int x = 0; x < copyW; ++x) {
            const BYTE b = dp[x * 4 + 0];
            const BYTE g = dp[x * 4 + 1];
            const BYTE r = dp[x * 4 + 2];
            const BYTE a = dp[x * 4 + 3];
            if (a < 8 || IsColorkeyRgb(r, g, b)) {
                dp[x * 4 + 0] = 0;
                dp[x * 4 + 1] = 0;
                dp[x * 4 + 2] = 0;
                dp[x * 4 + 3] = 0;
                continue;
            }
            BYTE nr = r;
            BYTE ng = g;
            BYTE nb = b;
            ApplyHueMatrixToRgb(nr, ng, nb, cm);
            dp[x * 4 + 0] = nb;
            dp[x * 4 + 1] = ng;
            dp[x * 4 + 2] = nr;
            dp[x * 4 + 3] = a;
        }
    }
    return dst;
}

} /* namespace */

bool SpriteEngine::Init()
{
    if (inited_)
        return true;
    if (g_gdiRefCount == 0) {
        Gdiplus::GdiplusStartupInput input;
        if (Gdiplus::GdiplusStartup(&g_gdiToken, &input, nullptr) != Gdiplus::Ok)
            return false;
    }
    ++g_gdiRefCount;
    inited_ = true;
    return true;
}

void SpriteEngine::Shutdown()
{
    if (!inited_)
        return;
    ClearFrames();
    if (g_gdiRefCount > 0) {
        --g_gdiRefCount;
        if (g_gdiRefCount == 0 && g_gdiToken) {
            Gdiplus::GdiplusShutdown(g_gdiToken);
            g_gdiToken = 0;
        }
    }
    inited_ = false;
}

void SpriteEngine::ClearFrames()
{
    for (auto& f : frames_) {
        if (f.bitmap)
            DeleteObject(f.bitmap);
    }
    frames_.clear();
    frameIndex_ = 0;
    lastFrameAdvanceMs_ = 0;
}

void SpriteEngine::SetHueShift(int degrees)
{
    degrees = ((degrees % 360) + 360) % 360;
    hueShift_ = degrees;
}

SpriteFrame SpriteEngine::MakePlaceholder(const std::wstring& label)
{
    const int w = SIX_SEVEN_SPRITE_WIDTH;
    const int h = SIX_SEVEN_SPRITE_HEIGHT;
    HDC hdc = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(hdc);
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biPlanes = 1;
    void* bits = nullptr;
    HBITMAP hbmp = CreateDIBSection(mem, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HGDIOBJ old = SelectObject(mem, hbmp);
    const COLORREF key = MagentaKey();
    HBRUSH keyBrush = CreateSolidBrush(key);
    RECT rc = { 0, 0, w, h };
    FillRect(mem, &rc, keyBrush);
    HBRUSH body = CreateSolidBrush(RGB(120, 60, 180));
    RECT bodyRc = { 16, 24, 80, 120 };
    FillRect(mem, &bodyRc, body);
    SetBkMode(mem, TRANSPARENT);
    SetTextColor(mem, RGB(255, 255, 255));
    DrawTextW(mem, label.c_str(), -1, &bodyRc, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
    SelectObject(mem, old);
    DeleteObject(keyBrush);
    DeleteObject(body);
    if (bits) {
        const COLORREF key = MagentaKey();
        const int stride = ((w * 4 + 3) / 4) * 4;
        for (int py = 0; py < h; ++py) {
            for (int px = 0; px < w; ++px) {
                BYTE* p = static_cast<BYTE*>(bits) + static_cast<size_t>(py) * stride + px * 4;
                if (IsColorkeyRgb(p[2], p[1], p[0])) {
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
    return { hbmp, w, h };
}

bool SpriteEngine::LoadFrames(const std::wstring& folder)
{
    ClearFrames();
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
        SpriteFrame ph = MakePlaceholder(folder.substr(folder.find_last_of(L"\\/") + 1));
        if (ph.bitmap)
            frames_.push_back(ph);
        return !frames_.empty();
    }
    for (const auto& file : files) {
        Gdiplus::Bitmap bmp(file.c_str());
        if (bmp.GetLastStatus() != Gdiplus::Ok)
            continue;
        std::unique_ptr<Gdiplus::Bitmap> normalized = NormalizeSpriteBitmap(&bmp, file);
        Gdiplus::Bitmap* use = normalized ? normalized.get() : &bmp;
        int w = 0, h = 0;
        HBITMAP hb = CreateBitmapFromGdiplus(use, w, h);
        if (hb)
            frames_.push_back({ hb, w, h });
    }
    if (frames_.empty()) {
        SpriteFrame ph = MakePlaceholder(L"?");
        frames_.push_back(ph);
    }
    return true;
}

bool SpriteEngine::ApplySpriteSettings(const char* spritePathUtf8, bool animate)
{
    (void)spritePathUtf8;
    animate_ = animate;

    const SixSevenSpriteDef* def = FindSpriteDef(spritePathUtf8);
    fps_ = def ? def->fps : 12;
    loop_ = def ? def->loop : true;

    if (!animate_) {
        oneshotFinished_ = true;
    } else if (loop_) {
        oneshotFinished_ = false;
    } else {
        oneshotFinished_ = false;
        firedOneshot_ = false;
    }
    return true;
}

bool SpriteEngine::SetSprite(const char* spritePathUtf8, bool animate)
{
    if (!spritePathUtf8) {
        ClearFrames();
        currentPathStorage_.clear();
        currentPath_ = nullptr;
        oneshotFinished_ = true;
        return false;
    }

    const bool samePath =
        !currentPathStorage_.empty() && currentPathStorage_ == spritePathUtf8 && !frames_.empty();
    if (samePath && animate_ == animate)
        return true;

    ApplySpriteSettings(spritePathUtf8, animate);
    currentPathStorage_ = spritePathUtf8;
    currentPath_ = currentPathStorage_.c_str();

    if (samePath)
        return true;

    std::string rel = std::string(SIX_SEVEN_SPRITES_ROOT) + "/" + spritePathUtf8;
    const std::wstring folder = AssetPath(rel.c_str());
    if (!LoadFrames(folder)) {
        oneshotFinished_ = true;
        return false;
    }
    frameIndex_ = 0;
    lastFrameAdvanceMs_ = 0;
    return true;
}

bool SpriteEngine::SetSpriteIfDifferent(const char* spritePathUtf8, bool animate)
{
    if (!spritePathUtf8)
        return SetSprite(nullptr, animate);
    if (!currentPathStorage_.empty() && currentPathStorage_ == spritePathUtf8 && animate_ == animate &&
        !frames_.empty())
        return true;
    return SetSprite(spritePathUtf8, animate);
}

void SpriteEngine::TickFrame()
{
    if (!animate_ || frames_.empty())
        return;

    const DWORD now = GetTickCount();
    if (lastFrameAdvanceMs_ == 0)
        lastFrameAdvanceMs_ = now;
    const int msPerFrame = std::max(1, 1000 / std::max(1, fps_));
    if (now - lastFrameAdvanceMs_ < static_cast<DWORD>(msPerFrame))
        return;
    lastFrameAdvanceMs_ = now;

    ++frameIndex_;
    if (frameIndex_ >= static_cast<int>(frames_.size())) {
        if (loop_) {
            frameIndex_ = 0;
        } else {
            frameIndex_ = static_cast<int>(frames_.size()) - 1;
            if (!oneshotFinished_) {
                oneshotFinished_ = true;
                if (!firedOneshot_ && onOneshotEnd_) {
                    firedOneshot_ = true;
                    onOneshotEnd_();
                }
            }
        }
    }
}

void SpriteEngine::Draw(HDC hdc, int destX, int destY, int windowH, int& outSpriteTop)
{
    if (frames_.empty())
        return;
    const SpriteFrame& fr = frames_[static_cast<size_t>(frameIndex_)];
    outSpriteTop = destY;

    if (hueShift_ == 0) {
        HDC mem = CreateCompatibleDC(hdc);
        HGDIOBJ old = SelectObject(mem, fr.bitmap);
        BLENDFUNCTION bf = {};
        bf.BlendOp = AC_SRC_OVER;
        bf.SourceConstantAlpha = 255;
        bf.AlphaFormat = AC_SRC_ALPHA;
        GdiAlphaBlend(hdc, destX, destY, SIX_SEVEN_SPRITE_WIDTH, SIX_SEVEN_SPRITE_HEIGHT, mem,
                      0, 0, fr.width, fr.height, bf);
        SelectObject(mem, old);
        DeleteDC(mem);
        return;
    }

    HBITMAP tinted = CreateHueTintedCopy(fr.bitmap, fr.width, fr.height, hueShift_);
    HBITMAP drawBmp = tinted ? tinted : fr.bitmap;

    HDC mem = CreateCompatibleDC(hdc);
    HGDIOBJ old = SelectObject(mem, drawBmp);
    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;
    GdiAlphaBlend(hdc, destX, destY, SIX_SEVEN_SPRITE_WIDTH, SIX_SEVEN_SPRITE_HEIGHT, mem, 0, 0,
                  fr.width, fr.height, bf);
    SelectObject(mem, old);
    DeleteDC(mem);
    if (tinted)
        DeleteObject(tinted);
}

void SpriteEngine::DrawScaled(HDC hdc, int destX, int destY, int scaleW, int scaleH)
{
    if (frames_.empty() || scaleW <= 0 || scaleH <= 0)
        return;
    const SpriteFrame& fr = frames_[static_cast<size_t>(frameIndex_)];
    HDC mem = CreateCompatibleDC(hdc);
    HGDIOBJ old = SelectObject(mem, fr.bitmap);
    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;
    GdiAlphaBlend(hdc, destX, destY, scaleW, scaleH, mem, 0, 0, fr.width, fr.height, bf);
    SelectObject(mem, old);
    DeleteDC(mem);
}

int SpriteEngine::FrameWidth() const
{
    if (frames_.empty())
        return 0;
    return frames_[static_cast<size_t>(frameIndex_)].width;
}

int SpriteEngine::FrameHeight() const
{
    if (frames_.empty())
        return 0;
    return frames_[static_cast<size_t>(frameIndex_)].height;
}

bool SpriteEngine::PointInSpriteBounds(int clientX, int clientY, int spriteDrawX,
                                      int spriteDrawY) const
{
    if (frames_.empty())
        return false;
    const SpriteFrame& fr = frames_[static_cast<size_t>(frameIndex_)];
    const int x = clientX - spriteDrawX;
    const int y = clientY - spriteDrawY;
    return x >= 0 && y >= 0 && x < fr.width && y < fr.height;
}

bool SpriteEngine::HitTest(int clientX, int clientY, int spriteDrawX, int spriteDrawY) const
{
    if (!PointInSpriteBounds(clientX, clientY, spriteDrawX, spriteDrawY))
        return false;

    const SpriteFrame& fr = frames_[static_cast<size_t>(frameIndex_)];
    const int x = clientX - spriteDrawX;
    const int y = clientY - spriteDrawY;

    DIBSECTION ds = {};
    if (GetObject(fr.bitmap, sizeof(ds), &ds) != sizeof(DIBSECTION) || !ds.dsBm.bmBits)
        return true;

    const int stride = ds.dsBm.bmWidthBytes;
    const bool topDown = ds.dsBmih.biHeight < 0;
    const int row = topDown ? y : (fr.height - 1 - y);
    const auto* bits = static_cast<const BYTE*>(ds.dsBm.bmBits);
    const BYTE* p = bits + static_cast<size_t>(row) * stride + x * 4;
    if (p[3] < 8 || IsColorkeyRgb(p[2], p[1], p[0]))
        return false;
    return true;
}

} /* namespace six_seven */
