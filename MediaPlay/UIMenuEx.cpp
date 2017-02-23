#include "stdafx.h"
#include "UIMenuEx.h"

namespace DuiLib {

/////////////////////////////////////////////////////////////////////////////////////
//
ContextMenuObserver s_context_menu_observer;

CMenuExUI::CMenuExUI()
{
	if (GetHeader() != NULL)
		GetHeader()->SetVisible(false);
}

CMenuExUI::~CMenuExUI()
{}

LPCTSTR CMenuExUI::GetClass() const
{
    return _T("MenuUI");
}

LPVOID CMenuExUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcsicmp(pstrName, DUI_CTR_MENU) == 0 ) return static_cast<CMenuExUI*>(this);
    return CListUI::GetInterface(pstrName);
}

void CMenuExUI::DoEvent(TEventUI& event)
{
	return __super::DoEvent(event);
}

bool CMenuExUI::Add(CControlUI* pControl)
{
	CMenuExElementUI* pMenuItem = static_cast<CMenuExElementUI*>(pControl->GetInterface(DUI_CTR_MENUELEMENT));
	if (pMenuItem == NULL)
		return false;

	for (int i = 0; i < pMenuItem->GetCount(); ++i)
	{
		if (pMenuItem->GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT) != NULL)
		{
			(static_cast<CMenuExElementUI*>(pMenuItem->GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT)))->SetInternVisible(false);
		}
	}
	return CListUI::Add(pControl);
}

bool CMenuExUI::AddAt(CControlUI* pControl, int iIndex)
{
	CMenuExElementUI* pMenuItem = static_cast<CMenuExElementUI*>(pControl->GetInterface(DUI_CTR_MENUELEMENT));
	if (pMenuItem == NULL)
		return false;

	for (int i = 0; i < pMenuItem->GetCount(); ++i)
	{
		if (pMenuItem->GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT) != NULL)
		{
			(static_cast<CMenuExElementUI*>(pMenuItem->GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT)))->SetInternVisible(false);
		}
	}
	return CListUI::AddAt(pControl, iIndex);
}

int CMenuExUI::GetItemIndex(CControlUI* pControl) const
{
	CMenuExElementUI* pMenuItem = static_cast<CMenuExElementUI*>(pControl->GetInterface(DUI_CTR_MENUELEMENT));
	if (pMenuItem == NULL)
		return -1;

	return __super::GetItemIndex(pControl);
}

bool CMenuExUI::SetItemIndex(CControlUI* pControl, int iIndex)
{
	CMenuExElementUI* pMenuItem = static_cast<CMenuExElementUI*>(pControl->GetInterface(DUI_CTR_MENUELEMENT));
	if (pMenuItem == NULL)
		return false;

	return __super::SetItemIndex(pControl, iIndex);
}

bool CMenuExUI::Remove(CControlUI* pControl)
{
	CMenuExElementUI* pMenuItem = static_cast<CMenuExElementUI*>(pControl->GetInterface(DUI_CTR_MENUELEMENT));
	if (pMenuItem == NULL)
		return false;

	return __super::Remove(pControl);
}

SIZE CMenuExUI::EstimateSize(SIZE szAvailable)
{
	int cxFixed = 0;
    int cyFixed = 0;
    for( int it = 0; it < GetCount(); it++ ) {
        CControlUI* pControl = static_cast<CControlUI*>(GetItemAt(it));
        if( !pControl->IsVisible() ) continue;
        SIZE sz = pControl->EstimateSize(szAvailable);
        cyFixed += sz.cy;
		if( cxFixed < sz.cx )
			cxFixed = sz.cx;
    }
	cyFixed += 12;
	cxFixed += 20;
    return CSize(cxFixed, cyFixed);
}

void CMenuExUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
	CListUI::SetAttribute(pstrName, pstrValue);
}

/////////////////////////////////////////////////////////////////////////////////////
//
class CMenuBuilderCallback: public IDialogBuilderCallback
{
	CControlUI* CreateControl(LPCTSTR pstrClass)
	{
		if (_tcsicmp(pstrClass, DUI_CTR_MENU) == 0)
		{
			return new CMenuExUI();
		}
		else if (_tcsicmp(pstrClass, DUI_CTR_MENUELEMENT) == 0)
		{
			return new CMenuExElementUI();
		}
		return NULL;
	}
};

CMenuExWnd::CMenuExWnd(HWND hParent):
m_hParent(hParent),
m_pOwner(NULL),
m_pLayout(),
m_xml(_T("")),
m_pParentCtrl(NULL)
{}

BOOL CMenuExWnd::Receive(ContextMenuExParam param)
{
	switch (param.wParam)
	{
	case 1:
		{
			Close();
			if (param.lParam != NULL && m_pParentCtrl != NULL)
			{
				m_pParentCtrl->SetUserData((LPCTSTR)param.lParam);
				::PostMessage(m_hParent, WM_MENUEXCLICK, NULL, (LPARAM)m_pParentCtrl);
			}
		}
		break;
	case 2:
		{
			HWND hParent = GetParent(m_hWnd);
			while (hParent != NULL)
			{
				if (hParent == param.hWnd)
				{
					Close();
					break;
				}
				hParent = GetParent(hParent);
			}
		}
		break;
	default:
		break;
	}

	return TRUE;
}

void CMenuExWnd::EnableMenuItem(LPCTSTR lpszName, BOOL bEnable)
{
	CMenuExElementUI* pControl = (CMenuExElementUI*)m_pm.FindControl(lpszName);
	if (pControl != NULL)
		pControl->SetEnabled(bEnable);
}

void CMenuExWnd::CheckMenuItem(LPCTSTR lpszName, BOOL bCheck)
{
	CMenuExElementUI* pControl = (CMenuExElementUI*)m_pm.FindControl(lpszName);
	if (pControl != NULL)
		pControl->SetCheckState(bCheck);
}

void CMenuExWnd::Init(CMenuExElementUI* pOwner, STRINGorID xml, LPCTSTR pSkinType, POINT point, CControlUI * pParentCtrl/* = NULL*/)
{
	m_BasedPoint = point;
    m_pOwner = pOwner;
    m_pLayout = NULL;
	m_pParentCtrl = pParentCtrl;

	if (pSkinType != NULL)
		m_sType = pSkinType;

	m_xml = xml;

	s_context_menu_observer.AddReceiver(this);

	Create((m_pOwner == NULL) ? m_hParent : m_pOwner->GetManager()->GetPaintWindow(), NULL, WS_POPUP, WS_EX_TOOLWINDOW | WS_EX_TOPMOST, CDuiRect());
	// HACK: Don't deselect the parent's caption
    HWND hWndParent = m_hWnd;
    while( ::GetParent(hWndParent) != NULL ) hWndParent = ::GetParent(hWndParent);
    ::ShowWindow(m_hWnd, SW_SHOW);
#if defined(WIN32) && !defined(UNDER_CE)
    ::SendMessage(hWndParent, WM_NCACTIVATE, TRUE, 0L);
#endif	
}

LPCTSTR CMenuExWnd::GetWindowClassName() const
{
    return _T("AIGuiMenu");
}

void CMenuExWnd::OnFinalMessage(HWND hWnd)
{
	RemoveObserver();
	if( m_pOwner != NULL ) {
		for( int i = 0; i < m_pOwner->GetCount(); i++ ) {
			if( static_cast<CMenuExElementUI*>(m_pOwner->GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT)) != NULL ) {
				(static_cast<CMenuExElementUI*>(m_pOwner->GetItemAt(i)))->SetOwner(m_pOwner->GetParent());
				(static_cast<CMenuExElementUI*>(m_pOwner->GetItemAt(i)))->SetVisible(false);
				(static_cast<CMenuExElementUI*>(m_pOwner->GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT)))->SetInternVisible(false);
			}
		}
		m_pOwner->m_pWindow = NULL;
		m_pOwner->m_uButtonState &= ~ UISTATE_PUSHED;
		m_pOwner->Invalidate();
	}
    delete this;
}

LRESULT CMenuExWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if( uMsg == WM_CREATE ) {
		if( m_pOwner != NULL) {
			LONG styleValue = ::GetWindowLong(*this, GWL_STYLE);
			styleValue &= ~WS_CAPTION;
			::SetWindowLong(*this, GWL_STYLE, styleValue | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
			RECT rcClient;
			::GetClientRect(*this, &rcClient);
			::SetWindowPos(*this, NULL, rcClient.left, rcClient.top, rcClient.right - rcClient.left, \
				rcClient.bottom - rcClient.top, SWP_FRAMECHANGED);

			m_pm.Init(m_hWnd);
			m_pm.SetBackgroundTransparent(true);
			// The trick is to add the items to the new container. Their owner gets
			// reassigned by this operation - which is why it is important to reassign
			// the items back to the righfull owner/manager when the window closes.
			m_pLayout = new CMenuExUI();
			m_pm.UseParentResource(m_pOwner->GetManager());
			m_pLayout->SetManager(&m_pm, NULL, true);
			LPCTSTR pDefaultAttributes = m_pOwner->GetManager()->GetDefaultAttributeList(DUI_CTR_MENU);
			if( pDefaultAttributes ) {
				m_pLayout->ApplyAttributeList(pDefaultAttributes);
			}
			m_pLayout->SetAutoDestroy(false);

			for( int i = 0; i < m_pOwner->GetCount(); i++ ) {
				if(m_pOwner->GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT) != NULL ){
					(static_cast<CMenuExElementUI*>(m_pOwner->GetItemAt(i)))->SetOwner(m_pLayout);
					m_pLayout->Add(static_cast<CControlUI*>(m_pOwner->GetItemAt(i)));
				}
			}
			m_pm.AttachDialog(m_pLayout);

			// Position the popup window in absolute space
			RECT rcOwner = m_pOwner->GetPos();
			RECT rc = rcOwner;

			int cxFixed = 0;
			int cyFixed = 0;

#if defined(WIN32) && !defined(UNDER_CE)
			MONITORINFO oMonitor = {}; 
			oMonitor.cbSize = sizeof(oMonitor);
			::GetMonitorInfo(::MonitorFromWindow(*this, MONITOR_DEFAULTTOPRIMARY), &oMonitor);
			CDuiRect rcWork = oMonitor.rcWork;
#else
			CDuiRect rcWork;
			GetWindowRect(m_pOwner->GetManager()->GetPaintWindow(), &rcWork);
#endif
			SIZE szAvailable = { rcWork.right - rcWork.left, rcWork.bottom - rcWork.top };

			for( int it = 0; it < m_pOwner->GetCount(); it++ ) {
				if(m_pOwner->GetItemAt(it)->GetInterface(DUI_CTR_MENUELEMENT) != NULL ){
					CControlUI* pControl = static_cast<CControlUI*>(m_pOwner->GetItemAt(it));
					SIZE sz = pControl->EstimateSize(szAvailable);
					cyFixed += sz.cy;

					if( cxFixed < sz.cx )
						cxFixed = sz.cx;
				}
			}

			cyFixed += 12;
			cxFixed += 20;

			RECT rcWindow;
			GetWindowRect(m_pOwner->GetManager()->GetPaintWindow(), &rcWindow);

			rc.top = rcOwner.top;
			rc.bottom = rc.top + cyFixed;
			::MapWindowRect(m_pOwner->GetManager()->GetPaintWindow(), HWND_DESKTOP, &rc);
			rc.left = rcWindow.right;
			rc.right = rc.left + cxFixed;
			rc.right += 2;

			bool bReachBottom = false;
			bool bReachRight = false;
			LONG chRightAlgin = 0;
			LONG chBottomAlgin = 0;

			RECT rcPreWindow = {0};
			ContextMenuObserver::IteratorEx<BOOL, ContextMenuExParam> iterator(s_context_menu_observer);
			ReceiverExImplBase<BOOL, ContextMenuExParam>* pReceiver = iterator.next();
			while( pReceiver != NULL ) {
				CMenuExWnd* pContextMenu = dynamic_cast<CMenuExWnd*>(pReceiver);
				if( pContextMenu != NULL ) {
					GetWindowRect(pContextMenu->GetHWND(), &rcPreWindow);

					bReachRight = rcPreWindow.left >= rcWindow.right;
					bReachBottom = rcPreWindow.top >= rcWindow.bottom;
					if( pContextMenu->GetHWND() == m_pOwner->GetManager()->GetPaintWindow() 
						||  bReachBottom || bReachRight )
						break;
				}
				pReceiver = iterator.next();
			}

			if (bReachBottom)
			{
				rc.bottom = rcWindow.top;
				rc.top = rc.bottom - cyFixed;
			}

			if (bReachRight)
			{
				rc.right = rcWindow.left;
				rc.left = rc.right - cxFixed;
			}

			if( rc.bottom > rcWork.bottom )
			{
				rc.bottom = rc.top;
				rc.top = rc.bottom - cyFixed;
			}

			if (rc.right > rcWork.right)
			{
				rc.right = rcWindow.left;
				rc.left = rc.right - cxFixed;

				rc.top = rcWindow.bottom;
				rc.bottom = rc.top + cyFixed;
			}

			if( rc.top < rcWork.top )
			{
				rc.top = rcOwner.top;
				rc.bottom = rc.top + cyFixed;
			}

			if (rc.left < rcWork.left)
			{
				rc.left = rcWindow.right;
				rc.right = rc.left + cxFixed;
			}

			MoveWindow(m_hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
		}
		else {
			m_pm.Init(m_hWnd);
			m_pm.SetBackgroundTransparent(true);

			CDialogBuilder builder;
			CMenuBuilderCallback menuCallback;

			CControlUI* pRoot = builder.Create(m_xml, m_sType.GetData(), &menuCallback, &m_pm);
			m_pm.AttachDialog(pRoot);

#if defined(WIN32) && !defined(UNDER_CE)
			MONITORINFO oMonitor = {}; 
			oMonitor.cbSize = sizeof(oMonitor);
			::GetMonitorInfo(::MonitorFromWindow(*this, MONITOR_DEFAULTTOPRIMARY), &oMonitor);
			CDuiRect rcWork = oMonitor.rcWork;
#else
			CDuiRect rcWork;
			GetWindowRect(m_pOwner->GetManager()->GetPaintWindow(), &rcWork);
#endif
			SIZE szAvailable = { rcWork.right - rcWork.left, rcWork.bottom - rcWork.top };
			szAvailable = pRoot->EstimateSize(szAvailable);
			m_pm.SetInitSize(szAvailable.cx, szAvailable.cy);

			DWORD dwAlignment = eMenuExAlignment_Left | eMenuExAlignment_Top;

			SIZE szInit = m_pm.GetInitSize();
			CDuiRect rc;
			CPoint point = m_BasedPoint;
			rc.left = point.x;
			rc.top = point.y;
			rc.right = rc.left + szInit.cx;
			rc.bottom = rc.top + szInit.cy;

			int nWidth = rc.GetWidth();
			int nHeight = rc.GetHeight();

			if (dwAlignment & eMenuExAlignment_Right)
			{
				rc.right = point.x;
				rc.left = rc.right - nWidth;
			}

			if (dwAlignment & eMenuExAlignment_Bottom)
			{
				rc.bottom = point.y;
				rc.top = rc.bottom - nHeight;
			}

			SetForegroundWindow(m_hWnd);
			MoveWindow(m_hWnd, rc.left, rc.top, rc.GetWidth(), rc.GetHeight(), FALSE);
			SetWindowPos(m_hWnd, HWND_TOPMOST, rc.left, rc.top, rc.GetWidth(), rc.GetHeight(), SWP_SHOWWINDOW);
		}

		return 0;
    }
    else if( uMsg == WM_CLOSE ) {
		if( m_pOwner != NULL )
		{
			m_pOwner->SetManager(m_pOwner->GetManager(), m_pOwner->GetParent(), false);
			m_pOwner->SetPos(m_pOwner->GetPos());
			m_pOwner->SetFocus();
		}
	}
	else if( uMsg == WM_RBUTTONDOWN || uMsg == WM_CONTEXTMENU || uMsg == WM_RBUTTONUP || uMsg == WM_RBUTTONDBLCLK )
	{
		return 0L;
	}
	else if( uMsg == WM_KILLFOCUS )
	{
		HWND hFocusWnd = (HWND)wParam;

		BOOL bInMenuWindowList = FALSE;
		ContextMenuExParam param;
		param.hWnd = GetHWND();

		ContextMenuObserver::IteratorEx<BOOL, ContextMenuExParam> iterator(s_context_menu_observer);
		ReceiverExImplBase<BOOL, ContextMenuExParam>* pReceiver = iterator.next();
		while( pReceiver != NULL ) {
			CMenuExWnd* pContextMenu = dynamic_cast<CMenuExWnd*>(pReceiver);
			if( pContextMenu != NULL && pContextMenu->GetHWND() ==  hFocusWnd ) {
				bInMenuWindowList = TRUE;
				break;
			}
			pReceiver = iterator.next();
		}

		if( !bInMenuWindowList ) {
			param.wParam = 1;
			param.lParam = NULL;
			s_context_menu_observer.RBroadcast(param);

			return 0;
		}
	}
	else if( uMsg == WM_KEYDOWN)
	{
		if( wParam == VK_ESCAPE)
		{
			Close();
		}
	}

    LRESULT lRes = 0;
    if( m_pm.MessageHandler(uMsg, wParam, lParam, lRes) ) return lRes;
    return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////////////
//

// MenuElementUI
CMenuExElementUI::CMenuExElementUI():
m_pWindow(NULL), m_bCheck(FALSE)
{
	m_cxyFixed.cy = 22;
	m_bMouseChildEnabled = true;

	SetMouseChildEnabled(false);
}

CMenuExElementUI::~CMenuExElementUI()
{}

LPCTSTR CMenuExElementUI::GetClass() const
{
	return DUI_CTR_MENUELEMENT;
}

LPVOID CMenuExElementUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcsicmp(pstrName, DUI_CTR_MENUELEMENT) == 0 ) return static_cast<CMenuExElementUI*>(this);    
    return CListContainerElementUI::GetInterface(pstrName);
}

void CMenuExElementUI::DoPaint(HDC hDC, const RECT& rcPaint)
{
    if( !::IntersectRect(&m_rcPaint, &rcPaint, &m_rcItem) ) return;
	CMenuExElementUI::DrawItemBk(hDC, m_rcItem);
	DrawItemText(hDC, m_rcItem);
	if (m_bCheck)
		DrawCheckState(hDC, m_rcItem);
	for (int i = 0; i < GetCount(); ++i)
	{
		if (GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT) == NULL)
			GetItemAt(i)->DoPaint(hDC, rcPaint);
	}
}

void CMenuExElementUI::DrawItemText(HDC hDC, const RECT& rcItem)
{
    if( m_sText.IsEmpty() ) return;

    if( m_pOwner == NULL ) return;
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    DWORD iTextColor = pInfo->dwTextColor;
    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        iTextColor = pInfo->dwHotTextColor;
    }
    if( IsSelected() ) {
        iTextColor = pInfo->dwSelectedTextColor;
    }
    if( !IsEnabled() ) {
        iTextColor = pInfo->dwDisabledTextColor;
    }
    int nLinks = 0;
    RECT rcText = rcItem;
    rcText.left += pInfo->rcTextPadding.left;
    rcText.right -= pInfo->rcTextPadding.right;
    rcText.top += pInfo->rcTextPadding.top;
    rcText.bottom -= pInfo->rcTextPadding.bottom;

    if( pInfo->bShowHtml )
        CRenderEngine::DrawHtmlText(hDC, m_pManager, rcText, m_sText, iTextColor, \
        NULL, NULL, nLinks, DT_SINGLELINE | pInfo->uTextStyle);
    else
        CRenderEngine::DrawText(hDC, m_pManager, rcText, m_sText, iTextColor, \
        pInfo->nFont, DT_SINGLELINE | pInfo->uTextStyle);
}


SIZE CMenuExElementUI::EstimateSize(SIZE szAvailable)
{
	SIZE cXY = {0};
	for( int it = 0; it < GetCount(); it++ ) {
		CControlUI* pControl = static_cast<CControlUI*>(GetItemAt(it));
		if( !pControl->IsVisible() ) continue;
		SIZE sz = pControl->EstimateSize(szAvailable);
		cXY.cy += sz.cy;
		if( cXY.cx < sz.cx )
			cXY.cx = sz.cx;
	}
	if(cXY.cy == 0) {
		TListInfoUI* pInfo = m_pOwner->GetListInfo();

		DWORD iTextColor = pInfo->dwTextColor;
		if( (m_uButtonState & UISTATE_HOT) != 0 ) {
			iTextColor = pInfo->dwHotTextColor;
		}
		if( IsSelected() ) {
			iTextColor = pInfo->dwSelectedTextColor;
		}
		if( !IsEnabled() ) {
			iTextColor = pInfo->dwDisabledTextColor;
		}

		RECT rcText = { 0, 0, max(szAvailable.cx, m_cxyFixed.cx), 9999 };
		rcText.left += pInfo->rcTextPadding.left;
		rcText.right -= pInfo->rcTextPadding.right;
		if( pInfo->bShowHtml ) {   
			int nLinks = 0;
			CRenderEngine::DrawHtmlText(m_pManager->GetPaintDC(), m_pManager, rcText, m_sText, iTextColor, NULL, NULL, nLinks, DT_CALCRECT | pInfo->uTextStyle);
		}
		else {
			CRenderEngine::DrawText(m_pManager->GetPaintDC(), m_pManager, rcText, m_sText, iTextColor, pInfo->nFont, DT_CALCRECT | pInfo->uTextStyle);
		}
		cXY.cx = rcText.right - rcText.left + pInfo->rcTextPadding.left + pInfo->rcTextPadding.right + 20;
		cXY.cy = rcText.bottom - rcText.top + pInfo->rcTextPadding.top + pInfo->rcTextPadding.bottom;
	}

	if( m_cxyFixed.cy != 0 ) cXY.cy = m_cxyFixed.cy;
	return cXY;
}

void CMenuExElementUI::DoEvent(TEventUI& event)
{
	if( event.Type == UIEVENT_MOUSEENTER )
	{
		CListContainerElementUI::DoEvent(event);
		if( m_pWindow ) return;
		bool hasSubMenu = false;
		for( int i = 0; i < GetCount(); ++i )
		{
			if( GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT) != NULL )
			{
				(static_cast<CMenuExElementUI*>(GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT)))->SetVisible(true);
				(static_cast<CMenuExElementUI*>(GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT)))->SetInternVisible(true);

				hasSubMenu = true;
			}
		}
		if( hasSubMenu )
		{
			m_pOwner->SelectItem(GetIndex(), true);
			CreateMenuWnd();
		}
		else
		{
			ContextMenuExParam param;
			param.hWnd = m_pManager->GetPaintWindow();
			param.wParam = 2;
			param.lParam = NULL;
			s_context_menu_observer.RBroadcast(param);
			m_pOwner->SelectItem(GetIndex(), true);
		}
		return;
	}

	if( event.Type == UIEVENT_BUTTONDOWN )
	{
		if( IsEnabled() ){
			CListContainerElementUI::DoEvent(event);

			if( m_pWindow ) return;

			bool hasSubMenu = false;
			for( int i = 0; i < GetCount(); ++i ) {
				if( GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT) != NULL ) {
					(static_cast<CMenuExElementUI*>(GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT)))->SetVisible(true);
					(static_cast<CMenuExElementUI*>(GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT)))->SetInternVisible(true);

					hasSubMenu = true;
				}
			}
			if( hasSubMenu )
			{
				CreateMenuWnd();
			}
			else
			{
				ContextMenuExParam param;
				param.hWnd = m_pManager->GetPaintWindow();
				param.wParam = 1;
				param.lParam = (LPARAM)GetName().GetData();
				s_context_menu_observer.RBroadcast(param);
			}
        }
        return;
    }

    CListContainerElementUI::DoEvent(event);
}

bool CMenuExElementUI::Activate()
{
	if (CListContainerElementUI::Activate() && m_bSelected)
	{
		if( m_pWindow ) return true;
		bool hasSubMenu = false;
		for (int i = 0; i < GetCount(); ++i)
		{
			if (GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT) != NULL)
			{
				(static_cast<CMenuExElementUI*>(GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT)))->SetVisible(true);
				(static_cast<CMenuExElementUI*>(GetItemAt(i)->GetInterface(DUI_CTR_MENUELEMENT)))->SetInternVisible(true);

				hasSubMenu = true;
			}
		}
		if (hasSubMenu)
		{
			CreateMenuWnd();
		}
		else
		{
			ContextMenuExParam param;
			param.hWnd = m_pManager->GetPaintWindow();
			param.wParam = 1;
			param.lParam = (LPARAM)GetName().GetData();
			s_context_menu_observer.RBroadcast(param);
		}

		return true;
	}
	return false;
}

CMenuExWnd* CMenuExElementUI::GetMenuWnd()
{
	return m_pWindow;
}

void CMenuExElementUI::CreateMenuWnd()
{
	if( m_pWindow ) return;

	m_pWindow = new CMenuExWnd(m_pManager->GetPaintWindow());
	ASSERT(m_pWindow);

	ContextMenuExParam param;
	param.hWnd = m_pManager->GetPaintWindow();
	param.wParam = 2;
	param.lParam = NULL;
	s_context_menu_observer.RBroadcast(param);

	m_pWindow->Init(static_cast<CMenuExElementUI*>(this), _T(""), _T(""), CPoint(), m_pParent);
}

void CMenuExElementUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
	if (_tcscmp(pstrName, _T("text")) == 0)
		int t = 1;

	if( _tcscmp(pstrName, _T("checked")) == 0 )
		SetCheckState(_tcscmp(pstrValue, _T("true")) == 0);
	else if( _tcscmp(pstrName, _T("checkimg")) == 0 )
		m_strCheckImage = pstrValue;
	else
		CListContainerElementUI::SetAttribute(pstrName, pstrValue);
}

void CMenuExElementUI::SetCheckState(BOOL bCheck)
{
	m_bCheck = bCheck;
}

void CMenuExElementUI::DrawItemBk(HDC hDC, const RECT& rcItem)
{
	if (!IsEnabled())
		return;
	else
		CListContainerElementUI::DrawItemBk(hDC, rcItem);
}

void CMenuExElementUI::DrawCheckState(HDC hDC, const RECT& rcItem)
{
	TListInfoUI* pInfo = m_pOwner->GetListInfo();

	RECT rcCheck = rcItem;
	rcCheck.right = pInfo->rcTextPadding.left;
	rcCheck.top += pInfo->rcTextPadding.top;
	rcCheck.bottom -= pInfo->rcTextPadding.bottom;

	if (!m_strCheckImage.IsEmpty())
		CRenderEngine::DrawImageString(hDC, m_pManager, rcCheck, m_rcPaint, m_strCheckImage, NULL);
}
} // namespace DuiLib
