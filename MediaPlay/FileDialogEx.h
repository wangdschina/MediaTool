#pragma once

#include <commdlg.h>
#include <string>

struct __POSITION {};
typedef __POSITION* POSITION;

class CFileDialogEx
{
public:
	CFileDialogEx(void);
	~CFileDialogEx(void);

public:
	void SetDefExt(LPCTSTR lpszDefExt);
	void SetFileName(LPCTSTR lpszFileName);
	void SetFlags(DWORD dwFlags);
	void SetFilter(LPCTSTR lpszFilter);
	void SetMultiSel(BOOL bMultiSel = TRUE);
	void SetParentWnd(HWND hParentWnd);
	void SetTitle(LPCTSTR lpszTitle);
	void SetFileNameBufferSize(DWORD dwSize);
	HWND GetHwnd();

	BOOL ShowOpenFileDlg();
	BOOL ShowSaveFileDlg();

	tstring GetPathName();
	tstring GetFileName();
	tstring GetFileExt();
	tstring GetFileTitle();
	tstring GetFolderPath();

	POSITION GetStartPosition();
	tstring GetNextPathName(POSITION& pos);
	int GetFileOffset();

public:
	OPENFILENAME m_stOFN;

private:
	TCHAR m_szDefExt[64];
	TCHAR m_szFilter[MAX_PATH];
	TCHAR m_szFileName[MAX_PATH];
	TCHAR * m_lpszFileName;
};
