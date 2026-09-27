
// ImgProcAmpDlg.cpp: 구현 파일
//
#include "pch.h"
#include "framework.h"
#include "ImgProcAmp.h"
#include "ImgProcAmpDlg.h"
#include "afxdialogex.h"

#include "CommonUtil.h"
#include "opencv2/imgcodecs.hpp"

#include <algorithm>
#include <cctype>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
public:
//	afx_msg void OnClose();
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CImgProcAmpDlg 대화 상자
CImgProcAmpDlg::CImgProcAmpDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_IMGPROCAMP_DIALOG, pParent),
	m_pImgListSrc(nullptr),
	m_srcPath(""),
	m_dstPath(""),
	m_workMode(WorkMode::Sequencial),
	m_opencvTh(0),
	m_imgProcInputMode(EImgProcInputMode::FileName),
	m_taskImgFileListSrc(0),
	m_taskImgFileListDst(0),
	m_taskImgBatchSingleThread(0),
	m_threadProc(4, 100),
	m_sysThreadNum(4),
	m_workThreadNum(4)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CImgProcAmpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_PATH_SRC, m_ctrlEditPathSrc);
	DDX_Control(pDX, IDC_EDIT_LOG, m_ctrlEditLog);
	DDX_Control(pDX, IDC_PRGS_WORK, m_ctrlPrgsWork);
	DDX_Control(pDX, IDC_CMB_THR_SET, m_ctrlCmbThrSet);
	DDX_Control(pDX, IDC_RADIO_SEQ, m_ctrlRdoSeq);
	DDX_Control(pDX, IDC_RADIO_THP, m_ctrlRdoThp);
	DDX_Control(pDX, IDC_CHK_OPENCV_TH, m_ctrlChkOpencvTh);
	DDX_Control(pDX, IDC_CMB_IMG_PROC_MODE, m_ctrlCmbImgProcMode);
	DDX_Control(pDX, IDC_STC_WORK_STATUS, m_ctrlStcWorkStatus);
}

BEGIN_MESSAGE_MAP(CImgProcAmpDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_SEARCH_SRC, &CImgProcAmpDlg::OnBnClickedBtnSearchSrc)
	ON_BN_CLICKED(IDC_BTN_START, &CImgProcAmpDlg::OnBnClickedBtnStart)
	ON_BN_CLICKED(IDC_BTN_STOP, &CImgProcAmpDlg::OnBnClickedBtnStop)
	ON_WM_CLOSE()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BTN_EXIT, &CImgProcAmpDlg::OnBnClickedBtnExit)
	ON_BN_CLICKED(IDC_BTN_LOG_SAVE, &CImgProcAmpDlg::OnBnClickedBtnLogSave)
	ON_BN_CLICKED(IDC_RADIO_SEQ, &CImgProcAmpDlg::OnBnClickedRadioSeq)
	ON_BN_CLICKED(IDC_RADIO_THP, &CImgProcAmpDlg::OnBnClickedRadioThp)
	ON_BN_CLICKED(IDC_CHK_OPENCV_TH, &CImgProcAmpDlg::OnBnClickedChkOpencvTh)
	ON_CBN_SELCHANGE(IDC_CMB_IMG_PROC_MODE, &CImgProcAmpDlg::OnCbnSelchangeImgProcMode)

	ON_MESSAGE(WM_CTHREADPROC_TASK_STARTED, &CImgProcAmpDlg::OnImgProcTaskStarted)
	ON_MESSAGE(WM_CTHREADPROC_TASK_COMPLETED, &CImgProcAmpDlg::OnImgProcTaskCompleted)
	ON_MESSAGE(WM_CTHREADPROC_TASK_FAILED, &CImgProcAmpDlg::OnImgProcTaskFailed)
	ON_MESSAGE(WM_CTHREADPROC_IDLE, &CImgProcAmpDlg::OnImgProcIdle)
	ON_MESSAGE(UM_IMG_TASK_ENQUEUE_COMPLETED, &CImgProcAmpDlg::OnImgTaskEnqueueCompleted)

	ON_MESSAGE(WM_CTHREADPROC_SINGLE_TASK_STARTED, &CImgProcAmpDlg::OnSingleTaskStarted)
	ON_MESSAGE(WM_CTHREADPROC_SINGLE_TASK_COMPLETED, &CImgProcAmpDlg::OnSingleTaskCompleted)
	ON_MESSAGE(WM_CTHREADPROC_SINGLE_TASK_FAILED, &CImgProcAmpDlg::OnSingleTaskFailed)
	
	ON_MESSAGE(UM_PROGRESS_SET_POS, &CImgProcAmpDlg::OnProgressSetPos)
	ON_MESSAGE(UM_PROGRESS_SET_RANGE, &CImgProcAmpDlg::OnProgressSetRange)
	ON_MESSAGE(UM_EDIT_LOG_SET_TEXT, &CImgProcAmpDlg::OnEditLogSetText)

END_MESSAGE_MAP()


void CImgProcAmpDlg::ListFile(const std::string& path, const int displayImgList)
{
	uint32_t* taskId = nullptr;

	if (displayImgList == DISP_IMG_LIST_SRC) taskId = &m_taskImgFileListSrc;
	else if (displayImgList == DISP_IMG_LIST_DST) taskId = &m_taskImgFileListDst;

	*taskId = m_threadProc.StartSingleTask(
		[this, path, displayImgList](const std::atomic<bool>& stopRequested)
		{
			return ListFileAndDisplay(path, displayImgList, stopRequested);
		}
	);

	if (*taskId == 0)
	{
		ClosePrepareNotice();
		SetCtrlStatus(WorkStatus::WorkStop);
		AfxMessageBox(_T("싱글 작업을 시작하지 못했습니다."));
	}
}

int CImgProcAmpDlg::ListFileAndDisplay(const std::string& path, const int displayImgList, const std::atomic<bool>& stopRequest)
{
	CFormImgList* pImgLIstCtrl = nullptr;
	std::vector<std::string>* pvImgFileList = nullptr;

	if (displayImgList == DISP_IMG_LIST_SRC)
	{
		pImgLIstCtrl = m_pImgListSrc;
		pvImgFileList = &m_vSrcImgFileList;
	}
	else if (displayImgList == DISP_IMG_LIST_DST)
	{
		pImgLIstCtrl = m_pImgListDst;
		pvImgFileList = &m_vDstImgFileList;
	}

	if (!pvImgFileList->empty()) *pvImgFileList = std::vector<std::string>();
	std::vector<std::string> extList{ ".jpg", ".png", ".bmp", ".gif" };

	int rt = ListFileByExt(path, extList, *pvImgFileList, stopRequest);
	if (rt == 0)
	{
		pImgLIstCtrl->SetImgList(*pvImgFileList);
	}

	PostMessage(UM_PROGRESS_SET_POS, (WPARAM)50, NULL);

	return rt;
}


void CImgProcAmpDlg::SetCtrlStatus(const WorkStatus& status)
{
	switch (status)
	{
	//defualt:
	case WorkStatus::AppStart:
		m_btnSearchPathSrc.EnableWindow(TRUE);
		m_ctrlRdoSeq.EnableWindow(TRUE);
		m_ctrlRdoThp.EnableWindow(TRUE);
		m_ctrlChkOpencvTh.EnableWindow(TRUE);
		m_ctrlCmbImgProcMode.EnableWindow(TRUE);
		m_ctrlCmbThrSet.EnableWindow(FALSE);

		m_btnStart.EnableWindow(TRUE);
		m_btnStop.EnableWindow(FALSE);
		m_btnLogSave.EnableWindow(FALSE);
		m_btnClose.EnableWindow(TRUE);
		break;

	case WorkStatus::SrcImgLoaded:
	case WorkStatus::ImgProcStart:
		m_btnSearchPathSrc.EnableWindow(FALSE);
		m_ctrlRdoSeq.EnableWindow(FALSE);
		m_ctrlRdoThp.EnableWindow(FALSE);
		m_ctrlChkOpencvTh.EnableWindow(FALSE);
		m_ctrlCmbImgProcMode.EnableWindow(FALSE);
		m_ctrlCmbThrSet.EnableWindow(FALSE);

		m_btnStart.EnableWindow(FALSE);
		m_btnStop.EnableWindow(TRUE);
		m_btnLogSave.EnableWindow(FALSE);
		m_btnClose.EnableWindow(FALSE);
		break;

	case WorkStatus::ImgProcEnd:
		m_btnSearchPathSrc.EnableWindow(TRUE);
		m_ctrlRdoSeq.EnableWindow(TRUE);
		m_ctrlRdoThp.EnableWindow(TRUE);
		m_ctrlChkOpencvTh.EnableWindow(TRUE);
		m_ctrlCmbImgProcMode.EnableWindow(TRUE);
		
		if (m_ctrlRdoSeq.GetCheck() == 1) m_ctrlCmbThrSet.EnableWindow(FALSE);
		else m_ctrlCmbThrSet.EnableWindow(TRUE);

		m_btnStart.EnableWindow(TRUE);
		m_btnStop.EnableWindow(FALSE);
		m_btnLogSave.EnableWindow(TRUE);
		m_btnClose.EnableWindow(TRUE);
		break;

	case WorkStatus::WorkStop:
		m_btnSearchPathSrc.EnableWindow(TRUE);
		m_ctrlRdoSeq.EnableWindow(TRUE);
		m_ctrlRdoThp.EnableWindow(TRUE);
		m_ctrlChkOpencvTh.EnableWindow(TRUE);
		m_ctrlCmbImgProcMode.EnableWindow(TRUE);

		if (m_ctrlRdoSeq.GetCheck() == 1) m_ctrlCmbThrSet.EnableWindow(FALSE);
		else m_ctrlCmbThrSet.EnableWindow(TRUE);

		m_btnStart.EnableWindow(TRUE);
		m_btnStop.EnableWindow(FALSE);
		m_btnLogSave.EnableWindow(TRUE);
		m_btnClose.EnableWindow(TRUE);
		m_ctrlPrgsWork.SetPos(0);
		break;
	}
}


// CImgProcAmpDlg 메시지 처리기
BOOL CImgProcAmpDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	//----------------------------------------------------------------------
	
	// ui 컨트롤 생성
	m_pImgListSrc = new CFormImgList(IDD_FORM_BASE, IDD_FORM_BASE, IDC_DUMY_HORZ_IMG_LIST_SRC);
	m_pImgListSrc->SetDisplayColor(RGB(30, 30, 30), RGB(55, 55, 55), RGB(38, 86, 145));
	m_pImgListSrc->SetFontStyle(RGB(245, 245, 245), FONT_FACENAME_MALGUN, 12, TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE);
	m_pImgListSrc->SetSplitLineStyle(RGB(90, 90, 90), 1);
	if (!m_pImgListSrc->CreateFormImgList(SB_HORZ, 24, 6, this))
	{
		delete m_pImgListSrc;
		m_pImgListSrc = nullptr;
		return TRUE;
	}

	m_pImgListSrc->SetScrollBarColor(RGB(45, 45, 48), RGB(90, 90, 94), RGB(0, 122, 204),
		RGB(150, 150, 155), RGB(245, 245, 245), RGB(180, 220, 250), RGB(70, 70, 74));


	m_pImgListDst = new CFormImgList(IDD_FORM_BASE, IDD_FORM_BASE, IDC_DUMY_HORZ_IMG_LIST_DST);
	m_pImgListDst->SetDisplayColor(RGB(30, 30, 30), RGB(55, 55, 55), RGB(38, 86, 145));
	m_pImgListDst->SetFontStyle(RGB(245, 245, 245), FONT_FACENAME_MALGUN, 12, TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE);
	m_pImgListDst->SetSplitLineStyle(RGB(90, 90, 90), 1);
	if (!m_pImgListDst->CreateFormImgList(SB_HORZ, 24, 6, this))
	{
		delete m_pImgListDst;
		m_pImgListDst = nullptr;
		return TRUE;
	}

	m_pImgListDst->SetScrollBarColor(RGB(45, 45, 48), RGB(90, 90, 94), RGB(0, 122, 204),
		RGB(150, 150, 155), RGB(245, 245, 245), RGB(180, 220, 250), RGB(70, 70, 74));


	m_btnSearchPathSrc.CreateBtn(this, IDC_BTN_SEARCH_SRC);
	m_btnClose.CreateBtn(this, IDC_BTN_EXIT);
	m_btnStart.CreateBtn(this, IDC_BTN_START);
	m_btnStop.CreateBtn(this, IDC_BTN_STOP);
	m_btnLogSave.CreateBtn(this, IDC_BTN_LOG_SAVE);

	m_ctrlRdoSeq.SetTextColor(COLOR_WHITE, TRUE);
	m_ctrlRdoSeq.SetBackgroundColor(COLOR_MINT_GRAY);
	m_ctrlRdoThp.SetTextColor(COLOR_WHITE, TRUE);
	m_ctrlRdoThp.SetBackgroundColor(COLOR_MINT_GRAY);
	m_ctrlRdoSeq.SetCheck(BST_CHECKED);
	m_workMode = WorkMode::Sequencial;

	// 스레드 풀 사용 스레드 수 옵션 설정
	SetThreadNumOption(m_ctrlCmbThrSet);
	m_ctrlCmbThrSet.EnableWindow(FALSE);

	m_ctrlChkOpencvTh.SetTextColor(COLOR_WHITE, TRUE);
	m_ctrlChkOpencvTh.SetBackgroundColor(COLOR_MINT_GRAY, TRUE);
	m_ctrlChkOpencvTh.SetCheck(BST_CHECKED);
	m_opencvTh = 1;

	m_ctrlCmbImgProcMode.AddString(_T("이미지 파일 경로"));
	m_ctrlCmbImgProcMode.AddString(_T("cv mat raw 데이터"));
	m_ctrlCmbImgProcMode.AddString(_T("이미지 encoded 데이터"));
	m_ctrlCmbImgProcMode.SetCurSel(static_cast<int>(EImgProcInputMode::FileName));
	m_imgProcInputMode = EImgProcInputMode::FileName;

	m_ctrlPrgsWork.SetRange(0, 100);
	m_ctrlPrgsWork.SetPos(0);

	m_BkBrush.CreateSolidBrush(COLOR_MINT_GRAY);

	// 스레드 객체가 보내는 메시지를 수신하는 대상 설정
	m_threadProc.SetNotifyWindow(this->GetSafeHwnd());

	// 실행 파일과 같은 디렉터리에서 이미지 처리 DLL 로드
	const std::string imgProcDllPath = MakeAbsPath(GetExePath().GetString()) + "/ImgProc.dll";
	const int loadResult = LoadImgProcDll(imgProcDllPath.c_str(), m_imgProcDll);
	if (loadResult != 0)
	{
		CString errorMessage;
		errorMessage.Format(_T("ImgProc.dll을 로드하지 못했습니다. (오류=%d)"), loadResult);
		AfxMessageBox(errorMessage, MB_ICONERROR);
		EndDialog(IDCANCEL);
		return FALSE;
	}

	// 로그 파일이름 생성 - 프로그램 폴더의 log 폴더에 생성, 프로그램 시작 시간으로 구분
	std::string logPath = MakeAbsPath(GetExePath().GetString()) + "/log";
	if (!fs::exists(logPath)) fs::create_directories(logPath);
	m_logFilePathFile = logPath + "/ImgProcLog-" + GetCurrentDateTime() + ".log";

	// ui 컨트롤 상태 설정
	SetCtrlStatus(WorkStatus::AppStart);

	//----------------------------------------------------------------------

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}


void CImgProcAmpDlg::OnDestroy()
{
	ClosePrepareNotice();

	// 윈도우와 DLL을 정리하기 전에 등록 스레드 및 작업 스레드가 더 이상 객체를 사용하지 않도록 종료
	m_imgTaskEnqueueStopRequested.store(true);
	m_threadProc.SetNotifyWindow(nullptr);
	m_threadProc.RequestStop(true); // 큐 공간을 기다리는 EnqueueImageTasks의 AddTask 호출도 깨움
	JoinImageTaskEnqueueThread();
	m_threadProc.StopSingleTask(true);
	m_threadProc.Stop(true, true);
	{
		std::lock_guard<std::mutex> lock(m_imgProcTaskMutex);
		m_mapImgProcTasks.clear();
	}

	// 이미지 처리 dll 언로드
	FreeImgProcDll(m_imgProcDll);

	// ui 컨트롤 메모리 정리
	if (m_pImgListSrc != nullptr)
	{
		delete m_pImgListSrc;
		m_pImgListSrc = nullptr;
	}

	if (m_pImgListDst != nullptr)
	{
		delete m_pImgListDst;
		m_pImgListDst = nullptr;
	}

	m_BkBrush.DeleteObject();

	CDialogEx::OnDestroy();
}

void CImgProcAmpDlg::OnClose()
{
	int rt = MessageBox("프로그램을 종료하시겠습니까?", "알림", MB_YESNO | MB_ICONQUESTION);

	if (rt == IDYES) CDialogEx::OnClose();
	else return;
}

BOOL CImgProcAmpDlg::PreTranslateMessage(MSG* pMsg)
{
	// dialog 닫힘 방지
	if (pMsg->message == WM_KEYDOWN)
	{
		if ((pMsg->wParam == VK_RETURN) || (pMsg->wParam == VK_ESCAPE)) return TRUE;
	}

	// dialog 닫힘 방지
	if (pMsg->message == WM_SYSKEYDOWN)
	{
		if (pMsg->wParam == VK_F4) return TRUE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CImgProcAmpDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CImgProcAmpDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); 

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CImgProcAmpDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CImgProcAmpDlg::OnBnClickedBtnSearchSrc()
{
	CString csPath = "";
	ShowFolderPickerDlg(csPath);
	if (csPath != "")
	{
		m_srcPath = MakeAbsPath(csPath.GetString());        // 경로 문자열 정리
		m_ctrlEditPathSrc.SetWindowText(m_srcPath.c_str()); // ui 처리
	}
	else
	{
		;
	}
}

bool CImgProcAmpDlg::ShowPrepareNotice()
{
	if (::IsWindow(m_prepareNotice.GetSafeHwnd())) return false;

	const CString className = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_WAIT), ::GetSysColorBrush(COLOR_3DFACE));
	
	CRect rect(0, 0, 360, 120);
	if (!m_prepareNotice.CreateEx(WS_EX_DLGMODALFRAME, className, _T("알림"), WS_POPUP | WS_CAPTION, rect, this, 0)) 
		return false;
	
	m_prepareNotice.GetClientRect(&rect);
	rect.DeflateRect(12, 12);

	if (!m_prepareNoticeText.Create(_T("작업을 준비 중입니다."), 
		WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		rect, &m_prepareNotice, 1))
	{
		m_prepareNotice.DestroyWindow();
		return false;
	}
	m_prepareNoticeText.SetFont(GetFont());
	m_prepareNotice.CenterWindow(this);
	EnableWindow(FALSE);
	m_prepareNotice.ShowWindow(SW_SHOW);
	m_prepareNotice.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	
	return true;
}

void CImgProcAmpDlg::ClosePrepareNotice()
{
	if (!::IsWindow(m_prepareNotice.GetSafeHwnd())) return;
	EnableWindow(TRUE);
	m_prepareNotice.DestroyWindow();
	SetActiveWindow();
}

void CImgProcAmpDlg::OnBnClickedBtnStart()
{
	if (::IsWindow(m_prepareNotice.GetSafeHwnd())) return;
	m_ctrlPrgsWork.SetRange(0, 100);
	m_ctrlPrgsWork.SetPos(0);

	// 원본 이미지 경로 확인
	if (m_srcPath.empty())
	{
		MessageBox("원본 이미지 폴더가 지정되지 않았습니다. 경로를 확인하십시오.", "알림", MB_OK | MB_ICONERROR);
		return;
	}
	
	MessageBox("결과 저장을 위해서 원본 이미지 폴더에 output-[현재시간] 폴더를 생성합니다.", "알림", MB_OK | MB_ICONINFORMATION);
	m_dstPath = MakeDirByDateTime(m_srcPath, "output");
	
	if (!m_srcPath.empty() && !m_dstPath.empty())
	{
		if (!ShowPrepareNotice())
		{
			std::cerr << "prepare notify dlg show fail.\n";
			return;
		}
		SetCtrlStatus(WorkStatus::ImgProcStart);

		m_vSrcImgFileList.clear();
		m_pImgListSrc->ClearImgInfoList(true);

		m_vDstImgFileList.clear();
		m_pImgListDst->ClearImgInfoList(true);

		// 작업 대상 원본 이미지 목록 만들기 - 후속 작업은 스레드 완료 메시지에 따라서 진행
		ListFile(m_srcPath, DISP_IMG_LIST_SRC);
	}
	else
	{
		;
	}
}

void CImgProcAmpDlg::OnBnClickedBtnStop()
{
	ClosePrepareNotice();
	// 작업 등록 스레드에 먼저 중지를 알리고, AddTask 대기를 해제하기 위해 스레드 풀에도 중지 요청 전달
	m_imgTaskEnqueueStopRequested.store(true);
	m_threadProc.RequestSingleTaskStop();
	m_threadProc.RequestStop(true);
	JoinImageTaskEnqueueThread();

	// 실행 중인 처리 함수가 완료될 때까지 기다린 뒤 스레드 정리
	m_threadProc.StopSingleTask(true);
	m_threadProc.Stop(true, true);

	// 작업 스레드가 이미 PostMessage한 완료 메시지가 나중에 처리되더라도
	// 취소된 작업의 후속 처리를 하지 않도록 연관 변수 초기화
	m_taskImgFileListSrc = 0;
	m_taskImgFileListDst = 0;
	m_taskImgBatchSingleThread = 0;

	m_bImgBatchRunning = false;
	m_bImgTaskEnqueueCompleted.store(false);
	{
		// 등록 및 완료 스레드가 모두 끝난 뒤 작업 관리 목록을 안전하게 비움
		std::lock_guard<std::mutex> lock(m_imgProcTaskMutex);
		m_mapImgProcTasks.clear();
	}
	m_nImgTaskEnqueued.store(0);
	m_nImgTaskFinished.store(0);

	MessageBox("작업이 취소되었습니다.", "알림", MB_OK | MB_ICONINFORMATION);

	SetCtrlStatus(WorkStatus::WorkStop);
}

void CImgProcAmpDlg::OnBnClickedBtnExit()
{
	SendMessage(WM_CLOSE, 0, NULL);
}

void CImgProcAmpDlg::OnBnClickedBtnLogSave()
{
	// 로그 파일 저장
	int rt = SaveLog(m_logFilePathFile, m_ossLogAll);

	if (rt == 0)
	{
		std::string msg = "로그 파일이 저장되었습니다.\r\n" + std::string("파일 위치 : ") + m_logFilePathFile;
		MessageBox(msg.c_str(), "알림", MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		MessageBox("로그 파일을 저장할 수 없습니다.", "알림", MB_OK | MB_ICONERROR);
	}
}

void CImgProcAmpDlg::OnBnClickedRadioSeq()
{
	m_ctrlCmbThrSet.EnableWindow(FALSE);
}

void CImgProcAmpDlg::OnBnClickedRadioThp()
{
	m_ctrlCmbThrSet.EnableWindow(TRUE);
}

void CImgProcAmpDlg::OnBnClickedChkOpencvTh()
{
	m_opencvTh = m_ctrlChkOpencvTh.GetCheck();

	std::cout << "opencv internal thread=" << m_opencvTh << std::endl;
}

void CImgProcAmpDlg::OnCbnSelchangeImgProcMode()
{
	const int selectedIndex = m_ctrlCmbImgProcMode.GetCurSel();
	if (selectedIndex >= static_cast<int>(EImgProcInputMode::FileName) &&
		selectedIndex <= static_cast<int>(EImgProcInputMode::EncodedImage))
	{
		m_imgProcInputMode = static_cast<EImgProcInputMode>(selectedIndex);
	}
}


HBRUSH CImgProcAmpDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	//------------------------------------------------

	if (nCtlColor == CTLCOLOR_DLG)
	{
		hbr = (HBRUSH)m_BkBrush;
		pDC->SetBkMode(TRANSPARENT);
	}

	if (nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetTextColor(COLOR_WHITE);
		hbr = (HBRUSH)m_BkBrush;
		pDC->SetBkMode(TRANSPARENT);
	}

	else if (nCtlColor == CTLCOLOR_BTN)
	{
		UINT ctrlID = pWnd->GetDlgCtrlID();

		if (ctrlID == IDC_RADIO_SRP || ctrlID == IDC_RADIO_THP)
		{
			//pDC->SetTextColor(RGB(255, 255, 255));
			pDC->SetBkMode(TRANSPARENT);
			hbr = (HBRUSH)m_BkBrush;
		}
	}

	//------------------------------------------------

	return hbr;
}


//----------------------------------------------------------------------------------


LRESULT CImgProcAmpDlg::OnImgProcTaskStarted(WPARAM wParam, LPARAM lParam)
{
	const std::uint32_t nTaskId = static_cast<std::uint32_t>(wParam);
	std::string inputFileName;
	ClosePrepareNotice();

	{
		// EnqueueImageTasks의 작업 목록 등록과 동시에 접근할 수 있으므로 mutex로 보호
		std::lock_guard<std::mutex> lock(m_imgProcTaskMutex);
		auto it = m_mapImgProcTasks.find(nTaskId);
		if (it != m_mapImgProcTasks.end()) inputFileName = it->second->inputFileName;
	}

	if (!inputFileName.empty())
	{
		std::cout << "이미지 처리 시작 " << nTaskId << " " << inputFileName << std::endl;
	}

	return 0;
}

LRESULT CImgProcAmpDlg::OnImgProcTaskCompleted(WPARAM wParam, LPARAM lParam)
{
	uint32_t nTaskId = (uint32_t)wParam;      // 작업이 완료된 작업의 id
	int nResult = static_cast<int>(lParam);   // 작업 결과 - 작업함수 리턴값

	std::shared_ptr<TImgProcTaskState> pState;
	{
		// 등록 스레드와의 map 접근 충돌을 막고 완료된 작업을 목록에서 제거
		std::lock_guard<std::mutex> lock(m_imgProcTaskMutex);
		auto it = m_mapImgProcTasks.find(nTaskId);
		if (it != m_mapImgProcTasks.end())
		{
			pState = it->second;
			m_mapImgProcTasks.erase(it);
		}
	}

	if (!pState)
	{
		std::cout << "작업 내용 찾을 수 없음. 작업 id=" << nTaskId << std::endl;
		return 0;
	}

	// 결과 내용에 따라서 필요한 처리 진행
	if (nResult == IMG_PROC_SUCCESS)
	{
		std::cout << "이미지 처리 완료 " << nTaskId << " "
			<< pState->outputFileName << std::endl;
	}
	else if (nResult == IMG_PROC_CANCELLED)
	{
		std::cout << "이미지 처리 취소 " << nTaskId << std::endl;
	}
	else
	{
		std::cout << "이미지 처리 실패 " << nTaskId << " "
			<< pState->errorMessage << std::endl;;
	}

	const std::size_t finishedCount = m_nImgTaskFinished.fetch_add(1) + 1;

	// ui 동작 - 프로그래스바
	PostMessage(UM_PROGRESS_SET_POS, static_cast<WPARAM>(finishedCount), NULL);

	// 스레드 풀의 모든 작업이 끝났는지 검사
	CheckImgProcBatchCompleted();

	return 0;
}

LRESULT CImgProcAmpDlg::OnImgProcTaskFailed(WPARAM wParam, LPARAM lParam)
{
	// 실패한 작업 id 받음
	const std::uint32_t nTaskId = static_cast<std::uint32_t>(wParam);

	std::shared_ptr<TImgProcTaskState> pState;
	{
		// 등록 스레드와의 map 접근 충돌을 막고 예외가 발생한 작업을 목록에서 제거
		std::lock_guard<std::mutex> lock(m_imgProcTaskMutex);
		auto it = m_mapImgProcTasks.find(nTaskId);
		if (it != m_mapImgProcTasks.end())
		{
			pState = it->second;
			m_mapImgProcTasks.erase(it);
		}
	}

	if (pState)
	{
		std::cerr << "이미지 처리 중 예외 발생 id, file=" 
			<< nTaskId << "," << pState->inputFileName << "\n";

		const std::size_t finishedCount = m_nImgTaskFinished.fetch_add(1) + 1;

		// ui 처리 - 프로그래스바 
		PostMessage(UM_PROGRESS_SET_POS, static_cast<WPARAM>(finishedCount), NULL);
	}

	// 모든 작업 끝났는지 검사
	CheckImgProcBatchCompleted();

	return 0;
}

LRESULT CImgProcAmpDlg::OnImgProcIdle(WPARAM wParam, LPARAM lParam)
{
	std::cout << "모든 이미지 처리 작업이 완료되었습니다." << std::endl;
	
	// 진행 표시 종료
	// 완료 버튼 활성화
	// 결과 목록 갱신

	return 0;
}

LRESULT CImgProcAmpDlg::OnImgTaskEnqueueCompleted(WPARAM wParam, LPARAM lParam)
{
	// 작업 등록이 끝나면 등록용 스레드 join - 등록 완료 상태 확정
	JoinImageTaskEnqueueThread();

	// 전체 작업 중지 처리 중에는 남아 있는 완료 메시지로 후속 작업을 시작하지 않음
	if (!m_bImgBatchRunning) return 0;

	m_bImgTaskEnqueueCompleted = true;
	CheckImgProcBatchCompleted();
	return 0;
}

int CImgProcAmpDlg::ProcessImgProcTask(std::shared_ptr<TImgProcTaskState> pState, TImgProcOption tOption, const std::atomic<bool>& stopRequested)
{
	if (stopRequested == true) return IMG_PROC_CANCELLED;

	{
		// AddTask 직후 작업이 바로 시작되어도 관리 map 등록이 끝난 뒤 처리하도록 대기
		std::unique_lock<std::mutex> lock(pState->mutex);
		pState->enqueueCondition.wait(lock, [&pState, &stopRequested]()
			{
				return pState->bEnqueueRegistered || stopRequested.load();
			});
		if (!pState->bEnqueueRegistered || stopRequested.load()) return IMG_PROC_CANCELLED;
	}

	// 이미지 처리와 결과 파일 저장
	int processResult = ProcessImageFile(pState->inputFileName, pState->outputFileName, tOption);
	bool bSuccess = (processResult == 0);
	{
		std::lock_guard<std::mutex> lock(pState->mutex);

		// 결과 기록
		pState->bSuccess = bSuccess;
		if (!bSuccess)
		{
			pState->errorMessage = "ImgProc.dll 이미지 처리 실패. 오류=" + std::to_string(processResult);
		}

		if (!bSuccess) std::cerr << "img proc process and save fail\n";
	}

	if (bSuccess) return IMG_PROC_SUCCESS;
	return IMG_PROC_FAILED;
}

int CImgProcAmpDlg::ProcessImageFile(const std::string& csInputFileName, 
	const std::string& csOutputFileName, 
	const TImgProcOption& tOption)
{
	switch (m_imgProcInputMode)
	{
	default:
	case EImgProcInputMode::FileName:
		// 이미지 파일 이름으로 입출력 처리
		return ProcessImageByFileName(csInputFileName, csOutputFileName, tOption);

	case EImgProcInputMode::MatRaw:
		// cv::Mat로 입출력 처리  
		return ProcessImageByMatRaw(csInputFileName, csOutputFileName, tOption);

	case EImgProcInputMode::EncodedImage:
		// 인코딩된 이미지 파일 데이터로 입출력 처리
		return ProcessImageByEncodedData(csInputFileName, csOutputFileName, tOption);
	}
}

int CImgProcAmpDlg::ProcessImageByFileName(const std::string& csInputFileName,
	const std::string& csOutputFileName,
	const TImgProcOption& tOption)
{
	try
	{
		// dll 호출함수 로드 점검
		if (m_imgProcDll.ProcessFileImg == nullptr) return -100;

		return m_imgProcDll.ProcessFileImg(csInputFileName.c_str(), csOutputFileName.c_str(), tOption);
	}
	catch (...)
	{
		std::cerr << "process image by file name unknown error\n";
		return -107;
	}
}

int CImgProcAmpDlg::ProcessImageByMatRaw(const std::string& csInputFileName,
	const std::string& csOutputFileName,
	const TImgProcOption& tOption)
{
	// dll 호출함수 로드 점검
	if (m_imgProcDll.ProcessMatRawImg == nullptr ||
		m_imgProcDll.GetProcessedMatRawImg == nullptr ||
		m_imgProcDll.ReleaseResult == nullptr)
	{
		return -101;
	}

	try
	{
		// 이미지를 cv::Mat 형태로 로드 - 입력 이미지
		const cv::Mat matInput = cv::imread(csInputFileName, cv::IMREAD_COLOR);
		if (matInput.empty())
		{
			return -102;
		}

		// 처리 결과를 받기 위한 핸들
		ImageResultHandle rawHandle = nullptr;
		
		// cv::Mat를 원시 데이터 형태로 dll로 전달 - opencv 객체 직접 전달 위험 방지
		const int processResult = m_imgProcDll.ProcessMatRawImg(
			matInput.data,
			matInput.cols,
			matInput.rows,
			matInput.type(),
			matInput.step,
			tOption,
			&rawHandle);

		if (processResult != 0)
		{
			return processResult;
		}
		if (rawHandle == nullptr)
		{
			return -103;
		}

		// 처리 결과 핸들을 이용해서 결과 이미지 데이터 받아옴 - 할당되었던 메모리는 자동 해제
		auto result = MakeImgProcDllResultPtr(m_imgProcDll, rawHandle);
		TImgRetInfo retInfo;
		m_imgProcDll.GetProcessedMatRawImg(result.get(), &retInfo);
		if (retInfo.data == nullptr || retInfo.width <= 0 ||
			retInfo.height <= 0 || retInfo.type < 0)
		{
			return -104;
		}

		// 받아온 결과 데이터를 이용해서 cv::Mat 구성
		const cv::Mat matOutput(
			retInfo.height,
			retInfo.width,
			retInfo.type,
			const_cast<unsigned char*>(retInfo.data));

		// 처리 결과를 이미지 파일로 저장
		return cv::imwrite(csOutputFileName, matOutput) ? 0 : -105;
	}
	catch (const cv::Exception& e)
	{
		std::cerr << "ProcessMatRawImg path exception=" << e.what() << std::endl;
		return -106;
	}
}

int CImgProcAmpDlg::ProcessImageByEncodedData(const std::string& csInputFileName,
	const std::string& csOutputFileName,
	const TImgProcOption& tOption)
{
	// dll 호출함수 로드 점검
	if (m_imgProcDll.ProcessEncodedImg == nullptr ||
		m_imgProcDll.GetProcessedMatRawImg == nullptr ||
		m_imgProcDll.ReleaseResult == nullptr)
	{
		return -108;
	}

	try
	{
		int rt = 0;

		// 이미지 파일 데이터 로드
		std::vector<unsigned char> encodedInput;
		rt = ReadFileToVecBuf(csInputFileName, encodedInput);
		if (rt != 0)
		{
			std::cerr << "process img by encoded data image file load fail=" << rt << "\n";
			return -109;
		}

		// 출력 이미지 파일 형식 설정 - 입력 이미지 파일 형식과 동일하도록 설정
		std::string outputExtension = ToLowerString(fs::path(csOutputFileName).extension().string());
		int outputEncodeType = FORMAT_JPG;
		if (outputExtension == ".png") outputEncodeType = FORMAT_PNG;
		else if (outputExtension == ".bmp") outputEncodeType = FORMAT_BMP;

		// 작업 결과를 가져오기 위한 핸들
		ImageResultHandle rawHandle = nullptr;

		// 이미지 처리
		const int processResult = m_imgProcDll.ProcessEncodedImg(
			encodedInput.data(),
			encodedInput.size(),
			tOption,
			outputEncodeType,
			&rawHandle);

		if (processResult != 0)
		{
			return processResult;
		}
		if (rawHandle == nullptr)
		{
			return -112;
		}

		// 작업 결과 가져오기 - 작업 결과 저장을 위해 dll에서 할당되었던 메모리는 자동 해제
		auto result = MakeImgProcDllResultPtr(m_imgProcDll, rawHandle);
		TImgRetInfo retInfo;
		m_imgProcDll.GetProcessedMatRawImg(result.get(), &retInfo);
		if (retInfo.data == nullptr || retInfo.dataSize == 0 || retInfo.dataType != 1)
		{
			return -113;
		}

		// 이미지 데이터 형식으로 인코딩 되어 있는 작업 결과를 파일로 저장
		rt = WriteFileFromBuf(csOutputFileName, retInfo.data, retInfo.dataSize);
		if (rt != 0)
		{
			std::cerr << "process img by encoded data image file write fail=" << rt << "\n";
			return -114;
		}

		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << "ProcessEncodedImg path exception=" << e.what() << std::endl;
		return -116;
	}
}


bool CImgProcAmpDlg::AddImgProcTask(const std::string& inputFileName,
	const std::string& outputFileName,
	const TImgProcOption& tOption)
{
	// 작업 대상과 결과 전달 변수 생성 - 작업 목록으로 관리
	// pState는 스레드로 구동되는 작업함수로 전달되고 입력 이미지 파일이름, 출력 이미지 파일이름을 작업 함수로 전달
	// 작업이 끝나면 작업 함수에서 pState에 결과 저장
	// shared ptr로 생성 - ui, thread 양쪽에서 소유
	// 작업 끝나면 ui 쪽에서 결과를 가져올 수 있도록 처리, 양쪽 참조 끝나면 메모리 자동 해제
	auto pState = std::make_shared<TImgProcTaskState>();

	pState->inputFileName = inputFileName;   // 원본 이미지 파일 이름
	pState->outputFileName = outputFileName; // 결과 이미지 파일 이름

	// 스레드 풀 작업 큐에 작업 추가
	const std::uint32_t nTaskId = m_threadProc.AddTask(
		[this, pState, tOption](const std::atomic<bool>& stopRequested)
		{
			return ProcessImgProcTask(pState, tOption, stopRequested);
		}
	);
		
	if (nTaskId == 0)
	{
		// 큐가 가득 찬 경우에는 내부에서 빈자리가 생길 때까지 대기하므로,
		// 여기서는 스레드 풀이 중지되었거나 실행 중이 아닌 경우에만 등록 실패
		return false;
	}

	{
		// 등록 전용 스레드와 UI의 완료 메시지 처리 함수가 map을 동시에 접근하지 않도록 보호
		std::lock_guard<std::mutex> lock(m_imgProcTaskMutex);
		m_mapImgProcTasks.emplace(nTaskId, pState);
	}
	m_nImgTaskEnqueued.fetch_add(1);

	{
		// map과 작업 수량 등록이 끝났음을 워커에 알린 뒤 실제 이미지 처리를 진행시킴
		std::lock_guard<std::mutex> lock(pState->mutex);
		pState->bEnqueueRegistered = true;
	}
	pState->enqueueCondition.notify_one();

	return true;
}

// 스레드 풀에 할당된 작업이 끝났는지 검사
void CImgProcAmpDlg::CheckImgProcBatchCompleted()
{
	if (!m_bImgBatchRunning) return;

	// 작업 큐 등록 중이면 완료 아님
	if (!m_bImgTaskEnqueueCompleted.load()) return;

	// 완료 메시지를 받지 않은 작업이 있으면 완료 아님
	if (m_nImgTaskFinished.load() != m_nImgTaskEnqueued.load()) return;

	{
		// 등록 스레드가 map에 작업을 추가할 수 있으므로 목록 확인도 mutex로 보호
		std::lock_guard<std::mutex> lock(m_imgProcTaskMutex);
		if (!m_mapImgProcTasks.empty()) return;
	}

	m_bImgBatchRunning = false;
	OnImgProcTasksCompletedAll();
}

void CImgProcAmpDlg::OnImgProcTasksCompletedAll()
{
	ClosePrepareNotice();
	// 스레드 풀 작업이 모두 끝나면 스레드 풀 종료
	m_threadProc.Stop();

	// 스레드 풀 작업이 모두 끝나면 작업시간 계산
	m_endTm = std::chrono::steady_clock::now();
	double ms = std::chrono::duration<double, std::milli>(m_endTm - m_startTm).count();
	
	// 로그 구성
	m_ossLogAll << MakeLog(ms);

	// ui 처리
	m_ctrlPrgsWork.SetPos(100);
	SetCtrlStatus(WorkStatus::ImgProcEnd);

	// 작업 결과 파일을 이미지 리스트에 설정
	ListFile(m_dstPath, DISP_IMG_LIST_DST);

	// 로그 창에 로그 표시
	PostMessage(UM_EDIT_LOG_SET_TEXT, 0, NULL);
}

void CImgProcAmpDlg::StartImageBatchThreadPool()
{
	// 이전 배치의 등록 스레드 핸들이 남아 있으면 새 등록 스레드를 만들기 전에 정리
	JoinImageTaskEnqueueThread();
	{
		std::lock_guard<std::mutex> lock(m_imgProcTaskMutex);
		m_mapImgProcTasks.clear();
	}

	m_nImgTaskEnqueued.store(0);
	m_nImgTaskFinished.store(0);

	m_imgTaskEnqueueStopRequested.store(false);
	m_bImgTaskEnqueueCompleted.store(false);
	m_bImgBatchRunning = true;
		
	TImgProcOption option;
	
	option.nMaxWidth = 1600;
	option.nMaxHeight = 1600;

	option.dGamma = 0.0; // 자동 감마
	option.dClaheClipLimit = 2.0;
	option.dSharpenAmount = 1.0;

	std::vector<TImgProcFile> vInOutFileList;
	fs::path pathObj("");
	if (!m_vSrcImgFileList.empty())
	{
		TImgProcFile inoutFile;
		std::string dstImg = "";
		for (const std::string& srcImg : m_vSrcImgFileList)
		{
			inoutFile.input = srcImg.c_str();

			dstImg = m_dstPath + "/" + pathObj.assign(srcImg).stem().string() + "_proc.jpg";
			inoutFile.output = dstImg.c_str();

			vInOutFileList.push_back(inoutFile);

			std::cout << "input=" << vInOutFileList.back().input << std::endl;
			std::cout << "output=" << vInOutFileList.back().output << "\n" <<std::endl;
		}	
	}
	else
	{
		ClosePrepareNotice();
		SetCtrlStatus(WorkStatus::WorkStop);
		m_bImgBatchRunning = false;
		AfxMessageBox(_T("처리할 원본 이미지가 없습니다."));
		return;
	}

	try
	{
		// 작업 큐가 가득 차도 UI 스레드는 대기하지 않도록 등록 작업만 별도 싱글 스레드에서 실행
		m_imgTaskEnqueueThread = std::thread(
			&CImgProcAmpDlg::EnqueueImageTasks,
			this,
			std::move(vInOutFileList),
			option);
	}
	catch (...)
	{
		m_bImgBatchRunning = false;
		m_bImgTaskEnqueueCompleted = true;

		// 등록 스레드를 만들지 못한 경우 이미 시작된 스레드 풀도 함께 정리
		m_threadProc.Stop(true, true);
		ClosePrepareNotice();
		SetCtrlStatus(WorkStatus::WorkStop);
		AfxMessageBox(_T("이미지 작업 등록 스레드를 시작하지 못했습니다."));
	}
}


void CImgProcAmpDlg::EnqueueImageTasks(std::vector<TImgProcFile> vInOutFileList, TImgProcOption option)
{
	for (const auto& file : vInOutFileList)
	{
		if (m_imgTaskEnqueueStopRequested.load()) break;

		// 큐가 가득 차면 AddImgProcTask 내부에서 빈자리가 생길 때까지 이 등록 스레드만 대기
		if (!AddImgProcTask(file.input, file.output, option))
		{
			if (!m_imgTaskEnqueueStopRequested.load())
				std::cerr << "작업 큐 등록 실패: " << file.input << "\n";
			break;
		}

		std::cout << "img proc task enqueued" << std::endl;
	}

	// 등록 완료 후의 배치 완료 판단과 UI 처리는 반드시 UI 스레드에서 수행
	PostMessage(UM_IMG_TASK_ENQUEUE_COMPLETED, 0, 0);
}


void CImgProcAmpDlg::JoinImageTaskEnqueueThread()
{
	// 등록 스레드를 detach하지 않고 소유하여 다이얼로그 종료 전에 안전하게 회수
	if (m_imgTaskEnqueueThread.joinable() &&
		m_imgTaskEnqueueThread.get_id() != std::this_thread::get_id())
	{
		m_imgTaskEnqueueThread.join();
	}
}


void CImgProcAmpDlg::StartImageBatchThreadSingle()
{
	TImgProcOption option;

	option.nMaxWidth = 1600;
	option.nMaxHeight = 1600;

	option.dGamma = 0.0; // 자동 감마
	option.dClaheClipLimit = 2.0;
	option.dSharpenAmount = 1.0;

	std::vector<TImgProcFile> vInOutFileList;
	fs::path pathObj("");
	if (!m_vSrcImgFileList.empty())
	{
		TImgProcFile inoutFile;
		std::string dstImg = "";
		for (const std::string& srcImg : m_vSrcImgFileList)
		{
			inoutFile.input = srcImg.c_str();

			dstImg = m_dstPath + "/" + pathObj.assign(srcImg).stem().string() + "_proc.jpg";
			inoutFile.output = dstImg.c_str();

			vInOutFileList.push_back(inoutFile);

			std::cout << "input=" << vInOutFileList.back().input << std::endl;
			std::cout << "output=" << vInOutFileList.back().output << "\n" << std::endl;
		}
	}
	else
	{
		ClosePrepareNotice();
		SetCtrlStatus(WorkStatus::WorkStop);
		m_bImgBatchRunning = false;
		AfxMessageBox(_T("처리할 원본 이미지가 없습니다."));
		return;
	}

	m_taskImgBatchSingleThread = m_threadProc.StartSingleTask(
		[this, vInOutFileList, option](const std::atomic<bool>& stopRequested)
		{
			return ImageBatchThreadSingle(vInOutFileList, option, stopRequested);
		}
	);

	if (m_taskImgBatchSingleThread == 0)
	{
		ClosePrepareNotice();
		SetCtrlStatus(WorkStatus::WorkStop);
		AfxMessageBox(_T("싱글 작업을 시작하지 못했습니다."));
	}
}

int CImgProcAmpDlg::ImageBatchThreadSingle(std::vector<TImgProcFile> vInOutFileList, 
	TImgProcOption option, 
	const std::atomic<bool>& stopRequest)
{
	m_nImgTaskFinished.store(0);

	for (const TImgProcFile& inoutFile : vInOutFileList)
	{
		if (stopRequest == true)
		{
			std::cout << "image batch thread single stop by request" << std::endl;
			break;
		}

		const int processResult = ProcessImageFile(inoutFile.input, inoutFile.output, option);
		if (processResult != 0)
		{
			std::cerr << "ImgProc.dll image processing failed=" << processResult << std::endl;
			return IMG_PROC_FAILED;
		}
		std::cout << "image batch thread process image" << std::endl;

		const std::size_t finishedCount = m_nImgTaskFinished.fetch_add(1) + 1;

		// ui 동작 - 프로그래스바
		PostMessage(UM_PROGRESS_SET_POS, static_cast<WPARAM>(finishedCount), NULL);
	}

	if (stopRequest == true) return -1;
	else return 0;
}

std::string CImgProcAmpDlg::MakeLog(const double workTime)
{
	// 로그 구성
	std::string workName = "";
	if (m_workMode == WorkMode::Sequencial) workName = "싱글 스레드 작업 종료";
	else workName = "스레드 풀 작업 종료";

	std::string opencvThreadUse = "";
	if (m_opencvTh == 1) opencvThreadUse = "사용";
	else opencvThreadUse = "사용안함";

	std::string dllInputMode;
	switch (m_imgProcInputMode)
	{
	default:
	case EImgProcInputMode::FileName:
		dllInputMode = "ProcessFileImg";
		break;
	case EImgProcInputMode::MatRaw:
		dllInputMode = "ProcessMatRawImg";
		break;
	case EImgProcInputMode::EncodedImage:
		dllInputMode = "ProcessEncodedImg";
		break;
	}

	std::ostringstream ossLog;
	ossLog << "[" << GetCurrentDateTime(true) << "] " << workName <<"\r\n";
	ossLog << "작업 이미지 수량 : " << m_nImgTaskFinished.load() << "개" << "\r\n";

	if (m_workMode == WorkMode::ThreadPool) 
		ossLog << "스레드 개수 : " << m_workThreadNum << "개" << "\r\n";

	ossLog << "OpenCV 내부 스레드 : " << opencvThreadUse << "\r\n";
	ossLog << "DLL 입력 방식 : " << dllInputMode << "\r\n";
	ossLog << "작업 시간 : " << workTime << " ms" << "\r\n";
	
	ossLog << "---------------------------------------------------------" << "\r\n";
	
	std::cout << ossLog.str() << std::endl;

	return ossLog.str();
}

void CImgProcAmpDlg::SetThreadNumOption(CComboBox& ctrlComboBox)
{
	// 시스템 스레드 수 파악
	unsigned int supportThreadNum = std::thread::hardware_concurrency();
	if (supportThreadNum == 0) supportThreadNum = 4;

	m_vWorkThreadNum.clear();

	char cmbText[128] = {};
	unsigned int workThreadNum = 0;
	float weight[] = { 3.0f, 2.0f, 1.5f, 1.0f, 0.8f, 0.5f };

	for (int i = 0; i < (int)std::size(weight); i++)
	{
		workThreadNum = (unsigned int)(supportThreadNum * weight[i]);
		snprintf(cmbText, sizeof(cmbText), "%d (CPU 지원 스레드 수 X %.1f)", workThreadNum, weight[i]);
		ctrlComboBox.AddString(cmbText);
		m_vWorkThreadNum.push_back(workThreadNum);
	}

	ctrlComboBox.SetCurSel(0);
}

//-------------------------------------------

LRESULT CImgProcAmpDlg::OnSingleTaskCompleted(WPARAM wParam, LPARAM lParam)
{
	const std::uint32_t nTaskId = static_cast<std::uint32_t>(wParam);
	const int nResult = static_cast<int>(lParam);
	
	if (nTaskId == m_taskImgFileListSrc)
	{
		// 원본 이미지 목록 만들기 작업이 끝난 경우
		m_taskImgFileListSrc = 0;

		if (nResult != 0)
		{
			ClosePrepareNotice();
			SetCtrlStatus(WorkStatus::WorkStop);
			AfxMessageBox(_T("이미지 목록 검색에 실패했습니다."));
			return 0;
		}

		// 화면에 원본 이미지 표시
		m_ctrlPrgsWork.SetPos(100);
		m_pImgListSrc->Invalidate(FALSE);

		//----------------------------------------
		// 후속작업 진행 - 이미지 처리 작업

		// 작업 모드 설정 - 싱글 스레드, 스레드 풀
		if (m_ctrlRdoSeq.GetCheck() == 1) m_workMode = WorkMode::Sequencial;
		else m_workMode = WorkMode::ThreadPool;

		// opencv 내부 스레드 수 설정
		if (m_ctrlChkOpencvTh.GetCheck() == 0) cv::setNumThreads(1);
		else  cv::setNumThreads(0);

		m_ctrlPrgsWork.SetRange(0, (int)m_vSrcImgFileList.size());
		m_ctrlPrgsWork.SetPos(0);

		if (m_workMode == WorkMode::Sequencial)
		{
			// 싱글 스레드로 이미지 처리작업 시작
			m_startTm = std::chrono::steady_clock::now();
			StartImageBatchThreadSingle();
		}
		else
		{
			// 스레드 풀에서 사용할 스레드 개수 설정
			m_workThreadNum = m_vWorkThreadNum[m_ctrlCmbThrSet.GetCurSel()];

			//if (m_ctrlCmbThrSet.GetCurSel() == 0) m_workThreadNum = m_sysThreadNum - 1;
			//else m_workThreadNum = m_sysThreadNum / 2;

			// 기존에 설정되어있는 스레드 수량과 다르면 새로운 값으로 설정
			if (m_threadProc.GetThreadCount() != m_workThreadNum)
			{
				m_threadProc.Stop();
				m_threadProc.SetThreadCount(m_workThreadNum);
			}

			std::cout << "thread pool thread num=" << m_workThreadNum << std::endl;
			
			if (!m_threadProc.IsRunning())
			{
				if (!m_threadProc.Start())
				{
					ClosePrepareNotice();
					SetCtrlStatus(WorkStatus::WorkStop);
					AfxMessageBox(_T("이미지 처리 스레드풀을 시작하지 못했습니다."));
					return 0;
				}
			}

			// 스레드 풀로 이미지 처리작업 시작
			m_startTm = std::chrono::steady_clock::now();
			StartImageBatchThreadPool();
		}
	}
	else if (nTaskId == m_taskImgFileListDst)
	{
		// 이미지 처리가 끝나고 결과 이미지 목록 만들기 작업이 끝난 경우
		// 프로그래스바로 작업 끝남을 알리고 결과 이미지를 화면에 표시
		int low = 0;
		int high = 0;
		m_ctrlPrgsWork.GetRange(low, high);
		m_ctrlPrgsWork.SetPos(high);
		m_pImgListDst->Invalidate(FALSE);
	}
	else if (nTaskId == m_taskImgBatchSingleThread)
	{
		// 싱글 스레드 작업이 끝나면 작업시간 계산 - 스레드 풀 작업 종료는 OnImgProcTasksCompleteAll
		m_endTm = std::chrono::steady_clock::now();
		double ms = std::chrono::duration<double, std::milli>(m_endTm - m_startTm).count();

		// 로그 구성
		m_ossLogAll << MakeLog(ms);

		// 작업 결과 파일을 이미지 리스트에 설정
		ListFile(m_dstPath, DISP_IMG_LIST_DST);

		// 로그 창에 로그 표시
		PostMessage(UM_EDIT_LOG_SET_TEXT, 0, NULL);

		SetCtrlStatus(WorkStatus::ImgProcEnd);
	}
	else
	{
		;
	}
	
	return 0;
}

LRESULT CImgProcAmpDlg::OnSingleTaskStarted(WPARAM wParam, LPARAM lParam)
{
	if (m_taskImgBatchSingleThread != 0 && wParam == m_taskImgBatchSingleThread)
		ClosePrepareNotice();
	std::cout << "싱글 스레드 작업이 시작되었습니다." << std::endl;
	return 0;
}

LRESULT CImgProcAmpDlg::OnSingleTaskFailed(WPARAM wParam, LPARAM lParam)
{
	if (wParam == m_taskImgFileListSrc || wParam == m_taskImgBatchSingleThread)
	{
		ClosePrepareNotice();
		SetCtrlStatus(WorkStatus::WorkStop);
		AfxMessageBox(_T("작업을 실행하지 못했습니다."));
	}
	std::cout << "싱글 스레드 작업이 실패했습니다." << std::endl;
	return 0;
}

LRESULT CImgProcAmpDlg::OnProgressSetPos(WPARAM wParam, LPARAM lParam)
{
	int pos = (int)wParam;
	m_ctrlPrgsWork.SetPos(pos);
	return 0;
}

LRESULT CImgProcAmpDlg::OnProgressSetRange(WPARAM wParam, LPARAM lParam)
{
	int rangeMax = (int)(wParam);
	m_ctrlPrgsWork.SetRange(0, rangeMax);
	return 0;
}

LRESULT CImgProcAmpDlg::OnEditLogSetText(WPARAM wParam, LPARAM lParam)
{
	m_ctrlEditLog.SetWindowText(m_ossLogAll.str().c_str());

	int length = m_ctrlEditLog.GetWindowTextLength();
	m_ctrlEditLog.SetSel(length, length);
	m_ctrlEditLog.SendMessage(EM_SCROLLCARET);

	return 0;
}


