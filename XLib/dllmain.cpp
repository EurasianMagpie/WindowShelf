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
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

LRESULT WINAPI GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) {
    XLOG(_T("XLIB | GetMsgProc nCode:%d WPARAM:0x%X LPARAM:0x%X"), nCode);
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

#ifdef __cplusplus
extern "C" {

BOOL WINAPI xiHook(DWORD dwThreadId) {
    BOOL bRet = FALSE;
    if (dwThreadId == 0 || g_hHook) {
        return FALSE;
    }
    g_hHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, g_hModule, dwThreadId);
    bRet = g_hHook != NULL;
    if (bRet) {
        bRet = PostThreadMessage(dwThreadId, WM_NULL, 0, 0);
    }
    return bRet;
}

BOOL WINAPI xiUnhook() {
    BOOL bRet = FALSE;
    if (g_hHook) {
        bRet = UnhookWindowsHookEx(g_hHook);
        g_hHook = NULL;
    }
    return bRet;
}

}
#endif