#pragma once

#include "UIMenuEx.h"
#include "SdlWnd.h"

#include "ToolTask.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/Observer.h"
#include "Poco/AutoPtr.h"
#include "Poco/Mutex.h"


using Poco::TaskManager;
using Poco::NotificationCenter;
using Poco::TaskStartedNotification;
using Poco::TaskCancelledNotification;
using Poco::TaskFinishedNotification;
using Poco::TaskFailedNotification;
using Poco::TaskProgressNotification;
using Poco::Observer;
using Poco::AutoPtr;
using Poco::FastMutex;

#define WM_UPDATE_PROGRESS	(WM_USER+1000)
#define WM_LOGOUTPUT		(WM_USER+1001)
#define BUFF_LEN				0x98000

const tstring STR_PLAY = _T("播放");
const tstring STR_PAUSE = _T("暂停");

struct LOG_ITEM_DATA
{
	WT_EVENTLOG_TYPE emType;
	SYSTEMTIME stTime;
	tstring strMsg;
};

class CMainWndDlg : public WindowImplBase
{
	friend CPlayTask;
	friend CParseTask;
public:
	CMainWndDlg(void);
	~CMainWndDlg(void);

public:
	TaskManager m_taskManager;
	mutable FastMutex m_mciMutex;

public:
	LPCTSTR GetWindowClassName() const;

	virtual CDuiString GetSkinFile();
	virtual CDuiString GetSkinFolder();
	virtual void OnFinalMessage(HWND hWnd);
	virtual LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual void InitWindow();
	virtual CControlUI* CreateControl(LPCTSTR pstrClassName);
	LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	void Notify(TNotifyUI& msg);

public:
	static void CALLBACK LogMsgShow(WT_EVENTLOG_TYPE emType, SYSTEMTIME stTime, LPCTSTR lpszMsg);

protected:
	LRESULT OnLogOutput(WPARAM wParam, LPARAM lParam);

protected:
	DUI_DECLARE_MESSAGE_MAP()
	virtual void OnWindowInit(TNotifyUI& msg);
	virtual void OnClick(TNotifyUI& msg);
	virtual void OnSelectChanged(TNotifyUI& msg);
	virtual	void OnSetFocus(TNotifyUI& msg);
	virtual void OnValueChanged(TNotifyUI& msg);

	void taskStarted(TaskStartedNotification* pNf);
	void taskCancelled(TaskCancelledNotification* pNf);
	void taskFinished(TaskFinishedNotification* pNf);
	void taskFailed(TaskFailedNotification* pNf);
	void taskProgress(TaskProgressNotification* pNf);


	void OnTimer(UINT_PTR nIDEvent);

private:
	void InitConfig();
	void InitD3D();

private:
	void ClickPlayBtn(tstring strText, bool bEnabled = true);
	void ClickStopBtn();
	void ClickOpenFileBtn();

private:
	void UpdateProgress();		/*	更新进度条	*/

protected:
	static DWORD WINAPI ParseMediaFile(LPVOID lpvData);

private:
	void LoadConfig(void);


private:
	long m_lCurFrame;
	Task* m_spPlayTask;

private:
	avi_t*	m_pAvi;
	CSdlWnd m_sdlWnd;
	CWndUI* m_pMediaWnd;
	CDirect3d m_d3d;


private:
	/*	标题栏	*/
	CButtonUI* m_pCloseBtn;
	CButtonUI* m_pMaxBtn;
	CButtonUI* m_pRestoreBtn;
	CButtonUI* m_pMinBtn;

	/*	图片信息栏	*/
	CEditUI*	m_pDateEdit;
	CEditUI*	m_pTimeEdit;
	CEditUI*	m_pAddrEdit;
	CEditUI*	m_pLongitudeEdit;
	CEditUI*	m_pLatitudeEdit;

	/*	记录日志	*/
	COptionUI*	m_pLogOption;

	/*	播放区域	*/
	CControlUI* m_pMediaCtrl;

	/*	进度条	*/
	//CProgressUI*	m_pPlayProgress;
	CSliderUI*		m_pPlayProgress;
	CLabelUI*		m_pStartTimeLabel;
	CLabelUI*		m_pEndTimeLabel;

	/*	控制栏	*/
	CButtonUI*	m_pPlayBtn;
	CButtonUI*	m_pStopBtn;
	CButtonUI*	m_pOpenFileBtn;

	/*	日志栏	*/
	CRichEditUI*	m_pLogRichEdit;

private:
	mapString m_mapString;
	tstring	m_strPathName;
};

extern CMainWndDlg* g_pMainWndDlg;

