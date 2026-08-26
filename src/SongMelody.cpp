#include "../include/SongMelody.h"
#include "../include/RussianSyllables.h"
#include "../config.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cwctype>
#include <string>

namespace six_seven {

namespace {

std::wstring XmlEscape(const std::wstring& text)
{
    std::wstring out;
    out.reserve(text.size());
    for (wchar_t ch : text) {
        switch (ch) {
        case L'&':
            out += L"&amp;";
            break;
        case L'<':
            out += L"&lt;";
            break;
        case L'>':
            out += L"&gt;";
            break;
        case L'"':
            out += L"&quot;";
            break;
        case L'\'':
            out += L"&apos;";
            break;
        default:
            out += ch;
            break;
        }
    }
    return out;
}

bool IsPunctuationToken(const std::wstring& word)
{
    return word.size() == 1 && (word[0] == L',' || word[0] == L'.' || word[0] == L'!' ||
                                word[0] == L'?' || word[0] == L';' || word[0] == L'…' ||
                                word[0] == L'—' || word[0] == L'-');
}

int SemitonesToAbsMiddleClamped(int semitones)
{
    if (semitones > 10)
        return 10;
    if (semitones < -10)
        return -10;
    return semitones;
}

int SemitonesToAbsMiddle(int semitone, int minSemi, int maxSemi)
{
    if (maxSemi <= minSemi)
        return SemitonesToAbsMiddleClamped(semitone);
    const double t =
        (static_cast<double>(semitone - minSemi) / static_cast<double>(maxSemi - minSemi));
    const int scaled = static_cast<int>(std::round(t * 20.0 - 10.0));
    if (scaled > 10)
        return 10;
    if (scaled < -10)
        return -10;
    return scaled;
}

int SemitoneDeltaToMiddle(int delta)
{
    if (delta > 5)
        delta = 5;
    if (delta < -5)
        delta = -5;
    return delta;
}

void MelodySemitoneRange(const SongMelodySpec& melody, int& outMin, int& outMax)
{
    outMin = 0;
    outMax = 0;
    if (melody.semitones.empty())
        return;
    outMin = outMax = melody.semitones.front();
    for (int s : melody.semitones) {
        outMin = std::min(outMin, s);
        outMax = std::max(outMax, s);
    }
}

int PickSemitone(const SongMelodySpec& melody, int wordIndex)
{
    static const int kDefaultScale[] = { 0, 2, 4, 5, 7, 5, 4, 2, 0, -1, 0, 2 };
    if (!melody.semitones.empty())
        return melody.semitones[static_cast<size_t>(wordIndex) % melody.semitones.size()];
    return kDefaultScale[wordIndex % 12];
}

int PickBreakMs(const SongMelodySpec& melody, int unitIndex, wchar_t punct)
{
    if (punct == L'.' || punct == L'!' || punct == L'?')
        return SIX_SEVEN_TTS_SING_BREAK_SENTENCE_MS;
    if (punct == L',')
        return SIX_SEVEN_TTS_SING_BREAK_COMMA_MS;
    if (!melody.break_ms.empty())
        return melody.break_ms[static_cast<size_t>(unitIndex) % melody.break_ms.size()];
    return melody.default_break_ms > 0 ? melody.default_break_ms
                                       : SIX_SEVEN_TTS_SING_BREAK_WORD_MS;
}

} /* namespace */

bool ParseNoteName(const std::string& name, int& outMidi)
{
    if (name.empty())
        return false;
    size_t i = 0;
    const char letter = static_cast<char>(std::toupper(static_cast<unsigned char>(name[i++])));
    int semitoneInOctave = -1;
    switch (letter) {
    case 'C':
        semitoneInOctave = 0;
        break;
    case 'D':
        semitoneInOctave = 2;
        break;
    case 'E':
        semitoneInOctave = 4;
        break;
    case 'F':
        semitoneInOctave = 5;
        break;
    case 'G':
        semitoneInOctave = 7;
        break;
    case 'A':
        semitoneInOctave = 9;
        break;
    case 'B':
        semitoneInOctave = 11;
        break;
    default:
        return false;
    }
    if (i < name.size() && name[i] == '#') {
        ++semitoneInOctave;
        ++i;
    } else if (i < name.size() && (name[i] == 'b' || name[i] == 'B')) {
        --semitoneInOctave;
        ++i;
    }
    int octave = 4;
    if (i < name.size() && std::isdigit(static_cast<unsigned char>(name[i])))
        octave = name[i++] - '0';
    if (semitoneInOctave < 0)
        semitoneInOctave += 12;
    if (semitoneInOctave >= 12)
        semitoneInOctave -= 12;
    outMidi = (octave + 1) * 12 + semitoneInOctave;
    return true;
}

SongMelodySpec BuildMelodyFromTimedNotes(const std::wstring& lyrics,
                                         const std::vector<SongNoteEvent>& events,
                                         int refMidi, int defaultBreakMs)
{
    SongMelodySpec spec;
    spec.default_break_ms = defaultBreakMs > 0 ? defaultBreakMs : 45;
    if (events.empty())
        return spec;

    std::vector<SongNoteEvent> normalized = events;
    for (SongNoteEvent& ev : normalized)
        ev.semitone -= refMidi;

    const auto words = TokenizeSongWords(lyrics);
    size_t noteIdx = 0;
    for (const std::wstring& w : words) {
        if (IsPunctuationToken(w))
            continue;
        const auto syllables = SplitRussianSyllables(w);
        for (size_t s = 0; s < syllables.size(); ++s) {
            const size_t idx = noteIdx % normalized.size();
            spec.semitones.push_back(normalized[idx].semitone);
            ++noteIdx;
        }
    }
    if (spec.semitones.empty())
        spec.semitones.push_back(0);
    spec.break_ms.push_back(spec.default_break_ms);
    return spec;
}

std::vector<std::wstring> TokenizeSongWords(const std::wstring& text)
{
    std::vector<std::wstring> words;
    std::wstring current;
    for (wchar_t ch : text) {
        if (std::iswspace(ch)) {
            if (!current.empty()) {
                words.push_back(current);
                current.clear();
            }
            continue;
        }
        if (ch == L',' || ch == L'.' || ch == L'!' || ch == L'?' || ch == L';' || ch == L'…' ||
            ch == L'—') {
            if (!current.empty()) {
                words.push_back(current);
                current.clear();
            }
            words.push_back(std::wstring(1, ch));
            continue;
        }
        if (ch == L'-') {
            if (!current.empty()) {
                words.push_back(current);
                current.clear();
            }
            words.push_back(std::wstring(1, ch));
            continue;
        }
        current += ch;
    }
    if (!current.empty())
        words.push_back(current);
    return words;
}

std::wstring BuildSongSsml(const std::wstring& text, const SongMelodySpec& melody)
{
    const auto words = TokenizeSongWords(text);
    int minSemi = 0;
    int maxSemi = 0;
    MelodySemitoneRange(melody, minSemi, maxSemi);

    std::wstring markup;
    markup.reserve(text.size() * 4);
    int wordIndex = 0;
    int prevSemi = 0;
    bool havePrev = false;
    for (const std::wstring& word : words) {
        if (IsPunctuationToken(word)) {
            markup += L"<silence msec=\"";
            markup += std::to_wstring(PickBreakMs(melody, wordIndex, word[0]));
            markup += L"\"/>";
            continue;
        }
        const int semi = PickSemitone(melody, wordIndex);
        if (!havePrev) {
            markup += L"<pitch absmiddle=\"";
            markup += std::to_wstring(SemitonesToAbsMiddle(semi, minSemi, maxSemi));
            markup += L"\">";
            havePrev = true;
            prevSemi = semi;
        } else {
            const int delta = semi - prevSemi;
            markup += L"<pitch middle=\"";
            markup += std::to_wstring(SemitoneDeltaToMiddle(delta));
            markup += L"\">";
            prevSemi = semi;
        }
        markup += XmlEscape(word);
        markup += L"</pitch>";
        markup += L"<silence msec=\"";
        markup += std::to_wstring(PickBreakMs(melody, wordIndex, 0));
        markup += L"\"/>";
        ++wordIndex;
    }
    return markup;
}

std::vector<SongSpeakChunk> BuildSongSpeakChunks(const std::wstring& text,
                                               const SongMelodySpec& melody)
{
    std::vector<SongSpeakChunk> chunks;
    const auto words = TokenizeSongWords(text);
    if (words.empty())
        return chunks;

    int minSemi = 0;
    int maxSemi = 0;
    MelodySemitoneRange(melody, minSemi, maxSemi);

    int syllableIndex = 0;
    int wordIndex = 0;
    int prevSemi = 0;
    bool havePrev = false;
    size_t scan = 0;

    for (const std::wstring& word : words) {
        while (scan < text.size() && std::iswspace(text[scan]))
            ++scan;
        if (scan >= text.size())
            break;

        const size_t wordStartScan = scan;

        if (IsPunctuationToken(word)) {
            if (text[scan] == word[0])
                scan += word.size();
            SongSpeakChunk chunk;
            chunk.reveal_through = scan;
            chunk.markup = L"<silence msec=\"";
            chunk.markup += std::to_wstring(PickBreakMs(melody, wordIndex, word[0]));
            chunk.markup += L"\"/>";
            chunks.push_back(std::move(chunk));
            ++wordIndex;
            continue;
        }

        if (text.compare(scan, word.size(), word) == 0)
            scan += word.size();
        else
            scan = std::min(text.size(), scan + word.size());
        const size_t wordEndScan = scan;

        const auto syllables = SplitRussianSyllables(word);
        for (size_t si = 0; si < syllables.size(); ++si) {
            const int semi = PickSemitone(melody, syllableIndex);
            SongSpeakChunk chunk;
            chunk.reveal_through =
                (si + 1 == syllables.size()) ? wordEndScan : wordStartScan;

            std::wstring markup = L"<prosody rate=\"";
            markup += std::to_wstring(SIX_SEVEN_TTS_SING_SYLLABLE_RATE);
            markup += L"\">";
            if (!havePrev) {
                markup += L"<pitch absmiddle=\"";
                markup += std::to_wstring(SemitonesToAbsMiddle(semi, minSemi, maxSemi));
                markup += L"\">";
                havePrev = true;
            } else {
                const int delta = semi - prevSemi;
                markup += L"<pitch middle=\"";
                markup += std::to_wstring(SemitoneDeltaToMiddle(delta));
                markup += L"\">";
            }
            markup += XmlEscape(syllables[si]);
            markup += L"</pitch></prosody>";
            if (si + 1 < syllables.size()) {
                markup += L"<silence msec=\"";
                markup += std::to_wstring(SIX_SEVEN_TTS_SING_BREAK_SYLLABLE_MS);
                markup += L"\"/>";
            } else {
                markup += L"<silence msec=\"";
                markup += std::to_wstring(PickBreakMs(melody, wordIndex, 0));
                markup += L"\"/>";
            }
            chunk.markup = std::move(markup);
            chunks.push_back(std::move(chunk));
            prevSemi = semi;
            ++syllableIndex;
        }
        ++wordIndex;
    }
    return chunks;
}

} /* namespace six_seven */
