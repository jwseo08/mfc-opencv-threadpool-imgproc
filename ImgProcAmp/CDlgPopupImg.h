#pragma once
#include "afxdialogex.h"

#include "CFormImg.h"

// CDlgPopupImg 대화 상자

class CDlgPopupImg : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgPopupImg)

public:
	CDlgPopupImg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgPopupImg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_POPUP_IMG };
#endif


private:
	CFormImg* m_pImgBox;
	int m_nDlgWidth;
	int m_nDlgHeight;

	COLORREF m_dwBkColor;
	CBrush m_BkBrush;

public:
	void SetDlgSize(int nWidth, int nHeight);
	void SetImg(Bitmap* pBitImg);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual void PostNcDestroy();
};
