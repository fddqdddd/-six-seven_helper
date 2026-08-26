#ifndef SIX_SEVEN_RUSSIAN_SYLLABLES_H
#define SIX_SEVEN_RUSSIAN_SYLLABLES_H

#include <string>
#include <vector>

namespace six_seven {

bool IsRussianVowel(wchar_t ch);
std::vector<std::wstring> SplitRussianSyllables(const std::wstring& word);
int CountRussianSyllablesInText(const std::wstring& text);

} /* namespace six_seven */

#endif
