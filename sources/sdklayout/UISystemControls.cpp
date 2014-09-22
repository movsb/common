#include "stdafx.h"

namespace SdkLayout{

	bool map_style(DWORD* dwStyle, style_map* known_styles, std::vector<std::string>& styles)
	{
		int n = 0;
		for (auto& style : styles){
			if (style.size()){
				for (int i = 0; known_styles[i].strStyle != nullptr; i++){
					if (strcmp(known_styles[i].strStyle, style.c_str()) == 0){
						*dwStyle |= known_styles[i].dwStyle;
						style = "";
						n++;
						break;
					}
				}
			}
			else{
				n++;
			}
		}
		return n == styles.size();
	}

	CSystemControlUI::CSystemControlUI()
		: m_dwStyle(WS_CHILD|WS_VISIBLE)
		, m_dwExStyle(0)
	{

	}

	void CSystemControlUI::SetAttribute( LPCTSTR pstrName, LPCTSTR pstrValue )
	{
		if(_tcscmp(pstrName, "text") == 0) m_strText = pstrValue;
		else if(_tcscmp(pstrName, "style")==0){
			std::vector<std::string> styles;
			split_string(&styles, pstrValue, ',');
			SetStyle(styles);
		}
		else if(_tcscmp(pstrName, "exstyle")==0){
			std::vector<std::string> exstyles;
			split_string(&exstyles, pstrValue, ',');
			SetStyle(exstyles, true);
		}
		else CControlUI::SetAttribute(pstrName, pstrValue);
	}
	
	void CSystemControlUI::Create( LPCTSTR lpClass )
	{
		m_hWnd = ::CreateWindowEx(m_dwExStyle, lpClass, m_strText.c_str(), m_dwStyle,
			0, 0, 0, 0, m_pManager->GetHWND(), HMENU(m_id), GetModuleHandle(0), NULL);
		assert(m_hWnd != NULL);
	}

	void CSystemControlUI::SetManager(CPaintManagerUI* mgr)
	{
		m_pManager = mgr;
		Create(GetWindowClass());
	}

	void CSystemControlUI::SetStyle(std::vector<std::string>& styles, bool bex)
	{
		static style_map known_styles[] =
		{
			{ WS_BORDER, "border" },
			{ WS_CAPTION, "caption" },
			{ WS_CHILD, "child" },
			{ WS_CLIPSIBLINGS, "clipsiblings" },
			{ WS_CLIPCHILDREN, "clipchildren" },
			{ WS_DISABLED, "disabled" },
			{ WS_GROUP, "group" },
			{ WS_HSCROLL, "hscroll" },
			{ WS_TABSTOP, "tabstop" },
			{ WS_VSCROLL, "vscroll" },
			{ 0, nullptr }
		};

		static style_map known_ex_styles[] =
		{
			{ WS_EX_ACCEPTFILES, "acceptfiles" },
			{ WS_EX_CLIENTEDGE, "clientedge" },
			{ WS_EX_STATICEDGE, "staticedge" },
			{ WS_EX_TOOLWINDOW, "toolwindow" },
			{ WS_EX_TOPMOST, "topmost" },
			{ WS_EX_TRANSPARENT, "transparent" },
			{ 0, nullptr }
		};

		auto ks = bex ? &known_ex_styles[0] : &known_styles[0];
		auto& thestyle = bex ? m_dwExStyle : m_dwStyle;
		if (!map_style(&thestyle, ks, styles)){
			assert(0 && "unknown style detected!");
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CButtonUI::SetStyle(std::vector<std::string>& styles, bool bex /*= false*/)
	{
		return __super::SetStyle(styles, bex);
	}

	//////////////////////////////////////////////////////////////////////////
	COptionUI::COptionUI()
	{
		m_bHasWsGroup = false;
		m_dwStyle |= BS_AUTORADIOBUTTON;
	}

	//////////////////////////////////////////////////////////////////////////
	void COptionUI::SetStyle(std::vector<std::string>& styles, bool bex /*= false*/)
	{
		return __super::SetStyle(styles, bex);
	}

	//////////////////////////////////////////////////////////////////////////
	CCheckUI::CCheckUI()
	{
		m_bCheck = false;
		m_dwStyle |= BS_AUTOCHECKBOX;
	}

	void CCheckUI::SetManager( CPaintManagerUI* mgr )
	{
		__super::SetManager(mgr);
		::SendMessage(GetHWND(), BM_SETCHECK, m_bCheck ? BST_CHECKED : BST_UNCHECKED, 0);
	}

	void CCheckUI::SetAttribute( LPCTSTR pstrName, LPCTSTR pstrValue )
	{
		if(_tcscmp(pstrName, "checked") == 0) m_bCheck = _tcscmp(pstrValue,"true")==0;
		else CSystemControlUI::SetAttribute(pstrName, pstrValue);
	}

	//////////////////////////////////////////////////////////////////////////
	CGroupUI::CGroupUI()
	{
		m_dwStyle |= BS_GROUPBOX;
	}

	//////////////////////////////////////////////////////////////////////////
	CEditUI::CEditUI()
	{
		m_dwStyle |= ES_AUTOHSCROLL | ES_AUTOVSCROLL;
	}

	void CEditUI::SetStyle(std::vector<std::string>& styles, bool bex /*= false*/)
	{
		static style_map known_styles[] =
		{
			{ ES_CENTER, "center" },
			{ ES_MULTILINE, "multiline" },
			{ ES_NOHIDESEL, "nohidesel" },
			{ ES_NUMBER, "number" },
			{ ES_READONLY, "readonly" },
			{ ES_WANTRETURN, "wantreturn" },
			{ 0, nullptr }
		};

		static style_map known_ex_styles[] =
		{
			{ 0, nullptr }
		};

		auto ks = bex ? &known_ex_styles[0] : &known_styles[0];
		auto& thestyle = bex ? m_dwExStyle : m_dwStyle;
		if (!map_style(&thestyle, ks, styles)){
			return __super::SetStyle(styles, bex);
		}
	}



}

