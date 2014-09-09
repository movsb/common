#ifndef __UICONTAINER_H__
#define __UICONTAINER_H__

#pragma once

namespace SdkLayout {
/////////////////////////////////////////////////////////////////////////////////////
//

class CContainerUI : public CControlUI
{
public:
    CContainerUI();
    virtual ~CContainerUI();

	virtual void DoInit();

#ifdef _DEBUG
	virtual bool SetFocus()
	{
		// Should I just remove the virtual modifier or put a 'final' on it?
		assert(0 && _T("Do NOT call SetFocus() from CContainerUI-based classes!"));
		return false;
	}
#endif

	virtual LPCTSTR GetClass() const {return GetClassStatic();}
	static LPCTSTR GetClassStatic() {return _T("Container");}

public:
    int GetCount() const;
    bool Add(CControlUI* pControl);
    bool AddAt(CControlUI* pControl, int iIndex);
    bool Remove(CControlUI* pControl);
    bool RemoveAt(int iIndex);
    void RemoveAll();
	CControlUI* GetAt(int iIndex) {return static_cast<CControlUI*>(m_items[iIndex]);}

    virtual void SetPos(const CDuiRect& rc);

	virtual void SetVisible(bool bVisible = true);
	virtual void SetDisplayed(bool bDisplayed);

	virtual void SetManager(CPaintManagerUI* mgr);
	virtual CControlUI* FindControl(LPCTSTR name);
	virtual CControlUI* FindControl(HWND hwnd);
	virtual void SetFont(int id);

protected:
    CStdPtrArray m_items;
	bool m_bEnableUpdate;	//防止在SetVisibleByParent时子控件重复调用NeedParentUpdate()
};

} // namespace SdkLayout

#endif // __UICONTAINER_H__
