#pragma once
#include <vector>
#include <deque>
#include <condition_variable>
#include <mutex>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Job;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct JobSystemConfig
{
	unsigned int m_numWorkerThreads = 0;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class JobSystem
{
	friend class JobWorkerThread;

public:
	JobSystem(JobSystemConfig const& config);
	~JobSystem();

	void							Startup();
	void							Shutdown();


	void							BeginFrame();
	void							EndFrame();

	unsigned int					GetActiveThreads();

	unsigned int					GetNumPendingJobs();
	unsigned int					GetNumExecutingJobs();
	unsigned int					GetNumCompletedJobs();

	void							AddJob(Job* jobToAdd);
	bool							CancelPendingJob(Job* jobToCancel);
	void							CancelPendingJobs();
	void							WaitForJobsToFinish();

	bool							CheckAndRetrieveCompletedJob(Job* jobToRetrieve);
	Job*							RetrieveLastCompletedJob();
	Job*							RetrieveEarliestCompletedJob();
	std::vector<Job*>				RetrieveCompletedJobs();
	
private:

	// Called only by Worker Threads. Removes the oldest job in the job queue and adds it to the executing queue. Returns the same.
	Job*							GetJobToExecute();
	void							MoveJobToCompletedPile(Job* jobToMove);

	JobSystemConfig					m_config = {};
		
	std::vector<JobWorkerThread*>	m_workerThreads;

	std::deque<Job*>				m_pendingJobs;
	std::deque<Job*>				m_executingJobs;
	std::deque<Job*>				m_completedJobs;

	std::condition_variable			m_workAvailableCV;
	std::mutex						m_jobMutex;
	bool							m_isRunning = true;
};
