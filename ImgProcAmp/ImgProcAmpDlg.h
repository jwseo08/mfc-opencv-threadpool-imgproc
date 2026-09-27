
// ImgProcAmpDlg.h: 헤더 파일
//

#pragma once

#include <iostream>
#include <string>
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>

#include "CFormImgList.h"
#include "CModiButton.h"
#include "CModRadioBtn.h"
#include "CModCheckBox.h"
#include "CThreadProc.h"
#include "ImgProcDll.h"
#include "def.h"

namespace fs = std::filesystem;

// ui 동작 메시지
#define UM_PROGRESS_SET_POS    (WM_USER + 1001)         // 작업 진행 표시 프로그래스바 값 지정
#define UM_PROGRESS_SET_RANGE  (WM_USER + 1002)         // 작업 진행 표시 프로그래스바 범위 지정
#define UM_EDIT_LOG_SET_TEXT   (WM_USER + 1003)         // 로그 표시 에디트 박스에 텍스트 표시
#define UM_STC_WORK_STATUS_SET_TEXT   (WM_USER + 1004)  // 프로그래스바 타이틀 텍스트 변경 
#define UM_IMG_TASK_ENQUEUE_COMPLETED (WM_USER + 1005)  // 이미지 작업 큐 등록 스레드 완료 알림

// 작업 상태
enum EImgProcTaskResult
{
	IMG_PROC_SUCCESS = 0,
	IMG_PROC_CANCELLED = 1,
	IMG_PROC_FAILED = 2
};

enum class EImgProcInputMode
{
	FileName = 0,
	MatRaw,
	EncodedImage
};

// 
struct TImgProcTaskState
{
	// 입력 파일 이름, 출력 파일 이름, 에러 메시지
	std::string inputFileName;
	std::string outputFileName;
	std::string errorMessage;

	// 작업 결과
	bool bSuccess = false;
	
	// 작업 결과 수신 충돌 방지 - 안전장치
	std::mutex mutex;

	// 작업 관리 목록에 등록되기 전에 워커가 처리를 끝내는 경쟁 상태를 방지하기 위한 등록 완료 신호
	std::condition_variable enqueueCondition;
	bool bEnqueueRegistered = false;
};

// 이미지 작업 입력 파일, 출력 파일
const struct TImgProcFile
{
	std::string input = "";
	std::string output = "";
};

// 작업 준비 알림 창 클래스
class CPrepareNotice : public CWnd
{
protected:
	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override
	{
		if (message == WM_CLOSE ||
			(message == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_CLOSE))
		{
			return 0;
		}

		return CWnd::WindowProc(message, wParam, lParam);
	}
};

// 메인 다이얼로그
class CImgProcAmpDlg : public CDialogEx
{
// 생성입니다.
public:
	CImgProcAmpDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IMGPROCAMP_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

private:
	CFormImgList* m_pImgListSrc;	// 원본 이미지 리스트 컨트롤
	CFormImgList* m_pImgListDst;    // 출력 이미지 리스트 컨트롤
	CModiButton m_btnSearchPathSrc; // 원본 이미지 경로 찾기 버튼
	CModiButton m_btnClose;         // 프로그램 종료 버튼
	CModiButton m_btnStart;         // 작업 시작 버튼
	CModiButton m_btnStop;          // 작업 중지 버튼
	CModiButton m_btnLogSave;       // 로그 저장 버튼
	CModRadioBtn m_ctrlRdoSeq;      // 순차 처리 선택 라디오 버튼
	CModRadioBtn m_ctrlRdoThp;      // 스레드 풀 처리 선택 라디오 버튼
	CModCheckBox m_ctrlChkOpencvTh; // opencv 스레드 사용 여부 체크 박스
	CComboBox m_ctrlCmbImgProcMode; // DLL 이미지 입출력 방식 선택 콤보 박스
	CBrush m_BkBrush;               // 다아얼로그 배경 색상 브러시

private:
	// 준비 중 알림 창
	CPrepareNotice m_prepareNotice;
	CStatic m_prepareNoticeText;
	bool ShowPrepareNotice();   // 알림 창 보이기
	void ClosePrepareNotice();  // 알림 창 닫기

private:
	std::string m_srcPath;   // 원본 이미지 폴더 경로
	std::string m_dstPath;   // 결과 이미지 저장 폴더 경로

	std::vector<std::string> m_vSrcImgFileList;   // 이미지 처리 대상인 원본 이미지 파일 이름 목록
	std::vector<std::string> m_vDstImgFileList;   // 결과 이미지 파일 이름 목록

	WorkMode m_workMode;    // 이미지 처리 모드 - 순차처리, 스레드 풀 처리
	int m_opencvTh;         // opencv 내부 스레드 사용 여부
	EImgProcInputMode m_imgProcInputMode;
	int m_sysThreadNum;     // cpu에서 지원하는 스레드 수량
	int m_workThreadNum;    // 스레드 풀 모드에서 사용할 스레드 수량

	uint32_t m_taskImgFileListSrc;        // 원본 이미지 파일 목록 작성 작업 아이디
	uint32_t m_taskImgFileListDst;        // 결과 이미지 파일 목록 작성 작업 아이디
	uint32_t m_taskImgBatchSingleThread;  // 이미지 순차 처리 작업 아이디
	
	// 옵션에 따라서 스레드 풀에 사용할 스레드 수 리스트
	std::vector<unsigned int> m_vWorkThreadNum;

private:
	// list file and diplay 함수를 스레드로 구동
	void ListFile(const std::string& path, const int displayImgList);
	
	// 폴더에 있는 이미지 파일 이름 리스트를 만들고 이미지 리스트 컨트롤에 로드
	int ListFileAndDisplay(const std::string& path, const int displayImgList, const std::atomic<bool>& stopRequest);

	// 작업 단계에 따라서 컨트롤 상태 변경
	void SetCtrlStatus(const WorkStatus& status);

private:
	CThreadProc m_threadProc; // 일반 싱글 스레드, 스레드 풀 기능 제공 클래스 객체
	TImgProcDllFunc m_imgProcDll; // 동적으로 로드한 이미지 처리 DLL 함수
	
	// 스레드 풀 작업 시 각각의 작업 id를 키로 지정해서 해당 작업의 결과 세부 내용 관리
	std::map<std::uint32_t, std::shared_ptr<TImgProcTaskState>> m_mapImgProcTasks;
	// 작업 등록 스레드와 UI 메시지 처리 함수가 작업 목록을 동시에 접근하지 않도록 보호
	std::mutex m_imgProcTaskMutex;

	// 스레드 풀에 작업 할당
	bool AddImgProcTask(const std::string& inputFileName,  
		const std::string& outputFileName,
		const TImgProcOption& tOption);
	
	// 스레드 풀 사용 이미지 처리 작업 - 작업 시작
	void StartImageBatchThreadPool();   

	// 스레드 풀 작업 등록 진행 동안 UI가 멈추지 않도록 작업을 큐에 등록하는 전용 싱글 스레드와 실행 함수
	std::thread m_imgTaskEnqueueThread;
	std::atomic<bool> m_imgTaskEnqueueStopRequested{ false };
	void EnqueueImageTasks(std::vector<TImgProcFile> vInOutFileList, TImgProcOption option);
	void JoinImageTaskEnqueueThread();

	// 스레드 풀 사용 이미지 처리 작업
	int ProcessImgProcTask(std::shared_ptr<TImgProcTaskState> pState, 
		TImgProcOption tOption, 
		const std::atomic<bool>& stopRequested);
	
	// 이미지 처리 작업 - 실제 처리 함수
	// 내부에서 파일이름으로 입출력과 cv::Mat로 입출력 분기
	int ProcessImageFile(const std::string& csInputFileName,
		const std::string& csOutputFileName,
		const TImgProcOption& tOption);
		
	//  ProcessImageFile의 서브 함수 - 파일 이름으로 입출력
	int ProcessImageByFileName(const std::string& csInputFileName, 
		const std::string& csOutputFileName, 
		const TImgProcOption& tOption);

	//  ProcessImageFile의 서브 함수 - cv::Mat로 입출력
	int ProcessImageByMatRaw(const std::string& csInputFileName,
		const std::string& csOutputFileName,
		const TImgProcOption& tOption);

	// ProcessImageFile의 서브 함수 - 인코딩된 이미지 파일 데이터로 입출력
	int ProcessImageByEncodedData(const std::string& csInputFileName,
		const std::string& csOutputFileName,
		const TImgProcOption& tOption);

	// 싱글 스레드 사용 이미지 처리 작업 - 작업 시작
	void StartImageBatchThreadSingle();

	// 싱글 스레드 사용 이미지 처리 작업 - 실제 처리 함수
	int ImageBatchThreadSingle(std::vector<TImgProcFile> vInOutFileList, 
		TImgProcOption option, 
		const std::atomic<bool>& stopRequest);
	
	// 등록 스레드와 UI 스레드에서 동시에 접근하므로 작업 수량을 atomic으로 관리
	std::atomic<std::size_t> m_nImgTaskEnqueued{ 0 };
	std::atomic<std::size_t> m_nImgTaskFinished{ 0 };

	// 스레드 풀에 작업 큐 등록 완료 여부 - true면 더 이상 등록할 작업이 없음
	std::atomic<bool> m_bImgTaskEnqueueCompleted{ false };
	
	// 스레드 풀 동작 중 여부
	bool m_bImgBatchRunning = false;

	// 스레드 풀에 할당된 작업이 모두 끝났는지 검사
	void CheckImgProcBatchCompleted();
	void OnImgProcTasksCompletedAll();

	// 로그 내용 구성
	std::string MakeLog(const double workTime);

	// 스레드 풀 사용 스레드 수 옵션 설정
	void SetThreadNumOption(CComboBox& ctrlComboBox);

private:
	std::chrono::steady_clock::time_point m_startTm;  // 작업 시작 시간 - 작업시간 측정용
	std::chrono::steady_clock::time_point m_endTm;    // 작업 종료 시간 - 작업시간 측정용
	std::ostringstream m_ossLogAll; // 작업 로그 집합
	std::string m_logFilePathFile;  // 저장할 로그 파일 이름


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	
	CEdit m_ctrlEditPathSrc;      // 원본 경로 표시 에디트 컨트롤
	CEdit m_ctrlEditLog;          // 로그 표시 에디트 컨트롤
	CProgressCtrl m_ctrlPrgsWork; // 작업 진행 표시 프로그래스 바 컨트롤
	CComboBox m_ctrlCmbThrSet;    // 스레드 수 선택 콤보 컨트롤
	CStatic m_ctrlStcWorkStatus;

	afx_msg void OnDestroy();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnClose();
	
	afx_msg void OnBnClickedBtnSearchSrc();  // 원본 이미지 경로 찾기 버튼 동작
	afx_msg void OnBnClickedBtnStart();      // 이미지 처리 시작 버튼 동작
	afx_msg void OnBnClickedBtnStop();       // 이미지 처리 중지 버튼 동작
	afx_msg void OnBnClickedBtnLogSave();    // 로그 저장 버튼 동작
	afx_msg void OnBnClickedBtnExit();       // 프로그램 종료 버튼 동작
	afx_msg void OnBnClickedRadioSeq();      // 순차 처리 선택 
	afx_msg void OnBnClickedRadioThp();      // 스레드 풀 처리 선택 
	afx_msg void OnBnClickedChkOpencvTh();   // opencv 내부 스레드 사용 여부 선택
	afx_msg void OnCbnSelchangeImgProcMode(); // DLL 이미지 입출력 방식 선택

	afx_msg LRESULT OnImgProcTaskStarted(WPARAM wParam, LPARAM lParam);    // 스레드 풀의 작업 1개 시작 - 각각의 task에 대한 메시지
	afx_msg LRESULT OnImgProcTaskCompleted(WPARAM wParam, LPARAM lParam);  // 스레드 풀의 작업 1개 마침 - 각각의 task에 대한 메시지
	afx_msg LRESULT OnImgProcTaskFailed(WPARAM wParam, LPARAM lParam);     // 스레드 풀의 작업 1개 실패 - 각각의 task에 대한 메시지
	afx_msg LRESULT OnImgProcIdle(WPARAM wParam, LPARAM lParam);           // 스레드 풀에서 처리할 작업 없음
	afx_msg LRESULT OnImgTaskEnqueueCompleted(WPARAM wParam, LPARAM lParam); // 이미지 작업 큐 등록 스레드 완료

	afx_msg LRESULT OnSingleTaskCompleted(WPARAM wParam, LPARAM lParam);   // 싱글 스레드 작업 마침
	afx_msg LRESULT OnSingleTaskStarted(WPARAM wParam, LPARAM lParam);     // 싱글 스레드 작업 시작
	afx_msg LRESULT OnSingleTaskFailed(WPARAM wParam, LPARAM lParam);      // 싱글 스레드 작업 실패
	
	afx_msg LRESULT OnProgressSetPos(WPARAM wParam, LPARAM lParam);        // 프로그래스바 값 지정
	afx_msg LRESULT OnProgressSetRange(WPARAM wParam, LPARAM lParam);      // 프로그래스바 범위 지정
	afx_msg LRESULT OnEditLogSetText(WPARAM wParam, LPARAM lParam);        // 로그 표시 컨트롤에 로그 표출

};
