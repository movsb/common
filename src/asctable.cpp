#include "StdAfx.h"

#include "asctable.h"

static char* __THIS_FILE__ = __FILE__;

/**********************************************************
文件名称:asctable.c
文件路径:../common/asctable.c
创建时间:2013-2-14,22:03:48,今天情人节
文件作者:女孩不哭
文件说明:显示出0~127的ASCII码
**********************************************************/

#define ASC_FONT_HEIGHT		14					//字体高度
#define ASC_TEXT_ASCENT		5					//行距
#define ASC_LINES_CLIENT	15					//每屏幕显示的行数
#define ASC_CLIENT_WIDTH	380					//窗口客户区宽度
#define ASC_MOUSE_DELTA		ASC_LINES_CLIENT	//鼠标滚轮滚一次步进值
#define ASC_TOTAL_ENTRY		256					//0~127,ASCII码总数,根据实际情况修改
#define ASC_FACENAME		"Consolas"			//字体名

static struct{
	unsigned char index;
	char* description;
}table1[32]={
	{0x00,"NUL NULL"},
	{0x01,"SOH Start of Heading"},
	{0x02,"STX Start of Text"},
	{0x03,"ETX End of Text"},
	{0x04,"EOT End of Transmission"},
	{0x05,"ENQ Enquiry"},
	{0x06,"ACK Acknowledge"},
	{0x07,"BEL Bell"},
	{0x08,"BS  Backspace"},
	{0x09,"TAB Horizontal Tab"},
	{0x0A,"LF  NL Line Heed,new line"},
	{0x0B,"VT  Vertical Tab"},
	{0x0C,"FF  NP Form Heed,new page"},
	{0x0D,"CR  Carriage Return"},
	{0x0E,"SO  Shift Out"},
	{0x0F,"SI  Shift In"},
	{0x10,"DLE Data Link Escape"},
	{0x11,"DC1 Device Control 1"},
	{0x12,"DC2 Device Control 2"},
	{0x13,"DC3 Device Control 3"},
	{0x14,"DC4 Device Control 4"},
	{0x15,"NAK Negative Acknowledge"},
	{0x16,"SYN Synchronous Idle"},
	{0x17,"ETB End of Trans. Block"},
	{0x18,"CAN Cancel"},
	{0x19,"EM  End of Medium"},
	{0x1A,"SUB Substitute"},
	{0x1B,"ESC Escape"},
	{0x1C,"FS  File Separator"},
	{0x1D,"GS  Group Separator"},
	{0x1E,"RS  Record Separator"},
	{0x1F,"US  Unit Separator"}
};

static COLORREF cr_table[] ={
	RGB(255,0,0),RGB(0,255,0),RGB(0,0,255),RGB(255,255,0),
	RGB(0,255,255),RGB(255,0,255),RGB(0,0,0),RGB(255,255,255),
};


namespace Common{
	int c_asctable_dlg::axisx = -1;
	int c_asctable_dlg::axisy = -1;
	int c_asctable_dlg::_fgcolor = 0;
	int c_asctable_dlg::_bgcolor = 6;

	LRESULT c_asctable_dlg::handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		switch(uMsg)
		{
		case WM_ERASEBKGND:
			return TRUE;

		case WM_LBUTTONDOWN:
			_fgcolor = (++_fgcolor) % __ARRAY_SIZE(cr_table);
			::InvalidateRect(m_hWnd, NULL, TRUE);
			return 0;
		case WM_RBUTTONDOWN:
			_bgcolor = (++_bgcolor) % __ARRAY_SIZE(cr_table);
			::InvalidateRect(m_hWnd, NULL, TRUE);
			return 0;
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc;
				RECT rc;
				HBRUSH hbr;

				int i;

				int x=0,y=0;
				char str[64];
				int len;

				HFONT hf;
				SCROLLINFO si;
				si.cbSize=sizeof(si);
				si.fMask=SIF_ALL;

				GetScrollInfo(m_hWnd,SB_VERT,&si);

				hdc = BeginPaint(m_hWnd,&ps);

				GetClientRect(m_hWnd, &rc);
				hbr = CreateSolidBrush(cr_table[_bgcolor]);
				FillRect(hdc, &rc, hbr);
				DeleteObject(hbr);

				hf=(HFONT)SelectObject(hdc,(HGDIOBJ)_hFont);
				SetBkMode(hdc,TRANSPARENT);
				SetTextColor(hdc,cr_table[_fgcolor]);

				len=sprintf(str,"%4s %4s  %4s  %s","十","八","十六","描述");
				TextOut(hdc,x,y,str,len);
				y += ASC_FONT_HEIGHT+ASC_TEXT_ASCENT;
				len=sprintf(str,"----------------------------------------");
				TextOut(hdc,x,y,str,len);
				y += ASC_FONT_HEIGHT+ASC_TEXT_ASCENT;

				for(i=si.nPos;i<si.nPos+ASC_LINES_CLIENT;i++){
					if(i<32){
						int index=table1[i].index;
						len=_snprintf(str,sizeof(str),"%4d, %03o,  %02X   %s",index,index,index,table1[index].description);
					}else if(i==32){
						len=_snprintf(str,sizeof(str),"%4d, %03o,  %02X   %s",32,32,32,"(Space)");
					}else if(i<127){
						len=_snprintf(str,sizeof(str),"%4d, %03o,  %02X   %c",i,i,i,(char)i);
					}else if(i==127){
						len=_snprintf(str,sizeof(str),"%4d, %03o,  %02X   %s",127,127,127,"(DEL)");
					}else if(i>127 && i<=255){
						len=_snprintf(str,sizeof(str),"%4d, %03o,  %02X   %c",i,i,i,(char)i);
					}else{
						break;
					}
					TextOut(hdc,x,y,str,len);
					y += ASC_FONT_HEIGHT+ASC_TEXT_ASCENT;
				}

				SelectObject(hdc,hf);
				EndPaint(m_hWnd,&ps);

				return 0;
			}
		case WM_VSCROLL:
			{
				SCROLLINFO si;
				ZeroMemory(&si,sizeof(si));
				si.cbSize = sizeof(si);
				si.fMask = SIF_ALL;
				GetScrollInfo(m_hWnd,SB_VERT,&si);
				switch(LOWORD(wParam))
				{
				case SB_ENDSCROLL:break;
				case SB_TOP:si.nPos=0;break;
				case SB_BOTTOM:si.nPos=ASC_TOTAL_ENTRY-1;break;
				case SB_LINEUP:si.nPos--;break;
				case SB_LINEDOWN:si.nPos++;break;
				case SB_PAGEUP:si.nPos-=si.nPage;break;
				case SB_PAGEDOWN:si.nPos+=si.nPage;break;
				case SB_THUMBTRACK:
				case SB_THUMBPOSITION:
					si.nPos = si.nTrackPos;break;
				}
				if(LOWORD(wParam)==SB_LINEUP && si.nPos < si.nMin ||
					LOWORD(wParam)==SB_LINEDOWN && si.nPos+ASC_LINES_CLIENT-1 > si.nMax)
				{
					MessageBeep(MB_OK);
					return 0;
				}
				if(si.nPos<si.nMin) si.nPos=si.nMin;
				if(si.nPos+(int)si.nPage-1>si.nMax) 
					si.nPos=si.nMax-si.nPage+1;
				SetScrollInfo(m_hWnd,SB_VERT,&si,TRUE);
				InvalidateRect(m_hWnd,NULL,TRUE);
				return 0;
			}
		case WM_MOUSEWHEEL:
			{
				int delta;
				SCROLLINFO si;
				delta = (short)HIWORD(wParam);//WHEEL_DELTA
				ZeroMemory(&si, sizeof(SCROLLINFO));
				si.fMask = SIF_ALL;
				si.cbSize = sizeof(SCROLLINFO);
				GetScrollInfo(m_hWnd,SB_VERT,&si);
				if(si.nPos==ASC_TOTAL_ENTRY-ASC_LINES_CLIENT&&delta<0 || si.nPos==0 && delta>0)
				{
					MessageBeep(MB_OK);
					return 0;
				}
				if(delta < 0) si.nPos+=ASC_MOUSE_DELTA;
				if(delta > 0) si.nPos-=ASC_MOUSE_DELTA;
				if (si.nPos < si.nMin) si.nPos = si.nMin;
				if (si.nPos+(int)si.nPage-1 > si.nMax) 
					si.nPos = si.nMax-si.nPage+1;
				SetScrollInfo(m_hWnd,SB_VERT,&si,TRUE);
				InvalidateRect(m_hWnd,NULL,TRUE);
				return 0;
			}
		case WM_INITDIALOG:
			{
				::SetWindowText(m_hWnd, "ASCII码表: 左键: 前景色, 右键: 背景色");
				SCROLLINFO si;
				si.cbSize = sizeof(si);
				si.fMask = SIF_ALL;
				si.nMin=0; 
				si.nMax = ASC_TOTAL_ENTRY-1;
				si.nPos=si.nTrackPos=0;
				si.nPage = ASC_LINES_CLIENT;
				SetScrollInfo(m_hWnd,SB_VERT,&si,TRUE);

				for(;;){
					RECT rcW;RECT rcC;
					int borderheight;
					GetWindowRect(m_hWnd,&rcW);
					GetClientRect(m_hWnd,&rcC);
					borderheight = (rcW.bottom-rcW.top)-(rcC.bottom-rcC.top);
					MoveWindow(m_hWnd,axisx,axisy,ASC_CLIENT_WIDTH+16, // "猜的"
						borderheight+(ASC_LINES_CLIENT+2)*(ASC_FONT_HEIGHT+ASC_TEXT_ASCENT),TRUE);
					break;	ZeroMemory(&si,sizeof(si));
			
				}

				for(;;){
					LOGFONT lf = { 0 };
					GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
					strncpy(lf.lfFaceName, ASC_FACENAME, LF_FACESIZE);
					lf.lfCharSet = DEFAULT_CHARSET;
					lf.lfHeight = -ASC_FONT_HEIGHT;
					_hFont = CreateFontIndirect(&lf);
					break;
				}

				if (axisx == -1 && axisy == -1)
					CenterWindow();

				return 0;
			}
		case WM_CLOSE:
			{
				RECT rc;
				GetWindowRect(m_hWnd,&rc);
				axisx=rc.left;
				axisy=rc.top;
				DeleteObject(_hFont);
			}
			break;
		}
		return __super::handle_message(uMsg, wParam, lParam, bHandled);
	}

	c_asctable_dlg::c_asctable_dlg()
		: _hFont(0)
	{

	}

	LPCTSTR c_asctable_dlg::get_skin_xml() const
	{
		return 
			"<Window size=\"400,380\">"
			"	<Vertical>"
			"		<Vertical>"
			"		</Vertical>"
			"	</Vertical>"
			"</Window>";
	}

	DWORD c_asctable_dlg::get_window_style() const
	{
		return WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX &~ WS_SIZEBOX;
	}

}
