#include "../include/AudioEngine.h"
#include "../include/Util.h"

#include <windows.h>
#include <mmsystem.h>

namespace six_seven {

void AudioEngine::PlayFile(const char* relativeSoundUtf8)
{
    if (!relativeSoundUtf8 || !*relativeSoundUtf8)
        return;
    const std::wstring path = AssetPath(relativeSoundUtf8);
    if (!FileExists(path))
        return;
    PlaySoundW(path.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

void AudioEngine::Stop()
{
    PlaySoundW(nullptr, nullptr, SND_PURGE);
}

} /* namespace six_seven */
