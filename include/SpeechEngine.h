#ifndef SIX_SEVEN_SPEECH_ENGINE_H
#define SIX_SEVEN_SPEECH_ENGINE_H

#include "SongMelody.h"

#include <functional>
#include <string>
#include <windows.h>

namespace six_seven {

class SpeechEngine {
public:
    bool Init();
    void Shutdown();
    void Speak(const std::wstring& text, std::function<void()> onDone);
    void Sing(const std::wstring& text, std::function<void()> onDone,
              const SongMelodySpec& melody = {});
    bool IsSinging() const { return singing_; }
    void Stop();
    void Poll();
    bool IsSpeaking() const { return speaking_; }
    size_t VisibleTextLength() const { return visibleChars_; }
    const std::wstring& CurrentText() const { return fullText_; }

private:
    int EstimateDurationMs(const std::wstring& text) const;
    void UpdateVisibleProgress();
    void RestoreNormalRate();
    void SpeakPayload(const std::wstring& payload, const std::wstring& displayText, DWORD flags,
                      std::function<void()> onDone);
    void SpeakNextSongChunk();
    void FinishSong();

    void* voice_ = nullptr;
    bool speaking_ = false;
    bool singing_ = false;
    bool songWordMode_ = false;
    std::function<void()> pendingDone_;
    std::function<void()> songOnDone_;
    std::vector<SongSpeakChunk> songChunks_;
    size_t songChunkIndex_ = 0;
    std::wstring fullText_;
    size_t visibleChars_ = 0;
    DWORD speakStartMs_ = 0;
    int estimatedMs_ = 0;
};

} /* namespace six_seven */

#endif
