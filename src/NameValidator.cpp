#include "../include/NameValidator.h"

#include <cwctype>
#include <string>
namespace six_seven {

namespace {

std::wstring TrimName(const std::wstring& name)
{
    size_t begin = 0;
    size_t end = name.size();
    while (begin < end && std::iswspace(static_cast<unsigned short>(name[begin])))
        ++begin;
    while (end > begin && std::iswspace(static_cast<unsigned short>(name[end - 1])))
        --end;
    return name.substr(begin, end - begin);
}

std::wstring NormalizeCompact(const std::wstring& name)
{
    std::wstring out;
    out.reserve(name.size());
    for (wchar_t ch : name) {
        if (ch == L' ' || ch == L'-' || ch == L'_' || ch == L'\t')
            continue;
        out += static_cast<wchar_t>(std::towlower(static_cast<unsigned short>(ch)));
    }
    return out;
}

std::wstring ToLatinLower(const std::wstring& name)
{
    std::wstring out;
    out.reserve(name.size());
    for (wchar_t ch : name) {
        if (ch == L'ё')
            ch = L'е';
        if (ch == L'Ё')
            ch = L'е';
        out += static_cast<wchar_t>(std::towlower(static_cast<unsigned short>(ch)));
    }
    return out;
}

bool ContainsAsciiWord(const std::wstring& haystack, const wchar_t* needle)
{
    return haystack.find(needle) != std::wstring::npos;
}

bool IsSixSevenVariant(const std::wstring& compact)
{
    if (compact == L"67")
        return true;
    return compact == L"sixseven";
}

bool HasAnyDigit(const std::wstring& name)
{
    for (wchar_t ch : name) {
        if (std::iswdigit(static_cast<unsigned short>(ch)))
            return true;
    }
    return false;
}

bool ContainsBannedWord(const std::wstring& latinLower)
{
    static const wchar_t* kBanned[] = {
        /* русский */
        L"хуй", L"хуя", L"хуе", L"хуи", L"хуё", L"пизд", L"бля", L"бляд", L"сука", L"сучк",
        L"еба", L"ёба", L"йеб", L"уеб", L"нах", L"говн", L"жоп", L"муда", L"мудил", L"долбо",
        L"пидор", L"пидр", L"гандон", L"шлюх", L"срать", L"сран", L"дерьм",
        /* english */
        L"fuck", L"shit", L"bitch", L"asshole", L"cunt", L"dick", L"piss", L"whore", L"slut",
        L"nigger", L"nigga", L"faggot", L"retard",
        /* транслит */
        L"xui", L"hui", L"pizd", L"blyat", L"blyad", L"bljat", L"suka", L"suchk",
        L"ebat", L"ebal", L"yeb", L"nahui", L"nahuy", L"govno", L"gopa", L"zhop", L"muda",
        L"pidor", L"pidr", L"shluh", L"dermo", L"fak",
    };
    for (const wchar_t* word : kBanned) {
        if (ContainsAsciiWord(latinLower, word))
            return true;
    }
    return false;
}

} /* namespace */

NameValidationResult ValidateUserName(const std::wstring& rawName)
{
    const std::wstring name = TrimName(rawName);
    if (name.empty())
        return NameValidationResult::HasDigits;

    const std::wstring compact = NormalizeCompact(name);
    if (IsSixSevenVariant(compact))
        return NameValidationResult::SixSevenName;

    if (HasAnyDigit(name))
        return NameValidationResult::HasDigits;

    const std::wstring latin = ToLatinLower(name);
    if (ContainsBannedWord(latin))
        return NameValidationResult::Profanity;

    return NameValidationResult::Ok;
}

const wchar_t* NameRejectionPhrase(NameValidationResult result)
{
    switch (result) {
    case NameValidationResult::SixSevenName:
        return L"Ха, ха, не смеши! Six_Seven — это я, а не ты!";
    case NameValidationResult::HasDigits:
        return L"Человека не могут так называть. Попробуй ввести по-другому...";
    case NameValidationResult::Profanity:
        return L"Ой, нет. Так не пойдёт... Плохие слова я знаю, и мне не нравится, что ты "
               L"назвал себя так.";
    default:
        return L"";
    }
}

} /* namespace six_seven */
