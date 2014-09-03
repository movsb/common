#include "StdAfx.h"

namespace SdkLayout
{
	CContainerUI::CContainerUI()
		: m_bEnableUpdate(true)
	{

	}

	CContainerUI::~CContainerUI()
	{
		RemoveAll();
	}

	void CContainerUI::DoInit()
	{
		CControlUI::DoInit();

		for(int c=GetCount(), i=0; i < c; i++){
			CControlUI* pControl = static_cast<CControlUI*>(m_items[i]);
			pControl->DoInit();
		}
	}

	int CContainerUI::GetCount() const
	{
		return m_items.GetSize();
	}

	bool CContainerUI::Add(CControlUI* pControl)
	{
		if( pControl == NULL) return false;
		pControl->SetParent(this);
		return m_items.Add(pControl);   
	}

	bool CContainerUI::Remove(CControlUI* pControl)
	{
		if( pControl == NULL) return false;

		for( int it = 0; it < m_items.GetSize(); it++ ) {
			if( static_cast<CControlUI*>(m_items[it]) == pControl ) {
				return m_items.Remove(it);
			}
		}
		return false;
	}

	void CContainerUI::RemoveAll()
	{
		for(int c=m_items.GetSize(), i=0; i<c; ++i){
			delete static_cast<CControlUI*>(m_items[i]);
		}
		m_items.Empty();
	}

	void CContainerUI::SetPos(const CDuiRect& rc)
	{
		CControlUI::SetPos(rc);

		if( m_items.IsEmpty() ) return;

		CDuiRect rct = m_rcItem;
		rct.left   += m_rcInset.left;
		rct.top    += m_rcInset.top;
		rct.right  -= m_rcInset.right;
		rct.bottom -= m_rcInset.bottom;

		for( int it = 0; it < m_items.GetSize(); it++ ) {
			CControlUI* pControl = static_cast<CControlUI*>(m_items[it]);
			if( !pControl->IsVisible() ) continue;
			pControl->SetPos(rct); // 所有非float子控件放大到整个客户区
		}
	}

	void CContainerUI::SetManager(CPaintManagerUI* mgr)
	{
		CControlUI::SetManager(mgr);

		for(int c = GetCount(), i=0; i<c; ++i){
			CControlUI* pControl = static_cast<CControlUI*>(m_items[i]);
			pControl->SetManager(mgr);
		}
	}

	CControlUI* CContainerUI::FindControl(LPCTSTR name)
	{
		UINT hash = HashKey(name);
		if(hash == m_name) return static_cast<CControlUI*>(this);
		else{
			for(int i=0; i<m_items.GetSize(); i++){
				CControlUI* pControl = static_cast<CControlUI*>(m_items[i]);
				CControlUI* pSubControl = NULL;
				pSubControl = pControl->FindControl(name);
				if(pSubControl)
					return pSubControl;
			}
		}
		return NULL;
	}

	void CContainerUI::SetVisible( bool bVisible /*= true*/)
	{
		m_bVisible = bVisible;

		for(int i=0; i<m_items.GetSize(); i++){
			CControlUI* pControl = static_cast<CControlUI*>(m_items[i]);
			pControl->SetVisibleByParent(bVisible);
		}

		NeedParentUpdate();
	}

	void CContainerUI::SetDisplayed( bool bDisplayed )
	{
		m_bDisplayed = bDisplayed;

		for(int i=0; i<m_items.GetSize(); i++){
			CControlUI* pControl = static_cast<CControlUI*>(m_items[i]);
			pControl->SetVisibleByParent(IsVisible());
		}

		NeedUpdate();
	}
	
	void CContainerUI::SetFont( int id )
	{
		CControlUI::SetFont(id);

		for(int i=0; i<m_items.GetSize(); i++){
			CControlUI* pControl = static_cast<CControlUI*>(m_items[i]);
			pControl->SetFont(id);
		}
	}

} // namespace SdkLayout
