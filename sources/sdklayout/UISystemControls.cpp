#include "stdafx.h"

namespace SdkLayout{


	CSystemControlUI::CSystemControlUI()
		: m_dwStyle(WS_CHILD|WS_VISIBLE)
		, m_dwExStyle(0)
	{

	}

	void CSystemControlUI::SetAttribute( LPCTSTR pstrName, LPCTSTR pstrValue )
	{
		if(_tcscmp(pstrName, "text") == 0) m_strText = pstrValue;
		else if(_tcscmp(pstrName, "style")==0){
			TCHAR* p;
			m_dwStyle = _tcstol(pstrValue, &p, 10);
		}
		else if(_tcscmp(pstrName, "exstyle")==0){
			TCHAR* p;
			m_dwExStyle= _tcstol(pstrValue, &p, 10);
		}

		else CControlUI::SetAttribute(pstrName, pstrValue);
	}

	void CSystemControlUI::Create( LPCTSTR lpClass )
	{
		m_hWnd = ::CreateWindowEx(m_dwExStyle, lpClass, m_strText.c_str(), m_dwStyle,
			0, 0, 0, 0, m_pManager->GetHWND(), HMENU(m_id), GetModuleHandle(0), NULL);
		assert(m_hWnd != NULL);
	}

	//////////////////////////////////////////////////////////////////////////
	void CButtonUI::SetManager( CPaintManagerUI* mgr )
	{
		m_pManager = mgr;
		Create(WC_BUTTON);
	}

	//////////////////////////////////////////////////////////////////////////
	COptionUI::COptionUI()
	{
		m_bHasWsGroup = false;
		m_dwStyle |= BS_AUTORADIOBUTTON;
	}

	void COptionUI::SetManager( CPaintManagerUI* mgr )
	{
		m_pManager = mgr;
		if(m_bHasWsGroup) m_dwStyle |= WS_GROUP;
		Create(WC_BUTTON);
	}

	void COptionUI::SetAttribute( LPCTSTR pstrName, LPCTSTR pstrValue )
	{
		if(_tcscmp(pstrName, "group") == 0) m_bHasWsGroup = _tcscmp(pstrValue,"true")==0;
		else CSystemControlUI::SetAttribute(pstrName, pstrValue);
	}

	//////////////////////////////////////////////////////////////////////////
	CCheckUI::CCheckUI()
	{
		m_bCheck = false;
		m_dwStyle |= BS_AUTOCHECKBOX;
	}

	void CCheckUI::SetManager( CPaintManagerUI* mgr )
	{
		m_pManager = mgr;
		Create(WC_BUTTON);
		::SendMessage(GetHWND(), BM_SETCHECK, m_bCheck ? BST_CHECKED : BST_UNCHECKED, 0);
	}

	void CCheckUI::SetAttribute( LPCTSTR pstrName, LPCTSTR pstrValue )
	{
		if(_tcscmp(pstrName, "checked") == 0) m_bCheck = _tcscmp(pstrValue,"true")==0;
		else CSystemControlUI::SetAttribute(pstrName, pstrValue);
	}

	//////////////////////////////////////////////////////////////////////////
	void CStaticUI::SetManager( CPaintManagerUI* mgr )
	{
		m_pManager = mgr;
		Create(WC_STATIC);
	}

	void CStaticUI::SetAttribute( LPCTSTR pstrName, LPCTSTR pstrValue )
	{
		CSystemControlUI::SetAttribute(pstrName, pstrValue);
	}

	//////////////////////////////////////////////////////////////////////////
	CGroupUI::CGroupUI()
	{
		m_dwStyle |= BS_GROUPBOX;
	}

	void CGroupUI::SetManager(CPaintManagerUI* mgr)
	{
		m_pManager = mgr;
		Create(WC_BUTTON);
	}

	//////////////////////////////////////////////////////////////////////////
	CEditUI::CEditUI()
	{

	}

	void CEditUI::SetManager(CPaintManagerUI* mgr)
	{
		m_pManager = mgr;
		Create(WC_EDIT);
	}

}

