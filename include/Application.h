#ifndef SIX_SEVEN_APPLICATION_H
#define SIX_SEVEN_APPLICATION_H

#include "../include/ActionRunner.h"
#include "../include/AudioEngine.h"
#include "../include/MiniGameManager.h"
#include "../include/MovementEngine.h"
#include "../include/Settings.h"
#include "../include/UserInformation.h"
#include "../include/SpeechBubble.h"
#include "../include/SpeechEngine.h"
#include "../include/SpriteEngine.h"
#include "../include/TrayIcon.h"
#include "../include/TerminalGuard.h"

#include <functional>
#include <string>
#include <vector>
#include <windows.h>

namespace six_seven {

struct BirthdayDialogData;

class Application {
public:
    int Run(HINSTANCE inst);

    HWND MainHwnd() const { return hwnd_; }
    HINSTANCE Inst() const { return inst_; }
    SpriteEngine& Sprites() { return sprites_; }
    ActionRunner& Actions() { return actions_; }
    SpeechBubble& Bubble() { return bubble_; }
    void RepaintMain() { Paint(); }
    void ClampMainWindow(int& x, int& y, const POINT* anchor = nullptr);
    void ReturnToIdleSprite();
    void GetSpriteDrawPos(int& drawX, int& drawY) const;
    MiniGameManager& MiniGames() { return miniGames_; }
    AudioEngine& Audio() { return audio_; }
    void AllowAppTerminal(unsigned count = 1);
    void AwardVaultFragment();
    void SpeakNotice(const std::wstring& text, std::function<void()> onDone = {});

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK KeyboardHookProc(int code, WPARAM wParam, LPARAM lParam);
    bool Init(HINSTANCE inst);
    void Shutdown();
    void Paint();
    void OnTimer(WPARAM timerId);
    void NoteUserActivity();
    void ScheduleDef();
    void FireDefIfDue();
    void FireTimeActions();
    void StartStartupChain();
    void OnStartupChainNext();
    const char* PickHelloContextPhraseFile() const;
    void RefreshMood();
    int Mood() const { return mood_; }
    void TickCursorCatch();
    void TickAnger();
    void TickLoudTyping();
    void TickCadPanic();
    void TickApps();
    void TickStretch();
    void TickVaultPatience();
    void SaveAngerIfNeeded();
    void WriteServeFileToDesktop();
    void WriteGiftToDesktop();
    void WriteTeaseFileToDesktop();
    void CreateTrapDocument();
    void TickTrapDocument();
    void HideFilesToVault();
    void TryRestoreVault();
    void StartFirstRun();
    void OnFirstRunWake();
    void OnFirstRunAppearanceDone();
    void BeginFirstRunDialogue();
    void AskFirstRunName();
    void AskFirstRunColor();
    void AskFirstRunBirthday();
    void AskFirstRunSeason();
    void AskFirstRunFood();
    void SpeakFirstRunBirthdayDateReaction(int month, int day);
    void SpeakFirstRunTutorialAndFinish();
    void FinishFirstRun();
    void SpeakFirstRunLine(const std::wstring& text, std::function<void()> onDone,
                           bool greetingAnim = false);
    void StartFirstRunSmallWander();
    void TickFirstRunWander();
    void TickFirstRunGlide();
    bool ShowNameInputDialog(std::wstring& outName);
    bool ShowFavoriteColorDialog(std::wstring& outColor);
    bool ShowFavoriteSeasonDialog(std::wstring& outSeason, const std::wstring& suggested);
    bool ShowFavoriteFoodDialog(std::wstring& outFood);
    bool ShowBirthdayPickerDialog(int& outMonth, int& outDay, int& outYear);
    bool RunModalUntilDestroyed(HWND dlg);
    static LRESULT CALLBACK NameDialogWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK ColorDialogWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK SeasonDialogWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK FoodDialogWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK SongsDialogWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK BirthdayDialogWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK AdminDialogWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK AdminPanelDialogWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK CommandsDialogWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK ChatDialogWndProc(HWND, UINT, WPARAM, LPARAM);
    static bool AcceptBirthdayDialog(HWND hwnd, BirthdayDialogData* data);
    static void LayoutBirthdayDialog(HWND hwnd, BirthdayDialogData* data);
    void RunSongByIndex(int songIndex);
    void ShowSongsDialog();
    void StartShutdownChain();
    void OnShutdownChainNext();
    void ForceQuit();
    void ShowClickMenu(POINT pt);
    void ShowColorPicker();
    bool ShowAdminPasswordDialog();
    void TryPromoteToAdmin();
    void OpenAdminPanel();
    void LayoutAdminPanel(HWND hwnd);
    void ShowCommandsDialog();
    void UnlockTerminalRestrictions();
    void OnTerminalBlocked();
    void OnUserTerminalOpened();
    void ExecuteTerminalCommand(const std::wstring& command);
    void ShowChatDialog();
    std::wstring DeepSeekKeyStored();
    void SaveDeepSeekKeyToSettings(const std::wstring& key);
    std::wstring AskDeepSeek(const std::wstring& key, const std::wstring& question);
    std::wstring AddressName() const;
    void CenterCharacterOnScreen();
    void MoveCharacterAboveDialogsOnce();
    static void CenterDialog(HWND dlg, int width, int height);
    void ToggleAutostart();
    void RevertWindowsProfile();
    void RestartOnboarding();
    void ApplyProfileThenRestart();
    void OfferRestartForFullFunctionality();
    static void InitiateSystemRestart();
    static void BlockMouseInput();
    static void UnblockMouseInput();
    static LRESULT CALLBACK ColorPickerWndProc(HWND, UINT, WPARAM, LPARAM);
    void ClampToWorkArea(int& x, int& y, const POINT* followPoint = nullptr);
    bool IsInteractiveAt(int clientX, int clientY) const;
    int RandomWorkX();
    int RandomWorkY();

    HINSTANCE inst_ = nullptr;
    HWND hwnd_ = nullptr;
    HBITMAP dib_ = nullptr;
    void* dibBits_ = nullptr;
    HDC dibDc_ = nullptr;

    SpriteEngine sprites_;
    AudioEngine audio_;
    SpeechEngine speech_;
    SpeechBubble bubble_;
    MovementEngine movement_;
    ActionRunner actions_;
    TrayIcon tray_;
    Settings settings_;
    UserInformation userInfo_;
    MiniGameManager miniGames_;
    TerminalGuard terminalGuard_;

    bool muted_ = false;
    bool idleBreath_ = true;
    int defDelayMinMs_ = 90000;
    int defDelayMaxMs_ = 180000;
    bool dragging_ = false;
    int dragStartX_ = 0;
    int dragStartY_ = 0;
    POINT dragMouseStart_ = {};
    POINT dragWindowStart_ = {};
    bool shuttingDown_ = false;
    bool startupDone_ = false;
    bool firstRunActive_ = false;
    bool firstRunSleeping_ = false;
    bool firstRunAwaitingAppearance_ = false;
    bool firstRunDialogueActive_ = false;
    DWORD firstRunNextWanderAt_ = 0;
    bool firstRunGlideActive_ = false;
    bool firstRunMovedForDialog_ = false;
    POINT firstRunGlideFrom_ = {};
    POINT firstRunGlideTo_ = {};
    DWORD firstRunGlideStartMs_ = 0;
    int firstRunGlideDurationMs_ = 900;
    int firstRunLastWanderAngleDeg_ = -1;
    bool bootSplashShown_ = false;
    bool leavingStarted_ = false;
    bool movementWasActive_ = false;
    std::string lastMoveSprite_;
    DWORD lastActivity_ = 0;
    DWORD nextDefAt_ = 0;
    int mood_ = 65;
    int anger_ = 0;
    int vaultFragments_ = 0;
    int lastFragmentDay_ = 0;
    DWORD nextAngerDecayAt_ = 0;
    DWORD nextAngerSaveAt_ = 0;
    DWORD nextLoudTypingAt_ = 0;
    DWORD loudTypingWindowStart_ = 0;
    int loudTypingHits_ = 0;
    DWORD nextCadPanicAt_ = 0;
    DWORD cadDesktopSince_ = 0;
    bool onCadDesktop_ = false;
    bool cadPanicHidden_ = false;
    DWORD nextTeaseFileAt_ = 0;
    std::wstring trapFilePath_;
    bool trapTriggered_ = false;
    DWORD trapNextCheckAt_ = 0;
    int trapDeleteAttempts_ = 0;
    HHOOK keyboardHook_ = nullptr;
    DWORD nextAppsCheckAt_ = 0;
    std::wstring lastSeenApp_;
    DWORD nextStretchAt_ = 0;
    DWORD nextCursorCatchUntilMs_ = 0;
    DWORD nextCursorCatchAtMs_ = 0;
    std::vector<DWORD> timeLastFire_;
    HWND colorPickerHwnd_ = nullptr;
    bool pendingNameDialogAccepted_ = false;
    std::wstring pendingNameDialogValue_;
    bool pendingColorDialogAccepted_ = false;
    std::wstring pendingColorDialogValue_;
    bool pendingSeasonDialogAccepted_ = false;
    std::wstring pendingSeasonDialogValue_;
    std::wstring seasonDialogSuggested_;
    bool pendingFoodDialogAccepted_ = false;
    std::wstring pendingFoodDialogValue_;
    bool pendingAdminDialogAccepted_ = false;
    std::wstring pendingAdminDialogValue_;
    bool pendingBirthdayDialogAccepted_ = false;
    int pendingBirthdayMonth_ = 0;
    int pendingBirthdayDay_ = 0;
    int pendingBirthdayYear_ = 0;

    static Application* instance_;
};

} /* namespace six_seven */

#endif
