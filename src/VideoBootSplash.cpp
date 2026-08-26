#include "../include/VideoBootSplash.h"
#include "../include/Util.h"
#include "../config.h"

#include <mmsystem.h>
#include <string>

#pragma comment(lib, "winmm.lib")

namespace six_seven {

bool VideoBootSplash::classRegistered_ = false;
bool VideoBootSplash::wasShown_ = false;

namespace {

const wchar_t* kRunKey =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t* kRunValueName = L"Six_Seven_VideoBoot";

int ReadBootIniInt(const wchar_t* key, int defaultValue)
{
    const std::wstring ini = PathJoin(GetExeDirectory(), L"six_seven.ini");
    return GetPrivateProfileIntW(L"videoboot", key, defaultValue, ini.c_str());
}

bool BootIniEnabled()
{
    return ReadBootIniInt(L"enabled", 0) != 0;
}

bool IsBootLaunch()
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return false;
    bool boot = false;
    for (int i = 1; i < argc; ++i) {
        if (_wcsicmp(argv[i], L"--boot") == 0) {
            boot = true;
            break;
        }
    }
    LocalFree(argv);
    return boot;
}

MCIERROR MciOpenWav(const wchar_t* path, UINT* outDev)
{
    MCI_OPEN_PARMSW mci = {};
    mci.lpstrDeviceType = L"waveaudio";
    mci.lpstrElementName = path;
    MCIERROR err = mciSendCommandW(0, MCI_OPEN,
        MCI_OPEN_ELEMENT | MCI_OPEN_TYPE, (DWORD_PTR)&mci);
    if (err == 0 && outDev)
        *outDev = mci.wDeviceID;
    return err;
}

MCIERROR MciOpenVideo(const wchar_t* path, UINT* outDev)
{
    MCI_OPEN_PARMSW mci = {};
    mci.lpstrElementName = path;
    MCIERROR err = mciSendCommandW(0, MCI_OPEN,
        MCI_OPEN_ELEMENT, (DWORD_PTR)&mci);
    if (err == 0 && outDev)
        *outDev = mci.wDeviceID;
    return err;
}

DWORD MciGetLength(UINT dev)
{
    MCI_STATUS_PARMS mci = {};
    mci.dwItem = MCI_STATUS_LENGTH;
    if (mciSendCommandW(dev, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&mci))
        return 0;
    return static_cast<DWORD>(mci.dwReturn);
}

} /* namespace */

std::wstring VideoBootSplash::VideoPath()
{
    return PathJoin(GetExeDirectory(), L"assets\\boot\\video.mp4");
}

std::wstring VideoBootSplash::SoundPath()
{
    const std::wstring exeDir = GetExeDirectory();
    const std::wstring wav = PathJoin(exeDir, L"assets\\boot\\sound.wav");
    if (FileExists(wav))
        return wav;
    return PathJoin(exeDir, L"assets\\boot\\sound.mp3");
}

bool VideoBootSplash::ShouldShow()
{
    if (!IsBootLaunch())
        return false;
    if (!BootIniEnabled())
        return false;
    if (!FileExists(VideoPath().c_str()))
        return false;
    if (GetTickCount() > 60000)
        return false;
    return true;
}

void VideoBootSplash::SyncAutostart(bool enable)
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        return;

    if (enable) {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        wchar_t value[MAX_PATH + 16] = {};
        wsprintfW(value, L"\"%s\" --boot", path);
        RegSetValueExW(key, kRunValueName, 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(value),
                       static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(key, kRunValueName);
    }
    RegCloseKey(key);
}

LRESULT CALLBACK VideoBootSplash::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_NCHITTEST)
        return HTCLIENT;
    if (msg == WM_ERASEBKGND)
        return 1;
    if (msg == WM_KEYDOWN && wp == VK_ESCAPE) {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool VideoBootSplash::Create(HINSTANCE inst)
{
    inst_ = inst;

    int screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (screenW <= 0 || screenH <= 0)
        return false;

    if (!classRegistered_) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WndProc;
        wc.hInstance = inst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
        wc.lpszClassName = L"SixSevenVideoBootSplash";
        if (!RegisterClassExW(&wc))
            return false;
        classRegistered_ = true;
    }

    hwnd_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"SixSevenVideoBootSplash", L"Six_Seven Video Boot",
        WS_POPUP,
        screenX, screenY, screenW, screenH,
        nullptr, nullptr, inst, nullptr);
    if (!hwnd_)
        return false;

    SetLayeredWindowAttributes(hwnd_, 0, 255, LWA_ALPHA);
    ShowWindow(hwnd_, SW_SHOWMAXIMIZED);
    UpdateWindow(hwnd_);
    SetForegroundWindow(hwnd_);

    startedMs_ = GetTickCount();
    active_ = true;
    return true;
}

void VideoBootSplash::PlayVideo()
{
    const std::wstring path = VideoPath();
    if (path.empty())
        return;

    if (MciOpenVideo(path.c_str(), &videoDev_) != 0)
        return;
    videoOpen_ = true;

    durationMs_ = MciGetLength(videoDev_);

    MCI_PLAY_PARMS play = {};
    mciSendCommandW(videoDev_, MCI_PLAY, MCI_NOTIFY, (DWORD_PTR)&play);
}

void VideoBootSplash::PlayAudio()
{
    const std::wstring path = SoundPath();
    if (path.empty())
        return;

    if (MciOpenWav(path.c_str(), &audioDev_) != 0)
        return;
    audioOpen_ = true;

    MCI_PLAY_PARMS play = {};
    mciSendCommandW(audioDev_, MCI_PLAY, 0, (DWORD_PTR)&play);
}

void VideoBootSplash::StopAudio()
{
    if (!audioOpen_)
        return;
    mciSendCommandW(audioDev_, MCI_STOP, 0, 0);
    mciSendCommandW(audioDev_, MCI_CLOSE, 0, 0);
    audioDev_ = 0;
    audioOpen_ = false;
}

void VideoBootSplash::Destroy()
{
    active_ = false;

    if (videoOpen_) {
        mciSendCommandW(videoDev_, MCI_STOP, 0, 0);
        mciSendCommandW(videoDev_, MCI_CLOSE, 0, 0);
        videoDev_ = 0;
        videoOpen_ = false;
    }
    StopAudio();

    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

bool VideoBootSplash::IsShellReady() const
{
    HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!tray)
        return false;
    return GetShellWindow() != nullptr;
}

bool VideoBootSplash::ShouldEnd() const
{
    const DWORD now = GetTickCount();
    const DWORD elapsed = now - startedMs_;

    if (elapsed > 30000)
        return true;

    if (durationMs_ > 0 && elapsed >= durationMs_ + 1000)
        return true;

    if (elapsed > 3000 && IsShellReady())
        return true;

    return false;
}

void VideoBootSplash::PumpMessages()
{
    MSG msg = {};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT)
            break;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

bool VideoBootSplash::Run(HINSTANCE inst)
{
    wasShown_ = false;
    if (!ShouldShow())
        return false;

    bool shown = false;
    if (Create(inst)) {
        shown = true;
        PlayVideo();
        PlayAudio();

        while (active_) {
            PumpMessages();
            if (ShouldEnd())
                break;
            Sleep(16);
        }
        Destroy();
    }

    wasShown_ = shown;
    return shown;
}

} /* namespace six_seven */
