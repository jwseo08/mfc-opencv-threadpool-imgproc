#pragma once

#define COLOR_BLACK RGB(2, 11, 13)
#define COLOR_DARK_GRAY RGB(52, 52, 52)
#define COLOR_GRAY RGB(109, 109, 109)
#define COLOR_BROWN RGB(178, 132, 115)
#define COLOR_WHITE RGB(255, 255, 255)
#define COLOR_BLUE RGB(113, 171, 213)　

#define COLOR_BTN_RUN RGB(98, 122, 125)
#define COLOR_BTN_NEXT_BACK RGB(128, 128, 0)

#define COLOR_OATMEAL_GRAY   RGB(237, 233, 228)
#define COLOR_SLATE_CHARCOAL RGB(40, 44, 52)
#define COLOR_SAGE_GREEN RGB(206, 214, 204)
#define COLOR_DEEP_PINE_GREEN RGB(35, 48, 43)
#define COLOR_MINT_GRAY RGB(95, 90, 86)
#define COLOR_DARK_BLUE RGB(73, 131, 113)
#define COLOR_SKY_BLUE RGB(113, 171, 231);

#define COLOR_MIN_WHITE RGB(235, 235, 235);

#define WM_IMG_WORK_START        (WM_USER + 1000)
#define WM_IMG_WORK_END          (WM_USER + 1001)
#define WM_NOTIFY_PROC_STATUS    (WM_USER + 2000)

#define DISP_IMG_LIST_SRC   0
#define DISP_IMG_LIST_DST   1


enum class WorkStatus
{
	AppStart = 0,
	SrcImgLoaded = 1,
	ImgProcStart = 2,
	ImgProcEnd = 3,
	WorkStop = 4
};

enum class WorkMode
{
	Sequencial = 0,
	ThreadPool = 1
};


// 로드된 이미지 정보
struct TImgInfo
{
	CString csPathFileName;	// 이미지 파일 전체 경로
	CString csCaption;		// 이미지 표출 시 캡션
	CString csComment;		// 이미지 표출 시 표시할 코맨트
	Bitmap* pBitmap;		// 이미지 썸네일 포인터 - 리스트에는 본 이미지가 아니라 썸네일을 표시
	LONG_PTR Owner;			// 이미지를 로드한 창 주소

	TImgInfo()
	{
		csPathFileName = "";
		csCaption = "";
		csComment = "";
		pBitmap = nullptr;
		Owner = NULL;
	}

	// 파일 이름 비교를 위한 연산자 정의
	bool operator==(const TImgInfo& other) const
	{
		if (csPathFileName.CompareNoCase(other.csPathFileName) == 0) return true;
		else return false;
	}

};




