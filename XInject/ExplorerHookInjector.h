#pragma once

#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "../common/include/XLib.h"
#include "../common/include/XLog.h"

class ExplorerHookInjector {
public:
    ExplorerHookInjector() : mThreadId(0) {
    }

    ~ExplorerHookInjector() {
        UnhookExplor();
    }

    void HookExplor() {
        if (mThreadId == 0) {
            mThreadId = findExplorerUIThread();
        }
        xiHook(mThreadId);
        XLOG(_T("XINJECT | ExplorerHookInjector::Hook | threadId:%d"), mThreadId);
    }

    void UnhookExplor() {
        xiUnhook();
        XLOG(_T("XINJECT | ExplorerHookInjector::Unhook"));
    }

private:
    DWORD findExplorerUIThread() {
        HWND hWndLV = GetFirstChild(GetFirstChild(FindWindow(_T("ProgMan"), NULL)));
        if (!IsWindow(hWndLV)) {
            return 0;
        }
        return GetWindowThreadProcessId(hWndLV, NULL);
    }

private:
    DWORD mThreadId;
};