#include "StdAfx.h"
#include "WindowManager.h"

namespace Common{
	c_ptr_array<CWindowManager> CWindowManager::m_aWndMgrs;

	CWindowManager::CWindowManager()
		: m_hWnd(0)
		, m_pMsgFilter(0)
	{

	}

	CWindowManager::~CWindowManager()
	{

	}

	bool CWindowManager::FilterMessage(MSG* pmsg)
	{
		return m_pMsgFilter && m_pMsgFilter->FilterMessage(
			pmsg->hwnd, pmsg->message, pmsg->wParam, pmsg->lParam);
	}

	bool CWindowManager::AddWindowManager(CWindowManager* pwm)
	{
		return m_aWndMgrs.add(pwm);
	}

	bool CWindowManager::RemoveWindowManager(CWindowManager* pwm)
	{
		return m_aWndMgrs.remove(pwm);
	}

	bool CWindowManager::TranslateAccelerator( MSG* pmsg )
	{
		for(int i=0; i<m_AcceTrans.size(); i++){
			if(m_AcceTrans[i]->TranslateAccelerator(pmsg)){
				return true;
			}
		}
		return false;
	}

	bool CWindowManager::AddAcceleratorTranslator( IAcceleratorTranslator* pat )
	{
		return m_AcceTrans.add(pat);
	}

	bool CWindowManager::RemoveAcceleratorTranslator( IAcceleratorTranslator* pat )
	{
		return m_AcceTrans.remove(pat);
	}

	void CWindowManager::MessageLoop()
	{
		MSG msg;
		while(::GetMessage(&msg, NULL, 0, 0)){
			if(!TranslateMessage(&msg)){
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
		assert(m_aWndMgrs.size() == 0);
	}

	bool CWindowManager::TranslateMessage( MSG* pmsg )
	{
		bool bChild = !!(GetWindowStyle(pmsg->hwnd) & WS_CHILD);
		if(bChild){
// 			HWND hParent = ::GetParent(pmsg->hwnd);
// 			for(int i=0; i<m_aWndMgrs.size(); i++){
// 				CWindowManager* pWM = m_aWndMgrs.getat(i);
// 				HWND hTempParent = hParent;
// 				while(hTempParent){
// 					if(pmsg->hwnd == pWM->hWnd() || hTempParent==pWM->hWnd()){
// 						if(pWM->TranslateAccelerator(pmsg))
// 							return true;
// 						if(pWM->FilterMessage(pmsg))
// 							return true;
// 
// 						return false;
// 					}
// 					hTempParent = ::GetParent(hTempParent);
// 				}
// 			}
			HWND hParent = pmsg->hwnd;
			while (hParent && ::GetWindowLongPtr(hParent, GWL_STYLE)&WS_CHILD){
				hParent = ::GetParent(hParent);
			}

			if (hParent != NULL){
				for (int i = 0; i < m_aWndMgrs.size(); i++){
					CWindowManager* pWM = m_aWndMgrs.getat(i);
					if (pWM->hWnd() == hParent){
						if (pWM->TranslateAccelerator(pmsg))
							return true;
						if (pWM->FilterMessage(pmsg))
							return true;
						return false;
					}
				}
			}
		}
		else{
			for(int i=0; i<m_aWndMgrs.size(); i++){
				CWindowManager* pWM = m_aWndMgrs.getat(i);
				if(pmsg->hwnd == pWM->hWnd()){
					if(pWM->TranslateAccelerator(pmsg))
						return true;
					if(pWM->FilterMessage(pmsg))
						return true;

					return false;
				}
			}
		}
		return false;
	}

	void CWindowManager::Init( HWND hWnd , IMessageFilter* flt)
	{
		//TODO
		//assert((GetWindowLongPtr(hWnd, GWL_STYLE)&WS_CHILD) == 0);
		m_hWnd = hWnd;
		AddWindowManager(this);
		MessageFilter() = flt;
	}

	void CWindowManager::DeInit()
	{
		RemoveWindowManager(this);
	}

}
