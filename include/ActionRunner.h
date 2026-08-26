#ifndef SIX_SEVEN_ACTION_RUNNER_H
#define SIX_SEVEN_ACTION_RUNNER_H

#include "../include/ActionTypes.h"
#include "../include/SongMelody.h"
#include <functional>
#include <string>
#include <windows.h>

namespace six_seven {

class UserInformation;
class AudioEngine;
class MovementEngine;
class SpeechBubble;
class SpeechEngine;
class SpriteEngine;

class ActionRunner {
public:
    void Bind(HWND hwnd, SpriteEngine* sprites, AudioEngine* audio, SpeechEngine* speech,
              SpeechBubble* bubble, MovementEngine* move, bool* muted, bool* idleBreath,
              UserInformation* userInfo, std::function<void()> onReturnStay,
              std::function<void()> onChainNext, std::function<void()> onExitApp,
              std::function<void()> onRedraw);

    void Run(const SixSevenActionDef& action);
    void RunSong(const char* spritePath, const std::wstring& lyrics, bool spriteLoop,
                 const SongMelodySpec& melody = {});
    void ReturnToStay();
    void Cancel();
    bool IsBusy() const { return busy_; }
    bool UsesDictor() const { return usesDictor_; }
    bool IsSinging() const { return singing_; }
    void OnSpriteOneshotEnd();
    void PollMovement();
    void SetWanderWalkSec(int sec);
    void PollSpeech();
    bool IsPersistent() const { return persistent_; }
    void WakeFromPersistent();
    void TriggerWanderMove();
    void TriggerSleepActivity();

private:
    void CompleteAction();
    void BeginMoveForDef();

    HWND hwnd_ = nullptr;
    SpriteEngine* sprites_ = nullptr;
    AudioEngine* audio_ = nullptr;
    SpeechEngine* speech_ = nullptr;
    SpeechBubble* bubble_ = nullptr;
    MovementEngine* move_ = nullptr;
    bool* muted_ = nullptr;
    bool* idleBreath_ = nullptr;
    UserInformation* userInfo_ = nullptr;
    std::function<void()> onReturnStay_;
    std::function<void()> onChainNext_;
    std::function<void()> onExitApp_;
    std::function<void()> onRedraw_;

    bool busy_ = false;
    bool usesDictor_ = true;
    bool singing_ = false;
    bool stopTtsOnFinish_ = false;
    bool finishHandled_ = false;
    bool moveAction_ = false;
    bool persistent_ = false;
    DWORD wanderEndAt_ = 0;
    int wanderWalkMs_ = 12000;
    SixSevenOnFinish onFinish_ = SixSevenOnFinish::ReturnStay;
    std::wstring currentPhrase_;
};

} /* namespace six_seven */

#endif
