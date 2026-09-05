#pragma once

#include <afxext.h>
#include <vector>
#include "add_gdiplus.h"
#include "opencv2/opencv.hpp"

#include "def.h"
#include "CDlgPopup.h"


// 표시할 이미지 가로세로 크기와 비율
struct TImgRatioInfo
{
	double dRatio;
	int nLeft;
	int nTop;
	int nWidth;
	int nHeight;

	TImgRatioInfo()
	{
		dRatio = 0.0;
		nLeft = 0;
		nTop = 0;
		nWidth = 0;
		nHeight = 0;
	}

};


// 합성 의심 부분 표시 영역
typedef struct _REGION_INFO
{
	int nVertexCnt;
	POINT tVertex[8];	// 최대 8각형

	_REGION_INFO()
	{
		nVertexCnt = 0;
		for (int i = 0; i < 8; i++)
		{
			tVertex[i].x = 0;
			tVertex[i].y = 0;
		}
	}

} TRegionInfo;

// 합성 의심 부분 표시 영역 스타일
typedef struct _REGION_STYLE
{
	COLORREF dwColor;

	// 투명도 0.0 ~ 1.0
	float fOpacity;

	_REGION_STYLE()
	{
		dwColor = RGB(255, 0, 0);
		fOpacity = (float)0.3;
	}

} TRegionStyle;


// 랜드마크 좌료 리스트
typedef std::vector <POINT> VecPoint;

// 의심영역 좌표 리스트
typedef std::vector<TRegionInfo> VecRegion;

// 디모핑 이미지 여러장
typedef std::vector<cv::Mat> VecMatDemorph;
typedef std::vector<float> VecDemorphAlpha;


// 이미지 저장 형식
#define FORMAT_NONE	-1
#define FORMAT_JPG	0
#define FORMAT_PNG	1
#define FORMAT_BMP	2

// 이미지가 로드되었음을 부모 창에 알림
#define UM_FORM_IMG_DATA_SET		(WM_USER + 3500)

// 이미지 클래스 클릭 시 선택한 이미지를 보여주는 창 클래스
class CFormImg : public CFormView
{
public:
	// 명시를 위한 생성자
	CFormImg();	
	
	// 실제 사용 생성자 - nid 폼 리소스 아이디, dumi id 폼 위치를 잡기위한 더미 컨트롤 아이디
	CFormImg(UINT nID);
	CFormImg(UINT nID, UINT nDumyID);

	virtual ~CFormImg();

private:
	COLORREF m_dwBkColor;
	CBrush m_BkBrush;

private:
	void InitVariable();

private:
	UINT m_nTempleteID;   // 폼 리소스 아이디
	UINT m_nDumyID;       // 폼 위치를 잡기위한 더미 컨트롤의 아이디 

	// 표시할 이미지 할당을 파일이름으로 받은 경우 사용
	CString m_csPathFileName;

	// 표시할 이미지 할당을 opencv mat로 받은 경우 사용
	cv::Mat m_MatImg;

	cv::Mat m_MatImgBf;

	// 표시할 이미지
	Bitmap* m_pDisplayBitImg;				
	Bitmap* m_pBitImgMat;
	Bitmap* m_pBitImgFile;
	Bitmap* m_pBitImgCImg;

	// 이미지 정보
	TImgInfo m_tImgInfo;
		
public:
	void PreSubclassWindow();
	
	// 창 생성
	BOOL CreateFormImg(CRect DestRect, CWnd* pParent);
	BOOL CreateFormImg(CWnd* pParent);

public:
	// 표시할 이미지 세팅
	void SetImgData(Bitmap* pBitImg);
	void SetImgData(cv::Mat* pMatImg);
	void SetImgData(CString csPathFile);
	void SetImgData(CImage* pImg);
	Bitmap* GetImgData();
	CString GetImgPathFile();
	
	void SetImgInfo(TImgInfo tImgInfo);
	TImgInfo GetImgInfo();

	cv::Mat GetRetImg(Bitmap* pBitmap);
	cv::Mat CaptureWindowToMat();// (HWND hwnd);

private:
	// 내부적으로만 사용
	void SetImgFromMat(cv::Mat MatImg);

private:
	// 표시할 영역 크기 대비 이미지 비율 계산
	TImgRatioInfo GetImgRatioInfo(Bitmap* pDrawImg, CRect DestRect);

	// 현재 로드된 이미지가 있는지 여부
	bool IsImgEmpty();

public:
	// 이미지 제거
	void ClearImgInfo(bool bRedraw = true);

	// 현재 보여지고 있는 이미지를 파일로 저장
	bool SaveImg(CString csPathFile);

private:
	int GetEncoderClsid(WCHAR* format, CLSID* pClsid);
	bool SaveBitmapToFile(Bitmap* pBitmap, CString csPathFile, CString csFormat);

public:
	CDlgPopup* m_pDlgPop;
	void ImgPop(int nWidth, int nHeight);
	cv::Mat GetBfMatImg();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};

