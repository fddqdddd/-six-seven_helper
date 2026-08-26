#ifndef SIX_SEVEN_SPRITE_ENGINE_H
#define SIX_SEVEN_SPRITE_ENGINE_H

#include <functional>
#include <string>
#include <vector>
#include <windows.h>

namespace six_seven {

struct SpriteFrame {
    HBITMAP bitmap = nullptr;
    int width = 0;
    int height = 0;
};

class SpriteEngine {
public:
    using FinishedCallback = std::function<void()>;

    bool Init();
    void Shutdown();

    bool SetSprite(const char* spritePathUtf8, bool animate);
    bool SetSpriteIfDifferent(const char* spritePathUtf8, bool animate);
    void TickFrame();
    void Draw(HDC hdc, int destX, int destY, int windowH, int& outSpriteTop);

    void SetHueShift(int degrees);
    int HueShift() const { return hueShift_; }

    bool IsAnimating() const { return animate_; }
    bool IsLoop() const { return loop_; }
    bool OneshotFinished() const { return oneshotFinished_; }
    void SetOnOneshotEnd(FinishedCallback cb) { onOneshotEnd_ = std::move(cb); }

    bool HitTest(int clientX, int clientY, int spriteDrawX, int spriteDrawY) const;
    bool PointInSpriteBounds(int clientX, int clientY, int spriteDrawX, int spriteDrawY) const;

    int FrameWidth() const;
    int FrameHeight() const;

private:
    bool inited_ = false;
    bool LoadFrames(const std::wstring& folder);
    void ClearFrames();
    SpriteFrame MakePlaceholder(const std::wstring& label);
    bool ApplySpriteSettings(const char* spritePathUtf8, bool animate);
    const char* currentPath_ = nullptr;
    std::vector<SpriteFrame> frames_;
    int frameIndex_ = 0;
    int fps_ = 12;
    bool loop_ = true;
    bool animate_ = false;
    bool oneshotFinished_ = true;
    FinishedCallback onOneshotEnd_;
    bool firedOneshot_ = false;
    std::string currentPathStorage_;
    DWORD lastFrameAdvanceMs_ = 0;
    int hueShift_ = 0;
};

} /* namespace six_seven */

#endif
