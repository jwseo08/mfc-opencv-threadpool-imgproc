#include "pch.h"
#include "CFormImg.h"

#include <iostream>
#include <algorithm>

#include "CommonUtil.h"

#define IDD_DLG_POP		140

CFormImg::CFormImg() : CFormView(UINT(0))
{
	m_nTempleteID = 0;
	m_nDumyID = 0;
		
	InitVariable();
}

CFormImg::CFormImg(UINT nID) : CFormView(nID)
{
	m_nTempleteID = nID;
	m_nDumyID = 0;

	InitVariable();
}

CFormImg::CFormImg(UINT nID, UINT nDumyID) : CFormView(nID)
{
	m_nTempleteID = nID;
	m_nDumyID = nDumyID;

	InitVariable();
}

CFormImg::~CFormImg()
{
	m_BkBrush.DeleteObject();

	if (m_pDlgPop != nullptr)
	{
		//m_pDlgPop->EndDialog(IDOK);
		//delete m_pDlgPop;
	}
}


BEGIN_MESSAGE_MAP(CFormImg, CFormView)
	ON_WM_PAINT()
	ON_WM_DROPFILES()
	ON_WM_CTLCOLOR()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


void CFormImg::PreSubclassWindow()
{
	ModifyStyle(0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	ModifyStyleEx(0, WS_EX_ACCEPTFILES);

	CFormView::PreSubclassWindow();
}

void CFormImg::InitVariable()
{
	m_pDisplayBitImg = nullptr;
	m_pBitImgMat = nullptr;
	m_pBitImgFile = nullptr;
	m_pBitImgCImg = nullptr;

	m_csPathFileName = "";

	m_dwBkColor = RGB(0, 0, 0);
	m_BkBrush.CreateSolidBrush(m_dwBkColor);

	m_pDlgPop = nullptr;
}

BOOL CFormImg::CreateFormImg(CRect DestRect, CWnd* pParent)
{
	BOOL bRet = Create("", "", WS_CHILD | WS_VISIBLE, DestRect, pParent, m_nTempleteID, NULL);
	//BOOL bRet = Create("", "", WS_CHILD | WS_VISIBLE | WS_BORDER, DestRect, pParent, m_nTempleteID, NULL);
	if (bRet == TRUE)
	{
		SetScrollSizes(MM_TEXT, CSize(0, 0));
		ShowWindow(SW_SHOW);

		DragAcceptFiles(TRUE);
	}

	return bRet;
}

BOOL CFormImg::CreateFormImg(CWnd* pParent)
{
	CWnd* pDumy = pParent->GetDlgItem(m_nDumyID);
	CRect DestRect;
	pDumy->GetWindowRect(DestRect);
	pParent->ScreenToClient(DestRect);
	pDumy->ShowWindow(SW_HIDE);

	BOOL bRet = Create("", "", WS_CHILD | WS_VISIBLE, DestRect, pParent, m_nTempleteID, NULL);
	//BOOL bRet = Create("", "", WS_CHILD | WS_VISIBLE | WS_BORDER, DestRect, pParent, m_nTempleteID, NULL);
	if (bRet == TRUE)
	{
		ModifyStyle(0, WS_CLIPCHILDREN);
		SetScrollSizes(MM_TEXT, CSize(0, 0));
		ShowWindow(SW_SHOW);

		DragAcceptFiles(TRUE);
	}
	
	return bRet;
}

void CFormImg::SetImgData(Bitmap* pBitImg)
{
	m_pDisplayBitImg = pBitImg;
	Invalidate(FALSE);

	GetParent()->SendMessage(UM_FORM_IMG_DATA_SET, 0, (LPARAM)this);
}

void CFormImg::SetImgData(cv::Mat* pMatImg)
{
	ClearImgInfo();

	cv::cvtColor(*pMatImg, m_MatImg, cv::COLOR_BGR2BGRA);
	m_pBitImgMat = new Bitmap((INT)m_MatImg.size().width,
		(INT)m_MatImg.size().height,
		(INT)m_MatImg.step,
		PixelFormat32bppARGB,
		m_MatImg.data);

	SetImgData(m_pBitImgMat);
}

void CFormImg::SetImgData(CString csPathFile)
{
	ClearImgInfo();
	//ClearImgAndDrawInfo();

	m_csPathFileName = csPathFile;

	USES_CONVERSION;
	m_pBitImgFile = new Bitmap(A2W(csPathFile));
	SetImgData(m_pBitImgFile);
}

void CFormImg::SetImgData(CImage* pImg)
{
	ClearImgInfo();

	HBITMAP hBitmap = (HBITMAP)(*pImg);
	m_pBitImgCImg = new Bitmap(hBitmap, NULL);
	SetImgData(m_pBitImgCImg);
}

void CFormImg::SetImgInfo(TImgInfo tImgInfo)
{
	ClearImgInfo();

	m_tImgInfo = tImgInfo;
	m_csPathFileName = tImgInfo.csPathFileName;

	USES_CONVERSION;
	m_pBitImgFile = new Bitmap(A2W(tImgInfo.csPathFileName));

	SetImgData(m_pBitImgFile);
}

Bitmap* CFormImg::GetImgData()
{
	return m_pDisplayBitImg;
}

CString CFormImg::GetImgPathFile()
{
	return m_csPathFileName;
}

TImgInfo CFormImg::GetImgInfo()
{
	return m_tImgInfo;
}



TImgRatioInfo CFormImg::GetImgRatioInfo(Bitmap* pDrawImg, CRect DestRect)
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

	TImgRatioInfo tImgRatioInfo;

	tImgRatioInfo.dRatio = dRatio;
	tImgRatioInfo.nLeft = nLeft;
	tImgRatioInfo.nTop = nTop;
	tImgRatioInfo.nWidth = nWidth;
	tImgRatioInfo.nHeight = nHeight;

	return tImgRatioInfo;
}

void CFormImg::ClearImgInfo(bool bRedraw)
{
	if (m_pBitImgMat != nullptr)
	{
		delete m_pBitImgMat;
		m_pBitImgMat = nullptr;
	}

	if (m_pBitImgCImg != nullptr)
	{
		delete m_pBitImgCImg;
		m_pBitImgCImg = nullptr;
	}

	if (m_pBitImgFile != nullptr)
	{
		delete m_pBitImgFile;
		m_pBitImgFile = nullptr;
	}

	m_pDisplayBitImg = nullptr;

	// test
	if (m_MatImg.empty() == false)
	{
		m_MatImg.release();
	}

	m_csPathFileName = "";

	if (bRedraw == true)
	{
		Invalidate(FALSE);
	}
}

bool CFormImg::IsImgEmpty()
{
	if (m_pDisplayBitImg == nullptr)
	{
		return true;
	}
	else
	{
		return false;
	}
}












//-------------------------------------------------------------

void CFormImg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	
	CRect DestRect;
	GetWindowRect(DestRect);

	// 메모리 gr 생성
	Graphics DestGr(dc.GetSafeHdc());
	//DestGr.SetSmoothingMode(SmoothingModeAntiAlias);

	Bitmap BackBitImg(DestRect.Width(), DestRect.Height(), &DestGr);
	Graphics MemGr(&BackBitImg);
	
	// 주의 - smooth 모드일 때 좌측과 상단에 태두리 생김 - 임시로 막음
	//MemGr.SetSmoothingMode(SmoothingModeAntiAlias);

	if (m_pDisplayBitImg != nullptr)
	{
		// 이미지가 있으면 바탕을 검은색으로
		Gdiplus::SolidBrush BlackBrush(Color(0, 0, 0));
		MemGr.FillRectangle(&BlackBrush, Gdiplus::Rect(0, 0, DestRect.Width(), DestRect.Height()));

		// 비율에 맞춰서 이미지 표시
		TImgRatioInfo tRatioInfo = GetImgRatioInfo(m_pDisplayBitImg, DestRect);
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

	// report
	/*if (m_bShowRegion == true)
	{
		m_MatImgBf = GetRetImg(&BackBitImg);
	}*/
}

cv::Mat CFormImg::GetBfMatImg()
{
	return m_MatImgBf;
}

cv::Mat CFormImg::GetRetImg(Bitmap* pBitmap)
{
	int width = pBitmap->GetWidth();
	int height = pBitmap->GetHeight();

	// GDI+의 비트맵 데이터를 잠금
	Gdiplus::Rect rect(0, 0, width, height);
	Gdiplus::BitmapData bitmapData;

	// 비트맵 데이터를 읽기 모드로 잠금
	if (pBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat24bppRGB, &bitmapData) != Gdiplus::Ok) {
		std::cerr << "Failed to lock bitmap data." << std::endl;
		return cv::Mat(); // 실패 시 빈 cv::Mat 반환
	}

	// OpenCV cv::Mat 생성
	cv::Mat mat(height, width, CV_8UC3, bitmapData.Scan0, bitmapData.Stride);

	// GDI+ 비트맵 데이터를 해제
	pBitmap->UnlockBits(&bitmapData);

	// GDI+는 BGR 형식, OpenCV에서 RGB로 변환 필요
	cv::Mat matRGB;
	cv::cvtColor(mat, matRGB, cv::COLOR_BGR2RGB);

	return matRGB.clone(); // clone()으로 안전하게 복사하여 반환
}

cv::Mat CFormImg::CaptureWindowToMat()//(HWND hwnd) 
{
	HWND hwnd = this->GetSafeHwnd();

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

	HDC hdcWindow = ::GetDC(hwnd);
	HDC hdcMemDC = CreateCompatibleDC(hdcWindow);

	RECT rcClient;
	::GetClientRect(hwnd, &rcClient);
	int width = rcClient.right;
	int height = rcClient.bottom;

	HBITMAP hbmScreen = CreateCompatibleBitmap(hdcWindow, width, height);
	SelectObject(hdcMemDC, hbmScreen);

	BitBlt(hdcMemDC, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);

	Gdiplus::Bitmap bitmap(hbmScreen, nullptr);

	Gdiplus::Rect rect(0, 0, width, height);
	Gdiplus::BitmapData bitmapData;

	cv::Mat mat;
	if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat24bppRGB, &bitmapData) == Gdiplus::Ok) {
		mat = cv::Mat(height, width, CV_8UC3, bitmapData.Scan0, bitmapData.Stride).clone();
		bitmap.UnlockBits(&bitmapData);
	}

	DeleteObject(hbmScreen);
	DeleteDC(hdcMemDC);
	::ReleaseDC(hwnd, hdcWindow);
	Gdiplus::GdiplusShutdown(gdiplusToken);

	return mat;
}

void CFormImg::SetImgFromMat(cv::Mat MatImg)
{
	cvtColor(MatImg, m_MatImg, cv::COLOR_BGR2BGRA);
	m_pBitImgMat = new Bitmap((INT)m_MatImg.size().width,
		(INT)m_MatImg.size().height,
		(INT)m_MatImg.step,
		PixelFormat32bppARGB,
		m_MatImg.data);

	SetImgData(m_pBitImgMat);
}


void CFormImg::OnDropFiles(HDROP hDropInfo)
{
	UINT fileCount = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);

	for (UINT i = 0; i < fileCount; i++)
	{
		TCHAR filePath[MAX_PATH];
		DragQueryFile(hDropInfo, i, filePath, MAX_PATH);

		//AfxMessageBox(filePath);

		
		SetImgData(filePath);
	}

	DragFinish(hDropInfo);

	CFormView::OnDropFiles(hDropInfo);
}


HBRUSH CFormImg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CFormView::OnCtlColor(pDC, pWnd, nCtlColor);

	if (nCtlColor == CTLCOLOR_DLG)
	{
		hbr = (HBRUSH)m_BkBrush;
		pDC->SetBkMode(TRANSPARENT);
	}

	return hbr;
}


bool CFormImg::SaveImg(CString csPathFile)
{
	bool bRet = false;

	if (m_pDisplayBitImg != nullptr)
	{
		CString csExt = GetFileExtFromPathFile(csPathFile);

		//int nIndex = csExt.ReverseFind('.');
		//csExt = csExt.Mid(nIndex + 1, csExt.GetLength() - nIndex);

		bRet = SaveBitmapToFile(m_pDisplayBitImg, csPathFile, csExt);
	}

	return bRet;
}

int CFormImg::GetEncoderClsid(WCHAR* format, CLSID* pClsid) 
{
	UINT numEncoders = 0;
	UINT size = 0;

	Gdiplus::ImageCodecInfo* pImageCodecInfo = nullptr;
	Gdiplus::GetImageEncodersSize(&numEncoders, &size);
	if (size == 0) return -1;

	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == nullptr) return -1;

	Gdiplus::GetImageEncoders(numEncoders, size, pImageCodecInfo);
	for (UINT j = 0; j < numEncoders; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}

	free(pImageCodecInfo);
	return -1;
}

// Bitmap을 파일로 저장하는 함수
bool CFormImg::SaveBitmapToFile(Gdiplus::Bitmap* pBitmap, CString csPathFile, CString csFormat) 
{
	if (pBitmap == nullptr) {
		//AfxMessageBox(_T("유효하지 않은 이미지입니다."));
		return false;
	}

	int nSaveFormat = FORMAT_JPG;

	// 이미지 형식에 맞는 MIME 타입 설정
	CLSID clsidEncoder;
	if (csFormat.CompareNoCase(_T("jpg")) == 0 || csFormat.CompareNoCase(_T("jpeg")) == 0) 
	{
		GetEncoderClsid(L"image/jpeg", &clsidEncoder);
		nSaveFormat = FORMAT_JPG;
	}
	else if (csFormat.CompareNoCase(_T("png")) == 0) 
	{
		GetEncoderClsid(L"image/png", &clsidEncoder);
		nSaveFormat = FORMAT_PNG;
	}
	else if (csFormat.CompareNoCase(_T("bmp")) == 0) 
	{
		GetEncoderClsid(L"image/bmp", &clsidEncoder);
		nSaveFormat = FORMAT_BMP;
	}
	else 
	{
		nSaveFormat = FORMAT_NONE;
		//AfxMessageBox(_T("지원하지 않는 파일 형식입니다."));
		return false;
	}

	// Bitmap을 지정된 경로와 형식으로 저장
	Gdiplus::Status status = Gdiplus::GenericError;
	if (nSaveFormat == FORMAT_JPG)
	{
		// 0 ~ 100
		ULONG Quality = 100;

		Gdiplus::EncoderParameters EncParam;
		EncParam.Count = 1;
		EncParam.Parameter[0].Guid = Gdiplus::EncoderQuality;
		EncParam.Parameter[0].Type = EncoderParameterValueTypeLong;
		EncParam.Parameter[0].NumberOfValues = 1;
		EncParam.Parameter[0].Value = &Quality;

		USES_CONVERSION;
		status = pBitmap->Save(A2W(csPathFile), &clsidEncoder, &EncParam);
	}
	else if ((nSaveFormat == FORMAT_PNG) || (nSaveFormat == FORMAT_BMP))
	{
		USES_CONVERSION;
		status = pBitmap->Save(A2W(csPathFile), &clsidEncoder, NULL);
	}
	else
	{
		;
	}
	
	
	if (status != Gdiplus::Ok) 
	{
		//AfxMessageBox(_T("이미지 저장 실패"));
		return false;
	}
	else 
	{
		//AfxMessageBox(_T("이미지가 성공적으로 저장되었습니다."));
		return true;
	}
}

void CFormImg::ImgPop(int nWidth, int nHeight)
{
	/*
	if (m_pDlgPop != nullptr)
	{
		m_pDlgPop->EndDialog(IDOK);
		delete m_pDlgPop;
		m_pDlgPop = nullptr;
	}
	*/

	m_pDlgPop = new CDlgPopup();
		
	m_pDlgPop->SetDlgSize(nWidth, nHeight);

	m_pDlgPop->Create(IDD_DLG_POP, this);
	m_pDlgPop->ShowWindow(SW_SHOW);

	m_pDlgPop->SetImg(m_pDisplayBitImg, m_csPathFileName);

	// key down
	m_pDlgPop->SetFocus();
}

void CFormImg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	// 주의 - 임시 크기
	ImgPop(600, 800);

	CFormView::OnLButtonDblClk(nFlags, point);
}


void CFormImg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// key down
	SetFocus();

	CFormView::OnLButtonDown(nFlags, point);
}
