#pragma once

//  RemoteThreadInjector.h

#include <Windows.h>
#include <malloc.h>
#include <strsafe.h>
#include <TlHelp32.h>
#include <string>
#include <set>
#define ENABLE_XLOG
#include "../common/include/XLog.h"

#ifdef UNICODE
#define InjectLib  RemoteThreadUtil::InjectLibW
#define EjectLib  RemoteThreadUtil::EjectLibW
#else
#define InjectLib  RemoteThreadUtil::InjectLibA
#define EjectLib  RemoteThreadUtil::EjectLibA
#endif // !UNICODE

#ifndef KERNEL32_MODULE_NAME
#define KERNEL32_MODULE_NAME TEXT("Kernel32.dll")
#endif

/*
* ���߷���
* ʹ�ô���Զ���߳� CreateRemoteThread �ķ�ʽ����DLLע��
*/
class RemoteThreadUtil {
public:
static BOOL InjectLibW(DWORD dwProcessId, PCWSTR pszLibFile) {
    BOOL bOk = FALSE;
    HANDLE hProcess = NULL;
    HANDLE hThread = NULL;
    PWSTR pszLibFileRemote = NULL;

    XLOG(_T("XINJECT | RemoteThreadUtil::InjectLibW | dwProcessId:%d  libFile:%s"), dwProcessId, pszLibFile);
    __try {
        // ����Ԥ�ڵ�Ȩ�ޣ���ȡĿ����̵�HANDLE
        hProcess = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE,
            FALSE,
            dwProcessId);
        if (!hProcess) {
            XLOG(_T("XINJECT | RemoteThreadUtil::InjectLibW | OpenProcess Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // ������Ŀ������У�����ļ�·���ַ���������ڴ��С
        int sizeInCh = 1 + lstrlenW(pszLibFile);
        int sizeInByte = sizeInCh * sizeof(wchar_t);

        // ��Ŀ�����Ϊ�ļ�·���ַ��������ڴ�
        pszLibFileRemote = (PWSTR)VirtualAllocEx(hProcess, NULL, sizeInByte, MEM_COMMIT, PAGE_READWRITE);
        if (!pszLibFileRemote) {
            XLOG(_T("XINJECT | RemoteThreadUtil::InjectLibW | VirtualAllocEx Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // ����DLL�ļ�·����Ŀ������ڴ�
        if (!WriteProcessMemory(hProcess, pszLibFileRemote, (PVOID)pszLibFile, sizeInByte, NULL)) {
            XLOG(_T("XINJECT | RemoteThreadUtil::InjectLibW | WriteProcessMemory Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // ��ȡ LoadLibraryW ������ַ��
        // Kernel32.dll ��ÿ�������еļ��ص�ַ����ͬ���������еĺ�����ַҲ��ͬ
        PTHREAD_START_ROUTINE pfnThreadRoutine = (PTHREAD_START_ROUTINE)
            GetProcAddress(GetModuleHandle(KERNEL32_MODULE_NAME), "LoadLibraryW");
        if (!pfnThreadRoutine) {
            XLOG(_T("XINJECT | RemoteThreadUtil::InjectLibW | GetProcAddress LoadLibraryW Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // ʹ�� LoadLibraryW ��Ϊ��ں�������Ŀ������д����߳�
        hThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRoutine, pszLibFileRemote, 0, NULL);
        if (!hThread) {
            XLOG(_T("XINJECT | RemoteThreadUtil::InjectLibW | CreateRemoteThread Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // �ȵ�Ŀ��������´������߳�ִ����ϣ����߳�ֻ����DLL�����Ͼͻ����
        WaitForSingleObject(hThread, INFINITE);

        // ������Ϊ�ɹ�
        bOk = TRUE;
    }
    __finally {
        // ���������ͷ�ǰ����Ŀ������з�����ڴ棬�رվ��
        if (pszLibFileRemote) {
            VirtualFreeEx(hProcess, pszLibFileRemote, 0, MEM_RELEASE);
            pszLibFileRemote = NULL;
        }
        if (hThread) {
            CloseHandle(hThread);
            hThread = NULL;
        }
        if (hProcess) {
            CloseHandle(hProcess);
            hProcess = NULL;
        }
    }
    return bOk;
}

static BOOL InjectLibA(DWORD dwProcessId, PCSTR pszLibFile) {
    // ��ANSI�ַ���ת��ΪUNICODE�ַ���
    SIZE_T sizeInCh = lstrlenA(pszLibFile) + 1;
    // _alloca ��ջ�Ϸ���ռ䣬�ɱ���������ջƽ��
    PWSTR pszLibFileW = (PWSTR)_alloca(sizeInCh * sizeof(wchar_t));
    StringCchPrintfW(pszLibFileW, sizeInCh, L"%S", pszLibFile);

    // ����UNICODE�汾�� InjectLibW
    return InjectLibW(dwProcessId, pszLibFileW);
}

static BOOL EjectLibW(DWORD dwProcessId, PCWSTR pszLibFile) {
    BOOL bOk = FALSE;
    HANDLE hthSnapshot = NULL;
    HANDLE hProcess = NULL;
    HANDLE hThread = NULL;

    XLOG(_T("XINJECT | RemoteThreadUtil::EjectLibW | dwProcessId:%d  libFile:%s"), dwProcessId, pszLibFile);
    __try {
        // ��ȡĿ����̵İ���ģ����Ϣ�Ŀ���
        hthSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
        if (!hthSnapshot) {
            XLOG(_T("XINJECT | RemoteThreadUtil::EjectLibW | CreateToolhelp32Snapshot Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // ���� pszLibFile ����ģ����Ϣ
        MODULEENTRY32W me = { sizeof(me) };
        BOOL bFound = FALSE;
        BOOL bHasMore = Module32FirstW(hthSnapshot, &me);
        while (bHasMore) {
            bFound = (_wcsicmp(me.szModule, pszLibFile) == 0)
                || (_wcsicmp(me.szExePath, pszLibFile) == 0);
            if (bFound) {
                break;
            }
            bHasMore = Module32NextW(hthSnapshot, &me);
        }
        if (!bFound) {
            XLOG(_T("XINJECT | RemoteThreadUtil::EjectLibW | Search taget modul Failed, Can not find target module"));
            __leave;
        }

        // ����Ԥ�ڵ�Ȩ�ޣ���ȡĿ����̵�HANDLE
        hProcess = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION,
            FALSE,
            dwProcessId);
        if (!hProcess) {
            XLOG(_T("XINJECT | RemoteThreadUtil::EjectLibW | OpenProcess Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // ��ȡĿ������� FreeLibrary ������ַ
        PTHREAD_START_ROUTINE pfnThreadRoutine = (PTHREAD_START_ROUTINE)
            GetProcAddress(GetModuleHandle(KERNEL32_MODULE_NAME), "FreeLibrary");
        if (!pfnThreadRoutine) {
            XLOG(_T("XINJECT | RemoteThreadUtil::EjectLibW | GetProcAddress FreeLibrary Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // ʹ�� FreeLibrary ��Ϊ��ں�������Ŀ������д����߳�
        hThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRoutine, me.modBaseAddr, 0, NULL);
        if (!hThread) {
            XLOG(_T("XINJECT | RemoteThreadUtil::EjectLibW | CreateRemoteThread Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // �ȵ�Ŀ��������´������߳�ִ����ϣ����߳�ֻ����DLL�����Ͼͻ����
        WaitForSingleObject(hThread, INFINITE);

        // ������Ϊ�ɹ�
        bOk = TRUE;
    }
    __finally {
        // ���������رվ��
        if (hthSnapshot) {
            CloseHandle(hthSnapshot);
            hthSnapshot = NULL;
        }
        if (hThread) {
            CloseHandle(hThread);
            hThread = NULL;
        }
        if (hProcess) {
            CloseHandle(hProcess);
            hProcess = NULL;
        }
    }
    return bOk;
}

static BOOL EjectLibA(DWORD dwProcessId, PCSTR pszLibFile) {
    // ��ANSI�ַ���ת��ΪUNICODE�ַ���
    SIZE_T sizeInCh = lstrlenA(pszLibFile) + 1;
    // _alloca ��ջ�Ϸ���ռ䣬�ɱ���������ջƽ��
    PWSTR pszLibFileW = (PWSTR)_alloca(sizeInCh * sizeof(wchar_t));
    StringCchPrintfW(pszLibFileW, sizeInCh, L"%S", pszLibFile);

    // ����UNICODE�汾�� EjectLibW
    return EjectLibW(dwProcessId, pszLibFileW);
}

};

/*
* ������
* ʹ�ô���Զ���߳� CreateRemoteThread �ķ�ʽ����DLLע��
*/
class RemoteThreadInjector {
public:
    RemoteThreadInjector(LPCTSTR lpszLibFile) : mLibFilePath(lpszLibFile) { }

    ~RemoteThreadInjector() {
        EjectAll();
    }

    BOOL Inject(DWORD dwProcessId) {
        if (mInjectedProcessId.find(dwProcessId) != mInjectedProcessId.end()) {
            return FALSE;
        }
        if (!InjectLib(dwProcessId, mLibFilePath.c_str())) {
            return FALSE;
        }
        mInjectedProcessId.insert(dwProcessId);
        return TRUE;
    }

    void Eject(DWORD dwProcessId) {
        auto ite = mInjectedProcessId.find(dwProcessId);
        if (ite != mInjectedProcessId.end()) {
            EjectLib(dwProcessId, mLibFilePath.c_str());
            mInjectedProcessId.erase(ite);
        }
    }

private:
    void EjectAll() {
        for (auto pid : mInjectedProcessId) {
            EjectLib(pid, mLibFilePath.c_str());
        }
        mInjectedProcessId.clear();
    }

private:
#ifdef UNICODE
    std::wstring mLibFilePath;
#else
    std::string mLibFilePath;
#endif // !UNICODE
    std::set<DWORD> mInjectedProcessId;

};