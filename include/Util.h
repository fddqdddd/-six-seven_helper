#ifndef SIX_SEVEN_UTIL_H
#define SIX_SEVEN_UTIL_H

#include "ActionTypes.h"

#include <windows.h>

#include <string>
#include <vector>

namespace six_seven {

std::wstring GetExeDirectory();
std::wstring Utf8ToWide(const char* utf8);
std::string WideToUtf8(const wchar_t* wide);
std::string CaesarShiftEncode(const std::string& text, int shift);
std::string CaesarShiftDecode(const std::string& text, int shift);
std::wstring PathJoin(const std::wstring& a, const std::wstring& b);
std::wstring AssetPath(const char* relativeUtf8);
bool FileExists(const std::wstring& path);
bool WriteTextFile(const std::wstring& path, const std::string& utf8Contents, bool addBom);
int RandomInt(int minInclusive, int maxInclusive);
const SixSevenActionDef& PickRandom(const SixSevenActionDef* arr, int count);

std::wstring BaseName(const std::wstring& path);
std::wstring GetDesktopPath();
std::wstring PickDesktopFolder(int depth);
std::vector<std::wstring> ListSubdirectories(const std::wstring& dir);
std::vector<std::wstring> ListFilesInDirectory(const std::wstring& dir);
std::wstring UniquePathInFolder(const std::wstring& dir, const std::wstring& fileName);
bool IsFileOpenByOtherProcess(const std::wstring& path);
bool WindowTitleContains(const std::wstring& needle);
int CloseEditorWindowsTitled(const std::wstring& needle);

RECT GetCombinedWorkArea();
bool GetWorkAreaAtPoint(const POINT& pt, RECT& outWork);
void ClampWindowToWorkArea(int& x, int& y, int width, int height, const POINT* followPoint);
void RandomSpriteWindowPos(int& winX, int& winY, int marginPx);
void CenterSpriteWindow(int& winX, int& winY);
void CenterWindowOnScreen(HWND hwnd, int width, int height);
void ClampWindowToSpriteWorkArea(int& winX, int& winY, int spriteDrawX, int spriteDrawY,
                                 int spriteW, int spriteH, const POINT* spriteAnchor);

void EnsureAdminPasswordFile();
bool VerifyAdminPassword(const std::wstring& attempt);

} /* namespace six_seven */

#endif
