#pragma once

// APIHook.h

#include <ImageHlp.h>
#pragma comment(lib, "ImageHlp")
#include <TlHelp32.h>
#include <tchar.h>
#include <strsafe.h>

#include <XLog.h>

/*
* API 拦截实现
* 每个对象实现对一个函数的拦截，采用RAII方式封装，构造时进行Hook，析构时撤销Hook
* 该类的所有对象被放入到以 s_pHead 为头节点的链表中进行管理
* 当一次性Hook完成后，新加载的DLL也会自动完成对拦截函数的Hook
*/
class APIHook {
public:

    APIHook(PCSTR pszCalleeModName, PCSTR pszFuncName, PROC pfnHook) {
        /*
        * 注意
        * 只有当提供导出函数模块被加载之后，目标函数才能被成功拦截。一种解决思路是，
        * 用成员变量保存函数名，然后在 LoadLibrary* 拦截函数中，遍历所有 APIHook 实例对象，
        * 判断 pszCalleeModName 是否与某个需要被拦截的模块名相同，然后检查该模块导出表，
        * 并对所有模块的导入表重新Hook所需拦截的函数
        */

        // 用当前头节点作为this的next
        m_pNext = s_pHead;
        // 用this作为新的头节点
        s_pHead = this;

        // 保持被Hook函数的信息
        m_pszCalleeModName = pszCalleeModName;
        m_pszFuncName = pszFuncName;

        m_pfnHook = pfnHook;
        m_pfnOrig = GetProcAddressRaw(GetModuleHandleA(pszCalleeModName), m_pszFuncName);

        // 检查函数名是否存在，如果不存在则无法拦截
        // 通常在发生于目标函数所在模块还没有被进程加载
        if (pfnHook == NULL) {
            XLOGA("APIHOOK constructor | impossible to find Function[%s] in Module[%s]",
                pszFuncName,
                pszCalleeModName);
            return;
        }

        // 在当前已加载的所有模块中Hook该函数
        ReplaceIATEntryInAllMods(m_pszCalleeModName, m_pfnOrig, m_pfnHook);

#ifdef _DEBUG
        XLOGA("APIHOOK constructor | Function[%s]", pszFuncName);
#endif
    }

    ~APIHook() {
        // 从当前已加载的所有模块中撤销对当前函数的Hook
        ReplaceIATEntryInAllMods(m_pszCalleeModName, m_pfnOrig, m_pfnHook);

        // 从链表中移除当前对象
        APIHook* p = s_pHead;
        if (p == this) {
            s_pHead = p->m_pNext;
        }
        else {
            BOOL bFound = FALSE;
            while (!bFound && p->m_pNext != NULL) {
                if (p->m_pNext == this) {
                    p->m_pNext = p->m_pNext->m_pNext;
                    bFound = TRUE;
                }
                p = p->m_pNext;
            }
        }
    }

    operator PROC() {
        return (m_pfnOrig);
    }

    // 不对当前类 APIHook 所在模块进行拦截
    static BOOL s_ExcludeAPIHookMod;

public:
    // 注意，这个函数一定不能内联 / This function must NOT inlined
    static __declspec(noinline) FARPROC WINAPI GetProcAddressRaw(HMODULE hMod, PCSTR pszProcName) {
        return ::GetProcAddress(hMod, pszProcName);
    }

private:
    // Maximum private memory address
    static PVOID s_pvMaxAppAddr;
    // APIHook 对象链表的第一个对象
    static APIHook* s_pHead;

    // 下一个对象
    APIHook* m_pNext;
    // 包含被拦截函数的模块名，ANSI字符串
    PCSTR m_pszCalleeModName;
    // 被拦截函数名，ANSI字符串
    PCSTR m_pszFuncName;
    // 被拦截函数原始地址
    PROC m_pfnOrig;
    // 替换被拦截函数的Hook函数地址
    PROC m_pfnHook;

private:
    // 处理当模块被卸载时可能产生的异常
    static LONG WINAPI InvalidReadExceptionFilter(PEXCEPTION_POINTERS pep) {
        // 处理所有异常，返回 EXCEPTION_EXECUTE_HANDLER
        // 因为不会修改任何模块信息
        LONG lDisposition = EXCEPTION_EXECUTE_HANDLER;
        
        // 注意 pep->ExceptionRecord->ExceptionCode 的值为 0xc0000005
        return lDisposition;
    }

    // 获取包含特定地址的 HMODULE
    static HMODULE ModuleFromAddress(PVOID pv) {
        MEMORY_BASIC_INFORMATION mbi;
        return (VirtualQuery(pv, &mbi, sizeof(mbi)) != 0)
            ? (HMODULE)mbi.AllocationBase
            : NULL;
    }

    // 在所有模块的 导入段（import section）中替换一个符号地址
    static void WINAPI ReplaceIATEntryInAllMods(PCSTR pszCalleeModName, PROC pfnOrig, PROC pfnHook) {
        // 如果需要取当前模块句柄，则获取当前函数地址所在模块的句柄
        HMODULE hModThisMod = s_ExcludeAPIHookMod
            ? ModuleFromAddress(ReplaceIATEntryInAllMods)
            : NULL;

        HANDLE hthSnapshot = NULL;
        __try {
            // 获取目标进程的包含模块信息的快照
            hthSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
            if (hthSnapshot == NULL) {
                XLOG(_T("APIHOOK ReplaceIATEntryInAllMods | CreateToolhelp32Snapshot Failed, lastError:%d"), GetLastError());
                __leave;
            }

            MODULEENTRY32W me = { sizeof(me) };
            BOOL bFound = FALSE;
            BOOL bHasMore = Module32FirstW(hthSnapshot, &me);
            while (bHasMore) {
                // 不Hook这个函数所在模块
                if (me.hModule != hModThisMod) {
                    ReplaceIATEntryInOneMod(pszCalleeModName, pfnOrig, pfnHook, me.hModule);
                }
                bHasMore = Module32NextW(hthSnapshot, &me);
            }
        }
        __finally {
            if (hthSnapshot) {
                CloseHandle(hthSnapshot);
                hthSnapshot = NULL;
            }
        }
    }

    // 在一个模块的 导入段（import section）中替换一个符号地址
    static void WINAPI ReplaceIATEntryInOneMod(PCSTR pszCalleeModName, PROC pfnOrig, PROC pfnHook, HMODULE hModCaller) {
        // 获取模块导入段(import section)地址
        ULONG ulSize;

        // An exception was triggered by Explorer (when browsing the content of
        // a folder) into imagehlp.dll. It looks like one module was unloaded...
        // Maybe some threading problem: the list of modules from Toolhelp might
        // not be accurate if FreeLibrary is called during the enumeration.
        PIMAGE_IMPORT_DESCRIPTOR pImportDesc = NULL;
        __try {
            pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(
                hModCaller, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ulSize);
        }
        __except (InvalidReadExceptionFilter(GetExceptionInformation())) {
            // 这里不做任何处理
            // 保持 pImportDesc 值为 NULL，继续执行
        }

        if (pImportDesc == NULL) {
            // 模块没有导入段或者不再被加载
            return;
        }

        // 在导入描述中查找被调用函数的引用
        // 解析每一个导入段(import section)，直到找到并替换目标函数
        for (; pImportDesc->Name; pImportDesc++) {
            PSTR pszModName = (PSTR)((PBYTE)hModCaller + pImportDesc->Name);
            if (lstrcmpiA(pszModName, pszCalleeModName) == 0) {
                // Get caller's import address table (IAT) for the callee's funtions
                PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)((PBYTE)hModCaller + pImportDesc->FirstThunk);

                // 用新函数地址替换当前函数地址
                for (; pThunk->u1.Function; pThunk++) {
                    // 获取函数地址
                    PROC* ppfn = (PROC*)&pThunk->u1.Function;

                    // 判断该函数是否为需要查找的函数
                    BOOL bFound = (*ppfn == pfnOrig);
                    if (bFound) {
                        if (!WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnHook, sizeof(pfnHook), NULL)
                            && ERROR_NOACCESS == GetLastError()) {
                            DWORD dwOldProtect;
                            if (VirtualProtect(ppfn, sizeof(pfnHook), PAGE_WRITECOPY, &dwOldProtect)) {
                                WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnHook, sizeof(pfnHook), NULL);
                                VirtualProtect(ppfn, sizeof(pfnHook), dwOldProtect, &dwOldProtect);
                            }
                        }
                        // 完成
                        return;
                    }
                }
            }
        }
    }

    // 在一个模块的 导出段（export sections）中替换一个符号地址
    static void WINAPI ReplaceEATEntryInOneMod(HMODULE hMod, PCSTR pszFuncName, PROC pfnNew) {
        // 获取模块导出段(export section)的地址
        ULONG ulSize;

        PIMAGE_EXPORT_DIRECTORY pExportDir = NULL;
        __try {
            pExportDir = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryEntryToData(
                hMod, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &ulSize);
        }
        __except (InvalidReadExceptionFilter(GetExceptionInformation())) {
            // 这里不做任何处理
            // 在 pExportDir 值为 NULL 的情况下，继续执行
        }

        if (pExportDir == NULL) {
            // 这个模块没有导出段(export section)，或者没有被加载(或已经被卸载)
            return;
        }

        PDWORD pdwNamesRvas = (PDWORD)((PBYTE)hMod + pExportDir->AddressOfNames);
        PWORD pwNameOrdinals = (PWORD)((PBYTE)hMod + pExportDir->AddressOfNameOrdinals);
        PDWORD pdwFunctionAddresses = (PDWORD)((PBYTE)hMod + pExportDir->AddressOfFunctions);

        // 遍历模块的函数名数组
        for (DWORD n = 0; n < pExportDir->NumberOfNames; n++) {
            // 获取函数名
            PSTR pszNextFuncName = (PSTR)((PBYTE)hMod + pdwNamesRvas[n]);

            // 如果函数名不匹配，继续循环
            if (lstrcmpiA(pszNextFuncName, pszFuncName) != 0) {
                continue;
            }

            // 获取函数序号
            WORD ordinal = pwNameOrdinals[n];

            // 获取函数地址
            PROC* ppfn = (PROC*)&pdwFunctionAddresses[ordinal];

            // 转换用作替换的函数地址, 是一个相对于模块基地址的偏移量
            pfnNew = (PROC)((PBYTE)pfnNew - (PBYTE)hMod);

            // 将目标函数地址替换为新函数地址
            if (!WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnNew, sizeof(pfnNew), NULL)
                && (ERROR_NOACCESS == GetLastError())) {
                DWORD dwOldProtect;
                if (VirtualProtect(ppfn, sizeof(pfnNew), PAGE_WRITECOPY, &dwOldProtect)) {
                    WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnNew, sizeof(pfnNew), NULL);
                    VirtualProtect(ppfn, sizeof(pfnNew), dwOldProtect, &dwOldProtect);
                }
            }

            // 完成
            break;
        }
    }

private:
    // 当完成对一个函数Hook之后，又加载了新的DLL时，使用如下函数
    static void WINAPI FixupNewlyLoadedModule(HMODULE hMod, DWORD dwFlags) {
        // 如果一个新的模块被加载，对该模块Hook所有应该被Hook的函数
        if (hMod == NULL
            || hMod == ModuleFromAddress(FixupNewlyLoadedModule)
            || (dwFlags & LOAD_LIBRARY_AS_DATAFILE) != 0
            || (dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) != 0
            || (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE) != 0) {
            // 忽略当前函数所在模块
            // 忽略资源DLL
            return;
        }

        for (APIHook* p = s_pHead; p != NULL; p = p->m_pNext) {
            if (p->m_pfnOrig != NULL) {
                ReplaceIATEntryInAllMods(p->m_pszCalleeModName, p->m_pfnOrig, p->m_pfnHook);
            }
            else {
#ifdef _DEBUG
                // 不应该出现的情况
                XLOG(_T("APIHOOK FixupNewlyLoadedModule | impossible to find Function[%s]"),
                    p->m_pszCalleeModName);
#endif
            }
        }
    }

    /*
    * 以下 LoadLibraryXX 函数，用于捕获有新DLL加载
    */
    static HMODULE WINAPI LoadLibraryA(PCSTR pszModulePath) {
        HMODULE hMod = ::LoadLibraryA(pszModulePath);
        FixupNewlyLoadedModule(hMod, 0);
        return hMod;
    }

    static HMODULE WINAPI LoadLibraryW(PCWSTR pszModulePath) {
        HMODULE hMod = ::LoadLibraryW(pszModulePath);
        FixupNewlyLoadedModule(hMod, 0);
        return hMod;
    }

    static HMODULE WINAPI LoadLibraryExA(PCSTR pszModulePath, HANDLE hFile, DWORD dwFlags) {
        HMODULE hMod = ::LoadLibraryExA(pszModulePath, hFile, dwFlags);
        FixupNewlyLoadedModule(hMod, dwFlags);
        return hMod;
    }

    static HMODULE WINAPI LoadLibraryExW(PCWSTR pszModulePath, HANDLE hFile, DWORD dwFlags) {
        HMODULE hMod = ::LoadLibraryExW(pszModulePath, hFile, dwFlags);
        FixupNewlyLoadedModule(hMod, dwFlags);
        return hMod;
    }

    // 用于在被拦截函数被调用时，返回替换的函数地址
    static FARPROC WINAPI GetProcAddress(HMODULE hMod, PCSTR pszProcName) {
        // 获取函数的真正地址
        FARPROC pfn = GetProcAddressRaw(hMod, pszProcName);

        // 判断这个函数是不是一个被Hook的函数
        APIHook* p = s_pHead;
        for (; pfn != NULL && p != NULL; p = p->m_pNext) {
            if (pfn == p->m_pfnOrig) {
                // 这是一个被Hook的函数，返回该函数被替换的函数地址
                pfn = p->m_pfnHook;
                break;
            }
        }

        return pfn;
    }

private:
    // Hook以下函数
    static APIHook s_LoadLibraryA;
    static APIHook s_LoadLibraryW;
    static APIHook s_LoadLibraryExA;
    static APIHook s_LoadLibraryExW;
    static APIHook s_GetProcAddress;
};

APIHook* APIHook::s_pHead = NULL;

BOOL APIHook::s_ExcludeAPIHookMod = TRUE;

APIHook APIHook::s_LoadLibraryA("Kernel32.dll", "LoadLibraryA", (PROC)(APIHook::LoadLibraryA));
APIHook APIHook::s_LoadLibraryW("Kernel32.dll", "LoadLibraryW", (PROC)(APIHook::LoadLibraryW));
APIHook APIHook::s_LoadLibraryExA("Kernel32.dll", "LoadLibraryExA", (PROC)(APIHook::LoadLibraryExA));
APIHook APIHook::s_LoadLibraryExW("Kernel32.dll", "LoadLibraryExW", (PROC)(APIHook::LoadLibraryExW));
APIHook APIHook::s_GetProcAddress("Kernel32.dll", "GetProcAddress", (PROC)(APIHook::GetProcAddress));