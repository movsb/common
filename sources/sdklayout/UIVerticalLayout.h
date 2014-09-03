#ifndef __UIVERTICALLAYOUT_H__
#define __UIVERTICALLAYOUT_H__

#pragma once

namespace SdkLayout
{
	class CVerticalLayoutUI : public CContainerUI
	{
	public:
		virtual LPCTSTR GetClass() const {return GetClassStatic();}
		static LPCTSTR GetClassStatic() {return _T("Vertical");}

		virtual void SetPos(const CDuiRect& rc);
	};
}
#endif // __UIVERTICALLAYOUT_H__
