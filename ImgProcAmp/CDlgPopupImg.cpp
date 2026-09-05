// CDlgPopupImg.cpp: 구현 파일
//

#include "pch.h"
#include "ImgProcAmp.h"
#include "afxdialogex.h"
#include "CDlgPopupImg.h"


// CDlgPopupImg 대화 상자

IMPLEMENT_DYNAMIC(CDlgPopupImg, CDialogEx)

CDlgPopupImg::CDlgPopupImg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_POPUP_IMG, pParent)
{
	m_pImgBox = nullptr;
	m_nDlgWidth = 0;
	m_nDlgHeight = 0;

	m_dwBkColor = RGB(52, 52, 52);
	m_BkBrush.CreateSolidBrush(m_dwBkColor);
}

CDlgPopupImg::~CDlgPopupImg()
{
	if (m_pImgBox != nullptr)
	{
		delete m_pImgBox;
	}

	m_BkBrush.DeleteObject();
}

void CDlgPopupImg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgPopupImg, CDialogEx)
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


void CDlgPopupImg::SetDlgSize(int nWidth, int nHeight)
{
	m_nDlgWidth = nWidth;
	m_nDlgHeight = nHeight;
}

void CDlgPopupImg::SetImg(Bitmap* pBitImg)
{
	m_pImgBox->SetImgData(pBitImg);
}

// CDlgPopupImg 메시지 처리기


BOOL CDlgPopupImg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.

	// 메인 다이얼로그 스타일 변경 - 깜빡임 방지
	//ModifyStyle(0, WS_CLIPCHILDREN);
	ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	// 메인 다이얼로그 크기 조정 - 임시 - 높이 안맞음
	SetWindowPos(NULL, 0, 0, m_nDlgWidth, m_nDlgHeight, SWP_NOMOVE);
	CRect DlgRect;
	GetClientRect(DlgRect);
	if ((DlgRect.Width() != m_nDlgWidth) || (DlgRect.Height() != m_nDlgHeight))
	{
		int nDlgWidth = m_nDlgWidth + (m_nDlgWidth - DlgRect.Width());
		int nDlgHeight = m_nDlgHeight + (m_nDlgHeight - DlgRect.Height());

		int nSysEdgeSize = GetSystemMetrics(SM_CYEDGE);
		int nSysTitleHeight = GetSystemMetrics(SM_CYCAPTION) + nSysEdgeSize;
		int nSysBorderWidth = GetSystemMetrics(SM_CXBORDER);
		int nSysBorderHeight = GetSystemMetrics(SM_CYBORDER);

		nDlgWidth -= nSysEdgeSize;
		nDlgHeight -= (nSysTitleHeight + nSysBorderHeight * 2);


		SetWindowPos(NULL, 0, 0, nDlgWidth, nDlgHeight, SWP_NOMOVE);
	}

	CRect ClRect;
	GetClientRect(ClRect);
	ClientToScreen(ClRect);
	GetDlgItem(IDC_DM_POPUP_IMG)->SetWindowPos(NULL, 0, 0, ClRect.Width(), ClRect.Height(), SWP_NOMOVE);

	m_pImgBox = new CFormImg(IDD_FORM_BASE, IDC_DM_POPUP_IMG);
	m_pImgBox->CreateFormImg(this);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void CDlgPopupImg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}


HBRUSH CDlgPopupImg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (nCtlColor == CTLCOLOR_DLG)
	{
		hbr = (HBRUSH)m_BkBrush;
		pDC->SetBkMode(TRANSPARENT);
	}

	return hbr;
}



void CDlgPopupImg::PostNcDestroy()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CDialogEx::PostNcDestroy();

	delete this;
}
