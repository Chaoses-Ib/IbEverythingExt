// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <IbDllHijack/dlls/WindowsCodecs.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        static auto module = LoadLibraryW(L"plugins\\IbEverythingExt.dll");
        if (module) {
            auto start = (bool (*)())GetProcAddress(module, "plugin_start");
            start();
        }
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        if (module) {
            auto stop = (void (*)())GetProcAddress(module, "plugin_stop");
            stop();
        }
        break;
    }
    return TRUE;
}

