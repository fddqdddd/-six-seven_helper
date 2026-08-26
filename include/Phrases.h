#ifndef SIX_SEVEN_PHRASES_H
#define SIX_SEVEN_PHRASES_H

#include <string>

namespace six_seven {

class UserInformation;

std::wstring ResolvePhrase(const char* phraseFileUtf8, const wchar_t* inlinePhrase,
                           const UserInformation* info = nullptr);
std::wstring LoadRandomLine(const char* phraseFileUtf8);

} /* namespace six_seven */

#endif
