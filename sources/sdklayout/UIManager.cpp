#include "StdAfx.h"

namespace SdkLayout {

CPaintManagerUI::CPaintManagerUI()
{
    LOGFONT lf = { 0 };
    ::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
    lf.lfCharSet = DEFAULT_CHARSET;
    m_DefaultFontInfo = ::CreateFontIndirect(&lf);
}

CPaintManagerUI::~CPaintManagerUI()
{
    ::DeleteObject(m_DefaultFontInfo);
    RemoveAllFonts();
}

HFONT CPaintManagerUI::GetDefaultFont()
{
  return m_DefaultFontInfo;
}

void CPaintManagerUI::SetDefaultFont( LPCTSTR pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic )
{
	if(m_DefaultFontInfo) ::DeleteObject(m_DefaultFontInfo);

	LOGFONT lf = { 0 };
	::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
	_tcsncpy(lf.lfFaceName, pStrFontName, LF_FACESIZE);
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfHeight = -nSize;
	if( bBold ) lf.lfWeight += FW_BOLD;
	if( bUnderline ) lf.lfUnderline = TRUE;
	if( bItalic ) lf.lfItalic = TRUE;
	m_DefaultFontInfo = ::CreateFontIndirect(&lf);
}

DWORD CPaintManagerUI::GetCustomFontCount() const
{
    return m_aCustomFonts.GetSize();
}

HFONT CPaintManagerUI::AddFont(LPCTSTR pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic)
{
    LOGFONT lf = { 0 };
    ::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
    _tcsncpy(lf.lfFaceName, pStrFontName, LF_FACESIZE);
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfHeight = -nSize;
    if( bBold ) lf.lfWeight += FW_BOLD;
    if( bUnderline ) lf.lfUnderline = TRUE;
    if( bItalic ) lf.lfItalic = TRUE;
    HFONT hFont = ::CreateFontIndirect(&lf);
    if( hFont == NULL ) return NULL;

    if( !m_aCustomFonts.Add(hFont) ) {
        ::DeleteObject(hFont);
        return NULL;
    }
    return hFont;
}

bool CPaintManagerUI::RemoveFont(HFONT hFont)
{
    for( int it = 0; it < m_aCustomFonts.GetSize(); it++ ) {
		HFONT hFont2 = static_cast<HFONT>(m_aCustomFonts[it]);
        if( hFont == hFont2 ) {
            ::DeleteObject(hFont);
            return m_aCustomFonts.Remove(it);
        }
    }
    return false;
}

void CPaintManagerUI::RemoveAllFonts()
{
    for( int it = 0; it < m_aCustomFonts.GetSize(); it++ ) {
        HFONT hFont = static_cast<HFONT>(m_aCustomFonts[it]);
        ::DeleteObject(hFont);
    }
    m_aCustomFonts.Empty();
}

HFONT CPaintManagerUI::GetFont( int index )
{
	if( index < 0 || index >= m_aCustomFonts.GetSize() ) return GetDefaultFont();
	return static_cast<HFONT>(m_aCustomFonts[index]);
}

int CPaintManagerUI::GetFont(HFONT hFont)
{
	for( int it = 0; it < m_aCustomFonts.GetSize(); it++ ) {
        HFONT hF = static_cast<HFONT>(m_aCustomFonts[it]);
        if(hFont == hF)
			return it;
    }
	return -1;
}

} // namespace SdkLayout
