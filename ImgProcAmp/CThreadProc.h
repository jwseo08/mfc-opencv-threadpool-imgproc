#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// 스레드 풀 작업 알림 메시지
#define WM_CTHREADPROC_STARTED        (WM_APP + 1001) // 작업 시작, wparam - 시작된 워커 스레드 수
#define WM_CTHREADPROC_TASK_STARTED   (WM_APP + 1002) // 개별 작업 1개 시작, wparam - 시작된 작업 id
#define WM_CTHREADPROC_TASK_COMPLETED (WM_APP + 1003) // 개별 작업 1개 끝남, wparam - 끝난 작업 id
#define WM_CTHREADPROC_TASK_FAILED    (WM_APP + 1004) // 개별 작업 1개 실패, wparam - 실패 작업 id
#define WM_CTHREADPROC_IDLE           (WM_APP + 1005) // 스레드 풀에서 실행 중인 작업 없음
#define WM_CTHREADPROC_STOP_REQUESTED (WM_APP + 1006) // 종료 요청 들어옴
#define WM_CTHREADPROC_STOPPED        (WM_APP + 1007) // 스레드 풀 동작 종료됨

// 싱글 스레드 작업 알림 메시지 - 1회 동작 스레드
#define WM_CTHREADPROC_SINGLE_TASK_STARTED   (WM_APP + 1008) // 작업 시작, wparam - 작업 id
#define WM_CTHREADPROC_SINGLE_TASK_COMPLETED (WM_APP + 1009) // 작업 끝남, wparam - 작업 id, lparam - 작업함수 리턴값
#define WM_CTHREADPROC_SINGLE_TASK_FAILED    (WM_APP + 1010) // 작업 실패, wparam - 작업 id

// 스레드 풀과 단순 스레드 기능 제공 클래스
class CThreadProc
{
public:
	// 스레드로 구동시킬 함수 타입 정의
	using Task = std::function<int(const std::atomic<bool>& stopRequested)>;
	using SimpleTask = std::function<void()>;

	// 객체 생성 시 스레드 풀의 스레드와 작업 큐 수량 설정
	explicit CThreadProc(std::size_t threadCount = 1, std::size_t maxQueueSize = 100);
	~CThreadProc();

	// CThreadProc 객체 복사, 복사 대입 금지 - 안정성 확보
	CThreadProc(const CThreadProc&) = delete;
	CThreadProc& operator=(const CThreadProc&) = delete;

	// 스레드 풀에서 사용하는 스레드, 작업 큐 수량 설정 
	bool SetThreadCount(std::size_t threadCount);
	std::size_t GetThreadCount() const noexcept;
	bool SetMaxQueueSize(std::size_t maxQueueSize);

	//----------------------------------------------------------

	// 스레드 풀 시작
	bool Start();
	
	// 스레드 풀 종료
	void Stop(bool wait = true, bool cancelPending = true);

	// 스레드 풀 작업 중지 - 호출하는 쪽에서 스레드 풀로 중지 요청 시 사용
	void RequestStop(bool cancelPending = true);
	void Wait();

	// 스레드 풀 큐에 작업 추가
	std::uint32_t AddTask(Task task);
	std::uint32_t AddTask(SimpleTask task);

	// 스레드 풀 상태 확인 반환 - 동작 중, 중지 요청 받음 상태
	bool IsRunning() const noexcept;
	bool IsStopRequested() const noexcept;

	// 작업 큐에 등록되어있고 아직 처리되지 않은 작업 수량 반환
	std::size_t GetPendingTaskCount() const;

	//----------------------------------------------------------

	// 싱글 스레드 시작 - 스레드풀과 별개, Start 필요 없음
	std::uint32_t StartSingleTask(Task task);
	std::uint32_t StartSingleTask(SimpleTask task);

	void RequestSingleTaskStop();                    // 싱글 스레드 중지 요청
	void StopSingleTask(bool wait = true);           // 싱글 스레드 중지
	void WaitSingleTask();                           // 싱글 스레드 작업이 끝날 때까지 대기하고 자원 정리
	bool IsSingleTaskRunning() const noexcept;       // 싱글 스레드 동작 상태 반환 - 동작 중인지
	bool IsSingleTaskStopRequested() const noexcept; // 싱글 스레드 동작 상태 반환 - 중지 요청을 받았는지

	// 싱글 스레드로 SimpleTask 형태의 작업을 실행 - StartSingleTask(SimpleTask)의 서브 함수
	int RunSimpleTask(SimpleTask function, const std::atomic<bool>& stopRequested);

	//----------------------------------------------------------
	
	// 메시지를 보낼 윈도우 설정, 가져오기
	void SetNotifyWindow(HWND notifyWindow);
	HWND GetNotifyWindow() const noexcept;

private:
	// 작업 요소
	struct TaskItem
	{
		std::uint32_t id = 0; // 작업 id
		Task taskFunction;    // 실행할 작업 함수
	};

	std::size_t m_threadCount;    // 스레드 풀에서 사용하는 스레드 수량
	std::size_t m_maxQueueSize;   // 스레드 풀에서 사용하는 작업 큐 수량
	std::size_t m_activeTaskCount = 0;  // 스레드 풀에서 처리 중인 작업 수량
	std::size_t m_liveWorkerCount = 0;  // 스레드 풀에서 실행 중인 스레드 수량
	std::atomic<bool> m_running{ false };          // 동작 중인지 상태 표시 flag
	std::atomic<bool> m_stopRequested{ false };    // 중지 요청 flag - 작업 함수에 인자로 전달됨
	std::atomic<bool> m_cancelPending{ true };
	std::atomic<uint32_t> m_nextTaskId{ 1 };

	mutable std::mutex m_singleMutex;             // 싱글 스레드 동작 시 부가 작업에 대한 안전 장치
	std::thread m_singleWorker;                   // 싱글 스레드 객체
	std::atomic<bool> m_singleRunning{ false };   // 싱글 스레드가 동작중인지 표시
	std::atomic<bool> m_singleStopRequested{ false };  // 싱글 스레드 작업 중지 플래그 - 작업 함수에 인자로 전달됨

	HWND m_notifyWindow = nullptr;      // 메시지 송신 대상 윈도우

private:
	mutable std::mutex m_mutex;              // 스레드 풀 동작 시 부가 작업의 안전을 위한 mutex
	std::condition_variable m_taskCondition; // 스레드 풀을 구성하는 스레드의 대기와 시작 제어
	std::queue<TaskItem> m_tasks;            // 스레드 풀 작업 목록 - 작업 큐
	std::vector<std::thread> m_workers;      // 스레드 풀을 구성하는 스레드 목록

private:
	// 스레드 풀 작업 실행 - 작업 큐에 추가된 작업 처리
	void WorkerLoop(); 

	// 싱글 스레드 작업 실행
	void SingleTaskLoop(std::uint32_t taskId, Task task); 

	// 대상 윈도우에 메시지 송신
	void PostNotify(UINT msg, WPARAM wParam = 0, LPARAM lParam = NULL) const;
	
	void JoinWorkers();      // 스레드 풀에 등록된 모든 worker에 대해서 종료 대기와 thread 목록 정리
	void JoinSingleWorker(); // 싱글 스레드 worker 종료 대기

	
};

