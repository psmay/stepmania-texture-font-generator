#include "stdafx.h"
#include "Texture Font Generator.h"
#include "Texture Font GeneratorDlg.h"
#include "TextureFont.h"
#include "Utils.h"

#include "charmaps.h"

#include <algorithm>
#include <vector>
#include <fstream>
#include <sstream>
#include <set>
using namespace std;

#include <math.h>

IMPLEMENT_DYNAMIC(CTextureFontGeneratorDlg, CDialog);
CTextureFontGeneratorDlg::CTextureFontGeneratorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTextureFontGeneratorDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	textureFont = new TextureFont;
}
CTextureFontGeneratorDlg::~CTextureFontGeneratorDlg()
{
	delete textureFont;
}

void CTextureFontGeneratorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PICTURE, m_FontView);
	DDX_Control(pDX, IDC_SHOWN_PAGE, m_ShownPage);
	DDX_Control(pDX, IDC_FAMILY_LIST, m_FamilyList);
	DDX_Control(pDX, IDC_TEXT_OVERLAP, m_TextOverlap);
	DDX_Control(pDX, IDC_FONT_SIZE, m_FontSize);
	DDX_Control(pDX, IDC_ERROR_OR_WARNING, m_ErrorOrWarning);
	DDX_Control(pDX, IDC_CLOSEUP, m_CloseUp);
	DDX_Control(pDX, IDC_SPIN_TOP, m_SpinTop);
	DDX_Control(pDX, IDC_SPIN_BASELINE, m_SpinBaseline);
	DDX_Control(pDX, IDC_PADDING, m_Padding);
}

BEGIN_MESSAGE_MAP(CTextureFontGeneratorDlg, CDialog)
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_SHOWN_PAGE, OnCbnSelchangeShownPage)
	ON_CBN_SELCHANGE(IDC_FAMILY_LIST, OnCbnSelchangeFamilyList)
	ON_EN_CHANGE(IDC_FONT_SIZE, OnEnChangeFontSize)
	ON_COMMAND(ID_STYLE_BOLD, OnStyleBold)
	ON_COMMAND(ID_STYLE_ITALIC, OnStyleItalic)
	ON_COMMAND(ID_STYLE_ANTIALIASED, OnStyleAntialiased)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_TOP, OnDeltaposSpinTop)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_BASELINE, OnDeltaposSpinBaseline)
	ON_EN_CHANGE(IDC_PADDING, OnEnChangePadding)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_EXIT, OnFileExit)
	ON_COMMAND(ID_ACCELERATOR_ITALIC, OnStyleItalic)
	ON_COMMAND(ID_ACCELERATOR_BOLD, OnStyleBold)
	ON_COMMAND(ID_OPTIONS_DOUBLERES, &CTextureFontGeneratorDlg::OnOptionsDoubleres)
	ON_COMMAND(ID_OPTIONS_EXPORTSTROKETEMPLATES, &CTextureFontGeneratorDlg::OnOptionsExportstroketemplates)
	ON_COMMAND(ID_OPTIONS_NUMBERSONLY, &CTextureFontGeneratorDlg::OnOptionsNumbersonly)
END_MESSAGE_MAP()

inline FontPageDescription generate_font_page_desc(const wstring & name, const wchar_t * map)
{
	FontPageDescription desc;

	desc.name = name;
	for( int i = 0; map[i]; ++i )
	{
		wchar_t wc = map[i];
		if( wc != 0xFFFF )
			desc.chars.push_back(wc);
	}

	return desc;
}

inline vector<FontPageDescription> generate_font_page_descs(bool bNumbersOnly)
{
	vector<FontPageDescription> descs;

	if( !bNumbersOnly )
	{
		descs.push_back(generate_font_page_desc(L"main", map_cp1252));
		descs.push_back(generate_font_page_desc(L"alt", map_iso_8859_2));
	}
	else
	{
		descs.push_back(generate_font_page_desc(L"numbers", map_numbers));
	}

	return descs;
}

inline wstring get_text(CWnd & window)
{
	CString text;
	window.GetWindowText(text);
	CStringW result(text);
	return result.GetString();
}

inline bool get_checked(CMenu * pMenu, UINT item)
{
	return !!(pMenu->GetMenuState(item, 0) & MF_CHECKED);
}

bool CTextureFontGeneratorDlg::isChecked(UINT item)
{
	return get_checked(GetMenu(), item);
}

bool CTextureFontGeneratorDlg::isBold()
{
	return isChecked(ID_STYLE_BOLD);
}

bool CTextureFontGeneratorDlg::isItalic()
{
	return isChecked(ID_STYLE_ITALIC);
}

bool CTextureFontGeneratorDlg::isAntialiased()
{
	return isChecked(ID_STYLE_ANTIALIASED);
}

bool CTextureFontGeneratorDlg::isNumbersOnly()
{
	return isChecked(ID_OPTIONS_NUMBERSONLY);
}

bool CTextureFontGeneratorDlg::exportsStrokeTemplates()
{
	return isChecked(ID_OPTIONS_EXPORTSTROKETEMPLATES);
}

bool CTextureFontGeneratorDlg::doubleRes()
{
	return isChecked(ID_OPTIONS_DOUBLERES);
}

wstring CTextureFontGeneratorDlg::getFamily()
{
	return get_text(m_FamilyList);
}

float CTextureFontGeneratorDlg::getFontSizePixels()
{
	return (float) wcstod(get_text(m_FontSize).c_str(), NULL);
}

int CTextureFontGeneratorDlg::getPadding()
{
	return (int) wcstol(get_text(m_Padding).c_str(), NULL, 10);
}

/* Regenerate the font, pulling in settings from widgets. */
void CTextureFontGeneratorDlg::UpdateFont( bool bSavingDoubleRes )
{
	m_bUpdateFontNeeded = false;

	m_FontView.SetBitmap( NULL );

	wstring sOld;
	{
		int iOldSel = m_ShownPage.GetCurSel();
		if( iOldSel != -1 && iOldSel < (int) textureFont->m_PagesToGenerate.size() )
			sOld = textureFont->m_PagesToGenerate[iOldSel].name;
	}

	textureFont->m_sFamily = getFamily();
	textureFont->m_fFontSizePixels = getFontSizePixels() * (bSavingDoubleRes ? 2.0f : 1.0f);
	textureFont->m_iPadding = getPadding() * (bSavingDoubleRes ? 2 : 1);
	textureFont->m_bBold = isBold();
	textureFont->m_bItalic = isItalic();
	textureFont->m_bAntiAlias = isAntialiased();
	textureFont->m_PagesToGenerate = generate_font_page_descs(isNumbersOnly());

	/* Go: */
	textureFont->FormatFontPages();

	m_SpinTop.SetPos( textureFont->m_iCharTop );
	m_SpinBaseline.SetPos( textureFont->m_iCharBaseline );

	m_ShownPage.ResetContent();
	for( unsigned p = 0; p < textureFont->m_PagesToGenerate.size(); ++p )
		m_ShownPage.AddString( CString(textureFont->m_PagesToGenerate[p].name.c_str()).GetString() );
	int iRet = m_ShownPage.FindStringExact( -1, CString(sOld.c_str()).GetString() );
	if( iRet == CB_ERR )
		iRet = 0;
	m_ShownPage.SetCurSel( iRet );
}

void CTextureFontGeneratorDlg::UpdateCloseUp()
{
	HBITMAP hBitmap = m_CloseUp.GetBitmap();
	m_CloseUp.SetBitmap( NULL );
	if( hBitmap )
		DeleteObject( hBitmap );

	HBITMAP hSample = textureFont->m_Characters[L'A'];
	if( hSample == NULL )
		return;

	HDC hCharacterDC = CreateCompatibleDC( NULL );
	HGDIOBJ hOldCharacterBitmap = SelectObject( hCharacterDC, hSample );

	BITMAPINFO CharacterInfo;
	memset( &CharacterInfo, 0, sizeof(CharacterInfo) );
	CharacterInfo.bmiHeader.biSize = sizeof(CharacterInfo.bmiHeader);
	GetDIBits( hCharacterDC, hSample, 0, 0, NULL, &CharacterInfo, DIB_RGB_COLORS );

	int iWidth = CharacterInfo.bmiHeader.biWidth;
	int iHeight = CharacterInfo.bmiHeader.biHeight;

	/* Set up a bitmap to zoom the image into. */
	int iSourceHeight = textureFont->m_BoundingRect.bottom;
	const int iZoomFactor = 4;

	HBITMAP hZoom;
	{
		HDC hTempDC = ::GetDC(NULL);
		hZoom = CreateCompatibleBitmap( hTempDC, iWidth * iZoomFactor, iSourceHeight * iZoomFactor );
		::ReleaseDC( NULL, hTempDC );
	}
	HDC hZoomDC = CreateCompatibleDC( NULL );
	HGDIOBJ hOldZoomBitmap = SelectObject( hZoomDC, hZoom );

	StretchBlt(
		hZoomDC, 0, 0,
		iWidth * iZoomFactor, iHeight * iZoomFactor,
		hCharacterDC, 0, 0,
		iWidth, iHeight,
		SRCCOPY
	);
	SelectObject( hCharacterDC, hOldCharacterBitmap );
	::DeleteDC( hCharacterDC );

	HPEN hPen = CreatePen( PS_SOLID, 1, RGB(0, 255, 255) );
	HGDIOBJ hOldPen = SelectObject( hZoomDC, hPen );

	/* Align the line with the top of the unit, so, when correct, the font
	 * "sits" on the line. */
	int iY = iZoomFactor*textureFont->m_iCharBaseline - 1;
	MoveToEx( hZoomDC, 0, iY, NULL );
	LineTo( hZoomDC, iWidth * iZoomFactor, iY );

	SelectObject( hZoomDC, hOldPen );
	DeleteObject( hPen );

	hPen = CreatePen( PS_SOLID, 1, RGB(255, 255, 50) );
	hOldPen = SelectObject( hZoomDC, hPen );
	
	/* Align the line with the bottom of the unit, so, when correct, the line
	 * "sits" on the font. */
	iY = iZoomFactor*textureFont->m_iCharTop - 1;
	MoveToEx( hZoomDC, 0, iY, NULL );
	LineTo( hZoomDC, iWidth * iZoomFactor, iY );

	SelectObject( hZoomDC, hOldPen );
	DeleteObject( hPen );

	SelectObject( hZoomDC, hOldZoomBitmap );
	::DeleteDC( hZoomDC );

	m_CloseUp.SetBitmap( hZoom );
}


int CALLBACK CTextureFontGeneratorDlg::EnumFontFamiliesCallback( const LOGFONTA *pLogicalFontData, const TEXTMETRICA *pPhysicalFontData, DWORD FontType, LPARAM lParam )
{
	CTextureFontGeneratorDlg *pThis = (CTextureFontGeneratorDlg *) lParam;
	set<CString> *pSet = (set<CString> *) lParam;
	pSet->insert( pLogicalFontData->lfFaceName );
	return 1;
}

BOOL CTextureFontGeneratorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// XXX: AddFontResourceEx
	// AddFontMemResourceEx
	// drag drop
	{
		LOGFONT font;
		memset( &font, 0, sizeof(font) );
		font.lfCharSet = DEFAULT_CHARSET;
		CPaintDC dc(this);
		set<CString> setFamilies;
		EnumFontFamiliesEx( dc.GetSafeHdc(), &font, EnumFontFamiliesCallback, (LPARAM) &setFamilies, 0 );
		for( set<CString>::const_iterator it = setFamilies.begin(); it != setFamilies.end(); ++it )
			m_FamilyList.AddString( *it );
		m_FamilyList.SetCurSel( 0 );
	}

	CMenu *pMenu = GetMenu();
	pMenu->CheckMenuItem( ID_STYLE_ANTIALIASED, MF_CHECKED );

	m_FontSize.SetWindowText( "20" );
	m_Padding.SetWindowText( "2" );
	m_SpinTop.SetRange( 64, 0 );
	m_SpinBaseline.SetRange( 64, 0 );

	m_bUpdateFontNeeded = true;
	m_bUpdateFontViewAndCloseUpNeeded = true;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

inline void set_window_text(CWnd & window, const wstring & text)
{
	window.SetWindowText(CString(text.c_str()).GetString());
}

void CTextureFontGeneratorDlg::UpdateFontViewAndCloseUp()
{
	m_bUpdateFontViewAndCloseUpNeeded = false;
	m_FontView.SetBitmap( NULL );

	m_SpinTop.EnableWindow( true );
	m_SpinBaseline.EnableWindow( true );
	GetMenu()->EnableMenuItem( ID_FILE_SAVE, MF_ENABLED );

	if( textureFont->m_sError != L"" )
	{
		m_SpinTop.EnableWindow(false);
		m_SpinBaseline.EnableWindow(false);
		GetMenu()->EnableMenuItem( ID_FILE_SAVE, MF_GRAYED );

		wstring errorMessage(L"Error: ");
		errorMessage += textureFont->m_sError;

		set_window_text(m_ErrorOrWarning, wstring(L"Error: ") + textureFont->m_sError);
		return;
	}

	set_window_text(m_ErrorOrWarning, textureFont->m_sWarnings);

	const int iSelectedPage = m_ShownPage.GetCurSel();
	ASSERT( iSelectedPage < (int) textureFont->m_apPages.size() );

	textureFont->m_iCharTop = LOWORD(m_SpinTop.GetPos());
	textureFont->m_iCharBaseline = LOWORD(m_SpinBaseline.GetPos());

	HBITMAP hBitmap = textureFont->m_apPages[iSelectedPage]->m_hPage;
	m_FontView.SetBitmap( hBitmap );
	
	UpdateCloseUp();

	CString sStr;
	sStr.Format("Overlap: %i, %i\nMaximum size: %ix%i", 
		textureFont->m_iCharLeftOverlap, textureFont->m_iCharRightOverlap,
		textureFont->m_BoundingRect.right - textureFont->m_BoundingRect.left,
		textureFont->m_BoundingRect.bottom - textureFont->m_BoundingRect.top
		);
	m_TextOverlap.SetWindowText( sStr );
}

void CTextureFontGeneratorDlg::OnDestroy()
{
	WinHelp(0L, HELP_QUIT);
	CDialog::OnDestroy();
}

void CTextureFontGeneratorDlg::OnPaint() 
{
	if( IsIconic() )
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
		return;
	}

	if( m_bUpdateFontNeeded )
	{
		m_bUpdateFontViewAndCloseUpNeeded = true;
		UpdateFont( false );
	}

	if( m_bUpdateFontViewAndCloseUpNeeded )
		UpdateFontViewAndCloseUp();

	CDialog::OnPaint();
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTextureFontGeneratorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CTextureFontGeneratorDlg::OnClose() 
{
	if( CanExit() )
		CDialog::OnClose();
}

void CTextureFontGeneratorDlg::OnOK() 
{
	if( CanExit() )
		CDialog::OnOK();
}

void CTextureFontGeneratorDlg::OnCancel() 
{
	if (CanExit())
		CDialog::OnCancel();
}

BOOL CTextureFontGeneratorDlg::OnMouseWheel( UINT nFlags, short zDelta, CPoint pt )
{
	CWnd *pFocus = this->GetFocus();
	if( nFlags & MK_CONTROL )
		zDelta *= 5;
	if( pFocus == &m_Padding || pFocus == &m_FontSize )
	{
		CEdit *pFocusedEdit = dynamic_cast<CEdit *>(pFocus);
		CString sText;
		pFocusedEdit->GetWindowText( sText );
		int i = atoi( sText );
		i += zDelta / WHEEL_DELTA;
		if( pFocus == &m_Padding )
			i = max( i, 0 );
		else if( pFocus == &m_FontSize )
			i = max( i, 1 );

		sText.Format( "%i", i );
		pFocusedEdit->SetWindowText( sText );
		return FALSE;
	}

	return CDialog::OnMouseWheel( nFlags, zDelta, pt );
}

BOOL CTextureFontGeneratorDlg::CanExit()
{
	return TRUE;
}

void CTextureFontGeneratorDlg::OnCbnSelchangeShownPage()
{
	m_bUpdateFontViewAndCloseUpNeeded = true;
	Invalidate( FALSE );
	UpdateWindow();
}

void CTextureFontGeneratorDlg::OnCbnSelchangeFamilyList()
{
	m_bUpdateFontNeeded = true;
	Invalidate( FALSE );
	UpdateWindow();
}

void CTextureFontGeneratorDlg::OnEnChangeFontSize()
{
	m_bUpdateFontNeeded = true;
	Invalidate( FALSE );
	UpdateWindow();
}

void CTextureFontGeneratorDlg::OnEnChangePadding()
{
	m_bUpdateFontNeeded = true;
	Invalidate( FALSE );
}

void CTextureFontGeneratorDlg::OnStyleAntialiased()
{
	CMenu *pMenu = GetMenu();
	int Checked = pMenu->GetMenuState(ID_STYLE_ANTIALIASED, 0) & MF_CHECKED;
	Checked ^= MF_CHECKED;
	pMenu->CheckMenuItem( ID_STYLE_ANTIALIASED, Checked );
	m_bUpdateFontNeeded = true;
	Invalidate( FALSE );
}

void CTextureFontGeneratorDlg::OnStyleBold()
{
	CMenu *pMenu = GetMenu();
	int Checked = pMenu->GetMenuState(ID_STYLE_BOLD, 0) & MF_CHECKED;
	Checked ^= MF_CHECKED;
	pMenu->CheckMenuItem( ID_STYLE_BOLD, Checked );
	m_bUpdateFontNeeded = true;
	Invalidate( FALSE );
}

void CTextureFontGeneratorDlg::OnStyleItalic()
{
	CMenu *pMenu = GetMenu();
	int Checked = pMenu->GetMenuState(ID_STYLE_ITALIC, 0) & MF_CHECKED;
	Checked ^= MF_CHECKED;
	pMenu->CheckMenuItem( ID_STYLE_ITALIC, Checked );
	m_bUpdateFontNeeded = true;
	Invalidate( FALSE );
}

void CTextureFontGeneratorDlg::OnOptionsDoubleres()
{
	CMenu *pMenu = GetMenu();
	int Checked = pMenu->GetMenuState(ID_OPTIONS_DOUBLERES, 0) & MF_CHECKED;
	Checked ^= MF_CHECKED;
	pMenu->CheckMenuItem( ID_OPTIONS_DOUBLERES, Checked );
	m_bUpdateFontNeeded = true;
	Invalidate( FALSE );
}

void CTextureFontGeneratorDlg::OnOptionsExportstroketemplates()
{
	CMenu *pMenu = GetMenu();
	int Checked = pMenu->GetMenuState(ID_OPTIONS_EXPORTSTROKETEMPLATES, 0) & MF_CHECKED;
	Checked ^= MF_CHECKED;
	pMenu->CheckMenuItem( ID_OPTIONS_EXPORTSTROKETEMPLATES, Checked );
}

void CTextureFontGeneratorDlg::OnOptionsNumbersonly()
{
	CMenu *pMenu = GetMenu();
	int Checked = pMenu->GetMenuState(ID_OPTIONS_NUMBERSONLY, 0) & MF_CHECKED;
	Checked ^= MF_CHECKED;
	pMenu->CheckMenuItem( ID_OPTIONS_NUMBERSONLY, Checked );
	m_bUpdateFontNeeded = true;
	Invalidate( FALSE );
}

void CTextureFontGeneratorDlg::OnDeltaposSpinTop(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_bUpdateFontViewAndCloseUpNeeded = true;
	Invalidate( FALSE );
}

void CTextureFontGeneratorDlg::OnDeltaposSpinBaseline(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_bUpdateFontViewAndCloseUpNeeded = true;
	Invalidate( FALSE );
}

wstring get_default_save_file_name(TextureFont * textureFont)
{
	wstring sName = textureFont->m_sFamily;
	transform(sName.begin(), sName.end(), sName.begin(), ::tolower);

	wostringstream ss;
	ss << L"_"
		<< sName
		<< (textureFont->m_bBold ? " Bold" : "")
		<< (textureFont->m_bItalic ? " Italic" : "")
		<< textureFont->m_fFontSizePixels
		<< "px";

	return ss.str();
}

inline wstring show_file_save_dialog(HWND owner, TextureFont * textureFont)
{
	// XXX
	OPENFILENAME ofn;
	memset( &ofn, 0, sizeof(ofn) );
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = "*.ini\000Font control files\000\000";

	char szFile[1024] = "";
	
	ofn.lpstrFile = szFile;
	{
		wstring saveFileName = get_default_save_file_name(textureFont);
		strncpy(szFile, CString(saveFileName.c_str()).GetString(), sizeof(szFile) - 1);
	}
	ofn.nMaxFile = sizeof(szFile);

	// lpstrInitialDir
	ofn.Flags = OFN_HIDEREADONLY|OFN_LONGNAMES;

	if( !GetSaveFileName(&ofn) )
		return L"";

	/* If the filename provided has an extension, remove it. */
	{
		char *pExt = strrchr( szFile, '.' );
		if( pExt )
			*pExt = 0;
	}

	return wstring(CStringW(szFile).GetString());
}

void CTextureFontGeneratorDlg::OnFileSave()
{
	wstring filename = show_file_save_dialog(m_hWnd, textureFont);
	if(filename == L"")
		return;

	bool bExportStrokeTemplates = exportsStrokeTemplates();
	bool bDoubleRes = doubleRes();

	if( bDoubleRes )
	{
		textureFont->Save( filename, L"", true, false, bExportStrokeTemplates );	// save metrics
		UpdateFont( true );	// generate DoubleRes bitmaps
		textureFont->Save( filename, L" (doubleres)", false, true, bExportStrokeTemplates );	// save bitmaps
		// reset to normal, non-DoubleRes font
		m_bUpdateFontNeeded = true;
		Invalidate( FALSE );
		UpdateWindow();
	}
	else	// normal res
	{
		textureFont->Save( filename, L"", true, true, bExportStrokeTemplates );	// save metrics and bitmaps
	}
}

void CTextureFontGeneratorDlg::OnFileExit()
{
	DestroyWindow();
}

/*
 * Copyright (c) 2003-2007 Glenn Maynard
 * Copyright (c) 2014 Peter S. May
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
