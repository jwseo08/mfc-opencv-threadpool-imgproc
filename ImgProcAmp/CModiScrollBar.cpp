#include "pch.h"
#include "CModiScrollBar.h"
#include <iostream>
#include <algorithm>

#include "def.h"

#define loop_once_start		do
#define loop_once_exit		break
#define loop_once_end		while(false)

#define THUMB_CIRCLE

CModiScrollBar::CModiScrollBar() : CFormView(UINT(0))
{
	m_nTempleteID = 0;
	m_nDumyID = 0;

	InitVariable();
}

CModiScrollBar::CModiScrollBar(UINT nID) : CFormView(nID)
{
	m_nTempleteID = nID;
	m_nDumyID = 0;

	InitVariable();
}

CModiScrollBar::CModiScrollBar(UINT nID, UINT nDumyID) : CFormView(nID)
{
	m_nTempleteID = nID;
	m_nDumyID = nDumyID;

	InitVariable();
}

CModiScrollBar::~CModiScrollBar()
{
	// unique ptr 로 변경
	/*
	delete m_pBtnDecBitImg;
	delete m_pBtnIncBitImg;
	delete m_pBtnDecClickBitImg;
	delete m_pBtnIncClickBitImg;
	delete m_pThumbBitImg;
	delete m_pTrackBitImg;
	*/

	m_BkBrush.DeleteObject();
}


BEGIN_MESSAGE_MAP(CModiScrollBar, CFormView)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


void CModiScrollBar::InitVariable()
{
	m_nMaxRange = 0;
	m_nMinRange = 0;
	m_nScrollPos = 0;
	m_nSbStyle = SB_VERT;

	m_nBtnWidth = 0;
	m_nBtnHeight = 0;
	m_nThumbHeight = 14;
	m_nTrackWidth = 2;
	m_nBtnArrowMargin = 6;
	m_nBorderLineSize = 1;
	m_nRoundness = 5;

	m_dwBkColor = COLOR_MINT_GRAY; 
	m_dwBtnFaceColor = RGB(109, 109, 109);
	m_dwBtnClickColor = RGB(113, 171, 214);
	m_dwBtnBorderColor = RGB(255, 255, 255);
	m_dwBtnArrowColor = RGB(255, 255, 255);
	m_dwThumbColor = RGB(255, 255, 255);
	m_dwTrackColor = RGB(255, 255, 255);

	m_nStatus = STATUS_NONE;

	m_nPrevMousePosY = 0;
	m_nTrackClikedPosY = 0;

	m_pBtnDecBitImg = nullptr;
	m_pBtnDecClickBitImg = nullptr;
	m_pBtnIncBitImg = nullptr;
	m_pBtnIncClickBitImg = nullptr;
	m_pThumbBitImg = nullptr;
	m_pTrackBitImg = nullptr;

	m_nBtnDepth = 1;

	m_BkBrush.CreateSolidBrush(m_dwBkColor);
}

void CModiScrollBar::ResetBitmaps()
{
	m_pBtnDecBitImg.reset();
	m_pBtnDecClickBitImg.reset();
	m_pBtnIncBitImg.reset();
	m_pBtnIncClickBitImg.reset();
	m_pThumbBitImg.reset();
	m_pTrackBitImg.reset();
}

int CModiScrollBar::GetAxisPoint(CPoint point) const
{
	return (m_nSbStyle == SB_HORZ) ? point.x : point.y;
}

int CModiScrollBar::GetRectAxisStart(const CRect& rect) const
{
	return (m_nSbStyle == SB_HORZ) ? rect.left : rect.top;
}

int CModiScrollBar::GetRectAxisEnd(const CRect& rect) const
{
	return (m_nSbStyle == SB_HORZ) ? rect.right : rect.bottom;
}

void CModiScrollBar::SetColors(COLORREF dwBkColor, COLORREF dwBtnFaceColor,
	COLORREF dwBtnClickColor, COLORREF dwBtnBorderColor, COLORREF dwBtnArrowColor,
	COLORREF dwThumbColor, COLORREF dwTrackColor, BOOL bRedraw)
{
	m_dwBkColor = dwBkColor;
	m_dwBtnFaceColor = dwBtnFaceColor;
	m_dwBtnClickColor = dwBtnClickColor;
	m_dwBtnBorderColor = dwBtnBorderColor;
	m_dwBtnArrowColor = dwBtnArrowColor;
	m_dwThumbColor = dwThumbColor;
	m_dwTrackColor = dwTrackColor;

	m_BkBrush.DeleteObject();
	m_BkBrush.CreateSolidBrush(m_dwBkColor);
	ResetBitmaps();
	if (bRedraw && GetSafeHwnd() != NULL) Invalidate(FALSE);
}

void CModiScrollBar::SetMetrics(int nThumbSize, int nTrackWidth, int nArrowMargin,
	int nBorderLineSize, int nRoundness, BOOL bRedraw)
{
	m_nThumbHeight = (std::max)(2, nThumbSize);
	m_nTrackWidth = (std::max)(1, nTrackWidth);
	m_nBtnArrowMargin = (std::max)(1, nArrowMargin);
	m_nBorderLineSize = (std::max)(0, nBorderLineSize);
	m_nRoundness = (std::max)(1, nRoundness);
	ResetBitmaps();
	if (bRedraw && GetSafeHwnd() != NULL) Invalidate(FALSE);
}

void CModiScrollBar::SetRange(int nMin, int nMax)
{
	m_nMinRange = nMin;
	m_nMaxRange = (std::max)(nMin, nMax);
	SetScrollPos(m_nScrollPos, FALSE);
}

BOOL CModiScrollBar::CreateScBar(CRect DestRect, int nSbStyle, CWnd* pParent)
{
	m_nSbStyle = (nSbStyle == SB_HORZ) ? SB_HORZ : SB_VERT;
	
	std::cout << "dest rect=left, top, right, bottom=" 
		<< DestRect.left << "," << DestRect.top << "," 
		<< DestRect.right << "," << DestRect.bottom << "\n";

	BOOL bRet = Create(NULL, NULL, WS_CHILD | WS_VISIBLE, DestRect, pParent, m_nTempleteID, NULL);
	if (bRet == TRUE)
	{
		ModifyStyle(0, WS_CLIPCHILDREN);
		SetScrollSizes(MM_TEXT, CSize(0, 0));
		ShowWindow(SW_SHOW);
	}

	return bRet;
}

BOOL CModiScrollBar::CreateScBar(int nSbStyle, CWnd* pParent)
{
	m_nSbStyle = (nSbStyle == SB_HORZ) ? SB_HORZ : SB_VERT;
	CWnd* pDumy = pParent->GetDlgItem(m_nDumyID);
	CRect DestRect;
	pDumy->GetWindowRect(DestRect);
	pParent->ScreenToClient(DestRect);
	pDumy->ShowWindow(SW_HIDE);

	std::cout << "dest rect=left, top, right, bottom="
		<< DestRect.left << "," << DestRect.top << ","
		<< DestRect.right << "," << DestRect.bottom << "\n";

	BOOL bRet = Create(NULL, NULL, WS_CHILD | WS_VISIBLE, DestRect, pParent, m_nTempleteID, NULL);
	if (bRet == TRUE)
	{
		ModifyStyle(0, WS_CLIPCHILDREN);
		SetScrollSizes(MM_TEXT, CSize(0, 0));
		ShowWindow(SW_SHOW);
	}

	return bRet;
}

void CModiScrollBar::CreateBitImg(HDC hDC, CRect DestRect)
{
	Graphics DestGr(hDC);
	DestRect.SetRect(0, 0, DestRect.Width(), DestRect.Height());
	
	std::cout << "oncreate rect=left, top, right, bottom, style="
		<< DestRect.left << "," << DestRect.top << ","
		<< DestRect.right << "," << DestRect.bottom << "," 
		<< m_nSbStyle << "\n";

	// 스크롤바 방향에 따라서 스크롤바 화살표 버튼, 트래커, 썸 좌표와 크기 지정
	if (m_nThumbHeight % 2 != 0) ++m_nThumbHeight;
	if (m_nSbStyle == SB_HORZ)
	{
		const int nButtonSize = (std::min)(DestRect.Height(), DestRect.Width() / 2);
		m_BtnDecRect.SetRect(DestRect.left, DestRect.top, DestRect.left + nButtonSize, DestRect.bottom);
		m_BtnIncRect.SetRect(DestRect.right - nButtonSize, DestRect.top, DestRect.right, DestRect.bottom);
		m_TrackRect.SetRect(m_BtnDecRect.right, DestRect.top, m_BtnIncRect.left, DestRect.bottom);
		m_ThumbRect.SetRect(m_TrackRect.left, m_TrackRect.top,
			(std::min)(m_TrackRect.right, m_TrackRect.left + m_nThumbHeight), m_TrackRect.bottom);
	}
	else
	{
		const int nButtonSize = (std::min)(DestRect.Width(), DestRect.Height() / 2);
		m_BtnDecRect.SetRect(DestRect.left, DestRect.top, DestRect.right, DestRect.top + nButtonSize);
		m_BtnIncRect.SetRect(DestRect.left, DestRect.bottom - nButtonSize, DestRect.right, DestRect.bottom);
		m_TrackRect.SetRect(DestRect.left, m_BtnDecRect.bottom, DestRect.right, m_BtnIncRect.top);
		m_ThumbRect.SetRect(m_TrackRect.left, m_TrackRect.top,
			m_TrackRect.right, (std::min)(m_TrackRect.bottom, m_TrackRect.top + m_nThumbHeight));
	}
		
	// 스크롤바 구성요소에 해당하는 비트맵 생성
	m_pBtnDecBitImg = std::make_unique<Bitmap>(m_BtnDecRect.Width(), m_BtnDecRect.Height(), &DestGr);
	m_pBtnIncBitImg = std::make_unique<Bitmap>(m_BtnIncRect.Width(), m_BtnIncRect.Height(), &DestGr);
	m_pBtnDecClickBitImg = std::make_unique<Bitmap>(m_BtnDecRect.Width(), m_BtnDecRect.Height(), &DestGr);
	m_pBtnIncClickBitImg = std::make_unique<Bitmap>(m_BtnIncRect.Width(), m_BtnIncRect.Height(), &DestGr);
	m_pThumbBitImg = std::make_unique<Bitmap>(m_ThumbRect.Width(), m_ThumbRect.Height(), &DestGr);
	m_pTrackBitImg = std::make_unique<Bitmap>(m_TrackRect.Width(), m_TrackRect.Height(), &DestGr);

	// 생성된 비트맵에 구성요소에 해당하는 모양을 만듬

	// 상단 버튼 일반 상태
	DrawBtn(m_pBtnDecBitImg.get(), m_BtnDecRect, m_dwBtnFaceColor);
	DrawBtnArrow(m_pBtnDecBitImg.get(), m_BtnDecRect, BTN_DEC);

	DrawBtn(m_pBtnDecClickBitImg.get(), m_BtnDecRect, m_dwBtnClickColor);
	DrawBtnArrow(m_pBtnDecClickBitImg.get(), m_BtnDecRect, BTN_DEC);

	// 하단 버튼 일반 상태
	DrawBtn(m_pBtnIncBitImg.get(), m_BtnIncRect, m_dwBtnFaceColor);
	DrawBtnArrow(m_pBtnIncBitImg.get(), m_BtnIncRect, BTN_INC);

	DrawBtn(m_pBtnIncClickBitImg.get(), m_BtnIncRect, m_dwBtnClickColor);
	DrawBtnArrow(m_pBtnIncClickBitImg.get(), m_BtnIncRect, BTN_INC);

#ifdef THUMB_CIRCLE  // thumb 를 원형으로, 아니면 라운드 모서리 사각형으로
	DrawThumb(m_pThumbBitImg.get(), m_ThumbRect);
#else	
	DrawBtn(m_pThumbBitImg, m_ThumbRect, m_dwBtnFaceColor);	// m_dwThumbColor
#endif	

	DrawTrack(m_pTrackBitImg.get(), m_TrackRect);

	// 단순화를 위해서 일단 막음
	// thumb 가 그려지는 크기 때문에 track 영역을 줄임 
	//m_TrackRect.DeflateRect(0, m_nThumbHeight / 2);

	// 위 내용을 아래로 대체
	m_TrackScrollRect = m_TrackRect;
	if (m_nSbStyle == SB_HORZ)
		m_TrackScrollRect.DeflateRect((int)(m_nThumbHeight / 2.0 + 0.5), 0);
	else
		m_TrackScrollRect.DeflateRect(0, (int)(m_nThumbHeight / 2.0 + 0.5));
	m_ThumbRect = ScrollPosToThumbRect(m_nScrollPos, m_TrackScrollRect);
}

void CModiScrollBar::DrawBtn(Bitmap* pDestBitImg, CRect BitImgRect, COLORREF dwFaceColor)
{
	Graphics MemGr(pDestBitImg);
	MemGr.SetSmoothingMode(SmoothingModeAntiAlias);

	CRect FaceRect;
	FaceRect.SetRect(0, 0, BitImgRect.Width(), BitImgRect.Height());
		
	GraphicsPath BorderPath;
	SetBorderPath(BorderPath, FaceRect, m_nRoundness);

	DrawBorder(&MemGr, &BorderPath, FaceRect, &dwFaceColor, &m_dwBtnBorderColor);
}

void CModiScrollBar::DrawBtnArrow(Bitmap* pDestBitImg, CRect BitImgRect, int nDirection)
{
	Graphics MemGr(pDestBitImg);

	Color BtnArrowColor;
	BtnArrowColor.SetFromCOLORREF(m_dwBtnArrowColor);
	Gdiplus::SolidBrush BtnArrowBrush(BtnArrowColor);

	CRect FaceRect;
	FaceRect.SetRect(0, 0, BitImgRect.Width(), BitImgRect.Height());

	if (m_nSbStyle == SB_HORZ && nDirection == BTN_DEC)
	{
		Gdiplus::Point BtnArrowPt[3] = {
			Point(FaceRect.right - m_nBtnArrowMargin, FaceRect.top + m_nBtnArrowMargin),
			Point(FaceRect.left + m_nBtnArrowMargin, FaceRect.CenterPoint().y),
			Point(FaceRect.right - m_nBtnArrowMargin, FaceRect.bottom - m_nBtnArrowMargin),
		};
		MemGr.FillPolygon(&BtnArrowBrush, BtnArrowPt, 3);
	}
	else if (m_nSbStyle == SB_HORZ)
	{
		Gdiplus::Point BtnArrowPt[3] = {
			Point(FaceRect.left + m_nBtnArrowMargin, FaceRect.top + m_nBtnArrowMargin),
			Point(FaceRect.right - m_nBtnArrowMargin, FaceRect.CenterPoint().y),
			Point(FaceRect.left + m_nBtnArrowMargin, FaceRect.bottom - m_nBtnArrowMargin),
		};
		MemGr.FillPolygon(&BtnArrowBrush, BtnArrowPt, 3);
	}
	else if (nDirection == BTN_DEC)
	{
		Gdiplus::Point BtnArrowPt[3] = {
			Point(FaceRect.left + m_nBtnArrowMargin, FaceRect.bottom - m_nBtnArrowMargin),
			Point(FaceRect.CenterPoint().x, FaceRect.top + m_nBtnArrowMargin),
			Point(FaceRect.right - m_nBtnArrowMargin, FaceRect.bottom - m_nBtnArrowMargin),
			};
		MemGr.FillPolygon(&BtnArrowBrush, BtnArrowPt, 3);
	}
	else
	{
		Gdiplus::Point BtnArrowPt[3] = {
			Point(FaceRect.left + m_nBtnArrowMargin, FaceRect.top + m_nBtnArrowMargin),
			Point(FaceRect.right - m_nBtnArrowMargin, FaceRect.top + m_nBtnArrowMargin),
			Point(FaceRect.CenterPoint().x, FaceRect.bottom - m_nBtnArrowMargin),
			};
		MemGr.FillPolygon(&BtnArrowBrush, BtnArrowPt, 3);
	}
}

void CModiScrollBar::DrawTrack(Bitmap* pDestBitImg, CRect BitImgRect)
{
	Graphics MemGr(pDestBitImg);
	
	Color FaceColor;
	FaceColor.SetFromCOLORREF(m_dwTrackColor);
	Gdiplus::SolidBrush FaceBrush(FaceColor);
		
	// 트렉바 라인 넓이는 편의상 짝수로 강제함
	if (m_nTrackWidth % 2 != 0) m_nTrackWidth += 1;
		
	Gdiplus::Rect FaceRect;
	if (m_nSbStyle == SB_HORZ)
	{
		FaceRect = Gdiplus::Rect(0,
			(int)(BitImgRect.Height() / 2.0 + 0.5) - (m_nTrackWidth / 2),
			m_TrackRect.Width(), m_nTrackWidth);
	}
	else
	{
		FaceRect = Gdiplus::Rect(
			(int)(BitImgRect.Width() / 2.0 + 0.5) - (m_nTrackWidth / 2),
			0, m_nTrackWidth, m_TrackRect.Height());
	}

	MemGr.FillRectangle(&FaceBrush, FaceRect);
}

void CModiScrollBar::DrawThumb(Bitmap* pDestBitImg, CRect BitImgRect)
{
	Graphics MemGr(pDestBitImg);

	Color FaceColor;
	FaceColor.SetFromCOLORREF(m_dwThumbColor);
	Gdiplus::SolidBrush FaceBrush(FaceColor);

	const int nDiameter = (std::min)(m_nThumbHeight,
		(std::min)(BitImgRect.Width(), BitImgRect.Height()));
	Gdiplus::Rect FaceRect(
		(int)((BitImgRect.Width() - nDiameter) / 2.0),
		(int)((BitImgRect.Height() - nDiameter) / 2.0),
		nDiameter, nDiameter);

	MemGr.SetSmoothingMode(SmoothingModeAntiAlias);
	MemGr.FillEllipse(&FaceBrush, FaceRect);
}

void CModiScrollBar::SendScrollInfo()
{
	// 주의 - 빈 리스트면 움직임 방지
	if (m_nMaxRange == 0) return;

	UINT nSbCode = 0;
	
	switch (m_nStatus)
	{
	case STATUS_BTN_DEC_CLICKED :
		nSbCode = (m_nSbStyle == SB_HORZ) ? SB_LINELEFT : SB_LINEUP;
		break;

	case STATUS_BTN_INC_CLICKED :
		nSbCode = (m_nSbStyle == SB_HORZ) ? SB_LINERIGHT : SB_LINEDOWN;
		break;

	case STATUS_TRACK_CLICKED :
		if (m_nTrackClikedPosY < GetRectAxisStart(m_ThumbRect))
		{
			nSbCode = (m_nSbStyle == SB_HORZ) ? SB_PAGELEFT : SB_PAGEUP;
		}
		else if (m_nTrackClikedPosY > GetRectAxisEnd(m_ThumbRect))
		{
			nSbCode = (m_nSbStyle == SB_HORZ) ? SB_PAGERIGHT : SB_PAGEDOWN;
		}
		else
		{
			;
		}

		//nSbCode = SB_THUMBPOSITION;
		break;

	case STATUS_THUMB_CLICK:
	//case STATUS_THUMB_MOVE :
		nSbCode = SB_THUMBTRACK;
		break;

	case STATUS_THUMB_CLICKED :
		nSbCode = SB_THUMBPOSITION;
		break;

	case STATUS_WHEEL_MOVE_UP :
		nSbCode = (m_nSbStyle == SB_HORZ) ? SB_PAGELEFT : SB_PAGEUP;
		break;

	case STATUS_WHEEL_MOVE_DN:
		nSbCode = (m_nSbStyle == SB_HORZ) ? SB_PAGERIGHT : SB_PAGEDOWN;
		break;

	default :
		;
	}

	CWnd* pParent = GetParent();
	HWND hWnd = GetSafeHwnd();
	pParent->PostMessage(UM_VSCROLL, MAKEWPARAM(nSbCode, m_nScrollPos), (LPARAM)hWnd);
}

// thumb rect 위치를 기눈으로 scroll pos 계산
int CModiScrollBar::ThumbRectToScrollPos(CRect ThumbRect, CRect TrackScrollRect)
{
	const int nStart = GetRectAxisStart(TrackScrollRect);
	const int nEnd = GetRectAxisEnd(TrackScrollRect);
	const int nCenter = (m_nSbStyle == SB_HORZ) ? ThumbRect.CenterPoint().x : ThumbRect.CenterPoint().y;
	if (nCenter <= nStart) return m_nMinRange;
	if (nCenter >= nEnd) return m_nMaxRange;
	if (nEnd <= nStart || m_nMaxRange <= m_nMinRange) return m_nMinRange;

	const double dRatio = (nCenter - nStart) / (double)(nEnd - nStart);
	return m_nMinRange + (int)(dRatio * (m_nMaxRange - m_nMinRange) + 0.5);
}

// set scroll pos 로 설정된 scroll pos 에 따라서 thumb rect 계산
CRect CModiScrollBar::ScrollPosToThumbRect(int nScrollPos, CRect TrackScrollRect)
{
	CRect ThumbRect(0, 0, 0, 0);
	const int nStart = GetRectAxisStart(TrackScrollRect);
	const int nEnd = GetRectAxisEnd(TrackScrollRect);
	const int nRange = m_nMaxRange - m_nMinRange;
	const double dRatio = (nRange > 0) ? (nScrollPos - m_nMinRange) / (double)nRange : 0.0;
	const int nCenter = nStart + (int)((nEnd - nStart) * dRatio + 0.5);
	const int nHalf = m_nThumbHeight / 2;

	if (m_nSbStyle == SB_HORZ)
		ThumbRect.SetRect(nCenter - nHalf, m_TrackRect.top, nCenter + nHalf, m_TrackRect.bottom);
	else
		ThumbRect.SetRect(m_TrackRect.left, nCenter - nHalf, m_TrackRect.right, nCenter + nHalf);

	return ThumbRect;
}

int CModiScrollBar::GetScrollPos()
{
	return m_nScrollPos;
}

int CModiScrollBar::SetScrollPos(int nPos, BOOL bReDraw)
{
	if (nPos < m_nMinRange)
	{
		nPos = m_nMinRange;
	}
	else if (nPos > m_nMaxRange)
	{
		nPos = m_nMaxRange;
	}
	else
	{
		;
	}

	int nPrevScrollPos = m_nScrollPos;
	m_nScrollPos = nPos;

	m_ThumbRect = ScrollPosToThumbRect(nPos, m_TrackScrollRect);
	if (bReDraw && GetSafeHwnd() != NULL) Invalidate(FALSE);

	return nPrevScrollPos;
}

void CModiScrollBar::GetRoundRectPath(CRect SrcRect, int nRadius, GraphicsPath& Path)
{
	int nX1 = SrcRect.left;
	int nY1 = SrcRect.top;
	int nX2 = SrcRect.right;
	int nY2 = SrcRect.bottom;
	int nDia = nRadius * 2;

	Path.AddArc(nX1, nY1, nDia, nDia, 180, 90);
	Path.AddArc(nX2 - nDia, nY1, nDia, nDia, 270, 90);
	Path.AddArc(nX2 - nDia, nY2 - nDia, nDia, nDia, 0, 90);
	Path.AddArc(nX1, nY2 - nDia, nDia, nDia, 90, 90);

	Path.CloseFigure();
}

void CModiScrollBar::SetBorderPath(GraphicsPath& DestBorderPath, CRect SrcRect, int nRadius)
{
	SrcRect.DeflateRect(m_nBorderLineSize, m_nBorderLineSize);

	GetRoundRectPath(SrcRect, nRadius, DestBorderPath);
}

void CModiScrollBar::DrawBorder(Graphics* pDestGr, GraphicsPath* pBorderPath, CRect FaceRect, 
	COLORREF* pFaceColor, COLORREF* pBorderColor)
{
	Gdiplus::Rect DestRect(FaceRect.left, FaceRect.top, FaceRect.Width(), FaceRect.Height());

	Color BkColor;
	BkColor.SetFromCOLORREF(m_dwBkColor);
	Gdiplus::SolidBrush BkBrush(BkColor);

	Color FaceColor;
	Color BorderColor;
	Gdiplus::SolidBrush FaceBrush(Color(0, 0, 0));
	Gdiplus::SolidBrush BorderBrush(Color(0, 0, 0));

	if (pFaceColor == NULL) FaceColor.SetFromCOLORREF(m_dwBtnFaceColor);
	else FaceColor.SetFromCOLORREF(*pFaceColor);

	FaceBrush.SetColor(FaceColor);

	if (pBorderColor == NULL) BorderColor.SetFromCOLORREF(m_dwBtnBorderColor);
	else BorderColor.SetFromCOLORREF(*pBorderColor);

	BorderBrush.SetColor(BorderColor);

	Gdiplus::Pen BorderPen(&BorderBrush, (REAL)m_nBorderLineSize);


	pDestGr->FillRectangle(&BkBrush, DestRect);
	pDestGr->FillPath(&FaceBrush, pBorderPath);
	pDestGr->DrawPath(&BorderPen, pBorderPath);
}

void CModiScrollBar::OnPaint()
{
	CPaintDC dc(this);
	CRect DestRect;
	GetClientRect(DestRect);

	if (m_pTrackBitImg == nullptr || m_BtnIncRect.right != DestRect.right || m_BtnIncRect.bottom != DestRect.bottom)
	{
		ResetBitmaps();
		CreateBitImg(dc.GetSafeHdc(), DestRect);

		// 주의 - 보여주기용
		//SetScrollPos(0);
	}

	Graphics* pDestGr = new Graphics(dc.GetSafeHdc());

	// 배경색 채움
	Bitmap* pBkBitImg = new Bitmap(DestRect.Width(), DestRect.Height());
	Graphics* pMemGr = new Graphics(pBkBitImg);
	Color* pBkColor = new Color();
	pBkColor->SetFromCOLORREF(m_dwBkColor);
	Gdiplus::SolidBrush* pBkBrush = new SolidBrush(*pBkColor);
	pMemGr->FillRectangle(pBkBrush, Gdiplus::Rect(0, 0, DestRect.Width(), DestRect.Height()));

	delete pBkBrush;
	delete pBkColor;

	// 트랙 그리기
	Gdiplus::Rect TrackRect(m_TrackRect.left, m_TrackRect.top, m_TrackRect.Width(), m_TrackRect.Height());
	pMemGr->DrawImage(m_pTrackBitImg.get(), TrackRect);

	// 상단 버튼 그리기
	Gdiplus::Rect BtnDecRect(m_BtnDecRect.left, m_BtnDecRect.top, m_BtnDecRect.Width(), m_BtnDecRect.Height());
	if ((m_nStatus == STATUS_BTN_DEC_CLICK) && (m_nMaxRange > 0))
	{
		pMemGr->DrawImage(m_pBtnDecClickBitImg.get(), BtnDecRect);
	}
	else
	{
		pMemGr->DrawImage(m_pBtnDecBitImg.get(), BtnDecRect);
	}

	// thumb 그리기
	Gdiplus::Rect ThumbRect(m_ThumbRect.left, m_ThumbRect.top, m_ThumbRect.Width(), m_ThumbRect.Height());
	pMemGr->DrawImage(m_pThumbBitImg.get(), ThumbRect);
		
	// 하단 버튼 그리기 
	Gdiplus::Rect BtnIncRect(m_BtnIncRect.left, m_BtnIncRect.top, m_BtnIncRect.Width(), m_BtnIncRect.Height());
	if ((m_nStatus == STATUS_BTN_INC_CLICK) && (m_nMaxRange > 0))
	{
		pMemGr->DrawImage(m_pBtnIncClickBitImg.get(), BtnIncRect);
	}
	else
	{
		pMemGr->DrawImage(m_pBtnIncBitImg.get(), BtnIncRect);
	}

	// 부드러운 전환을 위해서 cache bitmap 적용
	CachedBitmap* pCaBitmap = new CachedBitmap(pBkBitImg, pDestGr);
	pDestGr->DrawCachedBitmap(pCaBitmap, 0, 0);

	// 작업 후 메모리 정리
	pDestGr->Flush();
	delete pDestGr;
	
	pMemGr->Flush();
	delete pMemGr;
	
	delete pCaBitmap;
	delete pBkBitImg;
}

void CModiScrollBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	// 테스트
	SetFocus();

	bool bClick = false;
	loop_once_start
	{
		if (m_BtnDecRect.PtInRect(point) == TRUE)
		{
			bClick = true;
			m_nStatus = STATUS_BTN_DEC_CLICK;
			loop_once_exit;
		}

		if (m_BtnIncRect.PtInRect(point) == TRUE)
		{
			bClick = true;
			m_nStatus = STATUS_BTN_INC_CLICK;
			loop_once_exit;
		}

		if (m_ThumbRect.PtInRect(point) == TRUE)
		{
			bClick = true;
			m_nStatus = STATUS_THUMB_CLICK;
			m_nPrevMousePosY = GetAxisPoint(point);
			SetCapture();
			loop_once_exit;
		}

		if (m_TrackRect.PtInRect(point) == TRUE)
		{
			bClick = true;
			m_nStatus = STATUS_TRACK_CLICK;
			loop_once_exit;
		}
	}
	loop_once_end;


	if (bClick == true)
	{
		// 버튼 이미지 갱신을 위해서 다시 그림 - 나머지 구성요소는 set scroll pos 를 통해서 갱신
		if ((m_nStatus == STATUS_BTN_DEC_CLICK) || (m_nStatus == STATUS_BTN_INC_CLICK))
		{
			Invalidate(FALSE);
		}
	}

	CFormView::OnLButtonDown(nFlags, point);
}


void CModiScrollBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	bool bClicked = false;
	loop_once_start
	{
		if ((m_BtnDecRect.PtInRect(point) == TRUE) && (m_nStatus == STATUS_BTN_DEC_CLICK))
		{
			bClicked = true;
			m_nStatus = STATUS_BTN_DEC_CLICKED;
			loop_once_exit;
		}

		if ((m_BtnIncRect.PtInRect(point) == TRUE) && (m_nStatus == STATUS_BTN_INC_CLICK))
		{
			bClicked = true;
			m_nStatus = STATUS_BTN_INC_CLICKED;
			loop_once_exit;
		}

		if (m_nStatus == STATUS_THUMB_CLICK)
		{
			bClicked = true;
			m_nStatus = STATUS_THUMB_CLICKED;
			ReleaseCapture();
			
			loop_once_exit;
		}

		if ((m_TrackRect.PtInRect(point) == TRUE) && (m_nStatus == STATUS_TRACK_CLICK))
		{
			bClicked = true;
			m_nStatus = STATUS_TRACK_CLICKED;
			m_nPrevMousePosY = GetAxisPoint(point);
			m_nTrackClikedPosY = GetAxisPoint(point);

			loop_once_exit;
		}
	}
	loop_once_end;

	if (bClicked == true)
	{
		SendScrollInfo();
	}
	
	CFormView::OnLButtonUp(nFlags, point);
}


void CModiScrollBar::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_nStatus == STATUS_THUMB_CLICK)
	{
		const int nAxis = (std::max)(GetRectAxisStart(m_TrackScrollRect),
			(std::min)(GetAxisPoint(point), GetRectAxisEnd(m_TrackScrollRect)));
		const int nHalf = m_nThumbHeight / 2;
		if (m_nSbStyle == SB_HORZ)
			m_ThumbRect.SetRect(nAxis - nHalf, m_TrackRect.top, nAxis + nHalf, m_TrackRect.bottom);
		else
			m_ThumbRect.SetRect(m_TrackRect.left, nAxis - nHalf, m_TrackRect.right, nAxis + nHalf);
		
		m_nScrollPos = ThumbRectToScrollPos(m_ThumbRect, m_TrackScrollRect);
		
		//m_nStatus = STATUS_THUMB_MOVE;

		SendScrollInfo();
		Invalidate(FALSE);
	}
	
	CFormView::OnMouseMove(nFlags, point);
}


BOOL CModiScrollBar::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	GetParent()->SendMessage(WM_MOUSEWHEEL, MAKEWPARAM(0, zDelta), NULL);
	
	return CFormView::OnMouseWheel(nFlags, zDelta, pt);
	
}


void CModiScrollBar::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	std::cout << "double click\n";

	OnLButtonDown(nFlags, point);
	//OnLButtonUp(nFlags, point);

	CFormView::OnLButtonDblClk(nFlags, point);
}


void CModiScrollBar::PreSubclassWindow()
{
	ModifyStyle(0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	CFormView::PreSubclassWindow();
}


HBRUSH CModiScrollBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CFormView::OnCtlColor(pDC, pWnd, nCtlColor);

	if (nCtlColor == CTLCOLOR_DLG)
	{
		hbr = (HBRUSH)m_BkBrush;
		pDC->SetBkMode(TRANSPARENT);
	}

	return hbr;
}

