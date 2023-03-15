#pragma once

// APIHook.h

#include <ImageHlp.h>
#pragma comment(lib, "ImageHlp")
#include <TlHelp32.h>
#include <tchar.h>
#include <strsafe.h>

#include <XLog.h>

/*
* API ����ʵ��
* ÿ������ʵ�ֶ�һ�����������أ�����RAII��ʽ��װ������ʱ����Hook������ʱ����Hook
* ��������ж��󱻷��뵽�� s_pHead Ϊͷ�ڵ�������н��й���
* ��һ����Hook��ɺ��¼��ص�DLLҲ���Զ���ɶ����غ�����Hook
*/
class APIHook {
public:

    APIHook(PCSTR pszCalleeModName, PCSTR pszFuncName, PROC pfnHook) {
        /*
        * ע��
        * ֻ�е��ṩ��������ģ�鱻����֮��Ŀ�꺯�����ܱ��ɹ����ء�һ�ֽ��˼·�ǣ�
        * �ó�Ա�������溯������Ȼ���� LoadLibrary* ���غ����У��������� APIHook ʵ������
        * �ж� pszCalleeModName �Ƿ���ĳ����Ҫ�����ص�ģ������ͬ��Ȼ�����ģ�鵼����
        * ��������ģ��ĵ��������Hook�������صĺ���
        */

        // �õ�ǰͷ�ڵ���Ϊthis��next
        m_pNext = s_pHead;
        // ��this��Ϊ�µ�ͷ�ڵ�
        s_pHead = this;

        // ���ֱ�Hook��������Ϣ
        m_pszCalleeModName = pszCalleeModName;
        m_pszFuncName = pszFuncName;

        m_pfnHook = pfnHook;
        m_pfnOrig = GetProcAddressRaw(GetModuleHandleA(pszCalleeModName), m_pszFuncName);

        // ��麯�����Ƿ���ڣ�������������޷�����
        // ͨ���ڷ�����Ŀ�꺯������ģ�黹û�б����̼���
        if (pfnHook == NULL) {
            XLOGA("APIHOOK constructor | impossible to find Function[%s] in Module[%s]",
                pszFuncName,
                pszCalleeModName);
            return;
        }

        // �ڵ�ǰ�Ѽ��ص�����ģ����Hook�ú���
        ReplaceIATEntryInAllMods(m_pszCalleeModName, m_pfnOrig, m_pfnHook);

#ifdef _DEBUG
        XLOGA("APIHOOK constructor | Function[%s]", pszFuncName);
#endif
    }

    ~APIHook() {
        // �ӵ�ǰ�Ѽ��ص�����ģ���г����Ե�ǰ������Hook
        ReplaceIATEntryInAllMods(m_pszCalleeModName, m_pfnOrig, m_pfnHook);

        // ���������Ƴ���ǰ����
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

    // ���Ե�ǰ�� APIHook ����ģ���������
    static BOOL s_ExcludeAPIHookMod;

public:
    // ע�⣬�������һ���������� / This function must NOT inlined
    static __declspec(noinline) FARPROC WINAPI GetProcAddressRaw(HMODULE hMod, PCSTR pszProcName) {
        return ::GetProcAddress(hMod, pszProcName);
    }

private:
    // Maximum private memory address
    static PVOID s_pvMaxAppAddr;
    // APIHook ��������ĵ�һ������
    static APIHook* s_pHead;

    // ��һ������
    APIHook* m_pNext;
    // ���������غ�����ģ������ANSI�ַ���
    PCSTR m_pszCalleeModName;
    // �����غ�������ANSI�ַ���
    PCSTR m_pszFuncName;
    // �����غ���ԭʼ��ַ
    PROC m_pfnOrig;
    // �滻�����غ�����Hook������ַ
    PROC m_pfnHook;

private:
    // ����ģ�鱻ж��ʱ���ܲ������쳣
    static LONG WINAPI InvalidReadExceptionFilter(PEXCEPTION_POINTERS pep) {
        // ���������쳣������ EXCEPTION_EXECUTE_HANDLER
        // ��Ϊ�����޸��κ�ģ����Ϣ
        LONG lDisposition = EXCEPTION_EXECUTE_HANDLER;
        
        // ע�� pep->ExceptionRecord->ExceptionCode ��ֵΪ 0xc0000005
        return lDisposition;
    }

    // ��ȡ�����ض���ַ�� HMODULE
    static HMODULE ModuleFromAddress(PVOID pv) {
        MEMORY_BASIC_INFORMATION mbi;
        return (VirtualQuery(pv, &mbi, sizeof(mbi)) != 0)
            ? (HMODULE)mbi.AllocationBase
            : NULL;
    }

    // ������ģ��� ����Σ�import section�����滻һ�����ŵ�ַ
    static void WINAPI ReplaceIATEntryInAllMods(PCSTR pszCalleeModName, PROC pfnOrig, PROC pfnHook) {
        // �����Ҫȡ��ǰģ���������ȡ��ǰ������ַ����ģ��ľ��
        HMODULE hModThisMod = s_ExcludeAPIHookMod
            ? ModuleFromAddress(ReplaceIATEntryInAllMods)
            : NULL;

        HANDLE hthSnapshot = NULL;
        __try {
            // ��ȡĿ����̵İ���ģ����Ϣ�Ŀ���
            hthSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
            if (hthSnapshot == NULL) {
                XLOG(_T("APIHOOK ReplaceIATEntryInAllMods | CreateToolhelp32Snapshot Failed, lastError:%d"), GetLastError());
                __leave;
            }

            MODULEENTRY32W me = { sizeof(me) };
            BOOL bFound = FALSE;
            BOOL bHasMore = Module32FirstW(hthSnapshot, &me);
            while (bHasMore) {
                // ��Hook�����������ģ��
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

    // ��һ��ģ��� ����Σ�import section�����滻һ�����ŵ�ַ
    static void WINAPI ReplaceIATEntryInOneMod(PCSTR pszCalleeModName, PROC pfnOrig, PROC pfnHook, HMODULE hModCaller) {
        // ��ȡģ�鵼���(import section)��ַ
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
            // ���ﲻ���κδ���
            // ���� pImportDesc ֵΪ NULL������ִ��
        }

        if (pImportDesc == NULL) {
            // ģ��û�е���λ��߲��ٱ�����
            return;
        }

        // �ڵ��������в��ұ����ú���������
        // ����ÿһ�������(import section)��ֱ���ҵ����滻Ŀ�꺯��
        for (; pImportDesc->Name; pImportDesc++) {
            PSTR pszModName = (PSTR)((PBYTE)hModCaller + pImportDesc->Name);
            if (lstrcmpiA(pszModName, pszCalleeModName) == 0) {
                // Get caller's import address table (IAT) for the callee's funtions
                PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)((PBYTE)hModCaller + pImportDesc->FirstThunk);

                // ���º�����ַ�滻��ǰ������ַ
                for (; pThunk->u1.Function; pThunk++) {
                    // ��ȡ������ַ
                    PROC* ppfn = (PROC*)&pThunk->u1.Function;

                    // �жϸú����Ƿ�Ϊ��Ҫ���ҵĺ���
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
                        // ���
                        return;
                    }
                }
            }
        }
    }

    // ��һ��ģ��� �����Σ�export sections�����滻һ�����ŵ�ַ
    static void WINAPI ReplaceEATEntryInOneMod(HMODULE hMod, PCSTR pszFuncName, PROC pfnNew) {
        // ��ȡģ�鵼����(export section)�ĵ�ַ
        ULONG ulSize;

        PIMAGE_EXPORT_DIRECTORY pExportDir = NULL;
        __try {
            pExportDir = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryEntryToData(
                hMod, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &ulSize);
        }
        __except (InvalidReadExceptionFilter(GetExceptionInformation())) {
            // ���ﲻ���κδ���
            // �� pExportDir ֵΪ NULL ������£�����ִ��
        }

        if (pExportDir == NULL) {
            // ���ģ��û�е�����(export section)������û�б�����(���Ѿ���ж��)
            return;
        }

        PDWORD pdwNamesRvas = (PDWORD)((PBYTE)hMod + pExportDir->AddressOfNames);
        PWORD pwNameOrdinals = (PWORD)((PBYTE)hMod + pExportDir->AddressOfNameOrdinals);
        PDWORD pdwFunctionAddresses = (PDWORD)((PBYTE)hMod + pExportDir->AddressOfFunctions);

        // ����ģ��ĺ���������
        for (DWORD n = 0; n < pExportDir->NumberOfNames; n++) {
            // ��ȡ������
            PSTR pszNextFuncName = (PSTR)((PBYTE)hMod + pdwNamesRvas[n]);

            // �����������ƥ�䣬����ѭ��
            if (lstrcmpiA(pszNextFuncName, pszFuncName) != 0) {
                continue;
            }

            // ��ȡ�������
            WORD ordinal = pwNameOrdinals[n];

            // ��ȡ������ַ
            PROC* ppfn = (PROC*)&pdwFunctionAddresses[ordinal];

            // ת�������滻�ĺ�����ַ, ��һ�������ģ�����ַ��ƫ����
            pfnNew = (PROC)((PBYTE)pfnNew - (PBYTE)hMod);

            // ��Ŀ�꺯����ַ�滻Ϊ�º�����ַ
            if (!WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnNew, sizeof(pfnNew), NULL)
                && (ERROR_NOACCESS == GetLastError())) {
                DWORD dwOldProtect;
                if (VirtualProtect(ppfn, sizeof(pfnNew), PAGE_WRITECOPY, &dwOldProtect)) {
                    WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnNew, sizeof(pfnNew), NULL);
                    VirtualProtect(ppfn, sizeof(pfnNew), dwOldProtect, &dwOldProtect);
                }
            }

            // ���
            break;
        }
    }

private:
    // ����ɶ�һ������Hook֮���ּ������µ�DLLʱ��ʹ�����º���
    static void WINAPI FixupNewlyLoadedModule(HMODULE hMod, DWORD dwFlags) {
        // ���һ���µ�ģ�鱻���أ��Ը�ģ��Hook����Ӧ�ñ�Hook�ĺ���
        if (hMod == NULL
            || hMod == ModuleFromAddress(FixupNewlyLoadedModule)
            || (dwFlags & LOAD_LIBRARY_AS_DATAFILE) != 0
            || (dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) != 0
            || (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE) != 0) {
            // ���Ե�ǰ��������ģ��
            // ������ԴDLL
            return;
        }

        for (APIHook* p = s_pHead; p != NULL; p = p->m_pNext) {
            if (p->m_pfnOrig != NULL) {
                ReplaceIATEntryInAllMods(p->m_pszCalleeModName, p->m_pfnOrig, p->m_pfnHook);
            }
            else {
#ifdef _DEBUG
                // ��Ӧ�ó��ֵ����
                XLOG(_T("APIHOOK FixupNewlyLoadedModule | impossible to find Function[%s]"),
                    p->m_pszCalleeModName);
#endif
            }
        }
    }

    /*
    * ���� LoadLibraryXX ���������ڲ�������DLL����
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

    // �����ڱ����غ���������ʱ�������滻�ĺ�����ַ
    static FARPROC WINAPI GetProcAddress(HMODULE hMod, PCSTR pszProcName) {
        // ��ȡ������������ַ
        FARPROC pfn = GetProcAddressRaw(hMod, pszProcName);

        // �ж���������ǲ���һ����Hook�ĺ���
        APIHook* p = s_pHead;
        for (; pfn != NULL && p != NULL; p = p->m_pNext) {
            if (pfn == p->m_pfnOrig) {
                // ����һ����Hook�ĺ��������ظú������滻�ĺ�����ַ
                pfn = p->m_pfnHook;
                break;
            }
        }

        return pfn;
    }

private:
    // Hook���º���
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