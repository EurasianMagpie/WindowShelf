
//  xlog.h

#ifndef _____xx_s_log_header_____
#define _____xx_s_log_header_____

//////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_XLOG


//////////////////////////////////////////////////////////////////////////
#include <string>
#include <algorithm>
#include <tchar.h>

#ifndef tstring
#if defined(_UNICODE ) || defined(UNICODE)
#define tstring    std::wstring
#else
#define tstring    std::string
#endif
#endif

#define XLOG_MAX_LENGTH     4096

class CxLog
{
    class IsEqual
    {
        TCHAR mChar;
    public:
        IsEqual(TCHAR ch)
        {
            mChar = ch;
        }
        bool operator()(TCHAR& val) 
        {
            return val == mChar;
        }
    };

    CxLog()
    {
        m_dwPid = GetCurrentProcessId();
        
        TCHAR szModuleName[2048] = {};
        ::GetModuleFileName(NULL, szModuleName, 2048);
        if(_tcslen(szModuleName))
        {
            tstring strFilePath = szModuleName;
            size_t npos = strFilePath.rfind(_T('\\'));
            m_strRunPath = strFilePath.substr(0, npos);
            m_strProcessName = strFilePath.substr(npos+1);
        }
    }

public:
    static CxLog & getInstance()
    {
        static CxLog xlog;
        return xlog;
    }

public:
    void Log(LPCTSTR lpszFmt, va_list args)
    {
        DWORD tid = GetCurrentThreadId();
        SYSTEMTIME systime = {};
        ::GetLocalTime(&systime);
        TCHAR szpre[256] = {};
        _stprintf_s(szpre, _T("%04d-%02d-%02d %02d:%02d:%02d %03d [%d][%d][%s]"),
            systime.wYear, systime.wMonth, systime.wDay,
            systime.wHour, systime.wMinute, systime.wSecond,
            systime.wMilliseconds,
            m_dwPid, tid, m_strProcessName.c_str());

        tstring strFmt = szpre;
        strFmt += lpszFmt;
        std::replace_if(strFmt.begin(), strFmt.end(), IsEqual(_T('\n')), _T(' '));
        strFmt += _T('\n');
        //_stprintf(m_szBuf, strFmt.c_str(), args);
        _vsntprintf_s(m_szBuf, _countof(m_szBuf), XLOG_MAX_LENGTH, strFmt.c_str(), args);

        ::OutputDebugString(m_szBuf);
    }

    void Log(LPCTSTR lpszFmt, ...)
    {
        va_list argList;
        va_start( argList, lpszFmt );
        Log(lpszFmt, argList);
    }

private:
    TCHAR       m_szBuf[XLOG_MAX_LENGTH];
    DWORD       m_dwLogFlags;

    DWORD       m_dwPid;
    tstring     m_strRunPath;
    tstring     m_strProcessName;
};

#define XLOG    CxLog::getInstance().Log

#else
#define XLOG    sizeof

#endif

#endif//_____xx_s_log_header_____