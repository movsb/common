#include "stdafx.h"

namespace Common{
namespace Window{

	//////////////////////////////////////////////////////////////////////////
	bool c_edit::back_delete_char( int n )
	{
		return false;
	}

	bool c_edit::append_text( const char* str )
	{
		int len = ::GetWindowTextLength(m_hWnd);
		Edit_SetSel(m_hWnd, len, len);
		Edit_ReplaceSel(m_hWnd, str);
		return true;
	}

	void c_edit::limit_text( int sz )
	{
		SendMessage(EM_LIMITTEXT, sz == -1 ? 0 : sz);
	}

	void c_edit::set_text(const char* str)
	{
		::SetWindowText(*this, str);
	}

	HMENU c_edit::_load_default_menu()
	{
		return nullptr;
	}

	LRESULT c_edit::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		if (uMsg == WM_CONTEXTMENU) { 
			if (!_bUseDefMenu){
				return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
			}
			else{

			}
		}
		return __super::HandleMessage(uMsg, wParam, lParam, bHandled);
	}

	bool c_edit::is_read_only()
	{
		return !!(::GetWindowLongPtr(*this, GWL_STYLE) & ES_READONLY);
	}

	//////////////////////////////////////////////////////////////////////////
	bool c_rich_edit::back_delete_char( int n )
	{
		int cch;
		GETTEXTLENGTHEX gtl;

		if(n <= 0)
			return false;

		gtl.flags = GTL_DEFAULT;
		gtl.codepage = CP_ACP;
		cch = SendMessage(EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
		if(cch > 0){
			if(n >= cch){
				SetWindowText(m_hWnd, "");
				return true;
			}
			else{
				CHARRANGE rng;
				rng.cpMax = cch;
				rng.cpMin = cch - n;
				SendMessage(EM_EXSETSEL, 0, (LPARAM)&rng);
				SendMessage(EM_REPLACESEL, FALSE, (LPARAM)"");
			}
		}
		return true;
	}

	bool c_rich_edit::append_text( const char* str )
	{
		GETTEXTLENGTHEX gtl;
		CHARRANGE rng;
		int cch;

		gtl.flags = GTL_DEFAULT;
		gtl.codepage = CP_ACP;
		cch = SendMessage(EM_GETTEXTLENGTHEX, (WPARAM)&gtl);

		rng.cpMax = cch;
		rng.cpMin = cch;
		SendMessage(EM_EXSETSEL, 0, (LPARAM)&rng);
		Edit_ReplaceSel(m_hWnd,str);

		// Richedit bug: EM_SCROLLCARET will not work if a richedit gets no focus
		// http://stackoverflow.com/questions/9757134/scrolling-richedit-without-it-having-focus
		SendMessage(WM_VSCROLL, SB_BOTTOM);

		return true;
	}

	bool c_rich_edit::apply_linux_attributes(char* attrs)
	{
		const char* p = attrs;
		const char* end; 

		if (!attrs || !*attrs)
			return false;
		
		end = attrs + strlen(attrs) - 1;

		assert(*p++ == '\033');
		assert(*p++ == '[');

		switch (*end)
		{
		case 'm': // \033[a1;a1;...m
		{
			for (;*p != 'm';){
				if (*p == ';'){
					p++;
				}
				else if (*p >= '0' && *p <= '9'){
					int a = 0;
					p += read_integer(p, &a);
					apply_linux_attribute_m(a);
				}
				else{
					SMART_ASSERT(0)(*p).Fatal();
				}
			}
			break;
		}

		}


		return true;
	}

	bool c_rich_edit::apply_linux_attribute_m(int attr)
	{
		CHARFORMAT2 cf;
		static struct{
			int k;
			COLORREF v;
		}def_colors[] = 
		{
			{30, RGB(0,    0,  0)},
			{31, RGB(255,  0,  0)},
			{32, RGB(0,  255,  0)},
			{33, RGB(255,255,  0)},
			{34, RGB(0,    0,255)},
			{35, RGB(255,  0,255)},
			{36, RGB(0,  255,255)},
			{37, RGB(255,255,255)},
			{-1, RGB(0,0,0)      },
		};

		cf.cbSize = sizeof(cf);

		if(attr>=30 && attr<=37){
			cf.dwMask = CFM_COLOR;
			cf.dwEffects = 0;
			assert(_deffg>=30 && _deffg<=37);
			cf.crTextColor = def_colors[attr-30].v;
			SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		}
		else if(attr>=40 && attr<=47){
			cf.dwMask = CFM_BACKCOLOR;
			cf.dwEffects = 0;
			assert(_defbg>=40 && _defbg<=47);
			cf.crBackColor = def_colors[attr-40].v;
			SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		}
		else if(attr == 0){
			cf.dwMask = CFM_COLOR | CFM_BACKCOLOR | CFM_BOLD;
			cf.dwEffects = 0;
			assert( (_deffg>=30 && _deffg<=37)
				&& (_defbg>=40 && _defbg<=47));
			cf.crTextColor = def_colors[_deffg-30].v;
			cf.crBackColor = def_colors[_defbg-40].v;
			SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		}
		else if(attr == 1){
			cf.dwMask = CFM_BOLD;
			cf.dwEffects = CFE_BOLD;
			SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		}
		else{
			//debug_out(("unknown or unsupported Linux control format!\n"));
		}

		return true;
	}

	LRESULT c_rich_edit::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		return __super::HandleMessage(uMsg, wParam, lParam, bHandled);
	}

	bool c_rich_edit::get_sel_range(int* start /*= nullptr*/, int* end /*= nullptr*/)
	{
		CHARRANGE rng;
		SendMessage(EM_EXGETSEL, 0, LPARAM(&rng));
		if (start) *start = rng.cpMin;
		if (end)   *end = rng.cpMax;
		return rng.cpMin != rng.cpMax;
	}

	void c_rich_edit::do_copy()
	{
		SendMessage(WM_COPY);
	}

	void c_rich_edit::do_cut()
	{
		SendMessage(WM_CUT);
	}

	void c_rich_edit::do_paste()
	{
		SendMessage(WM_PASTE);
	}

	void c_rich_edit::do_delete()
	{
		SendMessage(WM_CLEAR);
	}

	void c_rich_edit::do_sel_all()
	{
		SendMessage(EM_SETSEL, 0, -1);
	}

	void c_rich_edit::set_default_text_fgcolor(COLORREF fg)
	{
		CHARFORMAT2 cf;
		::memset(&cf, 0, sizeof(cf));
		cf.cbSize = sizeof(cf);
		cf.dwMask = CFM_COLOR;
		cf.dwEffects = 0;
		cf.crTextColor = fg;
		SMART_ENSURE(SendMessage(EM_SETCHARFORMAT, 0, LPARAM(&cf)), != 0).Warning();
	}

	void c_rich_edit::set_default_text_bgcolor(COLORREF bg)
	{
		CHARFORMAT2 cf;
		cf.cbSize = sizeof(cf);
		cf.dwMask = CFM_BACKCOLOR;
		cf.dwEffects = 0;
		cf.crBackColor = bg;
		SMART_ENSURE(SendMessage(EM_SETCHARFORMAT, SCF_ALL, LPARAM(&cf)), != 0).Warning();
	}

    void c_rich_edit::limit_text(int sz) {
        // https://msdn.microsoft.com/en-us/library/windows/desktop/bb761647(v=vs.85).aspx
        SendMessage(EM_EXLIMITTEXT, 0, sz == -1 ? 0x7ffffffe : sz);
    }

}
}
