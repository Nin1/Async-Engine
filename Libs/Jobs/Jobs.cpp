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
// Job queues per thread, for execution on the main thread only. The main thread will steal these jobs.
std::vector<JobStack> Jobs::m_mainThreadJobQueues;
// Threads
std::vector<std::thread> Jobs::m_threads;
thread_local int Jobs::m_thisThreadIndex;
bool Jobs::m_running = true;

Jobs::Jobs(JobFunc mainJob, void* mainJobData)
{
	// TODO: Automatically determine number of threads
	Init(4, mainJob, mainJobData);
}

void Jobs::Init(int numThreads, JobFunc mainJob, void* mainJobData)
{
	// Set up job system
	m_running = true;
	int maxThreadIndex = numThreads - 1;

	// Initialise shared vectors
	m_jobInUse.resize(numThreads);
	m_counterInUse.resize(numThreads);
	m_jobQueues.reserve(numThreads);
	m_mainThreadJobQueues.reserve(numThreads);
	m_threads.reserve(numThreads);

	// Kick off all threads except one
	for (int i = 0; i < maxThreadIndex; ++i)
	{
		m_jobQueues.emplace_back(MAX_JOBS_PER_THREAD);
		m_mainThreadJobQueues.emplace_back(MAX_JOBS_PER_THREAD);
		m_threads.emplace_back(std::thread(Jobs::WorkerThread, i));
	}

	// Turn this thread into the final job thread
	m_jobQueues.emplace_back(MAX_JOBS_PER_THREAD);
	m_mainThreadJobQueues.emplace_back(MAX_JOBS_PER_THREAD);
	MainThread(maxThreadIndex, mainJob, mainJobData);

	// Wait for all threads to finish
	for (auto& thread : m_threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
}

void Jobs::WorkerThread(int threadIndex)
{
	m_thisThreadIndex = threadIndex;
	std::cout << "Initialising thread " << m_thisThreadIndex << std::endl;

	while (m_running)
	{
		JobPtr jobPtr = GetJob();
		if (jobPtr.IsValid())
		{
			Job& job = jobPtr.Get();
			Execute(job);

			// Decrement dependency counter
			if (job.m_decCounter.IsValid())
			{
				int numJobs = --job.m_decCounter.Get().m_numJobs;
			}

			// Decrement dependent counter
			JobCounterPtr& dependantsCounter = job.m_waitCounter;
			if (dependantsCounter.IsValid())
			{
				--dependantsCounter.m_counter->m_numDependants;
				if (dependantsCounter.m_counter->m_numDependants == 0)
				{
					DeallocateCounter(job.m_waitCounter);
				}
			}

			// Free job
			DeallocateJob(jobPtr);
		}
		else
		{
			// Didn't get a job - yield for a bit
			_YIELD_PROCESSOR();
		}
	}

	// This thread has completed
	std::cout << "Thread " << threadIndex << " complete" << std::endl;
}

void Jobs::MainThread(int threadIndex, JobFunc mainJob, void* mainJobData)
{
	m_thisThreadIndex = threadIndex;
	std::cout << "Initialising thread " << m_thisThreadIndex << std::endl;

	// The main thread may have an initial job
	if (mainJob != nullptr)
	{
		CreateJob(mainJob, mainJobData);
	}

	while (m_running)
	{
		// Get the next job to run (prioritise main thread jobs)
		JobPtr jobPtr = GetMainThreadJob();
		if (!jobPtr.IsValid())
		{
			// No main thread jobs to run, so fall back to regular job
			jobPtr = GetJob();
		}

		if (jobPtr.IsValid())
		{
			Job& job = jobPtr.Get();
			Execute(job);

			// Decrement dependency counter
			if (job.m_decCounter.IsValid())
			{
				int numJobs = --job.m_decCounter.Get().m_numJobs;
			}

			// Decrement dependent counter
			JobCounterPtr& dependantsCounter = job.m_waitCounter;
			if (dependantsCounter.IsValid())
			{
				--dependantsCounter.m_counter->m_numDependants;
				if (dependantsCounter.m_counter->m_numDependants == 0)
				{
					DeallocateCounter(job.m_waitCounter);
				}
			}

			// Free job
			DeallocateJob(jobPtr);
		}
		else
		{
			// Didn't get a job - yield for a bit
			_YIELD_PROCESSOR();
		}
	}

	// This thread has completed
	std::cout << "Thread " << threadIndex << " complete" << std::endl;
}

void Jobs::Execute(const Job& job)
{
	// TODO: thread_local pointer to job, so that any Create requests within this execution can inherit the caller's decCounter
	job.m_func(job.m_data);
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

void Jobs::CreateJobOnMainThread(JobFunc func, void* data)
{
	JobPtr jobPtr = AllocateJob();
	jobPtr.m_job->m_func = func;
	jobPtr.m_job->m_data = data;
	m_mainThreadJobQueues[m_thisThreadIndex].Push(jobPtr);
}

void Jobs::CreateJobOnMainThreadWithDependency(JobFunc func, void* data, JobCounterPtr& dependencyCounter)
{
	JobPtr jobPtr = AllocateJob();
	jobPtr.m_job->m_func = func;
	jobPtr.m_job->m_data = data;
	jobPtr.m_job->m_waitCounter = dependencyCounter;
	++(dependencyCounter.m_counter->m_numDependants);
	m_mainThreadJobQueues[m_thisThreadIndex].Push(jobPtr);
}

void Jobs::CreateJobOnMainThreadAndCount(JobFunc func, void* data, JobCounterPtr& jobCounter)
{
	JobPtr jobPtr = AllocateJob();
	jobPtr.m_job->m_func = func;
	jobPtr.m_job->m_data = data;
	jobPtr.m_job->m_decCounter = jobCounter;
	++(jobCounter.m_counter->m_numJobs);
	m_mainThreadJobQueues[m_thisThreadIndex].Push(jobPtr);
}

void Jobs::CreateJobOnMainThreadWithDependencyAndCount(JobFunc func, void* data, JobCounterPtr& dependencyCounter, JobCounterPtr& jobCounter)
{
	JobPtr jobPtr = AllocateJob();
	jobPtr.m_job->m_func = func;
	jobPtr.m_job->m_data = data;
	jobPtr.m_job->m_waitCounter = dependencyCounter;
	jobPtr.m_job->m_decCounter = jobCounter;
	++(jobCounter.m_counter->m_numJobs);
	++(dependencyCounter.m_counter->m_numDependants);
	m_mainThreadJobQueues[m_thisThreadIndex].Push(jobPtr);
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
	return GetJobInner(m_jobQueues);
}

JobPtr Jobs::GetMainThreadJob()
{
	return GetJobInner(m_mainThreadJobQueues);
}

JobPtr Jobs::GetJobInner(std::vector<JobStack>& queues)
{
	int threadIndex = m_thisThreadIndex;
	bool checkedEveryQueue = false;
	JobPtr job;


	// Attempt pop or steal from each queue until we find a job that isn't waiting for any dependency. Give up after checking every queue.
	while (!job.IsValid() && !checkedEveryQueue)
	{
		JobStack& jobQueue = queues[threadIndex];
		if (threadIndex == m_thisThreadIndex)
		{
			// Array to hold jobs that can't be run yet
			std::array<JobPtr, MAX_JOBS_PER_THREAD> deferredJobs;
			int numDeferredJobs = 0;
			job = jobQueue.Pop();
			while (job.IsValid() && job.Get().m_waitCounter.IsValid() && job.Get().m_waitCounter.m_counter->m_numJobs > 0)
			{
				// Job still has dependencies - Put it aside and pick the next job
				deferredJobs[numDeferredJobs] = job;
				numDeferredJobs++;
				job = jobQueue.Pop();
			}
			// Push jobs with dependencies back on queue
			for (int i = 0; i < numDeferredJobs; ++i)
			{
				jobQueue.Push(deferredJobs[i]);
			}
		}
		else
		{
			// Can only try to steal one job from other threads since Pop/Push cannot be called on them
			job = jobQueue.Steal();
		}

		// Check next thread
		threadIndex = (threadIndex + 1) % m_jobQueues.size();

		// Give up if we've searched every thread
		if (threadIndex == m_thisThreadIndex)
		{
			checkedEveryQueue = true;
		}
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
	// Reset the counter
	m_counters[m_counterBufferHead].m_numJobs = 0;
	m_counters[m_counterBufferHead].m_numDependants = 0;
	m_counterInUse[m_thisThreadIndex][m_counterBufferHead] = true;
	return JobCounterPtr(m_counters[m_counterBufferHead], m_counterBufferHead, m_thisThreadIndex);
}

void Jobs::DeallocateCounter(const JobCounterPtr& counter)
{
	// This should be safe - No other thread will be doing anything with this
	m_counterInUse[counter.m_parentThread][counter.m_index] = false;
}