#pragma once
#include <atomic>
#include <thread>
#include <condition_variable>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class JobWorkerThread
{
	friend class JobSystem;

private:
	JobWorkerThread(unsigned int threadID, JobSystem* owner);
	~JobWorkerThread();

	void	ThreadMain();


private:
	JobSystem*	m_jobSystem = nullptr;
	bool		m_isWorking	= false;

	//Thread--------------------
	std::thread m_workerThread;
	unsigned int m_threadID = 0;
};