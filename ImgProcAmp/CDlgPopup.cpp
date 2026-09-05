// CDlgPopup.cpp: 구현 파일
//

#include "pch.h"
#include "ImgProcAmp.h"
#include "afxdialogex.h"
#include "CDlgPopup.h"
#include "CommonUtil.h"
#include <algorithm>

// CDlgPopup 대화 상자

IMPLEMENT_DYNAMIC(CDlgPopup, CDialogEx)

CDlgPopup::CDlgPopup(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_POPUP, pParent)
{
	m_nDlgWidth = 0;
	m_nDlgHeight = 0;

	m_dwBkColor = RGB(52, 52, 52);
	m_BkBrush.CreateSolidBrush(m_dwBkColor);

	m_pDisplayBitImg = nullptr;
	m_pBitImgFromFile = nullptr;
}

CDlgPopup::~CDlgPopup()
{
	delete m_pBitImgFromFile;
	m_pBitImgFromFile = nullptr;
	m_pDisplayBitImg = nullptr;

	m_BkBrush.DeleteObject();
}

void CDlgPopup::PostNcDestroy()
{
	CDialogEx::PostNcDestroy();
	delete this;
}

void CDlgPopup::OnOK()
{
	DestroyWindow();
}

void CDlgPopup::OnCancel()
{
	DestroyWindow();
}

void CDlgPopup::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgPopup, CDialogEx)
	ON_WM_PAINT()
END_MESSAGE_MAP()


// CDlgPopup 메시지 처리기

BOOL CDlgPopup::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	ResizeToImage();

	return TRUE;
}


void CDlgPopup::OnPaint()
{
	CPaintDC dc(this); 

	//-----------------------------------------

	CRect DestRect;
	GetClientRect(DestRect);

	// 메모리 gr 생성
	Graphics DestGr(dc.GetSafeHdc());

	Bitmap BackBitImg(DestRect.Width(), DestRect.Height(), &DestGr);
	Graphics MemGr(&BackBitImg);

	if (m_pDisplayBitImg != nullptr)
	{
		// 이미지가 있으면 바탕을 검은색으로
		Gdiplus::SolidBrush BlackBrush(Color(0, 0, 0));
		MemGr.FillRectangle(&BlackBrush, Gdiplus::Rect(0, 0, DestRect.Width(), DestRect.Height()));

		// 비율에 맞춰서 이미지 표시
		TRatioInfo tRatioInfo = GetImgRatioInfo(m_pDisplayBitImg, DestRect);
		MemGr.DrawImage(m_pDisplayBitImg, tRatioInfo.nLeft, tRatioInfo.nTop, tRatioInfo.nWidth, tRatioInfo.nHeight);
	}
	else
	{
		// 이미지가 없으면 배경을 검은 색으로
		Gdiplus::SolidBrush BlackBrush(Color(0, 0, 0));
		MemGr.FillRectangle(&BlackBrush, Gdiplus::Rect(0, 0, DestRect.Width(), DestRect.Height()));
	}

	// memgr 화면 표출
	CachedBitmap CaBitmap(&BackBitImg, &DestGr);
	DestGr.DrawCachedBitmap(&CaBitmap, 0, 0);
}


void CDlgPopup::SetDlgSize(int nWidth, int nHeight)
{
	m_nDlgWidth = nWidth;
	m_nDlgHeight = nHeight;

	if (GetSafeHwnd() != nullptr)
	{
		ResizeToImage();
	}
}

void CDlgPopup::SetImg(Bitmap* pBitImg, CString csImgPathFile)
{
	m_pDisplayBitImg = pBitImg;

	CString csFileName = "";

	if (csImgPathFile.GetLength() > 0)
	{
		csFileName = GetFileNameFromPathFile(csImgPathFile);
	}

	SetWindowText(csFileName);
	ResizeToImage();

	Invalidate(FALSE);
}


void CDlgPopup::SetImgData(CString csPathFile)
{
	USES_CONVERSION;

	delete m_pBitImgFromFile;
	m_pBitImgFromFile = new Bitmap(A2W(csPathFile));
	
	SetImg(m_pBitImgFromFile, csPathFile);
	Invalidate(FALSE);
}

void CDlgPopup::ResizeToImage()
{
	if (GetSafeHwnd() == nullptr || m_pDisplayBitImg == nullptr)
	{
		return;
	}

	const UINT nImgWidth = m_pDisplayBitImg->GetWidth();
	const UINT nImgHeight = m_pDisplayBitImg->GetHeight();
	if (nImgWidth == 0 || nImgHeight == 0) return;
	
	// dialog 최대 크기는 제목 표시줄과 테두리 포함 크기
	CRect rcNonClient(0, 0, 0, 0);
	::AdjustWindowRectEx(&rcNonClient, GetStyle(), FALSE, GetExStyle());
	const int nNonClientWidth = rcNonClient.Width();
	const int nNonClientHeight = rcNonClient.Height();
	const int nMaxClientWidth = std::max(1, MAX_DLG_SIZE - nNonClientWidth);
	const int nMaxClientHeight = std::max(1, MAX_DLG_SIZE - nNonClientHeight);

	double dScale = std::min((double)nMaxClientWidth / nImgWidth,
		(double)nMaxClientHeight / nImgHeight);

	// 작은 이미지는 확대 안함
	dScale = std::min(1.0, dScale);

	int nClientWidth = std::max(1, (int)(nImgWidth * dScale + 0.5));
	int nClientHeight = std::max(1, (int)(nImgHeight * dScale + 0.5));

	// set dlg size가 지정된 경우에도 이미지 비율을 유지하고 상한으로 사용
	if (m_nDlgWidth > 0 && m_nDlgHeight > 0)
	{
		const int nRequestedClientWidth = std::max(1, std::min(m_nDlgWidth, nMaxClientWidth));
		const int nRequestedClientHeight = std::max(1, std::min(m_nDlgHeight, nMaxClientHeight));
		
		dScale = std::min((double)nRequestedClientWidth / nImgWidth, (double)nRequestedClientHeight / nImgHeight);

		nClientWidth = std::max(1, (int)(nImgWidth * dScale + 0.5));
		nClientHeight = std::max(1, (int)(nImgHeight * dScale + 0.5));
	}

	CRect rcWindow(0, 0, nClientWidth, nClientHeight);
	::AdjustWindowRectEx(&rcWindow, GetStyle(), FALSE, GetExStyle());
	SetWindowPos(nullptr, 0, 0, rcWindow.Width(), rcWindow.Height(),
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

TRatioInfo CDlgPopup::GetImgRatioInfo(Bitmap* pDrawImg, CRect DestRect)
{
	int nOrgWidth = pDrawImg->GetWidth();
	int nOrgHeight = pDrawImg->GetHeight();

	int nDestWidth = DestRect.Width();
	int nDestHeight = DestRect.Height();

	double dRatioX = (double)nDestWidth / nOrgWidth;
	double dRatioY = (double)nDestHeight / nOrgHeight;

	double dRatio = 0.0;
	int nLeft = 0;
	int nTop = 0;
	int nWidth = 0;
	int nHeight = 0;

	nWidth = (int)(nOrgWidth * dRatioX);
	nHeight = (int)(nOrgHeight * dRatioX);

	if (nHeight > nDestHeight)
	{
		nWidth = (int)(nOrgWidth * dRatioY);
		nHeight = (int)(nOrgHeight * dRatioY);

		dRatio = dRatioY;
	}
	else
	{
		dRatio = dRatioX;
	}

	if (nWidth < nDestWidth)
	{
		nLeft = (int)((nDestWidth - nWidth) / 2.0);
	}

	if (nHeight < nDestHeight)
	{
		nTop = (int)((nDestHeight - nHeight) / 2.0);
	}

	TRatioInfo tImgRatioInfo;

	tImgRatioInfo.dRatio = dRatio;
	tImgRatioInfo.nLeft = nLeft;
	tImgRatioInfo.nTop = nTop;
	tImgRatioInfo.nWidth = nWidth;
	tImgRatioInfo.nHeight = nHeight;

	return tImgRatioInfo;
}
