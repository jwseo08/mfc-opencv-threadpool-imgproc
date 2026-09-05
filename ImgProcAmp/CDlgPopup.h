#pragma once
#include "afxdialogex.h"
#include "add_gdiplus.h"

struct TRatioInfo
{
	double dRatio;
	int nLeft;
	int nTop;
	int nWidth;
	int nHeight;

	TRatioInfo()
	{
		dRatio = 0.0;
		nLeft = 0;
		nTop = 0;
		nWidth = 0;
		nHeight = 0;
	}
};

// 이미지 리스트 더블클릭 시 확대 이미지 표시 팝업 창 클래스
class CDlgPopup : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgPopup)

public:
	CDlgPopup(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgPopup();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_POPUP };
#endif

private:
	static const int MAX_DLG_SIZE = 800; // dialog 가로세로 최대 크기

	int m_nDlgWidth;	// dialog 가로 크기
	int m_nDlgHeight;   // dialog 세로 크기

	COLORREF m_dwBkColor;  // dialog 배경색
	CBrush m_BkBrush;      // 배경색 적용을 위한 브러시

	Bitmap* m_pDisplayBitImg;   // dialog에 표시할 이미지 비트맵
	Bitmap* m_pBitImgFromFile;  // dialog에 표시할 이미지 파일

	void ResizeToImage();

public:
	void SetDlgSize(int nWidth, int nHeight);
	void SetImg(Bitmap* pBitImg, CString csImgPathFile);
	void SetImgData(CString csPathFile);
	TRatioInfo GetImgRatioInfo(Bitmap* pDrawImg, CRect DestRect);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual void OnOK();
	virtual void OnCancel();
	virtual void PostNcDestroy();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
};
