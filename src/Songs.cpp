#include "../include/Songs.h"
#include "../include/SongCatalog.h"
#include "../include/UserInformation.h"

#include <cstring>

namespace six_seven {

int FindSongIndex(const char* id)
{
    if (!id)
        return -1;
    const auto& songs = GetSongs();
    for (int i = 0; i < static_cast<int>(songs.size()); ++i) {
        if (songs[static_cast<size_t>(i)].id == id)
            return i;
    }
    return -1;
}

void FillAvailableSongIndices(const UserInformation* info, std::vector<int>& outIndices)
{
    outIndices.clear();
    const auto& songs = GetSongs();
    for (int i = 0; i < static_cast<int>(songs.size()); ++i) {
        if (songs[static_cast<size_t>(i)].birthday_only && (!info || !info->IsBirthdayToday()))
            continue;
        outIndices.push_back(i);
    }
}

} /* namespace six_seven */
