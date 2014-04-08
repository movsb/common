#include <Windows.h>
#include <windowsx.h>

#include "send_cmd.h"
#include "../res/resource.h"
#include "struct/Thunk.h"
#include "utils.h"
#include "deal.h"
#include "./struct/memory.h"


/**********************************************************
文件名称:send_cmd.cpp
文件路径:common/sources/send_cmd.cpp
创建时间:2013-11-2 14:00
文件作者:
文件说明:新写的, 用来替代原来的命令发送
**********************************************************/

HWND newCmdWindow(const char* fn)
{
	ASendCmd* cmd = new ASendCmd(fn);
	return cmd->GethWnd();
}

INT_PTR __stdcall CmdItemDialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	ACmdItem* pItem = (ACmdItem*)GetWindowLong(hDlg,GWL_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			((ACmdItem*)lParam)->SethWnd(hDlg);
			SetWindowLong(hDlg,GWL_USERDATA,LONG(lParam));//set to this
			ShowWindow(hDlg,SW_SHOW);
			//return TRUE;
			pItem = (ACmdItem*)lParam;
			break;
		}
	}
	if(pItem) return pItem->HandleMsg(uMsg,wParam,lParam);
	else	  return 0;
}

//////////////////////////////////////////////////////////////////////////

ACmdItem::ACmdItem(HWND hWndParent,command_item* pci,int id):
	m_pci(NULL)
{
	m_hParent = hWndParent;
	m_pci = pci;
	m_id = id;
	m_hWnd = CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_SEND_CMD),hWndParent,DLGPROC(CmdItemDialogProc),LPARAM(this));
}

ACmdItem::~ACmdItem()
{
	if(IsWindow(m_hWnd)){
		DestroyWindow(m_hWnd);
	}
}

INT_PTR ACmdItem::SetResult(LONG result,bool bHandled)
{
	if(bHandled) SetDlgMsgResult(m_hWnd,m_uMsg,result);
	return bHandled?TRUE:FALSE;
}

INT_PTR ACmdItem::HandleMsg(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	m_uMsg = uMsg;

	switch(uMsg)
	{
	case WM_COMMAND:

		if(LOWORD(wParam) == IDC_CMD_STATIC_TYPE && HIWORD(wParam)==STN_DBLCLK){
			if(m_pci->method[0]=='H'){
				m_pci->method[0] = 'C';
			}else{
				m_pci->method[0] = 'H';
			}
			SendMessage(hCmd,EM_SETMODIFY,TRUE,0);
			SetWindowText(hType,m_pci->method[0]=='H'?" 十六进制(双击改变)":" 字符数据(双击改变)");
			UpdatePci();
		}

		if(LOWORD(wParam)==IDC_CMD_BUTTON_SEND&&HIWORD(wParam)==BN_CLICKED){
			if(SendMessage(hCmd,EM_GETMODIFY,0,0)){
				UpdatePci();
			}

			if(m_pci->valid == 0){
				utils.msgbox(m_hWnd,MB_ICONEXCLAMATION,NULL,"无效的数据,不能发送, 请检查!");
				return 0;
			}

			if(m_pci->bytes < 1){
				utils.msgbox(m_hWnd,MB_ICONINFORMATION,NULL,"没有数据需要发送!");
				return 0;
			}
		}
		ClassMsg msg;
		msg.hWnd = m_hWnd;
		msg.uMsg = uMsg;
		msg.wParam = wParam;
		msg.lParam = lParam;
		msg.extra = (void*)m_pci;

		return SetResult(SendMessage(this->GetParent(),WM_NULL,0,LPARAM(&msg)));

	case WM_INITDIALOG:
		{
			hName  = GetDlgItem(m_hWnd,IDC_CMD_STATIC_NAME);
			hType  = GetDlgItem(m_hWnd,IDC_CMD_STATIC_TYPE);
			hSize  = GetDlgItem(m_hWnd,IDC_CMD_STATIC_SIZE);
			hCmd   = GetDlgItem(m_hWnd,IDC_CMD_EDIT_CMD);
			hSend  = GetDlgItem(m_hWnd,IDC_CMD_BUTTON_SEND);

			oldEditProc=(WNDPROC)SetWindowLong(hCmd,GWL_WNDPROC,LONG(m_EditThunk.Stdcall(this,&ACmdItem::EditProc)));

			UpdatePci();
			return FALSE;
		}
	case WM_DESTROY:
		SetWindowLong(hCmd,GWL_WNDPROC,LONG(oldEditProc));
		return 0;
	default:
		return 0;
	}
}

INT_PTR __stdcall ACmdItem::EditProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if(uMsg == WM_KEYDOWN)	{
		if(wParam == VK_RETURN){
			UpdatePci();
			return 0;
		}
	}
	return CallWindowProc(oldEditProc,hWnd,uMsg,wParam,lParam);
}

void ACmdItem::UpdatePci(void)
{
	if(SendMessage(hCmd,EM_GETMODIFY,0,0)){
		char t[CMDI_MAX_COMMAND]={0};
		GetWindowText(hCmd,t,sizeof(t)-1);
		strcpy(m_pci->command,t);
	}

	make_command_item(m_pci);

	char text[128];
	_snprintf(text,sizeof(text),"[ %d ] %s%s",m_id,m_pci->valid?"":"(无效)",m_pci->name);
	SetWindowText(hName,text);

	_snprintf(text,sizeof(text)," %s(双击改变)",m_pci->method[0]=='H'?"十六进制":m_pci->method[0]=='C'?"字符数据":"未知数据");
	SetWindowText(hType,text);

	_snprintf(text,sizeof(text),"数据大小: %d 字节%s",m_pci->valid?m_pci->bytes:0,m_pci->valid?"":"(NaN)");
	SetWindowText(hSize,text);

	SetWindowText(hCmd,m_pci->command);
}

//////////////////////////////////////////////////////////////////////////

ASendCmd::ASendCmd(const char* fn):
	pItem(NULL),
	nItems(0)
{
	strncpy(m_fn,fn,sizeof(m_fn));

	ParseCmdFile();

	void* pThunk = m_Thunk.Stdcall(this,&ASendCmd::DialogProc);
	CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_CMD_TEMPLATE),NULL,DLGPROC(pThunk),0);
	ShowWindow(m_hWnd,SW_SHOWNORMAL);
}

ASendCmd::~ASendCmd()
{
	//delete this;
}

INT_PTR ASendCmd::SetResult(LONG result,bool bHandled)
{
	if(bHandled) SetDlgMsgResult(m_hWnd,m_uMsg,result);
	return bHandled?TRUE:FALSE;
}

INT_PTR __stdcall ASendCmd::DialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	m_uMsg=uMsg;
	switch(uMsg)
	{
	case WM_NULL:
		{

			if(lParam==0) return 0;
			ACmdItem::ClassMsg* pMsg = reinterpret_cast<ACmdItem::ClassMsg*>(lParam);
			switch(pMsg->uMsg)
			{
			case WM_COMMAND:
				{
					if(LOWORD(pMsg->wParam)==IDC_CMD_BUTTON_SEND&&HIWORD(pMsg->wParam)==BN_CLICKED){
						command_item* pci = (command_item*)pMsg->extra;
						SEND_DATA* psd = deal.make_send_data(0,pci->data,pci->bytes);
						if(psd){
							deal.add_send_packet(psd);
						}else{
							utils.msgbox(m_hWnd,MB_ICONEXCLAMATION,NULL,"此时不能发送数据~ 请检查串口是否开启~");
						}
						return 0;
					}
				}
			}
			return 0;
		}
	case WM_INITDIALOG:
		{
			m_hWnd = hDlg;

			int height=0;

			for(size_t i=0; i<nItems; i++){
				ACmdItem* item = new ACmdItem(hDlg,pItem+i,i+1);
				RECT rc;
				GetWindowRect(item->GethWnd(),&rc);

				SetWindowPos(item->GethWnd(),0,0,(rc.bottom-rc.top)*i,0,0,SWP_NOZORDER|SWP_NOSIZE);;

				height = rc.bottom-rc.top;

				m_ACmdItems.push_back(item);
			}

			char* p = strrchr(m_fn,'\\');
			char buf[300];
			_snprintf(buf,sizeof(buf),"发送命令文件:%s",p?p+1:m_fn);
			SetWindowText(m_hWnd,buf);

			SCROLLINFO si = {0};
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			si.nMin=0; 
			si.nMax = nItems*height-1;
			si.nPos=si.nTrackPos=0;
			si.nPage = height;
			::SetScrollInfo(hDlg,SB_VERT,&si,TRUE);

			return FALSE;
		}
	case WM_CLOSE:
		{
			for(std::vector<ACmdItem*>::iterator it=m_ACmdItems.begin(); it!=m_ACmdItems.end(); it++){
				delete *it;
			}
			
			DestroyWindow(m_hWnd);
			return 0;
		}
	case WM_DESTROY:
		{
			memory.free_mem((void**)&pItem,"send_cmd");
			return 0;
		}
	case WM_NCDESTROY:
		{

			//对象指针不需要手动维护, 窗口关闭时自动销毁
			delete this;
			return 0;
		}
	case WM_VSCROLL:
		{
			int iVertPos;
			SCROLLINFO si = {0};
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hDlg,SB_VERT,&si);
			iVertPos = si.nPos;
			switch(LOWORD(wParam))
			{
			case SB_ENDSCROLL:						break;
			case SB_TOP:		si.nPos=si.nMin;	break;
			case SB_BOTTOM:		si.nPos=si.nMax;	break;
			case SB_LINEUP:		si.nPos--;			break;
			case SB_LINEDOWN:	si.nPos++;			break;
			case SB_PAGEUP:		si.nPos-=si.nPage;	break;
			case SB_PAGEDOWN:	si.nPos+=si.nPage;	break;
			case SB_THUMBTRACK:
			case SB_THUMBPOSITION:
				si.nPos = si.nTrackPos;break;
			}
			si.fMask = SIF_POS;
			::SetScrollInfo(hDlg,SB_VERT,&si,TRUE);
			::GetScrollInfo(hDlg,SB_VERT,&si);

			if(si.nPos != iVertPos){
				::ScrollWindow(hDlg,0,(iVertPos-si.nPos)/**si.nPage*/,NULL,NULL);
				UpdateWindow(hDlg);
			}

			return 0;
		}
	}
	return 0;
}

void ASendCmd::ParseCmdFile(void)
{
	parse_cmd(m_fn,&pItem,&nItems);
}
