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
* 工具方法
* 使用创建远程线程 CreateRemoteThread 的方式进行DLL注入
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
        // 按照预期的权限，获取目标进程的HANDLE
        hProcess = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE,
            FALSE,
            dwProcessId);
        if (!hProcess) {
            XLOG(_T("XINJECT | RemoteThreadUtil::InjectLibW | OpenProcess Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // 计算在目标进程中，存放文件路径字符串所需的内存大小
        int sizeInCh = 1 + lstrlenW(pszLibFile);
        int sizeInByte = sizeInCh * sizeof(wchar_t);

        // 在目标进程为文件路径字符串分配内存
        pszLibFileRemote = (PWSTR)VirtualAllocEx(hProcess, NULL, sizeInByte, MEM_COMMIT, PAGE_READWRITE);
        if (!pszLibFileRemote) {
            XLOG(_T("XINJECT | RemoteThreadUtil::InjectLibW | VirtualAllocEx Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // 复制DLL文件路径到目标进程内存
        if (!WriteProcessMemory(hProcess, pszLibFileRemote, (PVOID)pszLibFile, sizeInByte, NULL)) {
            XLOG(_T("XINJECT | RemoteThreadUtil::InjectLibW | WriteProcessMemory Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // 获取 LoadLibraryW 函数地址，
        // Kernel32.dll 在每个进程中的加载地址都相同，所以其中的函数地址也相同
        PTHREAD_START_ROUTINE pfnThreadRoutine = (PTHREAD_START_ROUTINE)
            GetProcAddress(GetModuleHandle(KERNEL32_MODULE_NAME), "LoadLibraryW");
        if (!pfnThreadRoutine) {
            XLOG(_T("XINJECT | RemoteThreadUtil::InjectLibW | GetProcAddress LoadLibraryW Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // 使用 LoadLibraryW 作为入口函数，在目标进程中创建线程
        hThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRoutine, pszLibFileRemote, 0, NULL);
        if (!hThread) {
            XLOG(_T("XINJECT | RemoteThreadUtil::InjectLibW | CreateRemoteThread Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // 等到目标进程中新创建的线程执行完毕，该线程只加载DLL，马上就会完成
        WaitForSingleObject(hThread, INFINITE);

        // 到此认为成功
        bOk = TRUE;
    }
    __finally {
        // 清理工作，释放前面在目标进程中分配的内存，关闭句柄
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
    // 将ANSI字符串转换为UNICODE字符串
    SIZE_T sizeInCh = lstrlenA(pszLibFile) + 1;
    // _alloca 在栈上分配空间，由编译器负责栈平衡
    PWSTR pszLibFileW = (PWSTR)_alloca(sizeInCh * sizeof(wchar_t));
    StringCchPrintfW(pszLibFileW, sizeInCh, L"%S", pszLibFile);

    // 调用UNICODE版本的 InjectLibW
    return InjectLibW(dwProcessId, pszLibFileW);
}

static BOOL EjectLibW(DWORD dwProcessId, PCWSTR pszLibFile) {
    BOOL bOk = FALSE;
    HANDLE hthSnapshot = NULL;
    HANDLE hProcess = NULL;
    HANDLE hThread = NULL;

    XLOG(_T("XINJECT | RemoteThreadUtil::EjectLibW | dwProcessId:%d  libFile:%s"), dwProcessId, pszLibFile);
    __try {
        // 获取目标进程的包含模块信息的快照
        hthSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
        if (!hthSnapshot) {
            XLOG(_T("XINJECT | RemoteThreadUtil::EjectLibW | CreateToolhelp32Snapshot Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // 查找 pszLibFile 所在模块信息
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

        // 按照预期的权限，获取目标进程的HANDLE
        hProcess = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION,
            FALSE,
            dwProcessId);
        if (!hProcess) {
            XLOG(_T("XINJECT | RemoteThreadUtil::EjectLibW | OpenProcess Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // 获取目标进程中 FreeLibrary 函数地址
        PTHREAD_START_ROUTINE pfnThreadRoutine = (PTHREAD_START_ROUTINE)
            GetProcAddress(GetModuleHandle(KERNEL32_MODULE_NAME), "FreeLibrary");
        if (!pfnThreadRoutine) {
            XLOG(_T("XINJECT | RemoteThreadUtil::EjectLibW | GetProcAddress FreeLibrary Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // 使用 FreeLibrary 作为入口函数，在目标进程中创建线程
        hThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRoutine, me.modBaseAddr, 0, NULL);
        if (!hThread) {
            XLOG(_T("XINJECT | RemoteThreadUtil::EjectLibW | CreateRemoteThread Failed, lastError:%d"), GetLastError());
            __leave;
        }

        // 等到目标进程中新创建的线程执行完毕，该线程只加载DLL，马上就会完成
        WaitForSingleObject(hThread, INFINITE);

        // 到此认为成功
        bOk = TRUE;
    }
    __finally {
        // 清理工作，关闭句柄
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
    // 将ANSI字符串转换为UNICODE字符串
    SIZE_T sizeInCh = lstrlenA(pszLibFile) + 1;
    // _alloca 在栈上分配空间，由编译器负责栈平衡
    PWSTR pszLibFileW = (PWSTR)_alloca(sizeInCh * sizeof(wchar_t));
    StringCchPrintfW(pszLibFileW, sizeInCh, L"%S", pszLibFile);

    // 调用UNICODE版本的 EjectLibW
    return EjectLibW(dwProcessId, pszLibFileW);
}

};

/*
* 工具类
* 使用创建远程线程 CreateRemoteThread 的方式进行DLL注入
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