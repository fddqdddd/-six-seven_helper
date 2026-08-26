#include "../include/SpeechBubble.h"
#include "../config.h"

#include <algorithm>
#include <windows.h>

namespace six_seven {

COLORREF SpeechBubble::ConfigColor(int rgb)
{
    return RGB((rgb)&0xFF, (rgb >> 8) & 0xFF, (rgb >> 16) & 0xFF);
}

bool SpeechBubble::PointInRoundRect(int px, int py, int l, int t, int r, int b, int radius)
{
    if (px < l || px >= r || py < t || py >= b)
        return false;
    radius = std::max(0, radius);
    if (px >= l + radius && px < r - radius)
        return true;
    if (py >= t + radius && py < b - radius)
        return true;
    int cx = l + radius;
    int cy = t + radius;
    if (px < l + radius && py < t + radius) {
        const int dx = px - cx;
        const int dy = py - cy;
        return dx * dx + dy * dy <= radius * radius;
    }
    cx = r - radius - 1;
    cy = t + radius;
    if (px >= r - radius && py < t + radius) {
        const int dx = px - cx;
        const int dy = py - cy;
        return dx * dx + dy * dy <= radius * radius;
    }
    cx = l + radius;
    cy = b - radius - 1;
    if (px < l + radius && py >= b - radius) {
        const int dx = px - cx;
        const int dy = py - cy;
        return dx * dx + dy * dy <= radius * radius;
    }
    cx = r - radius - 1;
    cy = b - radius - 1;
    {
        const int dx = px - cx;
        const int dy = py - cy;
        return dx * dx + dy * dy <= radius * radius;
    }
}

bool SpeechBubble::PointInTriangle(int px, int py, const POINT& a, const POINT& b, const POINT& c)
{
    const auto sign = [](int x1, int y1, int x2, int y2, int x3, int y3) {
        return (x1 - x3) * (y2 - y3) - (x2 - x3) * (y1 - y3);
    };
    const int d1 = sign(px, py, a.x, a.y, b.x, b.y);
    const int d2 = sign(px, py, b.x, b.y, c.x, c.y);
    const int d3 = sign(px, py, c.x, c.y, a.x, a.y);
    const bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    const bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);
    return !(hasNeg && hasPos);
}

void SpeechBubble::BeginPhrase(const std::wstring& fullText)
{
    fullText_ = fullText;
    visibleChars_ = 0;
    layout_.visible = !fullText_.empty();
}

void SpeechBubble::SyncReveal(size_t charCount)
{
    if (fullText_.empty())
        return;
    const size_t n = std::min(charCount, fullText_.size());
    if (n > visibleChars_)
        visibleChars_ = n;
}

void SpeechBubble::RevealAll()
{
    visibleChars_ = fullText_.size();
}

void SpeechBubble::SetVisibleLength(size_t charCount)
{
    if (fullText_.empty()) {
        visibleChars_ = 0;
        return;
    }
    visibleChars_ = std::min(charCount, fullText_.size());
}

std::wstring SpeechBubble::VisibleText() const
{
    if (visibleChars_ >= fullText_.size())
        return fullText_;
    return fullText_.substr(0, visibleChars_);
}

void SpeechBubble::Clear()
{
    fullText_.clear();
    visibleChars_ = 0;
    layout_.visible = false;
}

int SpeechBubble::CountWrappedLines(HDC hdc, const std::wstring& text) const
{
    if (text.empty())
        return 0;
    RECT rc = { 0, 0, SIX_SEVEN_BUBBLE_MAX_WIDTH, 0 };
    DrawTextW(hdc, text.c_str(), static_cast<int>(text.size()), &rc,
              DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL);
    const int lineH = std::max(1, LineHeightPx(hdc));
    return std::max(1, static_cast<int>((rc.bottom + lineH - 1) / lineH));
}

int SpeechBubble::WrappedTextHeight(HDC hdc, const std::wstring& text) const
{
    if (text.empty())
        return 0;
    RECT rc = { 0, 0, SIX_SEVEN_BUBBLE_MAX_WIDTH, 0 };
    DrawTextW(hdc, text.c_str(), static_cast<int>(text.size()), &rc,
              DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL);
    return rc.bottom;
}

int SpeechBubble::LineHeightPx(HDC hdc) const
{
    TEXTMETRICW tm = {};
    GetTextMetricsW(hdc, &tm);
    return std::max(1, static_cast<int>(tm.tmHeight + tm.tmExternalLeading));
}

size_t SpeechBubble::ScrollCharOffset(HDC hdc, const std::wstring& shown) const
{
    if (shown.empty() || maxLines_ <= 0)
        return 0;
    if (CountWrappedLines(hdc, shown) <= maxLines_)
        return 0;

    size_t offset = 0;
    while (offset < shown.size()) {
        const std::wstring tail = shown.substr(offset);
        if (CountWrappedLines(hdc, tail) <= maxLines_)
            break;
        while (offset < shown.size() && shown[offset] != L' ' && shown[offset] != L'\n' &&
               shown[offset] != L'\r')
            ++offset;
        while (offset < shown.size() &&
               (shown[offset] == L' ' || shown[offset] == L'\n' || shown[offset] == L'\r'))
            ++offset;
    }
    return offset;
}

void SpeechBubble::Draw(HDC hdc, int windowWidth, int spriteTopY)
{
#if !SIX_SEVEN_BUBBLE_ENABLED
    (void)hdc;
    (void)windowWidth;
    (void)spriteTopY;
    return;
#endif
    if (!layout_.visible || fullText_.empty())
        return;

    const std::wstring shown = VisibleText();
    if (shown.empty())
        return;

    SetBkMode(hdc, TRANSPARENT);

    HFONT font = CreateFontW(
        SIX_SEVEN_BUBBLE_FONT_SIZE, 0, 0, 0, SIX_SEVEN_BUBBLE_FONT_WEIGHT, FALSE, FALSE,
        FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, SIX_SEVEN_BUBBLE_FONT);
    HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, font));

    HDC measureDc = CreateCompatibleDC(hdc);
    HGDIOBJ oldMeasureFont = SelectObject(measureDc, font);
    const int lineH = LineHeightPx(measureDc);
    const int maxTextH = lineH * maxLines_;
    const size_t scrollAt = ScrollCharOffset(measureDc, shown);
    const std::wstring display = shown.substr(scrollAt);
    int textH = WrappedTextHeight(measureDc, display);
    if (textH > maxTextH)
        textH = maxTextH;
    SelectObject(measureDc, oldMeasureFont);
    DeleteDC(measureDc);

    const int pad = SIX_SEVEN_BUBBLE_PADDING_PX;
    const int bw = SIX_SEVEN_BUBBLE_MAX_WIDTH + pad * 2;
    const int bh = textH + pad * 2;
    const int tailH = SIX_SEVEN_BUBBLE_TAIL_ENABLED ? SIX_SEVEN_BUBBLE_TAIL_HEIGHT_PX : 0;

    layout_.w = bw + SIX_SEVEN_BUBBLE_BORDER_PX * 2;
    layout_.h = bh + tailH + SIX_SEVEN_BUBBLE_BORDER_PX * 2;
    layout_.x = (windowWidth - layout_.w) / 2;
    layout_.y = SIX_SEVEN_BUBBLE_OFFSET_Y;
    if (layout_.y + layout_.h > spriteTopY - 4)
        layout_.y = spriteTopY - layout_.h - 4;
    if (layout_.y < 0)
        layout_.y = 0;

    HBRUSH bg = CreateSolidBrush(ConfigColor(SIX_SEVEN_BUBBLE_BG_RGB));
    HPEN border =
        CreatePen(PS_SOLID, SIX_SEVEN_BUBBLE_BORDER_PX, ConfigColor(SIX_SEVEN_BUBBLE_BORDER_RGB));
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, bg));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, border));

    const int bx = layout_.x;
    const int by = layout_.y;
    const int br = bx + layout_.w;
    const int bb = by + layout_.h - tailH;

    RoundRect(hdc, bx, by, br, bb, SIX_SEVEN_BUBBLE_RADIUS_PX * 2,
              SIX_SEVEN_BUBBLE_RADIUS_PX * 2);

#if SIX_SEVEN_BUBBLE_TAIL_ENABLED
    const int cx = (bx + br) / 2;
    POINT tail[3] = {
        { cx - SIX_SEVEN_BUBBLE_TAIL_WIDTH_PX / 2, bb - 1 },
        { cx + SIX_SEVEN_BUBBLE_TAIL_WIDTH_PX / 2, bb - 1 },
        { cx, bb + tailH },
    };
    Polygon(hdc, tail, 3);
#endif

    SetTextColor(hdc, ConfigColor(SIX_SEVEN_BUBBLE_TEXT_RGB));
    RECT textRc = { bx + pad, by + pad, bx + pad + SIX_SEVEN_BUBBLE_MAX_WIDTH, bb - pad };
    DrawTextW(hdc, display.c_str(), static_cast<int>(display.size()), &textRc,
              DT_WORDBREAK | DT_NOPREFIX | DT_TOP | DT_CENTER | DT_EDITCONTROL);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldFont);
    DeleteObject(bg);
    DeleteObject(border);
    DeleteObject(font);
}

void SpeechBubble::ApplyLayerAlpha(BYTE* dibBits, int dibWidth, int dibHeight) const
{
#if !SIX_SEVEN_BUBBLE_ENABLED
    (void)dibBits;
    (void)dibWidth;
    (void)dibHeight;
    return;
#endif
    if (!layout_.visible || !dibBits || layout_.w <= 0 || layout_.h <= 0)
        return;

    const int tailH = SIX_SEVEN_BUBBLE_TAIL_ENABLED ? SIX_SEVEN_BUBBLE_TAIL_HEIGHT_PX : 0;
    const int bx = layout_.x;
    const int by = layout_.y;
    const int br = bx + layout_.w;
    const int bb = by + layout_.h - tailH;
    const int radius = SIX_SEVEN_BUBBLE_RADIUS_PX;

    const int x0 = std::max(0, bx);
    const int y0 = std::max(0, by);
    const int x1 = std::min(dibWidth, bx + layout_.w);
    const int y1 = std::min(dibHeight, by + layout_.h);

#if SIX_SEVEN_BUBBLE_TAIL_ENABLED
    const int cx = (bx + br) / 2;
    const POINT tailA = { cx - SIX_SEVEN_BUBBLE_TAIL_WIDTH_PX / 2, bb - 1 };
    const POINT tailB = { cx + SIX_SEVEN_BUBBLE_TAIL_WIDTH_PX / 2, bb - 1 };
    const POINT tailC = { cx, bb + tailH };
#endif

    for (int py = y0; py < y1; ++py) {
        BYTE* row = dibBits + (py * dibWidth + x0) * 4;
        for (int px = x0; px < x1; ++px) {
            bool inside = PointInRoundRect(px, py, bx, by, br, bb, radius);
#if SIX_SEVEN_BUBBLE_TAIL_ENABLED
            if (!inside && py >= bb - 1)
                inside = PointInTriangle(px, py, tailA, tailB, tailC);
#endif
            if (inside)
                row[(px - x0) * 4 + 3] = 255;
        }
    }
}

} /* namespace six_seven */
