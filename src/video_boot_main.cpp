#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../include/VideoBootSplash.h"

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, LPWSTR, int)
{
    (void)GetModuleHandleW(nullptr);

    six_seven::VideoBootSplash splash;
    if (splash.ShouldShow())
        splash.Run(inst);

    return 0;
}
