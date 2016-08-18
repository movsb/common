#include "stdafx.h"

#include "../res/resource.h"

#include "about.h"
#include "asctable.h"
#include "SendCmd.h"
#include "msg.h"
#include "comm.h"

static char* __THIS_FILE__  = __FILE__;

namespace Common {
	//////////////////////////////////////////////////////////////////////////
	CComWnd::CComWnd()
		: m_layout(0)
		, m_hAccel(0)
	{
		_b_recv_char_edit_fullscreen = false;
		_b_send_data_format_hex = false; // 字符
		_send_data_format_hex   = SendDataFormatHex::sdfh_kNone;
		_send_data_format_char  = SendDataFormatChar::sdfc_kNone;
		_recv_cur_edit          = NULL;
        _b_refresh_comport      = false;
	}

	CComWnd::~CComWnd()
	{

	}

	LRESULT CComWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		switch(uMsg)
		{
		case WM_INITDIALOG:		return on_create(m_hWnd, GetModuleHandle(0));
		case WM_VSCROLL: 
		case WM_HSCROLL:		return on_scroll(uMsg, wParam, lParam);
		case WM_SIZE:			
								__super::HandleMessage(uMsg, wParam, lParam, bHandled);
								return on_size(LOWORD(lParam), HIWORD(lParam));
		case WM_CLOSE:			return on_close();
		case WM_COMMAND:		return on_command(HWND(lParam), LOWORD(wParam), HIWORD(wParam));
		case WM_DEVICECHANGE:	return on_device_change(wParam, (DEV_BROADCAST_HDR*)lParam);
		case WM_SETTINGCHANGE:	return on_setting_change(wParam, LPCTSTR(lParam));
		case WM_CONTEXTMENU:	return on_contextmenu(HWND(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		case WM_LBUTTONDOWN:	return SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, 0);
		}
		if (uMsg >= WM_APP && uMsg <= 0xBFFF)
			return on_app(uMsg, wParam, lParam);
        else if(uMsg == _command_message) {
            return OnCommCommand();
        }

		return __super::HandleMessage(uMsg, wParam, lParam, bHandled);
	}

	void CComWnd::OnFinalMessage( HWND hWnd )
	{
		__super::OnFinalMessage(hWnd);
		PostQuitMessage(0);
	}

	LRESULT CComWnd::on_create( HWND hWnd, HINSTANCE hInstance )
	{
		SetWindowText(hWnd, COMMON_NAME_AND_VERSION);

		memory.set_notifier(this);

		struct {HWND* phwnd; UINT  id;}hwndlist[] = {
				{&_hCP,		IDC_CBO_CP},
				{&_hBR,		IDC_CBO_BR},
				{&_hPA,		IDC_CBO_CHK},
				{&_hSB,		IDC_CBO_STOP},
				{&_hDB,		IDC_CBO_DATA},
				{&_hStatus,	IDC_STATIC_STATUS},
				{&_hOpen,	IDC_BTN_OPEN},
		};

		for (int i = 0; i < sizeof(hwndlist) / sizeof(hwndlist[0]); i++){
			SMART_ENSURE(*hwndlist[i].phwnd = ::GetDlgItem(m_hWnd, hwndlist[i].id), !=NULL)(i).Fatal();
		}

		editor_recv_char()->Create(hWnd, "", WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_READONLY |
			ES_MULTILINE | ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL ,
			WS_EX_CLIENTEDGE,
			0,0,0,0, (HMENU)IDC_EDIT_RECV2);
		WNDPROC new_rich_proc = static_cast<WNDPROC>(_thunk_rich_edit.Stdcall(this, &CComWnd::RichEditProc));
		_thunk_rich_edit_old_proc = SubclassWindow(*editor_recv_char(), new_rich_proc);
		::ImmAssociateContext(*editor_recv_char(), nullptr);

		editor_recv_hex()->Attach(::GetDlgItem(hWnd, IDC_EDIT_RECV));
		editor_send()->Attach(::GetDlgItem(hWnd, IDC_EDIT_SEND));

		editor_recv_hex()->limit_text(-1);
		editor_recv_char()->limit_text(-1);
		editor_send()->limit_text(-1);

		SendMessage(WM_SETICON, ICON_SMALL, LPARAM(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1))));

		m_hAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
		m_wndmgr.AcceleratorTranslator() = this;

		m_layout = ::layout_new(hWnd, MAKEINTRESOURCE(IDR_RCDATA2), hInstance);
		layout_visible(layout_control(m_layout, "recv_wnd_recv"), FALSE);
		layout_resize(m_layout, NULL);

		// 界面元素
		::SendMessage(_hCP, CB_SETDROPPEDWIDTH, 350, 0);
		::SetDlgItemText(m_hWnd, IDC_STATIC_TIMER, "00:00:00");
		
		// 界面预定义
		switch_simple_ui(true, false);
		switch_window_top_most(true, false);
		switch_send_data_format(true, false);
		switch_recv_data_format(true, false);
		switch_auto_send(true, false, -1);

		// 窗口关闭事件
		_window_close_handler.add([&](){
			if (_b_recv_char_edit_fullscreen){
				_b_recv_char_edit_fullscreen = false;
				switch_rich_edit_fullscreen(_b_recv_char_edit_fullscreen);
				return true;
			}
			return false;
		});

		_window_close_handler.add([&](){
			if (_comm.is_opened()){
				com_try_close(true);
				_timer.stop();
                if(_auto_send_timer.is_running())
                    _auto_send_timer.stop();
			}
			return false;
		});

		_window_close_handler.add([&](){
			save_to_config_file();
			return false;
		});

		// 相关接口
        _command_message = ::RegisterWindowMessage("Common:CommandNotifyMessage");
        _command_notifier.init(*this, _command_message);
        _comm.set_notifier(&_command_notifier);

		_timer.set_period(1000);
		_timer.set_timer(this);
		_timer.set_notifier(this);
		_auto_send_timer.set_period(1000);
		_auto_send_timer.set_period_timer(this);
		_auto_send_timer.set_notifier(this);

		// 接收器
		_hex_data_receiver.set_editor(&_recv_hex_edit);
		_text_data_receiver.set_editor(&_recv_char_edit);
		_comm.add_data_receiver(&_hex_data_receiver);
		_comm.add_data_receiver(&_text_data_receiver);
		_comm.add_data_receiver(&_file_data_receiver);


		com_update_item_list();
		com_add_prompt_if_no_cp_presents();

		// 从配置文件加载配置
		init_from_config_file();

		// 欢迎语
		update_status("欢迎使用 Common串口调试工具! Enjoy! :-)");

		return 0;
	}

	LRESULT CComWnd::on_scroll( UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		layout_scroll(m_layout, uMsg, wParam, lParam);
		return 0;
	}

	LRESULT CComWnd::on_size( int width,int height )
	{
		layout_resize(m_layout, NULL);
		return 0;
	}

	LRESULT CComWnd::on_close()
	{
		if (!_window_close_handler.call_observers())
			return 0;

		DestroyWindow();
		return 0;
	}

	LRESULT CComWnd::on_destroy()
	{
		return 0;
	}

	LRESULT CComWnd::on_command( HWND hWndCtrl, int id, int codeNotify )
	{
		if (!hWndCtrl){
			if (codeNotify == 0)
				return on_command_menu(id);
			else
				return on_command_acce(id);
		}
		else{
			return on_command_ctrl(hWndCtrl, id, codeNotify);
		}
	}

	LRESULT CComWnd::on_device_change( WPARAM event, DEV_BROADCAST_HDR* pDBH )
	{
		if(event==DBT_DEVICEARRIVAL || event==DBT_DEVICEREMOVECOMPLETE){
			if (pDBH->dbch_devicetype == DBT_DEVTYP_PORT){
				DEV_BROADCAST_PORT* pPort = reinterpret_cast<DEV_BROADCAST_PORT*>(pDBH);
				const char* name = &pPort->dbcp_name[0];
				if (_strnicmp("COM", name, 3) == 0){
					int comid = atoi(name + 3);
					if (event == DBT_DEVICEARRIVAL){
						update_status("串口设备 %s 已插入!", name);
						if (!_comm.is_opened()){
							com_update_comport_list_and_select_current();
						}
                        else {
                            _b_refresh_comport = true;
                        }
					}
					else{
						update_status("串口设备 %s 已移除!", name);
						// 保持当前选中的设备依然为选中状态
						if (!_comm.is_opened()){
							com_update_comport_list_and_select_current();
						}
						else{ // 如果移除的是当前COM
							int index = ComboBox_GetCurSel(_hCP);
							c_comport* cp = index >= 0 ? (c_comport*)ComboBox_GetItemData(_hCP, index) : nullptr;
							int comidcur = (int)cp > 0xFFFF ? cp->get_i() : 0;
							if (comid == comidcur){
								com_openclose();
							}
                            else {
                                _b_refresh_comport = true;
                            }
						}
					}

				}
			}
		}
		return 0;
	}

	LRESULT CComWnd::on_setting_change(WPARAM wParam, LPCTSTR area)
	{
		if (area && strcmp(area, "Ports")==0) {
            if(!_comm.is_opened())
                com_update_comport_list_and_select_current();
            else
                _b_refresh_comport = true;

		}
		return 0;
	}

	LRESULT CComWnd::on_app(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		PrivateMessage pm = (PrivateMessage)uMsg;
		switch (pm)
		{
		case kUpdateStatus:
		{
			::SetWindowText(_hStatus, (LPCTSTR)lParam);
			return 0;
		}
		case kUpdateTimer:
		{
			const char* tstr = (const char*)lParam;
			::SetDlgItemText(m_hWnd, IDC_STATIC_TIMER, tstr);
			return 0;
		}
		case kMessageBox:
		{
			auto p = reinterpret_cast<void**>(lParam);
			auto msg = reinterpret_cast<std::string*>(p[0]);
			auto cap = reinterpret_cast<std::string*>(p[1]);
			auto ico = reinterpret_cast<int*>(p[2]);

			int x = ::MessageBox(m_hWnd, msg->c_str(), cap->c_str(), *ico);

			delete msg;
			delete cap;
			delete ico;

			delete[] p;

			return x;
		}
		case kAutoSend:
			debug_out(("自动发送中...\n"));
			com_do_send(true);
			return 0;
		}
		return 0;
	}

	LRESULT CComWnd::on_contextmenu(HWND hwnd, int x, int y)
	{
		if (hwnd == _recv_char_edit){
			HMENU hMenu = ::LoadMenu(theApp, MAKEINTRESOURCE(MENU_RICHEDIT_CONTEXTMENU));
			HMENU hSubMenu0 = ::GetSubMenu(hMenu, 0);

			bool bsel = _recv_char_edit.get_sel_range();
			::EnableMenuItem(hSubMenu0, ID_EDITCONTEXTMENU_COPY, bsel ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(hSubMenu0, ID_EDITCONTEXTMENU_CUT, bsel ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(hSubMenu0, ID_EDITCONTEXTMENU_DELETE, bsel ? MF_ENABLED : MF_DISABLED | MF_GRAYED);

			bool breadonly = _recv_char_edit.is_read_only();
			::EnableMenuItem(hSubMenu0, ID_EDITCONTEXTMENU_PASTE, (!breadonly && ::IsClipboardFormatAvailable(CF_TEXT)) ? MF_ENABLED : MF_DISABLED | MF_GRAYED);

			::CheckMenuItem(hSubMenu0, ID_EDITCONTEXTMENU_FULLSCREEN, _b_recv_char_edit_fullscreen ? MF_CHECKED : MF_UNCHECKED);

			::TrackPopupMenu(hSubMenu0, TPM_LEFTALIGN | TPM_LEFTBUTTON, x, y, 0, *this, nullptr);

		}
		return 0;
	}

	void CComWnd::com_update_item_list()
	{
		list_callback_ud ud;
		ud.that = this;

		struct {
			list_callback_ud::e_type type;
			i_com_list* plist;
			HWND hwnd;
		} ups[] = {
			{list_callback_ud::e_type::cp, _comm.comports()->update_list() , _hCP},
			{list_callback_ud::e_type::br, _comm.baudrates()->update_list() , _hBR},
			{list_callback_ud::e_type::pa, _comm.parities()->update_list() , _hPA},
			{list_callback_ud::e_type::sb, _comm.stopbits()->update_list() , _hSB},
			{list_callback_ud::e_type::db, _comm.databits()->update_list() , _hDB},
		};

		for(int i=0; i<sizeof(ups)/sizeof(*ups); i++){
			ud.type = ups[i].type;
			ud.hwnd = ups[i].hwnd;
			ComboBox_ResetContent(ud.hwnd);
			ups[i].plist->callback(&CComWnd::com_udpate_list_callback, &ud);
			if (ComboBox_GetCount(ud.hwnd) > 0){
				ComboBox_SetCurSel(ud.hwnd, 0);
			}
		}

		int ii = ComboBox_InsertString(_hBR, -1, "<输入>");
		ComboBox_SetItemData(_hBR, ii, 1);	// 1 - 自定义
	}

	void CComWnd::com_udpate_list_callback( void* ud, const t_com_item* t )
	{
		list_callback_ud* pud = (list_callback_ud*)ud;
		CComWnd* that = pud->that;
		int index = -1;

		if(pud->type == list_callback_ud::e_type::cp){
			c_comport* d = (c_comport*)t;
			SMART_ENSURE(index = ComboBox_InsertString(pud->hwnd, -1, d->get_id_and_name().c_str()), >= 0).Warning();
		}
		else{
			SMART_ENSURE(index=ComboBox_InsertString(pud->hwnd, -1, t->get_s().c_str()), >= 0).Warning();
		}
		if (index >= 0){
			ComboBox_SetItemData(pud->hwnd, index, t);
		}
	}

	bool CComWnd::com_flush_settings_from_combobox()
	{
		CComm::s_setting_comm ssc;

		// baudrate
		t_com_item*  item = (t_com_item*)ComboBox_GetItemData(_hBR, ComboBox_GetCurSel(_hBR));
		SMART_ASSERT((int)item > 0xffff).Fatal();
		ssc.baud_rate = item->get_i();

		// parity
		item = (t_com_item*)ComboBox_GetItemData(_hPA, ComboBox_GetCurSel(_hPA));
		ssc.parity = item->get_i();

		// databit
		item = (t_com_item*)ComboBox_GetItemData(_hDB, ComboBox_GetCurSel(_hDB));
		ssc.databit = item->get_i();

		// stopbit
		item = (t_com_item*)ComboBox_GetItemData(_hSB, ComboBox_GetCurSel(_hSB));
		ssc.stopbit = item->get_i();

		return _comm.setting_comm(&ssc);
	}

	void CComWnd::com_update_comport_list()
	{
		i_com_list* list = _comm.comports()->update_list();
		list_callback_ud ud;
		ud.that = this;
		ud.type = list_callback_ud::e_type::cp;
		ud.hwnd = _hCP;
		ComboBox_ResetContent(ud.hwnd);
		list->callback(&CComWnd::com_udpate_list_callback, &ud);
		if (ComboBox_GetCount(ud.hwnd) > 0){
			ComboBox_SetCurSel(ud.hwnd, 0);
		}
	}

	LRESULT CComWnd::on_command_menu(int id)
	{
		switch (id)
		{
		// 工具箱/帮助菜单
		case MENU_OTHER_HELP:		(new c_about_dlg)->do_modal(*this); break;
		//case MENU_OTHER_STR2HEX:	(new c_str2hex_dlg)->do_modeless(this); break;
		case MENU_OTHER_ASCII:		(new c_asctable_dlg)->do_modeless(*this);break;
		case MENU_OTHER_CALC:		::ShellExecute(m_hWnd, "open", "calc", NULL, NULL, SW_SHOWNORMAL);	break;
		case MENU_OTHER_NOTEPAD:	::ShellExecute(m_hWnd, "open", "notepad", NULL, NULL, SW_SHOWNORMAL); break;
		case MENU_OTHER_DEVICEMGR:	::ShellExecute(m_hWnd, "open", "devmgmt.msc", NULL, NULL, SW_SHOWNORMAL); break;

		case MENU_OTHER_MONITOR:
		case MENU_OTHER_DRAW:
			msgbox(MB_ICONINFORMATION,0,"not implemented!"); break;
		case MENU_OTHER_NEWVERSION:break;

		// 文本接收数据区菜单
		case ID_EDITCONTEXTMENU_COPY:		_recv_char_edit.do_copy(); break;
		case ID_EDITCONTEXTMENU_CUT:		_recv_char_edit.do_cut(); break;
		case ID_EDITCONTEXTMENU_PASTE:		_recv_char_edit.do_paste(); break;
		case ID_EDITCONTEXTMENU_DELETE:		_recv_char_edit.do_delete(); break;
		case ID_EDITCONTEXTMENU_CLRSCR:		_recv_char_edit.clear(); break;
		case ID_EDITCONTEXTMENU_SELALL:		_recv_char_edit.do_sel_all(); break;
		case ID_EDITCONTEXTMENU_FULLSCREEN:	
			_b_recv_char_edit_fullscreen = !_b_recv_char_edit_fullscreen;
			switch_rich_edit_fullscreen(_b_recv_char_edit_fullscreen); 
			break;
		case ID_EDITCONTEXTMENU_CALC:		::ShellExecute(m_hWnd, "open", "calc", NULL, NULL, SW_SHOWNORMAL);	break;

        case MENU_MORE_PINCTRL:
        {
            #include "pinctrl.h"
            show_pinctrl(m_hWnd, [&]() {return _comm.get_handle(); });
            return 0;
        }

		}
		return 0;
	}

	LRESULT CComWnd::on_command_acce(int id)
	{
		switch (id)
		{
		case IDACC_SEND:		return on_command_ctrl(GetDlgItem(m_hWnd, IDC_BTN_SEND), IDC_BTN_SEND, BN_CLICKED);
		case IDACC_OPEN:		return on_command_ctrl(GetDlgItem(m_hWnd, IDC_BTN_OPEN), IDC_BTN_OPEN, BN_CLICKED);
		case IDACC_CLRCOUNTER:	return on_command_ctrl(GetDlgItem(m_hWnd, IDC_BTN_CLR_COUNTER), IDC_BTN_CLR_COUNTER, BN_CLICKED);
		case IDACC_STOPDISP:	return on_command_ctrl(GetDlgItem(m_hWnd, IDC_BTN_STOPDISP), IDC_BTN_STOPDISP, BN_CLICKED);
		}
		return 0;
	}

	LRESULT CComWnd::on_command_ctrl(HWND hwnd, int id, int code)
	{
		switch (id)
		{
		case IDC_BTN_SAVEFILE:
		{
			if(_file_data_receiver.size() == 0) {
				msgbox(MB_ICONINFORMATION, "提示", "缓冲区没有数据。");
				return -1;
			}

			c_file_save_dlg dlg;
			dlg.set_title("选择保存文件名");
			dlg.set_filter("所有文件(*.*), 请手写文件名+扩展名\0*.*\0");
			if(!dlg.do_modal(*this)) return 0;

			c_binary_file file;
			if(!file.open(dlg.get_buffer(), "wb")) {
				msgerr("文件打开失败");
				return -1;
			}

			auto data = _file_data_receiver.data();
			auto size = _file_data_receiver.size();

			if(!file.write(data, (int)size)) {
				msgerr("文件写入失败。");
				return -1;
			}

			file.flush();
			file.close();

			msgbox(MB_ICONINFORMATION, dlg.get_buffer(), "文件已成功保存。\n文件大小: %d", size);
			return 0;
		}
		case IDC_BTN_SEND:
			if (code == BN_CLICKED){
				com_do_send(false);
				return 0;
			}
			break;
		case IDC_BTN_OPEN:
			if (code == BN_CLICKED){
				com_openclose();
				return 0;
			}
			break;
		case IDC_BTN_MORE_SETTINGS:
			if (code == BN_CLICKED){
				POINT pt;
				HMENU hMenu;
				::GetCursorPos(&pt);
				hMenu = ::GetSubMenu(::LoadMenu(theApp.instance(), MAKEINTRESOURCE(IDR_MENU_MORE)), 0);
				::TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, *this, NULL);
				return 0;
			}
			break;
		case IDC_CBO_CP:
			// todo: buggy, multi times update
			if (code == CBN_SELENDOK || code == CBN_SELENDCANCEL){
				if (ComboBox_GetCount(hwnd) == 1){
					int itemdata = ComboBox_GetItemData(hwnd, 0);
					if (itemdata == 0){
						com_update_comport_list();
						com_add_prompt_if_no_cp_presents();
					}
				}
			}
			break;
		case IDC_CBO_BR:
			if (code == CBN_SELENDOK){
				int index = ComboBox_GetCurSel(_hBR);
				int itemdata = ComboBox_GetItemData(_hBR, index);
				if (itemdata == 1){ // for custom defined
					class c_input_baudrate_dlg : public i_input_box
					{
					public:
						virtual bool try_close()
						{
							return true;
						}
						virtual bool check_valid(const char* str)
						{
							if (!_that->test_get_int_value() || _that->get_int_value()<=0){
								_notifier->msgbox(MB_ICONEXCLAMATION, nullptr, "请输入正整数值!");
								return false;

							}
							return true;
						}
						virtual void set_notifier(i_notifier* notifier)
						{
							_notifier = notifier;
						}
						virtual void set_this(c_input_box* that)
						{
							_that = that;
						}
						virtual const char* get_enter_text()
						{
							return "";
						}
						virtual const char* get_prompt_text()
						{
							return "请输入自定义波特率:";
						}

					protected:
						i_notifier* _notifier;
						c_input_box* _that;
					};

					c_input_baudrate_dlg cibd;
					c_input_box brinput(&cibd);
					brinput.do_modal(*this);
					if (brinput.get_dlg_code() == IDOK){
						int br = brinput.get_int_value();
						std::string s = brinput.get_string_value();
						const c_baudrate& item = _comm.baudrates()->add(c_baudrate(br, s.c_str(), false));
						index = ComboBox_InsertString(_hBR, index, s.c_str());
						ComboBox_SetItemData(_hBR, index, &item);
						ComboBox_SetCurSel(_hBR, index);
					}
					else{
						ComboBox_SetCurSel(_hBR, index - 1);
					}
					return 0;
				}
			}
			break;
		case IDC_RADIO_SEND_CHAR:
		case IDC_RADIO_SEND_HEX:
			if (code == BN_CLICKED){
				switch_send_data_format();
				return 0;
			}
			break;
		case IDC_RADIO_RECV_CHAR:
		case IDC_RADIO_RECV_HEX:
			if (code == BN_CLICKED){
				switch_recv_data_format();
				return 0;
			}
			break;
		case IDC_BTN_SEND_FMT_CONFIG:
			if (code == BN_CLICKED){
				bool bchar = is_send_data_format_char();
				c_send_data_format_dlg* psdf = new c_send_data_format_dlg(
					bchar, bchar ? &_send_data_format_char : &_send_data_format_hex);
				psdf->do_modal(*this);
				return 0;
			}
			break;
		case IDC_CHK_AUTO_SEND:
			switch_auto_send();
			return 0;
		// 接收数据中间的按钮
		case IDC_BTN_HELP:
			if(code==BN_CLICKED){
				HMENU hMenu;
				POINT pt;
				hMenu = ::GetSubMenu(::LoadMenu(theApp.instance(), MAKEINTRESOURCE(IDR_MENU_OTHER)), 0);
				::GetCursorPos(&pt);
				::TrackPopupMenu(hMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, *this, NULL);
				return 0;
			}
			break;
		// 发送数据按钮
		case IDC_BTN_CLR_COUNTER:
			if(code==BN_CLICKED){
				//未发送计数不需要清零
                _comm.reset_counter(true, true, false);
				return 0;
			}
			break;

		// 数据复制按钮
		case IDC_BTN_COPY_RECV:
			if (code == BN_CLICKED){
				if (_recv_cur_edit && ::IsWindow(*_recv_cur_edit)){
					com_copy_text_data_to_clipboard(*_recv_cur_edit);
					return 0;
				}
				else{
					msgbox(MB_ICONINFORMATION, "", "当前没有数据显示控件!");
					return 0;
				}
			}
			break;
		case IDC_BTN_COPY_SEND:
			if(code==BN_CLICKED){
				com_copy_text_data_to_clipboard(_send_edit);
				return 0;
			}
			break;

		// 数据清除按钮
		case IDC_BTN_CLR_RECV:
		case IDC_BTN_CLR_SEND:
			if (code == BN_CLICKED){
				if (id == IDC_BTN_CLR_SEND){
					editor_send()->clear();
				}
				else if (id == IDC_BTN_CLR_RECV){
					editor_recv_hex()->clear();
					editor_recv_char()->clear();
					_hex_data_receiver.set_count(0);
					_file_data_receiver.reset_buffer();
				}
				return 0;
			}
			break;
		// 置顶 && 简洁模式
		case IDC_CHK_TOP:
			if (code == BN_CLICKED){
				switch_window_top_most();
				return 0;
			}
			break;
		case IDC_CHECK_SIMPLE:
			if (code == BN_CLICKED){
				switch_simple_ui();
				return 0;
			}
			break;

		// 从文件加载
		case IDC_BTN_LOADFILE:
			if (code == BN_CLICKED){
				com_load_file();
				return 0;
			}
			break;
		}

		return 0;
	}
	  
	void CComWnd::com_lock_ui_panel(bool lock)
	{
		HWND ids[] = { _hCP,_hBR,_hPA,_hSB,_hDB};
		for (int i = 0; i < sizeof(ids) / sizeof(ids[0]); i++){
			::EnableWindow(ids[i], lock ? FALSE : TRUE);
		}
	}

	bool CComWnd::com_try_close(bool b_thread_started)
	{
		if (!_comm.is_opened())
			return true;

		if(b_thread_started){
			c_send_data_packet* psdp = _comm.alloc_packet(0);
			psdp->type = csdp_type::csdp_exit;
			_comm.put_packet(psdp, true);
			_comm.end_threads();
		}
		_comm.empty_packet_list();
		_comm.close();
		return true;
	}

	bool CComWnd::com_try_open()
	{
		int cursel = ComboBox_GetCurSel(_hCP);
		t_com_item* pi = (t_com_item*)(cursel == -1 ? 0 : ComboBox_GetItemData(_hCP,cursel));
		if (!pi){
			msgbox(MB_ICONEXCLAMATION, NULL, "没有可用的串口, 请点击串口列表刷新!");
			return false;
		}

		return _comm.open(pi->get_i());
	}

	void CComWnd::com_add_prompt_if_no_cp_presents()
	{
		int count = ComboBox_GetCount(_hCP);
		if (count == 0){
			ComboBox_InsertString(_hCP, -1, "< 没 有 找 到 任 何 可 用 的 串 口 ! >  点 击 刷 新 列 表");
			ComboBox_SetItemData(_hCP, 0, 0);
			update_status("没有找到可用的串口!");
		}
		else{
			update_status("共找到 %d 个串口设备!", count);
		}
	}

	void CComWnd::update_status(const char* fmt, ...)
	{
		va_list va;
		char smsg[1024] = { 0 };
		va_start(va, fmt);
		_vsnprintf(smsg, sizeof(smsg), fmt, va);
		va_end(va);
		SendMessage(kUpdateStatus, 0, LPARAM(smsg));
	}

	void CComWnd::com_update_open_btn_text()
	{
		::SetWindowText(_hOpen, _comm.is_opened() ? "关闭(&W)" : "打开(&W)");
	}

	void CComWnd::update_timer(int h, int m, int s)
	{
		char tstr[9];
		sprintf(tstr, "%02d:%02d:%02d", h, m, s);
		SendMessage(kUpdateTimer, 0, LPARAM(tstr));
	}

	void CComWnd::update_timer_period()
	{
		SendMessage(kAutoSend);
	}

	void CComWnd::switch_simple_ui(bool manual/* = false*/, bool bsimple/* = false*/)
	{
		if (manual){
			::CheckDlgButton(m_hWnd, IDC_CHECK_SIMPLE, bsimple ? BST_CHECKED : BST_UNCHECKED);
		}
		else{
			bsimple = !!::IsDlgButtonChecked(m_hWnd, IDC_CHECK_SIMPLE);
		}

		// 因为有些布局共用了相同的控件, 所以设置隐藏有先后顺序
		if (bsimple){ // 简洁模式
			layout_visible(layout_control(m_layout, "recv_btns"), FALSE);
			layout_visible(layout_control(m_layout, "simple_mode_help_btn"), TRUE);
			layout_visible(layout_control(m_layout, "simple_mode_panel"), TRUE);
		}
		else{
			layout_visible(layout_control(m_layout, "simple_mode_panel"), FALSE);
			layout_visible(layout_control(m_layout, "recv_btns"), TRUE);
		}

		layout_visible(layout_control(m_layout, "send_wnd"), !bsimple);
		layout_visible(layout_control(m_layout, "send_btns"), !bsimple);
		layout_visible(layout_control(m_layout, "auto_send"), !bsimple);
		layout_visible(layout_control(m_layout, "send_fmt"), !bsimple);

		layout_resize(m_layout, 0);
		return;
	}

	void CComWnd::switch_window_top_most(bool manual/*=false*/, bool topmost /*= true*/)
	{
		if (manual){
			::CheckDlgButton(m_hWnd, IDC_CHK_TOP, topmost ? BST_CHECKED : BST_UNCHECKED);
		}
		else{
			topmost = !!::IsDlgButtonChecked(m_hWnd, IDC_CHK_TOP);
		}

		::SetWindowPos(m_hWnd, topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	void CComWnd::switch_send_data_format(bool manual/*=false*/, bool bhex/*=false*/, DWORD fmthex/*=0*/, DWORD fmtchar/*=0*/)
	{
		if (manual){
			_b_send_data_format_hex = bhex;
			::CheckRadioButton(m_hWnd, IDC_RADIO_SEND_HEX, IDC_RADIO_SEND_CHAR,
				_b_send_data_format_hex ? IDC_RADIO_SEND_HEX : IDC_RADIO_SEND_CHAR);
		}
		else{
			_b_send_data_format_hex = !!::IsDlgButtonChecked(m_hWnd, IDC_RADIO_SEND_HEX);
		}
	}


	void CComWnd::switch_recv_data_format(bool manual /*= false*/, bool bhex /*= false*/, DWORD fmthex /*= 0*/, DWORD fmtchar /*= 0*/)
	{
		if (manual){
			_b_recv_data_format_hex = bhex;
			::CheckRadioButton(m_hWnd, IDC_RADIO_RECV_HEX, IDC_RADIO_RECV_CHAR,
				_b_recv_data_format_hex ? IDC_RADIO_RECV_HEX : IDC_RADIO_RECV_CHAR);
		}
		else{
			_b_recv_data_format_hex = !!::IsDlgButtonChecked(m_hWnd, IDC_RADIO_RECV_HEX);
		}

		_recv_cur_edit = is_recv_data_format_hex() ? &_recv_hex_edit : &_recv_char_edit;

		m_layout->FindControl("edit_recv_hex")->SetVisible(is_recv_data_format_hex());
		m_layout->FindControl("edit_recv_char")->SetVisible(is_recv_data_format_char());
	}

	void CComWnd::switch_auto_send(bool manual, bool bauto, int interval)
	{
		const int interval_min = 50;
		const int interval_max = 60000;
		const int interval_default = 1000;

		if (manual){
			bool valid = interval >= interval_min && interval <= interval_max;
			if (interval == -1) {
				valid = true;
				interval = interval_default;
			}

			::CheckDlgButton(m_hWnd, IDC_CHK_AUTO_SEND, bauto && valid ? BST_CHECKED : BST_UNCHECKED);
			::EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_DELAY), bauto && valid && _comm.is_opened() ? FALSE : TRUE);
			::SetDlgItemInt(m_hWnd, IDC_EDIT_DELAY, valid ? interval : interval_default, FALSE);
			if (!valid){
				msgbox(MB_ICONEXCLAMATION, nullptr, "自动发送时间不合法, 已设置为默认值!");
			}
			else{
				_auto_send_timer.set_period(interval);
			}
		}
		else{
			bool bStart = !!::IsDlgButtonChecked(m_hWnd, IDC_CHK_AUTO_SEND);
			if(_comm.is_opened()){
				if (bStart){
					BOOL bTranslated;
					int ti = ::GetDlgItemInt(m_hWnd, IDC_EDIT_DELAY, &bTranslated, FALSE);
					if (!bTranslated || !(ti >= interval_min && ti <= interval_max)){
						msgbox(MB_ICONEXCLAMATION, nullptr, "自动发送时间设置有误!");
						return;
					}

					::EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_DELAY), FALSE);
					_auto_send_timer.set_period(ti);
					_auto_send_timer.start();
				}
				else{
					::EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_DELAY), TRUE);
					_auto_send_timer.stop();
				}
			}
			else{
				::EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_DELAY), TRUE);
				_auto_send_timer.stop();
			}
		}
	}

	void CComWnd::com_copy_text_data_to_clipboard(HWND hwnd)
	{
		int len = ::GetWindowTextLength(hwnd)+1;
		char* pchar = new char[len];
		GetWindowText(hwnd, pchar, len);
		pchar[len - 1] = '\0';
		set_clipboard_data(pchar);
		delete[] pchar;
	}

	void CComWnd::com_load_file()
	{
		c_send_file_format_dlg sffdlg;
		c_binary_file bf;
		int file_size;

		sffdlg.do_modal(*this);
		SdkLayout::CTinyString selected = sffdlg.get_selected_type();
		if (selected.size()==0 || selected=="nothing")
			return;


		if (!_com_load_file_prompt_size(sffdlg.get_selected_type(), bf))
		{
			return;
		}

		bf.seek(0, SEEK_END);
		file_size = bf.tell();
		bf.seek(0, SEEK_SET);

		unsigned char* buffer = new unsigned char[file_size+1];
		buffer[file_size + 1 - 1] = '\0';
		if (!bf.read(buffer, file_size)){
			delete[] buffer;
			return;
		}

		if (selected == "hexseq"){
			editor_send()->set_text((char*)buffer);
			switch_send_data_format(true, true);
		}
		else if (selected == "text"){
			editor_send()->set_text((char*)buffer);
			switch_send_data_format(true, false);
		}
		else if (selected == "any"){
            const int line_cch = 16;
			int length = file_size;
			char* hexstr = c_text_formatting::hex2str(
				buffer, &length, line_cch, 0, NULL, 0, c_text_formatting::newline_type::NLT_CRLF);
			if (hexstr){
				editor_send()->set_text(hexstr);
				switch_send_data_format(true, true);
				memory.free((void**)&hexstr, "");
				switch_send_data_format(true, true);
			}
		}
		else if (selected == "cmd"){
			bf.close();
			delete[] buffer;
			sendcmd_try_load_xml(*this, bf.get_fn().c_str(), &_comm);
			return; // !!!
		}

		delete[] buffer;
	}

	bool CComWnd::_com_load_file_prompt_size(SdkLayout::CTinyString& selected, c_binary_file& bf)
	{
		c_file_open_dlg fodlg;
		if (selected == "text"){
			fodlg.set_title("选择一个纯文本文件...");
			fodlg.set_filter("文本文件(*.txt)\0*.txt\0所有文件(*.*)\0*.*\0");
		}
		else if (selected == "any"){
			fodlg.set_title("任意选择一个文件, 但不要太大...");
			fodlg.set_filter("所有文件(*.*)\0*.*\0");
		}
		else if (selected == "hexseq"){
			fodlg.set_title("选择一个包含16进制序列的文本, 内容应该是: XX XX XX XX ...");
			fodlg.set_filter("十六进制序列文本文件(*.txt)\0*.txt\0所有文件(*.*)\0*.*\0");
		}
		else if (selected == "cmd"){
			fodlg.set_title("命令文件能方便地发送一组相关的命令, 选择一个吧~");
			fodlg.set_filter("命令文本文件(*.xml)\0 * .xml\0所有文件(*.*)\0 * .*\0");
		}

		if (!fodlg.do_modal(*this) || !bf.open(fodlg.get_buffer(), "rb"))
			return false;

		const int max_load_size = 1 << 20;
		int load_size;
		bf.seek(0, SEEK_END);
		load_size = bf.tell();
		bf.seek(0, SEEK_SET);
		if (load_size > max_load_size)
			return msgbox(MB_ICONQUESTION | MB_YESNO,
			"文件太大",
			"文件: %s\n"
			"已经超过最大打开文件大小: %d 字节\n"
			"\n要继续吗?"
			, bf.get_fn().c_str()
			, max_load_size) == IDYES;
		else
			return true;
	}

	bool CComWnd::com_do_send(bool callfromautosend)
	{
		auto cancel_auto_send = [=](){
			if (callfromautosend){
				::CheckDlgButton(m_hWnd, IDC_CHK_AUTO_SEND, BST_UNCHECKED);
				switch_auto_send();
			}
		};

		int len = ::GetWindowTextLength(_send_edit);
		if (len <= 0) return true;

		char* text = NULL;
		if (len+1 > sizeof(_send_buffer))
			text = new char[len+1];
		else
			text = _send_buffer;

		*text = '\0';
		::GetWindowText(_send_edit, text, len+1);

		if (is_send_data_format_char()){
			switch (_send_data_format_char & 0x03)
			{
			case SendDataFormatChar::sdfc_kNoCrlf:
				len = c_text_formatting::remove_string_crlf(text);
				break;
			case SendDataFormatChar::sdfc_kCr:
				len = c_text_formatting::remove_string_lf(text);
				break;
			case SendDataFormatChar::sdfc_kLf:
				len = c_text_formatting::remove_string_cr(text);
				break;
			case SendDataFormatChar::sdfc_kCrlf:
				// 当前是 Edit 控件, 以 '\r\n' 换行, 无需作转换
				break;
			}

			if (_send_data_format_char & SendDataFormatChar::sdfc_kUseEscape){
				unsigned int n = c_text_formatting::parse_string_escape_char(text);
				len = n & 0x7FFFFFFF;
				if ((n & 0x80000000) == 0){
					cancel_auto_send();
					msgbox(MB_ICONEXCLAMATION, NULL,
						"解析转义字符串时遇到错误!\n\n"
						"在第 %d 个字符附近出现语法解析错误!",
						len
						);
					return false;
				}
			}
		}
		else{
			unsigned int n = c_text_formatting::str2hex(text, (unsigned char**)&text, len);
			len = n & 0x7FFFFFFF;
			if ((n & 0x80000000) == 0){
				cancel_auto_send();
				msgbox(MB_ICONEXCLAMATION, NULL, "发送区的数据解析错误, 请检查!\n\n是不是选错了发送数据的格式\?\n\n"
					"在第 %d 个字符附近出现语法解析错误!", len);
				return false;
			}
		}

		c_send_data_packet* packet = _comm.alloc_packet(len);
		::memcpy(&packet->data[0], text, len);

		if (text != _send_buffer)
			delete[] text;

		return _comm.put_packet(packet, false, callfromautosend);
	}

	void CComWnd::com_openclose()
	{
		if (_comm.is_opened()){
			if (com_try_close(true)){
				com_lock_ui_panel(false);
                if(_b_refresh_comport) {
                    com_update_comport_list_and_select_current();
                    _b_refresh_comport = false;
                }
				com_update_open_btn_text();
				_timer.stop();
				switch_auto_send();
			}
		}
		else{
			if (com_try_open()){
				if (!com_flush_settings_from_combobox()){
					com_try_close(false);
					return;
				}
				com_lock_ui_panel(true);
				_text_data_receiver.reset_buffer();
				_hex_data_receiver.reset_buffer();
				_file_data_receiver.reset_buffer();
				_comm.begin_threads();
				com_update_open_btn_text();
				update_status("串口已打开!");
				_timer.start();
			}
		}
		switch_auto_send();
	}

	void CComWnd::switch_rich_edit_fullscreen(bool full)
	{
		SdkLayout::CControlUI* p_main_wnd = m_layout->FindControl("main_wnd");
		SdkLayout::CControlUI* p_recv_rich = m_layout->FindControl("fullscreen_recv_rich_wnd");

		if (full){
			p_main_wnd->SetVisible(false);
			p_recv_rich->SetVisible(true);
		}
		else{
			p_recv_rich->SetVisible(false);
			p_main_wnd->SetVisible(true);
		}
		::SetFocus(*editor_recv_char());
	}

	LRESULT CALLBACK CComWnd::RichEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_CHAR){
			if (_comm.is_opened()){
				char ch = char(wParam);
				c_send_data_packet* psdp = _comm.alloc_packet(1);
				::memcpy(psdp->data, &ch, 1);
				if (!_comm.put_packet(psdp)){

				}
				return 0;
			}
			return 0;
		}
		else if (uMsg == WM_KEYDOWN){
			if (wParam == VK_F11){
				_b_recv_char_edit_fullscreen = !_b_recv_char_edit_fullscreen;
				switch_rich_edit_fullscreen(_b_recv_char_edit_fullscreen);
				return 0;
			}
		}
		else if(uMsg == WM_MBUTTONDOWN){
			editor_recv_char()->clear();
			return 0;
		}
		return CallWindowProc(_thunk_rich_edit_old_proc, hWnd, uMsg, wParam, lParam);
	}

	bool CComWnd::TranslateAccelerator(MSG* pmsg)
	{
		return !!::TranslateAccelerator(m_hWnd, m_hAccel, pmsg);
	}

	void CComWnd::init_from_config_file()
	{
		auto get_font_info = [](const std::string& s, std::string* face, int* sz){
			int pos = s.find(',');
			*face = s.substr(0, pos);
			*sz = atoi(&s[pos + 1]);
		};

		auto set_ctrl_font = [=](const char* ctrl, const std::string& face, int sz){
			SdkLayout::CControlUI* pRE = m_layout->FindControl(ctrl);
			HFONT hFont = m_layout->GetManager()->AddFont(face.c_str(), sz, false, false, false);
			pRE->SetFont(m_layout->GetManager()->GetFont(hFont));
		};

		if (auto item = comcfg->get_key("app.title")){
			::SetWindowText(m_hWnd, item->val().c_str());
		}
		if (auto item = comcfg->get_key("app.icon")){
			HICON hIcon = (HICON)LoadImage(NULL, item->val().c_str(), IMAGE_ICON, 48, 48, LR_LOADFROMFILE);
			if(hIcon != nullptr){
				SendMessage(WM_SETICON, ICON_SMALL, LPARAM(hIcon));
				SendMessage(WM_SETICON, ICON_BIG, LPARAM(hIcon));
			}
		}
		if (auto item = comcfg->get_key("gui.font")){
			std::string face;
			int sz;
			get_font_info(item->val(), &face, &sz);
			m_layout->SetDefFont(face.c_str(), sz);
		}
		if (auto item = comcfg->get_key("gui.recv.edit.char.font")){
			std::string face;
			int sz;
			get_font_info(item->val(), &face, &sz);
			set_ctrl_font("edit_recv_char", face, sz);
		}
		if (auto item = comcfg->get_key("gui.recv.edit.hex.font")){
			std::string face;
			int sz;
			get_font_info(item->val(), &face, &sz);
			set_ctrl_font("edit_recv_hex", face, sz);
		}
		if (auto item = comcfg->get_key("gui.recv.edit.char.fgcolor")){

		}
		if (auto item = comcfg->get_key("gui.recv.edit.char.bgcolor")){

		}
		if (auto item = comcfg->get_key("gui.fullscreen")){
			_b_recv_char_edit_fullscreen = item->get_bool();
			switch_rich_edit_fullscreen(_b_recv_char_edit_fullscreen);
		}
		if (auto item = comcfg->get_key("gui.simplemode")){
			switch_simple_ui(true, item->get_bool());
		}
		if (auto item = comcfg->get_key("gui.topmost")){
			switch_window_top_most(true, item->get_bool());
		}

		// 数据发送格式设置
		if (auto item = comcfg->get_key("comm.send.format")){
			switch_send_data_format(true, item->val() == "hex");
		}
		if (auto item = comcfg->get_key("comm.send.format.char.crlf")){
			const char* crlftype[] = { "none", "cr", "lf", "crlf" };
			if (item->val() == crlftype[0])  _send_data_format_char &= ~0x00000003; 
			else if (item->val() == crlftype[1]) _send_data_format_char |= SendDataFormatChar::sdfc_kCr;
			else if (item->val() == crlftype[2]) _send_data_format_char |= SendDataFormatChar::sdfc_kLf;
			else if (item->val() == crlftype[3]) _send_data_format_char |= SendDataFormatChar::sdfc_kCrlf;
		}
		if (auto item = comcfg->get_key("comm.send.format.char.escape")){
			if (item->get_bool()) _send_data_format_char |= SendDataFormatChar::sdfc_kUseEscape;
			else _send_data_format_char &= ~SendDataFormatChar::sdfc_kUseEscape;
		}

		// 串口参数配置
		if (auto item = comcfg->get_key("comm.config.comport")){
			auto& cp = *_comm.comports();
			if (cp.size()){
				for (int i = 0; i < cp.size(); i++){
					if (item->get_int() == cp[i].get_i()){
						ComboBox_SetCurSel(_hCP, i);
						break;
					}
				}
			}
		}
		if (auto item = comcfg->get_key("comm.config.baudrate")){
			std::vector<std::string> brs;
			split_string(&brs, item->val().c_str(), '|');
			if (brs.size() > 1){
				for (int i = 0; i < (int)brs.size() - 1; i++){
					auto& b = _comm.baudrates()->add(c_baudrate(atoi(brs[i].c_str()), brs[i].c_str(), false));
					int idx = ComboBox_InsertString(_hBR, ComboBox_GetCount(_hBR)-1, brs[i].c_str());
					ComboBox_SetItemData(_hBR, idx, &b);
				}
			}

			if (brs.size() > 0){
				int index = -1;
				auto li = _comm.baudrates();
				for (int i = 0; i < li->size(); i++){
					if (brs[brs.size()-1] == (*li)[i].get_s()){
						index = i;
						break;
					}
				}
				if (index != -1){
					ComboBox_SetCurSel(_hBR, index);
				}
			}
		}
		if (auto item = comcfg->get_key("comm.config.parity")){
			int index = -1;
			auto li = _comm.parities();
			for (int i = 0; i < li->size(); i++){
				if (item->get_int() == (*li)[i].get_i()){
					index = i;
					break;
				}
			}
			if (index != -1){
				ComboBox_SetCurSel(_hPA, index);
			}
		}
		if (auto item = comcfg->get_key("comm.config.databit")){
			int index = -1;
			auto li = _comm.databits();
			for (int i = 0; i < li->size(); i++){
				if (item->get_int() == (*li)[i].get_i()){
					index = i;
					break;
				}
			}
			if (index != -1){
				ComboBox_SetCurSel(_hDB, index);
			}
		}
		if (auto item = comcfg->get_key("comm.config.stopbit")){
			int index = -1;
			auto li = _comm.stopbits();
			for (int i = 0; i < li->size(); i++){
				if (item->get_int() == (*li)[i].get_i()){
					index = i;
					break;
				}
			}
			if (index != -1){
				ComboBox_SetCurSel(_hSB, index);
			}
		}

		// 自动发送
		bool bAutoSend = false;
		int  interval = -1;
		if (auto item = comcfg->get_key("comm.autosend.enable")){
			bAutoSend = item->get_bool();
		}
		if (auto item = comcfg->get_key("comm.autosend.interval")){
			interval = item->get_int();
			if (interval == 0)
				interval = -1;
		}
		switch_auto_send(true, bAutoSend, interval);
	}

	void CComWnd::save_to_config_file()
	{
		comcfg->set_key("gui.fullscreen", _b_recv_char_edit_fullscreen);
		comcfg->set_key("gui.simplemode", !!::IsDlgButtonChecked(m_hWnd, IDC_CHECK_SIMPLE));
		comcfg->set_key("gui.topmost", !!::IsDlgButtonChecked(m_hWnd, IDC_CHK_TOP));

		// 数据发送格式设置
		comcfg->set_key("comm.send.format", _b_send_data_format_hex ? "hex" : "char");

		const char* crlftype[] = { "none", "cr", "lf", "crlf" };
		comcfg->set_key("comm.send.format.char.crlf", crlftype[_send_data_format_char & 0x03]);

		comcfg->set_key("comm.send.format.char.escape",
			_send_data_format_char & SendDataFormatChar::sdfc_kUseEscape ? "true" : "false");

		// 串口参数配置
		auto get_cbo_item_data = [](HWND hcbo){
			int i = ComboBox_GetCurSel(hcbo);
			SMART_ASSERT(i >= 0)(i).Fatal();
			return reinterpret_cast<t_com_item*>(ComboBox_GetItemData(hcbo, i));
		};

		// 当前串口号
		int icp = ComboBox_GetCurSel(_hCP);
		c_comport* cp = nullptr;
		if (icp >= 0){
			cp = reinterpret_cast<c_comport*>(ComboBox_GetItemData(_hCP, icp));
			if ((int)cp <= 0xFFFF) cp = nullptr;
		}
		if (cp) {
			comcfg->set_key("comm.config.comport", cp->get_i());
		}
		else{
			comcfg->set_key("comm.config.comport", "");
		}

		// 当前波特率
		auto& brs = *_comm.baudrates();
		std::string user_baudrates;
		for (int i = 0; i < brs.size(); i++){
			if (brs[i].is_added_by_user()){
				user_baudrates += brs[i].get_s();
				user_baudrates += "|";
			}
		}
		user_baudrates += get_cbo_item_data(_hBR)->get_s();
		comcfg->set_key("comm.config.baudrate", user_baudrates.c_str());

		// 当前 校验位, 数据位, 停止位
		comcfg->set_key("comm.config.parity", get_cbo_item_data(_hPA)->get_i());
		comcfg->set_key("comm.config.databit", get_cbo_item_data(_hDB)->get_i());
		comcfg->set_key("comm.config.stopbit", get_cbo_item_data(_hSB)->get_i());

		// 自动发送
		comcfg->set_key("comm.autosend.enable", !!::IsDlgButtonChecked(m_hWnd, IDC_CHK_AUTO_SEND));
		BOOL bTranslated;
		int interval = ::GetDlgItemInt(m_hWnd, IDC_EDIT_DELAY, &bTranslated, FALSE);
		if (!bTranslated){
			comcfg->set_key("comm.autosend.interval","");
		}
		else{
			comcfg->set_key("comm.autosend.interval", interval);
		}
	}

	void CComWnd::com_update_comport_list_and_select_current()
	{
		int index = ComboBox_GetCurSel(_hCP);
		c_comport* cp = index >= 0 ? (c_comport*)ComboBox_GetItemData(_hCP, index) : nullptr;
		int comidcur = (int)cp > 0xFFFF ? cp->get_i() : 0;
		com_update_comport_list();
		for (int i = 0; i < ComboBox_GetCount(_hCP); i++){
			c_comport* cp = (c_comport*)ComboBox_GetItemData(_hCP, i);
			if (cp->get_i() == comidcur){
				ComboBox_SetCurSel(_hCP, i);
				break;
			}
		}
		com_add_prompt_if_no_cp_presents();
	}

	int CComWnd::msgbox(UINT msgicon, char* caption, char* fmt, ...)
	{
		va_list va;
		char smsg[1024] = { 0 };
		va_start(va, fmt);
		_vsnprintf(smsg, sizeof(smsg), fmt, va);
		va_end(va);

		if (GetCurrentThreadId() == GetWindowThreadProcessId(m_hWnd, nullptr)){
			return ::MessageBox(m_hWnd, smsg, caption, msgicon);
		}
		else{
			auto msg = new std::string(smsg);
			auto cap = new std::string(caption?caption:"");
			auto ico = new int(msgicon);

			void** p = new void*[3];
			p[0] = msg;
			p[1] = cap;
			p[2] = ico;

			// 用SendMessage好不?
			PostMessage(kMessageBox, 0, LPARAM(p));
			return 0;
		}
	}

    LRESULT CComWnd::OnCommCommand() {
        while(Command* bpCmd = _comm.get_command()) {
            if(bpCmd->type == CommandType::kUpdateCounter) {
                //auto pCmd = static_cast<Command_UpdateCounter*>(bpCmd);
                int nRead, nWritten, nQueued;
                _comm.get_counter(&nRead, &nWritten, &nQueued);
                update_status("状态：接收计数:%u,发送计数:%u,等待发送:%u", nRead, nWritten, nQueued);
                delete bpCmd;
            }
        }

        return 0;
    }

	//////////////////////////////////////////////////////////////////////////
	LPCTSTR c_send_file_format_dlg::get_skin_xml() const
	{
		return 
			R"(
			<Window size="430,330">
				<Font name = "微软雅黑" size = "16" default = "true" />
				<Font name = "微软雅黑" size = "12"/>
				<Vertical>
					<Vertical inset = "5,5,5,5">
						<Container minheight="180" minwidth="180">
							<Group text="选择类型" />
							<Vertical inset="5,20,5,5">
								<Option name="text" text="纯文本文件"/>
								<Static text="即普通的纯ASCII码文件, 按原文内容显示在文本框中!" font="1" inset="20,0,0,0"/>
								<Option name="any" text="任意数据文件"/>
								<Static text="常说的二进制文件(直接打开有乱码), 将以16进制序列方式显示!" font="1" inset="20,0,0,0"/>
								<Option name="hexseq" text="包含16进制序列的文本文件"/>
								<Static text="两个字符一组的16进制序列文件, 比如: 12 AB FF" font="1" inset="20,0,0,0"/>
								<Option name="cmd" text="命令列表文件"/>
								<Static text="包含在文本文件中的命令列表索引!" font="1" inset="20,0,0,0"/>
								<Option name="nothing" text="取消" />
							</Vertical>
						</Container>
					</Vertical>
				</Vertical>
			</Window>
			)";
	}

	LRESULT c_send_file_format_dlg::handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
			CenterWindow();
			return 0;
		}

		return __super::handle_message(uMsg, wParam, lParam, bHandled);
	}

	LRESULT c_send_file_format_dlg::on_command_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code)
	{
		auto& name = ctrl->GetName();
		if (name == "text" || name == "any" || name == "hexseq" || name == "cmd" || name == "nothing"){
			if (code == BN_CLICKED){
				_selected = name;
				Close();
				return 0;
			}
		}
		return 0;
	}


	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	LPCTSTR c_send_data_format_dlg::get_skin_xml() const
	{
		return 
			_bchar ?
			R"feifei(
			<Window size="260,110">
				<Font name = "微软雅黑" size = "12" default = "true" />
				<Font name = "微软雅黑" size = "12"/>
				<Vertical>
					<Horizontal>
						<Container inset="5,5,5,5" height="110" width="130">
							<Group text="换行符类型"/>
							<Vertical inset="15,20,5,5">
								<Option name="nlt_crlf" text="回车换行(\r\n)" style="group"/>
								<Option name = "nlt_cr" text = "回车(\r)" />
								<Option name = "nlt_lf" text = "换行(\n)" />
								<Option name = "nlt_none" text = "忽略" />
							</Vertical>
						</Container>
						<Container inset="5,5,5,5" height="110" width="130">
							<Group text="转义字符"/>
							<Vertical inset="15,20,5,5">
								<Check name="escape_use" text="使用转义字符" />
							</Vertical>
						</Container>
					</Horizontal>
				</Vertical>
			</Window>
			)feifei"
			:
		R"feifei(
<Window size="300,100">
	<Font name = "微软雅黑" size = "12" default = "true" />
	<Font name = "黑体" size = "20"/>
	<Vertical>
		<Vertical>
			<Control />
			<Horizontal height="30">
				<Control />
				<Static text="当前没有可设置的属性" font="1" width="200"/>
				<Control />
			</Horizontal>
			<Control />
		</Vertical>
	</Vertical>
</Window>
		)feifei"
		;
	}

	LRESULT c_send_data_format_dlg::on_command_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code)
	{
		auto& name = ctrl->GetName();
		if (code == BN_CLICKED){
			if (name == "nlt_crlf"){
				*_dwAttr &= ~0x00000003;
				*_dwAttr |= CComWnd::SendDataFormatChar::sdfc_kCrlf;
			}
			else if (name == "nlt_cr"){
				*_dwAttr &= ~0x00000003;
				*_dwAttr |= CComWnd::SendDataFormatChar::sdfc_kCr;
			}
			else if (name == "nlt_lf"){
				*_dwAttr &= ~0x00000003;
				*_dwAttr |= CComWnd::SendDataFormatChar::sdfc_kLf;
			}
			else if (name == "nlt_none"){
				*_dwAttr &= ~0x00000003;
				*_dwAttr |= CComWnd::SendDataFormatChar::sdfc_kNoCrlf;
			}

			else if (name == "escape_use"){
				*_dwAttr &= ~0x00000004;
				*_dwAttr |= ::SendMessage(hwnd, BM_GETCHECK,0,0)==BST_CHECKED 
					? CComWnd::SendDataFormatChar::sdfc_kUseEscape : 0;
			}
		}
		return 0;
	}

	LRESULT c_send_data_format_dlg::handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
		{
			::SetWindowText(m_hWnd, _bchar ? "设置字符发送格式" : "设置十六进制发送格式");
			CenterWindow();

			if (_bchar){
				SdkLayout::CTinyString nlt;
				LPCTSTR nlt_names[] = { "nlt_none", "nlt_cr", "nlt_lf", "nlt_crlf" };
				SdkLayout::CControlUI* pNlt = _layout.FindControl(nlt_names[*_dwAttr & 0x00000003]);
				if (pNlt) ::SendMessage(*pNlt, BM_SETCHECK, BST_CHECKED, 0);

				::SendMessage(_layout.FindControl("escape_use")->GetHWND(), BM_SETCHECK,
					*_dwAttr & CComWnd::SendDataFormatChar::sdfc_kUseEscape ? BST_CHECKED : BST_UNCHECKED, 0);

			}
			return 0;
		}
		}
		return __super::handle_message(uMsg, wParam, lParam, bHandled);
	}

}

