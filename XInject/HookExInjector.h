#pragma once

#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <map>
#include "XLibHolder.h"
#define ENABLE_XLOG
#include "../common/include/XLog.h"

/*
* 使用 HOOK API -> SetWindowsHookEx 进行DLL注入
* 这个类不是线程安全的
*/
class HookExInjector {

public:
    ~HookExInjector() {
        UnhookAll();
    }

    /*
    * 向指定线程 Hook
    */
    BOOL Hook(DWORD dwThreadId) {
        BOOL bRet = FALSE;
        if (dwThreadId == 0 || mThreadHookMap.find(dwThreadId) != mThreadHookMap.end()) {
            return FALSE;
        }
        HMODULE hXLibModule = theXLibHolder.GetModule();
        HOOKPROC hookProc = theXLibHolder.GetHookProc();
        if (!hXLibModule || !hookProc) {
            return FALSE;
        }
        HHOOK hHook = SetWindowsHookEx(WH_GETMESSAGE, hookProc, hXLibModule, dwThreadId);
        bRet = hHook != NULL;
        if (bRet) {
            // HOOK 成功之后，立刻向目标线程发送消息，以激活目标线程，立即加载DLL
            bRet = PostThreadMessage(dwThreadId, WM_NULL, 0, 0);
        }
        mThreadHookMap[dwThreadId] = hHook;
        XLOG(_T("XINJECT | HookExInjector::Hook | threadId:%d"), dwThreadId);
        return bRet;
    }

    /*
    * 取消指定线程的 Hook
    */
    BOOL Unhook(DWORD dwThreadId) {
        BOOL bRet = FALSE;
        auto ite = mThreadHookMap.find(dwThreadId);
        if (ite != mThreadHookMap.end()) {
            bRet = UnhookWindowsHookEx(ite->second);
            mThreadHookMap.erase(ite);
            bRet = TRUE;
            XLOG(_T("XINJECT | HookExInjector::Unhook | threadId:%d"), dwThreadId);
        }
        return bRet;
    }

private:
    void UnhookAll() {
        auto ite = mThreadHookMap.begin();
        while (ite != mThreadHookMap.end()) {
            UnhookWindowsHookEx(ite->second);
            ite = mThreadHookMap.erase(ite);
        }
    }

private:
    // 已经设置的 Hook
    // key:线程ID - value:HHOOK
    std::map<DWORD, HHOOK> mThreadHookMap;
};


/*
* 向 Explorer 进程设置 Hook 的工具类
* 通过一个查找 Explorer 的UI线程ID的函数找到目标线程
*/
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
        mHookExInjector.Hook(mThreadId);
    }

    void UnhookExplor() {
        if (mThreadId) {
            mHookExInjector.Unhook(mThreadId);
            mThreadId = 0;
        }
    }

    static DWORD findExplorerUIThread() {
        HWND hWndLV = GetFirstChild(GetFirstChild(FindWindow(_T("ProgMan"), NULL)));
        if (!IsWindow(hWndLV)) {
            return 0;
        }
        return GetWindowThreadProcessId(hWndLV, NULL);
    }

private:
    DWORD mThreadId;
    HookExInjector mHookExInjector;
};