#ifndef SIX_SEVEN_AUDIO_ENGINE_H
#define SIX_SEVEN_AUDIO_ENGINE_H

namespace six_seven {

class AudioEngine {
public:
    void PlayFile(const char* relativeSoundUtf8);
    void Stop();
};

} /* namespace six_seven */

#endif
