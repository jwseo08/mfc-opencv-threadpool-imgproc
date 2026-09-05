#include "pch.h"
#include "CFormImgList.h"
#include "CFormImg.h"
#include <algorithm>
#include <unordered_set>
#include <filesystem>

#include "CommonUtil.h"
#include "resource.h"

namespace fs = std::filesystem;

namespace
{
	// 이미지 가로세로 비율에 맞도록 그리기 영역 크기 계산
	Gdiplus::Rect GetAspectFitRect(const CRect& targetRect, UINT imageWidth, UINT imageHeight)
	{
		if (targetRect.Width() <= 0 || targetRect.Height() <= 0 || imageWidth == 0 || imageHeight == 0)
			return Gdiplus::Rect(targetRect.left, targetRect.top, 0, 0);

		const double scale = (std::min)(
			static_cast<double>(targetRect.Width()) / imageWidth,
			static_cast<double>(targetRect.Height()) / imageHeight);

		const int width = (std::max)(1, static_cast<int>(imageWidth * scale + 0.5));
		const int height = (std::max)(1, static_cast<int>(imageHeight * scale + 0.5));

		return Gdiplus::Rect(
			targetRect.left + (targetRect.Width() - width) / 2,
			targetRect.top + (targetRect.Height() - height) / 2,
			width,
			height);
	}
}

// 파일 이름 순으로 정렬
bool compImgInfoPathFile(const TImgInfo& lhs, const TImgInfo& rhs)
{
	//return lhs.strName > rhs.strName; // 큰 값 우선
	//return lhs.szFolderName < rhs.szFolderName;	// 작은 값 우선

	// 작은값 우선 - 윗줄 참고
	if (lhs.csPathFileName < rhs.csPathFileName)
	{
		return true;
	}
	else
	{
		return false;
	}
}

// 해시 함수 정의 - unordered_set에서 사용
namespace std {
	template <>
	struct hash<TImgInfo> {
		std::size_t operator()(const TImgInfo& d) const {
			return std::hash<string>()(string(d.csPathFileName));  // 파일 전체 경로를 기준으로 해싱
		}
	};
}

CFormImgList::CFormImgList() : CFormView(UINT(0))
{
	m_nTempleteID = 0;
	m_nScBarTempleteID = 0;
	m_nDumyID = 0;
	
	InitVariable();
}

CFormImgList::CFormImgList(UINT nID, UINT nScBarID) : CFormView(nID) //MAKEINTRESOURCE(nID)
{
	// 리스트 컨트롤과 스크롤 바 컨트롤에 사용하는 리소스 아이디 지정
	m_nTempleteID = nID;
	m_nScBarTempleteID = nScBarID;

	// 더미 아이디는 리스트의 위치를 잡기 위해서 사용
	m_nDumyID = 0;

	InitVariable();
}

CFormImgList::CFormImgList(UINT nID, UINT nScBarID, int nDumyID) : CFormView(nID) //MAKEINTRESOURCE(nID)
{
	// 리스트 컨트롤과 스크롤 바 컨트롤에 사용하는 리소스 아이디 지정
	m_nTempleteID = nID;
	m_nScBarTempleteID = nScBarID;

	// 더미 아이디는 리스트의 위치를 잡기 위해서 사용
	m_nDumyID = nDumyID;

	InitVariable();
}

CFormImgList::~CFormImgList()
{
	if (m_ptPartArea != nullptr)
	{
		delete[] m_ptPartArea;
		m_ptPartArea = nullptr;
	}

	ClearDragTarget();
	ClearImgInfoList();

	m_BkBrush.DeleteObject();
}

BEGIN_MESSAGE_MAP(CFormImgList, CFormView)
	ON_WM_PAINT()
	ON_MESSAGE(UM_VSCROLL, OnUmVScroll)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_ENABLE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


void CFormImgList::InitVariable()
{
	m_nScBarShape = USE_MODI_BAR;
	m_nScBarType = SB_VERT;
	m_nScBarSize = 0;

	m_nPartAreaCnt = 0;
	m_ptPartArea = nullptr;
	m_nSelectedPartArea = -1;
	m_nSelectedImgInfoIndex = -1;

	m_nDragStatus = STATUS_DRAG_END;

	m_pModiScBar = nullptr;

	memset(&m_tMouseEvent, NULL, sizeof(m_tMouseEvent));
	m_tMouseEvent.cbSize = sizeof(m_tMouseEvent);
	m_tMouseEvent.dwFlags = TME_LEAVE;
	m_tMouseEvent.hwndTrack = NULL;
	m_tMouseEvent.dwHoverTime = 0;

	m_hOldCursor = NULL;
	m_hDragCursor = LoadCursor(AfxGetInstanceHandle(), IDC_HAND);

	m_csDragPathFileName = "";

	m_pTargetWnd = nullptr;

	m_dwBkColor = RGB(0, 0, 0);
	m_BkBrush.CreateSolidBrush(m_dwBkColor);

	m_pDlgPop = nullptr;

	m_bSelectEnable = true;
}

void CFormImgList::PreSubclassWindow()
{
	// 창 생성 전에 스타일 지정
	ModifyStyle(0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
		
	CFormView::PreSubclassWindow();
}

// 생성자에서 templete id, scroll templete id 를 받은 경우 - 폼 위치를 직접 입력 받음
BOOL CFormImgList::CreateFormImgList(CRect DestRect, int nScBarType, int nScBarSize, int nPartAreaCnt, CWnd* pParent)
{
	m_nScBarType = (nScBarType == SB_HORZ) ? SB_HORZ : SB_VERT;
	m_nScBarSize = (std::max)(0, nScBarSize);
	BOOL bRet = Create("", "", WS_CHILD | WS_VISIBLE | WS_BORDER, DestRect, pParent, m_nTempleteID, NULL);
	if (bRet == TRUE)
	{
		ModifyStyle(0, WS_CLIPCHILDREN);
		SetScrollSizes(MM_TEXT, CSize(0, 0));
		ShowWindow(SW_SHOW);

		// 스크롤바 생성
		CRect ScBarRect;
		GetClientRect(ScBarRect);

		if (m_nScBarShape == USE_NORMAL_BAR)
		{
			// 기본 스크롤 바 사용 시 컨트롤 생성
			switch (nScBarType)
			{
			case SB_VERT:
			default:
				ScBarRect.left = ScBarRect.right - nScBarSize;
				m_ctrlScBar.Create(WS_CHILD | WS_VISIBLE | SBS_VERT, ScBarRect, this, m_nScBarTempleteID);
				break;

			case SB_HORZ:
				ScBarRect.top = ScBarRect.bottom - nScBarSize;
				m_ctrlScBar.Create(WS_CHILD | WS_VISIBLE | SBS_HORZ, ScBarRect, this, m_nScBarTempleteID);
				break;
			}

			m_ctrlScBar.SetScrollRange(0, 0);
			m_ctrlScBar.ShowWindow(SW_SHOW);
		}
		else
		{
			// 커스텀 컨트롤 사용 시 컨트롤 생성
			if (m_nScBarType == SB_HORZ) ScBarRect.top = ScBarRect.bottom - nScBarSize;
			else ScBarRect.left = ScBarRect.right - nScBarSize;
			
			m_pModiScBar = new CModiScrollBar(m_nScBarTempleteID);
			m_pModiScBar->CreateScBar(ScBarRect, m_nScBarType, this);

			m_pModiScBar->SetRange(0, 0);
			m_pModiScBar->ShowWindow(SW_SHOW);
		}

		// 화면 분할 영역 설정 - 화면 분할 수량, 스크롤바 크기, 스크롤바 종류
		SetPartArea(nPartAreaCnt, nScBarSize, nScBarType);
	}

	return bRet;
}

// 생성자에서 templete id, scroll templete id, dumy id 를 생성자로 받았을 때
BOOL CFormImgList::CreateFormImgList(int nScBarType, int nScBarSize, int nPartAreaCnt, CWnd* pParent)
{
	m_nScBarType = (nScBarType == SB_HORZ) ? SB_HORZ : SB_VERT;
	m_nScBarSize = (std::max)(0, nScBarSize);

	// 이미지 리스트의 위치를 지정하기 위해서 더미 컨트롤의 위치를 가져옴
	// 작업 후 더미 컨트롤은 숨김
	CWnd* pDumy = pParent->GetDlgItem(m_nDumyID);
	CRect DestRect;
	pDumy->GetWindowRect(DestRect);
	pParent->ScreenToClient(DestRect);
	pDumy->ShowWindow(SW_HIDE);

	// 이미지 리스트 생성
	BOOL bRet = Create("", "", WS_CHILD | WS_VISIBLE, DestRect, pParent, m_nTempleteID, NULL);
	
	// 이미지 리스트 생성이 성공하면 필요한 설정을 추가하고 스크롤바 생성
	if (bRet == TRUE)
	{
		ModifyStyle(0, WS_CLIPCHILDREN);
		SetScrollSizes(MM_TEXT, CSize(0, 0));
		ShowWindow(SW_SHOW);

		// 주의 - modi sc bar 사용 시 rect 정확하지 않음 - 조정 필요
		// 스크롤바 생성
		CRect ScBarRect;
		GetClientRect(ScBarRect);
		
		if (m_nScBarShape == USE_NORMAL_BAR)
		{
			switch (nScBarType)
			{
			case SB_VERT:
			default:
				ScBarRect.left = ScBarRect.right - nScBarSize;
				m_ctrlScBar.Create(WS_CHILD | WS_VISIBLE | SBS_VERT, ScBarRect, this, m_nScBarTempleteID);
				break;

			case SB_HORZ:
				ScBarRect.top = ScBarRect.bottom - nScBarSize;
				m_ctrlScBar.Create(WS_CHILD | WS_VISIBLE | SBS_HORZ, ScBarRect, this, m_nScBarTempleteID);
				break;
			}

			m_ctrlScBar.SetScrollRange(0, 0);
			m_ctrlScBar.ShowWindow(SW_SHOW);
		}
		else
		{
			if (m_nScBarType == SB_HORZ)
				ScBarRect.top = ScBarRect.bottom - nScBarSize;
			else
				ScBarRect.left = ScBarRect.right - nScBarSize;

			m_pModiScBar = new CModiScrollBar(m_nScBarTempleteID);
			m_pModiScBar->CreateScBar(ScBarRect, m_nScBarType, this);

			m_pModiScBar->SetRange(0, 0);
			m_pModiScBar->ShowWindow(SW_SHOW);
		}

		// 화면 분할 영역 설정
		SetPartArea(nPartAreaCnt, nScBarSize, nScBarType);
	}

	return bRet;
}

// 이미지 리스트에서 각각의 이미지가 보여지는 영역 설정
void CFormImgList::SetPartArea(int nPartAreaCnt, int nScrollBarSize, int nScBarType)
{
	CRect DestRect;
	GetClientRect(DestRect);
	if (nPartAreaCnt <= 0) return;
	m_nScBarType = (nScBarType == SB_HORZ) ? SB_HORZ : SB_VERT;
	m_nScBarSize = (std::max)(0, nScrollBarSize);

	// 한번에 보여지는 분할 영역 수량 설정
	m_nPartAreaCnt = nPartAreaCnt;
	if (m_ptPartArea != nullptr) delete[] m_ptPartArea;
	m_ptPartArea = new TPartArea[nPartAreaCnt];

	// 각각의 분할 영역에 대한 이미지 표시 영역, 텍스트 표시 영역 설정
	CRect PartAreaRect;
	CRect ImgRect;
	CRect TextRect;

	if (nScBarType == SB_VERT)
	{
		int nPartAreaHeight = (int)((double)DestRect.Height() / nPartAreaCnt);

		for (int i = 0; i < nPartAreaCnt; i++)
		{
			PartAreaRect.left = 0;
			PartAreaRect.top = nPartAreaHeight * i;
			PartAreaRect.right = DestRect.right - nScrollBarSize;
			PartAreaRect.bottom = (i == nPartAreaCnt - 1) ? DestRect.bottom : PartAreaRect.top + nPartAreaHeight;

			ImgRect.left = 0;
			ImgRect.top = PartAreaRect.top;
			ImgRect.right = (int)(PartAreaRect.Width() * 0.33);
			ImgRect.bottom = PartAreaRect.bottom;

			TextRect.left = ImgRect.right;
			TextRect.top = PartAreaRect.top;
			TextRect.right = PartAreaRect.right;
			TextRect.bottom = PartAreaRect.bottom;

			m_ptPartArea[i].BaseRect = PartAreaRect;
			m_ptPartArea[i].ImgRect = ImgRect;
			m_ptPartArea[i].ImgRect.DeflateRect(2, 2);
			m_ptPartArea[i].TextRect = TextRect;
			m_ptPartArea[i].TextRect.DeflateRect(2, 2);
		}
	}
	else
	{
		int nPartAreaWidth = (int)((double)DestRect.Width() / nPartAreaCnt);

		for (int i = 0; i < nPartAreaCnt; i++)
		{
			PartAreaRect.left = nPartAreaWidth * i;
			PartAreaRect.top = 0;
			PartAreaRect.right = (i == nPartAreaCnt - 1)
				? DestRect.right : PartAreaRect.left + nPartAreaWidth;
			PartAreaRect.bottom = DestRect.bottom - nScrollBarSize;

			ImgRect.left = PartAreaRect.left;
			ImgRect.top = 0;
			ImgRect.right = PartAreaRect.right;
			ImgRect.bottom = (int)(PartAreaRect.Height() * 0.66);

			TextRect.left = PartAreaRect.left;
			TextRect.top = ImgRect.bottom;
			TextRect.right = PartAreaRect.right;
			TextRect.bottom = PartAreaRect.bottom;

			m_ptPartArea[i].BaseRect = PartAreaRect;
			m_ptPartArea[i].ImgRect = ImgRect;
			m_ptPartArea[i].ImgRect.DeflateRect(2, 2);
			m_ptPartArea[i].TextRect = TextRect;
			m_ptPartArea[i].TextRect.DeflateRect(2, 2);
		}
	}

}

void CFormImgList::SetScrollRangeAndStep(int nImgCnt, int nBaseStep)
{
	if (m_nPartAreaCnt <= 0 || nBaseStep <= 0)
		return;

	// 이미지 리스트에 로드된 이미지 개수를 기반으로 스크롤 바 이동 범위 설정
	const int nMaxScrollRange = (std::max)(0, nImgCnt - m_nPartAreaCnt) * nBaseStep;

	if (m_nScBarShape == USE_NORMAL_BAR)
	{
		m_ctrlScBar.SetScrollRange(0, nMaxScrollRange);
		m_ctrlScBar.SetScrollPos(0);
	}
	else
	{
		m_pModiScBar->SetRange(0, nMaxScrollRange);
		m_pModiScBar->SetScrollPos(0);
	}
		
	m_tScrollInfo.nMaxRange = nMaxScrollRange;
	m_tScrollInfo.nBaseStep = nBaseStep;
	m_tScrollInfo.nBtnStep = nBaseStep * m_nPartAreaCnt;
	m_tScrollInfo.nThumbStep = 0;
	m_tScrollInfo.nWheelStep = nBaseStep / 10;
	m_tScrollInfo.nKeyStep = nBaseStep;
}

bool CFormImgList::ScrollOneItem(bool bForward, BOOL bRedraw)
{
	if (m_nPartAreaCnt <= 0 || 
		static_cast<int>(m_vecImgInfo.size()) <= m_nPartAreaCnt || 
		m_tScrollInfo.nBaseStep <= 0)
	{
		return false;
	}

	// 스크롤바의 현재 스크롤 위치 파악
	int nCurrentPos = 0;
	if (m_nScBarShape == USE_NORMAL_BAR) 
	{
		nCurrentPos = m_ctrlScBar.GetScrollPos();
	}
	else
	{
		if (m_pModiScBar != nullptr) nCurrentPos = m_pModiScBar->GetScrollPos();
		else nCurrentPos = 0;
	}

	// 스크롤 방향에 따라서 이동할 새로운 위치 계산
	int nDelta = 0;
	if (bForward) nDelta = m_tScrollInfo.nBaseStep;
	else nDelta = -m_tScrollInfo.nBaseStep;

	int nNewPos = nCurrentPos + nDelta;
	if (nNewPos < 0) nNewPos = 0;

	if (nNewPos > m_tScrollInfo.nMaxRange) nNewPos = m_tScrollInfo.nMaxRange;
	
	// 현재 위치가 새로운 위치와 같으면 처리 안함
	if (nNewPos == nCurrentPos) return false;

	// 새로운 위치로 이동
	if (m_nScBarShape == USE_NORMAL_BAR) m_ctrlScBar.SetScrollPos(nNewPos);
	else if (m_pModiScBar != nullptr) m_pModiScBar->SetScrollPos(nNewPos);

	if (bRedraw)
	{
		Invalidate(FALSE);
		if (m_pModiScBar != nullptr) m_pModiScBar->Invalidate(FALSE);
	}

	return true;
}


void CFormImgList::SetImgList(const std::vector<std::string>& vImgList, bool redraw/* = false*/)
{
	// 기존 이미지 정보 리스트 내용 삭제
	m_vecImgInfo = std::vector<TImgInfo>();

	int nImgCnt = (int)vImgList.size();
	
	if (nImgCnt > 0)
	{
		// 리스트에서 선택 인덱스 초기화
		m_nSelectedImgInfoIndex = -1;
		m_nSelectedPartArea = -1;

		if (m_vecImgInfo.size() == 0)
		{
			// 리스트가 비어있으면 순서대로 삽입
			for (int i = 0; i < nImgCnt; i++)
			{
				// 이미지 파일을 리스트에 표시하기 위해서 bitmap 생성
				USES_CONVERSION;
				Bitmap BitImg(A2W(vImgList[i].c_str()));
				if (BitImg.GetLastStatus() != Ok || BitImg.GetWidth() == 0 || BitImg.GetHeight() == 0) continue;

				// 이미지 섬네일 크기를 이미지 가로세로 비율에 따라서 지정
				const Gdiplus::Rect thumbnailRect = GetAspectFitRect(m_ptPartArea[0].ImgRect, BitImg.GetWidth(), BitImg.GetHeight());

				// 섬네일 생성
				Bitmap* pThumbnail = static_cast<Bitmap*>(BitImg.GetThumbnailImage(thumbnailRect.Width,
					thumbnailRect.Height,
					NULL, NULL));

				if (pThumbnail == nullptr || pThumbnail->GetLastStatus() != Ok)
				{
					delete pThumbnail;
					continue;
				}

				// 이미지 파일이름 표시를 위해서 파일이름으로 텍스트 작성
				TImgInfo tImgInfo;
				tImgInfo.csPathFileName = vImgList[i].c_str();
				tImgInfo.csCaption = fs::path(vImgList[i].c_str()).filename().string().c_str();

				// 이미지 정보에 생성된 섬네일 추가 - 이미지 리스트에는 섬네일이 표시됨
				tImgInfo.pBitmap = pThumbnail;

				// 이미지 정보를 관리 리스트에 추가
				m_vecImgInfo.push_back(tImgInfo);
			}
		}
		else 
		{
			// 리스트에 들어있는 이미지가 있으면 앞에 삽입 - 추가로 불러온 이미지가 앞에 오도록
			std::vector<TImgInfo>::iterator iter = m_vecImgInfo.begin();

			for (int i = nImgCnt - 1; i >= 0; i--)
			{
				USES_CONVERSION;
				Bitmap BitImg(A2W(vImgList[i].c_str()));
				if (BitImg.GetLastStatus() != Ok || BitImg.GetWidth() == 0 || BitImg.GetHeight() == 0)
					continue;

				const Gdiplus::Rect thumbnailRect = GetAspectFitRect(m_ptPartArea[0].ImgRect, BitImg.GetWidth(), BitImg.GetHeight());
				Bitmap* pThumbnail = static_cast<Bitmap*>(BitImg.GetThumbnailImage(thumbnailRect.Width,
					thumbnailRect.Height,
					NULL, NULL));
				if (pThumbnail == nullptr || pThumbnail->GetLastStatus() != Ok)
				{
					delete pThumbnail;
					continue;
				}

				TImgInfo tImgInfo;
				tImgInfo.csPathFileName = vImgList[i].c_str();
				tImgInfo.csCaption = fs::path(vImgList[i].c_str()).filename().string().c_str();
				tImgInfo.pBitmap = pThumbnail;

				iter = m_vecImgInfo.insert(iter, tImgInfo);
			}
		}

		// 이미지 수량에 따라 이미지 리스트 스크롤 설정
		SetScrollRangeAndStep((int)m_vecImgInfo.size(), SCROLL_STEP);
	}

	if (redraw == true) Invalidate(FALSE);
}

void CFormImgList::SetImgListAndScroll(CString pathFile)
{
	// 순서대로 삽입
	USES_CONVERSION;
	Bitmap BitImg(A2W(pathFile));
	if (BitImg.GetLastStatus() != Ok || BitImg.GetWidth() == 0 || BitImg.GetHeight() == 0)
		return;

	const Gdiplus::Rect thumbnailRect = GetAspectFitRect(
		m_ptPartArea[0].ImgRect, BitImg.GetWidth(), BitImg.GetHeight());
	Bitmap* pThumbnail = static_cast<Bitmap*>(BitImg.GetThumbnailImage(thumbnailRect.Width,
		thumbnailRect.Height,
		NULL, NULL));
	if (pThumbnail == nullptr || pThumbnail->GetLastStatus() != Ok)
	{
		delete pThumbnail;
		return;
	}

	TImgInfo tImgInfo;
	tImgInfo.csPathFileName = pathFile;
	tImgInfo.csCaption = fs::path(pathFile.GetString()).filename().string().c_str();
	tImgInfo.pBitmap = pThumbnail;

	m_vecImgInfo.push_back(tImgInfo);

	SetScrollRangeAndStep((int)m_vecImgInfo.size(), SCROLL_STEP);

	m_nSelectedImgInfoIndex = static_cast<int>(m_vecImgInfo.size()) - 1;
	m_nSelectedPartArea = -1;

	// 항목이 화면 표시 개수를 넘을 때마다 새 항목이 보이도록 한칸 이동
	ScrollOneItem(true, FALSE);

	if (m_nSelectedImgInfoIndex != -1)
	{
		TImgInfo tImgInfo = m_vecImgInfo[m_nSelectedImgInfoIndex];
		tImgInfo.Owner = (LONG_PTR)this;
		SetImgInfoToTarget(tImgInfo);
	}

	Invalidate(FALSE);
	if (m_pModiScBar != nullptr) m_pModiScBar->Invalidate(FALSE);
}


void CFormImgList::ClearImgInfoList(bool bRedraw/* = false*/)
{
	if (m_vecImgInfo.size() > 0)
	{
		std::vector<TImgInfo>::iterator iter = m_vecImgInfo.begin();

		do
		{
			// 섬네일 삭제
			if (iter->pBitmap != nullptr)
			{
				delete iter->pBitmap;
				iter->pBitmap = nullptr;
			}

			++iter;
		} while (iter != m_vecImgInfo.end());

		// 이미지 정보 관리 리스트 삭제
		m_vecImgInfo.clear();
		m_vecImgInfo = std::vector<TImgInfo>();
	}

	if (bRedraw == true)
	{
		m_ctrlScBar.SetScrollPos(0);
		Invalidate(FALSE);
	}
}

void CFormImgList::SetDisplayColor(COLORREF dwBkColor, COLORREF dwEmptyBkColor, COLORREF dwSelectBkColor)
{
	m_tDisplayStyle.dwBkColor = dwBkColor;
	m_tDisplayStyle.dwEmptyBkColor = dwEmptyBkColor;
	m_tDisplayStyle.dwSelectBkColor = dwSelectBkColor;
	m_dwBkColor = dwBkColor;
	m_BkBrush.DeleteObject();
	m_BkBrush.CreateSolidBrush(m_dwBkColor);
	if (GetSafeHwnd() != NULL) Invalidate(FALSE);
}

void CFormImgList::SetFontStyle(COLORREF dwFontColor, CString csFontFaceName, int nFontSize, int nTextAlignHorz, int nTextAlignVert)
{
	m_tDisplayStyle.dwFontColor = dwFontColor;
	m_tDisplayStyle.csFontFaceName = csFontFaceName;
	m_tDisplayStyle.nFontSize = nFontSize;

	switch (nTextAlignHorz)
	{
	case TEXT_ALIGN_LEFT:
	default:
		m_tDisplayStyle.eTextAlignHorz = StringAlignmentNear;
		break;

	case TEXT_ALIGN_CENTER:
		m_tDisplayStyle.eTextAlignHorz = StringAlignmentCenter;
		break;

	case TEXT_ALIGN_RIGHT:
		m_tDisplayStyle.eTextAlignHorz = StringAlignmentFar;
		break;
	}

	switch (nTextAlignVert)
	{
	case TEXT_ALIGN_TOP:
	default:
		m_tDisplayStyle.eTextAlignVert = StringAlignmentNear;
		break;

	case TEXT_ALIGN_MIDDLE:
		m_tDisplayStyle.eTextAlignVert = StringAlignmentCenter;
		break;

	case TEXT_ALIGN_BOTTOM:
		m_tDisplayStyle.eTextAlignVert = StringAlignmentFar;
		break;
	}

	if (GetSafeHwnd() != NULL) Invalidate(FALSE);
}

void CFormImgList::SetSplitLineStyle(COLORREF dwLineColor, int nLineSize)
{
	// 이미지 리스트의 이미지 별 분할 영역 경계를 표시하는 분할 선 색상과 크기 설정
	m_tDisplayStyle.dwSplitLineColor = dwLineColor;
	m_tDisplayStyle.nSplitLineSize = (std::max)(0, nLineSize);
	if (GetSafeHwnd() != NULL) Invalidate(FALSE);
}

void CFormImgList::SetDisplayStyle(const TDisplayStyle& style, BOOL bRedraw)
{
	m_tDisplayStyle = style;
	m_tDisplayStyle.nFontSize = (std::max)(1, m_tDisplayStyle.nFontSize);
	m_tDisplayStyle.nSplitLineSize = (std::max)(0, m_tDisplayStyle.nSplitLineSize);
	m_dwBkColor = m_tDisplayStyle.dwBkColor;
	m_BkBrush.DeleteObject();
	m_BkBrush.CreateSolidBrush(m_dwBkColor);
	if (bRedraw && GetSafeHwnd() != NULL) Invalidate(FALSE);
}

void CFormImgList::SetTextColor(COLORREF dwFontColor, COLORREF dwSelectedFontColor, BOOL bRedraw)
{
	m_tDisplayStyle.dwFontColor = dwFontColor;
	m_tDisplayStyle.dwHighLightFontColor = dwSelectedFontColor;
	if (bRedraw && GetSafeHwnd() != NULL) Invalidate(FALSE);
}

bool CFormImgList::SetItemText(int nIndex, const CString& csText, BOOL bRedraw)
{
	if (nIndex < 0 || nIndex >= (int)m_vecImgInfo.size()) return false;
	m_vecImgInfo[nIndex].csCaption = csText;
	if (bRedraw && GetSafeHwnd() != NULL) Invalidate(FALSE);
	return true;
}

CString CFormImgList::GetItemText(int nIndex) const
{
	if (nIndex < 0 || nIndex >= (int)m_vecImgInfo.size()) return CString();
	return m_vecImgInfo[nIndex].csCaption;
}

void CFormImgList::SetScrollBarColor(COLORREF dwBkColor, COLORREF dwBtnFaceColor,
	COLORREF dwBtnClickColor, COLORREF dwBtnBorderColor, COLORREF dwBtnArrowColor,
	COLORREF dwThumbColor, COLORREF dwTrackColor, BOOL bRedraw)
{
	if (m_pModiScBar != nullptr)
	{
		m_pModiScBar->SetColors(dwBkColor, dwBtnFaceColor, dwBtnClickColor,
			dwBtnBorderColor, dwBtnArrowColor, dwThumbColor, dwTrackColor, bRedraw);
	}
}


void CFormImgList::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	
	CRect DestRect;
	GetWindowRect(DestRect);
	ScreenToClient(DestRect);

	// 메모리 gr 생성 - gdiplus의 Graphics 사용
	Graphics DestGr(dc.GetSafeHdc());
	Bitmap BackBitImg(DestRect.Width(), DestRect.Height(), &DestGr);
	Graphics MemGr(&BackBitImg);

	// 이미지 정보 리스트가 있으면 스크롤 위치에 맞춰서 memgr에 이미지 그림
	if (m_vecImgInfo.size() > 0)
	{
		// drag 처리는 추가되지 않음
		if (m_nDragStatus == STATUS_DRAG_MOVE)
		{
			;
		}

		// 이미지가 있으면 바탕을 검은색으로 채움
		Color BkColor;
		BkColor.SetFromCOLORREF(m_tDisplayStyle.dwBkColor);
		SolidBrush BkBrush(BkColor);
		
		MemGr.FillRectangle(&BkBrush, Gdiplus::Rect(DestRect.left, DestRect.top, DestRect.Width(), DestRect.Height()));

		// 스크롤 위치에 맞는 이미지 정보 인덱스 계산
		int nScPos = 0;
		if (m_nScBarShape == USE_NORMAL_BAR)
		{
			nScPos = m_ctrlScBar.GetScrollPos();
		}
		else
		{
			nScPos = m_pModiScBar->GetScrollPos();
			std::cout << "on paint sc pos=" << nScPos << "\n";
		}

		if (m_tScrollInfo.nBaseStep <= 0) m_tScrollInfo.nBaseStep = 100;

		int nImgInfoIndex = nScPos / m_tScrollInfo.nBaseStep;

		// 텍스트 컬러 지정
		Color FontColor;
		FontColor.SetFromCOLORREF(m_tDisplayStyle.dwFontColor);
		SolidBrush FontBrush(FontColor);

		Color HighLightFontColor;
		HighLightFontColor.SetFromCOLORREF(m_tDisplayStyle.dwHighLightFontColor);
		SolidBrush HighLightFontBrush(HighLightFontColor);
		
		// 분할 선 색상 지정
		Color LineColor;
		LineColor.SetFromCOLORREF(m_tDisplayStyle.dwSplitLineColor);
		SolidBrush LineBrush(LineColor);
		Pen LinePen(&LineBrush, (REAL)m_tDisplayStyle.nSplitLineSize);

		// 텍스트 표시를 위한 폰트 생성
		USES_CONVERSION;
		FontFamily fontFamily(A2W((LPCTSTR)m_tDisplayStyle.csFontFaceName));
		Gdiplus::Font font(&fontFamily, (REAL)m_tDisplayStyle.nFontSize, FontStyleRegular, UnitPixel);
		StringFormat StFormat;
		StFormat.SetAlignment(m_tDisplayStyle.eTextAlignHorz);
		StFormat.SetLineAlignment(m_tDisplayStyle.eTextAlignVert);

		// 리스트에 로드된 이미지 수량 만큼 반복
		for (int i = 0; i < m_nPartAreaCnt; i++)
		{
			if (nImgInfoIndex < m_vecImgInfo.size())
			{
				// 이미지 표시
				// 리스트에서 선택한 이미지는 배경을 파란색으로 표시
				if (nImgInfoIndex == m_nSelectedImgInfoIndex)
				{
					Gdiplus::Rect AllRect(m_ptPartArea[i].BaseRect.left, m_ptPartArea[i].BaseRect.top,
						m_ptPartArea[i].BaseRect.Width(), m_ptPartArea[i].BaseRect.Height());

					Color SelectBkColor;
					SelectBkColor.SetFromCOLORREF(m_tDisplayStyle.dwSelectBkColor);
					SolidBrush SelectBkBrush(SelectBkColor);
					MemGr.FillRectangle(&SelectBkBrush, AllRect);
				}

				// 이미지 정보 리스트에 저장된 섬네일 표시 
				Bitmap* pBitmap = m_vecImgInfo[nImgInfoIndex].pBitmap;
				if (pBitmap != nullptr && pBitmap->GetWidth() > 0 && pBitmap->GetHeight() > 0)
				{
					const Gdiplus::Rect imageRect = GetAspectFitRect(
						m_ptPartArea[i].ImgRect, pBitmap->GetWidth(), pBitmap->GetHeight());
					MemGr.DrawImage(pBitmap, imageRect);
				}

				// 텍스트 표시 영역 좌표 설정
				RectF TextRect((REAL)m_ptPartArea[i].TextRect.left, (REAL)m_ptPartArea[i].TextRect.top,
					(REAL)m_ptPartArea[i].TextRect.Width(), (REAL)m_ptPartArea[i].TextRect.Height());

				// 텍스트 표시
				USES_CONVERSION;
				SolidBrush* pTextBrush = (nImgInfoIndex == m_nSelectedImgInfoIndex)
					? &HighLightFontBrush : &FontBrush;
				MemGr.DrawString(A2W(m_vecImgInfo[nImgInfoIndex].csCaption), -1, &font,
					TextRect, &StFormat, pTextBrush);
				
				// 구분선 표시
				if (m_tDisplayStyle.nSplitLineSize > 0)
				{
					if (m_nScBarType == SB_HORZ)
					{
						MemGr.DrawLine(&LinePen,
							Gdiplus::Point(m_ptPartArea[i].BaseRect.right, m_ptPartArea[i].BaseRect.top),
							Gdiplus::Point(m_ptPartArea[i].BaseRect.right, m_ptPartArea[i].BaseRect.bottom));
					}
					else
					{
						MemGr.DrawLine(&LinePen,
							Gdiplus::Point(m_ptPartArea[i].BaseRect.left, m_ptPartArea[i].BaseRect.bottom),
							Gdiplus::Point(m_ptPartArea[i].BaseRect.right, m_ptPartArea[i].BaseRect.bottom));
					}
				}

				m_ptPartArea[i].nCurImgInfoIndex = nImgInfoIndex;

				nImgInfoIndex++;
			}
			else
			{
				// 표시할 이미지가 없는 part area 에는 표시할 이미지 없음으로 설정
				m_ptPartArea[i].nCurImgInfoIndex = -1;
			}
		}
	}
	else
	{
		// 이미지가 없으면 바탕을 회색으로
		Color EmptyBkColor;
		EmptyBkColor.SetFromCOLORREF(m_tDisplayStyle.dwEmptyBkColor);
		SolidBrush EmptyBkBrush(EmptyBkColor);
		MemGr.FillRectangle(&EmptyBkBrush, Gdiplus::Rect(DestRect.left, DestRect.top, DestRect.Width(), DestRect.Height()));
	}

	// memgr에 그려진 내용을 화면에 표출
	CachedBitmap CaBitmap(&BackBitImg, &DestGr);
	DestGr.DrawCachedBitmap(&CaBitmap, 0, 0);
}


LRESULT CFormImgList::OnUmVScroll(WPARAM wParam, LPARAM lParam)
{
	if (m_nScBarType == SB_HORZ)
		OnHScroll(LOWORD(wParam), HIWORD(wParam), NULL);
	else
		OnVScroll(LOWORD(wParam), HIWORD(wParam), NULL);

	return LRESULT();
}

void CFormImgList::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (m_vecImgInfo.size() == 0) return;

	int nScBarPos = 0;

	if (m_nScBarShape == USE_NORMAL_BAR)
	{
		nScBarPos = pScrollBar->GetScrollPos();
	}
	else
	{
		nScBarPos = m_pModiScBar->GetScrollPos();
	}

	std::cout << "scbarpos, npos=" << nScBarPos << ", " << nPos << "\n";

	int nScMovePos = 0;

	switch (nSBCode)
	{
	case SB_LINEUP:
		nScMovePos = nScBarPos - m_tScrollInfo.nBtnStep;
		break;

	case SB_LINEDOWN:
		nScMovePos = nScBarPos + m_tScrollInfo.nBtnStep;
		break;

	case SB_PAGEUP:
		// 임시
		if (m_nScBarShape == USE_NORMAL_BAR)
		{
			nScMovePos = nScBarPos - m_nPartAreaCnt;
		}
		else
		{
			nScMovePos = nScBarPos - m_tScrollInfo.nBtnStep;
		}
		break;

	case SB_PAGEDOWN:
		if (m_nScBarShape == USE_NORMAL_BAR)
		{
			nScMovePos = nScBarPos + m_nPartAreaCnt;
		}
		else
		{
			nScMovePos = nScBarPos + m_tScrollInfo.nBtnStep;
		}
		break;

	case SB_THUMBPOSITION:
		if (m_nScBarShape == USE_NORMAL_BAR)
		{
			nScMovePos = nPos + m_tScrollInfo.nThumbStep;
		}
		else
		{
			nScMovePos = nScBarPos + m_tScrollInfo.nThumbStep;
		}
		break;

	case SB_THUMBTRACK:
		// 임시
		if (m_nScBarShape == USE_NORMAL_BAR)
		{
			nScMovePos = nPos + m_tScrollInfo.nThumbStep;
		}
		else
		{
			nScMovePos = nScBarPos + m_tScrollInfo.nThumbStep;
			std::cout << "on vscroll set sc pos, sc pos=" << nScMovePos << ", " << nScBarPos << "\n";
		}
		break;

	default:
		;
	}

	if (m_nScBarShape == USE_NORMAL_BAR)
	{
		pScrollBar->SetScrollPos(nScMovePos);
	}
	else
	{
		m_pModiScBar->SetScrollPos(nScMovePos);
	}

	
	if (m_vecImgInfo.size() > 0) Invalidate(FALSE);

	CFormView::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CFormImgList::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (m_vecImgInfo.empty()) return;

	int nScBarPos = (m_nScBarShape == USE_NORMAL_BAR)
		? (pScrollBar != nullptr ? pScrollBar->GetScrollPos() : m_ctrlScBar.GetScrollPos())
		: m_pModiScBar->GetScrollPos();
	int nScMovePos = nScBarPos;

	switch (nSBCode)
	{
	case SB_LINELEFT:
		nScMovePos = nScBarPos - m_tScrollInfo.nBtnStep;
		break;

	case SB_LINERIGHT:
		nScMovePos = nScBarPos + m_tScrollInfo.nBtnStep;
		break;

	case SB_PAGELEFT:
		nScMovePos = nScBarPos - m_tScrollInfo.nBtnStep;
		break;

	case SB_PAGERIGHT:
		nScMovePos = nScBarPos + m_tScrollInfo.nBtnStep;
		break;

	case SB_THUMBPOSITION:
		nScMovePos = (m_nScBarShape == USE_NORMAL_BAR) ? nPos : nScBarPos;
		break;

	case SB_THUMBTRACK:
		nScMovePos = (m_nScBarShape == USE_NORMAL_BAR) ? nPos : nScBarPos;
		break;

	default:
		;
	}

	if (m_nScBarShape == USE_NORMAL_BAR)
		m_ctrlScBar.SetScrollPos(nScMovePos);
	else
		m_pModiScBar->SetScrollPos(nScMovePos);

	if (m_vecImgInfo.size() > 0) Invalidate(FALSE);

	if (pScrollBar != nullptr) CFormView::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CFormImgList::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	std::cout << "zdelta=" << zDelta << std::endl;

	// zDelta > 0 - 상단으로 이동
	// zDelta < 0 - 하단으로 이동

	int nScPos = 0;
	if (m_nScBarShape == USE_NORMAL_BAR)
	{
		nScPos = m_ctrlScBar.GetScrollPos();
	}
	else
	{
		nScPos = m_pModiScBar->GetScrollPos();
	}

	int nScMovePos = 0;

	if (zDelta > 0)
	{
		nScMovePos = nScPos - m_tScrollInfo.nWheelStep;
	}
	else
	{
		nScMovePos = nScPos + m_tScrollInfo.nWheelStep;
	}

	if (m_nScBarShape == USE_NORMAL_BAR)
	{
		m_ctrlScBar.SetScrollPos(nScMovePos);
	}
	else
	{
		m_pModiScBar->SetScrollPos(nScMovePos);
	}

	if (m_vecImgInfo.size() > 0) Invalidate(FALSE);

	return CFormView::OnMouseWheel(nFlags, zDelta, pt);
}

void CFormImgList::ArrowKeyScroll(int zDelta)
{
	if (m_vecImgInfo.size() > 0)
	{
		int nScPos = 0;
		if (m_nScBarShape == USE_NORMAL_BAR)
		{
			nScPos = m_ctrlScBar.GetScrollPos();
		}
		else
		{
			nScPos = m_pModiScBar->GetScrollPos();
		}

		int nScMovePos = 0;


		if (m_nSelectedImgInfoIndex == -1)
		{
			m_nSelectedImgInfoIndex = 0;

			nScPos = (m_nSelectedImgInfoIndex - 1) * m_tScrollInfo.nBaseStep;
			m_pModiScBar->SetScrollPos(nScPos);

			// 선택한 이미지가 있는 경우 타겟 창에 표시
			if (m_nSelectedImgInfoIndex != -1)
			{
				TImgInfo tImgInfo = m_vecImgInfo[m_nSelectedImgInfoIndex];
				tImgInfo.Owner = (LONG_PTR)this;
				SetImgInfoToTarget(tImgInfo);
			}

			if (m_vecImgInfo.size() > 0) Invalidate(FALSE);

			return;
		}


		// 사용자가 마우스 휠이나 스크롤 버튼을 조작해서 선택 항목이 위아래로 가려져 안보이는 경우 처리
		if (m_nSelectedImgInfoIndex != -1)
		{
			// 리스트 최상단에 있는 이미지 인덱스
			int nImgInfoIndexTop = nScPos / m_tScrollInfo.nBaseStep;

			// 리스트 최하단에 있는 이미지 인덱스
			int nImgInfoIndexBottom = nScPos / m_tScrollInfo.nBaseStep + m_nPartAreaCnt;

			if (zDelta > 0)
			{
				if (m_nSelectedImgInfoIndex < nImgInfoIndexTop)
				{
					nScPos = (m_nSelectedImgInfoIndex - 1) * m_tScrollInfo.nBaseStep;
					m_pModiScBar->SetScrollPos(nScPos);
				}

				if (m_nSelectedImgInfoIndex > nImgInfoIndexBottom)
				{
					nScPos = (m_nSelectedImgInfoIndex) * m_tScrollInfo.nBaseStep - (m_tScrollInfo.nBaseStep * m_nPartAreaCnt);
					m_pModiScBar->SetScrollPos(nScPos);
				}
			}
			else
			{
				if (m_nSelectedImgInfoIndex < nImgInfoIndexTop)
				{
					nScPos = (m_nSelectedImgInfoIndex + 1) * m_tScrollInfo.nBaseStep;
					m_pModiScBar->SetScrollPos(nScPos);
				}

				if (m_nSelectedImgInfoIndex > nImgInfoIndexBottom)
				{
					nScPos = (m_nSelectedImgInfoIndex) * m_tScrollInfo.nBaseStep - (m_tScrollInfo.nBaseStep * (m_nPartAreaCnt - 1));
					m_pModiScBar->SetScrollPos(nScPos);
				}
			}
		}
		
		if (zDelta > 0)
		{
			// 선택된 항목이 있는 경우 선택 표시 이동
			if (m_nSelectedImgInfoIndex != -1)
			{
				m_nSelectedImgInfoIndex = std::max(0, --m_nSelectedImgInfoIndex);
				
				// 리스트 최상단에 있는 이미지 인덱스
				int nImgInfoIndex = nScPos / m_tScrollInfo.nBaseStep;
				
				if (m_nSelectedImgInfoIndex < nImgInfoIndex)
				{
					nScPos = m_pModiScBar->GetScrollPos();

					nScMovePos = nScPos - m_tScrollInfo.nKeyStep;
					m_pModiScBar->SetScrollPos(nScMovePos);
				}

			}
			else
			{
				nScMovePos = nScPos - m_tScrollInfo.nKeyStep;
				m_pModiScBar->SetScrollPos(nScMovePos);
			}
		}
		else
		{
			// 선택된 항목이 있는 경우 선택 표시 이동
			if (m_nSelectedImgInfoIndex != -1)
			{
				m_nSelectedImgInfoIndex = std::min((int)m_vecImgInfo.size() - 1, ++m_nSelectedImgInfoIndex);

				// 리스트 최하단에 있는 이미지 인덱스
				int nImgInfoIndex = nScPos / m_tScrollInfo.nBaseStep + m_nPartAreaCnt;

				if (m_nSelectedImgInfoIndex >= nImgInfoIndex)
				{
					nScMovePos = nScPos + m_tScrollInfo.nKeyStep;
					m_pModiScBar->SetScrollPos(nScMovePos);
				}
			}
			else
			{
				nScMovePos = nScPos + m_tScrollInfo.nKeyStep;
				m_pModiScBar->SetScrollPos(nScMovePos);
			}
		}


		if (m_nScBarShape == USE_NORMAL_BAR)
		{
			m_ctrlScBar.SetScrollPos(nScMovePos);
		}
		else
		{
			;
		}

		// 선택한 이미지가 있는 경우 타겟 창에 표시
		if (m_nSelectedImgInfoIndex != -1)
		{
			TImgInfo tImgInfo = m_vecImgInfo[m_nSelectedImgInfoIndex];
			tImgInfo.Owner = (LONG_PTR)this;
			SetImgInfoToTarget(tImgInfo);
		}
	
		if (m_vecImgInfo.size() > 0) Invalidate(FALSE);
	}
}

// 리스트에서 항목 선택
void CFormImgList::OnLButtonUp(UINT nFlags, CPoint point)
{
	// 선택 비활성화
	if (m_bSelectEnable == false)
	{
		CFormView::OnLButtonUp(nFlags, point);
		return;
	}

#ifdef _DRAG_USE // drag 처리는 추가되지 않음
	if (m_nDragStatus == STATUS_DRAG_MOVE)
	{
		m_nDragStatus = STATUS_DRAG_END;

		CWnd* pTargetWnd = DragPtHit(point);
		if (pTargetWnd != nullptr)
		{
			TImgInfo tImgInfo;
			if (m_nSelectedPartArea >= 0)
			{
				TRACE("part area %d selected\n", m_nSelectedPartArea);

				int nImgInfoIndex = m_ptPartArea[m_nSelectedPartArea].nCurImgInfoIndex;

				if ((nImgInfoIndex >= 0) && (nImgInfoIndex < m_vecImgInfo.size()))
				{
					m_nSelectedImgInfoIndex = nImgInfoIndex;
					tImgInfo = m_vecImgInfo[nImgInfoIndex];
				}
			}

			SendDropFileMsg(pTargetWnd, tImgInfo.csPathFileName);
			//SendDropFileMsg(pTargetWnd, "c:\\temp\\test\\1.jpg");
		}

		ReleaseCapture();
	}
#endif

	for (int i = 0; i < m_nPartAreaCnt; i++)
	{

		if ((m_ptPartArea[i].BaseRect.left < point.x) && (m_ptPartArea[i].BaseRect.right > point.x)
			&& (m_ptPartArea[i].BaseRect.top < point.y) && (m_ptPartArea[i].BaseRect.bottom > point.y))
		{
			m_nSelectedPartArea = i;
			break;
		}
		else
		{
			m_nSelectedPartArea = -1;
		}
	}

	if (m_nSelectedPartArea >= 0)
	{
		std::cout << "part area selected=" << m_nSelectedPartArea << std::endl;

		int nImgInfoIndex = m_ptPartArea[m_nSelectedPartArea].nCurImgInfoIndex;

		if ((nImgInfoIndex >= 0) && (nImgInfoIndex < m_vecImgInfo.size()))
		{
			m_nSelectedImgInfoIndex = nImgInfoIndex;

			TImgInfo tImgInfo = m_vecImgInfo[nImgInfoIndex];
			tImgInfo.Owner = (LONG_PTR)this;

			SetImgInfoToTarget(tImgInfo);
		}

		Invalidate(FALSE);
	}

	CFormView::OnLButtonUp(nFlags, point);
}

void CFormImgList::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// 선택 비활성화 - 더블클릭해서 이미지 확대 창 뜨는 것은 허용
	//if (m_bSelectEnable == false)
	//{
	//	CFormView::OnLButtonDblClk(nFlags, point);
	//	return;
	//}

	for (int i = 0; i < m_nPartAreaCnt; i++)
	{

		if ((m_ptPartArea[i].BaseRect.left < point.x) && (m_ptPartArea[i].BaseRect.right > point.x)
			&& (m_ptPartArea[i].BaseRect.top < point.y) && (m_ptPartArea[i].BaseRect.bottom > point.y))
		{
			m_nSelectedPartArea = i;
			break;
		}
		else
		{
			m_nSelectedPartArea = -1;
		}
	}

	if (m_nSelectedPartArea >= 0)
	{
		std::cout << "part area selected=" << m_nSelectedPartArea << std::endl;
		
		int nImgInfoIndex = m_ptPartArea[m_nSelectedPartArea].nCurImgInfoIndex;

		if ((nImgInfoIndex >= 0) && (nImgInfoIndex < m_vecImgInfo.size()))
		{
			m_nSelectedImgInfoIndex = nImgInfoIndex;

			TImgInfo tImgInfo = m_vecImgInfo[nImgInfoIndex];

			m_pDlgPop = new CDlgPopup();
			BOOL rt = m_pDlgPop->Create(IDD_DLG_POPUP, this);
			m_pDlgPop->CenterWindow(this);
			m_pDlgPop->ShowWindow(SW_SHOW);
			m_pDlgPop->SetDlgSize(600, 800);
			m_pDlgPop->SetImgData(tImgInfo.csPathFileName);
		}

		// 선택 비활성화 - 더블클릭해서 이미지 확대 창 뜨는 것은 허용
		if (m_bSelectEnable == false)
		{
			m_nSelectedImgInfoIndex = -1;
		}

		Invalidate(FALSE);
	}

	CFormView::OnLButtonDblClk(nFlags, point);
}

// 드래그 시작 - drag 처리는 추가되지 않음
void CFormImgList::OnLButtonDown(UINT nFlags, CPoint point)
{
	// 선택 비활성화
	if (m_bSelectEnable == false)
	{
		CFormView::OnLButtonDown(nFlags, point);
		return;
	}

#ifdef _DRAG_USE
	if (m_nDragStatus == STATUS_DRAG_END)
	{
		m_nDragStatus = STATUS_DRAG_START;
	}

	for (int i = 0; i < m_nPartAreaCnt; i++)
	{

		if ((m_ptPartArea[i].BaseRect.left < point.x) && (m_ptPartArea[i].BaseRect.right > point.x)
			&& (m_ptPartArea[i].BaseRect.top < point.y) && (m_ptPartArea[i].BaseRect.bottom > point.y))
		{
			m_nSelectedPartArea = i;
			break;
		}
		else
		{
			m_nSelectedPartArea = -1;
		}
	}
#endif

	// key down
	SetFocus();

	CFormView::OnLButtonDown(nFlags, point);
}


void CFormImgList::OnMouseMove(UINT nFlags, CPoint point)
{
#ifdef _DRAG_USE
	if (m_nDragStatus == STATUS_DRAG_START)
	{
		m_nDragStatus = STATUS_DRAG_MOVE;
		
		if (m_tMouseEvent.hwndTrack == NULL)
		{
			m_tMouseEvent.hwndTrack = this->GetSafeHwnd();
		}
	}

	if (m_nDragStatus == STATUS_DRAG_MOVE)
	{
		HCURSOR hDragCursor = LoadCursor(NULL, IDC_HAND);
		SetCursor(hDragCursor);

		//TRACE("dag move=%d %d\n", point.x, point.y);

		::_TrackMouseEvent(&m_tMouseEvent);
		/*
		BOOL bDrag = ::_TrackMouseEvent(&m_tMouseEvent);
		if (bDrag == TRUE)
		{
			TRACE("dag move\n");
		}
		*/
	}
#endif

	CFormView::OnMouseMove(nFlags, point);
}


void CFormImgList::OnMouseLeave()
{
#ifdef _DRAG_USE
	TRACE("mouse leave\n");


	if (m_nDragStatus == STATUS_DRAG_MOVE)
	{
		SetCapture();

		/*HCURSOR hDragCursor = LoadCursor(NULL, IDC_HAND);
		SetCursor(hDragCursor);

		GetParent()->SendMessage(UM_DRAG_MOVE)
		m_nDragStatus = STATUS_DRAG_END;*/
	}
#endif

	CFormView::OnMouseLeave();
}


void CFormImgList::AddDragTarget(CWnd* pWnd)
{
	m_vecDragTarget.push_back(pWnd);
}

void CFormImgList::ClearDragTarget()
{
	if (m_vecDragTarget.size() > 0)
	{
		m_vecDragTarget.clear();
		VecDragTaret().swap(m_vecDragTarget);
	}
}

CWnd* CFormImgList::DragPtHit(CPoint point)
{
	CWnd* pTargetWnd = nullptr;
	CRect TargetRect;

	// 클라이언트 영역 벗어난 마우스 좌표를 사용하기 위해서 스크린 좌표로 변경
	ClientToScreen(&point);

	for (int i = 0; i < m_vecDragTarget.size(); i++)
	{
		// 스크린 좌표로 변경된 마우스 좌표와 대조하기 위해서 대상 창의 스크린 좌표 구함
		m_vecDragTarget[i]->GetWindowRect(TargetRect);
		
		std::cout << "target rect left, top, right, bottom, point_x, point_y=" 
			<< TargetRect.left << "," << TargetRect.top << "," << TargetRect.right << "," << TargetRect.bottom
			<< point.x << "," << point.y << std::endl;

		if (TargetRect.PtInRect(point) == TRUE)
		{
			std::cout << "drag drop hit ok\n";

			pTargetWnd = m_vecDragTarget[i];
			break;
		}
	}

	return pTargetWnd;
}

// 드래그 드롭 - 드롭 핸들 생성 - drag 처리는 추가되지 않음
HDROP CFormImgList::CreateFileDropHandle(LPCTSTR filePath)
{
	SIZE_T pathSize = (_tcslen(filePath) + 1) * sizeof(TCHAR);

	// Global 메모리를 할당하고 파일 경로를 복사
	HGLOBAL hGlobal = ::GlobalAlloc(GHND, sizeof(DROPFILES) + pathSize);
	if (hGlobal) 
	{
		DROPFILES* pDropFiles = (DROPFILES*)::GlobalLock(hGlobal);
		if (pDropFiles) 
		{
			pDropFiles->pFiles = sizeof(DROPFILES);
			//pDropFiles->fWide = TRUE; // 유니코드 명시
			pDropFiles->fWide = FALSE; // 유니코드 명시
			memcpy((BYTE*)pDropFiles + sizeof(DROPFILES), filePath, pathSize);
			::GlobalUnlock(hGlobal);

			return (HDROP)hGlobal;
		}
		::GlobalFree(hGlobal);
	}
	return NULL;
}

// drag 처리는 추가되지 않음
void CFormImgList::SendDropFileMsg(CWnd* pTargetWnd, CString csPathFileName)
{
	if (pTargetWnd->GetSafeHwnd() != NULL)
	{
		// 드롭할 파일
		const TCHAR* filePath = (LPCTSTR)csPathFileName;

		// 파일 목록을 담은 드롭 핸들 생성
		HDROP hDrop = CreateFileDropHandle(filePath);
		if (hDrop) 
		{
			// WM_DROPFILES 메시지 보냄
			// 주의 - post msg 안됨
			pTargetWnd->SendMessage(WM_DROPFILES, (WPARAM)hDrop, 0);

			// 드롭 핸들 정리
			::DragFinish(hDrop);
		}
	}
}

CString CFormImgList::RemoveSelectImg()
{
	CString csRemovePathFile = "";

	if ((m_nSelectedImgInfoIndex >= 0) && (m_nSelectedImgInfoIndex < m_vecImgInfo.size()))
	{
		csRemovePathFile = m_vecImgInfo[m_nSelectedImgInfoIndex].csPathFileName;
		m_vecImgInfo.erase(m_vecImgInfo.begin() + m_nSelectedImgInfoIndex);

		m_nSelectedImgInfoIndex = -1;
		SetScrollRangeAndStep((int)m_vecImgInfo.size(), SCROLL_STEP);

		Invalidate(FALSE);
	}
	
	return csRemovePathFile;
}

void CFormImgList::SetTarget(CWnd* pWnd)
{
	m_pTargetWnd = pWnd;
}

void CFormImgList::SetImgInfoToTarget(TImgInfo tImgInfo)
{
	if (m_pTargetWnd != nullptr)
	{
		CFormImg* pFormImg = (CFormImg*)m_pTargetWnd;
		pFormImg->SetImgInfo(tImgInfo);
	}
}


std::vector<TImgInfo>* CFormImgList::GetVecImgInfo()
{
	return (& m_vecImgInfo);
}

std::vector<TImgInfo> CFormImgList::GetImgList()
{
	return m_vecImgInfo;
}

bool CFormImgList::IsEmpty()
{
	if (m_vecImgInfo.size() > 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void CFormImgList::ResetList()
{
	if (m_vecImgInfo.size() > 0)
	{
		sort(m_vecImgInfo.begin(), m_vecImgInfo.end(), compImgInfoPathFile);
		
		for (int i = 0; i < m_vecImgInfo.size(); i++)
		{
			CString csFileName = GetFileNameFromPathFile(m_vecImgInfo[i].csPathFileName);
			m_vecImgInfo[i].csCaption = csFileName;
		}

		// 기존 리스트에서 선택 인덱스 초기화
		m_nSelectedImgInfoIndex = -1;
		m_nSelectedPartArea = -1;

		// 타겟 창 비우기
		if (m_pTargetWnd != nullptr)
		{
			CFormImg* pFormImg = (CFormImg*)m_pTargetWnd;
			pFormImg->ClearImgInfo(true);
		}

		// 이미지 리스트 창 범위와 step 재설정
		SetScrollRangeAndStep((int)m_vecImgInfo.size(), SCROLL_STEP);

		Invalidate(FALSE);
	}
}

void CFormImgList::ClearTargetWnd()
{
	// 타겟 창 비우기
	if (m_pTargetWnd != nullptr)
	{
		CFormImg* pFormImg = (CFormImg*)m_pTargetWnd;
		pFormImg->ClearImgInfo(true);
	}
}


void CFormImgList::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();

	std::cout << "on init update\n";
}


void CFormImgList::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	std::cout << "on active view\n";

	CFormView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


void CFormImgList::OnDestroy()
{
	CFormView::OnDestroy();

	// 주의 - 커스텀 스크롤바에서 gdi plus 를 사용했기 때문에 여기에서 객체 삭제
	if (m_pModiScBar != nullptr)
	{
		delete m_pModiScBar;
		m_pModiScBar = nullptr;
	}
}


HBRUSH CFormImgList::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CFormView::OnCtlColor(pDC, pWnd, nCtlColor);

	if (nCtlColor == CTLCOLOR_DLG)
	{
		hbr = (HBRUSH)m_BkBrush;
		pDC->SetBkMode(TRANSPARENT);
	}

	return hbr;
}


BOOL CFormImgList::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) 
	{
		int nDelta = 0;

		switch (pMsg->wParam) 
		{
		case VK_LEFT:
			if (m_nScBarType == SB_HORZ && m_ProcessedKeys.find(VK_LEFT) == m_ProcessedKeys.end())
			{
				nDelta = WHEEL_DELTA;
				m_ProcessedKeys.insert(VK_LEFT);
			}
			break;

		case VK_RIGHT:
			if (m_nScBarType == SB_HORZ && m_ProcessedKeys.find(VK_RIGHT) == m_ProcessedKeys.end())
			{
				nDelta = -WHEEL_DELTA;
				m_ProcessedKeys.insert(VK_RIGHT);
			}
			break;

		//case VK_LEFT:
		//	std::cout << "Left Arrow Key Pressed" << " " << this <<  std::endl;
		//	break;

		//case VK_RIGHT:
		//	std::cout << "Right Arrow Key Pressed" << " " << this << std::endl;
		//	break;

		case VK_UP:
			if (m_nScBarType == SB_VERT && m_ProcessedKeys.find(VK_UP) == m_ProcessedKeys.end())
			{
				nDelta = WHEEL_DELTA;

				std::cout << "Up Arrow Key Pressed" << " " << this << std::endl;

				m_ProcessedKeys.insert(VK_UP);
			}
			else
			{
				//m_ProcessedKeys.erase(VK_UP);
				//Sleep(100);
			}
			break;

		case VK_DOWN:
			if (m_nScBarType == SB_VERT && m_ProcessedKeys.find(VK_DOWN) == m_ProcessedKeys.end())
			{
				nDelta = -WHEEL_DELTA;

				std::cout << "Down Arrow Key Pressed" << " " << this << std::endl;

				m_ProcessedKeys.insert(VK_DOWN);
			}
			else
			{
				//m_ProcessedKeys.erase(VK_DOWN);
				//Sleep(100);
			}
			break;

		default:
			break;
		}

		if (nDelta != 0)
		{
			ArrowKeyScroll(nDelta);
			//SendMessage(WM_MOUSEWHEEL, MAKEWPARAM(0, nDelta), NULL);
		}

		return TRUE;
	}

	if (pMsg->message == WM_KEYUP)
	{
		switch (pMsg->wParam)
		{
		case VK_LEFT:
			m_ProcessedKeys.erase(VK_LEFT);
			break;
		case VK_RIGHT:
			m_ProcessedKeys.erase(VK_RIGHT);
			break;
		case VK_UP:
			m_ProcessedKeys.erase(VK_UP);
			break;

		case VK_DOWN:
			m_ProcessedKeys.erase(VK_DOWN);
			break;
		}

		return TRUE;
	}

	return CFormView::PreTranslateMessage(pMsg);
}


void CFormImgList::OnEnable(BOOL bEnable)
{
	std::cout << "on enable called" << std::endl;
	CFormView::OnEnable(bEnable);
}

void CFormImgList::OnSize(UINT nType, int cx, int cy)
{
	CFormView::OnSize(nType, cx, cy);
	if (cx <= 0 || cy <= 0) return;

	CRect ScBarRect(0, 0, cx, cy);
	if (m_nScBarType == SB_HORZ)
		ScBarRect.top = (std::max)(0, cy - m_nScBarSize);
	else
		ScBarRect.left = (std::max)(0, cx - m_nScBarSize);

	if (m_nScBarShape == USE_NORMAL_BAR && m_ctrlScBar.GetSafeHwnd() != NULL)
		m_ctrlScBar.MoveWindow(ScBarRect, TRUE);
	else if (m_pModiScBar != nullptr && m_pModiScBar->GetSafeHwnd() != NULL)
		m_pModiScBar->MoveWindow(ScBarRect, TRUE);

	if (m_nPartAreaCnt > 0)
		SetPartArea(m_nPartAreaCnt, m_nScBarSize, m_nScBarType);
}


void CFormImgList::SetSelectEnable(bool bEnable, int nSelectIndex)
{
	m_bSelectEnable = bEnable;

	if (bEnable == false)
	{
		// 선택 비활성화면 선택 항목 표시 안함
		m_nSelectedImgInfoIndex = -1;
		Invalidate(FALSE);
	}
	else
	{
		if (nSelectIndex > 0)
		{
			m_nSelectedImgInfoIndex = nSelectIndex;
			Invalidate(FALSE);
		}
	}
}

int CFormImgList::GetImgCount()
{
	// 로드된 이미지 정보 개수 반환
	return (int)m_vecImgInfo.size();
}




