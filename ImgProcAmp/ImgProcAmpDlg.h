
// ImgProcAmpDlg.h: 헤더 파일
//

#pragma once

#include <iostream>
#include <string>
#include <atomic>
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
#include "CImgProc.h"
#include "def.h"

namespace fs = std::filesystem;

// ui 동작 메시지
#define UM_PROGRESS_SET_POS    (WM_USER + 1001)
#define UM_PROGRESS_SET_RANGE  (WM_USER + 1002)
#define UM_EDIT_LOG_SET_TEXT   (WM_USER + 1003)
#define UM_STC_WORK_STATUS_SET_TEXT   (WM_USER + 1004)

// 작업 상태
enum EImgProcTaskResult
{
	IMG_PROC_SUCCESS = 0,
	IMG_PROC_CANCELLED = 1,
	IMG_PROC_FAILED = 2
};

// 
struct TImgProcTaskState
{
	// 입력 파일 이름, 출력 파일 이름, 에러 메시지
	CString csInputFileName;
	CString csOutputFileName;
	CString csErrorMessage;

	// 작업 결과
	bool bSuccess = false;
	
	// 작업 결과 수신 충돌 방지 - 안전장치
	std::mutex mutex;
};

// 이미지 작업 입력 파일, 출력 파일
const struct TImgProcFile
{
	CString pszInput = _T("");
	CString pszOutput = _T("");
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
	CModRadioBtn m_ctrlRdoThp;      // 스레드 풀 처리 선택 라이도 버튼
	CModCheckBox m_ctrlChkOpencvTh; // opencv 스레드 사용 여부 체크 박스
	CBrush m_BkBrush;               // 다아얼로그 배경 색상 브러시

private:
	std::string m_srcPath;   // 원본 이미지 폴더 경로
	std::string m_dstPath;   // 결과 이미지 저장 폴더 경로

	std::vector<std::string> m_vSrcImgFileList;   // 이미지 처리 대상인 원본 이미지 파일 이름 목록
	std::vector<std::string> m_vDstImgFileList;   // 결과 이미지 파일 이름 목록

	WorkMode m_workMode;    // 이미지 처리 모드 - 순차처리, 스레드 풀 처리
	int m_opencvTh;         // opencv 내부 스레드 사용 여부
	int m_sysThreadNum;     // cpu에서 지원하는 스레드 수량
	int m_workThreadNum;    // 스레드 풀 모드에서 사용할 스레드 수량

	uint32_t m_taskImgFileListSrc;        // 원본 이미지 파일 목록 작성 작업 아이디
	uint32_t m_taskImgFileListDst;        // 결과 이미지 파일 목록 작성 작업 아이디
	uint32_t m_taskImgBatchSingleThread;  // 이미지 순차 처리 작업 아이디
	
	
private:
	// list file and diplay 함수를 스레드로 구동
	void ListFile(const std::string& path, const int displayImgList);
	
	// 폴더에 있는 이미지 파일 이름 리스트를 만들고 이미지 리스트 컨트롤에 로드
	int ListFileAndDisplay(const std::string& path, const int displayImgList, const std::atomic<bool>& stopRequest);

	// 작업 단계에 따라서 컨트롤 상태 변경
	void SetCtrlStatus(const WorkStatus& status);

private:
	CThreadProc m_threadProc; // 일반 싱글 스레드, 스레드 풀 기능 제공 클래스 객체
	
	// 스레드 풀 작업 시 각각의 작업 id를 키로 지정해서 해당 작업의 결과 세부 내용 관리
	std::map<std::uint32_t, std::shared_ptr<TImgProcTaskState>> m_mapImgProcTasks; 

	// 스레드 풀에 작업 할당
	bool AddImgProcTask(const CString& csInputFileName,  
		const CString& csOutputFileName, 
		const TImgProcOption& tOption);
	// 스레드 풀 사용 이미지 처리 작업 - 작업 시작
	void StartImageBatchThreadPool();                   
	// 스레드 풀 사용 이미지 처리 작업 - 실제 처리 함수
	int ProcessImgProcTask(std::shared_ptr<TImgProcTaskState> pState, 
		TImgProcOption tOption, 
		const std::atomic<bool>& stopRequested);

	// 싱글 스레드 사용 이미지 처리 작업 - 실제 처리 함수
	int ImageBatchThreadSingle(std::vector<TImgProcFile> vInOutFileList, 
		TImgProcOption option, 
		const std::atomic<bool>& stopRequest);
	// 싱글 스레드 사용 이미지 처리 작업 - 작업 시작
	void StartImageBatchThreadSingle();
	
	// 처리해야 하는 작업 수량과 처리가 끝난 작업 수량
	std::size_t m_nImgTaskSubmitted = 0;
	std::size_t m_nImgTaskFinished = 0;

	// 스레드 풀에 작업 등록 완료 여부 - true 면 작업 등록이 끝남, 더 이상 등록하지 않음
	bool m_bImgTaskSubmissionCompleted = false;
	
	// 스레드 풀 동작 중 여부
	bool m_bImgBatchRunning = false;

	// 스레드 풀에 할당된 작업이 모두 끝났는지 검사
	void CheckImgProcBatchCompleted();
	void OnImgProcTasksCompletedAll();

	// 로그 내용 구성
	std::string MakeLog(const double workTime);

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

	afx_msg LRESULT OnImgProcTaskStarted(WPARAM wParam, LPARAM lParam);    // 스레드 풀의 작업 1개 시작 - 각각의 task에 대한 메시지
	afx_msg LRESULT OnImgProcTaskCompleted(WPARAM wParam, LPARAM lParam);  // 스레드 풀의 작업 1개 마침 - 각각의 task에 대한 메시지
	afx_msg LRESULT OnImgProcTaskFailed(WPARAM wParam, LPARAM lParam);     // 스레드 풀의 작업 1개 실패 - 각각의 task에 대한 메시지
	afx_msg LRESULT OnImgProcIdle(WPARAM wParam, LPARAM lParam);           // 스레드 풀에서 처리할 작업 없음

	afx_msg LRESULT OnSingleTaskCompleted(WPARAM wParam, LPARAM lParam);   // 싱글 스레드 작업 마침
	afx_msg LRESULT OnSingleTaskStarted(WPARAM wParam, LPARAM lParam);     // 싱글 스레드 작업 시작
	afx_msg LRESULT OnSingleTaskFailed(WPARAM wParam, LPARAM lParam);      // 싱글 스레드 작업 실패
	
	afx_msg LRESULT OnProgressSetPos(WPARAM wParam, LPARAM lParam);        // 프로그래스바 값 지정
	afx_msg LRESULT OnProgressSetRange(WPARAM wParam, LPARAM lParam);      // 프로그래스바 범위 지정
	afx_msg LRESULT OnEditLogSetText(WPARAM wParam, LPARAM lParam);        // 로그 표시 컨트롤에 로그 표출

};
