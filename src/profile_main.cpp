#include "../include/ProfileCustomizer.h"

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

    const bool apply = HasArg(argc, argv, L"--apply");
    const bool setup = HasArg(argc, argv, L"--setup");
    const bool applyBoot = HasArg(argc, argv, L"--apply-boot");
    const bool installBoot = HasArg(argc, argv, L"--install-boot-task");
    const bool revert = HasArg(argc, argv, L"--revert");
    LocalFree(argv);

    if (installBoot)
        return six_seven::ProfileCustomizer::InstallBootApplyTask() ? 0 : 1;
    if (applyBoot)
        return six_seven::ProfileCustomizer::ApplyBootElevated() ? 0 : 1;
    if (setup || apply)
        return six_seven::ProfileCustomizer::ApplyElevated() ? 0 : 1;
    if (revert)
        return six_seven::ProfileCustomizer::RevertElevated() ? 0 : 1;
    return 1;
}
