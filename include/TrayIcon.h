#ifndef SIX_SEVEN_TRAY_ICON_H
#define SIX_SEVEN_TRAY_ICON_H

#include <functional>
#include <windows.h>

namespace six_seven {

class TrayIcon {
public:
    using MenuHandler = std::function<void(int cmd)>;

    bool Create(HWND hwnd, HINSTANCE inst);
    void Destroy();
    void ShowMenu(HWND hwnd, const POINT& pt, bool autostartOn, MenuHandler handler);
    static constexpr UINT WM_TRAY = WM_USER + 50;

private:
    NOTIFYICONDATAW nid_ = {};
    bool created_ = false;
};

} /* namespace six_seven */

#endif
