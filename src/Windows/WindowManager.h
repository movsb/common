#pragma once

namespace Common {

	class IMessageFilter
	{
	public:
		virtual bool FilterMessage(HWND hChild, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	};

	class IAcceleratorTranslator
	{
	public:
		virtual bool TranslateAccelerator(MSG* pmsg) = 0;
	};

	class CWindowManager
	{
	public:
		CWindowManager();
		virtual ~CWindowManager();

		void Init(HWND hWnd, IMessageFilter* flt);
		void DeInit();
		HWND& hWnd() {return m_hWnd;}

		bool FilterMessage(MSG* pmsg);
		IMessageFilter*& MessageFilter(){return m_pMsgFilter;}

		bool TranslateAccelerator(MSG* pmsg);
		IAcceleratorTranslator*& AcceleratorTranslator() { return m_pAcceTrans; }

		static void MessageLoop();
		static bool TranslateMessage(MSG* pmsg);
		bool AddWindowManager(CWindowManager* pwm);
		bool RemoveWindowManager(CWindowManager* pwm);


	protected:
		HWND m_hWnd;
		IMessageFilter* m_pMsgFilter;
		IAcceleratorTranslator* m_pAcceTrans;
		static c_ptr_array<CWindowManager> m_aWndMgrs;
	};
}
