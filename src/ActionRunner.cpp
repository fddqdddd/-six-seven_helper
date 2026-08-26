#include "../include/ActionRunner.h"

#include <cstring>
#include <cmath>
#include <algorithm>
#include "../include/AudioEngine.h"
#include "../include/MovementEngine.h"
#include "../include/Phrases.h"
#include "../include/UserInformation.h"
#include "../include/SpeechBubble.h"
#include "../include/SpeechEngine.h"
#include "../include/SpriteEngine.h"
#include "../include/Util.h"
#include "../include/ConfigData.h"
#include "../config.h"

namespace six_seven {

void ActionRunner::Bind(HWND hwnd, SpriteEngine* sprites, AudioEngine* audio,
                        SpeechEngine* speech, SpeechBubble* bubble, MovementEngine* move,
                        bool* muted, bool* idleBreath, UserInformation* userInfo,
                        std::function<void()> onReturnStay, std::function<void()> onChainNext,
                        std::function<void()> onExitApp, std::function<void()> onRedraw)
{
    hwnd_ = hwnd;
    sprites_ = sprites;
    audio_ = audio;
    speech_ = speech;
    bubble_ = bubble;
    move_ = move;
    muted_ = muted;
    idleBreath_ = idleBreath;
    userInfo_ = userInfo;
    onReturnStay_ = std::move(onReturnStay);
    onChainNext_ = std::move(onChainNext);
    onExitApp_ = std::move(onExitApp);
    onRedraw_ = std::move(onRedraw);
}

void ActionRunner::Cancel()
{
    busy_ = false;
    singing_ = false;
    persistent_ = false;
    stopTtsOnFinish_ = false;
    finishHandled_ = false;
    moveAction_ = false;
    wanderEndAt_ = 0;
    onFinish_ = SixSevenOnFinish::ReturnStay;
    if (move_)
        move_->Stop();
    if (speech_)
        speech_->Stop();
    if (audio_)
        audio_->Stop();
#if SIX_SEVEN_BUBBLE_ENABLED
    if (bubble_)
        bubble_->Clear();
#endif
}

void ActionRunner::ReturnToStay()
{
    busy_ = false;
    singing_ = false;
    persistent_ = false;
    stopTtsOnFinish_ = false;
    finishHandled_ = false;
    moveAction_ = false;
    wanderEndAt_ = 0;
    if (move_)
        move_->Stop();
    if (sprites_) {
        const bool breath = idleBreath_ && *idleBreath_;
        sprites_->SetSprite(SIX_SEVEN_STAY_SPRITE, breath);
    }
    if (bubble_)
        bubble_->Clear();
    if (onRedraw_)
        onRedraw_();
}

void ActionRunner::CompleteAction()
{
    moveAction_ = false;
    wanderEndAt_ = 0;
    busy_ = false;
    if (onFinish_ == SixSevenOnFinish::ExitApp) {
        if (onExitApp_)
            onExitApp_();
        return;
    }
    if (onFinish_ == SixSevenOnFinish::ChainNext) {
        if (onChainNext_)
            onChainNext_();
        return;
    }
    ReturnToStay();
    if (onReturnStay_)
        onReturnStay_();
}

void ActionRunner::BeginMoveForDef()
{
    if (!hwnd_ || !move_)
        return;
    RECT rc = {};
    GetWindowRect(hwnd_, &rc);
    int tx = 0;
    int ty = 0;
    int tries = 0;
    do {
        RandomSpriteWindowPos(tx, ty, SIX_SEVEN_MOVE_MARGIN_PX);
        ++tries;
    } while (tries < 12 && std::abs(tx - rc.left) < 64 && std::abs(ty - rc.top) < 64);
    move_->Start(hwnd_, tx, ty, true);
}

void ActionRunner::WakeFromPersistent()
{
    if (!persistent_)
        return;
    if (audio_)
        audio_->Stop();
    if (speech_)
        speech_->Stop();
    persistent_ = false;
    ReturnToStay();
    if (onReturnStay_)
        onReturnStay_();
}

void ActionRunner::RunSong(const char* spritePath, const std::wstring& lyrics, bool spriteLoop,
                           const SongMelodySpec& melody)
{
    if (!sprites_ || !spritePath)
        return;
    if (busy_ && audio_)
        audio_->Stop();
    busy_ = true;
    singing_ = true;
    usesDictor_ = true;
    persistent_ = false;
    stopTtsOnFinish_ = false;
    finishHandled_ = false;
    moveAction_ = false;
    wanderEndAt_ = 0;
    onFinish_ = SixSevenOnFinish::ReturnStay;
    if (move_)
        move_->Stop();

    sprites_->SetOnOneshotEnd([this]() { OnSpriteOneshotEnd(); });
    sprites_->SetSprite(spritePath, spriteLoop);

    const bool mute = muted_ && *muted_;
    currentPhrase_ = lyrics;
    if (speech_)
        speech_->Stop();
#if SIX_SEVEN_BUBBLE_ENABLED
    if (bubble_) {
        bubble_->BeginPhrase(currentPhrase_);
        if (mute || !SIX_SEVEN_BUBBLE_TYPEWRITER)
            bubble_->RevealAll();
    }
#endif

    auto songDone = [this]() {
        singing_ = false;
#if SIX_SEVEN_BUBBLE_ENABLED
        if (bubble_)
            bubble_->RevealAll();
#endif
        if (onRedraw_)
            onRedraw_();
        CompleteAction();
    };

    if (!mute && speech_ && !currentPhrase_.empty()) {
        speech_->Sing(currentPhrase_, songDone, melody);
    } else {
        songDone();
    }

    if (onRedraw_)
        onRedraw_();
}

void ActionRunner::Run(const SixSevenActionDef& action)
{
    if (!sprites_)
        return;
    if (busy_ && audio_)
        audio_->Stop();
    busy_ = true;
    singing_ = false;
    finishHandled_ = false;
    usesDictor_ = action.dictors;
    persistent_ = action.persistent;
    onFinish_ = action.on_finish;
    stopTtsOnFinish_ = action.stop_tts_on_finish;
    moveAction_ = action.move;
    wanderEndAt_ = 0;

    const bool animate =
        action.sprite_mode == SixSevenSpriteMode::oneshot ||
        (action.sprite_mode == SixSevenSpriteMode::loop &&
         strcmp(action.sprite_path, SIX_SEVEN_STAY_SPRITE) != 0);

    sprites_->SetOnOneshotEnd([this]() { OnSpriteOneshotEnd(); });
    if (action.move)
        BeginMoveForDef();
    else if (move_)
        move_->Stop();

    const char* sprite = action.sprite_path;
    if (action.move && move_ && move_->IsActive())
        sprite = move_->CurrentSpritePath();

    sprites_->SetSprite(sprite, animate);

    const bool mute = muted_ && *muted_;
    if (!mute) {
        if (audio_ && action.sound && *action.sound)
            audio_->PlayFile(action.sound);
    }

    currentPhrase_ = ResolvePhrase(action.phrase_file, action.phrase, userInfo_);
    const bool useDictor = usesDictor_;
    if (speech_)
        speech_->Stop();
#if SIX_SEVEN_BUBBLE_ENABLED
    if (bubble_) {
        bubble_->BeginPhrase(currentPhrase_);
        if (mute || !useDictor || currentPhrase_.empty() || !SIX_SEVEN_BUBBLE_TYPEWRITER)
            bubble_->RevealAll();
    }
#endif

    auto speechDone = [this]() {
#if SIX_SEVEN_BUBBLE_ENABLED
        if (bubble_)
            bubble_->RevealAll();
#endif
        if (onRedraw_)
            onRedraw_();
        if (persistent_)
            return;
        if (moveAction_ && move_ && move_->IsActive()) {
            wanderEndAt_ = GetTickCount() + static_cast<DWORD>(wanderWalkMs_);
            return;
        }
        if (onFinish_ == SixSevenOnFinish::ExitApp)
            return;
        if (sprites_ && (sprites_->IsLoop() || sprites_->OneshotFinished()))
            CompleteAction();
    };

    if (!mute && useDictor && speech_ && !currentPhrase_.empty()) {
        speech_->Speak(currentPhrase_, speechDone);
    } else {
        speechDone();
    }

    if (onRedraw_)
        onRedraw_();

    if (!animate && !action.move && (!move_ || !move_->IsActive()))
        CompleteAction();
}

void ActionRunner::SetWanderWalkSec(int sec)
{
    wanderWalkMs_ = std::max(3, sec) * 1000;
}

void ActionRunner::PollMovement()
{
    if (!moveAction_ || !move_ || !move_->IsActive())
        return;
    if (wanderEndAt_ == 0)
        return;
    if (GetTickCount() < wanderEndAt_)
        return;
    move_->Stop();
    wanderEndAt_ = 0;
    if (!speech_ || !speech_->IsSpeaking())
        CompleteAction();
}

void ActionRunner::OnSpriteOneshotEnd()
{
    if (finishHandled_)
        return;
    if (persistent_)
        return;
    if (stopTtsOnFinish_ && speech_)
        speech_->Stop();
    if (move_ && move_->IsActive())
        return;
    if (onFinish_ != SixSevenOnFinish::ExitApp && speech_ && speech_->IsSpeaking())
        return;
#if SIX_SEVEN_BUBBLE_ENABLED
    if (bubble_ && onFinish_ == SixSevenOnFinish::ExitApp)
        bubble_->Clear();
#endif
    if (onFinish_ == SixSevenOnFinish::ExitApp && speech_)
        speech_->Stop();
    finishHandled_ = true;
    CompleteAction();
}

void ActionRunner::PollSpeech()
{
#if SIX_SEVEN_TTS_ENABLED
    if (!speech_ || !busy_)
        return;
    /* Completion handled in callback; optional purge */
#endif
}

void ActionRunner::TriggerWanderMove()
{
    if (!hwnd_ || busy_)
        return;
    for (int i = 0; i < kDefActionCount; ++i) {
        if (kDefActions[i].move) {
            Run(kDefActions[i]);
            return;
        }
    }
    busy_ = true;
    moveAction_ = true;
    onFinish_ = SixSevenOnFinish::ReturnStay;
    BeginMoveForDef();
    wanderEndAt_ = GetTickCount() + static_cast<DWORD>(wanderWalkMs_);
    if (sprites_ && move_ && move_->IsActive()) {
        const char* mp = move_->CurrentSpritePath();
        if (mp)
            sprites_->SetSprite(mp, true);
    }
    if (onRedraw_)
        onRedraw_();
}

void ActionRunner::TriggerSleepActivity()
{
    if (!hwnd_ || busy_)
        return;
    if (kSleepActionCount > 0)
        Run(PickRandom(kSleepActions, kSleepActionCount));
}

} /* namespace six_seven */
