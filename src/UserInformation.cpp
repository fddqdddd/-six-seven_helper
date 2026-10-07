#include "../include/UserInformation.h"
#include "../include/Util.h"
#include "../config.h"

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <secext.h>
#include <windows.h>

#include <algorithm>
#include <cwctype>
#include <vector>

namespace six_seven {

namespace {

constexpr wchar_t kInfoFileName[] = L"information";

void ReplaceToken(std::wstring& text, const wchar_t* token, const std::wstring& value)
{
    if (!token || !*token || value.empty())
        return;
    const std::wstring tok(token);
    size_t pos = 0;
    while ((pos = text.find(tok, pos)) != std::wstring::npos) {
        text.replace(pos, tok.size(), value);
        pos += value.size();
    }
}

} /* namespace */

void UserInformation::Load()
{
    name_.clear();
    color_.clear();
    realName_.clear();
    onboarded_ = false;

    const std::wstring path = PathJoin(GetExeDirectory(), kInfoFileName);
    wchar_t buf[256] = {};

    GetPrivateProfileStringW(L"user", L"name", L"", buf, 256, path.c_str());
    name_ = buf;

    GetPrivateProfileStringW(L"user", L"real_name", L"", buf, 256, path.c_str());
    realName_ = buf;

    GetPrivateProfileStringW(L"user", L"color", L"", buf, 256, path.c_str());
    color_ = buf;

    GetPrivateProfileStringW(L"user", L"season", L"", buf, 256, path.c_str());
    favoriteSeason_ = buf;

    GetPrivateProfileStringW(L"user", L"food", L"", buf, 256, path.c_str());
    favoriteFood_ = buf;

    birthdayMonth_ = GetPrivateProfileIntW(L"user", L"birthday_month", 0, path.c_str());
    birthdayDay_ = GetPrivateProfileIntW(L"user", L"birthday_day", 0, path.c_str());
    birthdayYear_ = GetPrivateProfileIntW(L"user", L"birthday_year", 0, path.c_str());

    onboarded_ = GetPrivateProfileIntW(L"user", L"onboarded", 0, path.c_str()) != 0;
    admin_ = GetPrivateProfileIntW(L"user", L"admin", 0, path.c_str()) != 0;
    terminalUnlocked_ =
        GetPrivateProfileIntW(L"user", L"terminal_unlocked", 0, path.c_str()) != 0;
    commandsUnlocked_ =
        GetPrivateProfileIntW(L"user", L"commands_unlocked", 0, path.c_str()) != 0;
    if (onboarded_ && (name_.empty() || color_.empty() || !HasBirthday()))
        onboarded_ = false;
}

void UserInformation::Save() const
{
    const std::wstring path = PathJoin(GetExeDirectory(), kInfoFileName);
    WritePrivateProfileStringW(L"user", L"name", name_.c_str(), path.c_str());
    WritePrivateProfileStringW(L"user", L"real_name", realName_.c_str(), path.c_str());
    WritePrivateProfileStringW(L"user", L"color", color_.c_str(), path.c_str());
    WritePrivateProfileStringW(L"user", L"season", favoriteSeason_.c_str(), path.c_str());
    WritePrivateProfileStringW(L"user", L"food", favoriteFood_.c_str(), path.c_str());
    wchar_t buf[16] = {};
    wsprintfW(buf, L"%d", birthdayMonth_);
    WritePrivateProfileStringW(L"user", L"birthday_month", buf, path.c_str());
    wsprintfW(buf, L"%d", birthdayDay_);
    WritePrivateProfileStringW(L"user", L"birthday_day", buf, path.c_str());
    wsprintfW(buf, L"%d", birthdayYear_);
    WritePrivateProfileStringW(L"user", L"birthday_year", buf, path.c_str());
    WritePrivateProfileStringW(L"user", L"onboarded", onboarded_ ? L"1" : L"0", path.c_str());
    WritePrivateProfileStringW(L"user", L"admin", admin_ ? L"1" : L"0", path.c_str());
    WritePrivateProfileStringW(L"user", L"terminal_unlocked", terminalUnlocked_ ? L"1" : L"0",
                               path.c_str());
    WritePrivateProfileStringW(L"user", L"commands_unlocked", commandsUnlocked_ ? L"1" : L"0",
                               path.c_str());
}

void UserInformation::SetBirthday(int month, int day, int year)
{
    birthdayMonth_ = month;
    birthdayDay_ = day;
    birthdayYear_ = year;
}

void UserInformation::ResetOnboarding()
{
    name_.clear();
    color_.clear();
    favoriteSeason_.clear();
    favoriteFood_.clear();
    birthdayMonth_ = 0;
    birthdayDay_ = 0;
    birthdayYear_ = 0;
    onboarded_ = false;
}

namespace {

bool IsJunkAccountName(const std::wstring& name)
{
    if (name.empty() || name == L"67")
        return true;
    return _wcsicmp(name.c_str(), L"six_seven") == 0;
}

} /* namespace */

bool UserInformation::ReadSystemRealName()
{
    wchar_t buf[256] = {};
    ULONG size = static_cast<ULONG>(std::size(buf));
    std::wstring got;
    if (GetUserNameExW(NameDisplay, buf, &size) && buf[0])
        got = buf;
    if (IsJunkAccountName(got)) {
        DWORD cb = sizeof(buf);
        buf[0] = L'\0';
        if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                         L"RegisteredOwner", RRF_RT_REG_SZ, nullptr, buf, &cb) == ERROR_SUCCESS &&
            buf[0])
            got = buf;
    }
    if (IsJunkAccountName(got))
        return false;
    realName_ = got;
    return true;
}

bool UserInformation::IsDateToday(int month, int day)
{
    if (month < 1 || month > 12 || day < 1)
        return false;
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    return static_cast<int>(st.wMonth) == month && static_cast<int>(st.wDay) == day;
}

bool UserInformation::IsBirthdayToday() const
{
    return HasBirthday() && IsDateToday(birthdayMonth_, birthdayDay_);
}

namespace {

bool IsLeapYear(int year)
{
    if (year <= 0)
        return true;
    if (year % 400 == 0)
        return true;
    if (year % 100 == 0)
        return false;
    return year % 4 == 0;
}

int DaysInMonth(int month, int year)
{
    static const int kDays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month < 1 || month > 12)
        return 0;
    if (month == 2 && IsLeapYear(year))
        return 29;
    return kDays[month - 1];
}

bool IsValidBirthday(int month, int day, int year)
{
    if (month < 1 || month > 12 || day < 1)
        return false;
    return day <= DaysInMonth(month, year);
}

bool BuildDateFromParts(const std::vector<int>& parts, int& month, int& day)
{
    if (parts.size() < 2)
        return false;

    int d = 0;
    int m = 0;
    int y = 0;
    if (parts.size() >= 3) {
        d = parts[0];
        m = parts[1];
        y = parts[2];
        if (d >= 1000) {
            y = d;
            m = parts[1];
            d = parts[2];
        }
    } else {
        d = parts[0];
        m = parts[1];
    }

    if (!IsValidBirthday(m, d, y))
        return false;
    month = m;
    day = d;
    return true;
}

} /* namespace */

bool UserInformation::ParseBirthdayInput(const std::wstring& input, int& month, int& day)
{
    month = 0;
    day = 0;
    if (input.empty())
        return false;

    std::vector<int> parts;
    std::wstring current;
    for (wchar_t ch : input) {
        if (std::iswdigit(static_cast<unsigned short>(ch))) {
            current += ch;
            continue;
        }
        if (!current.empty()) {
            parts.push_back(_wtoi(current.c_str()));
            current.clear();
        }
    }
    if (!current.empty())
        parts.push_back(_wtoi(current.c_str()));

    if (parts.size() < 2)
        return false;

    if (parts.size() == 2) {
        int a = parts[0];
        int b = parts[1];
        if (a >= 1 && a <= 12 && b >= 1 && b <= 31 && IsValidBirthday(a, b, 0)) {
            month = a;
            day = b;
            return true;
        }
        if (IsValidBirthday(b, a, 0)) {
            month = b;
            day = a;
            return true;
        }
        return false;
    }

    return BuildDateFromParts(parts, month, day);
}

std::wstring UserInformation::SeasonFromMonth(int month)
{
    if (month == 12 || month == 1 || month == 2)
        return L"Зима";
    if (month >= 3 && month <= 5)
        return L"Весна";
    if (month >= 6 && month <= 8)
        return L"Лето";
    if (month >= 9 && month <= 11)
        return L"Осень";
    return L"Лето";
}

int UserInformation::SeasonIndex(const std::wstring& season)
{
    if (season == L"Зима")
        return 0;
    if (season == L"Весна")
        return 1;
    if (season == L"Лето")
        return 2;
    if (season == L"Осень")
        return 3;
    return 2;
}

bool UserInformation::IsFavoriteGreen(const std::wstring& color)
{
    std::wstring c = color;
    for (auto& ch : c)
        ch = static_cast<wchar_t>(towlower(ch));
    return c == L"зелёный" || c == L"зеленый";
}

std::wstring UserInformation::ColorAssociations(const std::wstring& color)
{
    if (color == L"Красный")
        return L"огонь и страсть";
    if (color == L"Оранжевый")
        return L"закат и тёплые вечера";
    if (color == L"Жёлтый")
        return L"солнце и хорошее настроение";
    if (color == L"Зелёный")
        return L"жизнь и травку";
    if (color == L"Голубой")
        return L"небо и свежесть";
    if (color == L"Синий")
        return L"море и глубину";
    if (color == L"Фиолетовый")
        return L"волшебство и мечты";
    if (color == L"Белый")
        return L"чистоту и облака";
    if (color == L"Чёрный")
        return L"ночь и загадочность";
    if (color == L"Радужный")
        return L"праздник и все цвета сразу";
    return L"что-то особенное";
}

void PersonalizePhrase(std::wstring& text, const UserInformation* info, bool useRealName)
{
    if (!info || !info->IsOnboarded())
        return;
    std::wstring name = info->Name();
    if (useRealName && !info->RealName().empty())
        name = info->RealName();
    if (name.empty())
        name = SIX_SEVEN_NICKNAME;
    ReplaceToken(text, L"friend", name);
    ReplaceToken(text, L"color", info->FavoriteColor());
    ReplaceToken(text, L"season", info->FavoriteSeason());
    ReplaceToken(text, L"food", info->FavoriteFood());
}

} /* namespace six_seven */
