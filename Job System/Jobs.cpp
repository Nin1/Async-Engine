#include "Jobs.h"
#include <iostream>

thread_local std::array<Job, Jobs::MAX_JOBS_PER_THREAD> Jobs::m_jobs;
std::vector<std::array<bool, Jobs::MAX_JOBS_PER_THREAD>> Jobs::m_jobInUse;	// per-thread, accessed from other threads
thread_local uint32_t Jobs::m_jobBufferHead = 0;

thread_local std::array<JobCounter, Jobs::MAX_COUNTERS_PER_THREAD> Jobs::m_counters;
std::vector<std::array<bool, Jobs::MAX_COUNTERS_PER_THREAD>> Jobs::m_counterInUse;	// per-thread, accessed from other threads
thread_local uint32_t Jobs::m_counterBufferHead = 0;
// Job queue per thread
std::vector<JobStack> Jobs::m_jobQueues;
// Threads
std::vector<std::thread> Jobs::m_threads;
thread_local uint8_t Jobs::m_thisThreadIndex;
bool Jobs::m_running = true;

Jobs::Jobs(JobFunc mainJob)
{
	// Set up job system:
	// 1. Count number of available threads
	int numThreads = 4;	// TODO: Get from system - 1 less than total available

	// 2. Create 1 job queue per thread
	m_jobInUse.resize(numThreads);
	m_counterInUse.resize(numThreads);

	for (int i = 0; i < numThreads; ++i)
	{

		m_jobQueues.emplace_back(MAX_JOBS_PER_THREAD);
		//m_pausedJobs.emplace_back(MAX_PAUSED_JOBS_PER_THREAD);

		m_threads.emplace_back([i,mainJob]
		{
			m_thisThreadIndex = i;
			std::cout << "Initialising thread " << m_thisThreadIndex << std::endl;

			// Run main job on first job thread
			if (i == 0)
			{
				std::cout << "Starting main job on thread " << m_thisThreadIndex << std::endl;
				CreateJob(mainJob, nullptr);
			}

			while (m_running)
			{
				JobPtr job = GetJob();
				if (job.IsValid())
				{
					Execute(job);

					// Decrement dependency counter
					if (job.m_job->m_decCounter.IsValid())
					{
						int numJobs = --job.m_job->m_decCounter.m_counter->m_numJobs;
						// Debug
						//if (numJobs % 100 == 0)
						{
							//std::cout << "Jobs remaining: " << numJobs << std::endl;
						}
					}

					// Decrement dependent counter
					JobCounterPtr& dependantsCounter = job.m_job->m_waitCounter;
					if (dependantsCounter.IsValid())
					{
						--dependantsCounter.m_counter->m_numDependants;
						if (dependantsCounter.m_counter->m_numDependants == 0)
						{
							DeallocateCounter(job.m_job->m_waitCounter);
						}
					}

					// Free job
					DeallocateJob(job);
				}
			}
		});
	}
}

void Jobs::Execute(const JobPtr& job)
{
	job.m_job->m_func(job.m_job->m_data);
}

void Jobs::CreateJob(JobFunc func, void* data)
{
	JobPtr jobPtr = AllocateJob();
	jobPtr.m_job->m_func = func;
	jobPtr.m_job->m_data = data;
	m_jobQueues[m_thisThreadIndex].Push(jobPtr);
}

void Jobs::CreateJobWithDependency(JobFunc func, void* data, JobCounterPtr& dependencyCounter)
{
	JobPtr jobPtr = AllocateJob();
	jobPtr.m_job->m_func = func;
	jobPtr.m_job->m_data = data;
	jobPtr.m_job->m_waitCounter = dependencyCounter;
	++(dependencyCounter.m_counter->m_numDependants);
	m_jobQueues[m_thisThreadIndex].Push(jobPtr);
}

void Jobs::CreateJobAndCount(JobFunc func, void* data, JobCounterPtr& jobCounter)
{
	JobPtr jobPtr = AllocateJob();
	jobPtr.m_job->m_func = func;
	jobPtr.m_job->m_data = data;
	jobPtr.m_job->m_decCounter = jobCounter;
	++(jobCounter.m_counter->m_numJobs);
	m_jobQueues[m_thisThreadIndex].Push(jobPtr);
}

void Jobs::CreateJobWithDependencyAndCount(JobFunc func, void* data, JobCounterPtr& dependencyCounter, JobCounterPtr& jobCounter)
{
	JobPtr jobPtr = AllocateJob();
	jobPtr.m_job->m_func = func;
	jobPtr.m_job->m_data = data;
	jobPtr.m_job->m_waitCounter = dependencyCounter;
	jobPtr.m_job->m_decCounter = jobCounter;
	++(jobCounter.m_counter->m_numJobs);
	++(dependencyCounter.m_counter->m_numDependants);
	m_jobQueues[m_thisThreadIndex].Push(jobPtr);
}

JobPtr Jobs::AllocateJob()
{
	// Search the job ring-buffer to find the first available job
	m_jobBufferHead = (m_jobBufferHead + 1) & MAX_JOBS_PER_THREAD_MASK;
	//uint32_t start = m_jobBufferHead;
	while (m_jobInUse[m_thisThreadIndex][m_jobBufferHead] == true)
	{
		m_jobBufferHead = (m_jobBufferHead + 1) & MAX_JOBS_PER_THREAD_MASK;
		//if (m_jobBufferHead == start)
		//{
		//	// Job buffer is full - wait a bit and try again
		//	//std::cout << "No space for new job - Waiting" << std::endl;
		//	_YIELD_PROCESSOR();
		//}
	}
	m_jobInUse[m_thisThreadIndex][m_jobBufferHead] = true;
	return JobPtr(m_jobs[m_jobBufferHead], m_jobBufferHead, m_thisThreadIndex);
}

void Jobs::DeallocateJob(const JobPtr& job)
{
	// This should be safe - No other thread will be doing anything with this
	m_jobInUse[job.m_parentThread][job.m_index] = false;
}

JobPtr Jobs::GetJob()
{
	// Start with our own queue
	int threadIndex = m_thisThreadIndex;
	JobPtr job;
	// Pop or steal from each thread until we find a job that isn't waiting for any dependency
	while (!job.IsValid() || (job.m_job->m_waitCounter.IsValid() && job.m_job->m_waitCounter.m_counter->m_numJobs > 0))
	{
		JobStack& jobQueue = m_jobQueues[threadIndex];
		if (threadIndex == m_thisThreadIndex)
		{
			job = jobQueue.Pop();
		}
		else
		{
			job = jobQueue.Steal();
			//_YIELD_PROCESSOR();
		}
		// Work backwards through threads as thread 0 is most likely to have jobs at startup
		threadIndex = (threadIndex - 1) % m_jobQueues.size();
	}
	return job;
}

JobCounterPtr Jobs::AllocateCounter()
{
	// Search the job ring-buffer to find the first available job
	m_counterBufferHead = (m_counterBufferHead + 1) & MAX_COUNTERS_PER_THREAD_MASK;
	uint32_t start = m_counterBufferHead;
	while (m_counterInUse[m_thisThreadIndex][m_counterBufferHead] == true)
	{
		m_counterBufferHead = (m_counterBufferHead + 1) & MAX_COUNTERS_PER_THREAD_MASK;
		if (m_counterBufferHead == start)
		{
			// ERROR: Counter buffer is full
			return JobCounterPtr();
		}
	}
	m_counterInUse[m_thisThreadIndex][m_counterBufferHead] = true;
	return JobCounterPtr(m_counters[m_counterBufferHead], m_counterBufferHead, m_thisThreadIndex);
}

void Jobs::DeallocateCounter(const JobCounterPtr& counter)
{
	// This should be safe - No other thread will be doing anything with this
	m_counterInUse[counter.m_parentThread][counter.m_index] = false;
}