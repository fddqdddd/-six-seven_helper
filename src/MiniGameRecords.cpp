#include "../include/MiniGameRecords.h"
#include "../include/Util.h"
#include "../mini-games_config.h"

#include <windows.h>

namespace six_seven {

namespace {

std::wstring SectionName(const char* gameId) { return Utf8ToWide(gameId); }

const wchar_t* ModeKey(bool hardMode) { return hardMode ? L"hard" : L"normal"; }

/* Старый формат [records] click_six_seven / click_six_seven_hard */
void MigrateLegacyKey(const std::wstring& ini, const char* gameId, bool hardMode)
{
    std::string legacy = gameId;
    if (hardMode)
        legacy += "_hard";
    const std::wstring legacyKey = Utf8ToWide(legacy.c_str());
    const int legacyVal = GetPrivateProfileIntW(L"records", legacyKey.c_str(), -1, ini.c_str());
    if (legacyVal < 0)
        return;
    const int current =
        GetPrivateProfileIntW(SectionName(gameId).c_str(), ModeKey(hardMode), 0, ini.c_str());
    if (legacyVal > current) {
        wchar_t buf[32];
        wsprintfW(buf, L"%d", legacyVal);
        WritePrivateProfileStringW(SectionName(gameId).c_str(), ModeKey(hardMode), buf, ini.c_str());
    }
}

void EnsureGameSection(const std::wstring& ini, const char* gameId)
{
    const std::wstring sec = SectionName(gameId);
    if (GetPrivateProfileIntW(sec.c_str(), L"normal", -1, ini.c_str()) < 0)
        WritePrivateProfileStringW(sec.c_str(), L"normal", L"0", ini.c_str());
    if (GetPrivateProfileIntW(sec.c_str(), L"hard", -1, ini.c_str()) < 0)
        WritePrivateProfileStringW(sec.c_str(), L"hard", L"0", ini.c_str());
}

} /* namespace */

std::wstring MiniGameRecords::IniPath() const
{
    return PathJoin(GetExeDirectory(), Utf8ToWide(MINIGAME_RECORDS_INI));
}

void MiniGameRecords::Load()
{
    const std::wstring ini = IniPath();
    if (!FileExists(ini)) {
        const std::wstring example =
            PathJoin(GetExeDirectory(), Utf8ToWide("mini_games_records.ini.example"));
        if (FileExists(example))
            CopyFileW(example.c_str(), ini.c_str(), FALSE);
        else {
            EnsureGameSection(ini, MINIGAME_CLICK_ID);
            EnsureGameSection(ini, MINIGAME_MEMORY_ID);
        }
    } else {
        EnsureGameSection(ini, MINIGAME_CLICK_ID);
        EnsureGameSection(ini, MINIGAME_MEMORY_ID);
        MigrateLegacyKey(ini, MINIGAME_CLICK_ID, false);
        MigrateLegacyKey(ini, MINIGAME_CLICK_ID, true);
        MigrateLegacyKey(ini, MINIGAME_MEMORY_ID, false);
        MigrateLegacyKey(ini, MINIGAME_MEMORY_ID, true);
    }
}

int MiniGameRecords::Get(const char* gameId, bool hardMode) const
{
    const std::wstring ini = IniPath();
    return GetPrivateProfileIntW(SectionName(gameId).c_str(), ModeKey(hardMode), 0, ini.c_str());
}

bool MiniGameRecords::TrySave(const char* gameId, bool hardMode, int score)
{
    if (score <= 0)
        return false;
    const int prev = Get(gameId, hardMode);
    if (score <= prev)
        return false;
    const std::wstring ini = IniPath();
    EnsureGameSection(ini, gameId);
    wchar_t buf[32];
    wsprintfW(buf, L"%d", score);
    WritePrivateProfileStringW(SectionName(gameId).c_str(), ModeKey(hardMode), buf, ini.c_str());
    return true;
}

} /* namespace six_seven */
