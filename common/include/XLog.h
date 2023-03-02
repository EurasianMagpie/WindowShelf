
//  xlog.h

#ifndef _____xx_s_log_header_____
#define _____xx_s_log_header_____

//////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_XLOG


//////////////////////////////////////////////////////////////////////////
#include <string>
#include <algorithm>
#include <tchar.h>

#if defined(_UNICODE ) || defined(UNICODE)
#define xlog_tstring    std::wstring
#else
#define xlog_tstring    std::string
#endif

#define XLOG_MAX_LENGTH     4096

class CxLog
{
    class IsEqualxChr
    {
        TCHAR   m_xchr;
    public:
        IsEqualxChr(TCHAR xchr)
        {
            m_xchr = xchr;
        }
        bool operator()( TCHAR & val) 
        {
            return val==m_xchr;
        }
    };

    CxLog()
    {
        m_pid = GetCurrentProcessId();
        
        wchar_t szModuleName[2048] = {};
        ::GetModuleFileNameW(NULL, szModuleName, 2048);
        if(wcslen(szModuleName))
        {
            xlog_tstring strFilePath = szModuleName;
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
        _stprintf_s(szpre, _T("%04d-%02d-%02d %02d:%02d:%02d %03d [%d][%d][%s]"), systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds, m_pid, tid, m_strProcessName.c_str());

        xlog_tstring strFmt = szpre;
        strFmt += lpszFmt;
        std::replace_if(strFmt.begin(), strFmt.end(), IsEqualxChr(_T('\n')), _T(' '));
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
    TCHAR           m_szBuf[XLOG_MAX_LENGTH];
    DWORD           m_dwLogFlags;

    DWORD           m_pid;
    xlog_tstring    m_strRunPath;
    xlog_tstring    m_strProcessName;
};

#define XLOG    CxLog::getInstance().Log

#else
#define XLOG    sizeof

#endif

#endif//_____xx_s_log_header_____