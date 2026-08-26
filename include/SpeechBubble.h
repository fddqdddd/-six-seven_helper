#ifndef SIX_SEVEN_SPEECH_BUBBLE_H
#define SIX_SEVEN_SPEECH_BUBBLE_H

#include <string>
#include <windows.h>

namespace six_seven {

struct BubbleLayout {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    bool visible = false;
};

class SpeechBubble {
public:
    void SetMaxLines(int maxLines) { maxLines_ = std::max(1, maxLines); }
    void BeginPhrase(const std::wstring& fullText);
    void SyncReveal(size_t charCount);
    void RevealAll();
    void SetVisibleLength(size_t charCount);
    void Clear();
    bool IsVisible() const { return layout_.visible; }
    const BubbleLayout& Layout() const { return layout_; }
    void Draw(HDC hdc, int windowWidth, int spriteTopY);
    void ApplyLayerAlpha(BYTE* dibBits, int dibWidth, int dibHeight) const;

private:
    static COLORREF ConfigColor(int rgb);
    static bool PointInRoundRect(int px, int py, int l, int t, int r, int b, int radius);
    static bool PointInTriangle(int px, int py, const POINT& a, const POINT& b, const POINT& c);
    std::wstring VisibleText() const;
    size_t ScrollCharOffset(HDC hdc, const std::wstring& shown) const;
    int CountWrappedLines(HDC hdc, const std::wstring& text) const;
    int WrappedTextHeight(HDC hdc, const std::wstring& text) const;
    int LineHeightPx(HDC hdc) const;

    std::wstring fullText_;
    size_t visibleChars_ = 0;
    int maxLines_ = 6;
    BubbleLayout layout_;
};

} /* namespace six_seven */

#endif
