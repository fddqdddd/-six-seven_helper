#include "../include/BootLogoInstaller.h"
#include "../include/Util.h"
#include "../config.h"

#include <shellapi.h>
#include <gdiplus.h>

#include <algorithm>
#include <fstream>
#include <vector>

namespace six_seven {

namespace {

int FrameNumberFromPath(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L"\\/");
    const std::wstring name = slash == std::wstring::npos ? path : path.substr(slash + 1);
    int num = 0;
    bool any = false;
    for (wchar_t c : name) {
        if (c >= L'0' && c <= L'9') {
            num = num * 10 + (c - L'0');
            any = true;
        }
    }
    return any ? num : 0;
}

bool FramePathLess(const std::wstring& a, const std::wstring& b)
{
    const int na = FrameNumberFromPath(a);
    const int nb = FrameNumberFromPath(b);
    if (na != nb)
        return na < nb;
    return a < b;
}

bool GetBmpEncoderClsid(CLSID* out)
{
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
        return false;
    std::vector<BYTE> buf(size);
    auto* codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buf.data());
    if (Gdiplus::GetImageEncoders(num, size, codecs) != Gdiplus::Ok)
        return false;
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(codecs[i].MimeType, L"image/bmp") == 0) {
            *out = codecs[i].Clsid;
            return true;
        }
    }
    return false;
}

bool ExportPngToBmp24(const std::wstring& pngPath, const std::wstring& bmpPath)
{
    Gdiplus::Bitmap src(pngPath.c_str());
    if (src.GetLastStatus() != Gdiplus::Ok)
        return false;
    const int w = static_cast<int>(src.GetWidth());
    const int h = static_cast<int>(src.GetHeight());
    if (w <= 0 || h <= 0)
        return false;

    Gdiplus::Bitmap dst(w, h, PixelFormat24bppRGB);
    if (dst.GetLastStatus() != Gdiplus::Ok)
        return false;
    Gdiplus::Graphics g(&dst);
    g.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
    g.Clear(Gdiplus::Color(0, 0, 0));
    g.DrawImage(&src, 0, 0, w, h);

    CLSID clsid = {};
    if (!GetBmpEncoderClsid(&clsid))
        return false;
    return dst.Save(bmpPath.c_str(), &clsid, nullptr) == Gdiplus::Ok;
}

bool ListLoadingPngs(std::vector<std::wstring>& out)
{
    out.clear();
    const std::wstring folder =
        AssetPath((std::string(SIX_SEVEN_SPRITES_ROOT) + "/" + MOD_SPRITE_LOADING).c_str());
    WIN32_FIND_DATAW fd = {};
    const std::wstring pattern = PathJoin(folder, L"*.png");
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            out.push_back(PathJoin(folder, fd.cFileName));
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    std::sort(out.begin(), out.end(), FramePathLess);
    return !out.empty();
}

} /* namespace */

std::wstring BootLogoInstaller::BootLogoToolPath()
{
    return PathJoin(GetExeDirectory(), L"Six_Seven_BootLogo.exe");
}

std::wstring BootLogoInstaller::FindHackBgrtDir()
{
    const std::wstring exeDir = GetExeDirectory();
    const std::wstring candidates[] = {
        PathJoin(exeDir, L"HackBGRT"),
        PathJoin(exeDir, L"tools\\HackBGRT"),
    };
    for (const auto& dir : candidates) {
        if (FileExists(PathJoin(dir, L"setup.exe")))
            return dir;
    }
    return {};
}

bool BootLogoInstaller::WriteConfig(const std::wstring& hackDir,
                                    const std::vector<std::wstring>& bmpNames)
{
    const std::wstring cfgPath = PathJoin(hackDir, L"config.txt");
    std::ofstream out(WideToUtf8(cfgPath.c_str()), std::ios::binary);
    if (!out)
        return false;
    out << "boot=MS\r\n";
    out << "resolution=0x0\r\n";
    out << "log=0\r\n";
    out << "debug=0\r\n";
    for (const auto& name : bmpNames) {
        const std::string file = WideToUtf8(name.c_str());
        out << "image=path=" << file << "\r\n";
    }
    return true;
}

bool BootLogoInstaller::ExportLoadingFrames(const std::wstring& hackDir, std::wstring* err)
{
    std::vector<std::wstring> pngs;
    if (!ListLoadingPngs(pngs)) {
        if (err)
            *err = L"Нет PNG в assets/sprites/loading/.";
        return false;
    }

    std::vector<std::wstring> bmpNames;
    int index = 0;
    for (const auto& png : pngs) {
        wchar_t name[64];
        wsprintfW(name, L"six_seven_%04d.bmp", ++index);
        const std::wstring bmpPath = PathJoin(hackDir, name);
        if (!ExportPngToBmp24(png, bmpPath)) {
            if (err)
                *err = L"Не удалось конвертировать кадр в BMP.";
            return false;
        }
        bmpNames.push_back(name);
    }
    return WriteConfig(hackDir, bmpNames);
}

bool BootLogoInstaller::RunSetupBatch(const std::wstring& hackDir, const wchar_t* commands,
                                      std::wstring* err)
{
    const std::wstring setup = PathJoin(hackDir, L"setup.exe");
    std::wstring cmdLine = L"\"" + setup + L"\" batch " + commands;

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back(L'\0');

    if (!CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE, 0, nullptr,
                        hackDir.c_str(), &si, &pi)) {
        if (err)
            *err = L"Не удалось запустить HackBGRT setup.exe (нужны права администратора).";
        return false;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    if (code != 0) {
        if (err)
            *err = L"HackBGRT завершился с ошибкой. Проверьте UEFI и Secure Boot.";
        return false;
    }
    return true;
}

bool BootLogoInstaller::Install(std::wstring* outMessage, bool silent)
{
    Gdiplus::GdiplusStartupInput input;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &input, nullptr) != Gdiplus::Ok) {
        if (outMessage)
            *outMessage = L"GDI+ недоступен.";
        return false;
    }

    bool ok = false;
    std::wstring err;
    const std::wstring hackDir = FindHackBgrtDir();
    if (hackDir.empty()) {
        err = L"Положите HackBGRT (setup.exe) в папку HackBGRT рядом с Six_Seven.exe.\n"
              L"Скачать: github.com/Metabolix/HackBGRT/releases";
    } else if (!ExportLoadingFrames(hackDir, &err)) {
        /* err set */
    } else if (!RunSetupBatch(hackDir, L"install enable-bcdedit", &err)) {
        /* err set */
    } else {
        ok = true;
        err = L"Логотип загрузки ПК установлен.\n"
              L"Перезагрузите компьютер.\n\n"
              L"Если картинка производителя мелькает — это нормально для UEFI.\n"
              L"Windows загрузится как обычно.";
    }

    if (token)
        Gdiplus::GdiplusShutdown(token);

    if (outMessage)
        *outMessage = err;
    return ok;
}

void BootLogoInstaller::EnsureInstalledOnStartup()
{
#if SIX_SEVEN_BOOT_PC_LOGO_ENABLED
    if (FindHackBgrtDir().empty())
        return;
    Install(nullptr, true);
#endif
}

bool BootLogoInstaller::Uninstall(std::wstring* outMessage, bool silent)
{
    (void)silent;
    std::wstring err;
    const std::wstring hackDir = FindHackBgrtDir();
    if (hackDir.empty()) {
        err = L"HackBGRT не найден (папка HackBGRT/setup.exe).";
    } else if (!RunSetupBatch(hackDir, L"uninstall", &err)) {
        /* err set */
    } else {
        err = L"Логотип загрузки ПК снят. Перезагрузите компьютер.";
        if (outMessage)
            *outMessage = err;
        return true;
    }
    if (outMessage)
        *outMessage = err;
    return false;
}

void BootLogoInstaller::LaunchInstallElevated()
{
    const std::wstring tool = BootLogoToolPath();
    if (!FileExists(tool))
        return;
    ShellExecuteW(nullptr, L"runas", tool.c_str(), L"--install", GetExeDirectory().c_str(),
                  SW_SHOW);
}

void BootLogoInstaller::LaunchUninstallElevated()
{
    const std::wstring tool = BootLogoToolPath();
    if (!FileExists(tool))
        return;
    ShellExecuteW(nullptr, L"runas", tool.c_str(), L"--uninstall", GetExeDirectory().c_str(),
                  SW_SHOW);
}

} /* namespace six_seven */
