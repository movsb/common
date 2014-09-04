#include "StdAfx.h"

namespace SdkLayout {

CContainerUI* CDialogBuilder::Create(LPCTSTR xml, CPaintManagerUI* manager, HINSTANCE hInst)
{
	//资源ID为0-65535，两个字节；字符串指针为4个字节
    if( HIWORD(xml) != NULL ) {
		if( !m_xml.Load(xml) ) 
			return NULL;
    }
	else{
		HRSRC hResource = ::FindResource(hInst, xml, RT_RCDATA);
		if( hResource == NULL ) return NULL;
		HGLOBAL hGlobal = ::LoadResource(hInst, hResource);
		if( hGlobal == NULL ) {
			::FreeResource(hResource);
			return NULL;
		}

		if( !m_xml.LoadFromMem((BYTE*)::LockResource(hGlobal), ::SizeofResource(hInst, hResource) )) 
			return NULL;
		::FreeResource(hResource);
	}
    
	CMarkupNode root = m_xml.GetRoot();
	if( !root.IsValid() ) return NULL;

	LPCTSTR pRootName = root.GetName();
	if (_tcscmp(pRootName, _T("Window")))
		return NULL;

	int nAttributes = root.GetAttributeCount();
	for (int i = 0; i < nAttributes; i++) {
		LPCTSTR pstrName = root.GetAttributeName(i);
		LPCTSTR pstrValue = root.GetAttributeValue(i);
		if (_tcscmp(pstrName, _T("size")) == 0) {
			LPTSTR pstr = NULL;
			int cx = _tcstol(pstrValue, &pstr, 10);  assert(pstr);
			int cy = _tcstol(pstr + 1, &pstr, 10);    assert(pstr);
			SIZE& initsz = manager->InitSize();
			initsz.cx = cx;
			initsz.cy = cy;
		}
	}
	m_pManager = manager;

	return static_cast<CContainerUI*>(_Parse(&root, NULL));
}

CControlUI* CDialogBuilder::_Parse(CMarkupNode* pRoot, CContainerUI* pParent)
{
    CControlUI* pReturn = NULL;
    for( CMarkupNode node = pRoot->GetChild() ; node.IsValid(); node = node.GetSibling() ) {
        LPCTSTR pstrClass = node.GetName();

        CControlUI* pControl = NULL;
		LPCTSTR pstrName,pstrValue;

		if(_tcscmp(pstrClass, _T("Font")) == 0){
			int nAttributes = node.GetAttributeCount();
			LPCTSTR pFontName = NULL;
			int size = 12;
			bool bold = false;
			bool underline = false;
			bool italic = false;
			bool defaultfont = false;

			for( int i = 0; i < nAttributes; i++ ) {
				pstrName = node.GetAttributeName(i);
				pstrValue = node.GetAttributeValue(i);
				if( _tcscmp(pstrName, _T("name")) == 0 ) {
					pFontName = pstrValue;
				}
				else if( _tcscmp(pstrName, _T("size")) == 0 ) {
					TCHAR* pstr=NULL;
					size = _tcstol(pstrValue, &pstr, 10);
				}
				else if( _tcscmp(pstrName, _T("bold")) == 0 ) {
					bold = (_tcscmp(pstrValue, _T("true")) == 0);
				}
				else if( _tcscmp(pstrName, _T("underline")) == 0 ) {
					underline = (_tcscmp(pstrValue, _T("true")) == 0);
				}
				else if( _tcscmp(pstrName, _T("italic")) == 0 ) {
					italic = (_tcscmp(pstrValue, _T("true")) == 0);
				}
				else if( _tcscmp(pstrName, _T("default")) == 0 ) {
					defaultfont = (_tcscmp(pstrValue, _T("true")) == 0);
				}
			}
			if(pFontName && m_pManager) {
				m_pManager->AddFont(pFontName, size, bold, underline, italic);
				if(defaultfont)
					m_pManager->SetDefaultFont(pFontName, size, bold, underline, italic);
			}
			continue;
		}
		else if(_tcscmp(pstrClass, _T("Control")) == 0)		pControl = new CControlUI;
		else if(_tcscmp(pstrClass, _T("Container")) == 0)	pControl = new CContainerUI;
		else if(_tcscmp(pstrClass, _T("Vertical")) == 0)	pControl = new CVerticalLayoutUI;
		else if(_tcscmp(pstrClass, _T("Horizontal")) == 0)	pControl = new CHorizontalLayoutUI;
		else if(_tcscmp(pstrClass, _T("Button"))==0)		pControl = new CButtonUI;
		else if(_tcscmp(pstrClass, _T("Option"))==0)		pControl = new COptionUI;
		else if(_tcscmp(pstrClass, _T("Check"))==0)			pControl = new CCheckUI;
		else if(_tcscmp(pstrClass, _T("Static"))==0)		pControl = new CStaticUI;
		else if (_tcscmp(pstrClass, _T("Group")) == 0)		pControl = new CGroupUI;

        if( node.HasChildren() ) {
            _Parse(&node, static_cast<CContainerUI*>(pControl));
        }

		if( pParent != NULL ) {
			if( !pParent->Add(pControl) ) {
				delete pControl;
				continue;
			}
		}

        if( node.HasAttributes() ) {
            int nAttributes = node.GetAttributeCount();
            for( int i = 0; i < nAttributes; i++ ) {
                pControl->SetAttribute(node.GetAttributeName(i), node.GetAttributeValue(i));
            }
        }

        if( pReturn == NULL ) 
			pReturn = pControl;
    }
    return pReturn;
}

} // namespace SdkLayout
