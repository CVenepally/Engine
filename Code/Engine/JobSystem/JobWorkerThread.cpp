#include "Engine/JobSystem/JobWorkerThread.hpp"
#include "Engine/JobSystem/JobSystem.hpp"
#include "Engine/JobSystem/Job.hpp"

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
JobWorkerThread::JobWorkerThread(unsigned int threadID, JobSystem* owner)
	: m_jobSystem(owner)
	, m_threadID(threadID)
{
	m_workerThread = std::thread(&JobWorkerThread::ThreadMain, this);

	DebuggerPrintf(Stringf("[Job System Log] Thread Created. ID: %d\n", m_threadID).c_str());
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
JobWorkerThread::~JobWorkerThread()
{
	m_workerThread.join();
	m_jobSystem = nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void JobWorkerThread::ThreadMain()
{
	while(m_jobSystem->m_isRunning)
	{
		Job* job = nullptr;

		{
			std::unique_lock<std::mutex> lock(m_jobSystem->m_jobMutex);
			m_jobSystem->m_workAvailableCV.wait(lock, [this] { return !m_jobSystem->m_pendingJobs.empty() || !m_jobSystem->m_isRunning; });

			if(!m_jobSystem->m_isRunning)
			{
				break;
			}

			job = m_jobSystem->GetJobToExecute();
		}

		if(job)
		{
			m_isWorking = true;
			job->Execute();
			m_jobSystem->MoveJobToCompletedPile(job);
			m_isWorking = false;
		}

		std::this_thread::yield();
	}
}

