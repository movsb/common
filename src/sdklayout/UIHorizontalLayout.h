#ifndef __UIHORIZONTALLAYOUT_H__
#define __UIHORIZONTALLAYOUT_H__

#pragma once

namespace SdkLayout
{
	class  CHorizontalLayoutUI : public CContainerUI
	{
	public:
		virtual LPCTSTR GetClass() const {return GetClassStatic();}
		static LPCTSTR GetClassStatic() {return _T("Horizontal");}

		virtual void SetPos(const CDuiRect& rc);
	};
}
#endif // __UIHORIZONTALLAYOUT_H__
