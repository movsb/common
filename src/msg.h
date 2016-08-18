#pragma once

#include "DataProcessor.h"
#include "comm.h"

namespace Common {

    class CommandNotifier : public ICommandNotifier
    {
    public:
        virtual void OnCommand() {
            assert(::IsWindow(_wnd));
            ::PostMessage(_wnd, _msg, 0, 0);
        }

    public:
        void init(HWND wnd, UINT msg) {
            _wnd = wnd;
            _msg = msg;
        }

    private:
        HWND _wnd;
        UINT _msg;
    };

	// 发送文件格式选择对话框
	class c_send_file_format_dlg : public c_dialog_builder
	{
	public:
		c_send_file_format_dlg() {  }
		SdkLayout::CTinyString get_selected_type() { return _selected; }
	protected:
		virtual LRESULT		handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled) override;
		virtual LPCTSTR		get_skin_xml() const override;
		virtual LRESULT		on_command_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code) override;

	protected:
		SdkLayout::CTinyString _selected;
	};

	// 发送格式控制对话框
	class c_send_data_format_dlg : public c_dialog_builder
	{
	public:
		c_send_data_format_dlg(bool bchar, DWORD* attr)
			: _bchar(bchar)
			, _dwAttr(attr)
		{
		}

	protected:
		virtual LRESULT		handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
		virtual LPCTSTR		get_skin_xml() const override;
		virtual LRESULT		on_command_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code);
		virtual DWORD		get_window_style() const { return WS_OVERLAPPEDWINDOW; }
		virtual DWORD		get_window_ex_style() const override { return WS_EX_TOOLWINDOW; }
		virtual void		on_final_message(HWND hwnd) { __super::on_final_message(hwnd); delete this; }

	protected:
		bool _bchar;
		DWORD* _dwAttr;
	};

	class CComWnd 
		: public CWnd
		, public i_timer
		, public i_timer_period
		, public IAcceleratorTranslator
	{
		friend class c_send_data_format_dlg;
	public:
		CComWnd();
		~CComWnd();

    private:
        CommandNotifier _command_notifier;
        UINT            _command_message;

	private:
		enum PrivateMessage{
			__kPrivateStart = WM_APP,
			kUpdateTimer,
			kAutoSend,
		};

		// 发送方式只有两种: 16进制 + 文本字符, 由_b_send_data_frmat_hex决定; 调用函数来方便决定
		// 这里是其它选项, 比如: 回车类型, 是否处理控制字符等
		enum SendDataFormatHex{
			sdfh_kNone		= 0x00000000,
		};
		enum SendDataFormatChar{
			sdfc_kNone		= 0x00000000,
			// 01两位决定换行符类型
			sdfc_kNoCrlf	= 0x00000000,
			sdfc_kCr		= 0x00000001,
			sdfc_kLf		= 0x00000002,
			sdfc_kCrlf		= 0x00000003,
			// 第2位 决定是否使用转义字符
			sdfc_kUseEscape = 0x00000004,
		};

	protected:
        LRESULT OnCommCommand();
		// interface i_com_timer
		virtual void update_timer(int h, int m, int s) override;
		// interface 
		virtual void update_timer_period() override;

		void update_status(const char* fmt, ...);

		// IAcceleratorTranslator interface
		virtual bool TranslateAccelerator(MSG* pmsg);

	protected:
		LRESULT CALLBACK RichEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
		virtual void OnFinalMessage(HWND hWnd);

	private:
		LRESULT on_create(HWND hWnd, HINSTANCE hInstance);
		LRESULT on_scroll(UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT on_size(int width,int height);
		LRESULT on_close();
		LRESULT on_destroy();
		LRESULT on_command(HWND hWndCtrl, int id, int codeNotify);
		LRESULT on_device_change(WPARAM event, DEV_BROADCAST_HDR* pDBH);
		LRESULT on_setting_change(WPARAM wParam, LPCTSTR area);
		LRESULT on_app(UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT on_contextmenu(HWND hwnd, int x, int y);

	private:
		LRESULT on_command_menu(int id);
		LRESULT on_command_acce(int id);
		LRESULT on_command_ctrl(HWND hwnd, int id, int code);

	private:
		struct list_callback_ud{
			enum e_type{
				cp,br,pa,sb,db
			}type;
			CComWnd* that;
			HWND hwnd;
		};
		static void com_udpate_list_callback(void* ud, const t_com_item* t);
		void com_update_item_list();
		void com_update_comport_list();
		void com_update_comport_list_and_select_current();
		bool com_flush_settings_from_combobox();
		bool com_try_close(bool b_thread_started);
		bool com_try_open();
		void com_openclose();
		void com_lock_ui_panel(bool lock);
		void com_add_prompt_if_no_cp_presents();
		void com_update_open_btn_text();
		void com_copy_text_data_to_clipboard(HWND hwnd);
		void com_load_file();
		bool _com_load_file_prompt_size(SdkLayout::CTinyString& selected, c_binary_file& bf);
		bool com_do_send(bool callfromautosend);

		// 一些相关配置
		void init_from_config_file();
		void save_to_config_file();

		void switch_rich_edit_fullscreen(bool full);
		void switch_window_top_most(bool manual=false, bool topmost = true);
		void switch_simple_ui(bool manual=false, bool bsimple=false);
		void switch_send_data_format(bool manual=false, bool bhex=false, DWORD fmthex=0,DWORD fmtchar=0);
		void switch_recv_data_format(bool manual = false, bool bhex = false, DWORD fmthex = 0, DWORD fmtchar = 0);
		bool is_send_data_format_hex() { return _b_send_data_format_hex; }
		bool is_send_data_format_char(){ return !is_send_data_format_hex(); }
		bool is_recv_data_format_hex() { return _b_recv_data_format_hex; }
		bool is_recv_data_format_char(){ return !is_recv_data_format_hex(); }
		void switch_auto_send(bool manual=false, bool bauto=false, int interval=-1);
		

	public:
		Window::c_edit*			editor_send()		{return &_send_edit;}
		Window::c_edit*			editor_recv_hex()	{return &_recv_hex_edit;}
		Window::c_rich_edit*	editor_recv_char()	{return &_recv_char_edit;}

	private:
		HWND _hCP, _hBR, _hPA, _hSB, _hDB;
		HWND _hStatus, _hOpen;
		Window::c_rich_edit	_recv_char_edit;
		bool				_b_recv_char_edit_fullscreen;
		Window::c_edit		_recv_hex_edit;
		Window::c_edit		_send_edit;
		sdklayout*			m_layout;
		HACCEL				m_hAccel;

		CComm				_comm;
		c_timer				_timer;
		c_timer				_auto_send_timer;

		char				_send_buffer[10240];	//用于默认取发送数据框的数据
		Window::c_edit*		_recv_cur_edit;			// 当前接收数据格式文本控件
		bool				_b_send_data_format_hex;
		bool				_b_recv_data_format_hex;
		DWORD				_send_data_format_hex;
		DWORD				_send_data_format_char;

		c_hex_data_receiver		_hex_data_receiver;
		c_text_data_receiver	_text_data_receiver;
		c_file_data_receiver	_file_data_receiver;

		AThunk				_thunk_rich_edit;
		WNDPROC				_thunk_rich_edit_old_proc;

		c_observable		_window_close_handler;

        // 是否需要在关闭后刷新串口列表，因为串口打开时无法修改列表，所以~
        bool                _b_refresh_comport;
	};
}
