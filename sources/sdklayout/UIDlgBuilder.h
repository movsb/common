#ifndef __UIDLGBUILDER_H__
#define __UIDLGBUILDER_H__

#pragma once

namespace SdkLayout {

class CDialogBuilder
{
public:
    CContainerUI* Create(LPCTSTR xml, CPaintManagerUI* manager, HINSTANCE hInst = NULL);

private:
    CControlUI* _Parse(CMarkupNode* parent, CContainerUI* pParent = NULL);

    CMarkup m_xml;
	CPaintManagerUI* m_pManager;
};

} // namespace SdkLayout

#endif // __UIDLGBUILDER_H__
