#ifndef __SDK_LAYOUT_H__
#define __SDK_LAYOUT_H__

#pragma once

#if defined(__cplusplus)

#include <tchar.h>

#define lengthof(x) (sizeof(x)/sizeof(*x))
#define MAX max
#define MIN min
#define CLAMP(x,a,b) (MIN(b,MAX(a,x)))

#include "uiUtils.h"
#include "UIControl.h"
#include "UIContainer.h"
#include "UIManager.h"
#include "UIMarkup.h"
#include "UIDlgBuilder.h"
#include "UIVerticalLayout.h"
#include "UIHorizontalLayout.h"
#include "UISystemControls.h"

namespace SdkLayout{

	class IDialogBuilder_GetID
	{
	public:
		virtual UINT get_ctrl_id(LPCTSTR name) const = 0;
	};

class CSdkLayout
{
public:
	CSdkLayout(){ m_hWnd = NULL; m_pRoot = NULL; m_getid = NULL;}
	~CSdkLayout(){DeleteLayout();}
	HWND GetHWND() const {return m_hWnd;}
	CContainerUI* GetRoot() const {return m_pRoot;}
	const SIZE& GetPostSize() const
	{
		assert(m_pRoot);
		assert(m_pRoot->GetAt(0));
		return m_pRoot->GetAt(0)->GetPostSize();
	}
	void ProcessScrollMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	CPaintManagerUI* GetManager() {return &m_Manager;}
	bool SetLayout(HWND hWnd, LPCTSTR xml, HINSTANCE hInst=NULL);
	bool SetLayout(HWND hWnd, UINT id, HINSTANCE hInst=NULL);
	void SetDlgGetID(IDialogBuilder_GetID* pgetid){ m_getid = pgetid; }
	void DeleteLayout();
	void ResizeLayout(const RECT& rc);
	void ResizeLayout();
	CControlUI* FindControl(LPCTSTR name);
	CControlUI* FindControl(HWND hwnd);
	void SetDefFont(LPCTSTR face, int sz);

private:
	void _InitializeLayout();
	void _ProcessScrollBar(const CDuiRect& rc);

private:
	HWND m_hWnd;
	CContainerUI* m_pRoot;
	CPaintManagerUI m_Manager;
	CDuiRect m_rcLast;
	IDialogBuilder_GetID* m_getid;
};

}

#endif

#if defined(__cplusplus)
typedef SdkLayout::CSdkLayout	sdklayout;
typedef SdkLayout::CControlUI	sdkcontrol;
#else
typedef struct sdklayout		sdklayout;
typedef struct sdkcontrol		sdkcontrol;
#endif

#ifdef __cplusplus
EXTERN_C {
#endif

sdklayout*		layout_new(HWND hWnd, LPCTSTR xml, HINSTANCE hInst);
void			layout_delete(sdklayout* layout);
sdkcontrol*		layout_root(sdklayout* layout);
void			layout_post_size(sdklayout* layout, int* x, int* y);
void			layout_scroll(sdklayout* layout, UINT uMsg, WPARAM wParam, LPARAM lParam);
void			layout_resize(sdklayout* layout, SIZE* sz);
sdkcontrol*		layout_control(sdklayout* layout, LPCTSTR name);
void			layout_visible(sdkcontrol* ctrl, BOOL bVisible);
void			layout_deffont(sdklayout* layout, const char* face, int sz);
int				layout_newfont(sdklayout* layout, const char* face, int sz);
void			layout_setfont(sdkcontrol* ctrl, int id);

#ifdef __cplusplus
}
#endif

#endif // !__SDK_LAYOUT_H__
