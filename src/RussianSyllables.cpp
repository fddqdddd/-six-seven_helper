#include "../include/RussianSyllables.h"
#include "../include/SongMelody.h"

#include <cwctype>

namespace six_seven {

namespace {

wchar_t LowerRu(wchar_t ch)
{
    return static_cast<wchar_t>(towlower(ch));
}

} /* namespace */

bool IsRussianVowel(wchar_t ch)
{
    switch (LowerRu(ch)) {
    case L'a':
    case L'e':
    case L'\u0451':
    case L'i':
    case L'o':
    case L'u':
    case L'y':
    case L'\u044d':
    case L'\u044e':
    case L'\u044f':
        return true;
    default:
        return false;
    }
}

std::vector<std::wstring> SplitRussianSyllables(const std::wstring& word)
{
    std::vector<std::wstring> syllables;
    if (word.empty())
        return syllables;

    std::vector<size_t> vowelPos;
    vowelPos.reserve(word.size());
    for (size_t i = 0; i < word.size(); ++i) {
        if (IsRussianVowel(word[i]))
            vowelPos.push_back(i);
    }

    if (vowelPos.empty()) {
        syllables.push_back(word);
        return syllables;
    }

    size_t start = 0;
    for (size_t v = 0; v < vowelPos.size(); ++v) {
        const size_t vowelIdx = vowelPos[v];
        size_t end = word.size();
        if (v + 1 < vowelPos.size()) {
            const size_t betweenStart = vowelIdx + 1;
            const size_t betweenEnd = vowelPos[v + 1];
            const size_t betweenLen = betweenEnd - betweenStart;
            if (betweenLen == 0)
                end = vowelIdx + 1;
            else if (betweenLen == 1)
                end = vowelIdx + 1;
            else
                end = vowelIdx + 1 + 1;
        }
        if (end > word.size())
            end = word.size();
        if (end > start)
            syllables.push_back(word.substr(start, end - start));
        start = end;
    }
    if (start < word.size())
        syllables.push_back(word.substr(start));

    if (syllables.empty())
        syllables.push_back(word);
    return syllables;
}

int CountRussianSyllablesInText(const std::wstring& text)
{
    int count = 0;
    for (const std::wstring& word : TokenizeSongWords(text)) {
        if (word.size() == 1 && (word[0] == L',' || word[0] == L'.' || word[0] == L'!' ||
                                 word[0] == L'?' || word[0] == L';' || word[0] == L'…' ||
                                 word[0] == L'—' || word[0] == L'-'))
            continue;
        count += static_cast<int>(SplitRussianSyllables(word).size());
    }
    return count;
}

} /* namespace six_seven */
