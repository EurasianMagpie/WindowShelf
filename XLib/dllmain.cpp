// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#ifndef ENABLE_XLOG
#define ENABLE_XLOG
#endif
#include "../common/include/XLog.h"

HHOOK g_hHook = NULL;
HMODULE g_hModule = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        XLOG(_T("XLIB | DLL_PROCESS_ATTACH"));
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        XLOG(_T("XLIB | DLL_PROCESS_DETACH"));
        break;
    }
    return TRUE;
}

#ifdef __cplusplus
extern "C" {
#endif

LRESULT WINAPI GetMessageProc(int nCode, WPARAM wParam, LPARAM lParam) {
    XLOG(_T("XLIB | GetMessageProc nCode:%d WPARAM:0x%X LPARAM:0x%X"), nCode);
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

#ifdef __cplusplus
}
#endif