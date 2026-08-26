#include "../include/TrayIcon.h"
#include "../config.h"
#include "../include/Util.h"

namespace six_seven {

bool TrayIcon::Create(HWND hwnd, HINSTANCE inst)
{
    nid_ = {};
    nid_.cbSize = sizeof(nid_);
    nid_.hWnd = hwnd;
    nid_.uID = 1;
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid_.uCallbackMessage = WM_TRAY;
    nid_.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    const std::wstring iconPath = AssetPath(SIX_SEVEN_ICON_MAIN);
    if (FileExists(iconPath))
        nid_.hIcon = static_cast<HICON>(
            LoadImageW(nullptr, iconPath.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE));
    lstrcpynW(nid_.szTip, SIX_SEVEN_TRAY_TIP, static_cast<int>(sizeof(nid_.szTip) / sizeof(wchar_t)));
    created_ = Shell_NotifyIconW(NIM_ADD, &nid_) != FALSE;
    (void)inst;
    return created_;
}

void TrayIcon::Destroy()
{
    if (created_) {
        Shell_NotifyIconW(NIM_DELETE, &nid_);
        created_ = false;
    }
}

void TrayIcon::ShowMenu(HWND hwnd, const POINT& pt, bool autostartOn, MenuHandler handler)
{
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1001, L"Показать / Скрыть");
    AppendMenuW(menu, MF_STRING, 1002, L"Без звука");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 1006,
                autostartOn ? L"Автозапуск: вкл" : L"Автозапуск: выкл");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 1003, L"Выход");
    SetForegroundWindow(hwnd);
    const UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
    PostMessageW(hwnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
    if (cmd && handler)
        handler(static_cast<int>(cmd));
}

} /* namespace six_seven */
