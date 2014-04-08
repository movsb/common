#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <stdio.h>
#include "asctable.h"
#include "utils.h"		//center_window
#include "msg.h"		//handle.hWndMain
#include "../res/resource.h"	//IDI_ICON1

static char* __THIS_FILE__ = __FILE__;

/**********************************************************
文件名称:asctable.c
文件路径:../common/asctable.c
创建时间:2013-2-14,22:03:48,今天情人节
文件作者:女孩不哭
文件说明:显示出0~127的ASCII码
**********************************************************/

#define ASC_FONT_HEIGHT		12					//字体高度
#define ASC_TEXT_ASCENT		5					//行距
#define ASC_LINES_CLIENT	15					//每屏幕显示的行数
#define ASC_CLIENT_WIDTH	350					//窗口客户区宽度
#define ASC_MOUSE_DELTA		ASC_LINES_CLIENT	//鼠标滚轮滚一次步进值
#define ASC_TOTAL_ENTRY		128					//0~127,ASCII码总数,根据实际情况修改

struct{
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

static HFONT hFont;
static HWND hWndAsciiTable;
//COLORREF cur_cr=RGB(255,0,0);

//用于保存窗口坐标
static int axisx=0,axisy=0;

COLORREF set_text_color(int update)
{
	static struct{
		int cur_cr;
		COLORREF cr[8];
	}cr_table={
		0,
		{RGB(255,0,0),RGB(0,255,0),RGB(0,0,255),RGB(255,255,0),
		RGB(0,255,255),RGB(255,0,255),RGB(0,0,0),RGB(255,255,255)}
	};
	COLORREF cur_cr=cr_table.cr[cr_table.cur_cr];
	if(update)
		cr_table.cur_cr++;
	if(cr_table.cur_cr==__ARRAY_SIZE(cr_table.cr))
		cr_table.cur_cr=0;
	return cur_cr;
}

LRESULT CALLBACK AscWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_RBUTTONDOWN:
		set_text_color(1);
		InvalidateRect(hWnd,NULL,TRUE);
		return 0;
	case WM_LBUTTONDOWN:
		SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0);
		return 0;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;

			int i;

			int x=0,y=0;
			char str[64];
			int len;

			HFONT hf;
			SCROLLINFO si;
			si.cbSize=sizeof(si);
			si.fMask=SIF_ALL;

			GetScrollInfo(hWnd,SB_VERT,&si);

			hdc = BeginPaint(hWnd,&ps);
			hf=(HFONT)SelectObject(hdc,(HGDIOBJ)hFont);
			SetBkMode(hdc,TRANSPARENT);
			//SetTextColor(hdc,ASC_TEXT_COLOR);
			SetTextColor(hdc,set_text_color(0));
			
			len=sprintf(str,"DEC OCT HEX   Description");
			TextOut(hdc,x,y,str,len);
			y += ASC_FONT_HEIGHT+ASC_TEXT_ASCENT;
			len=sprintf(str,"----------------------------------------");
			TextOut(hdc,x,y,str,len);
			y += ASC_FONT_HEIGHT+ASC_TEXT_ASCENT;

			for(i=si.nPos;i<si.nPos+ASC_LINES_CLIENT;i++){
				if(i<32){
					int index=table1[i].index;
					len=_snprintf(str,sizeof(str),"%3d,%03o, %02X   %s",index,index,index,table1[index].description);
				}else if(i==32){
					len=_snprintf(str,sizeof(str),"%3d,%03o, %02X   %s",32,32,32,"(Space)");
				}else if(i<127){
					len=_snprintf(str,sizeof(str),"%3d,%03o, %02X   %c",i,i,i,(char)i);
				}else if(i==127){
					len=_snprintf(str,sizeof(str),"%3d,%03o, %02X   %s",127,127,127,"(DEL)");
				}else{
					break;
				}
				TextOut(hdc,x,y,str,len);
				y += ASC_FONT_HEIGHT+ASC_TEXT_ASCENT;
			}

			SelectObject(hdc,hf);
			EndPaint(hWnd,&ps);
			
			return 0;
		}
	case WM_VSCROLL:
		{
			SCROLLINFO si;
			ZeroMemory(&si,sizeof(si));
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hWnd,SB_VERT,&si);
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
			SetScrollInfo(hWnd,SB_VERT,&si,TRUE);
			InvalidateRect(hWnd,NULL,TRUE);
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
			GetScrollInfo(hWnd,SB_VERT,&si);
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
			SetScrollInfo(hWnd,SB_VERT,&si,TRUE);
			InvalidateRect(hWnd,NULL,TRUE);
			return 0;
		}
	case WM_CREATE:
		{
			SCROLLINFO si;
			ZeroMemory(&si,sizeof(si));
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			si.nMin=0; 
			si.nMax = ASC_TOTAL_ENTRY-1;
			si.nPos=si.nTrackPos=0;
			si.nPage = ASC_LINES_CLIENT;
			SetScrollInfo(hWnd,SB_VERT,&si,TRUE);

			for(;;){
				RECT rcW;RECT rcC;
				int borderheight;
				GetWindowRect(hWnd,&rcW);
				GetClientRect(hWnd,&rcC);
				borderheight = (rcW.bottom-rcW.top)-(rcC.bottom-rcC.top);
				MoveWindow(hWnd,axisx,axisy,rcW.right-rcW.left,
					borderheight+(ASC_LINES_CLIENT+2)*(ASC_FONT_HEIGHT+ASC_TEXT_ASCENT),TRUE);
				if(axisx==0 && axisy==0) utils.center_window(hWnd,msg.hWndMain);
				break;
			}

			hFont = CreateFont(ASC_FONT_HEIGHT,8,0,0,FW_REGULAR,FALSE,FALSE,FALSE,
				ANSI_CHARSET,OUT_DEVICE_PRECIS,CLIP_MASK,
				DEFAULT_QUALITY, DEFAULT_PITCH,"Courier");
			return 0;
		}
	case WM_CLOSE:
		{
			RECT rc;
			GetWindowRect(hWnd,&rc);
			axisx=rc.left;
			axisy=rc.top;
			DeleteObject(hFont);
			SendMessage(msg.hWndMain,WM_APP+0,1,(LPARAM)hWndAsciiTable);
			DestroyWindow(hWnd);
			hWndAsciiTable=NULL;
			return 0;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int ShowAsciiTable(void)
{
	WNDCLASSEX wcx = {0};
	HINSTANCE hInst = NULL;
	if(hWndAsciiTable){
		if(IsIconic(hWndAsciiTable))
			ShowWindow(hWndAsciiTable,SW_RESTORE);
		SetWindowPos(hWndAsciiTable,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		return 1;
	}
	hInst = GetModuleHandle(NULL);
	wcx.cbSize = sizeof(wcx);
	wcx.style = CS_HREDRAW|CS_VREDRAW;
	wcx.lpfnWndProc = AscWndProc;
	wcx.hInstance = hInst;
	wcx.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcx.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
	wcx.lpszMenuName = NULL;
	wcx.lpszClassName = "AsciiTable";
	wcx.hIconSm = wcx.hIcon;
	RegisterClassEx(&wcx);
	hWndAsciiTable = CreateWindowEx(0, "AsciiTable", "ASCII 码表(鼠标左键拖动,右键换色)",
		WS_OVERLAPPEDWINDOW&~WS_MAXIMIZEBOX&~WS_SIZEBOX|WS_VSCROLL,
		CW_USEDEFAULT, CW_USEDEFAULT, ASC_CLIENT_WIDTH, 0,
		msg.hWndMain, NULL, hInst, NULL); 
	if(hWndAsciiTable == NULL) return 0;
	ShowWindow(hWndAsciiTable, SW_SHOWNORMAL);
	UpdateWindow(hWndAsciiTable);
	SendMessage(msg.hWndMain,WM_APP+0,0,(LPARAM)hWndAsciiTable);
	return 1;
}
