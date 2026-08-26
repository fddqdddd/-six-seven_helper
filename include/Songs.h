#ifndef SIX_SEVEN_SONGS_H
#define SIX_SEVEN_SONGS_H

#include "SongMelody.h"

#include <string>
#include <vector>

namespace six_seven {

class UserInformation;

struct SongDef {
    std::string id;
    std::wstring title;
    std::wstring lyrics;
    std::string sprite;
    bool sprite_loop = false;
    bool birthday_only = false;
    SongMelodySpec melody;
};

int FindSongIndex(const char* id);
void FillAvailableSongIndices(const UserInformation* info, std::vector<int>& outIndices);

} /* namespace six_seven */

#endif
