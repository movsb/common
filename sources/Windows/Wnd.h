#pragma once

namespace Common {

class i_notifier
{
public:
	virtual int msgbox(UINT msgicon, char* caption, char* fmt, ...) = 0;
	virtual void msgerr(char* prefix = 0) = 0;
};

class  CWnd : public IMessageFilter, public i_notifier
{
public:
    CWnd();

    HWND GetHWND() const;
    operator HWND() const;

    HWND Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int cx = CW_USEDEFAULT, int cy = CW_USEDEFAULT, HMENU hMenu = NULL);
	HWND Create(HWND hParent, LPCTSTR lpTemplate);
	bool Attach(HWND hWnd);

    void ShowWindow(bool bShow = true, bool bTakeFocus = true);
    UINT ShowModal();
    void Close(UINT nRet = IDOK);
    void CenterWindow();
    LRESULT SendMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0L);
    LRESULT PostMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0L);
	void ResizeClient(int cx = -1, int cy = -1);
	//IMessageFilter
	virtual bool FilterMessage(HWND hChild,UINT uMsg, WPARAM wParam, LPARAM lParam);

	//IAcceleratorTranslator
	virtual bool TranslateAccelerator(MSG* pmsg) {return false;}

	//i_notifier
	virtual int msgbox(UINT msgicon, char* caption, char* fmt, ...);
	virtual void msgerr(char* prefix = 0);

protected:
	bool RegisterWindowClass();
	bool RegisterSuperclass();
	HWND Subclass(HWND hWnd);
	void Unsubclass();

	virtual LPCTSTR GetWindowClassName() const;
    virtual LPCTSTR GetSuperClassName() const;
    virtual UINT GetClassStyle() const;
	virtual HBRUSH GetClassBrush() const;

    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
	virtual void OnFirstMessage(HWND hWnd);
    virtual void OnFinalMessage(HWND hWnd);
	virtual bool ResponseDefaultKeyEvent(HWND hChild, WPARAM wParam);

    static LRESULT CALLBACK __WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK __ControlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK __DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	bool m_bHasMgr;
	bool m_bIsDialog;
    HWND m_hWnd;
    WNDPROC m_OldWndProc;
    bool m_bSubclassed;
	CWindowManager m_wndmgr;
};

}
