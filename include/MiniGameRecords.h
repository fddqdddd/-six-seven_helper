#ifndef SIX_SEVEN_MINI_GAME_RECORDS_H
#define SIX_SEVEN_MINI_GAME_RECORDS_H

#include <string>

namespace six_seven {

class MiniGameRecords {
public:
    void Load();
    int Get(const char* gameId, bool hardMode) const;
    bool TrySave(const char* gameId, bool hardMode, int score);

private:
    std::wstring IniPath() const;
};

} /* namespace six_seven */

#endif
