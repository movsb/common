#pragma once

namespace Common{
	class c_dialog_builder : public CWnd, public SdkLayout::IDialogBuilder_GetID
	{
	public:
		void show(HWND hParent);
		virtual LPCTSTR GetWindowClassName() const { return "c_dialog_builder"; };
		virtual UINT GetClassStyle() const { return __super::GetClassStyle() & ~CS_DBLCLKS; }
		virtual UINT GetDialogStyle() const { return WS_POPUPWINDOW | WS_SIZEBOX | WS_VISIBLE; }
		virtual HBRUSH GetClassBrush() const override { return (HBRUSH)(COLOR_WINDOW); }
		virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
		virtual LPCTSTR get_skin_xml() const;

	protected:
		SdkLayout::CSdkLayout _layout;
	};
}
