#include "stdafx.h"

namespace Common{
	bool sendcmd_try_load_xml(HWND hOwner, LPCTSTR xml_file)
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

			auto dlg = new c_send_cmd_dialog(doc);
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
		return 0;
	}

	LPCTSTR c_send_cmd_item::get_skin_xml() const
	{
		return 
			R"feifei(
<Window size="0,0">
	<Font name = "微软雅黑" size = "12" default = "true" />
	<Vertical>
		<Vertical inset="5,0,5,4">
			<Horizontal height="24">
				<Vertical minwidth="100">
					<Control />
					<Static name="comment" text="&lt;描述&gt;" height="20"/>
					<Control />
				</Vertical>
				<Control width="5" />
				<Button name="expand" text="+" width="24"/>
				<Control width="5" />
				<Button name="send" text="发送" width="50"/>
			</Horizontal>
			<Horizontal name="misc_wnd" height="110" visible="false">
				<Container minwidth="210">
					<Group/>
					<Vertical inset="5,15,5,5">
						<Horizontal height="40">
							<Vertical width="100">
								<Option name="type_hex" text="十六进制" group="true"/>
								<Option name = "type_char" text = "字符" />
							</Vertical>
							<Vertical width="100">
								<Check name="use_escape" text="使用转义字符" />
							</Vertical>
						</Horizontal>
						<Edit name="script"/>
					</Vertical>
				</Container>
				<Control width="55" />
			</Horizontal>
		</Vertical>
	</Vertical>
</Window>
)feifei";
	}

	c_send_cmd_item::c_send_cmd_item()
		: _b_expanded(false)
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
			_init_cmds_from_doc();
			return TRUE;
		}
		}
		return __super::handle_message(uMsg, wParam, lParam, bHandled);
	}

	LRESULT c_send_cmd_dialog::on_command_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code)
	{
		if (_tcscmp(ctrl->GetClass(), c_send_cmd_item_ui::GetClassStatic()) == 0){
			auto item = static_cast<c_send_cmd_item*>(ctrl->GetUserData());
			ctrl->SetFixedHeight(item->get_height());
			ctrl->NeedParentUpdate();
			_layout.ResizeLayout(); // only need updating the scroll bar
			return 0;
		}
		return 0;
	}

	LPCTSTR c_send_cmd_dialog::get_skin_xml() const
	{
		return
			R"feifei(
<Window size="300,100">
	<Font name = "微软雅黑" size = "12" default = "true" />
	<Vertical>
		<Vertical name="main" inset="5,0,5,4" height="0">
		</Vertical>
	</Vertical>
</Window>
)feifei";
	}

	c_send_cmd_dialog::c_send_cmd_dialog(tinyxml2::XMLDocument* pdoc)
		: _xml(pdoc)
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

	void c_send_cmd_dialog::_insert_new_cmd_to_ui(const tinyxml2::XMLElement* cmd)
	{
		auto type = cmd->Attribute("type");
		if (!type) return;

		auto item = new c_send_cmd_item;
		item->do_modeless(*this);

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
	}



}
