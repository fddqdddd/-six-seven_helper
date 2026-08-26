#include "../include/Phrases.h"
#include "../include/UserInformation.h"
#include "../include/Util.h"

#include <fstream>
#include <vector>

namespace six_seven {

std::wstring LoadRandomLine(const char* phraseFileUtf8)
{
    if (!phraseFileUtf8 || !*phraseFileUtf8)
        return {};
    const std::wstring path = AssetPath(phraseFileUtf8);
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in)
        return {};
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (!line.empty())
            lines.push_back(line);
    }
    if (lines.empty())
        return {};
    return Utf8ToWide(lines[RandomInt(0, static_cast<int>(lines.size()) - 1)].c_str());
}

std::wstring ResolvePhrase(const char* phraseFileUtf8, const wchar_t* inlinePhrase,
                           const UserInformation* info)
{
    std::wstring text;
    if (inlinePhrase && *inlinePhrase)
        text = inlinePhrase;
    else
        text = LoadRandomLine(phraseFileUtf8);
    PersonalizePhrase(text, info);
    return text;
}

} /* namespace six_seven */
