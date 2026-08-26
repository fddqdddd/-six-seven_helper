#ifndef SIX_SEVEN_MINI_GAME_MEMORY_H
#define SIX_SEVEN_MINI_GAME_MEMORY_H

#include "../include/MiniGameRecords.h"

#include <windows.h>

namespace six_seven {

class Application;
class MiniGameManager;

class MemoryShellGame {
public:
    void Bind(Application* app, MiniGameManager* mgr, MiniGameRecords* records);

    bool IsActive() const { return active_; }
    bool UsesMainCharacter() const { return false; }

    bool ShowPreStartDialog(bool* hardModeOut);
    void Start(bool hardMode);
    void Stop();
    void Tick();

    int RecordScore(bool hardMode) const;

    struct MemBitmap {
        HBITMAP hbmp = nullptr;
        int w = 0;
        int h = 0;
    };

private:

    enum class CupKind { Real = 0, Fake1, Fake2, Fake3 };

    enum class Phase {
        Preview,
        Darken,
        Shuffle,
        Pick,
        Success,
        Scare,
    };

    int OverlayWidth() const;
    int OverlayHeight() const;
    int SlotX(int slot) const;

    struct SwapAnim {
        bool active = false;
        int slotA = 0;
        int slotB = 0;
        float t = 0.f;
    };

    void LoadAssets();
    void FreeAssets();
    bool LoadMemBitmap(const char* spriteRel, MemBitmap& out, const wchar_t* placeholder);
    void SetupRound();
    void BeginPhase(Phase p, DWORD now);
    DWORD ShuffleDurationMs() const;
    DWORD SwapIntervalMs() const;
    int SlotCount() const;
    void DoSwap(int a, int b);
    void StartSwapAnim(int a, int b, DWORD now);
    void TickSwapAnim(DWORD now);
    int HitSlot(int clientX, int clientY) const;
    void OnSlotClick(int slot);
    void EndGame(bool win);
    void PaintOverlay();
    void PaintScareOverlay();
    void CreateOverlay();
    void DestroyOverlay();
    void CreateScareOverlay();
    void DestroyScareOverlay();
    void UpdateHud();
    void DrawCup(HDC hdc, int slot, CupKind kind, int x, int y, float brightness, float scale,
                 bool useReveal) const;
    const MemBitmap& BitmapFor(CupKind kind, bool useReveal) const;

    static LRESULT CALLBACK ScareWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK MemHudWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK OverlayWndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK MemPreStartWndProc(HWND, UINT, WPARAM, LPARAM);

    Application* app_ = nullptr;
    MiniGameManager* mgr_ = nullptr;
    MiniGameRecords* records_ = nullptr;

    bool active_ = false;
    bool hardMode_ = false;
    int round_ = 1;
    int score_ = 0;

    Phase phase_ = Phase::Preview;
    DWORD phaseStartMs_ = 0;
    DWORD shuffleEndMs_ = 0;
    DWORD nextSwapMs_ = 0;
    float brightness_ = 1.f;

    int slotContents_[4] = {};
    SwapAnim swap_;
    int scareSlot_ = -1;
    float scarePopT_ = 0.f;
    float scareTargetScale_ = 1.f;

    MemBitmap bmpReal_;
    MemBitmap bmpFake1_;
    MemBitmap bmpFake2_;
    MemBitmap bmpFake3_;
    MemBitmap bmpReveal_;
    MemBitmap bmpScare_;

    HWND overlayHwnd_ = nullptr;
    HBITMAP overlayDib_ = nullptr;
    void* overlayDibBits_ = nullptr;
    HDC overlayDibDc_ = nullptr;

    HWND scareHwnd_ = nullptr;
    HBITMAP scareDib_ = nullptr;
    void* scareDibBits_ = nullptr;
    HDC scareDibDc_ = nullptr;
    int scareScreenW_ = 0;
    int scareScreenH_ = 0;

    HWND hudHwnd_ = nullptr;
    HBITMAP hudDib_ = nullptr;
    void* hudDibBits_ = nullptr;
    HDC hudDibDc_ = nullptr;

    int savedWinLeft_ = 0;
    int savedWinTop_ = 0;
    bool savedWinPos_ = false;

    bool overlayClassRegistered_ = false;
    bool scareClassRegistered_ = false;
    bool hudClassRegistered_ = false;
    bool preStartClassRegistered_ = false;
    bool preStartStarted_ = false;
    bool preStartHard_ = false;
};

} /* namespace six_seven */

#endif
