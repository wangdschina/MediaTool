#pragma once

#include <process.h>
#include <string>

class UTIL_API CProcessController
{
public:
	CProcessController();
	~CProcessController();

public:
	bool Start(const TCHAR* pszAppliactionName, TCHAR* pszCommandLine);
	bool Start(const TCHAR* pszAppliactionName, TCHAR* pszCommandLine, const TCHAR* pszCaption);
	bool IsRunning();
	void SetAutoExit(bool bAutoExit);
	DWORD WaitForExit(DWORD dwMilliseconds);
	DWORD GetExitCode();
	void Terminate();

public:
	PROCESS_INFORMATION m_pi;   
	HWND m_hMainWnd;
	tstring m_strCaption;

private:
	bool m_bAutoExit;
};

