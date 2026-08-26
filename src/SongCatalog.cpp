#include "../include/SongCatalog.h"
#include "../include/SongMelody.h"
#include "../include/Util.h"
#include "../config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

namespace six_seven {

namespace {

std::vector<SongDef> g_Songs;

std::string TrimAscii(const std::string& s)
{
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
        ++b;
    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
        --e;
    return s.substr(b, e - b);
}

bool StartsWith(const std::string& s, const char* prefix)
{
    return s.rfind(prefix, 0) == 0;
}

std::wstring ReadUtf8File(const std::wstring& path)
{
    std::ifstream in(WideToUtf8(path.c_str()), std::ios::binary);
    if (!in)
        return {};
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xEF &&
        static_cast<unsigned char>(bytes[1]) == 0xBB && static_cast<unsigned char>(bytes[2]) == 0xBF)
        bytes.erase(0, 3);
    return Utf8ToWide(bytes.c_str());
}

std::vector<std::string> SplitPipe(const std::string& line)
{
    std::vector<std::string> parts;
    std::string cur;
    for (char ch : line) {
        if (ch == '|') {
            parts.push_back(TrimAscii(cur));
            cur.clear();
        } else {
            cur += ch;
        }
    }
    parts.push_back(TrimAscii(cur));
    return parts;
}

bool ParseNotesFile(const std::wstring& path, std::vector<SongNoteEvent>& events, int& refMidi,
                    int& breakMs)
{
    std::ifstream in(WideToUtf8(path.c_str()));
    if (!in)
        return false;
    refMidi = 60;
    breakMs = 45;
    events.clear();
    std::string line;
    while (std::getline(in, line)) {
        line = TrimAscii(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (StartsWith(line, "ref ")) {
            ParseNoteName(TrimAscii(line.substr(4)), refMidi);
            continue;
        }
        if (StartsWith(line, "break ")) {
            breakMs = std::max(10, std::atoi(line.c_str() + 6));
            continue;
        }
        for (char& ch : line) {
            if (ch == ',')
                ch = '.';
        }
        std::istringstream iss(line);
        double timeSec = 0.0;
        std::string note;
        if (!(iss >> timeSec >> note))
            continue;
        int midi = 0;
        if (!ParseNoteName(note, midi))
            continue;
        SongNoteEvent ev;
        ev.time_sec = timeSec;
        ev.semitone = midi;
        events.push_back(ev);
    }
    std::sort(events.begin(), events.end(),
              [](const SongNoteEvent& a, const SongNoteEvent& b) { return a.time_sec < b.time_sec; });
    return !events.empty();
}

bool LoadSongFolder(const std::wstring& songsRoot, const std::string& folder, SongDef& out)
{
    const std::wstring dir = PathJoin(songsRoot, Utf8ToWide(folder.c_str()));
    const std::wstring lyricsPath = PathJoin(dir, L"lyrics.txt");
    const std::wstring notesPath = PathJoin(dir, L"notes.txt");
    out.lyrics = ReadUtf8File(lyricsPath);
    if (out.lyrics.empty())
        return false;
    std::vector<SongNoteEvent> events;
    int refMidi = 60;
    int breakMs = 45;
    if (!ParseNotesFile(notesPath, events, refMidi, breakMs))
        return false;
    out.melody = BuildMelodyFromTimedNotes(out.lyrics, events, refMidi, breakMs);
    return true;
}

} /* namespace */

bool LoadSongCatalog()
{
    g_Songs.clear();
    const std::wstring manifest = AssetPath(SIX_SEVEN_SONGS_MANIFEST_REL);
    std::ifstream in(WideToUtf8(manifest.c_str()));
    if (!in)
        return false;

    const std::wstring songsRoot = AssetPath(SIX_SEVEN_SONGS_ROOT);
    std::string line;
    while (std::getline(in, line)) {
        line = TrimAscii(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (!StartsWith(line, "SONG "))
            continue;
        const std::vector<std::string> fields = SplitPipe(line.substr(5));
        if (fields.size() < 6)
            continue;
        SongDef song;
        song.id = fields[0];
        song.title = Utf8ToWide(fields[1].c_str());
        song.sprite = fields[2];
        song.sprite_loop = fields[3] == "1";
        song.birthday_only = fields[4] == "1";
        if (!LoadSongFolder(songsRoot, fields[5], song))
            continue;
        g_Songs.push_back(std::move(song));
    }
    return !g_Songs.empty();
}

void ShutdownSongCatalog()
{
    g_Songs.clear();
}

const std::vector<SongDef>& GetSongs()
{
    return g_Songs;
}

int GetSongCount()
{
    return static_cast<int>(g_Songs.size());
}

} /* namespace six_seven */
