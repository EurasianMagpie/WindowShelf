#pragma once

#include <Windows.h>
#include <string>
#include <XLib.h>

#ifndef tstring
#if defined(_UNICODE ) || defined(UNICODE)
#define tstring    std::wstring
#else
#define tstring    std::string
#endif
#endif

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
            mhModule = ::LoadLibrary(GetFilePath());
        }
        return mhModule;
    }

    HOOKPROC GetHookProc() {
        return (HOOKPROC)GetProcAddress(GetModule(), XLIB_HOOK_PROC);
    }

    LPCTSTR GetFilePath() {
        if (mModuleFilePath.size() == 0) {
            TCHAR szModuleName[2048] = {};
            ::GetModuleFileName(NULL, szModuleName, 2048);
            if (_tcslen(szModuleName))
            {
                tstring strFilePath = szModuleName;
                size_t npos = strFilePath.rfind(_T('\\'));
                mModuleFilePath = strFilePath.substr(0, npos);
                mModuleFilePath.append(_T("\\"));
                mModuleFilePath.append(XLIB_MODULE_NAME);
            }
        }
        return mModuleFilePath.c_str();
    }

private:
    void freeModule() {
        if (mhModule) {
            ::FreeLibrary(mhModule);
            mhModule = NULL;
        }
    }

private:
    tstring mModuleFilePath;
    HMODULE mhModule;
};

#define theXLibHolder XLibHolder::getXLibHolder()