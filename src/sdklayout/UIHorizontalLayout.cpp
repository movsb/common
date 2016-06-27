#include "stdafx.h"
#include "UIHorizontalLayout.h"

namespace SdkLayout
{
	void CHorizontalLayoutUI::SetPos(const CDuiRect& rc)
	{
		CControlUI::SetPos(rc);

		if( m_items.IsEmpty() ) return;

		CDuiRect rct = m_rcItem;
		rct.left   += m_rcInset.left;
		rct.top    += m_rcInset.top;
		rct.right  -= m_rcInset.right;
		rct.bottom -= m_rcInset.bottom;

		// Determine the width of elements that are sizeable
		SIZE szAvailable = {rct.GetWidth(), rct.GetHeight()};

		int nAdjustables = 0;
		int cxFixed = 0;
		int nEstimateNum = 0;
		for( int it1 = 0; it1 < m_items.GetSize(); it1++ ) {
			CControlUI* pControl = static_cast<CControlUI*>(m_items[it1]);
			if( !pControl->IsVisible() ) continue;
			SIZE sz = pControl->EstimateSize(szAvailable);
			if( sz.cx == 0 ) {
				nAdjustables++;
			}
			else {
				if( sz.cx < pControl->GetMinWidth() ) sz.cx = pControl->GetMinWidth();
				if( sz.cx > pControl->GetMaxWidth() ) sz.cx = pControl->GetMaxWidth();
			}
			cxFixed += sz.cx;
			nEstimateNum++;
		}

		int cxExpand = 0;
        int cxNeeded = 0;
		if( nAdjustables > 0 ) cxExpand = MAX(0, (szAvailable.cx - cxFixed) / nAdjustables);
		// Position the elements
		SIZE szRemaining = szAvailable;
		int iPosX = rct.left;
		int iAdjustable = 0;
		int cxFixedRemaining = cxFixed;
		for( int it2 = 0; it2 < m_items.GetSize(); it2++ ) {
			CControlUI* pControl = static_cast<CControlUI*>(m_items[it2]);
			if( !pControl->IsVisible() ) continue;

			SIZE sz = pControl->EstimateSize(szRemaining);
			if( sz.cx == 0 ) {
				iAdjustable++;
				sz.cx = cxExpand;
				// Distribute remaining to last element (usually round-off left-overs)
				if( iAdjustable == nAdjustables ) {
					sz.cx = MAX(0, szRemaining.cx - cxFixedRemaining);
				}
				if( sz.cx < pControl->GetMinWidth() ) sz.cx = pControl->GetMinWidth();
				if( sz.cx > pControl->GetMaxWidth() ) sz.cx = pControl->GetMaxWidth();
			}
			else {
				if( sz.cx < pControl->GetMinWidth() ) sz.cx = pControl->GetMinWidth();
				if( sz.cx > pControl->GetMaxWidth() ) sz.cx = pControl->GetMaxWidth();

				cxFixedRemaining -= sz.cx;
			}

			sz.cy = pControl->GetFixedHeight();
			if( sz.cy == 0 ) sz.cy = rct.bottom - rct.top;
			if( sz.cy < 0 ) sz.cy = 0;
			if( sz.cy < pControl->GetMinHeight() ) sz.cy = pControl->GetMinHeight();
			if( sz.cy > pControl->GetMaxHeight() ) sz.cy = pControl->GetMaxHeight();

			RECT rcCtrl = { iPosX, rct.top, iPosX + sz.cx, rct.top + sz.cy};
			pControl->SetPos(rcCtrl);
			iPosX += sz.cx;
            cxNeeded += sz.cx;
			szRemaining.cx -= sz.cx;
		}

		SIZE sztmp = {cxNeeded, m_rcItem.GetHeight()};
		sztmp.cx += m_rcInset.left + m_rcInset.right;
		SetPostSize(sztmp);
	}
}
