#include "pch.h"
#include "Jobs.h"
#include <iostream>
#include <cstdio>

#define LOG(msg, ...) //std::printf(msg, __VA_ARGS__); std::printf("\n");

#define BIT_IS_SET(flags, mask) (flags & mask) != 0


// Heap-allocated buffers
thread_local std::unique_ptr<std::array<Job, Jobs::MAX_JOBS_PER_THREAD>> Jobs::m_jobsUniquePtr = std::make_unique<std::array<Job, MAX_JOBS_PER_THREAD>>();
thread_local std::unique_ptr<std::array<JobCounter, Jobs::MAX_COUNTERS_PER_THREAD>> Jobs::m_countersUniquePtr = std::make_unique<std::array<JobCounter, MAX_COUNTERS_PER_THREAD>>();
thread_local std::unique_ptr<std::array<JobPtr, Jobs::MAX_JOBS_PER_THREAD>> Jobs::m_deferredJobsUniquePtr = std::make_unique<std::array<JobPtr, MAX_JOBS_PER_THREAD>>();
// Heap-allocated buffer references
thread_local std::array<Job, Jobs::MAX_JOBS_PER_THREAD>& Jobs::m_jobs = *m_jobsUniquePtr;
thread_local std::array<JobPtr, Jobs::MAX_JOBS_PER_THREAD>& Jobs::m_deferredJobs = *m_deferredJobsUniquePtr;
thread_local std::array<JobCounter, Jobs::MAX_COUNTERS_PER_THREAD>& Jobs::m_counters = *m_countersUniquePtr;

std::vector<std::array<std::unique_ptr<std::atomic<bool>>, Jobs::MAX_JOBS_PER_THREAD>> Jobs::m_jobInUse;	// per-thread, accessed from other threads
thread_local uint32_t Jobs::m_jobBufferHead = 0;

std::vector<std::array<std::unique_ptr<std::atomic<bool>>, Jobs::MAX_COUNTERS_PER_THREAD>> Jobs::m_counterInUse;	// per-thread, accessed from other threads
thread_local uint32_t Jobs::m_counterBufferHead = 0;
// Job queue per thread
std::vector<JobStack> Jobs::m_jobQueues;
// Job queues per thread, for execution on the main thread only. The main thread will steal these jobs.
std::vector<JobStack> Jobs::m_mainThreadJobQueues;
// Threads
std::vector<std::thread> Jobs::m_threads;
thread_local uint8_t Jobs::m_thisThreadIndex;
uint8_t Jobs::m_maxThreadIndex;
thread_local Job* Jobs::m_activeJob;
// Misc
bool Jobs::m_running = true;
Job Jobs::m_nullJob;
char Jobs::m_diskJobInProgress = false;
thread_local bool Jobs::m_thisThreadCanReadDisk;
#if JOBS_COLLECT_METRICS
size_t Jobs::m_mainThreadIndex;
std::vector<std::unique_ptr<std::atomic<int>>> Jobs::m_numStolenJobsExecutedPerThread;
std::vector<std::unique_ptr<std::atomic<int>>> Jobs::m_numOwnJobsExecutedPerThread;
std::vector<std::unique_ptr<std::atomic<int>>> Jobs::m_numExecutedLoopsPerThread;
std::vector<std::unique_ptr<std::atomic<int>>> Jobs::m_numStarvedLoopsPerThread;
std::vector<std::unique_ptr<std::atomic<int>>> Jobs::m_numJobsCreatedPerThread;
std::vector<std::unique_ptr<std::atomic<int>>> Jobs::m_numMainThreadJobsCreatedPerThread;
std::vector<std::unique_ptr<std::atomic<long long>>> Jobs::m_timeInJobsPerThreadNS;
std::vector<std::unique_ptr<std::atomic<long long>>> Jobs::m_timeNotInJobsPerThreadNS;
std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>> Jobs::m_lastJobFinishTimePerThreadNS;
std::chrono::time_point<std::chrono::high_resolution_clock> Jobs::m_lastMetricResetTime;
#endif

Jobs::Jobs(JobFunc mainJob, void* mainJobData)
{
	// TODO: Automatically determine number of threads
	Init(4, mainJob, mainJobData);
}

void Jobs::Init(int numThreads, JobFunc mainJob, void* mainJobData)
{
	// Set up job system
	m_running = true;
	m_maxThreadIndex = numThreads - 1;

	// Initialise shared vectors
	m_jobInUse.resize(numThreads);
	m_counterInUse.resize(numThreads);
	m_jobQueues.reserve(numThreads);
	m_mainThreadJobQueues.reserve(numThreads);
	m_threads.reserve(numThreads);

	for (int thread = 0; thread < numThreads; thread++)
	{
		for (int i = 0; i < MAX_JOBS_PER_THREAD; i++)
		{
			m_jobInUse[thread][i] = std::make_unique<std::atomic<bool>>(false);
		}
		for (int i = 0; i < MAX_COUNTERS_PER_THREAD; i++)
		{
			m_counterInUse[thread][i] = std::make_unique<std::atomic<bool>>(false);
		}
	}

#if JOBS_COLLECT_METRICS
	m_mainThreadIndex = m_maxThreadIndex;
	m_lastMetricResetTime = std::chrono::high_resolution_clock::now();
	m_numStolenJobsExecutedPerThread.resize(numThreads);
	m_numOwnJobsExecutedPerThread.resize(numThreads);
	m_numExecutedLoopsPerThread.resize(numThreads);
	m_numStarvedLoopsPerThread.resize(numThreads);
	m_numJobsCreatedPerThread.resize(numThreads);
	m_numMainThreadJobsCreatedPerThread.resize(numThreads);
	m_timeInJobsPerThreadNS.resize(numThreads);
	m_timeNotInJobsPerThreadNS.resize(numThreads);
	m_lastJobFinishTimePerThreadNS.resize(numThreads);
	for (int i = 0; i < numThreads; i++)
	{
		m_numStolenJobsExecutedPerThread[i] = std::make_unique<std::atomic<int>>(0);
		m_numOwnJobsExecutedPerThread[i] = std::make_unique<std::atomic<int>>(0);
		m_numExecutedLoopsPerThread[i] = std::make_unique<std::atomic<int>>(0);
		m_numStarvedLoopsPerThread[i] = std::make_unique<std::atomic<int>>(0);
		m_numJobsCreatedPerThread[i] = std::make_unique<std::atomic<int>>(0);
		m_numMainThreadJobsCreatedPerThread[i] = std::make_unique<std::atomic<int>>(0);
		m_timeInJobsPerThreadNS[i] = std::make_unique<std::atomic<long long>>(0);
		m_timeNotInJobsPerThreadNS[i] = std::make_unique<std::atomic<long long>>(0);
		m_lastJobFinishTimePerThreadNS[i] = std::chrono::high_resolution_clock::now();
	}
#endif

	// Allocate job queues
	for (uint8_t i = 0; i < m_maxThreadIndex; ++i)
	{
		m_jobQueues.emplace_back(MAX_JOBS_PER_THREAD);
		m_mainThreadJobQueues.emplace_back(MAX_JOBS_PER_THREAD);
	}

	// Kick off all threads except this one
	for (uint8_t i = 0; i < m_maxThreadIndex; ++i)
	{
		m_threads.emplace_back(std::thread(Jobs::WorkerThread, i));
	}

	// The main thread can only perform disk reads if it is the only thread
	m_thisThreadCanReadDisk = (numThreads == 1);
	// Turn this thread into the final job thread
	m_jobQueues.emplace_back(MAX_JOBS_PER_THREAD);
	m_mainThreadJobQueues.emplace_back(MAX_JOBS_PER_THREAD);
	MainThread(m_maxThreadIndex, mainJob, mainJobData);

	// Wait for all threads to finish
	for (auto& thread : m_threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
}

void Jobs::WorkerThread(uint8_t threadIndex)
{
	m_thisThreadIndex = threadIndex;
	m_thisThreadCanReadDisk = true;
	m_activeJob = &m_nullJob;
	std::cout << "Initialising thread " << (int)m_thisThreadIndex << std::endl;

	while (m_running)
	{
		JobPtr jobPtr = GetJob();
		ExecuteOuter(std::move(jobPtr));
	}

	// This thread has completed
	std::cout << "Thread " << (int)m_thisThreadIndex << " complete" << std::endl;
}

void Jobs::MainThread(uint8_t threadIndex, JobFunc mainJob, void* mainJobData)
{
	m_thisThreadIndex = threadIndex;
	m_activeJob = &m_nullJob;
	std::cout << "Initialising thread " << (int)m_thisThreadIndex << std::endl;

	// The main thread may have an initial job
	if (mainJob != nullptr)
	{
		CreateJob(mainJob, mainJobData, JOBFLAG_MAINTHREAD);
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
		ExecuteOuter(std::move(jobPtr));
	}

	// This thread has completed
	std::cout << "Thread " << (int)m_thisThreadIndex << " complete" << std::endl;
}

void Jobs::ExecuteOuter(JobPtr&& jobPtr)
{
	if (!jobPtr.IsValid())
	{
		// Didn't get a job - yield for a bit
		_YIELD_PROCESSOR();
	#if JOBS_COLLECT_METRICS
		(*m_numStarvedLoopsPerThread[m_thisThreadIndex])++;
	#endif
		return;
	}
#if JOBS_COLLECT_METRICS
	(*m_numExecutedLoopsPerThread[m_thisThreadIndex])++;
#endif

	Job& job = jobPtr.Get();
	Execute(job);

	if (!job.IsComplete())
	{
		// Job executed but is incomplete (because it has children). Push it back to the queue to check again later.
		m_jobQueues[m_thisThreadIndex].Push(std::move(jobPtr));
		return;
	}

	// Release disk access
	if (job.NeedsDiskActivity())
	{
		_ASSERT(m_diskJobInProgress);
		m_diskJobInProgress = false;
	}

	// Decrement parent's children counter
	if (job.m_parent != nullptr)
	{
		_ASSERT(job.m_parent->m_children > 0);
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

void Jobs::Execute(Job& job)
{
	if (job.m_func)
	{
		Job* currentJob = m_activeJob;
		m_activeJob = &job;

#if JOBS_COLLECT_METRICS
		auto jobStartTimeNS = std::chrono::high_resolution_clock::now();
		auto timeSinceLastJob = jobStartTimeNS - m_lastJobFinishTimePerThreadNS[m_thisThreadIndex];
		m_timeNotInJobsPerThreadNS[m_thisThreadIndex]->fetch_add(timeSinceLastJob.count());
#endif

		// Execute job!
		job.m_func(job.m_data);

#if JOBS_COLLECT_METRICS
		m_lastJobFinishTimePerThreadNS[m_thisThreadIndex] = std::chrono::high_resolution_clock::now();
		auto timeSinceJobStart = m_lastJobFinishTimePerThreadNS[m_thisThreadIndex] - jobStartTimeNS;
		m_timeInJobsPerThreadNS[m_thisThreadIndex]->fetch_add(timeSinceJobStart.count());
#endif

		// Set func to nullptr so we can't run it again (the job will persist if it has children that have not yet finished)
		job.m_func = nullptr;
		m_activeJob = currentJob;
	}
}

void Jobs::PushJob(JobPtr&& jobPtr, bool mainThread)
{
	if (mainThread)
	{
		m_mainThreadJobQueues[m_thisThreadIndex].Push(std::move(jobPtr));
	#if JOBS_COLLECT_METRICS
		(*m_numMainThreadJobsCreatedPerThread[m_thisThreadIndex])++;
	#endif
	}
	else
	{
		m_jobQueues[m_thisThreadIndex].Push(std::move(jobPtr));
	#if JOBS_COLLECT_METRICS
		(*m_numJobsCreatedPerThread[m_thisThreadIndex])++;
	#endif
	}
}

void Jobs::CreateJob(JobFunc func, void* data, uint8_t flags)
{
	JobPtr jobPtr = AllocateJob(func, data, flags);
	PushJob(std::move(jobPtr), BIT_IS_SET(flags, JOBFLAG_MAINTHREAD));
}

void Jobs::CreateJobWithDependency(JobFunc func, void* data, uint8_t flags, JobCounterPtr& dependencyCounter)
{
	JobPtr jobPtr = AllocateJob(func, data, flags);
	jobPtr.m_job->m_waitCounter = dependencyCounter;
	++(dependencyCounter.m_counter->m_numDependants);
	PushJob(std::move(jobPtr), BIT_IS_SET(flags, JOBFLAG_MAINTHREAD));
}

void Jobs::CreateJobAndCount(JobFunc func, void* data, uint8_t flags, JobCounterPtr& jobCounter)
{
	JobPtr jobPtr = AllocateJob(func, data, flags);
	jobPtr.m_job->m_decCounter = jobCounter;
	++(jobCounter.m_counter->m_numJobs);
	PushJob(std::move(jobPtr), BIT_IS_SET(flags, JOBFLAG_MAINTHREAD));
}

void Jobs::CreateJobWithDependencyAndCount(JobFunc func, void* data, uint8_t flags, JobCounterPtr& dependencyCounter, JobCounterPtr& jobCounter)
{
	JobPtr jobPtr = AllocateJob(func, data, flags);
	jobPtr.m_job->m_waitCounter = dependencyCounter;
	jobPtr.m_job->m_decCounter = jobCounter;
	++(jobCounter.m_counter->m_numJobs);
	++(dependencyCounter.m_counter->m_numDependants);
	PushJob(std::move(jobPtr), BIT_IS_SET(flags, JOBFLAG_MAINTHREAD));
}

void Jobs::JoinUntilCompleted(const JobCounterPtr& dependencyCounter)
{
	while (dependencyCounter.Get().m_numJobs > 0)
	{
		ExecuteOuter(std::move(GetJobFromThisThread(m_jobQueues, true)));
	}
	DeallocateCounter(dependencyCounter);
}

JobPtr Jobs::AllocateJob(JobFunc func, void* data, uint8_t flags)
{
	// Search the job ring-buffer to find the first available job
	m_jobBufferHead = (m_jobBufferHead + 1) & MAX_JOBS_PER_THREAD_MASK;
	uint32_t start = m_jobBufferHead;
	uint8_t numFails = 0;
	while (m_jobInUse[m_thisThreadIndex][m_jobBufferHead]->load() == true)
	{
		m_jobBufferHead = (m_jobBufferHead + 1) & MAX_JOBS_PER_THREAD_MASK;
		if (m_jobBufferHead == start)
		{
			// Job buffer is full
			if (++numFails > 200)
			{
				// Can't find space for jobs - application is creating too many
				_ASSERT(false);
			}
			// Wait for buffer to free up
			_YIELD_PROCESSOR();
		}
	}
	m_jobInUse[m_thisThreadIndex][m_jobBufferHead]->store(true);
	JobPtr job(m_jobs[m_jobBufferHead], m_jobBufferHead, m_thisThreadIndex);

	// Set up new job
	job.Get().m_func = func;
	job.Get().m_data = data;
	job.Get().m_flags = flags;
	// Don't track children for a job that calls itself
	bool isChild = BIT_IS_SET(flags, JOBFLAG_ISCHILD);
	if (isChild && m_activeJob != &m_nullJob && (m_activeJob->m_func != func || m_activeJob->m_data != data))
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
	job.m_job->m_parent = nullptr;
	_ASSERT(m_jobInUse[job.m_parentThread][index]->load() == true);
	m_jobInUse[job.m_parentThread][index]->store(false);
}

JobPtr Jobs::GetJob()
{
	return GetJobInner(m_jobQueues);
}

JobPtr Jobs::GetMainThreadJob()
{
	return GetJobInner(m_mainThreadJobQueues);
}

inline JobPtr PopOrStealJobFromThisThread(JobStack& jobQueue, bool popOnly)
{
	if (!popOnly && jobQueue.ShouldOwningThreadSteal())
	{
		return jobQueue.Steal();
	}
	else
	{
		return jobQueue.Pop();
	}
}

JobPtr Jobs::GetJobFromThisThread(std::vector<JobStack>& queues, bool popOnly)
{
	JobStack& jobQueue = queues[m_thisThreadIndex];
	// Array to hold jobs that can't be run yet
	int numDeferredJobs = 0;
	// Since owning thread is FIFO and stealing threads are LIFO, jobs can get stuck at the bottom of the queue if other threads are busy (or there is only one thread).
	// Because of this, we occasionally steal even if we are the owning thread.
	JobPtr job = PopOrStealJobFromThisThread(jobQueue, popOnly);

	while (job.IsValid())
	{
		// Check that this job satisfies all the conditions needed to execute (not waiting for dependencies or children)
		if (!job.HasDependencies() && !job.HasChildren())
		{
			// This job can run - If it needs disk access, see if we can acquire it
			if (job.m_job->NeedsDiskActivity())
			{
				if (m_thisThreadCanReadDisk && _InterlockedCompareExchange8(&m_diskJobInProgress, CHAR_TRUE, CHAR_FALSE) == CHAR_FALSE)
				{
					// We can execute this job
					break;
				}
			}
			else
			{
				// We can execute this job
				break;
			}
		}

		// We can't execute this job yet - put it aside and pick the next job
		m_deferredJobs[numDeferredJobs] = job;
		numDeferredJobs++;
		job = PopOrStealJobFromThisThread(jobQueue, popOnly);
	}
	// Push jobs with dependencies back on queue
	for (int i = 0; i < numDeferredJobs; ++i)
	{
		jobQueue.Push(std::move(m_deferredJobs[i]));
	}
#if JOBS_COLLECT_METRICS
	if (job.IsValid())
	{
		(*m_numOwnJobsExecutedPerThread[m_thisThreadIndex])++;
	}
#endif
	return job;
}

JobPtr Jobs::GetJobFromOtherThread(int threadIndex, std::vector<JobStack>& queues)
{
	JobStack& jobQueue = queues[threadIndex];
	// Can only try to steal one job from other threads since Pop/Push cannot be called on them
	JobPtr job = jobQueue.Steal();
	if (!job.IsValid())
	{
		return job;
	}
	if (job.HasDependencies() || job.HasChildren())
	{
		// Job is still waiting for dependencies or waiting for children to finish - Put it back (on this thread now)
		queues[m_thisThreadIndex].Push(std::move(job));
		job = JobPtr();
	}
	else if (job.m_job->NeedsDiskActivity())
	{
		if (!m_thisThreadCanReadDisk || _InterlockedCompareExchange8(&m_diskJobInProgress, CHAR_TRUE, CHAR_FALSE) == CHAR_TRUE)
		{
			// This job requires disk access but we can't get disk access right now. Put it back (on this thread now)
			queues[m_thisThreadIndex].Push(std::move(job));
			job = JobPtr();
		}
	}
#if JOBS_COLLECT_METRICS
	if (job.IsValid())
	{
		(*m_numStolenJobsExecutedPerThread[m_thisThreadIndex])++;
	}
#endif
	return job;
}

JobPtr Jobs::GetJobInner(std::vector<JobStack>& queues)
{
	int threadIndex = m_thisThreadIndex;
	JobPtr job;

	// Attempt pop or steal from each queue until we find a job that isn't waiting for any dependency. Give up after checking every queue.
	do
	{
		if (threadIndex == m_thisThreadIndex)
		{
			job = GetJobFromThisThread(queues, false);
		}
		else
		{
			job = GetJobFromOtherThread(threadIndex, queues);
		}

		// Check next thread
		threadIndex = threadIndex < m_maxThreadIndex ? (threadIndex + 1) : 0;
	}
	while (!job.IsValid() && threadIndex != m_thisThreadIndex);

	return job;
}

JobCounterPtr Jobs::AllocateCounter()
{
	// Search the job ring-buffer to find the first available job
	m_counterBufferHead = (m_counterBufferHead + 1) & MAX_COUNTERS_PER_THREAD_MASK;
	uint32_t start = m_counterBufferHead;
	uint8_t numFails = 0;
	while (m_counterInUse[m_thisThreadIndex][m_counterBufferHead]->load() == true)
	{
		m_counterBufferHead = (m_counterBufferHead + 1) & MAX_COUNTERS_PER_THREAD_MASK;
		if (m_counterBufferHead == start)
		{
			// Job buffer is full
			if (++numFails > 200)
			{
				// Can't find space for jobs - application is creating too many
				_ASSERT(false);
			}
			// Wait for buffer to free up
			_YIELD_PROCESSOR();
		}
	}
	// Reset the counter
	m_counters[m_counterBufferHead].m_numJobs = 0;
	m_counters[m_counterBufferHead].m_numDependants = 0;
	// Assert to soft-check that this is thread-safe
	_ASSERT(m_counterInUse[m_thisThreadIndex][m_counterBufferHead]->load() == false);
	m_counterInUse[m_thisThreadIndex][m_counterBufferHead]->store(true);
	return JobCounterPtr(m_counters[m_counterBufferHead], m_counterBufferHead, m_thisThreadIndex);
}

void Jobs::DeallocateCounter(const JobCounterPtr& counter)
{
	LOG("Deallocating counter %d on thread %d", counter.m_index, counter.m_parentThread);
	// This should be safe - No other thread will be doing anything with this
	// Assert that the counters have completed before deallocating.
	_ASSERT(counter.m_counter->m_numDependants == 0 && counter.m_counter->m_numJobs == 0);
	_ASSERT(m_counterInUse[counter.m_parentThread][counter.m_index]->load() == true);
	m_counterInUse[counter.m_parentThread][counter.m_index]->store(false);
}