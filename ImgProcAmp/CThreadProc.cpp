#include "pch.h"
#include "CThreadProc.h"

#include <algorithm>
#include <exception>
#include <utility>


CThreadProc::CThreadProc(std::size_t threadCount, std::size_t maxQueueSize)
	: m_threadCount((std::max)(std::size_t{ 1 }, threadCount)),
	m_maxQueueSize((std::max)(std::size_t{ 1 }, maxQueueSize))
{
}

CThreadProc::~CThreadProc()
{
	StopSingleTask(true);
	Stop(true, true);
}

bool CThreadProc::SetThreadCount(std::size_t threadCount)
{
	if (threadCount == 0 || m_running.load()) return false;

	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_workers.empty()) return false;

	m_threadCount = threadCount;
	return true;
}

std::size_t CThreadProc::GetThreadCount() const noexcept
{
	return m_threadCount;
}

bool CThreadProc::SetMaxQueueSize(std::size_t maxQueueSize)
{
	if (maxQueueSize == 0 || m_running.load()) return false;

	std::lock_guard<std::mutex> lock(m_mutex); // 안전 장치
	m_maxQueueSize = maxQueueSize; // 최대 사용가능 큐 수량 설정
	return true;
}

bool CThreadProc::Start()
{
	// 이미 동작 중이면 빠져나감
	if (m_running.load()) return false;

	JoinWorkers();
	
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		// 정지, pending, 상태 flag 설정
		m_stopRequested.store(false);
		m_cancelPending.store(true);
		m_running.store(true);
		m_liveWorkerCount = m_threadCount;

		try
		{
			// 설정된 스레드 수량만큼 스레드를 생성하고 m_workers에 저장
			// 실제로는 WorkerLoop 함수를 스레드로 실행시키고 m_workers가 스레드 풀로 관리
			m_workers.reserve(m_threadCount);
			for (std::size_t i = 0; i < m_threadCount; ++i)
				m_workers.emplace_back(&CThreadProc::WorkerLoop, this);
		}
		catch (...)
		{
			m_stopRequested.store(true);
			m_liveWorkerCount = m_workers.size();
			m_taskCondition.notify_all();
		}
	}

	if (m_workers.size() != m_threadCount)
	{
		JoinWorkers();
		m_running.store(false);
		return false;
	}

	// 스레드 풀 시작 알림
	PostNotify(WM_CTHREADPROC_STARTED, static_cast<WPARAM>(m_threadCount));
	return true;
}

std::uint32_t CThreadProc::StartSingleTask(Task task)
{
	if (!task) return 0;

	// 안전 장치 - 작업 실행하는 동안 복수 작업 동시 실행 방지
	std::lock_guard<std::mutex> lock(m_singleMutex); 
	
	// 이미 실행 중인 싱글 스레드 작업이 있으면 빠져나감
	if (m_singleRunning.load()) return 0;

	// 이전 작업이 끝났지만 아직 join되지 않은 경우 정리
	if (m_singleWorker.joinable()) m_singleWorker.join();

	// 새로운 작업 아이디 지정 - 기존 작업 id를 증가시킴
	std::uint32_t taskId = m_nextTaskId++;
	if (taskId == 0) taskId = m_nextTaskId++;

	// 싱글 스레드 동작 상태 명시
	m_singleStopRequested = false;
	m_singleRunning = true;

	try
	{
		// 스레드 생성 - SingleTaskLoop 실행
		m_singleWorker = std::thread(
			&CThreadProc::SingleTaskLoop,
			this,
			taskId,
			std::move(task));
	}
	catch (...)
	{
		m_singleRunning = false;
		m_singleStopRequested = false;
		return 0;
	}

	return taskId;
}

std::uint32_t CThreadProc::StartSingleTask(SimpleTask task)
{
	if (!task) return 0;

	// SingleTask 형식의 작업 함수를 받아서 스레드 구동
	Task convertedTask = std::bind(
		&CThreadProc::RunSimpleTask,
		this,
		task,
		std::placeholders::_1);

	return StartSingleTask(convertedTask);
}

int CThreadProc::RunSimpleTask(SimpleTask function, const std::atomic<bool>&)
{
	function();
	return 0;
}

void CThreadProc::RequestSingleTaskStop()
{
	if (m_singleRunning.load()) m_singleStopRequested.store(true);
}

void CThreadProc::StopSingleTask(bool wait)
{
	RequestSingleTaskStop();
	if (wait) JoinSingleWorker();
}

void CThreadProc::WaitSingleTask()
{
	JoinSingleWorker();
}

bool CThreadProc::IsSingleTaskRunning() const noexcept
{
	return m_singleRunning.load();
}

bool CThreadProc::IsSingleTaskStopRequested() const noexcept
{
	return m_singleStopRequested.load();
}

//------------------------------------------------

void CThreadProc::Stop(bool wait, bool cancelPending)
{
	RequestStop(cancelPending);
	if (wait) JoinWorkers();
}

void CThreadProc::RequestStop(bool cancelPending)
{
	if (!m_running.load()) return;

	const bool firstRequest = !m_stopRequested.exchange(true);
	
	if (cancelPending) m_cancelPending.store(true);
	else if (firstRequest) m_cancelPending.store(false);
	
	if (cancelPending)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		// 작업 큐 비움
		m_tasks = std::queue<TaskItem>();
	}

	// 스레드 풀을 구성하는 모든 스레드를 종료하기 위해서 모든 스레드 깨움
	// 대기 상태에서 벗어나야 종료 
	m_taskCondition.notify_all();

	if (firstRequest) PostNotify(WM_CTHREADPROC_STOP_REQUESTED);
}

void CThreadProc::Wait()
{
	JoinWorkers();
}

std::uint32_t CThreadProc::AddTask(Task task)
{
	// 스레드 풀이 시작되지 않았거나 정지 요청이 들어왔으면 빠져나감
	if (!task || !m_running.load() || m_stopRequested.load()) return 0;

	std::lock_guard<std::mutex> lock(m_mutex);

	// 스레드 풀 동작 여부, 정지 요청 여부, 작업목록의 작업 수량 검사
	if (!m_running.load() ||
		m_stopRequested.load() || 
		m_tasks.size() >= m_maxQueueSize) 
		return 0;

	// 새로운 작업 아이디 생성 - 기존 작업 아이디 + 1
	std::uint32_t id = m_nextTaskId++;
	if (id == 0) id = m_nextTaskId++;

	// 작업 큐에 작업 등록 - 메모리 절약을 위해서 move 사용
	m_tasks.push(TaskItem{ id, std::move(task) });
	
	// 스레드 풀 동작을 위한 worker loop에서 대기 중이던 스레드 하나를 동작시켜서 작업 시작
	m_taskCondition.notify_one();
	
	return id;
}

std::uint32_t CThreadProc::AddTask(SimpleTask task)
{
	if (!task) return 0;

	return AddTask([function = std::move(task)](const std::atomic<bool>&) -> int
		{
			function();
			return 0;
		});
}

bool CThreadProc::IsRunning() const noexcept
{
	return m_running.load();
}

bool CThreadProc::IsStopRequested() const noexcept
{
	return m_stopRequested.load();
}

std::size_t CThreadProc::GetPendingTaskCount() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_tasks.size();
}

void CThreadProc::SetNotifyWindow(HWND notifyWindow)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_notifyWindow = notifyWindow;
}

HWND CThreadProc::GetNotifyWindow() const noexcept
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_notifyWindow;
}

void CThreadProc::WorkerLoop()
{
	for (;;)
	{
		TaskItem item;
		{
			// m_taskCondition notify 호출 전까지 대기 - notify 호출되면 동작
			std::unique_lock<std::mutex> lock(m_mutex);
			m_taskCondition.wait(lock, [this]()
				{
					return m_stopRequested.load() || !m_tasks.empty();
				});

			if (m_stopRequested.load() && (m_cancelPending.load() || m_tasks.empty()))
			{
				break;
			}

			item = std::move(m_tasks.front());
			m_tasks.pop();
			++m_activeTaskCount; // task 시작되면 동작 중인 작업 수량 증가 시킴
		}

		PostNotify(WM_CTHREADPROC_TASK_STARTED, static_cast<WPARAM>(item.id));
		try
		{
			const int result = item.taskFunction(m_stopRequested);
			PostNotify(WM_CTHREADPROC_TASK_COMPLETED, static_cast<WPARAM>(item.id), static_cast<LPARAM>(result));
		}
		catch (...)
		{
			PostNotify(WM_CTHREADPROC_TASK_FAILED, static_cast<WPARAM>(item.id));
		}

		bool idle = false;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			--m_activeTaskCount; // task 끝나면 동작 중인 작업 수량 감소 시킴

			idle = m_tasks.empty() && m_activeTaskCount == 0;
		}
		if (idle) PostNotify(WM_CTHREADPROC_IDLE);
	}

	bool lastWorker = false;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_liveWorkerCount > 0) --m_liveWorkerCount;
		
		lastWorker = m_liveWorkerCount == 0;
	}

	if (lastWorker)
	{
		m_running.store(false);
		PostNotify(WM_CTHREADPROC_STOPPED);
	}
}

void CThreadProc::SingleTaskLoop(std::uint32_t taskId, Task task)
{
	PostNotify(WM_CTHREADPROC_SINGLE_TASK_STARTED, static_cast<WPARAM>(taskId));

	try
	{
		// 싱글 스레드는 대기 없이 task 바로 시작
		const int result = task(m_singleStopRequested);
		m_singleRunning.store(false);
		PostNotify(WM_CTHREADPROC_SINGLE_TASK_COMPLETED, static_cast<WPARAM>(taskId), static_cast<LPARAM>(result));
	}
	catch (...)
	{
		m_singleRunning.store(false);
		PostNotify(WM_CTHREADPROC_SINGLE_TASK_FAILED, static_cast<WPARAM>(taskId));
	}
}

void CThreadProc::PostNotify(UINT msg, WPARAM wParam/*=0*/, LPARAM lParam/*=NULL*/) const
{
	HWND notifyWindow = nullptr;
	{
		// 메시지 송신 대상 지정
		std::lock_guard<std::mutex> lock(m_mutex); // 안전장치
		notifyWindow = m_notifyWindow;
	}

	// 송신 대상 지정 여부와 대상이 유효한 윈도우인지 확인 후 메시지 전송
	if ((notifyWindow != nullptr) && IsWindow(notifyWindow))
	{
		PostMessageA(notifyWindow, msg, wParam, lParam);
	}
}

void CThreadProc::JoinWorkers()
{
	// 스레드 풀 스레드 각각에 대해서 조건 확인 후 종료까지 대기
	for (std::thread& worker : m_workers)
	{
		if (worker.joinable() && worker.get_id() != std::this_thread::get_id())
		{
			worker.join();
		}
	}

	// 스레드 목록 비우기
	m_workers.clear();
}

void CThreadProc::JoinSingleWorker()
{
	std::lock_guard<std::mutex> lock(m_singleMutex); // 안전 장치

	// 싱글 스레드 상태 확인 후 종료까지 대기
	if (m_singleWorker.joinable() && m_singleWorker.get_id() != std::this_thread::get_id())
	{
		m_singleWorker.join();
	}
}
