#pragma once

#include <Windows.h>
#include "../common/include/XLib.h"

/*
* 管理 XLib.dll 的工具类
*/
class XLibHolder {
public:
    static XLibHolder& getXLibHolder() {
        static XLibHolder sXLibHolder;
        return sXLibHolder;
    }

public:
    XLibHolder() : mhModule(NULL) {}

    ~XLibHolder() {
        freeModule();
    }

    HMODULE GetModule() {
        if (!mhModule) {
            mhModule = ::LoadLibrary(XLIB_MODULE_NAME);
        }
        return mhModule;
    }

    HOOKPROC GetHookProc() {
        return (HOOKPROC)GetProcAddress(GetModule(), XLIB_HOOK_PROC);
    }

private:
    void freeModule() {
        if (mhModule) {
            ::FreeLibrary(mhModule);
            mhModule = NULL;
        }
    }

private:
    HMODULE mhModule;
};

#define theXLibHolder XLibHolder::getXLibHolder()