#ifndef __UIDLGBUILDER_H__
#define __UIDLGBUILDER_H__

#pragma once

namespace SdkLayout {

	class IDialogBuilder_GetID;

class CDialogBuilder
{
public:
    CContainerUI* Create(LPCTSTR xml, CPaintManagerUI* manager, HINSTANCE hInst = NULL, IDialogBuilder_GetID* pgetid=0);

private:
    CControlUI* _Parse(CMarkupNode* parent, CContainerUI* pParent = NULL);

    CMarkup m_xml;
	CPaintManagerUI* m_pManager;
	IDialogBuilder_GetID* m_getid;
};

} // namespace SdkLayout

#endif // __UIDLGBUILDER_H__
