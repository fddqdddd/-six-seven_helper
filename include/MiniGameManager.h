#ifndef SIX_SEVEN_MINI_GAME_MANAGER_H
#define SIX_SEVEN_MINI_GAME_MANAGER_H

#include "../include/MiniGameRecords.h"
#include "../include/MiniGameMemory.h"
#include "../include/SpriteEngine.h"

#include <windows.h>

namespace six_seven {

enum class ActiveMiniGame { None, Click, Memory };

class Application;

class MiniGameManager {
public:
    void Bind(Application* app);
    void LoadRecords();

    bool IsActive() const { return activeGame_ != ActiveMiniGame::None; }
    bool UsesMainCharacterPaint() const { return activeGame_ == ActiveMiniGame::Click; }
    void Tick();
    bool OnCharacterClick(int clientX, int clientY);

    bool ShowPreStartDialog(bool* hardModeOut);
    void StartClickSixSeven(bool hardMode);

    bool ShowMemoryPreStartDialog(bool* hardModeOut);
    void StartMemoryShell(bool hardMode);

    void ShowRecordsDialog();
    int RecordScore(bool hardMode) const;
    int MemoryRecordScore(bool hardMode) const;

    void Stop();

    void BeginHardGlitches(DWORD now);
    void TickHardGlitches(DWORD now);
    void StopHardGlitches();
    void OnMemoryGameStopped();

private:
    struct GameHost;

    void TeleportCharacter();
    void UpdateHud();
    void PaintHud();
    void CreateHud();
    void DestroyHud();
    void TickGlitches(DWORD now);
    void ShowGrayGlitch(DWORD now);
    void HideGrayGlitch();
    void ShowSpriteGlitch(DWORD now);
    void HideSpriteGlitch();
    void PaintGlitchSprite();
    void EndGame();
    void FormatTime(wchar_t* buf, size_t count, DWORD msLeft) const;

    static LRESULT CALLBACK HudWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK GrayWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK GlitchSpriteWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK PreStartWndProc(HWND, UINT, WPARAM, LPARAM);

    Application* app_ = nullptr;
    MiniGameRecords records_;
    MemoryShellGame memory_;
    SpriteEngine glitchSprites_;

    ActiveMiniGame activeGame_ = ActiveMiniGame::None;
    bool hardMode_ = false;
    int score_ = 0;
    DWORD startMs_ = 0;
    DWORD endMs_ = 0;
    DWORD lastMoveMs_ = 0;
    DWORD lastHudMs_ = 0;

    DWORD nextGrayGlitchMs_ = 0;
    DWORD grayHideMs_ = 0;
    HWND grayHwnd_ = nullptr;

    DWORD nextSpriteGlitchMs_ = 0;
    DWORD spriteGlitchHideMs_ = 0;
    HWND glitchHwnd_ = nullptr;
    HBITMAP glitchDib_ = nullptr;
    void* glitchDibBits_ = nullptr;
    HDC glitchDibDc_ = nullptr;

    HWND hudHwnd_ = nullptr;
    HBITMAP hudDib_ = nullptr;
    void* hudDibBits_ = nullptr;
    HDC hudDibDc_ = nullptr;
    int savedWinLeft_ = 0;
    int savedWinTop_ = 0;
    bool savedWinPos_ = false;
    bool hudClassRegistered_ = false;
    bool grayClassRegistered_ = false;
    bool glitchClassRegistered_ = false;
    bool preStartClassRegistered_ = false;

    bool preStartStarted_ = false;
    bool preStartHard_ = false;
};

} /* namespace six_seven */

#endif
