#ifndef SIX_SEVEN_TERMINAL_GUARD_H
#define SIX_SEVEN_TERMINAL_GUARD_H

#include <functional>
#include <set>
#include <string>
#include <windows.h>

namespace six_seven {

class UserInformation;

class TerminalGuard {
public:
    using VoidCallback = std::function<void()>;
    using CommandCallback = std::function<void(const std::wstring&)>;

    void Bind(UserInformation* userInfo, VoidCallback onBlockTerminal,
              VoidCallback onUserTerminalOpened, CommandCallback onCommand);

    void Tick();
    void AllowAppTerminal(unsigned count = 1);

    static bool IsTerminalWindow(HWND hwnd);

private:
    struct TerminalWindowInfo {
        HWND hwnd = nullptr;
        DWORD pid = 0;
    };

    void ScanTerminals();
    void CloseTerminal(HWND hwnd, DWORD pid);
    void PollCommands();
    bool ReadConsoleText(DWORD pid, std::wstring& text) const;
    void TryExecuteCommandFromText(const std::wstring& text, DWORD pid);
    static BOOL CALLBACK EnumTerminalProc(HWND hwnd, LPARAM lp);

    UserInformation* userInfo_ = nullptr;
    VoidCallback onBlockTerminal_;
    VoidCallback onUserTerminalOpened_;
    CommandCallback onCommand_;

    unsigned appTerminalAllowance_ = 0;
    bool userTerminalGreeted_ = false;
    std::set<DWORD> knownTerminalPids_;
    std::set<std::wstring> executedCommandKeys_;
};

} /* namespace six_seven */

#endif
