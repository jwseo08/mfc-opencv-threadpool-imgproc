#pragma once
#include <afxext.h>
#include <memory>
#include "add_gdiplus.h"

#define STATUS_NONE					-1
#define STATUS_BTN_DEC_CLICK		0
#define STATUS_BTN_DEC_CLICKED		1
#define STATUS_BTN_INC_CLICK		2
#define STATUS_BTN_INC_CLICKED		3
#define STATUS_TRACK_CLICK			4
#define STATUS_TRACK_CLICKED		5
#define STATUS_THUMB_CLICK			6
#define STATUS_THUMB_CLICKED		7
#define STATUS_THUMB_MOVE			8
#define STATUS_WHEEL_MOVE_UP		9
#define STATUS_WHEEL_MOVE_DN		10

#define BTN_DEC						0		// 스크롤 값 감소 버튼 - 상단 버튼, 좌측 버튼
#define BTN_INC						1		// 스크롤 값 증가 버튼 - 하단 버튼, 우측 버튼

// 강제로 부모 윈도우에 vscroll 메시지 보냄
#define UM_VSCROLL		(WM_USER + 6000)

// 이미지 리스트의 스크롤바 기능 제공 클래스
class CModiScrollBar : public CFormView
{
public:
	// 명시를 위한 기본 생성자
	CModiScrollBar();

	// 실제 사용 생성자
	// nid - 스크롤바 리소스 아이디 - 실제는 폼뷰
	// dumy id - 스크롤바 위치를 잡기 위한 더미 컨트롤 아이디
	CModiScrollBar(UINT nID);
	CModiScrollBar(UINT nID, UINT nDumyID);
	virtual ~CModiScrollBar();

private:
	CBrush m_BkBrush;

private:
	UINT m_nTempleteID;
	UINT m_nDumyID;

public:
	int m_nScrollPos;
	
private:
	int m_nMaxRange;
	int m_nMinRange;

	int m_nSbStyle;     // 세로 스크롤인지, 가로 스크롤인지 SB_VERT, SB_HORZ
	int m_nBtnDepth;	// 스크롤바 눌리는 깊이 - 스크롤바 버튼 동작이 flat 인지 3d 인지


	int m_nBtnWidth;
	int m_nBtnHeight;
	int m_nThumbHeight;		// thumb 높이는 편의를 위해서 짝수로 강제함
	int m_nTrackWidth;
	int m_nBtnArrowMargin;
	int m_nBorderLineSize;
	int m_nRoundness;
	
private:
	COLORREF m_dwBkColor;
	COLORREF m_dwBtnFaceColor;
	COLORREF m_dwBtnClickColor;
	COLORREF m_dwBtnBorderColor;
	COLORREF m_dwBtnArrowColor;
	COLORREF m_dwThumbColor;
	COLORREF m_dwTrackColor;

private:
	std::unique_ptr<Bitmap> m_pBtnDecBitImg;		// 상단, 좌측 이동 버튼
	std::unique_ptr<Bitmap> m_pBtnDecClickBitImg;
	std::unique_ptr<Bitmap> m_pBtnIncBitImg;		// 하단, 우측 이동 버튼
	std::unique_ptr<Bitmap> m_pBtnIncClickBitImg;
	std::unique_ptr<Bitmap> m_pThumbBitImg;
	std::unique_ptr<Bitmap> m_pTrackBitImg;

private:
	CRect m_BtnDecRect;
	CRect m_BtnIncRect;
	CRect m_ThumbRect;
	CRect m_TrackRect;
	CRect m_TrackScrollRect;

private:
	int m_nStatus;
	int m_nPrevMousePosY;
	int m_nTrackClikedPosY;

	void ResetBitmaps();
	int GetAxisPoint(CPoint point) const;
	int GetRectAxisStart(const CRect& rect) const;
	int GetRectAxisEnd(const CRect& rect) const;
		
public:
	void InitVariable();
	void SetRange(int nMin, int nMax);
	int GetScrollBarStyle() const { return m_nSbStyle; }
	void SetColors(COLORREF dwBkColor, COLORREF dwBtnFaceColor, COLORREF dwBtnClickColor,
		COLORREF dwBtnBorderColor, COLORREF dwBtnArrowColor, COLORREF dwThumbColor,
		COLORREF dwTrackColor, BOOL bRedraw = TRUE);
	void SetMetrics(int nThumbSize, int nTrackWidth, int nArrowMargin,
		int nBorderLineSize, int nRoundness, BOOL bRedraw = TRUE);
	BOOL CreateScBar(CRect DestRect, int nSbStyle, CWnd* pParent);
	BOOL CreateScBar(int nSbStyle, CWnd* pParent);
	void CreateBitImg(HDC hDC, CRect DestRect);

	void GetRoundRectPath(CRect SrcRect, int nRadius, GraphicsPath& Path);
	void SetBorderPath(GraphicsPath& DestBorderPath, CRect SrcRect, int nRadius);
	void DrawBorder(Graphics* pDestGr, GraphicsPath* pBorderPath, CRect FaceRect,
		COLORREF* pFaceColor, COLORREF* pBorderColor);
	void DrawBtn(Bitmap* pDestBitImg, CRect BitImgRect, COLORREF dwFaceColor);
	void DrawBtnArrow(Bitmap* pDestBitImg, CRect BitImgRect, int nDirection);
	void DrawTrack(Bitmap* pDestBitImg, CRect BitImgRect);
	void DrawThumb(Bitmap* pDestBitImg, CRect BitImgRect);
		
public:
	int GetScrollPos();
	int SetScrollPos(int nPos, BOOL bReDraw = TRUE);

private:
	void SendScrollInfo();
	int ThumbRectToScrollPos(CRect ThumbRect, CRect TrackScrollRect);
	CRect ScrollPosToThumbRect(int nScrollPos, CRect TrackScrollRect);
	
public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	virtual void PreSubclassWindow();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};

