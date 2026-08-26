#ifndef SIX_SEVEN_VIDEO_BOOT_SPLASH_H
#define SIX_SEVEN_VIDEO_BOOT_SPLASH_H

#include <string>
#include <windows.h>

namespace six_seven {

/* VideoBootSplash — полноэкранное MP4-видео + звук при входе в Windows.
 * Отдельный .exe для тестирования на VM. НЕ модифицирует BIOS.
 * Видео: assets/boot/video.mp4, звук: assets/boot/sound.wav или .mp3. */
class VideoBootSplash {
public:
    static bool ShouldShow();
    static void SyncAutostart(bool enable);
    static bool WasShown() { return wasShown_; }

    bool Run(HINSTANCE inst);

    static std::wstring VideoPath();
    static std::wstring SoundPath();

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

    bool Create(HINSTANCE inst);
    void Destroy();
    void PlayVideo();
    void PlayAudio();
    void StopAudio();
    bool ShouldEnd() const;
    bool IsShellReady() const;
    void PumpMessages();

    HINSTANCE inst_ = nullptr;
    HWND hwnd_ = nullptr;
    bool active_ = false;
    DWORD startedMs_ = 0;
    DWORD durationMs_ = 0;

    UINT videoDev_ = 0;
    bool videoOpen_ = false;
    UINT audioDev_ = 0;
    bool audioOpen_ = false;

    static bool classRegistered_;
    static bool wasShown_;
};

} /* namespace six_seven */

#endif
