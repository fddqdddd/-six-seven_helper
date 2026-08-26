#include "../include/BootLogoInstaller.h"

#include <windows.h>

namespace {

bool HasArg(int argc, wchar_t** argv, const wchar_t* flag)
{
    for (int i = 1; i < argc; ++i) {
        if (_wcsicmp(argv[i], flag) == 0)
            return true;
    }
    return false;
}

} /* namespace */

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return 1;

    const bool install = HasArg(argc, argv, L"--install");
    const bool uninstall = HasArg(argc, argv, L"--uninstall");
    LocalFree(argv);

    std::wstring message;
    bool ok = false;
    if (install)
        ok = six_seven::BootLogoInstaller::Install(&message, false);
    else if (uninstall)
        ok = six_seven::BootLogoInstaller::Uninstall(&message, false);
    else
        message = L"Six_Seven Boot Logo\n\n"
                  L"--install   установить картинку из assets/sprites/loading/\n"
                  L"--uninstall убрать и вернуть стандартную загрузку";

    MessageBoxW(nullptr, message.c_str(), L"Six_Seven — логотип загрузки ПК",
                MB_OK | (ok ? MB_ICONINFORMATION : MB_ICONWARNING));
    return ok ? 0 : 1;
}
