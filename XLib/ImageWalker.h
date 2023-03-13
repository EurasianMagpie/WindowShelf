#pragma once
// ImageWalker.h

#include <Windows.h>
#include <tchar.h>
#include <XLog.h>

namespace ImageWalker {
    VOID WINAPI Walk(HINSTANCE hInstDll) {
        PBYTE pb = NULL;
        MEMORY_BASIC_INFORMATION mbi;
        XLOG(_T("Image | Walker - Begin ---------------------------"));
        while (VirtualQuery(pb, &mbi, sizeof(mbi)) == sizeof(mbi)) {
            int nLen;
            TCHAR szModName[MAX_PATH] = {};
            if (mbi.State == MEM_FREE) {
                mbi.AllocationBase = mbi.BaseAddress;
            }
            
            if ((mbi.AllocationBase == hInstDll)
                || (mbi.AllocationBase != mbi.BaseAddress)
                || (mbi.AllocationBase == NULL)) {
                // ��Щ������ԣ�
                //  1. region���ڵ�ǰ��DLL
                //  2. ��ǰblock����region����ʼblock
                //  3. ��ַΪ��
                nLen = 0;
            }
            else {
                nLen = GetModuleFileName((HINSTANCE)mbi.AllocationBase, szModName, _countof(szModName));
            }

            if (nLen > 0) {
                XLOG(_T("Image | %p - %s"), mbi.AllocationBase, szModName);
            }
            pb += mbi.RegionSize;
        }
        XLOG(_T("Image | Walker - End -----------------------------"));
    }
}
