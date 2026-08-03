#pragma once
#include <windows.h>

namespace RuneEngine {
    // Entry point for the worker thread created from DllMain.
    DWORD WINAPI Run(LPVOID hModule);
}
