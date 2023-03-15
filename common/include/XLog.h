
//  xlog.h

#ifndef _____xx_s_log_header_____
#define _____xx_s_log_header_____

//////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_XLOG


//////////////////////////////////////////////////////////////////////////
#include <string>
#include <algorithm>
#include <tchar.h>


#define XLOG_MAX_LENGTH     4096

class CxLog
{
    template<typename CharType>
    class IsEqual
    {
        CharType mChar;
    public:
        IsEqual(CharType ch)
        {
            mChar = ch;
        }
        bool operator()(CharType& val)
        {
            return val == mChar;
        }
    };

    CxLog()
    {
        m_dwPid = GetCurrentProcessId();
        
        CHAR szModuleNameA[2048] = {};
        ::GetModuleFileNameA(NULL, szModuleNameA, 2048);
        if(strlen(szModuleNameA))
        {
            std::string strFilePath = szModuleNameA;
            size_t npos = strFilePath.rfind('\\');
            m_strRunPathA = strFilePath.substr(0, npos);
            m_strProcessNameA = strFilePath.substr(npos+1);
        }

        WCHAR szModuleNameW[2048] = {};
        ::GetModuleFileNameW(NULL, szModuleNameW, 2048);
        if (wcslen(szModuleNameW))
        {
            std::wstring strFilePath = szModuleNameW;
            size_t npos = strFilePath.rfind(L'\\');
            m_strRunPathW = strFilePath.substr(0, npos);
            m_strProcessNameW = strFilePath.substr(npos + 1);
        }
    }

public:
    static CxLog & getInstance()
    {
        static CxLog xlog;
        return xlog;
    }

public:
    void LogA(LPCSTR lpszFmt, va_list args)
    {
        DWORD tid = GetCurrentThreadId();
        SYSTEMTIME systime = {};
        ::GetLocalTime(&systime);
        CHAR szpre[256] = {};
        sprintf_s(szpre, "%04d-%02d-%02d %02d:%02d:%02d %03d [%d][%d][%s]",
            systime.wYear, systime.wMonth, systime.wDay,
            systime.wHour, systime.wMinute, systime.wSecond,
            systime.wMilliseconds,
            m_dwPid, tid, m_strProcessNameA.c_str());

        std::string strFmt = szpre;
        strFmt += lpszFmt;
        std::replace_if(strFmt.begin(), strFmt.end(), IsEqual<CHAR>('\n'), ' ');
        strFmt += '\n';
        _vsnprintf_s(m_szBufA, _countof(m_szBufA), XLOG_MAX_LENGTH, strFmt.c_str(), args);

        ::OutputDebugStringA(m_szBufA);
    }

    void LogA(LPCSTR lpszFmt, ...)
    {
        va_list argList;
        va_start(argList, lpszFmt);
        LogA(lpszFmt, argList);
    }

    void LogW(LPCWSTR lpszFmt, va_list args)
    {
        DWORD tid = GetCurrentThreadId();
        SYSTEMTIME systime = {};
        ::GetLocalTime(&systime);
        TCHAR szpre[256] = {};
        wprintf_s(szpre, L"%04d-%02d-%02d %02d:%02d:%02d %03d [%d][%d][%s]",
            systime.wYear, systime.wMonth, systime.wDay,
            systime.wHour, systime.wMinute, systime.wSecond,
            systime.wMilliseconds,
            m_dwPid, tid, m_strProcessNameW.c_str());

        std::wstring strFmt = szpre;
        strFmt += lpszFmt;
        std::replace_if(strFmt.begin(), strFmt.end(), IsEqual<WCHAR>(L'\n'), L' ');
        strFmt += L'\n';
        _vsnwprintf_s(m_szBufW, _countof(m_szBufW), XLOG_MAX_LENGTH, strFmt.c_str(), args);

        ::OutputDebugStringW(m_szBufW);
    }

    void LogW(LPCWSTR lpszFmt, ...)
    {
        va_list argList;
        va_start( argList, lpszFmt );
        LogW(lpszFmt, argList);
    }

private:
    CHAR            m_szBufA[XLOG_MAX_LENGTH];
    WCHAR           m_szBufW[XLOG_MAX_LENGTH];
    DWORD           m_dwLogFlags;
    DWORD           m_dwPid;
    std::string     m_strRunPathA;
    std::string     m_strProcessNameA;
    std::wstring    m_strRunPathW;
    std::wstring    m_strProcessNameW;
};

#define XLOGW   CxLog::getInstance().LogW
#define XLOGA   CxLog::getInstance().LogA
#if defined(_UNICODE ) || defined(UNICODE)
#define XLOG    CxLog::getInstance().LogW
#else
#define XLOG    CxLog::getInstance().LogA
#endif

#else
#define XLOG    sizeof

#endif//ENABLE_XLOG






#endif//_____xx_s_log_header_____