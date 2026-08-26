#ifndef SIX_SEVEN_SONG_CATALOG_H
#define SIX_SEVEN_SONG_CATALOG_H

#include "Songs.h"

#include <vector>

namespace six_seven {

bool LoadSongCatalog();
void ShutdownSongCatalog();

const std::vector<SongDef>& GetSongs();
int GetSongCount();

} /* namespace six_seven */

#endif
