#include "StdAfx.h"

namespace SdkLayout {

CControlUI::CControlUI() : 
m_bInited(false),
m_bVisible(true), 
m_bVisibleByParent(true),
m_bDisplayed(true),
m_id(0),
m_name(0),
m_font(-1),
m_pManager(NULL),
m_pParent(NULL)
{
    m_cXY.cx = m_cXY.cy = 0;
    m_cxyFixed.cx = m_cxyFixed.cy = 0;
    m_cxyMin.cx = m_cxyMin.cy = 0;
    m_cxyMax.cx = m_cxyMax.cy = 9999;
	m_szPostSize.cx = m_szPostSize.cy = 0;

    ::ZeroMemory(&m_rcItem, sizeof(RECT));
	::ZeroMemory(&m_rcInset, sizeof(m_rcInset)); // class object
}

CControlUI::~CControlUI()
{
}

void CControlUI::DoInit()
{
	assert(m_pManager != NULL);
	m_bInited = true;
}

bool CControlUI::SetFocus()
{
	if(::IsWindow(GetHWND())){
		::SetFocus(GetHWND());
		return true;
	}
	else{
		return false;
	}
}

const CDuiRect& CControlUI::GetPos() const
{
    return m_rcItem;
}

void CControlUI::SetPos(const CDuiRect& rc)
{
    m_rcItem = rc;
	if(m_rcItem.right < m_rcItem.left) m_rcItem.right  = m_rcItem.left;
	if(m_rcItem.bottom< m_rcItem.top)  m_rcItem.bottom = m_rcItem.top;

	SIZE tmpsz = {m_rcItem.GetWidth(), m_rcItem.GetHeight()};
	SetPostSize(tmpsz);

	if(!IsWindow(m_hWnd) || m_rcItem.IsNull()) 
		return;

	CDuiRect rct = m_rcItem;
	rct = m_rcItem;

	rct.left   += m_rcInset.left;
	rct.top    += m_rcInset.top;
	rct.right  -= m_rcInset.right;
	rct.bottom -= m_rcInset.bottom;

	::SetWindowPos(m_hWnd, 0, rct.left, rct.top, rct.GetWidth(), rct.GetHeight(), SWP_NOZORDER);
}

int CControlUI::GetWidth() const
{
    return m_rcItem.right - m_rcItem.left;
}

int CControlUI::GetHeight() const
{
    return m_rcItem.bottom - m_rcItem.top;
}

int CControlUI::GetX() const
{
    return m_rcItem.left;
}

int CControlUI::GetY() const
{
    return m_rcItem.top;
}

SIZE CControlUI::GetFixedXY() const
{
    return m_cXY;
}

void CControlUI::SetFixedXY(SIZE szXY)
{
    m_cXY.cx = szXY.cx;
    m_cXY.cy = szXY.cy;
}

int CControlUI::GetFixedWidth() const
{
    return m_cxyFixed.cx;
}

void CControlUI::SetFixedWidth(int cx)
{
    if( cx < 0 ) return; 
    m_cxyFixed.cx = cx;
}

int CControlUI::GetFixedHeight() const
{
    return m_cxyFixed.cy;
}

void CControlUI::SetFixedHeight(int cy)
{
    if( cy < 0 ) return; 
    m_cxyFixed.cy = cy;
}

int CControlUI::GetMinWidth() const
{
    return m_cxyMin.cx;
}

void CControlUI::SetMinWidth(int cx)
{
    if( cx < 0 ) return; 
    m_cxyMin.cx = cx;
}

int CControlUI::GetMaxWidth() const
{
    return m_cxyMax.cx;
}

void CControlUI::SetMaxWidth(int cx)
{
    if( cx < 0 ) return; 
    m_cxyMax.cx = cx;
}

int CControlUI::GetMinHeight() const
{
    return m_cxyMin.cy;
}

void CControlUI::SetMinHeight(int cy)
{
    if( cy < 0 ) return; 
    m_cxyMin.cy = cy;
}

int CControlUI::GetMaxHeight() const
{
    return m_cxyMax.cy;
}

void CControlUI::SetMaxHeight(int cy)
{
    if( cy < 0 ) return; 
    m_cxyMax.cy = cy;
}

void CControlUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
    if( _tcscmp(pstrName, _T("id")) == 0 ) SetID(_ttoi(pstrValue));
	else if(_tcscmp(pstrName,_T("font")) == 0)  SetFont(_ttoi(pstrValue));
	else if(_tcscmp(pstrName, _T("name")) == 0) SetName(pstrValue);
    else if( _tcscmp(pstrName, _T("width")) == 0 ) SetFixedWidth(_ttoi(pstrValue));
    else if( _tcscmp(pstrName, _T("height")) == 0 ) SetFixedHeight(_ttoi(pstrValue));
    else if( _tcscmp(pstrName, _T("minwidth")) == 0 ) SetMinWidth(_ttoi(pstrValue));
    else if( _tcscmp(pstrName, _T("minheight")) == 0 ) SetMinHeight(_ttoi(pstrValue));
    else if( _tcscmp(pstrName, _T("maxwidth")) == 0 ) SetMaxWidth(_ttoi(pstrValue));
    else if( _tcscmp(pstrName, _T("maxheight")) == 0 ) SetMaxHeight(_ttoi(pstrValue));
	else if( _tcscmp(pstrName, _T("inset")) == 0) {
		RECT rcInset = { 0 };
		LPTSTR pstr = NULL;
		rcInset.left = _tcstol(pstrValue, &pstr, 10);
		rcInset.top = _tcstol(pstr + 1, &pstr, 10); 
		rcInset.right = _tcstol(pstr + 1, &pstr, 10);
		rcInset.bottom = _tcstol(pstr + 1, &pstr, 10);
		SetInset(rcInset);
	}
	else if(_tcscmp(pstrName, _T("visible")) == 0){
		bool bVisible = _tcscmp(pstrValue, _T("true"))==0;
		SetVisible(bVisible);
	}
	else if(_tcscmp(pstrName, _T("display")) == 0) SetDisplayed(_tcscmp(pstrValue, _T("true"))==0);
	else{
#ifdef _DEBUG
		MessageBox(NULL, pstrValue, pstrName, MB_ICONEXCLAMATION);
#endif
	}
}

SIZE CControlUI::EstimateSize(SIZE szAvailable)
{
    return m_cxyFixed;
}

CControlUI* CControlUI::FindControl(LPCTSTR name)
{
	UINT hash = HashKey(name);
	return hash==m_name ? this : NULL;
}

void CControlUI::SetFont( int id )
{
	if(id != -2)
		m_font = id; 
	if(IsWindow(GetHWND()) && m_pManager){
		HFONT hFont = m_font==-1 ? m_pManager->GetDefaultFont() : m_pManager->GetFont(m_font);
		SendMessage(m_hWnd, WM_SETFONT, WPARAM(hFont), MAKELPARAM(TRUE,0));
	}
}

void CControlUI::SetManager(CPaintManagerUI* mgr)
{
	m_pManager = mgr;
	if (m_id <= 0){
		m_hWnd = NULL;
	}
	else if (m_id > 0){
		m_hWnd = GetDlgItem(m_pManager->GetHWND(), m_id);
		assert(m_hWnd != 0);
	}
}

} // namespace SdkLayout
