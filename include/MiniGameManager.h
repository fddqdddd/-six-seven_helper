#ifndef SIX_SEVEN_MINI_GAME_MANAGER_H
#define SIX_SEVEN_MINI_GAME_MANAGER_H

#include "../include/MiniGameRecords.h"
#include "../include/MiniGameMemory.h"
#include "../include/SpriteEngine.h"

#include <deque>
#include <utility>
#include <windows.h>

namespace six_seven {

enum class ActiveMiniGame { None, Click, Memory, HideSeek };

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

    bool ShowHidePreStartDialog(bool* hardModeOut);
    void StartHideSeek(bool hardMode);

    bool ShowMemoryPreStartDialog(bool* hardModeOut);
    void StartMemoryShell(bool hardMode);

    bool RunGuessNumberGame();
    bool RunRpsGame();
    bool RunRiddleGame();
    bool RunSnakeGame();
    void ShowRecordsDialog();
    int RecordScore(bool hardMode) const;
    int MemoryRecordScore(bool hardMode) const;
    int GuessRecordScore(bool hardMode) const;
    int RpsRecordScore(bool hardMode) const;
    int HideRecordScore(bool hardMode) const;

    void Stop();

    void BeginHardGlitches(DWORD now);
    void TickHardGlitches(DWORD now);
    void StopHardGlitches();
    void OnMemoryGameStopped();

private:
    struct GameHost;

    void TeleportCharacter();
    void TeleportHideIcon();
    void PaintHideIcon();
    void CreateHideIcon();
    void DestroyHideIcon();
    void EndHideGame();
    void OnHideIconClick();
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
    static bool RunModalGame(HWND dlg);

    struct GuessGameData {
        MiniGameManager* mgr = nullptr;
        HWND status = nullptr;
        HWND edit = nullptr;
        int range = 0;
        int maxAttempts = 0;
        int attemptsLeft = 0;
        int secret = 0;
        int streak = 0;
        bool hard = false;
        bool done = false;
    };
    struct RiddleGameData {
        MiniGameManager* mgr = nullptr;
        HWND status = nullptr;
        HWND edit = nullptr;
        std::wstring answer;
        bool done = false;
    };
    struct RpsGameData {
        MiniGameManager* mgr = nullptr;
        HWND status = nullptr;
        int round = 1;
        int userWins = 0;
        int cpuWins = 0;
        int maxRounds = 0;
        bool hard = false;
    };
    struct SnakeGameData {
        MiniGameManager* mgr = nullptr;
        HWND scoreText = nullptr;
        std::deque<std::pair<int, int>> snake;
        std::pair<int, int> dir = { 1, 0 };
        std::pair<int, int> apple = { 8, 6 };
        int score = 0;
        bool running = true;
        bool ended = false;
    };
    void OnGuessSubmit(HWND hwnd, GuessGameData* data) const;
    void OnRpsMove(HWND hwnd, RpsGameData* data, int pick);
    void OnRiddleSubmit(HWND hwnd, RiddleGameData* data) const;
    static void RiddleNormalizeAnswer(std::wstring& s);
    void OnSnakeTick(HWND hwnd, SnakeGameData* data);
    void OnSnakeEnd(HWND hwnd, SnakeGameData* data);
    static LRESULT CALLBACK GuessGameWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK RpsGameWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK RiddleGameWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK SnakeGameWndProc(HWND, UINT, WPARAM, LPARAM);

    static LRESULT CALLBACK HudWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK HideIconWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK GrayWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK GlitchSpriteWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK PreStartWndProc(HWND, UINT, WPARAM, LPARAM);

    const wchar_t* preStartTitle_ = nullptr;
    const wchar_t* preStartText_ = nullptr;
    const wchar_t* preStartHardLabel_ = nullptr;

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

    bool guessClassRegistered_ = false;
    bool rpsClassRegistered_ = false;
    bool riddleClassRegistered_ = false;
    bool snakeClassRegistered_ = false;

    HWND hideIconHwnd_ = nullptr;
    HBITMAP hideIconDib_ = nullptr;
    void* hideIconDibBits_ = nullptr;
    HDC hideIconDibDc_ = nullptr;
    bool hideIconClassRegistered_ = false;
    DWORD hideNextMoveMs_ = 0;
    int hideIconIndex_ = 0;
};

} /* namespace six_seven */

#endif
