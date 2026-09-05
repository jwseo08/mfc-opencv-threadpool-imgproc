#include "pch.h"
#include "CModiButton.h"

#include <algorithm>
#include <iostream>

#include "def.h"

#define SMOOTH_MODE

TBtnColorInfo TBtnColorInfo::Accent(COLORREF dwAccent, COLORREF dwBkg, COLORREF dwFont)
{
	TBtnColorInfo color;
	color.dwBkg = dwBkg;
	color.dwFace = dwAccent;
	color.dwBorder = COLOR_MIN_WHITE;
	color.dwClick = RGB((std::min)(255, static_cast<int>(GetRValue(dwAccent)) + 35),
		(std::min)(255, static_cast<int>(GetGValue(dwAccent)) + 35),
		(std::min)(255, static_cast<int>(GetBValue(dwAccent)) + 35));
	color.dwDisable = COLOR_GRAY; //RGB(75, 75, 75);
	color.dwFont = dwFont;
	return color;
}

CModiButton::CModiButton()
{
	InitVariable();
}

CModiButton::CModiButton(ULONG_PTR* pGdiplusToken)
{
	InitVariable();
	
	m_pGdiplusToken = pGdiplusToken;
	
	if (*m_pGdiplusToken == NULL)
	{
		if (GdiplusStart(m_pGdiplusToken) == false)
		{
			std::cout << "gdi startup fail\n";
		}
	}
}

CModiButton::~CModiButton()
{
	// unique ptr로 변경
	/*delete m_pUpBitImg;
	delete m_pDnBitImg;
	delete m_pDisableBitImg;
	*/

	// gdi plus 종료는 parent 에서 작업
	/*
	if (*m_pGdiplusToken != NULL)
	{
		GdiplusEnd(m_pGdiplusToken);
	}
	*/
}


BEGIN_MESSAGE_MAP(CModiButton, CButton)
	ON_WM_PAINT()
	ON_CONTROL_REFLECT(BN_CLICKED, &CModiButton::OnBnClicked)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_ERASEBKGND()
	ON_WM_ENABLE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()



void CModiButton::InitVariable()
{
	m_pGdiplusToken = NULL;
	m_nState = STATUS_BTN_NONE;
	
	m_dwBkColor = COLOR_MINT_GRAY; 
	m_dwFaceColor = COLOR_DARK_BLUE;
	
	m_dwBorderColor = RGB(255, 255, 255);
	m_dwClickColor = RGB(113, 171, 214);
	m_dwDisableColor = COLOR_GRAY;

	m_csFontFaceName = FONT_FACENAME_DOTUM;
	m_nFontSize = 12;
	m_dwFontColor = RGB(255, 255, 255);

	m_nTextTopMargin = 4;
	m_nBorderLineSize = 1;

	m_nRoundness = 4;
	m_nDepth = 1;

	m_pUpBitImg = nullptr;
	m_pDnBitImg = nullptr;
	m_pDisableBitImg = nullptr;
}

void CModiButton::ResetBitmaps(BOOL bRedraw)
{
	m_pUpBitImg.reset();
	m_pDnBitImg.reset();
	m_pDisableBitImg.reset();
	if (bRedraw && GetSafeHwnd() != NULL) Invalidate(FALSE);
}


void CModiButton::SetColor(COLORREF dwBkColor, COLORREF dwFaceColor, COLORREF dwBorderColor, COLORREF dwClickColor, COLORREF dwDisableColor)
{
	m_dwBkColor = dwBkColor;
	m_dwFaceColor = dwFaceColor;
	m_dwBorderColor = dwBorderColor;
	m_dwClickColor = dwClickColor;
	m_dwDisableColor = dwDisableColor;
	ResetBitmaps();
}

void CModiButton::SetColor(const TBtnColorInfo& tBtnColor, BOOL bRedraw)
{
	m_dwBkColor = tBtnColor.dwBkg;
	m_dwFaceColor = tBtnColor.dwFace;
	m_dwBorderColor = tBtnColor.dwBorder;
	m_dwClickColor = tBtnColor.dwClick;
	m_dwDisableColor = tBtnColor.dwDisable;
	m_dwFontColor = tBtnColor.dwFont;
	ResetBitmaps(bRedraw);
}

void CModiButton::SetColor(COLORREF dwFaceColor, COLORREF dwClickColor, COLORREF dwFontColor)
{
	m_dwFaceColor = dwFaceColor;
	m_dwClickColor = dwClickColor;
	m_dwFontColor = dwFontColor;
	ResetBitmaps();
}

void CModiButton::SetStyle(int nRoundness, int nMoveDepth)
{
	m_nRoundness = nRoundness;
	m_nDepth = nMoveDepth;
	ResetBitmaps();
}

void CModiButton::SetFont(CString csFontFaceName, int nFontSize, COLORREF dwFontColor)
{
	m_csFontFaceName = csFontFaceName;
	m_nFontSize = nFontSize;
	m_dwFontColor = dwFontColor;
	ResetBitmaps();
}

void CModiButton::SetTextTopMargin(int nTextTopMargin)
{
	m_nTextTopMargin = nTextTopMargin;
	ResetBitmaps();
}


void CModiButton::GetRoundRectPath(CRect SrcRect, int nRadius, GraphicsPath& Path)
{
	int nX1 = SrcRect.left;
	int nY1 = SrcRect.top;
	int nX2 = SrcRect.right;
	int nY2 = SrcRect.bottom;
	int nDia = nRadius * 2;

	// 버튼의 모서리 4개에 대해서 라운드 처리가 들어간 경로 생성
	Path.AddArc(nX1, nY1, nDia, nDia, 180, 90);
	Path.AddArc(nX2 - nDia, nY1, nDia, nDia, 270, 90);
	Path.AddArc(nX2 - nDia, nY2 - nDia, nDia, nDia, 0, 90);
	Path.AddArc(nX1, nY2 - nDia, nDia, nDia, 90, 90);
	Path.CloseFigure();
}

void CModiButton::SetBorderPath(GraphicsPath &UpBtnBorderPath, GraphicsPath &DnBtnBorderPath, 
	CRect SrcRect, int nRadius, int nMoveDepth)
{
	CRect UpRect;
	CRect DnRect;

	if (nMoveDepth > 0) SrcRect.DeflateRect(nMoveDepth, nMoveDepth);
	else SrcRect.DeflateRect(m_nBorderLineSize, m_nBorderLineSize);
	
	UpRect = SrcRect;
	UpRect.OffsetRect(-nMoveDepth, -nMoveDepth);
	GetRoundRectPath(UpRect, nRadius, UpBtnBorderPath);

	if (nMoveDepth > 0)
	{
		DnRect = SrcRect;
		
		// 주의 - right, bottom 을 테두리 굵기 만큼 줄여줘야 테두리 라인이 잘리지 않음
		DnRect.OffsetRect(nMoveDepth, nMoveDepth);
		DnRect.right -= m_nBorderLineSize;
		DnRect.bottom -= m_nBorderLineSize;
		
		GetRoundRectPath(DnRect, nRadius, DnBtnBorderPath);
	}
	else
	{
		GetRoundRectPath(UpRect, nRadius, DnBtnBorderPath);
	}
}

void CModiButton::DrawBorder(Graphics* pDestGr, GraphicsPath* pBtnBorderPath, CRect BtnRect, COLORREF* pFaceColor, COLORREF* pBorderColor)
{
	Gdiplus::Rect DestRect(BtnRect.left, BtnRect.top, BtnRect.Width(), BtnRect.Height());

	Color BkColor;
	BkColor.SetFromCOLORREF(m_dwBkColor);
	Gdiplus::SolidBrush BkBrush(BkColor);

	Color FaceColor;
	Color BorderColor;
	Gdiplus::SolidBrush FaceBrush(Color(0, 0, 0));
	Gdiplus::SolidBrush BorderBrush(Color(0, 0, 0));

	if (pFaceColor == NULL) FaceColor.SetFromCOLORREF(m_dwFaceColor);
	else FaceColor.SetFromCOLORREF(*pFaceColor);

	FaceBrush.SetColor(FaceColor);

	if (pBorderColor == NULL) BorderColor.SetFromCOLORREF(m_dwBorderColor);
	else BorderColor.SetFromCOLORREF(*pBorderColor);

	BorderBrush.SetColor(BorderColor);

	Gdiplus::Pen BorderPen(&BorderBrush, (REAL)m_nBorderLineSize);
	

	pDestGr->FillRectangle(&BkBrush, DestRect);
	pDestGr->FillPath(&FaceBrush, pBtnBorderPath);
	pDestGr->DrawPath(&BorderPen, pBtnBorderPath);
}

void CModiButton::SetTextRect(CRect& UpBtnTextRect, CRect& DnBtnTextRect, CRect SrcRect, int nMoveDepth)
{
	SrcRect.DeflateRect(nMoveDepth, nMoveDepth);

	UpBtnTextRect = SrcRect;
	UpBtnTextRect.OffsetRect(-nMoveDepth, -nMoveDepth);
	
	// 버튼 수직방향 중앙 정렬을 수동으로 설정
	UpBtnTextRect.top += m_nTextTopMargin;
		
	if (nMoveDepth > 0)
	{
		DnBtnTextRect = SrcRect;
		DnBtnTextRect.OffsetRect(nMoveDepth, nMoveDepth);

		// 임시 - 버튼 수직방향 중앙 정렬 수동으로
		DnBtnTextRect.top += m_nTextTopMargin;
	}
	else
	{
		DnBtnTextRect = UpBtnTextRect;
	}
}

void CModiButton::DrawText(Graphics* pDestGr, CRect TextRect, CString csText)
{
	Color FontColor;
	FontColor.SetFromCOLORREF(m_dwFontColor);
	Gdiplus::SolidBrush FontBrush(FontColor);

	USES_CONVERSION;

	FontFamily gdiFontFamily(A2W(m_csFontFaceName));
	Gdiplus::Font gdiFont(&gdiFontFamily, (REAL)m_nFontSize, FontStyleRegular, UnitPixel);

	StringFormat StFormat;
	StFormat.SetAlignment(StringAlignmentCenter);
	StFormat.SetLineAlignment(StringAlignmentCenter);

	RectF DestRect((REAL)TextRect.left, (REAL)TextRect.top, (REAL)TextRect.Width(), (REAL)TextRect.Height());
	pDestGr->DrawString(A2W(csText), -1, &gdiFont, DestRect, &StFormat, &FontBrush);
}

void CModiButton::CreateBitImg(HDC hDC, CRect DestRect)
{
	Graphics DestGr(hDC);

	m_pUpBitImg = std::make_unique<Bitmap>(DestRect.Width(), DestRect.Height(), &DestGr);
	m_pDnBitImg = std::make_unique<Bitmap>(DestRect.Width(), DestRect.Height(), &DestGr);
	m_pDisableBitImg = std::make_unique<Bitmap>(DestRect.Width(), DestRect.Height(), &DestGr);

	Graphics UpBitImgMemGr(m_pUpBitImg.get());
	Graphics DnBitImgMemGr(m_pDnBitImg.get());
	Graphics DisableBitImgMemGr(m_pDisableBitImg.get());


#ifdef SMOOTH_MODE
	// 주의 - anti alias 모드 사용 시 잔상 남음
	UpBitImgMemGr.SetSmoothingMode(SmoothingModeAntiAlias);
	DnBitImgMemGr.SetSmoothingMode(SmoothingModeAntiAlias);
	DisableBitImgMemGr.SetSmoothingMode(SmoothingModeAntiAlias);
#endif

	GraphicsPath UpBtnBorderPath;
	GraphicsPath DnBtnBorderPath;
	SetBorderPath(UpBtnBorderPath, DnBtnBorderPath, DestRect, m_nRoundness, m_nDepth);
	
	DrawBorder(&UpBitImgMemGr, &UpBtnBorderPath, DestRect);
	DrawBorder(&DnBitImgMemGr, &DnBtnBorderPath, DestRect, &m_dwClickColor);
	DrawBorder(&DisableBitImgMemGr, &UpBtnBorderPath, DestRect, &m_dwDisableColor);

	CRect UpBtnTextRect;
	CRect DnBtnTextRect;
	SetTextRect(UpBtnTextRect, DnBtnTextRect, DestRect, m_nDepth);

	CString csCaption = "";
	GetWindowText(csCaption);
	DrawText(&UpBitImgMemGr, UpBtnTextRect, csCaption);
	DrawText(&DnBitImgMemGr, DnBtnTextRect, csCaption);
	DrawText(&DisableBitImgMemGr, UpBtnTextRect, csCaption);
}

HWND CModiButton::CreateBtn(CString csCaption, CWnd* pParent, UINT nDumyID)
{
	CWnd* pDumy = pParent->GetDlgItem(nDumyID);
	CRect DestRect;
	pDumy->GetWindowRect(DestRect);
	pParent->ScreenToClient(DestRect);
	pDumy->ShowWindow(SW_HIDE);

	BOOL bRet = Create(csCaption, WS_CHILD | WS_VISIBLE, DestRect, pParent, nDumyID);
	if (bRet == TRUE)
	{
		ModifyStyle(0, WS_CLIPCHILDREN);
		ShowWindow(SW_SHOW);

		return this->GetSafeHwnd();
	}
	else
	{
		return NULL;
	}
}

HWND CModiButton::CreateBtn(CWnd* pParent, UINT nDumyID)
{
	CWnd* pDumy = pParent->GetDlgItem(nDumyID);
	CRect DestRect;
	pDumy->GetWindowRect(DestRect);
	pParent->ScreenToClient(DestRect);
	
	CString csCaption = "";
	pDumy->GetWindowText(csCaption);
	
	pDumy->ShowWindow(SW_HIDE);

	BOOL bRet = Create(csCaption, WS_CHILD | WS_VISIBLE, DestRect, pParent, nDumyID);
	if (bRet == TRUE)
	{
		ModifyStyle(0, WS_CLIPCHILDREN);
		ShowWindow(SW_SHOW);

		return this->GetSafeHwnd();
	}
	else
	{
		return NULL;
	}
}

void CModiButton::PreSubclassWindow()
{
	SetButtonStyle(GetButtonStyle() | BS_OWNERDRAW);
	ModifyStyle(0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	CButton::PreSubclassWindow();
}

void CModiButton::OnPaint()
{
	CPaintDC dc(this); 


#ifdef SMOOTH_MODE
	CRect DestRect;
	GetWindowRect(DestRect);
	ScreenToClient(DestRect);

	if (m_pUpBitImg == nullptr)
	{
		CreateBitImg(dc.GetSafeHdc(), DestRect);
	}

	Graphics* pDestGr = new Graphics(dc.GetSafeHdc());
	
	Bitmap* pBkBitImg = new Bitmap(DestRect.Width(), DestRect.Height());
	Graphics* pMemGr = new Graphics(pBkBitImg);
		
	Gdiplus::Rect DrawRect(0, 0, DestRect.Width(), DestRect.Height());

	Color* pBkColor = new Color();
	pBkColor->SetFromCOLORREF(m_dwBkColor);
	Gdiplus::SolidBrush* pBkBrush = new Gdiplus::SolidBrush(*pBkColor);
	pMemGr->FillRectangle(pBkBrush, DrawRect);

	delete pBkBrush;
	delete pBkColor;

	if (IsWindowEnabled() == TRUE)
	{
		if (m_nState == STATUS_BTN_CLICK)
		{
			pMemGr->DrawImage(m_pDnBitImg.get(), DrawRect);
			//std::cout << "btn dn bitimg" << std::endl;
		}
		else
		{
			pMemGr->DrawImage(m_pUpBitImg.get(), DrawRect);
			//std::cout << "btn up bitimg" << std::endl;
		}
	}
	else
	{
		pMemGr->DrawImage(m_pDisableBitImg.get(), DrawRect);
		//std::cout << "btn disable bitimg\n" << std::endl;
	}

	CachedBitmap* pCaBitmap = new CachedBitmap(pBkBitImg, pDestGr);
	pDestGr->DrawCachedBitmap(pCaBitmap, 0, 0);

	pDestGr->Flush();
	delete pDestGr;

	pMemGr->Flush();
	delete pMemGr;

	delete pCaBitmap;
	delete pBkBitImg;
#else
	if (m_pUpBitImg == nullptr)
	{
		CRect DestRect;
		GetWindowRect(DestRect);
		ScreenToClient(DestRect);

		CreateBitImg(dc.GetSafeHdc(), DestRect);
	}

	Graphics* pDestGr = new Graphics(dc.GetSafeHdc());
	
	CachedBitmap* pCaBitmap = NULL;

	if (m_nState == STATUS_BTN_CLICK)
	{
		pCaBitmap = new CachedBitmap(m_pDnBitImg, pDestGr);
	}
	else
	{
		// STATUS_BTN_CLICKED
		pCaBitmap = new CachedBitmap(m_pUpBitImg, pDestGr);
	}

	pDestGr->DrawCachedBitmap(pCaBitmap, 0, 0);

	pDestGr->Flush();
	delete pDestGr;
	
	delete pCaBitmap;
#endif
}


void CModiButton::OnBnClicked()
{
	m_nState = STATUS_BTN_CLICKED;
	Invalidate(FALSE);

	UINT nID = GetDlgCtrlID();
	WPARAM wParam = MAKEWPARAM(nID, BN_CLICKED);
	GetParent()->SendMessage(WM_COMMAND, wParam, NULL);
}

void CModiButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_nState = STATUS_BTN_CLICK;
	Invalidate(FALSE);

	CButton::OnLButtonDown(nFlags, point);
}

void CModiButton::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	OnLButtonDown(nFlags, point);

	CButton::OnLButtonDblClk(nFlags, point);
}


BOOL CModiButton::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;

	//return CButton::OnEraseBkgnd(pDC);
}


void CModiButton::OnEnable(BOOL bEnable)
{
	CButton::OnEnable(bEnable);

	if (bEnable == TRUE)
	{
		// 깜빡이면서 뒷 배경 컨트롤 보이는 현상 방지
		ShowWindow(SW_HIDE);
		m_nState = STATUS_BTN_NONE;
		ShowWindow(SW_SHOW);
	}
	else
	{
		ShowWindow(SW_HIDE);
		m_nState = STATUS_BTN_DISABLE;
		ShowWindow(SW_SHOW);
	}

	//Invalidate(FALSE);
}


void CModiButton::OnMouseLeave()
{
	if (m_nState == STATUS_BTN_CLICK)
	{
		m_nState = STATUS_BTN_NONE;
		
		Invalidate(FALSE);
	}

	CButton::OnMouseLeave();
}
