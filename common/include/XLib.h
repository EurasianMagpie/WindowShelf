#pragma once

#include <Windows.h>

#ifdef __cplusplus

extern "C" {

BOOL WINAPI xiHook(DWORD dwThreadId);

BOOL WINAPI xiUnhook();

}

#endif // __cplusplus
