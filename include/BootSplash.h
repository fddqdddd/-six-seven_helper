#ifndef SIX_SEVEN_BOOT_SPLASH_H
#define SIX_SEVEN_BOOT_SPLASH_H

#include <vector>
#include <windows.h>

namespace six_seven {

/* Опциональный overlay при входе в Windows (не логотип ПК — см. BootLogoInstaller). */
class BootSplash {
public:
    static bool ShouldShow();
    static void SyncAutostart(bool enable);
    static bool WasShown() { return wasShown_; }

    bool Run(HINSTANCE inst);

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

    bool Create(HINSTANCE inst);
    void Destroy();
    bool LoadFrames();
    void ClearFrames();
    void TickFrame();
    void Paint();
    bool ShouldEnd() const;
    bool IsShellReady() const;
    void PumpMessages();

    HINSTANCE inst_ = nullptr;
    HWND hwnd_ = nullptr;
    HBITMAP dib_ = nullptr;
    void* dibBits_ = nullptr;
    HDC dibDc_ = nullptr;
    int screenX_ = 0;
    int screenY_ = 0;
    int screenW_ = 0;
    int screenH_ = 0;
    bool active_ = false;
    DWORD startedMs_ = 0;
    DWORD lastFrameMs_ = 0;
    int frameIndex_ = 0;
    int loopCount_ = 0;
    int fps_ = 24;
    struct Frame {
        HBITMAP bitmap = nullptr;
        int width = 0;
        int height = 0;
    };
    std::vector<Frame> frames_;
    static bool classRegistered_;
    static bool wasShown_;
};

} /* namespace six_seven */

#endif
