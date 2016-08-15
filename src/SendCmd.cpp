#include "stdafx.h"
#include "../res/resource.h"

#include "SendCmd.h"
#include "comm.h"

namespace Common{
	bool sendcmd_try_load_xml(HWND hOwner, LPCTSTR xml_file, CComm* comm)
	{
		auto doc = new tinyxml2::XMLDocument;
		try{
			if (doc->LoadFile(xml_file) != tinyxml2::XMLError::XML_NO_ERROR){
				throw "不是正确的xml格式文件!";
			}

			auto node_common = doc->FirstChildElement("common");
			if (node_common == nullptr){
				throw R"(没有找到"common"节点, 不是common配置文件!)";
			}
			else{
				auto type = node_common->Attribute("type");
				if (!type || strcmp(type, "commands") != 0){
					throw "是common配置文件, 但不是common命令文件!";
				}
			}

			auto dlg = new c_send_cmd_dialog(comm, doc, xml_file);
			return dlg->do_modeless(hOwner);

		}
		catch (const char* err){
			delete doc;
			::MessageBox(hOwner, err, nullptr, MB_ICONEXCLAMATION);
			return false;
		}

		return true;
	}

	void c_send_cmd_item::response_key_event(WPARAM vk)
	{

	}

	LRESULT c_send_cmd_item::handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
		{
			return TRUE;
		}
		}
		return __super::handle_message(uMsg, wParam, lParam, bHandled);
	}

	LRESULT c_send_cmd_item::on_command_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code)
	{
		auto& name = ctrl->GetName();
		if (name == "expand"){
			if (code == BN_CLICKED){
				do_expand();
				return 0;
			}
		}
		else if (name == "send"){
			if (code == BN_CLICKED){
				nm_send hdr;
				hdr.hwndFrom = *this;
				hdr.idFrom = 0;
				hdr.code = (UINT)scimsg::send;
				hdr.hEdit = *_layout.FindControl("script");
				hdr.bhex = ::SendMessage(*_layout.FindControl("type_hex"), BM_GETCHECK, 0, 0)==BST_CHECKED;
				hdr.buseescape = ::SendMessage(*_layout.FindControl("use_escape"), BM_GETCHECK, 0, 0) == BST_CHECKED;
				::SendMessage(::GetParent(*this), WM_NOTIFY, 0, LPARAM(&hdr));
				return 0;
			}
		}
		else if (name == "delete"){
			if (code == BN_CLICKED){
				NMHDR hdr;
				hdr.hwndFrom = *this;
				hdr.idFrom = 0;
				hdr.code = (UINT)scimsg::del;
				::SendMessage(::GetParent(*this), WM_NOTIFY, 0, LPARAM(&hdr));
				return 0;
			}
		}
		return 0;
	}

	LPCTSTR c_send_cmd_item::get_skin_xml() const
	{
		return 
			R"feifei(
<Window size="0,0">
	<Font name = "微软雅黑" size = "12" default = "true" />
	<Font name = "微软雅黑" size = "16" />
	<Vertical>
		<Vertical inset="3,3,3,3">
			<Horizontal height="24">
				<Vertical minwidth="100">
					<Control />
					<Edit name="comment" text="" height="20" font="1"/>
					<Control />
				</Vertical>
				<Control width="5" />
				<Button name="expand" text="+" width="24"/>
				<Control width="5" />
				<Button name="send" text="发送" width="50"/>
			</Horizontal>
			<Horizontal name="misc_wnd" height="60" visible="false">
				<Container minwidth="220">
					<Group/>
					<Vertical inset="5,15,5,5">
						<Horizontal height="20">
							<Option name="type_hex" text="十六进制" style="group" width="70"/>
							<Option name = "type_char" text = "字符" width="50"/>
							<Check name="use_escape" text="使用转义字符" />
						</Horizontal>
						<Edit name="script" exstyle="clientedge"/>
					</Vertical>
				</Container>
				<Vertical width="55" inset="5,5,5,5">
					<Button name="delete" text="删除" height="25"/>
				</Vertical>
			</Horizontal>
		</Vertical>
	</Vertical>
</Window>
)feifei";
	}

	c_send_cmd_item::c_send_cmd_item()
		: _b_expanded(false)
		, _cmd(nullptr)
	{

	}

	c_send_cmd_item::~c_send_cmd_item()
	{

	}

	DWORD c_send_cmd_item::get_window_style() const
	{
		return WS_CHILD;
	}

	int c_send_cmd_item::get_height() const
	{
		return _layout.GetPostSize().cy;
	}

	void c_send_cmd_item::do_expand()
	{
		auto ctrl = _layout.FindControl("expand");
		auto misc = _layout.FindControl("misc_wnd");
		HWND hExpand = *ctrl;

		_b_expanded = !_b_expanded;
		::SetWindowText(hExpand, _b_expanded ? "-" : "+");
		misc->SetVisible(_b_expanded);
		::SendMessage(::GetParent(m_hWnd), WM_COMMAND, MAKEWPARAM(0, scimsg::item_expand), LPARAM(m_hWnd));
	}

	void c_send_cmd_item::set_name(const char* name)
	{
		auto ctrl = _layout.FindControl("comment")->GetHWND();
		::SetWindowText(ctrl, name);
	}

	void c_send_cmd_item::set_script(const char* script)
	{
		auto ctrl = _layout.FindControl("script")->GetHWND();
		::SetWindowText(ctrl, script);
	}

	void c_send_cmd_item::set_format(bool bhex, bool useescape)
	{
		auto thex = _layout.FindControl("type_hex")->GetHWND();
		auto tchar = _layout.FindControl("type_char")->GetHWND();
		auto escape = _layout.FindControl("use_escape")->GetHWND();

		::SendMessage(bhex ? thex : tchar, BM_SETCHECK, BST_CHECKED, 0);
		::SendMessage(escape, BM_SETCHECK, useescape ? BST_CHECKED : BST_UNCHECKED, 0);
	}

	void c_send_cmd_item::collapse(bool bcollapse)
	{
		if (bcollapse){
			if (_b_expanded){
				do_expand();
			}
		}
		else{
			if (!_b_expanded){
				do_expand();
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	void c_send_cmd_dialog::response_key_event(WPARAM vk)
	{

	}

	LRESULT c_send_cmd_dialog::handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
		{
			::SetMenu(m_hWnd, ::LoadMenu(theApp, MAKEINTRESOURCE(IDR_MENU_SENDCMD)));
			_init_cmds_from_doc();
			return TRUE;
		}
		case WM_CLOSE:
		{
			_xml->SaveFile(_file.c_str());
			break;
		}
		}
		return __super::handle_message(uMsg, wParam, lParam, bHandled);
	}

	LRESULT c_send_cmd_dialog::on_command_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code)
	{
		if (_tcscmp(ctrl->GetClass(), c_send_cmd_item_ui::GetClassStatic()) == 0){
			auto item = static_cast<c_send_cmd_item*>(ctrl->GetUserData());
			if(code == c_send_cmd_item::scimsg::item_expand){
				ctrl->SetFixedHeight(item->get_height());
				ctrl->NeedParentUpdate();
				_layout.ResizeLayout(); // only need updating the scroll bar
				return 0;
			}
		}
		return 0;
	}

	LPCTSTR c_send_cmd_dialog::get_skin_xml() const
	{
		return
			R"feifei(
<Window size="350,350">
	<Font name = "微软雅黑" size = "12" default = "true" />
	<Vertical>
		<Vertical name="main" inset="5,0,5,4" height="0">
		</Vertical>
	</Vertical>
</Window>
)feifei";
	}

	c_send_cmd_dialog::c_send_cmd_dialog(CComm* comm, tinyxml2::XMLDocument* pdoc, const char* fn)
		: _comm(comm)
		, _xml(pdoc)
		, _file(fn)
	{

	}

	c_send_cmd_dialog::~c_send_cmd_dialog()
	{

	}

	void c_send_cmd_dialog::_init_cmds_from_doc()
	{
		if (!_xml) return;

		auto commands = _xml->FirstChildElement("common")->FirstChildElement("commands");
		if (!commands) return;

		for (auto cmd = commands->FirstChildElement("command");
			cmd != nullptr;
			cmd = cmd->NextSiblingElement("command"))
		{
			_insert_new_cmd_to_ui(cmd);
		}
	}

	void c_send_cmd_dialog::_insert_new_cmd_to_ui(const tinyxml2::XMLElement* cmd, bool bexpand)
	{
		auto type = cmd->Attribute("type");
		if (!type) return;

		auto item = new c_send_cmd_item;
		item->do_modeless(*this);
		if (bexpand) item->collapse(false);
		item->set_cmd(cmd);

		auto ctrl = new c_send_cmd_item_ui;
		ctrl->SetHWND(*item);
		ctrl->SetFixedHeight(item->get_height());
		ctrl->SetUserData(item);

		static_cast<SdkLayout::CContainerUI*>(_layout.FindControl("main"))->Add(ctrl);

		auto handle = tinyxml2::XMLConstHandle(cmd);
		if (auto k = handle.FirstChildElement("name").FirstChild().ToText()){
			item->set_name(k->Value());
		}
		if (auto k = handle.FirstChildElement("script").FirstChild().ToText()){
			item->set_script(k->Value());
		}

		bool useescape = false;
		auto aescape = cmd->Attribute("escape");
		if (aescape) useescape = strcmp(aescape, "true") == 0;

		bool bhex = false;
		auto ahex = cmd->Attribute("type");
		if (ahex) bhex = strcmp(ahex, "hex") == 0;

		item->set_format(bhex, useescape);
	}

	LRESULT c_send_cmd_dialog::on_notify_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code, NMHDR* hdr)
	{
		if (_tcscmp(ctrl->GetClass(), c_send_cmd_item_ui::GetClassStatic()) == 0){
			auto item = static_cast<c_send_cmd_item*>(ctrl->GetUserData());
			if (code == c_send_cmd_item::scimsg::send){
				if (!_comm->is_opened()){
					msgbox(MB_ICONEXCLAMATION, nullptr, "串口未打开!");
					return 0;
				}
				auto nmsend = static_cast<c_send_cmd_item::nm_send*>(hdr);
				char _send_buffer[1024];
				
				int len = ::GetWindowTextLength(nmsend->hEdit);
				if (len <= 0) return 0;

				char* text = NULL;
				if (len + 1 > sizeof(_send_buffer))
					text = new char[len + 1];
				else
					text = _send_buffer;

				*text = '\0';
				::GetWindowText(nmsend->hEdit, text, len + 1);

				if (nmsend->bhex == false){
					if (nmsend->buseescape){
						unsigned int n = c_text_formatting::parse_string_escape_char(text);
						len = n & 0x7FFFFFFF;
						if ((n & 0x80000000) == 0){
							msgbox(MB_ICONEXCLAMATION, NULL,
								"解析转义字符串时遇到错误!\n\n"
								"在第 %d 个字符附近出现语法解析错误!",
								len
								);
							return 0;
						}
					}
				}
				else{
					unsigned int n = c_text_formatting::str2hex(text, (unsigned char**)&text, len);
					len = n & 0x7FFFFFFF;
					if ((n & 0x80000000) == 0){
						msgbox(MB_ICONEXCLAMATION, NULL, "发送区的数据解析错误, 请检查!\n\n是不是选错了发送数据的格式\?\n\n"
							"在第 %d 个字符附近出现语法解析错误!", len);
						return false;
					}
				}

				c_send_data_packet* packet = _comm->alloc_packet(len);
				::memcpy(&packet->data[0], text, len);

				if (text != _send_buffer)
					delete[] text;

				return _comm->put_packet(packet);
			}
			else if (code == c_send_cmd_item::scimsg::del){
				ctrl->GetParent()->Remove(ctrl);
				auto commands = _xml->FirstChildElement("common")->FirstChildElement("commands");
				commands->DeleteChild(const_cast<tinyxml2::XMLElement*>(item->get_cmd()));
				item->Close();
			}
		}
		return 0;
	}

	LRESULT c_send_cmd_dialog::on_menu(int id)
	{
		switch (id)
		{
		case MENU_SENDCMD_COLLAPSEALL:
		{
			auto client = _layout.FindControl("main")->ToContainerUI();
			for (int i = 0; i < client->GetCount(); i++){
				auto c = client->GetAt(i);
				auto item = static_cast<c_send_cmd_item*>(c->GetUserData());
				item->collapse();
			}
			return 0;
		}
		case MENU_SENDCMD_NEWCMD:
		{
			auto commands = _xml->FirstChildElement("common")->FirstChildElement("commands");
			auto command = _xml->NewElement("command");
			command->SetAttribute("type", "hex");
			command->SetAttribute("escape", "false");
			auto name = _xml->NewElement("name");
			auto namet = _xml->NewText("");
			name->LinkEndChild(namet);
			auto script = _xml->NewElement("script");
			auto scriptt = _xml->NewText("");
			script->LinkEndChild(scriptt);
			command->LinkEndChild(name);
			command->LinkEndChild(script);
			commands->LinkEndChild(command);

			_insert_new_cmd_to_ui(command, true);

			SendMessage(WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
			_layout.ResizeLayout();
			SendMessage(WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
			return 0;
		}
		}
		return 0;
	}



}
