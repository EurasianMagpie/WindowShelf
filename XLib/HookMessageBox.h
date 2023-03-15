#pragma once

// HookMessageBox.h

#include <windowsx.h>
#include <tchar.h>
#include "APIHook.h"
#include <XLog.h>


/*
* 对 MessageBox* 函数 Hook 的测试
* 当被拦截的 MessageBox* 函数被调用后，会打印日志
*/
class HookMessageBox {
public:
    typedef int (WINAPI* PFNMESSAGEBOXA)(HWND, PCSTR, PCSTR, UINT);
    typedef int (WINAPI* PFNMESSAGEBOXW)(HWND, PCWSTR, PCWSTR, UINT);

    static APIHook s_MessageBoxA;
    static APIHook s_MessageBoxW;

public:
    static int WINAPI HookMessageBoxA(HWND hWnd, PCSTR pszText, PCSTR pszCaption, UINT uType) {
        int nResult = ((PFNMESSAGEBOXA)(PROC)s_MessageBoxW)(hWnd, pszText, pszCaption, uType);
        XLOGA("XLIB | HookMessageBoxA Caption:%s Result:%p", pszCaption, nResult);
        return nResult;
    }

    static int WINAPI HookMessageBoxW(HWND hWnd, PCWSTR pszText, PCWSTR pszCaption, UINT uType) {
        int nResult = ((PFNMESSAGEBOXW)(PROC)s_MessageBoxW)(hWnd, pszText, pszCaption, uType);
        XLOGW(L"XLIB | HookMessageBoxW Caption:%s Result:%p", pszCaption, nResult);
        return nResult;
    }
};


APIHook HookMessageBox::s_MessageBoxA("User32.dll", "MessageBoxA", (PROC)HookMessageBox::HookMessageBoxA);
APIHook HookMessageBox::s_MessageBoxW("User32.dll", "MessageBoxW", (PROC)HookMessageBox::HookMessageBoxW);