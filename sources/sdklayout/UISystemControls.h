#pragma once

namespace SdkLayout{
	class CSystemControlUI : public CControlUI
	{
	public:
		CSystemControlUI();
		virtual void SetManager(CPaintManagerUI* mgr){assert(0 && "Plz override!");}
		virtual void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);
		virtual void Create(LPCTSTR lpClass);
	protected:
		DWORD		m_dwStyle;
		DWORD		m_dwExStyle;
		std::string m_strText;
	};

	class CButtonUI : public CSystemControlUI
	{
	public:
		virtual void SetManager(CPaintManagerUI* mgr);
	};

	class COptionUI : public CSystemControlUI
	{
	public:
		COptionUI();
		virtual void SetManager(CPaintManagerUI* mgr);
		virtual void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);

	protected:
		bool m_bHasWsGroup;
	};

	class CCheckUI : public CSystemControlUI
	{
	public:
		CCheckUI();
		virtual void SetManager(CPaintManagerUI* mgr);
		virtual void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);

	protected:
		bool m_bCheck;
	};

	class CStaticUI : public CSystemControlUI
	{
	public:
		virtual void SetManager(CPaintManagerUI* mgr);
		virtual void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);
	};

	// Group 和其它标准控件一样使用, 不要当作控件的容器!
	class CGroupUI : public CSystemControlUI
	{
	public:
		CGroupUI();
		virtual void SetManager(CPaintManagerUI* mgr);
	};
}
