#include "../include/TerminalGuard.h"

#include "../include/UserInformation.h"

#include <algorithm>
#include <cctype>
#include <vector>

namespace six_seven {

namespace {

constexpr wchar_t kConsoleClass[] = L"ConsoleWindowClass";
constexpr wchar_t kWtClass[] = L"CASCADIA_HOSTING_WINDOW_CLASS";

std::wstring ToLower(std::wstring s)
{
    for (auto& ch : s)
        ch = static_cast<wchar_t>(towlower(ch));
    return s;
}

std::wstring TrimWide(const std::wstring& s)
{
    size_t b = 0;
    while (b < s.size() && iswspace(s[b]))
        ++b;
    size_t e = s.size();
    while (e > b && iswspace(s[e - 1]))
        --e;
    return s.substr(b, e - b);
}

bool IsOurProcess(DWORD pid)
{
    return pid == GetCurrentProcessId();
}

bool IsTerminalProcessName(const wchar_t* name)
{
    if (!name || !*name)
        return false;
    const std::wstring lower = ToLower(name);
    return lower == L"cmd.exe" || lower == L"powershell.exe" || lower == L"pwsh.exe" ||
           lower == L"windowsterminal.exe" || lower == L"wt.exe" || lower == L"conhost.exe";
}

bool GetProcessImageName(DWORD pid, std::wstring& out)
{
    out.clear();
    HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!proc)
        return false;
    wchar_t path[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    const bool ok = QueryFullProcessImageNameW(proc, 0, path, &size) != 0;
    CloseHandle(proc);
    if (!ok)
        return false;
    const wchar_t* slash = wcsrchr(path, L'\\');
    out = slash ? slash + 1 : path;
    return true;
}

bool IsTerminalPid(DWORD pid)
{
    if (!pid || IsOurProcess(pid))
        return false;
    std::wstring name;
    if (!GetProcessImageName(pid, name))
        return false;
    return IsTerminalProcessName(name.c_str());
}

} /* namespace */

void TerminalGuard::Bind(UserInformation* userInfo, VoidCallback onBlockTerminal,
                         VoidCallback onUserTerminalOpened, CommandCallback onCommand)
{
    userInfo_ = userInfo;
    onBlockTerminal_ = std::move(onBlockTerminal);
    onUserTerminalOpened_ = std::move(onUserTerminalOpened);
    onCommand_ = std::move(onCommand);
    if (userInfo_ && userInfo_->AreCommandsUnlocked())
        userTerminalGreeted_ = true;
}

void TerminalGuard::AllowAppTerminal(unsigned count)
{
    appTerminalAllowance_ += count;
}

bool TerminalGuard::IsTerminalWindow(HWND hwnd)
{
    if (!hwnd || !IsWindowVisible(hwnd))
        return false;
    wchar_t cls[64] = {};
    if (!GetClassNameW(hwnd, cls, 64))
        return false;
    if (wcscmp(cls, kConsoleClass) == 0 || wcscmp(cls, kWtClass) == 0)
        return true;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    return IsTerminalPid(pid);
}

BOOL CALLBACK TerminalGuard::EnumTerminalProc(HWND hwnd, LPARAM lp)
{
    auto* self = reinterpret_cast<TerminalGuard*>(lp);
    if (!self || !IsTerminalWindow(hwnd))
        return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    self->ScanTerminals();
    return TRUE;
}

void TerminalGuard::ScanTerminals()
{
    if (!userInfo_)
        return;

    std::vector<TerminalWindowInfo> found;
    auto collect = [](HWND hwnd, LPARAM lp) -> BOOL {
        auto* list = reinterpret_cast<std::vector<TerminalWindowInfo>*>(lp);
        if (!IsTerminalWindow(hwnd))
            return TRUE;
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        list->push_back({ hwnd, pid });
        return TRUE;
    };
    EnumWindows(collect, reinterpret_cast<LPARAM>(&found));

    const bool unlocked = userInfo_->IsTerminalUnlocked();

    for (const auto& term : found) {
        if (IsOurProcess(term.pid))
            continue;

        if (appTerminalAllowance_ > 0) {
            --appTerminalAllowance_;
            knownTerminalPids_.insert(term.pid);
            continue;
        }

        if (!unlocked) {
            if (onBlockTerminal_)
                onBlockTerminal_();
            CloseTerminal(term.hwnd, term.pid);
            continue;
        }

        if (IsOurProcess(term.pid))
            continue;

        knownTerminalPids_.insert(term.pid);

        if (!userInfo_->AreCommandsUnlocked() && !userTerminalGreeted_) {
            userTerminalGreeted_ = true;
            if (onUserTerminalOpened_)
                onUserTerminalOpened_();
        }
    }

    if (unlocked && userInfo_->AreCommandsUnlocked())
        PollCommands();
}

void TerminalGuard::CloseTerminal(HWND hwnd, DWORD pid)
{
    if (hwnd && IsWindow(hwnd))
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
    if (!pid)
        return;
    HANDLE proc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!proc)
        return;
    TerminateProcess(proc, 0);
    CloseHandle(proc);
}

bool TerminalGuard::ReadConsoleText(DWORD pid, std::wstring& text) const
{
    text.clear();
    if (!pid || IsOurProcess(pid))
        return false;

    const DWORD selfPid = GetCurrentProcessId();
    FreeConsole();
    if (!AttachConsole(pid)) {
        AttachConsole(selfPid);
        return false;
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!hOut || hOut == INVALID_HANDLE_VALUE) {
        FreeConsole();
        AttachConsole(selfPid);
        return false;
    }

    CONSOLE_SCREEN_BUFFER_INFO info = {};
    if (!GetConsoleScreenBufferInfo(hOut, &info)) {
        FreeConsole();
        AttachConsole(selfPid);
        return false;
    }

    const SHORT width = info.srWindow.Right - info.srWindow.Left + 1;
    const SHORT height = info.srWindow.Bottom - info.srWindow.Top + 1;
    if (width <= 0 || height <= 0) {
        FreeConsole();
        AttachConsole(selfPid);
        return false;
    }

    const DWORD cellCount = static_cast<DWORD>(width) * static_cast<DWORD>(height);
    std::vector<CHAR_INFO> buffer(cellCount);
    COORD bufSize = { width, height };
    COORD bufOrigin = { 0, 0 };
    SMALL_RECT readRect = info.srWindow;
    if (!ReadConsoleOutputW(hOut, buffer.data(), bufSize, bufOrigin, &readRect)) {
        FreeConsole();
        AttachConsole(selfPid);
        return false;
    }

    for (SHORT row = 0; row < height; ++row) {
        std::wstring line;
        line.reserve(static_cast<size_t>(width));
        for (SHORT col = 0; col < width; ++col) {
            const CHAR_INFO& cell = buffer[static_cast<size_t>(row) * width + col];
            line.push_back(cell.Char.UnicodeChar ? cell.Char.UnicodeChar : L' ');
        }
        while (!line.empty() && iswspace(line.back()))
            line.pop_back();
        if (!line.empty()) {
            if (!text.empty())
                text.push_back(L'\n');
            text += line;
        }
    }

    FreeConsole();
    AttachConsole(selfPid);
    return !text.empty();
}

void TerminalGuard::TryExecuteCommandFromText(const std::wstring& text, DWORD pid)
{
    if (!onCommand_)
        return;

    size_t lineStart = 0;
    while (lineStart <= text.size()) {
        size_t lineEnd = text.find(L'\n', lineStart);
        if (lineEnd == std::wstring::npos)
            lineEnd = text.size();
        std::wstring line = TrimWide(text.substr(lineStart, lineEnd - lineStart));
        lineStart = lineEnd + 1;
        if (line.empty())
            continue;

        const std::wstring lower = ToLower(line);
        std::wstring cmd;
        if (lower == L"six-seven_open" || lower.find(L"six-seven_open") != std::wstring::npos)
            cmd = L"six-seven_open";
        else if (lower == L"kill_67" || lower.find(L"kill_67") != std::wstring::npos)
            cmd = L"kill_67";
        else if (lower == L"sleep67" || lower.find(L"sleep67") != std::wstring::npos)
            cmd = L"sleep67";
        else if (lower == L"67move" || lower.find(L"67move") != std::wstring::npos)
            cmd = L"67move";
        else
            continue;

        const std::wstring key = std::to_wstring(pid) + L":" + cmd + L":" + line;
        if (executedCommandKeys_.count(key))
            continue;
        executedCommandKeys_.insert(key);
        onCommand_(cmd);
    }
}

void TerminalGuard::PollCommands()
{
    if (!userInfo_ || !userInfo_->IsTerminalUnlocked() || !userInfo_->AreCommandsUnlocked())
        return;

    std::vector<DWORD> pids;
    auto collect = [](HWND hwnd, LPARAM lp) -> BOOL {
        if (!IsTerminalWindow(hwnd))
            return TRUE;
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        auto* list = reinterpret_cast<std::vector<DWORD>*>(lp);
        if (pid && !IsOurProcess(pid)) {
            bool exists = false;
            for (DWORD existing : *list) {
                if (existing == pid) {
                    exists = true;
                    break;
                }
            }
            if (!exists)
                list->push_back(pid);
        }
        return TRUE;
    };
    EnumWindows(collect, reinterpret_cast<LPARAM>(&pids));

    for (DWORD pid : pids) {
        std::wstring text;
        if (!ReadConsoleText(pid, text))
            continue;
        TryExecuteCommandFromText(text, pid);
    }
}

void TerminalGuard::Tick()
{
    ScanTerminals();
}

} /* namespace six_seven */
