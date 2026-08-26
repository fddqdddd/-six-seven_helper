#ifndef SIX_SEVEN_USER_INFORMATION_H
#define SIX_SEVEN_USER_INFORMATION_H

#include <string>

namespace six_seven {

class UserInformation {
public:
    void Load();
    void Save() const;
    bool IsOnboarded() const { return onboarded_; }

    const std::wstring& Name() const { return name_; }
    const std::wstring& FavoriteColor() const { return color_; }
    const std::wstring& FavoriteSeason() const { return favoriteSeason_; }
    const std::wstring& FavoriteFood() const { return favoriteFood_; }
    int BirthdayMonth() const { return birthdayMonth_; }
    int BirthdayDay() const { return birthdayDay_; }
    int BirthdayYear() const { return birthdayYear_; }
    bool HasBirthday() const { return birthdayMonth_ > 0 && birthdayDay_ > 0; }

    void SetName(const std::wstring& name) { name_ = name; }
    void SetFavoriteColor(const std::wstring& color) { color_ = color; }
    void SetFavoriteSeason(const std::wstring& season) { favoriteSeason_ = season; }
    void SetFavoriteFood(const std::wstring& food) { favoriteFood_ = food; }
    void SetBirthday(int month, int day, int year = 0);
    void SetOnboarded(bool value) { onboarded_ = value; }
    void ResetOnboarding();

    bool IsAdmin() const { return admin_; }
    void SetAdmin(bool value) { admin_ = value; }

    bool IsTerminalUnlocked() const { return terminalUnlocked_; }
    void SetTerminalUnlocked(bool value) { terminalUnlocked_ = value; }
    bool AreCommandsUnlocked() const { return commandsUnlocked_; }
    void SetCommandsUnlocked(bool value) { commandsUnlocked_ = value; }

    bool IsBirthdayToday() const;
    static bool ParseBirthdayInput(const std::wstring& input, int& month, int& day);
    static bool IsDateToday(int month, int day);
    static std::wstring SeasonFromMonth(int month);
    static int SeasonIndex(const std::wstring& season);
    static std::wstring ColorAssociations(const std::wstring& color);
    static bool IsFavoriteGreen(const std::wstring& color);

private:
    std::wstring name_;
    std::wstring color_;
    std::wstring favoriteSeason_;
    std::wstring favoriteFood_;
    int birthdayMonth_ = 0;
    int birthdayDay_ = 0;
    int birthdayYear_ = 0;
    bool onboarded_ = false;
    bool admin_ = false;
    bool terminalUnlocked_ = false;
    bool commandsUnlocked_ = false;
};

void PersonalizePhrase(std::wstring& text, const UserInformation* info);

} /* namespace six_seven */

#endif
