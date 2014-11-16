#include "stdafx.h"

#include <thread>

#include "libtccw32/src/libtcc.h"
#include "cscripting.h"

namespace Common{

	c_cs_console* __this_cs_console;

	extern "C" static void errfunc(void* opaque, const char* msg){
		if (msg && strncmp(msg, "tcc: ", 5) == 0){ // :-) !!!!!!!!!!!!!!!
			msg += 5; 
		}
		MessageBox(__this_cs_console->GetHWND(), msg, nullptr, MB_ICONEXCLAMATION);
	}

	extern "C" static int my_printf(const char* fmt, ...){
		char buf[4096];
		int len;
		buf[0] = '\0';
		
		va_list va;
		va_start(va, fmt);
		len = vsnprintf(buf, sizeof(buf), fmt, va);
		va_end(va);

		char* str = c_text_formatting::hex2chs((unsigned char*)buf, len, 0, 0, c_text_formatting::newline_type::NLT_CRLF);
		if (str){
			__this_cs_console->append_text(str); // TODO: cross thread !!!
			memory.free((void**)&str,"");
		}

		return len;
	}

	class c_libtcc
	{
	public:
		c_libtcc()
			//: _tcc(tcc_new())
		{
		}

		~c_libtcc(){
			//tcc_delete(_tcc);
		}

		bool exec(const std::string& cs)
		{
			typedef int fmain();
			fmain* pmain = nullptr;

			TCCState* _tcc = tcc_new();

			tcc_set_error_func(_tcc, nullptr, errfunc);
			tcc_set_output_type(_tcc, TCC_OUTPUT_MEMORY);

			try{
				if (tcc_compile_string(_tcc, cs.c_str()) == -1)
					throw std::runtime_error("C语言脚本语法不正确!");

				tcc_add_symbol(_tcc, "printf", my_printf);

				if (tcc_relocate(_tcc, TCC_RELOCATE_AUTO) < 0)
					throw std::runtime_error("错误: 内部重定位错误!");

				pmain = reinterpret_cast<fmain*>(tcc_get_symbol(_tcc, "main"));
				if (!pmain)
					throw std::runtime_error("错误: 没有 `main' 函数!");
			}
			catch (...){
				tcc_delete(_tcc);
				throw;
			}

			// FIXME: delete thread
			// FIXME: delete tcc
			(new std::thread([&](){
				pmain();
			}));

			//tcc_delete(_tcc);
			return true;
		}

	private:
	};


	void c_cs_console::on_final_message(HWND hwnd)
	{
		__super::on_final_message(hwnd);
		delete this;
		__this_cs_console = nullptr;
	}

	LRESULT c_cs_console::on_notify_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code, NMHDR* hdr)
	{
		return 0;
	}

	LRESULT c_cs_console::on_command_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code)
	{
		if (ctrl->GetName() == "btn_exec"){
			if (code == BN_CLICKED){
				auto ctrl_script = static_cast<SdkLayout::CEditUI*>(_layout.FindControl("script_edit"))->GetHWND();
				int len = ::GetWindowTextLength(ctrl_script);
				if (len > 0){
					char* buf = new char[len + 1];
					GetWindowText(ctrl_script, buf, len + 1);
					std::string s(buf);
					delete[] buf;

					c_libtcc tcc;
					try{
						tcc.exec(s);
					}
					catch (std::runtime_error& e){
						msgbox(MB_ICONERROR, NULL, (char*)e.what());
					}
				}
				
			}
		}
		else if (ctrl->GetName() == "btn_hide_output"){
			if (code == BN_CLICKED){
				_show_output = !_show_output;

				auto of = static_cast<SdkLayout::CContainerUI*>(_layout.FindControl("output_group"));
				if (_show_output){
					::SetWindowText(ctrl->GetHWND(), "隐藏\n输出");
					of->SetVisible();
				}
				else{
					::SetWindowText(ctrl->GetHWND(), "显示\n输出");
					of->SetVisible(false);
				}
				return 0;
			}
		}
		else if (ctrl->GetName() == "btn_empty_output"){
			if (code == BN_CLICKED){
				::SetWindowText(_layout.FindControl("output_edit")->GetHWND(), "");
				return 0;
			}
		}
		return 0;
	}

	LPCTSTR c_cs_console::get_skin_xml() const
	{
		return
R"feifei(
<Window size="350,350">
	<Font name = "微软雅黑" size = "12" default = "true" />
	<Font name = "Consolas" size="12" />
	<Vertical>
		<Vertical name="main" inset="5,0,5,5" height="0">
			<Vertical>
				<Horizontal name="script" height="20">
					<Static text="脚本: " />
				</Horizontal>
				<Horizontal minheight="104">
					<Edit name="script_edit" style="multiline,nohidesel,vscroll,hscroll" exstyle="clientedge" font="1"/>
					<Control width="5" />
					<Vertical name="script_btns" width="50">
						<Button name="btn_exec" text="执行" height="30"/>
						<Button name="btn_stop" text="停止" height="30"/>
						<Control />
						<Button name="btn_hide_output" style="multiline" text=")feifei" "隐藏\n输出" R"feifei(" height="44" />
					</Vertical>
				</Horizontal>

				<Vertical name="output_group">
					<Control height="3" />
					<Horizontal height="20">
						<Static text="输出: " />
					</Horizontal>
					<Horizontal>
						<Edit name="output_edit" style="multiline,nohidesel,readonly,vscroll,hscroll" exstyle="clientedge" font="1"/>
						<Control width="5" />
						<Vertical name="output_btns" width="50">
							<Button name="btn_empty_output" text="清空" height="30"/>
						</Vertical>
					</Horizontal>
				</Vertical>
			</Vertical>
		</Vertical>
	</Vertical>
</Window>
)feifei";

	}

	LRESULT c_cs_console::handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
			__this_cs_console = this;
			CenterWindow();
			ShowWindow();
			return 0;
		}
		return __super::handle_message(uMsg, wParam, lParam, bHandled);
	}

	void c_cs_console::append_text(const char* s)
	{
		HWND houtput = static_cast<SdkLayout::CEditUI*>(_layout.FindControl("output_edit"))->GetHWND();
		int len = GetWindowTextLength(houtput);
		::SendMessage(houtput, EM_SETSEL, len, len);
		::SendMessage(houtput, EM_REPLACESEL, 0, LPARAM(s));
	}

}
