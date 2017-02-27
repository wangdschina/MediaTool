#include "stdafx.h"
#include "Resource.h"
#include "MainWnd.h"
#include "FileDialogEx.h"

#include "Poco/ThreadPool.h"

using Poco::ThreadPool;


CMainWndDlg* g_pMainWndDlg = nullptr;

//////////////////////////////////////////////////////////////////////////
///
DUI_BEGIN_MESSAGE_MAP(CMainWndDlg, WindowImplBase)
	DUI_ON_MSGTYPE(DUI_MSGTYPE_WINDOWINIT, OnWindowInit)
	DUI_ON_MSGTYPE(DUI_MSGTYPE_CLICK, OnClick)
	DUI_ON_MSGTYPE(DUI_MSGTYPE_SETFOCUS, OnSetFocus)
	DUI_ON_MSGTYPE(DUI_MSGTYPE_VALUECHANGED, OnValueChanged)
DUI_END_MESSAGE_MAP()

CMainWndDlg::CMainWndDlg(void)
{
	g_pMainWndDlg = this;

	m_pCloseBtn = nullptr;
	m_pMaxBtn = nullptr;
	m_pRestoreBtn = nullptr;
	m_pMinBtn = nullptr;

	m_pDateEdit = nullptr;
	m_pTimeEdit = nullptr;
	m_pAddrEdit = nullptr;
	m_pLongitudeEdit = nullptr;
	m_pLatitudeEdit = nullptr;

	m_pMediaCtrl = nullptr;
	m_pPlayProgress = nullptr;
	m_pStartTimeLabel = nullptr;
	m_pEndTimeLabel = nullptr;

	m_pPlayBtn = nullptr;
	m_pStopBtn = nullptr;
	m_pOpenFileBtn = nullptr;
	m_pSaveFileBtn = nullptr;
	m_pLogRichEdit = nullptr;

	m_pMediaWnd = nullptr;
	m_pAvi = nullptr;
	m_spPlayTask = nullptr;
}

CMainWndDlg::~CMainWndDlg(void)
{
	m_mapString.clear();
}

LPCTSTR CMainWndDlg::GetWindowClassName() const
{
	return _T("MediaPlay");
}

CDuiString CMainWndDlg::GetSkinFile()
{
	return _T("main_wnd.xml");
}

CDuiString CMainWndDlg::GetSkinFolder()
{
	return CDuiString(CPaintManagerUI::GetInstancePath() + _T("Skin"));
}

void CMainWndDlg::OnFinalMessage(HWND hWnd)
{
	WindowImplBase::OnFinalMessage(hWnd);
	delete this;
}

LRESULT CMainWndDlg::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// 处理菜单栏关闭消息
	if (wParam == SC_CLOSE)
	{
		Close();
		::PostQuitMessage(0L);
		bHandled = TRUE;
		return 0;
	}

	LRESULT lRes = CWindowWnd::HandleMessage(uMsg, wParam, lParam);
	return lRes;
}

void CMainWndDlg::InitWindow()
{
	m_pCloseBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("closeBtn")));
	m_pMaxBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("maxBtn")));
	m_pRestoreBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("restoreBtn")));
	m_pMinBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("minBtn")));

	m_pDateEdit = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("dateEdit")));
	m_pTimeEdit = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("timeEdit")));
	m_pAddrEdit = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("addrEdit")));
	m_pLongitudeEdit = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("longitudeEdit")));
	m_pLatitudeEdit = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("latitudeEdit")));

	m_pLogOption = static_cast<COptionUI*>(m_PaintManager.FindControl(_T("logOption")));

	m_pMediaCtrl = static_cast<CControlUI*>(m_PaintManager.FindControl(_T("mediaCtrl")));
	m_pPlayProgress = static_cast<CSliderUI*>(m_PaintManager.FindControl(_T("playProgress")));
	m_pStartTimeLabel = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("startTimeLabel")));
	m_pEndTimeLabel = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("endTimeLabel")));

	m_pPlayBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("playBtn")));
	m_pStopBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("stopBtn")));
	m_pOpenFileBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("openFileBtn")));
	m_pSaveFileBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("saveFileBtn")));
	m_pLogRichEdit = static_cast<CRichEditUI*>(m_PaintManager.FindControl(_T("logRichEdit")));

	WindowImplBase::InitWindow();
}

CControlUI* CMainWndDlg::CreateControl( LPCTSTR pstrClassName )
{
	if (_tcsicmp(pstrClassName, _T("WndMediaDisplay")) == 0)
	{
		CWndUI *pUI = new CWndUI;
		HWND   hWnd = CreateWindow(_T("#32770"), _T("WndMediaDisplay"), WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, m_PaintManager.GetPaintWindow(), (HMENU)0, NULL, NULL);
		pUI->Attach(hWnd);
		return pUI;
	}

	return nullptr;
}

LRESULT CMainWndDlg::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = TRUE; // 消息本地处理，不流入系统处理。
	switch (uMsg)
	{
	case WM_SIZE:
		switch (wParam)
		{
		case SIZE_RESTORED: // 捕获到窗口还原消息
			{
				if (m_PaintManager.GetRoot() != NULL)
				{
					if (m_pMaxBtn != NULL) m_pMaxBtn->SetVisible(true);
					if (m_pRestoreBtn != NULL) m_pRestoreBtn->SetVisible(false);
				}
			}
			break;
		case SIZE_MAXIMIZED: // 捕获到窗口最大化消息
			{
				if (m_PaintManager.GetRoot() != NULL)
				{
					if (m_pMaxBtn != NULL) m_pMaxBtn->SetVisible(false);
					if (m_pRestoreBtn != NULL) m_pRestoreBtn->SetVisible(true);
				}
			}
			break;
		default:
			{
				
			}
			break;
		}
		bHandled = FALSE;
		break;
	case WM_PAINT:
		{
			if (m_pMediaWnd != nullptr && m_pMediaCtrl)
			{
				m_pMediaWnd->SetPos(m_pMediaCtrl->GetPos());
			}
		}
		bHandled = FALSE;
		break;
	case WM_MENUEXCLICK:
		{
			CControlUI * pControl = (CControlUI *)lParam;
			if (pControl != NULL)
			{
				m_PaintManager.SendNotify(pControl, DUI_MSGTYPE_MENUCLICK, wParam, lParam);
			}
		}
		break;
	case WM_TIMER:
		{
			this->OnTimer((UINT_PTR)wParam);
			bHandled = FALSE;
		}
		break;
	case WM_LOGOUTPUT:
		{
			this->OnLogOutput(wParam, lParam);
		}
		break;
	case WM_UPDATE_PROGRESS:
		{
			this->UpdateProgress();
		}
		bHandled = FALSE;
		break;
	default:
		bHandled = FALSE; // 流入系统处理
	}

	return 0;
}

void CMainWndDlg::Notify(TNotifyUI& msg)
{
	return WindowImplBase::Notify(msg);
}

void CMainWndDlg::OnWindowInit( TNotifyUI& msg )
{
	g_logCallBack = CMainWndDlg::LogMsgShow;

	this->InitConfig();

	// 增加默认线程池的容量
	ThreadPool::defaultPool().addCapacity(320);

	// 添加任务观察者
	m_taskManager.addObserver(Observer<CMainWndDlg, TaskStartedNotification>(*this, &CMainWndDlg::taskStarted));
	m_taskManager.addObserver(Observer<CMainWndDlg, TaskCancelledNotification>(*this, &CMainWndDlg::taskCancelled));
	m_taskManager.addObserver(Observer<CMainWndDlg, TaskFailedNotification>(*this, &CMainWndDlg::taskFailed));
	m_taskManager.addObserver(Observer<CMainWndDlg, TaskFinishedNotification>(*this, &CMainWndDlg::taskFinished));
	m_taskManager.addObserver(Observer<CMainWndDlg, TaskProgressNotification>(*this, &CMainWndDlg::taskProgress));
}

void CMainWndDlg::OnClick( TNotifyUI& msg )
{
	if (msg.pSender == m_pCloseBtn)
	{
		Close();
		::PostQuitMessage(0L);
		return;
	}
	else if (msg.pSender == m_pMinBtn)
	{
		this->SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
	}
	else if (msg.pSender == m_pMaxBtn)
	{
		this->SendMessage(WM_SYSCOMMAND, SC_MAXIMIZE, 0);
	}
	else if (msg.pSender == m_pRestoreBtn)
	{
		this->SendMessage(WM_SYSCOMMAND, SC_RESTORE, 0);
	}
	else if (msg.pSender == m_pPlayProgress)
	{
	}
	else if (msg.pSender == m_pPlayBtn)
	{
		if (m_pPlayBtn->GetToolTip().CompareNoCase(STR_PLAY.c_str()) == 0)
		{
			if (m_pPlayProgress->GetValue() == 100)
			{
				return;
			}
			this->ClickPlayBtn(STR_PAUSE, true);
		}
		else
		{
			this->ClickPlayBtn(STR_PLAY, true);
		}
	}
	else if (msg.pSender == m_pStopBtn)
	{
		this->ClickStopBtn();
	}
	else if (msg.pSender == m_pOpenFileBtn)
	{
		this->ClickOpenFileBtn();
	}
	else if (msg.pSender == m_pSaveFileBtn)
	{
		this->ClickSaveFileBtn();
	}
	else if (msg.pSender == m_pPlayProgress)
	{
		::MessageBox(NULL, _T(""), _T(""), 0);
	}
}

void CMainWndDlg::InitConfig()
{
	//m_pPlayBtn->SetEnabled(false);
	this->InitD3D();
}


void CMainWndDlg::LoadConfig(void)
{
	m_mapString.clear();

	/*	配置文件名	*/
	tstring strFileName = CPath::GetModulePath(CPaintManagerUI::GetInstance());
	strFileName = CStringUtil::Format(_T("%s\\%s"), strFileName.c_str(), cstrConfName.c_str());

	//////////////////////////////////////////////////////////////////////////
	/*	获取文件内容	*/

	/*	获取 [CWREG] SECTION	*/
	TCHAR szAutoExe[MAX_PATH] = { 0 };
	GetPrivateProfileString(_T("CWREG"), cstrAutoRun.c_str(), _T(""), szAutoExe, MAX_PATH, strFileName.c_str());
	m_mapString.insert(std::make_pair(cstrAutoRun, szAutoExe));

	//////////////////////////////////////////////////////////////////////////
}

void CMainWndDlg::OnSelectChanged( TNotifyUI& msg )
{

}

void CMainWndDlg::ClickPlayBtn( tstring strText, bool bEnabled /*= true*/ )
{
	m_pPlayBtn->SetEnabled(bEnabled);
	if (strText.compare(STR_PLAY) == 0)
	{
		m_pPlayBtn->SetNormalImage(_T("file='res/btn_play.png' source='0,0,42,27' corner='2,2,2,2'"));
		m_pPlayBtn->SetHotImage(_T("file='res/btn_play.png' source='42,0,84,27' corner='2,2,2,2'"));
		m_pPlayBtn->SetPushedImage(_T("file='res/btn_play.png' source='84,0,126,27' corner='2,2,2,2'"));
		m_pPlayBtn->SetDisabledImage(_T("file='res/btn_play.png' source='126,0,168,27' corner='2,2,2,2'"));
	}
	else
	{
		m_pPlayBtn->SetNormalImage(_T("file='res/btn_pause.png' source='0,0,42,27' corner='2,2,2,2'"));
		m_pPlayBtn->SetHotImage(_T("file='res/btn_pause.png' source='42,0,84,27' corner='2,2,2,2'"));
		m_pPlayBtn->SetPushedImage(_T("file='res/btn_pause.png' source='84,0,126,27' corner='2,2,2,2'"));
		m_pPlayBtn->SetDisabledImage(_T("file='res/btn_pause.png' source='126,0,168,27' corner='2,2,2,2'"));

		
		::ShowWindow(m_pMediaWnd->GetHWND(), SW_NORMAL);
		::SetTimer(m_hWnd, 1, 500, NULL);
	}

	m_pPlayBtn->SetToolTip(strText.c_str());
}

void CMainWndDlg::ClickStopBtn()
{
	::KillTimer(m_hWnd, 1);

	this->ClickPlayBtn(STR_PLAY, false);

	/*	取消任务，清除资源	*/
	m_taskManager.cancelAll();
	m_spPlayTask = nullptr;
	m_d3d.Clear();
	FastMutex::ScopedLock lock(m_mciMutex);
	if (m_pAvi != NULL)
	{
		AVI_close(m_pAvi);
		m_pAvi = NULL;
	}

	
	if (m_pMediaWnd != nullptr)
	{
		::ShowWindow(m_pMediaWnd->GetHWND(), SW_HIDE);
	}
	m_pPlayProgress->SetEnabled(false);
	m_pPlayProgress->SetValue(0);

	m_pStartTimeLabel->SetText(_T(""));
	m_pEndTimeLabel->SetText(_T(""));
}

void CMainWndDlg::ClickOpenFileBtn()
{
	this->ClickStopBtn();

	CFileDialogEx openFileDlg;
	openFileDlg.SetTitle(_T("打开视频文件"));
	openFileDlg.SetFilter(_T("视频文件\0**.avi;\0\0"));
	openFileDlg.SetParentWnd(m_hWnd);
	if (openFileDlg.ShowOpenFileDlg())
	{
		m_strPathName = openFileDlg.GetPathName();
		if (m_pMediaCtrl != nullptr)
		{
			m_pMediaCtrl->SetVisible(true);

			m_taskManager.start(new CParseTask);
		}
	}
}

void CMainWndDlg::ClickSaveFileBtn()
{
	FastMutex::ScopedLock lock(m_mciMutex);

	USERDATA userData;
	m_d3d.Front(userData);
	if (userData.ulWidth == 0)
	{
		LogMsg(WT_EVENTLOG_ERROR_TYPE, _T("未获取到图片信息"));
		return;
	}
	
	/*	 位图文件头		*/
	BITMAPFILEHEADER bmpFileHeader;
	ZeroMemory(&bmpFileHeader, sizeof(BITMAPFILEHEADER));
	bmpFileHeader.bfType = 0x4d42;
	bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*256 + userData.ulWidth*userData.ulHeight/*userData.ulHeight*userData.ulYStride + userData.ulHeight* userData.ulUVStride/2 + userData.ulHeight* userData.ulUVStride/2*/;
	bmpFileHeader.bfReserved1 = 0;
	bmpFileHeader.bfReserved2 = 0;
	bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*256;

	/*	位图信息头		*/
	BITMAPINFOHEADER bmp_infoheader;
	ZeroMemory(&bmp_infoheader, sizeof(BITMAPINFOHEADER));
	bmp_infoheader.biSize = sizeof(BITMAPINFOHEADER);
	bmp_infoheader.biWidth = userData.ulWidth;
	bmp_infoheader.biHeight = userData.ulHeight;
	bmp_infoheader.biPlanes = 1;
	bmp_infoheader.biBitCount = 8;
	bmp_infoheader.biCompression = BI_RGB;
	bmp_infoheader.biSizeImage = 0;			//图像大小，无压缩的数据，这里可以为0
	bmp_infoheader.biXPelsPerMeter = 0;
	bmp_infoheader.biYPelsPerMeter = 0;
	bmp_infoheader.biClrUsed = 0;
	bmp_infoheader.biClrImportant = 0;

	//构造灰度图的调色版
	RGBQUAD rgbquad[256];
	for(int i = 0; i < 256; i++)
	{
		rgbquad[i].rgbBlue = i;
		rgbquad[i].rgbGreen = i;
		rgbquad[i].rgbRed = i;
		rgbquad[i].rgbReserved = 0;
	}

	SYSTEMTIME stLocalTime;
	GetLocalTime(&stLocalTime);

	tstring strDirPath = CStringUtil::Format(_T("%simage\\"), CPath::GetModulePath(g_hModule).c_str());
	tstring strImageFilePath = CStringUtil::Format(_T("%s%04d%02d%02d%02d%02d%02d.bmp"), strDirPath.c_str(), 
		stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay, stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond);
	CPath::CreateDirectoryW(strImageFilePath.c_str(), NULL);

	BOOL bFileCreated = !CPath::IsFileExist(strImageFilePath.c_str());
	FILE* fp = _tfopen(strImageFilePath.c_str(), _T("wb"));
	if (fp != NULL)
	{
		fwrite(&bmpFileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
		fwrite(&bmp_infoheader, sizeof(BITMAPINFOHEADER), 1, fp);
		fwrite(&rgbquad, sizeof(RGBQUAD)*256, 1, fp);

		for (int n = userData.ulHeight - 1; n >= 0; --n)
		{
			fwrite(userData.pszData + n*userData.ulWidth, userData.ulWidth, 1, fp);
		}

		fclose(fp);

		LogMsg(WT_EVENTLOG_INFORMATION_TYPE, _T("图片保存至 %s%s"), strDirPath.c_str(), strImageFilePath.c_str());
	}
}

void CMainWndDlg::OnTimer( UINT_PTR nIDEvent )
{
	if (nIDEvent == 1)
	{
		this->UpdateProgress();
	}
}


void CMainWndDlg::UpdateProgress()
{
	if (m_pAvi == nullptr)
	{
		return;
	}

	FastMutex::ScopedLock lock(m_mciMutex);

	m_pPlayProgress->SetEnabled(true);
	if (m_pPlayProgress->GetValue() == 100 && m_d3d.GetPos() == 0)
	{
		::KillTimer(m_hWnd, 1);
		this->ClickPlayBtn(STR_PLAY, true);
		return;
	}

	long lTotalFrames = m_pAvi->video_frames;
	double fFps = m_pAvi->fps;

	/*	更新总共时间	*/
	if (fFps <= 0)
	{
		fFps = 1.0f;
	}
	long lTotalSeconds = (double)lTotalFrames/fFps;
	if (lTotalSeconds/60 >= 100)
	{
		m_pEndTimeLabel->SetText(CStringUtil::Format(_T("%03d:%02d"), lTotalSeconds/60, lTotalSeconds%60).c_str());
	}
	else
	{
		m_pEndTimeLabel->SetText(CStringUtil::Format(_T("%02d:%02d"), lTotalSeconds/60, lTotalSeconds%60).c_str());
	}
	

	/*	更新当前播放时间	*/
	long lCurPos = m_d3d.GetPos();
	long lCurSeconds = (double)lCurPos/fFps;
	m_pStartTimeLabel->SetText(CStringUtil::Format(_T("%02d:%02d"), lCurSeconds/60, lCurSeconds%60).c_str());

	/*	更新进度条	*/
	if (lTotalSeconds > 0)
	{
		m_pPlayProgress->SetValue(lCurSeconds*100/lTotalSeconds);
	}
	else
	{
		m_pPlayProgress->SetValue(0);
	}
}

void CMainWndDlg::OnSetFocus( TNotifyUI& msg )
{
	if (msg.pSender == m_pPlayProgress)
	{
	}
}

void CMainWndDlg::LogMsgShow( WT_EVENTLOG_TYPE emType, SYSTEMTIME stTime, LPCTSTR lpszMsg )
{
	if (g_pMainWndDlg != NULL)
	{
		LOG_ITEM_DATA* pLogItemData = new LOG_ITEM_DATA();
		pLogItemData->emType = emType;
		pLogItemData->stTime = stTime;
		pLogItemData->strMsg = lpszMsg;

		if (!g_pMainWndDlg->PostMessage(WM_LOGOUTPUT, (WPARAM)pLogItemData, NULL)) 
		{ 
			// 窗口消息队列满会导致发送失败
			delete pLogItemData;
			pLogItemData = NULL;
		}
	}
}

LRESULT CMainWndDlg::OnLogOutput( WPARAM wParam, LPARAM lParam )
{
	LOG_ITEM_DATA* pLogItemData = (LOG_ITEM_DATA*)wParam;
	if (pLogItemData != NULL)
	{
		tstring strEventType;
		switch (pLogItemData->emType)
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

		tstring strMsgHeader, strMsgContent;

		strMsgHeader = CStringUtil::Format(_T("%04d-%02d-%02d %02d:%02d:%02d [%s]\r\n"), pLogItemData->stTime.wYear, 
			pLogItemData->stTime.wMonth, pLogItemData->stTime.wDay, pLogItemData->stTime.wHour, 
			pLogItemData->stTime.wMinute, pLogItemData->stTime.wSecond, strEventType.c_str());

		CHARFORMAT2 cfHeader;
		ZeroMemory(&cfHeader, sizeof(CHARFORMAT2));
		cfHeader.cbSize = sizeof(CHARFORMAT2);
		cfHeader.dwMask = CFM_BOLD | CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_SIZE | CFM_UNDERLINE;
		_tccpy(cfHeader.szFaceName, _T("宋体")); // 设置字体
		cfHeader.dwEffects = 0;
		cfHeader.yHeight = 210; // 文字高度
		cfHeader.crTextColor = RGB(0, 110, 254); // 文字颜色

		CHARFORMAT2 cfContent;
		ZeroMemory(&cfContent, sizeof(CHARFORMAT2));
		cfContent.cbSize = sizeof(CHARFORMAT2);
		cfContent.dwMask = CFM_BOLD | CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_SIZE | CFM_UNDERLINE;
		_tccpy(cfContent.szFaceName, _T("宋体")); // 设置字体
		cfContent.dwEffects = 0;
		cfContent.yHeight = 180; // 文字高度
		cfContent.crTextColor = RGB(0, 0, 0); // 文字颜色

		PARAFORMAT2 pfHeader;
		ZeroMemory(&pfHeader, sizeof(PARAFORMAT2));
		pfHeader.cbSize = sizeof(PARAFORMAT2);
		pfHeader.dwMask =  PFM_STARTINDENT | PFM_OFFSET;
		pfHeader.dxStartIndent = 0;
		pfHeader.dxOffset = 0;

		PARAFORMAT2 pfContent;
		ZeroMemory(&pfContent, sizeof(PARAFORMAT2));
		pfContent.cbSize = sizeof(PARAFORMAT2);
		pfContent.dwMask =  PFM_STARTINDENT | PFM_OFFSET;
		pfContent.dxStartIndent = 200;
		pfContent.dxOffset = 0;

		if (pLogItemData->strMsg.length() > 0 && pLogItemData->strMsg[pLogItemData->strMsg.length()-1]  == _T('\n'))
		{
			strMsgContent = CStringUtil::Format(_T("%s"), pLogItemData->strMsg.c_str());
		}
		else
		{
			strMsgContent = CStringUtil::Format(_T("%s\r\n"), pLogItemData->strMsg.c_str());
		}


		m_pLogRichEdit->SetSel(-1, -1);
		m_pLogRichEdit->SetSelectionCharFormat(cfHeader);
		m_pLogRichEdit->SetParaFormat(pfHeader);
		m_pLogRichEdit->ReplaceSel(strMsgHeader.c_str(), false);

		switch (pLogItemData->emType)
		{
		case WT_EVENTLOG_SUCCESS_TYPE:
			cfContent.crTextColor = RGB(60, 180, 117); // 绿色
			break;
		case WT_EVENTLOG_ERROR_TYPE:
			cfContent.crTextColor = RGB(254, 0, 2); // 红色
			break;
		case WT_EVENTLOG_WARNING_TYPE:
			cfContent.crTextColor = RGB(218, 69, 39); // 棕色
			break;
		case WT_EVENTLOG_INFORMATION_TYPE:
			break;
		default:
			break;
		}
		m_pLogRichEdit->SetSel(-1, -1);
		m_pLogRichEdit->SetSelectionCharFormat(cfContent);
		m_pLogRichEdit->SetParaFormat(pfContent);
		m_pLogRichEdit->ReplaceSel(strMsgContent.c_str(), false);

		/*if (m_pLogAutoClearOption->IsSelected())
		{
			if (m_pLogRichEdit->GetLineCount() >= 3000)
			{
				int nSelEnd = m_pLogRichEdit->LineIndex(1000); 
				m_pLogRichEdit->SetSel(0, nSelEnd);
				m_pLogRichEdit->ReplaceSel(_T(""), false);
			}
		}*/

		//if (m_pLogAutoScrollOption->IsSelected())
		{
			m_pLogRichEdit->EndDown();
		}

		delete pLogItemData;
	}

	return 0;
}



void CMainWndDlg::InitD3D()
{
	m_pMediaWnd = static_cast<CWndUI*>(m_PaintManager.FindControl(_T("mediaCtrl")));
	ASSERT(m_pMediaWnd != nullptr);

	m_pMediaWnd->SetPos(m_pMediaCtrl->GetPos());

	//m_d3d.InitD3D(m_pMediaWnd->GetHWND(), 1920, 1080);


	//m_sdlWnd.m_emStatus = CSdlWnd::E_INIT;
	//if (m_sdlWnd.Init() < 0)
	//{
	//	LogMsg(WT_EVENTLOG_ERROR_TYPE, _T("SDL 初始化失败 [%s]"), CStringUtil::AnsiToTStr(SDL_GetError()).c_str());
	//	m_sdlWnd.Quit();
	//	return;
	//}

	//CWndUI* pWnd = static_cast<CWndUI*>(m_PaintManager.FindControl(_T("mediaCtrl")));
	//ASSERT(pWnd != nullptr);
	//m_sdlWnd.m_pWnd = m_sdlWnd.CreateWnd(pWnd->GetHWND());
	//if (m_sdlWnd.m_pWnd == nullptr)
	//{
	//	LogMsg(WT_EVENTLOG_ERROR_TYPE, _T("创建窗口失败	[%s]"), CStringUtil::AnsiToTStr(SDL_GetError()).c_str());
	//	m_sdlWnd.Quit();
	//	return;
	//}

	//m_sdlWnd.m_pRender = m_sdlWnd.CreateRender();
	//if (m_sdlWnd.m_pRender == nullptr)
	//{
	//	LogMsg(WT_EVENTLOG_ERROR_TYPE, _T("创建渲染器失败 [%s]"), CStringUtil::AnsiToTStr(SDL_GetError()).c_str());
	//	m_sdlWnd.Quit();
	//	return;
	//}

	//m_sdlWnd.m_pTexture = m_sdlWnd.CreateTexture();
	//if (m_sdlWnd.m_pTexture == nullptr)
	//{
	//	LogMsg(WT_EVENTLOG_ERROR_TYPE, _T("创建纹理失败	[%s]"), CStringUtil::AnsiToTStr(SDL_GetError()).c_str());
	//	m_sdlWnd.Quit();
	//	return;
	//}

	////启动线程模拟视频数据更新  
	////SDL_Thread *video_thread = SDL_CreateThread(VideoThread, NULL, this);  
	//std::thread video_thread(VideoThread, this);

	////消息循环  
	//SDL_Event event;  
	//while (m_sdlWnd.m_emStatus != CSdlWnd::E_STOP)
	//{  
	//	SDL_WaitEvent(&event);  
	//	switch (event.type)  
	//	{  
	//	case SDL_QUIT:  
	//		m_sdlWnd.m_emStatus = CSdlWnd::E_REQ;  
	//		break;  
	//	}  
	//}  
}

void CMainWndDlg::taskStarted( TaskStartedNotification* pNf )
{
	LogMsg(WT_EVENTLOG_INFORMATION_TYPE, _T("Started"));
	pNf->release();
}

void CMainWndDlg::taskCancelled( TaskCancelledNotification* pNf )
{
	LogMsg(WT_EVENTLOG_INFORMATION_TYPE, _T("Cancelled"));
	pNf->release();
}

void CMainWndDlg::taskFinished( TaskFinishedNotification* pNf )
{
	LogMsg(WT_EVENTLOG_INFORMATION_TYPE, _T("Finished"));
	pNf->release();
}

void CMainWndDlg::taskFailed( TaskFailedNotification* pNf )
{
	LogMsg(WT_EVENTLOG_INFORMATION_TYPE, _T("Failed"));
	pNf->release();
}

void CMainWndDlg::taskProgress( TaskProgressNotification* pNf )
{
	LogMsg(WT_EVENTLOG_INFORMATION_TYPE, _T("Progress:%f"), pNf->progress());
	pNf->release();
}

void CMainWndDlg::OnValueChanged( TNotifyUI& msg )
{
	if (msg.pSender == m_pPlayProgress)
	{
		if (m_pPlayProgress->IsEnabled())
		{
			FastMutex::ScopedLock lock(m_mciMutex);

			POINT pt = msg.ptMouse;
			RECT rect = m_pPlayProgress->GetPos();
			if (pt.x >= rect.left && pt.x <= rect.right)
			{
				/*	设置进度条值	*/
				long lDis = pt.x - rect.left;
				long lWid = rect.right - rect.left;
				double d = ((double)lDis)/((double)lWid);
				int nPer = d*100.0;
				m_pPlayProgress->SetValue(nPer);

				m_d3d.Clear();
				/*	设置播放画面	*/
				m_lCurFrame = m_pAvi->video_frames*d;
				AVI_set_video_position(m_pAvi, m_lCurFrame, nullptr);
				HI_U8  buf[BUFF_LEN];
				HI_U32	len = -1;
				while ((len = AVI_read_frame(m_pAvi, (char*)buf)) > 0)
				{
					if ((buf[4] & 0x1f) == 0x07)	//	找SPS
					{
						break;
					}
				}

				/*	设置用户数据	*/
				m_pAddrEdit->SetText(_T("深之蓝"));
			}
			this->ClickPlayBtn(STR_PAUSE, true);
		}
		else
		{
			m_pPlayProgress->SetValue(0);
		}
	}
	else
	{
		m_pAddrEdit->SetText(_T(""));
	}
}
