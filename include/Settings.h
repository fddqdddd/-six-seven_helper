#ifndef SIX_SEVEN_SETTINGS_H
#define SIX_SEVEN_SETTINGS_H

#include <string>

namespace six_seven {

struct AppSettings {
    int x = -1;
    int y = -1;
    bool mute = false;
    bool idleBreath = true;
    int defDelayMinSec = 90;
    int defDelayMaxSec = 180;
    int wanderWalkSec = 12;
    int bubbleMaxLines = 6;
    int mood = 65;
};

class Settings {
public:
    void Load(AppSettings& out);
    void Save(int x, int y, bool mute, bool idleBreath);
    bool IsAutostartEnabled() const;
    void SetAutostart(bool enabled);

    void SaveMood(int mood) const;

private:
    void EnsureDefaultIni(const std::wstring& iniPath);
};

} /* namespace six_seven */

#endif
