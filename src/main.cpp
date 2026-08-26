#include "../include/Application.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, LPWSTR, int)
{
    six_seven::Application app;
    return app.Run(inst);
}
