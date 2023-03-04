#pragma once

#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <map>
#include "XLibHolder.h"
#define ENABLE_XLOG
#include "../common/include/XLog.h"

/*
* ʹ�� HOOK API -> SetWindowsHookEx ����DLLע��
* ����಻���̰߳�ȫ��
*/
class HookExInjector {

public:
    ~HookExInjector() {
        UnhookAll();
    }

    /*
    * ��ָ���߳� Hook
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
            // HOOK �ɹ�֮��������Ŀ���̷߳�����Ϣ���Լ���Ŀ���̣߳���������DLL
            bRet = PostThreadMessage(dwThreadId, WM_NULL, 0, 0);
        }
        mThreadHookMap[dwThreadId] = hHook;
        XLOG(_T("XINJECT | HookExInjector::Hook | threadId:%d"), dwThreadId);
        return bRet;
    }

    /*
    * ȡ��ָ���̵߳� Hook
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
    // �Ѿ����õ� Hook
    // key:�߳�ID - value:HHOOK
    std::map<DWORD, HHOOK> mThreadHookMap;
};


/*
* �� Explorer �������� Hook �Ĺ�����
* ͨ��һ������ Explorer ��UI�߳�ID�ĺ����ҵ�Ŀ���߳�
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