#include "Engine/JobSystem/JobSystem.hpp"
#include "Engine/JobSystem/JobWorkerThread.hpp"
#include "Engine/JobSystem/Job.hpp"

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Engine/Math/MathUtils.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
JobSystem::JobSystem(JobSystemConfig const& config)
	: m_config(config)
{
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
JobSystem::~JobSystem()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void JobSystem::Startup()
{
	m_isRunning = true;
	unsigned int numSupportedThreads = std::thread::hardware_concurrency();

	DebuggerPrintf(Stringf("[Job System Log] Num Concurrent Threads Supported: %d\n", numSupportedThreads).c_str());

	unsigned int numThreads = GetMin(numSupportedThreads - 1, m_config.m_numWorkerThreads);

	for(unsigned int threadID = 0; threadID < numThreads; ++threadID)
	{
		JobWorkerThread* workerThread = new JobWorkerThread(threadID, this);
		m_workerThreads.push_back(workerThread);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void JobSystem::Shutdown()
{
	m_isRunning = false;
	m_workAvailableCV.notify_all();

	{
		std::scoped_lock<std::mutex> lock(m_jobMutex);
		m_pendingJobs.clear();
	}

	WaitForJobsToFinish();

	for(JobWorkerThread* thread : m_workerThreads)
	{
		delete thread;
		thread = nullptr;
	}

	{
		std::scoped_lock<std::mutex> lock(m_jobMutex);
		m_completedJobs.clear();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void JobSystem::WaitForJobsToFinish()
{
	while(!m_executingJobs.empty())
	{
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void JobSystem::BeginFrame()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void JobSystem::EndFrame()
{
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int JobSystem::GetActiveThreads()
{
	unsigned int count = 0;

	for(size_t index = 0; index < m_workerThreads.size(); ++index)
	{
		if(m_workerThreads[index]->m_isWorking)
		{
			count += 1;
		}
	}

	return count;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int JobSystem::GetNumPendingJobs()
{
	std::scoped_lock<std::mutex> lock(m_jobMutex);
	return static_cast<unsigned int>(m_pendingJobs.size());
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int JobSystem::GetNumExecutingJobs()
{
	std::scoped_lock<std::mutex> lock(m_jobMutex);
	return static_cast<unsigned int>(m_executingJobs.size());
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int JobSystem::GetNumCompletedJobs()
{
	std::scoped_lock<std::mutex> lock(m_jobMutex);
	return static_cast<unsigned int>(m_completedJobs.size());
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void JobSystem::AddJob(Job* jobToAdd)
{
	std::scoped_lock<std::mutex> lock(m_jobMutex);

	m_pendingJobs.push_back(jobToAdd);

	m_workAvailableCV.notify_one();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool JobSystem::CancelPendingJob(Job* jobToCancel)
{
	std::scoped_lock<std::mutex> lock(m_jobMutex);

	for(auto iter = m_pendingJobs.begin(); iter != m_pendingJobs.end(); ++iter)
	{
		if(*iter == jobToCancel)
		{
			m_pendingJobs.erase(iter);
			return true;
		}
	}
	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void JobSystem::CancelPendingJobs()
{
	std::scoped_lock<std::mutex> lock(m_jobMutex);

	m_pendingJobs.clear();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool JobSystem::CheckAndRetrieveCompletedJob(Job* jobToRetrieve)
{
	std::scoped_lock<std::mutex> lock(m_jobMutex);

	for(auto iter = m_completedJobs.begin(); iter != m_completedJobs.end(); ++iter)
	{
		if(*iter == jobToRetrieve)
		{
			m_completedJobs.erase(iter);
			return true;
		}
	}
	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Job* JobSystem::RetrieveLastCompletedJob()
{
	std::scoped_lock<std::mutex> lock(m_jobMutex);

	if(m_completedJobs.empty())
	{
		return nullptr;
	}

	Job* lastCompletedJob = m_completedJobs.back();
	m_completedJobs.pop_back();

	return lastCompletedJob;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Job* JobSystem::RetrieveEarliestCompletedJob()
{
	std::scoped_lock<std::mutex> lock(m_jobMutex);

	if(m_completedJobs.empty())
	{
		return nullptr;
	}

	Job* earliestCompleted = m_completedJobs.front();
	m_completedJobs.pop_front();

	return earliestCompleted;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::vector<Job*> JobSystem::RetrieveCompletedJobs()
{
	std::scoped_lock<std::mutex> lock(m_jobMutex);

	std::vector<Job*> completedJobs;

	while(!m_completedJobs.empty())
	{
		completedJobs.push_back(m_completedJobs.front());
		m_completedJobs.pop_front();
	}
	
	return completedJobs;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Job* JobSystem::GetJobToExecute()
{
	if(m_pendingJobs.empty())
	{
		return nullptr;
	}

	Job* job = m_pendingJobs.front();
	m_pendingJobs.pop_front();
	m_executingJobs.push_back(job);
	
	return job;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void JobSystem::MoveJobToCompletedPile(Job* jobToMove)
{
	std::scoped_lock<std::mutex> lock(m_jobMutex);

	auto iter = std::find(m_executingJobs.begin(), m_executingJobs.end(), jobToMove);

	if(iter != m_executingJobs.end())
	{
		m_executingJobs.erase(iter);
		m_completedJobs.push_back(jobToMove);
	}

}
