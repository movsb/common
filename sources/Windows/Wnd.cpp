#include "StdAfx.h"
#include "Wnd.h"

namespace Common {

CWnd::CWnd()
	: m_hWnd(NULL)
	, m_OldWndProc(::DefWindowProc)
	, m_bSubclassed(false)
	, m_bIsWindowsDialog(false)
	, m_bIsWindowsDialogModeless(false)
	, m_bHasMgr(true)
{
}

HWND CWnd::GetHWND() const 
{ 
    return m_hWnd; 
}

CWnd::operator HWND() const
{
	return m_hWnd;
}

UINT CWnd::GetClassStyle() const
{
    return CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
}


HBRUSH CWnd::GetClassBrush() const
{
	return (HBRUSH)::GetStockObject(BLACK_BRUSH);
}

LPCTSTR CWnd::GetWindowClassName() const
{
	return NULL;
}

LPCTSTR CWnd::GetSuperClassName() const
{
    return NULL;
}

HWND CWnd::Create(HWND hParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, int x, int y, int cx, int cy, HMENU hMenu)
{
    if( GetSuperClassName() != NULL && !RegisterSuperclass() ) return NULL;
    if( GetSuperClassName() == NULL && !RegisterWindowClass() ) return NULL;
    m_hWnd = ::CreateWindowEx(dwExStyle, GetWindowClassName(), pstrName, dwStyle, 
		x, y, cx, cy, hParent, hMenu, GetModuleHandle(0), this);
    assert(m_hWnd!=NULL);
    return m_hWnd;
}

HWND CWnd::Create(HWND hParent, LPCTSTR lpTemplate)
{
	m_bIsWindowsDialog = true;
	m_hWnd = ::CreateDialogParam(GetModuleHandle(0), lpTemplate, hParent, __DialogProc, LPARAM(this));
	assert(m_hWnd != NULL);
	return m_hWnd;
}

bool CWnd::Attach( HWND hWnd )
{
	assert(IsWindow(hWnd));
	assert((GetWindowStyle(hWnd) & WS_CHILD) != 0);
	m_bHasMgr = false;
	return !!Subclass(hWnd);
}

HWND CWnd::Subclass(HWND hWnd)
{
    assert(::IsWindow(hWnd));
    assert(m_hWnd==NULL);
    m_OldWndProc = SubclassWindow(hWnd, __ControlProc);
    if( m_OldWndProc == NULL ) return NULL;
    m_bSubclassed = true;
    m_hWnd = hWnd;
	::SetProp(hWnd, "WndX", this);
    return m_hWnd;
}

void CWnd::Unsubclass()
{
    assert(::IsWindow(m_hWnd));
    if( !::IsWindow(m_hWnd) ) return;
    if( !m_bSubclassed ) return;
    SubclassWindow(m_hWnd, m_OldWndProc);
    m_OldWndProc = ::DefWindowProc;
    m_bSubclassed = false;
}

void CWnd::ShowWindow(bool bShow /*= true*/, bool bTakeFocus /*= false*/)
{
    assert(::IsWindow(m_hWnd));
    if( !::IsWindow(m_hWnd) ) return;
    ::ShowWindow(m_hWnd, bShow ? (bTakeFocus ? SW_SHOWNORMAL : SW_SHOWNOACTIVATE) : SW_HIDE);
}

void CWnd::Close(UINT nRet)
{
    assert(::IsWindow(m_hWnd));
    if( !::IsWindow(m_hWnd) ) return;
    PostMessage(WM_CLOSE, (WPARAM)nRet, 0L);
}

void CWnd::CenterWindow()
{
    assert(::IsWindow(m_hWnd));
    assert((GetWindowStyle(m_hWnd)&WS_CHILD)==0);
    RECT rcDlg = { 0 };
    ::GetWindowRect(m_hWnd, &rcDlg);
    RECT rcArea = { 0 };
    RECT rcCenter = { 0 };
	HWND hWnd=*this;
    HWND hWndParent = ::GetParent(m_hWnd);
    HWND hWndCenter = ::GetWindowOwner(m_hWnd);
	if (hWndCenter!=NULL)
		hWnd=hWndCenter;

	// 处理多显示器模式下屏幕居中
	MONITORINFO oMonitor = {};
	oMonitor.cbSize = sizeof(oMonitor);
	::GetMonitorInfo(::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &oMonitor);
	rcArea = oMonitor.rcWork;

    if( hWndCenter == NULL )
		rcCenter = rcArea;
	else
		::GetWindowRect(hWndCenter, &rcCenter);

    int DlgWidth = rcDlg.right - rcDlg.left;
    int DlgHeight = rcDlg.bottom - rcDlg.top;

    // Find dialog's upper left based on rcCenter
    int xLeft = (rcCenter.left + rcCenter.right) / 2 - DlgWidth / 2;
    int yTop = (rcCenter.top + rcCenter.bottom) / 2 - DlgHeight / 2;

    // The dialog is outside the screen, move it inside
    if( xLeft < rcArea.left ) xLeft = rcArea.left;
    else if( xLeft + DlgWidth > rcArea.right ) xLeft = rcArea.right - DlgWidth;
    if( yTop < rcArea.top ) yTop = rcArea.top;
    else if( yTop + DlgHeight > rcArea.bottom ) yTop = rcArea.bottom - DlgHeight;
    ::SetWindowPos(m_hWnd, NULL, xLeft, yTop, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

bool CWnd::RegisterWindowClass()
{
    WNDCLASS wc = { 0 };
    wc.style = GetClassStyle();
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hIcon = NULL;
    wc.lpfnWndProc = CWnd::__WndProc;
    wc.hInstance = GetModuleHandle(0);
    wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetClassBrush();
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = GetWindowClassName();
    ATOM ret = ::RegisterClass(&wc);
    assert(ret!=NULL || ::GetLastError()==ERROR_CLASS_ALREADY_EXISTS);
    return ret != NULL || ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

bool CWnd::RegisterSuperclass()
{
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    if( !::GetClassInfoEx(NULL, GetSuperClassName(), &wc) ) {
        if( !::GetClassInfoEx(GetModuleHandle(0), GetSuperClassName(), &wc) ) {
            assert(!"Unable to locate window class");
            return NULL;
        }
    }
    m_OldWndProc = wc.lpfnWndProc;
    wc.lpfnWndProc = CWnd::__ControlProc;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = GetWindowClassName();
    ATOM ret = ::RegisterClassEx(&wc);
    assert(ret!=NULL || ::GetLastError()==ERROR_CLASS_ALREADY_EXISTS);
    return ret != NULL || ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

LRESULT CALLBACK CWnd::__WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CWnd* pThis = NULL;
    if( uMsg == WM_NCCREATE ) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<CWnd*>(lpcs->lpCreateParams);
        pThis->m_hWnd = hWnd;
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(pThis));
		pThis->OnFirstMessage(hWnd);
	} 
    else {
        pThis = reinterpret_cast<CWnd*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if( uMsg == WM_NCDESTROY && pThis != NULL ) {
            LRESULT lRes = ::CallWindowProc(pThis->m_OldWndProc, hWnd, uMsg, wParam, lParam);
            ::SetWindowLongPtr(pThis->m_hWnd, GWLP_USERDATA, 0L);
            if( pThis->m_bSubclassed ) pThis->Unsubclass();
            pThis->OnFinalMessage(hWnd);
            return lRes;
        }
    }

    if( pThis != NULL ) {
		bool bHandled = true;
        return pThis->HandleMessage(uMsg, wParam, lParam, bHandled);
    } 
    else {
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}

LRESULT CALLBACK CWnd::__ControlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CWnd* pThis = NULL;
    if( uMsg == WM_NCCREATE ) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<CWnd*>(lpcs->lpCreateParams);
        ::SetProp(hWnd, _T("WndX"), (HANDLE) pThis);
        pThis->m_hWnd = hWnd;
		pThis->OnFirstMessage(hWnd);
    } 
    else {
        pThis = reinterpret_cast<CWnd*>(::GetProp(hWnd, _T("WndX")));
        if( uMsg == WM_NCDESTROY && pThis != NULL ) {
            LRESULT lRes = ::CallWindowProc(pThis->m_OldWndProc, hWnd, uMsg, wParam, lParam);
            if( pThis->m_bSubclassed ) pThis->Unsubclass();
            ::SetProp(hWnd, _T("WndX"), NULL);
            pThis->m_hWnd = NULL;
            pThis->OnFinalMessage(hWnd);
            return lRes;
        }
    }
    if( pThis != NULL ) {
		bool bHandled = true;
        return pThis->HandleMessage(uMsg, wParam, lParam, bHandled);
    } 
    else {
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}


INT_PTR CALLBACK CWnd::__DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	CWnd* pThis = NULL;
	if(uMsg == WM_INITDIALOG){
		pThis = (CWnd*)(lParam);
		pThis->m_hWnd = hWnd;
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(pThis));
		pThis->OnFirstMessage(hWnd);
	}
	else{       
		pThis = reinterpret_cast<CWnd*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if( uMsg == WM_NCDESTROY && pThis != NULL ) {
			LRESULT lRes = ::CallWindowProc(pThis->m_OldWndProc, hWnd, uMsg, wParam, lParam);
			::SetWindowLongPtr(pThis->m_hWnd, GWLP_USERDATA, 0L);
			pThis->m_hWnd = NULL;
			pThis->OnFinalMessage(hWnd);
			return lRes;
		}
	}

	if(pThis != NULL){
		bool bHandled = true;
		LRESULT r = pThis->HandleMessage(uMsg, wParam, lParam, bHandled);
		if(bHandled){
			return SetDlgMsgResult(hWnd, uMsg, r);
		}
	}
	return FALSE;
}

LRESULT CWnd::SendMessage(UINT uMsg, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/)
{
    assert(::IsWindow(m_hWnd));
    return ::SendMessage(m_hWnd, uMsg, wParam, lParam);
} 

LRESULT CWnd::PostMessage(UINT uMsg, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/)
{
    assert(::IsWindow(m_hWnd));
    return ::PostMessage(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
{
	if (m_bIsWindowsDialog || m_bIsWindowsDialogModeless){
		if (uMsg == WM_CLOSE){
			if (m_bIsWindowsDialog)
				EndDialog(m_hWnd, 0);
			else
				DestroyWindow();
			return 0;
		}
		else{
			bHandled = false;
			return 0;
		}
	}
	return ::CallWindowProc(m_OldWndProc, m_hWnd, uMsg, wParam, lParam);
}

void CWnd::OnFirstMessage(HWND hWnd)
{
	if (GetWindowStyle(hWnd) & WS_CHILD){
		m_bHasMgr = false;
	}
	else{
		m_wndmgr.Init(hWnd, this);
		m_bHasMgr = true;
	}
}

void CWnd::OnFinalMessage(HWND /*hWnd*/)
{
	if (m_bHasMgr)
		m_wndmgr.DeInit();
}

bool CWnd::FilterMessage(HWND hChild, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN)
	{
		switch (wParam)
		{
		case VK_RETURN:
		case VK_ESCAPE:
			return ResponseDefaultKeyEvent(hChild,wParam);
		default:
			break;
		}
	}
	return false;
}

bool CWnd::ResponseDefaultKeyEvent(HWND hChild, WPARAM wParam )
{
	if (wParam == VK_ESCAPE)
	{
		Close();
		return true;
	}
	return false;
}

int CWnd::msgbox( UINT msgicon, char* caption, char* fmt, ... )
{
	va_list va;
	char smsg[1024] = { 0 };
	va_start(va, fmt);
	_vsnprintf(smsg, sizeof(smsg), fmt, va);
	va_end(va);
	return ::MessageBox(m_hWnd, smsg, caption, msgicon);
}

void CWnd::msgerr( char* prefix )
{
	char* buffer = NULL;
	if (!prefix) prefix = "错误";
	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPSTR)&buffer, 1, NULL))
	{
		msgbox(MB_ICONHAND, NULL, "%s: %s", prefix, buffer);
		LocalFree(buffer);
	}
}

void CWnd::ResizeClient(int cx /*= -1*/, int cy /*= -1*/)
{
	RECT rc = { 0 };
	if (!::GetClientRect(m_hWnd, &rc)) return;
	if (cx != -1) rc.right = cx;
	if (cy != -1) rc.bottom = cy;
	if (!::AdjustWindowRectEx(&rc, GetWindowStyle(m_hWnd), (!(GetWindowStyle(m_hWnd) & WS_CHILD) && (::GetMenu(m_hWnd) != NULL)), GetWindowExStyle(m_hWnd))) return;
	::SetWindowPos(m_hWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

bool CWnd::do_modal(HWND owner)
{
	m_bIsWindowsDialog = true;
	DialogBoxParam(GetModuleHandle(0), MAKEINTRESOURCE(139), owner, __DialogProc, LPARAM(this));
	return true;
}

bool CWnd::do_modeless(HWND owner)
{
	m_hWnd = CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(139), owner,  __DialogProc, LPARAM(this));
	ShowWindow();
	return true;
}

void CWnd::DestroyWindow()
{
	::DestroyWindow(m_hWnd);
}


}
