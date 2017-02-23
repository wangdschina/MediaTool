#ifndef __UIMENUEX_H__
#define __UIMENUEX_H__

#pragma once

#include "observer_impl_base_ex.hpp"

namespace DuiLib {

/////////////////////////////////////////////////////////////////////////////////////
//
struct ContextMenuExParam
{
	// 1: remove all
	// 2: remove the sub menu
	WPARAM wParam;
	// 选中的菜单项名称
	LPARAM lParam;
	HWND hWnd;
};

enum MenuExAlignment
{
	eMenuExAlignment_Left = 1 << 1,
	eMenuExAlignment_Top = 1 << 2,
	eMenuExAlignment_Right = 1 << 3,
	eMenuExAlignment_Bottom = 1 << 4,
};

typedef class ObserverExImpl<BOOL, ContextMenuExParam> ContextMenuObserver;
typedef class ReceiverExImpl<BOOL, ContextMenuExParam> ContextMenuReceiver;

extern ContextMenuObserver s_context_menu_observer;

// MenuUI
#define		WM_MENUEXCLICK			WM_USER+1008 // 菜单选中消息
#define		DUI_CTR_MENU			(_T("Menu"))
#define		DUI_MSGTYPE_MENUCLICK   (_T("menuclick"))

class CListUI;
class CMenuExUI : public CListUI
{
public:
	CMenuExUI();
	~CMenuExUI();

    LPCTSTR GetClass() const;
    LPVOID GetInterface(LPCTSTR pstrName);

	virtual void DoEvent(TEventUI& event);

    virtual bool Add(CControlUI* pControl);
    virtual bool AddAt(CControlUI* pControl, int iIndex);

    virtual int GetItemIndex(CControlUI* pControl) const;
    virtual bool SetItemIndex(CControlUI* pControl, int iIndex);
    virtual bool Remove(CControlUI* pControl);

	SIZE EstimateSize(SIZE szAvailable);

	void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);
};

/////////////////////////////////////////////////////////////////////////////////////
//

// MenuElementUI
#define		DUI_CTR_MENUELEMENT		(_T("MenuElement"))

class CMenuExElementUI;
class CMenuExWnd : public CWindowWnd, public ContextMenuReceiver
{
public:
	CMenuExWnd(HWND hParent = NULL);
    void Init(CMenuExElementUI* pOwner, STRINGorID xml, LPCTSTR pSkinType, POINT point, CControlUI * pParentCtrl = NULL);
    LPCTSTR GetWindowClassName() const;
    void OnFinalMessage(HWND hWnd);

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	BOOL Receive(ContextMenuExParam param);

	void EnableMenuItem(LPCTSTR lpszName, BOOL bEnable);
	void CheckMenuItem(LPCTSTR lpszName, BOOL bCheck);

public:
	HWND m_hParent;
	POINT m_BasedPoint;
	STRINGorID m_xml;
	CDuiString m_sType;
    CPaintManagerUI m_pm;
    CMenuExElementUI* m_pOwner;
    CMenuExUI* m_pLayout;
	CControlUI * m_pParentCtrl;
};

class CListContainerElementUI;
class CMenuExElementUI : public CListContainerElementUI
{
	friend CMenuExWnd;
public:
    CMenuExElementUI();
	~CMenuExElementUI();

    LPCTSTR GetClass() const;
    LPVOID GetInterface(LPCTSTR pstrName);

    void DoPaint(HDC hDC, const RECT& rcPaint);

	void DrawItemText(HDC hDC, const RECT& rcItem);

	SIZE EstimateSize(SIZE szAvailable);

	bool Activate();

	void DoEvent(TEventUI& event);

	CMenuExWnd* GetMenuWnd();

	void CreateMenuWnd();

	void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);
	void SetCheckState(BOOL bCheck);
	void DrawItemBk(HDC hDC, const RECT& rcItem);
	void DrawCheckState(HDC hDC, const RECT& rcItem);

protected:
	CMenuExWnd* m_pWindow;
	BOOL m_bCheck;
	CDuiString m_strCheckImage;
};

} // namespace DuiLib

#endif // __UIMENU_H__
