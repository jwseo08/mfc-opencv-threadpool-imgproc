#pragma once

#include <afxwin.h>
#include <memory>

// gdi plus 사용
#include "add_gdiplus.h"

// 버튼에 표시되는 텍스트의 폰트 정의
#ifndef FONT_FACENAME_ARIAL
#define FONT_FACENAME_ARIAL		"Arial"
#define FONT_FACENAME_BATANG	"Batang"
#define FONT_FACENAME_DOTUM		"Dotum"
#define FONT_FACENAME_GULIM		"Gulim"
#define FONT_FACENAME_MALGUN	"맑은 고딕"
#endif

// 버튼 상태 정의
#define STATUS_BTN_NONE			-1
#define STATUS_BTN_CLICK		0
#define STATUS_BTN_CLICKED		1
#define STATUS_BTN_DISABLE		2

// 버튼 색상 정보 구조체
struct TBtnColorInfo
{
	COLORREF dwBkg;
	COLORREF dwFace;
	COLORREF dwBorder;
	COLORREF dwClick;
	COLORREF dwDisable;
	COLORREF dwFont;

	TBtnColorInfo()
	{
		dwBkg = RGB(52, 52, 52);
		dwFace = RGB(109, 109, 109);
		dwBorder = RGB(255, 255, 255);
		dwClick = RGB(113, 171, 214);
		dwDisable = RGB(20, 20, 20);
		dwFont = RGB(0, 0, 0);
	}

	static TBtnColorInfo Accent(COLORREF dwAccent, COLORREF dwBkg = RGB(52, 52, 52),
		COLORREF dwFont = RGB(255, 255, 255));
};

// 버튼 기능 제공 클래스
class CModiButton : public CButton
{
public:
	CModiButton();
	CModiButton(ULONG_PTR* pGdiplusToken);
	virtual ~CModiButton();

private:
	ULONG_PTR* m_pGdiplusToken;
	
private:
	// 버튼 색상 - 배경, 전면, 테두리, 클릭, 비활성화 
	COLORREF m_dwBkColor;
	COLORREF m_dwFaceColor;
	COLORREF m_dwBorderColor;
	COLORREF m_dwClickColor;
	COLORREF m_dwDisableColor;

	// 클릭 시 들어감 정도, 테두리 라운드 처리 정도
	int m_nDepth;
	int m_nRoundness;
	
	// 버튼 상태에 따른 비트맵
	// 별도 비트맵을 사용하지 않고 gdi plus로 동적으로 생성
	std::unique_ptr<Bitmap> m_pUpBitImg;
	std::unique_ptr<Bitmap> m_pDnBitImg;
	std::unique_ptr<Bitmap> m_pDisableBitImg;

	// 버튼 텍스트와 테두리 표시 설정
	CString m_csFontFaceName;
	COLORREF m_dwFontColor;
	int m_nTextMargin;
	int m_nFontSize;
	int m_nTextTopMargin;
	int m_nBorderLineSize;

private:
	// 버튼 상태
	int m_nState;

private:
	void InitVariable();
	void ResetBitmaps(BOOL bRedraw = TRUE);

private:
	// 테두리 라운드 처리를 위한 그리기 path 생성, path 지정
	void GetRoundRectPath(CRect SrcRect, int nRadius, GraphicsPath& Path);
	void SetBorderPath(GraphicsPath &UpBtnBorderPath, GraphicsPath &DnBtnBorderPath, 
		CRect SrcRect, int nRadius, int nDepth);

	// 버튼 텍스트 표시 영역 지정
	void SetTextRect(CRect& UpBtnTextRect, CRect& DnBtnTextRect, CRect SrcRect, int nMoveDepth);

	// 테두리 그리기, 텍스트 표시
	void DrawBorder(Graphics* pDestGr, GraphicsPath* pBtnBorderPath, CRect BtnRect, COLORREF* pFaceColor = NULL, COLORREF* pBorderColor = NULL);
	void DrawText(Graphics* pDestGr, CRect TextRect, CString csText);

	// 버튼 본체에 지정하는 비트맵 생성
	void CreateBitImg(HDC hDC, CRect DestRect);

public:
	// 색상 지정 - 인자 중에서 nDepth가 1 이상이면 push 스타일, 0 이면 flat 스타일
	void SetColor(COLORREF dwBkColor, COLORREF dwFaceColor, COLORREF dwBorderColor, 
		COLORREF dwClickColor, COLORREF dwDisableColor);
	void SetColor(const TBtnColorInfo& tBtnColor, BOOL bRedraw = TRUE);
	// 자주 사용하는 face/click/font 세 색상만으로 간편 설정
	void SetColor(COLORREF dwFaceColor, COLORREF dwClickColor, COLORREF dwFontColor);
	
	// 버튼 색상 지정, 텍스트 폰트 지정, 텍스트 상단 여백 지정
	void SetStyle(int nRoundness, int nMoveDepth);
	void SetFont(CString csFontFaceName, int nFontSize, COLORREF dwFontColor);
	void SetTextTopMargin(int nTextTopMargin);

public:
	// 버튼 생성
	HWND CreateBtn(CString csCaption, CWnd* pParent, UINT nDumyID);
	HWND CreateBtn(CWnd* pParent, UINT nDumyID);

public:
	// 버튼 이벤트 처리 - 화면 갱신과 마우스 이벤트
	DECLARE_MESSAGE_MAP()
	virtual void PreSubclassWindow();
	afx_msg void OnPaint();
	afx_msg void OnBnClicked();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg void OnMouseLeave();
};

