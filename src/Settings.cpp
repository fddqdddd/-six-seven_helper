#include "../include/BootSplash.h"
#include "../include/Settings.h"
#include "../include/Util.h"
#include "../config.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <fstream>

namespace six_seven {

namespace {

void WriteDefaultIni(const std::wstring& path)
{
    std::ofstream out(WideToUtf8(path.c_str()), std::ios::binary);
    if (!out)
        return;
    out << u8"; ============================================================\r\n"
        u8"; Six_Seven — настройки\r\n"
        u8"; Файл: six_seven.ini (рядом с Six_Seven.exe)\r\n"
        u8"; После изменений перезапустите программу.\r\n"
        u8"; Строки с «;» в начале — комментарии, программа их не читает.\r\n"
        u8"; ============================================================\r\n"
        u8"\r\n"
        u8"[idle]\r\n"
        u8"; Анимация дыхания в покое (кадры в assets/sprites/idle/).\r\n"
        u8"; 1 = включено, 0 = выключено (статичная картинка).\r\n"
        u8"breath=1\r\n"
        u8"\r\n"
        u8"[def]\r\n"
        u8"; «Скука»: если пользователь долго не трогает персонажа,\r\n"
        u8"; он сам что-то делает — моргает, говорит, уходит прогуляться.\r\n"
        u8"; Задержка в секундах; реальное время случайно между min и max.\r\n"
        u8"; Сбрасывается при перетаскивании, клике и речи.\r\n"
        u8"delay_min_sec=90\r\n"
        u8"delay_max_sec=180\r\n"
        u8"; Сколько секунд идти после фразы «пойду прогуляюсь» (ходьба + отскок от краёв).\r\n"
        u8"wander_walk_sec=12\r\n"
        u8"\r\n"
        u8"[window]\r\n"
        u8"; Позиция окна на экране в пикселях.\r\n"
        u8"; -1 = случайное место при каждом запуске.\r\n"
        u8"; При выходе программа сама сохраняет сюда текущую позицию.\r\n"
        u8"x=-1\r\n"
        u8"y=-1\r\n"
        u8"\r\n"
        u8"[audio]\r\n"
        u8"; Звук и озвучка (SAPI).\r\n"
        u8"; 1 = без звука, 0 = звук включён.\r\n"
        u8"mute=0\r\n"
        u8"\r\n"
        u8"[boot]\r\n"
        u8"; Логотип при включении ПК — assets/sprites/loading/ (трей → установить).\r\n"
        u8"enabled=0\r\n"
        u8"autostart=1\r\n"
        u8"min_sec=4\r\n"
        u8"\r\n"
        u8"[bubble]\r\n"
        u8"; Максимум строк в облачке речи; при переполнении текст листается вверх.\r\n"
        u8"max_lines=6\r\n"
        u8"\r\n"
        u8"[ai]\r\n"
        u8"; Настроение и «личность» (0–100).\r\n"
        u8"; Программа обновляет его сама по времени суток и праздникам.\r\n"
        u8"mood=65\r\n"
        u8"; Злость (0–100) — накапливается, когда над 67 издеваются.\r\n"
        u8"anger=0\r\n"
        u8"; Фрагменты ключа Vault (0–5): выигрывай мини-игры.\r\n"
        u8"vault_fragments=0\r\n";
}

int ReadInt(const wchar_t* section, const wchar_t* key, int defaultValue, const wchar_t* ini)
{
    return GetPrivateProfileIntW(section, key, defaultValue, ini);
}

bool IniKeyExists(const wchar_t* section, const wchar_t* key, const wchar_t* ini)
{
    wchar_t buf[8] = {};
    const DWORD n =
        GetPrivateProfileStringW(section, key, L"", buf, static_cast<DWORD>(std::size(buf)), ini);
    return n > 0;
}

void EnsureIniKey(const std::wstring& ini, const wchar_t* section, const wchar_t* key, int value)
{
    if (IniKeyExists(section, key, ini.c_str()))
        return;
    wchar_t buf[32];
    wsprintfW(buf, L"%d", value);
    WritePrivateProfileStringW(section, key, buf, ini.c_str());
}

} /* namespace */

void Settings::EnsureDefaultIni(const std::wstring& iniPath)
{
    if (FileExists(iniPath))
        return;
    const std::wstring example =
        PathJoin(GetExeDirectory(), L"six_seven.ini.example");
    if (FileExists(example))
        CopyFileW(example.c_str(), iniPath.c_str(), FALSE);
    else
        WriteDefaultIni(iniPath);
}

void Settings::Load(AppSettings& out)
{
    const std::wstring ini = PathJoin(GetExeDirectory(), L"six_seven.ini");
    EnsureDefaultIni(ini);

    const int defaultMinSec = static_cast<int>(SIX_SEVEN_DEF_MIN_MS / 1000);
    const int defaultMaxSec = static_cast<int>(SIX_SEVEN_DEF_MAX_MS / 1000);
    EnsureIniKey(ini, L"def", L"delay_min_sec", defaultMinSec);
    EnsureIniKey(ini, L"def", L"delay_max_sec", defaultMaxSec);
    EnsureIniKey(ini, L"def", L"wander_walk_sec", SIX_SEVEN_WANDER_WALK_SEC);
    EnsureIniKey(ini, L"idle", L"breath", SIX_SEVEN_IDLE_BREATH_DEFAULT);
    EnsureIniKey(ini, L"boot", L"enabled", SIX_SEVEN_BOOT_SPLASH_DEFAULT);
    EnsureIniKey(ini, L"boot", L"autostart", SIX_SEVEN_BOOT_AUTOSTART_DEFAULT);
    EnsureIniKey(ini, L"boot", L"min_sec", SIX_SEVEN_BOOT_SPLASH_MIN_SEC);
    EnsureIniKey(ini, L"bubble", L"max_lines", SIX_SEVEN_BUBBLE_MAX_LINES_DEFAULT);

    out.x = ReadInt(L"window", L"x", -1, ini.c_str());
    out.y = ReadInt(L"window", L"y", -1, ini.c_str());
    out.mute = ReadInt(L"audio", L"mute", 0, ini.c_str()) != 0;
    out.idleBreath =
        ReadInt(L"idle", L"breath", SIX_SEVEN_IDLE_BREATH_DEFAULT, ini.c_str()) != 0;

    out.defDelayMinSec = ReadInt(L"def", L"delay_min_sec", defaultMinSec, ini.c_str());
    out.defDelayMaxSec = ReadInt(L"def", L"delay_max_sec", defaultMaxSec, ini.c_str());
    if (out.defDelayMinSec < 5)
        out.defDelayMinSec = 5;
    if (out.defDelayMaxSec < out.defDelayMinSec)
        out.defDelayMaxSec = out.defDelayMinSec;
    out.wanderWalkSec =
        ReadInt(L"def", L"wander_walk_sec", SIX_SEVEN_WANDER_WALK_SEC, ini.c_str());
    if (out.wanderWalkSec < 3)
        out.wanderWalkSec = 3;
    out.bubbleMaxLines = ReadInt(L"bubble", L"max_lines", SIX_SEVEN_BUBBLE_MAX_LINES_DEFAULT,
                                 ini.c_str());
    if (out.bubbleMaxLines < 2)
        out.bubbleMaxLines = 2;
    if (out.bubbleMaxLines > 12)
        out.bubbleMaxLines = 12;

    out.mood = ReadInt(L"ai", L"mood", out.mood, ini.c_str());
    if (out.mood < 0)
        out.mood = 0;
    if (out.mood > 100)
        out.mood = 100;

    out.anger = ReadInt(L"ai", L"anger", out.anger, ini.c_str());
    if (out.anger < 0)
        out.anger = 0;
    if (out.anger > 100)
        out.anger = 100;

    out.vaultFragments = ReadInt(L"ai", L"vault_fragments", out.vaultFragments, ini.c_str());
    if (out.vaultFragments < 0)
        out.vaultFragments = 0;
    if (out.vaultFragments > 5)
        out.vaultFragments = 5;
}

bool Settings::IsAutostartEnabled() const
{
    const std::wstring ini = PathJoin(GetExeDirectory(), L"six_seven.ini");
    return GetPrivateProfileIntW(L"boot", L"autostart", SIX_SEVEN_BOOT_AUTOSTART_DEFAULT,
                                 ini.c_str()) != 0;
}

void Settings::SetAutostart(bool enabled)
{
    const std::wstring ini = PathJoin(GetExeDirectory(), L"six_seven.ini");
    EnsureDefaultIni(ini);
    WritePrivateProfileStringW(L"boot", L"autostart", enabled ? L"1" : L"0", ini.c_str());
#if SIX_SEVEN_BOOT_AUTOSTART_ENABLED
    BootSplash::SyncAutostart(enabled);
#endif
}

void Settings::SaveMood(int mood) const
{
    const std::wstring ini = PathJoin(GetExeDirectory(), L"six_seven.ini");
    if (mood < 0)
        mood = 0;
    if (mood > 100)
        mood = 100;
    wchar_t buf[16];
    wsprintfW(buf, L"%d", mood);
    WritePrivateProfileStringW(L"ai", L"mood", buf, ini.c_str());
}

void Settings::SaveAnger(int anger) const
{
    const std::wstring ini = PathJoin(GetExeDirectory(), L"six_seven.ini");
    if (anger < 0)
        anger = 0;
    if (anger > 100)
        anger = 100;
    wchar_t buf[16];
    wsprintfW(buf, L"%d", anger);
    WritePrivateProfileStringW(L"ai", L"anger", buf, ini.c_str());
}

void Settings::SaveVaultFragments(int fragments) const
{
    const std::wstring ini = PathJoin(GetExeDirectory(), L"six_seven.ini");
    if (fragments < 0)
        fragments = 0;
    if (fragments > 5)
        fragments = 5;
    wchar_t buf[16];
    wsprintfW(buf, L"%d", fragments);
    WritePrivateProfileStringW(L"ai", L"vault_fragments", buf, ini.c_str());
}

void Settings::Save(int x, int y, bool mute, bool idleBreath)
{
    const std::wstring ini = PathJoin(GetExeDirectory(), L"six_seven.ini");
    EnsureDefaultIni(ini);
    wchar_t buf[32];
    wsprintfW(buf, L"%d", x);
    WritePrivateProfileStringW(L"window", L"x", buf, ini.c_str());
    wsprintfW(buf, L"%d", y);
    WritePrivateProfileStringW(L"window", L"y", buf, ini.c_str());
    WritePrivateProfileStringW(L"audio", L"mute", mute ? L"1" : L"0", ini.c_str());
    WritePrivateProfileStringW(L"idle", L"breath", idleBreath ? L"1" : L"0", ini.c_str());
}

} /* namespace six_seven */
