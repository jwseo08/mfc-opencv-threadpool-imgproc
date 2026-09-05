#pragma once
#include <afxext.h>

#include <iostream>
#include <vector>
#include <list>
#include <set>

#include "add_gdiplus.h"
#include "opencv2/opencv.hpp"
#include "CModiScrollBar.h"

#include "def.h"
#include "CDlgPopup.h"

// 이미지와 텍스트 표시 영역 설정값 - 화면 분할 개수, 이미지 표시 영역, 텍스트 표시 영역...
struct TPartArea
{
	CRect BaseRect;
	CRect ImgRect;
	CRect TextRect;
	int nCurImgInfoIndex;

	TPartArea()
	{
		BaseRect.SetRect(0, 0, 0, 0);
		ImgRect.SetRect(0, 0, 0, 0);
		TextRect.SetRect(0, 0, 0, 0);
		nCurImgInfoIndex = -1;
	}

};

// 스크롤 이동 설정값 - 최대값, 기본 이동 단계값, 버튼 클릭시 단계값...
struct TScrollInfo
{
	int nMaxRange;
	int nBaseStep;
	int nBtnStep;
	int nThumbStep;
	int nWheelStep;
	int nKeyStep;

	TScrollInfo()
	{
		nMaxRange = 0;
		nBaseStep = 0;
		nBtnStep = 0;
		nThumbStep = 0;
		nWheelStep = 0;
		nKeyStep = 0;
	}
};

// 화면 표시 형태 설정값 - 배경색, 비어있는 영역 색상, 선택 색상...
struct TDisplayStyle
{
	COLORREF dwBkColor;
	COLORREF dwEmptyBkColor;
	COLORREF dwSelectBkColor;
	COLORREF dwSplitLineColor;
	int nSplitLineSize;
	COLORREF dwFontColor;
	COLORREF dwHighLightFontColor;
	CString csFontFaceName;
	int nFontSize;
	StringAlignment eTextAlignHorz;
	StringAlignment eTextAlignVert;

	TDisplayStyle()
	{
		dwBkColor = RGB(0, 0, 0);
		dwEmptyBkColor = RGB(128, 128, 128);
		dwSelectBkColor = RGB(0, 0, 255);
		dwSplitLineColor = RGB(255, 255, 255);
		nSplitLineSize = 1;
		dwFontColor = RGB(255, 255, 255);
		dwHighLightFontColor = RGB(255, 255, 0);
		csFontFaceName = "Dotum";
		nFontSize = 12;
		eTextAlignHorz = StringAlignmentCenter;
		eTextAlignVert = StringAlignmentCenter;
	}
};

// 드래그
typedef std::vector <CWnd*> VecDragTaret;			// 드래그 대상 창 리스트 - 끌어다놓는 창 리스트

// 스크롤바 스탭
#define SCROLL_STEP		100

// 리스트에 표시되는 텍스트 폰트
#ifndef FONT_FACENAME_ARIAL
#define FONT_FACENAME_ARIAL		"Arial"
#define FONT_FACENAME_BATANG	"Batang"
#define FONT_FACENAME_DOTUM		"Dotum"
#define FONT_FACENAME_GULIM		"Gulim"
#define FONT_FACENAME_MALGUN	"맑은 고딕"
#endif

// 리스트에 표시되는 텍스트 정렬 형식
#define TEXT_ALIGN_LEFT		StringAlignmentNear
#define TEXT_ALIGN_CENTER	StringAlignmentCenter
#define TEXT_ALIGN_RIGHT	StringAlignmentFar
#define TEXT_ALIGN_TOP		StringAlignmentNear
#define TEXT_ALIGN_MIDDLE	StringAlignmentCenter
#define TEXT_ALIGN_BOTTOM	StringAlignmentFar

// 리스트에서 항목 클릭 시 parent 에게 보내는 알림 메시지
#define UM_IMGLIST_CLICKED (WM_USER + 2000)

// 타겟 창으로 보내는 드래그 드랍 메시지
#define UM_DRAG_DROP (WM_USER + 2100)

// 스크롤바 모양 - 일반, modi 스크롤바 중에서 사용 선택 - 0 이면 일반, 1 이면 modi
#define USE_NORMAL_BAR		0
#define USE_MODI_BAR		1

// 드래그 상태
#define STATUS_DRAG_START	1
#define STATUS_DRAG_MOVE	2
#define STATUS_DRAG_END		3

// 리스트 형식으로 이미지 보여주기 클래스
class CFormImgList : public CFormView
{
public:
	// 기본 생성자
	CFormImgList();

	// 실제 사용 생성자 
	// nid - 리스트 기반이 되는 폼 리소스 아이디
	// sc bar id - modi 스크롤바 리소스 아이디
	// dumy id - 리스트 위치를 잡기위한 더미 컨트롤 아이디
	CFormImgList(UINT nID, UINT nScBarID);
	CFormImgList(UINT nID, UINT nScBarID, int nDumy);

	virtual ~CFormImgList();

private:
	COLORREF m_dwBkColor;
	CBrush m_BkBrush;

private:
	UINT m_nTempleteID;			// 리스트 창 폼 리소스 아이디
	UINT m_nScBarTempleteID;	// modi scroll bar 리소스 아이디
	UINT m_nDumyID;				// 리스트 창의 위치를 잡기위한 더미 컨트롤 아이디
	int m_nScBarShape;			// 일반, modi 스크롤바 중에서 사용 선택 - 0 이면 일반, 1 이면 modi
	int m_nScBarType;			// SB_VERT or SB_HORZ
	int m_nScBarSize;
	CScrollBar m_ctrlScBar;		  // 일반 스크롤바
	CModiScrollBar* m_pModiScBar; // modi 스크롤바 - 커스텀 스크롤바

private:
	int m_nPartAreaCnt;			        // 화면 분할 수
	TPartArea* m_ptPartArea;	        // 분할된 화면 각각의 영역 좌표
	std::vector<TImgInfo> m_vecImgInfo;	// 로드된 이미지 정보 리스트
	int m_nSelectedPartArea;	// 분할 화면 중 현재 선택된 화면 인덱스
	int m_nSelectedImgInfoIndex;// 이미지 중에서 현재 선택된 이미지의 인덱스
	TScrollInfo m_tScrollInfo;	// 스크롤 범위와 스탭

private:
	TDisplayStyle m_tDisplayStyle; // 화면 표시 색상, 폰트, 텍스트 형태, 구분선 형태

private:
	void InitVariable(); // 설정값 지정
	void SetScrollRangeAndStep(int nImgCnt, int nBaseStep);	// 스크롤 범위와 step 설정

public:
	// 창 생성
	BOOL CreateFormImgList(CRect DestRect, int nScBarType, int nScBarSize, int nPartAreaCnt, CWnd* pParent);
	BOOL CreateFormImgList(int nScBarType, int nScBarSize, int nPartAreaCnt, CWnd* pParent);

	// 창 분할 개수 설정 - 창에서 한번에 보이는 이미지 개수와 동일
	void SetPartArea(int nPartAreaCnt, int nScrollBarSize, int nScBarType);	

	// 리스트에 로드할 이미지들의 정보 설정 - 보여지는 이미지 리스트는 이미지 파일 이름 기반으로 이미지 로드
	void SetImgList(const std::vector<std::string>& vImgList, bool redraw = false);

	// 리스트에 보여줘야 하는 이미지 정보를 지정하고 이미지를 추가할 때마다 한칸씩 이동
	void SetImgListAndScroll(CString pathFile);

	// 표시 가능한 항목 수를 넘은 경우 현재 위치에서 이미지 한칸 이동
	bool ScrollOneItem(bool bForward = true, BOOL bRedraw = TRUE);

	// 로드한 이미지 정보 리스트 제거
	void ClearImgInfoList(bool bRedraw = false);

	// 색상, 폰트... 설정
	void SetDisplayColor(COLORREF dwBkColor, COLORREF dwEmptyBkColor, COLORREF dwSelectBkColor);
	void SetFontStyle(COLORREF dwFontColor, CString csFontFaceName, int nFontSize, int nTextAlignHorz, int nTextAlignVert);
	void SetSplitLineStyle(COLORREF dwLineColor, int nLineSize);
	void SetDisplayStyle(const TDisplayStyle& style, BOOL bRedraw = TRUE);
	const TDisplayStyle& GetDisplayStyle() const { return m_tDisplayStyle; }
	void SetTextColor(COLORREF dwFontColor, COLORREF dwSelectedFontColor, BOOL bRedraw = TRUE);
	bool SetItemText(int nIndex, const CString& csText, BOOL bRedraw = TRUE);
	CString GetItemText(int nIndex) const;
	void SetScrollBarColor(COLORREF dwBkColor, COLORREF dwBtnFaceColor, COLORREF dwBtnClickColor,
		COLORREF dwBtnBorderColor, COLORREF dwBtnArrowColor, COLORREF dwThumbColor,
		COLORREF dwTrackColor, BOOL bRedraw = TRUE);

	// 드래그 - 임시
	void AddDragTarget(CWnd* pWnd);
	
private:
	// 드래그 - 임시
	int m_nDragStatus;
	TRACKMOUSEEVENT m_tMouseEvent;
	HCURSOR m_hOldCursor;
	HCURSOR m_hDragCursor;
	CString m_csDragPathFileName;

	VecDragTaret m_vecDragTarget;
	void ClearDragTarget();
	CWnd* DragPtHit(CPoint point);

	HDROP CreateFileDropHandle(LPCTSTR filePath);
	void SendDropFileMsg(CWnd* pTargetWnd, CString csPathFileName);

public:
	CString RemoveSelectImg();

public:
	void SetTarget(CWnd* pWnd);
	std::vector<TImgInfo>* GetVecImgInfo();
	std::vector<TImgInfo> GetImgList();
	bool IsEmpty();
	void ResetList();
	void ClearTargetWnd();

	// 선택 활성화 비활성화
	void SetSelectEnable(bool bEnable, int nSelectIndex = -1);

	// 로드된 이미지 수 반환
	int GetImgCount();

	// 화면에 표시되는 이미지 수 반환
	int GetVisibleItemCount() const { return m_nPartAreaCnt; }
	

private:
	// 이미지를 클릭 했을 때 이미지를 보여주는 창 지정
	CWnd* m_pTargetWnd;
	void SetImgInfoToTarget(TImgInfo tImgInfo);

	// 이미지를 더블클릭 했을 때 팝업되는 이미지 보여주기 창
	CDlgPopup* m_pDlgPop;

	// 클릭과 더블클릭으로 사진 선택 비활성화
	// 컨트롤 enable 대신 사용
	bool m_bSelectEnable;
	
	// 방향키 스크롤
	void ArrowKeyScroll(int zDelta);

	// 방향 키로 스크롤 하기 위해서 입력된 방향 키 저장
	std::set<UINT> m_ProcessedKeys;

public:
	// 윈도우 메시지 함수와 이벤트 함수
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();           // 창 소멸 시 정리가 필요한 요소를 정리에 사용
	virtual void PreSubclassWindow();   // 창 생성 전에 창 스타일 지정에 사용
	afx_msg void OnPaint();             // 이미지, 텍스트 표출에 사용
	virtual void OnInitialUpdate();
	virtual BOOL PreTranslateMessage(MSG* pMsg);  // 방향 키로 리스트 이동을 위한 키 입력 처리에 사용
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor); // 구성요소 색상 변경에 사용
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	afx_msg LRESULT OnUmVScroll(WPARAM wParam, LPARAM lParam);  // 스크롤 동작를 위한 사용자 메시지 처리에 사용
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	
};

