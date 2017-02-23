#include "stdafx.h"
#include "BaseFunc.h"
#include "ProcessController.h"

#include <WinInet.h>
#include <sstream>
#include <fstream>
#include <strsafe.h>


HMODULE g_hModule = NULL;
LogCallBack g_logCallBack = NULL;

UTIL_API void LogMsgString( WT_EVENTLOG_TYPE emType, const tstring& pszMsg )
{
	LogMsg(emType, _T("%s"), pszMsg.c_str());
}

void LogMsg(WT_EVENTLOG_TYPE emType, LPCTSTR lpszFmt, ...)
{
	tstring strEventType;
	switch (emType)
	{
	case WT_EVENTLOG_SUCCESS_TYPE:
		strEventType = _T("成功");
		break;
	case WT_EVENTLOG_ERROR_TYPE:
		strEventType = _T("错误");
		break;
	case WT_EVENTLOG_WARNING_TYPE:
		strEventType = _T("警告");
		break;
	case WT_EVENTLOG_INFORMATION_TYPE:
		strEventType = _T("信息");
		break;
	default:
		break;
	}

	SYSTEMTIME stLocalTime;
	GetLocalTime(&stLocalTime);
	TCHAR szTime[50] = {0};
	_sntprintf(szTime, 50, _T("[%04d-%02d-%02d %02d:%02d:%02d:%03d] [%s]"), stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
		stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond, stLocalTime.wMilliseconds, strEventType.c_str()); // 写的日期

	tstring strMsg;
	va_list vMarker; 
	va_start(vMarker, lpszFmt);
	int nLen = _vsntprintf(NULL, 0, lpszFmt, vMarker) + 1;
	tstring strRet;
	std::vector<TCHAR> vBuffer(nLen, '\0');
	_vsntprintf(&vBuffer[0], vBuffer.size(), lpszFmt, vMarker);
	strMsg.assign(&vBuffer[0]);
	va_end(vMarker);

	tstring strDirPath = CStringUtil::Format(_T("%slog\\"), CPath::GetModulePath(g_hModule).c_str());
	tstring strLogFilePath = CStringUtil::Format(_T("%sLog_%04d%02d%02d.log"), strDirPath.c_str(), 
		stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay);
	CPath::CreateDirectory(strLogFilePath.c_str(), NULL);

	// 日志文本都以UTF8格式存储
	BOOL bFileCreated = !CPath::IsFileExist(strLogFilePath.c_str());
	FILE* fp = _tfopen(strLogFilePath.c_str(), _T("ab"));
	if (fp != NULL)
	{
		if (bFileCreated)
		{
			CHAR szBomU8[3] = {'\xEF','\xBB','\xBF'};
			fwrite(szBomU8, sizeof(CHAR), 3, fp);
		}
		std::string strWriteU8 = CStringUtil::TStrToUtf8(CStringUtil::Format(_T("%s %s\r\n"), szTime, strMsg.c_str()));
		fwrite(strWriteU8.c_str(), sizeof(CHAR), strWriteU8.length(), fp);
		fclose(fp);
	}

	if (g_logCallBack != NULL)
	{
		g_logCallBack(emType, stLocalTime, strMsg.c_str());
	}
}

void OutDebugString(LPCTSTR lpszFmt, ...)
{
	tstring strMsg;
	va_list vMarker; 
	va_start(vMarker, lpszFmt);
	int nLen = _vsntprintf(NULL, 0, lpszFmt, vMarker) + 1;
	tstring strRet;
	std::vector<TCHAR> vBuffer(nLen, '\0');
	_vsntprintf(&vBuffer[0], vBuffer.size(), lpszFmt, vMarker);
	strMsg.assign(&vBuffer[0]);
	va_end(vMarker);

	OutputDebugString(strMsg.c_str());
}

tstring GetGuid()
{
	GUID guid;
	::CoCreateGuid(&guid);
	return CStringUtil::Format(_T("{%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}")
		,guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

void WaitWithMessageLoop(unsigned long ulMilliseconds)
{
	DWORD dwStart = GetTickCount();
	while (true)
	{
		MSG msg;
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		DWORD dwDruation = GetTickCount() - dwStart;
		if (dwDruation > ulMilliseconds)
		{
			break;
		}
	}
}