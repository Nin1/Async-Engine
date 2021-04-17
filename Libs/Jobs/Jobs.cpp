#include "Jobs.h"
#include <iostream>
#include <cstdio>

#define LOG(msg, ...) //std::printf(msg, __VA_ARGS__); std::printf("\n");

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
thread_local Job* Jobs::m_activeJob;
bool Jobs::m_running = true;
Job Jobs::m_nullJob;

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
	m_activeJob = &m_nullJob;
	std::cout << "Initialising thread " << m_thisThreadIndex << std::endl;

	while (m_running)
	{
		JobPtr jobPtr = GetJob();
		ExecuteOuter(jobPtr);
	}

	// This thread has completed
	std::cout << "Thread " << threadIndex << " complete" << std::endl;
}

void Jobs::MainThread(int threadIndex, JobFunc mainJob, void* mainJobData)
{
	m_thisThreadIndex = threadIndex;
	m_activeJob = &m_nullJob;
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
		ExecuteOuter(jobPtr);
	}

	// This thread has completed
	std::cout << "Thread " << threadIndex << " complete" << std::endl;
}

void Jobs::ExecuteOuter(JobPtr& jobPtr)
{
	if (jobPtr.IsValid())
	{
		Job& job = jobPtr.Get();
		Execute(job);

		if (job.IsComplete())
		{
			// Decrement parent's children counter
			if (job.m_parent)
			{
				LOG("MT: Decremented parent counter on job %d", jobPtr.m_index);
				job.m_parent->m_children--;
			}

			// Decrement dependency counter
			if (job.m_decCounter.IsValid())
			{
				_ASSERT(job.m_decCounter.Get().m_numJobs > 0);
				job.m_decCounter.Get().m_numJobs--;
			}

			// Decrement dependent counter
			JobCounterPtr& dependantsCounter = job.m_waitCounter;
			if (dependantsCounter.IsValid())
			{
				_ASSERT(job.m_waitCounter.Get().m_numDependants > 0);
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
			// Job is incomplete (because it has children). Push it back to the queue to check again later.
			m_jobQueues[m_thisThreadIndex].Push(jobPtr);
		}
	}
	else
	{
		// Didn't get a job - yield for a bit
		_YIELD_PROCESSOR();
	}
}

void Jobs::Execute(Job& job)
{
	if (job.m_func)
	{
		m_activeJob = &job;
		job.m_func(job.m_data);
		// Set func to nullptr so we can't run it again (the job will persist if it has children that have not yet finished)
		job.m_func = nullptr;
	}
}

void Jobs::CreateJob(JobFunc func, void* data, bool asChild)
{
	JobPtr jobPtr = AllocateJob(func, data, asChild);
	m_jobQueues[m_thisThreadIndex].Push(jobPtr);
}

void Jobs::CreateJobWithDependency(JobFunc func, void* data, JobCounterPtr& dependencyCounter, bool asChild)
{
	JobPtr jobPtr = AllocateJob(func, data, asChild);
	jobPtr.m_job->m_waitCounter = dependencyCounter;
	++(dependencyCounter.m_counter->m_numDependants);
	m_jobQueues[m_thisThreadIndex].Push(jobPtr);
}

void Jobs::CreateJobAndCount(JobFunc func, void* data, JobCounterPtr& jobCounter, bool asChild)
{
	JobPtr jobPtr = AllocateJob(func, data, asChild);
	jobPtr.m_job->m_decCounter = jobCounter;
	++(jobCounter.m_counter->m_numJobs);
	m_jobQueues[m_thisThreadIndex].Push(jobPtr);
}

void Jobs::CreateJobWithDependencyAndCount(JobFunc func, void* data, JobCounterPtr& dependencyCounter, JobCounterPtr& jobCounter, bool asChild)
{
	JobPtr jobPtr = AllocateJob(func, data, asChild);
	jobPtr.m_job->m_waitCounter = dependencyCounter;
	jobPtr.m_job->m_decCounter = jobCounter;
	++(jobCounter.m_counter->m_numJobs);
	++(dependencyCounter.m_counter->m_numDependants);
	m_jobQueues[m_thisThreadIndex].Push(jobPtr);
}

void Jobs::CreateJobOnMainThread(JobFunc func, void* data, bool asChild)
{
	JobPtr jobPtr = AllocateJob(func, data, asChild);
	m_mainThreadJobQueues[m_thisThreadIndex].Push(jobPtr);
}

void Jobs::CreateJobOnMainThreadWithDependency(JobFunc func, void* data, JobCounterPtr& dependencyCounter, bool asChild)
{
	JobPtr jobPtr = AllocateJob(func, data, asChild);
	jobPtr.m_job->m_waitCounter = dependencyCounter;
	++(dependencyCounter.m_counter->m_numDependants);
	m_mainThreadJobQueues[m_thisThreadIndex].Push(jobPtr);
}

void Jobs::CreateJobOnMainThreadAndCount(JobFunc func, void* data, JobCounterPtr& jobCounter, bool asChild)
{
	JobPtr jobPtr = AllocateJob(func, data, asChild);
	jobPtr.m_job->m_decCounter = jobCounter;
	++(jobCounter.m_counter->m_numJobs);
	m_mainThreadJobQueues[m_thisThreadIndex].Push(jobPtr);
}

void Jobs::CreateJobOnMainThreadWithDependencyAndCount(JobFunc func, void* data, JobCounterPtr& dependencyCounter, JobCounterPtr& jobCounter, bool asChild)
{
	JobPtr jobPtr = AllocateJob(func, data, asChild);
	jobPtr.m_job->m_waitCounter = dependencyCounter;
	jobPtr.m_job->m_decCounter = jobCounter;
	++(jobCounter.m_counter->m_numJobs);
	++(dependencyCounter.m_counter->m_numDependants);
	m_mainThreadJobQueues[m_thisThreadIndex].Push(jobPtr);
}

JobPtr Jobs::AllocateJob(JobFunc func, void* data, bool asChild)
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
	JobPtr job(m_jobs[m_jobBufferHead], m_jobBufferHead, m_thisThreadIndex);

	// Set up new job
	job.Get().m_func = func;
	job.Get().m_data = data;
	// Don't track children for a job that calls itself
	if (asChild && m_activeJob != &m_nullJob && (m_activeJob->m_func != func || m_activeJob->m_data != data))
	{
		m_activeJob->m_children++;
		job.Get().m_parent = m_activeJob;
	}
	return job;
}

void Jobs::DeallocateJob(JobPtr& job)
{
	LOG("Deallocating job %d on thread %d", job.m_index, job.m_parentThread);
	// This should be safe - No other thread will be doing anything with this
	int index = job.m_index;
	job.m_index = -1;
	job.m_job->m_decCounter = JobCounterPtr();
	job.m_job->m_waitCounter = JobCounterPtr();
	_ASSERT(m_jobInUse[job.m_parentThread][index] == true);
	m_jobInUse[job.m_parentThread][index] = false;
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
			while (job.IsValid() && (job.HasDependencies() || job.HasChildren()))
			{
				// Job is still waiting for dependencies, or has children that need to run - Put it aside and pick the next job
				deferredJobs[numDeferredJobs] = job;
				numDeferredJobs++;
				job = jobQueue.Pop();
			}
			// Push jobs with dependencies back on queue
			for (int i = 0; i < numDeferredJobs; ++i)
			{
				queues[m_thisThreadIndex].Push(deferredJobs[i]);
			}
		}
		else
		{
			// Can only try to steal one job from other threads since Pop/Push cannot be called on them
			job = jobQueue.Steal();
			if (job.IsValid() && (job.HasDependencies() || job.HasChildren()))
			{
				// Job is still waiting for dependencies or has children that need to run - Put it back (on this thread now)
				queues[m_thisThreadIndex].Push(job);
				job = JobPtr();
			}
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
			_ASSERT(false);
			return JobCounterPtr();
		}
	}
	// Reset the counter
	m_counters[m_counterBufferHead].m_numJobs = 0;
	m_counters[m_counterBufferHead].m_numDependants = 0;
	// Assert to soft-check that this is thread-safe
	_ASSERT(m_counterInUse[m_thisThreadIndex][m_counterBufferHead] == false);
	m_counterInUse[m_thisThreadIndex][m_counterBufferHead] = true;
	return JobCounterPtr(m_counters[m_counterBufferHead], m_counterBufferHead, m_thisThreadIndex);
}

void Jobs::DeallocateCounter(const JobCounterPtr& counter)
{
	LOG("Deallocating counter %d on thread %d", counter.m_index, counter.m_parentThread);
	// This should be safe - No other thread will be doing anything with this
	// Assert that the counters have completed before deallocating.
	_ASSERT(counter.m_counter->m_numDependants == 0 && counter.m_counter->m_numJobs == 0);
	_ASSERT(m_counterInUse[counter.m_parentThread][counter.m_index] == true);
	m_counterInUse[counter.m_parentThread][counter.m_index] = false;
}