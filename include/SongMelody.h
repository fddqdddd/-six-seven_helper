#ifndef SIX_SEVEN_SONG_MELODY_H
#define SIX_SEVEN_SONG_MELODY_H

#include <string>
#include <vector>

namespace six_seven {

struct SongNoteEvent {
    double time_sec = 0.0;
    int semitone = 0;
};

struct SongMelodySpec {
    std::vector<int> semitones;
    std::vector<int> break_ms;
    int default_break_ms = 45;
};

struct SongSpeakChunk {
    std::wstring markup;
    size_t reveal_through = 0;
};

bool ParseNoteName(const std::string& name, int& outMidi);
SongMelodySpec BuildMelodyFromTimedNotes(const std::wstring& lyrics,
                                         const std::vector<SongNoteEvent>& events,
                                         int refMidi, int defaultBreakMs);

std::vector<std::wstring> TokenizeSongWords(const std::wstring& text);
std::wstring BuildSongSsml(const std::wstring& text, const SongMelodySpec& melody);
std::vector<SongSpeakChunk> BuildSongSpeakChunks(const std::wstring& text,
                                                 const SongMelodySpec& melody);

} /* namespace six_seven */

#endif
