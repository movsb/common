#pragma once

namespace SdkLayout{
	struct style_map
	{
		DWORD dwStyle;
		const char* strStyle;
	};

	bool map_style(DWORD* dwStyle, style_map* known_styles, std::vector<std::string>& styles);

	class CSystemControlUI : public CControlUI
	{
	public:
		CSystemControlUI();
		virtual void SetManager(CPaintManagerUI* mgr);
		virtual void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);
		virtual void SetStyle(std::vector<std::string>& styles, bool bex = false);
		virtual void Create(LPCTSTR lpClass);
		virtual LPCTSTR GetWindowClass() const = 0;
	protected:


		DWORD		m_dwStyle;
		DWORD		m_dwExStyle;
		std::string m_strText;
	};

	class CButtonUI : public CSystemControlUI
	{
	public:
		virtual LPCTSTR GetWindowClass() const override { return WC_BUTTON; }
		virtual void SetStyle(std::vector<std::string>& styles, bool bex = false);

	};

	class COptionUI : public CSystemControlUI
	{
	public:
		COptionUI();
		virtual LPCTSTR GetWindowClass() const override { return WC_BUTTON; }
		virtual void SetStyle(std::vector<std::string>& styles, bool bex = false);
	protected:
		bool m_bHasWsGroup;
	};

	class CCheckUI : public CSystemControlUI
	{
	public:
		CCheckUI();
		virtual void SetManager(CPaintManagerUI* mgr);
		virtual LPCTSTR GetWindowClass() const override { return WC_BUTTON; }
		virtual void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);

	protected:
		bool m_bCheck;
	};

	class CStaticUI : public CSystemControlUI
	{
	public:
		virtual LPCTSTR GetWindowClass() const override { return WC_STATIC; }
	};

	// Group 和其它标准控件一样使用, 不要当作控件的容器!
	class CGroupUI : public CSystemControlUI
	{
	public:
		CGroupUI();
		virtual LPCTSTR GetWindowClass() const override { return WC_BUTTON; }
	};

	class CEditUI : public CSystemControlUI
	{
	public:
		CEditUI();
		virtual LPCTSTR GetWindowClass() const override { return WC_EDIT; }
		virtual void SetStyle(std::vector<std::string>& styles, bool bex = false);
	};
}
