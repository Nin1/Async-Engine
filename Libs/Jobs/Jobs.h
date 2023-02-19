#pragma once
#include "Job.h"
#include "JobStack.h"
#include <array>
#include <thread>
#include <vector>
#include <functional>

// Debug to enable per-thread metric collection about jobs
#define JOBS_COLLECT_METRICS 0

/*
struct PausedJob
{
	PausedJob(JobPtr& job, std::atomic<int>& counter)
		: m_job(job)
		, m_counter(counter)
	{
	}

	JobPtr& m_job;
	std::atomic<int>& m_counter;
};
*/

/**
 * Arguments are:
 * T*: Pointer to first element of this chunk
 * size_t: Number of elements in this chunk
 * size_t: Index in the original array that this chunk starts at
 */
template<typename T>
using ParallelForFunc = std::function<void(T*, size_t, size_t)>;

class Jobs
{
public:
	/** Initialise job system, automatically detecting the number of threads and running mainJob on one of them. */
	Jobs(JobFunc mainJob, void* mainJobData);
	/** Initialise job system with the given number of threads, and running mainJob on this thread. */
	Jobs(int numThreads, JobFunc mainJob, void* mainJobData) { Init(numThreads, mainJob, mainJobData); }
	/** Stops jobs from running */
	static void Stop() { m_running = false; }

	/** Creates a counter for counting job dependencies.  */
	static JobCounterPtr GetNewJobCounter() { return AllocateCounter(); }

	/** Creates a job with no dependencies */
	static void CreateJob(JobFunc func, void* data, uint8_t flags);
	/** Create a job that will only be run once dependencyCounter is 0 */
	static void CreateJobWithDependency(JobFunc func, void* data, uint8_t flags, JobCounterPtr& dependencyCounter);
	/** Create a job that will add to jobCounter when created, and decrement it when complete */
	static void CreateJobAndCount(JobFunc func, void* data, uint8_t flags, JobCounterPtr& jobCounter);
	/** Create a job that will add to jobCounter when created, and decrement it when complete, but it will only execute once dependencyCounter is 0 */
	static void CreateJobWithDependencyAndCount(JobFunc func, void* data, uint8_t flags, JobCounterPtr& dependencyCounter, JobCounterPtr& jobCounter);

	/** Executes jobs exclusively from this thread's queue until the given counter is 0. The counter will then be deallocated automatically. */
	static void JoinUntilCompleted(const JobCounterPtr& dependencyCounter);

	static void PushJob(JobPtr&& jobPtr, bool mainThread);

	bool IsRunning() { return m_running; }

private:
	/** Parallel-For data structure for meta job */
	template<typename T>
	struct ParallelForJobData
	{
		T* m_data;
		size_t m_count;
		size_t m_startIndex;
		ParallelForFunc<T>* m_func;
	};

	/** Meta job that executes one chunk of a parallel-for */
	template<typename T>
	static void ParallelForJob(void* jobData)
	{
		const ParallelForJobData<T>* data = static_cast<const ParallelForJobData<T>*>(jobData);
		(*data->m_func)(data->m_data, data->m_count, data->m_startIndex);
	}

public:
	/** Creates a number of individual jobs that will process chunks of the given data as a parallel-for-loop */
	template<typename T>
	static void ParallelFor(T* dataStart, size_t count, size_t chunkSize, ParallelForFunc<T> func)
	{
		T* dataEnd = dataStart + count;
		T* dataCurrent = dataStart;
		JobCounterPtr counter = GetNewJobCounter();
		// ParallelForJobData objects must persist so the jobData pointer points at valid data
		std::vector<ParallelForJobData<T>> jobData;
		int dataIndex = 0;
		jobData.reserve((count / chunkSize) + 1);
		// Spawn multiple jobs that each operate on a different chunk of data
		while (dataCurrent != dataEnd)
		{
			size_t thisChunkSize = std::min(size_t(dataEnd - dataCurrent), chunkSize);
			jobData.push_back({dataCurrent, thisChunkSize, size_t(dataCurrent - dataStart), &func});
			CreateJobAndCount(ParallelForJob<T>, &jobData[dataIndex], JOBFLAG_NONE | JOBFLAG_DEBUG, counter);
			dataIndex++;
			dataCurrent += thisChunkSize;
		}
		// Execute
		JoinUntilCompleted(counter);
	}

private:
	void Init(int numThreads, JobFunc mainJob, void* mainJobData);

	/** Main method per non-main thread */
	static void WorkerThread(uint8_t threadIndex);
	/** Main method for the main thread */
	static void MainThread(uint8_t threadIndex, JobFunc mainJob, void* mainJobData);
	/** Outer method for job execution */
	static void ExecuteOuter(JobPtr&& jobPtr);

	/** Returns a pointer to an available Job. May return nullptr if there is no space. */
	static JobPtr AllocateJob(JobFunc func, void* data, uint8_t flags);
	/** Frees a job from the job buffer so it may be allocated again later. */
	static void DeallocateJob(JobPtr& job);
	/** Returns a Job to be actioned. */
	static JobPtr GetJob();
	/** Returns a Job from this thread to be actioned. */
	static JobPtr GetJobFromThisThread(std::vector<JobStack>& queues, bool popOnly);
	/** Returns a Job from this thread to be actioned. */
	static JobPtr GetJobFromOtherThread(int threadIndex, std::vector<JobStack>& queues);
	/** Returns a Job to be actioned on the main thread. */
	static JobPtr GetMainThreadJob();
	/** Helper method for GetJob() and GetMainThreadJob(). */
	static JobPtr GetJobInner(std::vector<JobStack>& queues);
	/** Returns a reference to a counter for use with job dependencies */
	static JobCounterPtr AllocateCounter();
	/** Frees a counter from the counter buffer */
	static void DeallocateCounter(const JobCounterPtr& counter);

	static void Execute(Job& job);

private:
	// Maximum number of jobs per-thread. Must be a power-of-two.
	static constexpr int MAX_JOBS_PER_THREAD = 2048;
	static constexpr int MAX_JOBS_PER_THREAD_MASK = MAX_JOBS_PER_THREAD - 1;

	// Maximum number of counters per-thread. Must be a power-of-two.
	static constexpr int MAX_COUNTERS_PER_THREAD = 128;
	static constexpr int MAX_COUNTERS_PER_THREAD_MASK = MAX_COUNTERS_PER_THREAD - 1;

	// Ring-buffer for allocating jobs per thread
	static thread_local std::array<Job, MAX_JOBS_PER_THREAD>& m_jobs;
	static std::vector<std::array<bool, MAX_JOBS_PER_THREAD>> m_jobInUse;	// per-thread, accessed from other threads
	static thread_local uint32_t m_jobBufferHead;

	// Heap-allocated large buffers
	static thread_local std::unique_ptr<std::array<Job, MAX_JOBS_PER_THREAD>> m_jobsUniquePtr;
	static thread_local std::unique_ptr<std::array<JobCounter, MAX_COUNTERS_PER_THREAD>> m_countersUniquePtr;
	static thread_local std::unique_ptr<std::array<JobPtr, MAX_JOBS_PER_THREAD>> m_deferredJobsUniquePtr;

	// Ring-buffer for allocating counters
	static thread_local std::array<JobCounter, MAX_COUNTERS_PER_THREAD>& m_counters;
	static std::vector<std::array<bool, MAX_COUNTERS_PER_THREAD>> m_counterInUse;	// per-thread, accessed from other threads
	static thread_local uint32_t m_counterBufferHead;

	// Job queue per thread
	static std::vector<JobStack> m_jobQueues;
	// Job queues per thread, for execution on the main thread only. The main thread will steal these jobs.
	static std::vector<JobStack> m_mainThreadJobQueues;
	// Per thread, a temporary buffer for deferring jobs that can't be executed yet. 
	static thread_local std::array<JobPtr, MAX_JOBS_PER_THREAD>& m_deferredJobs;
	// Threads
	static std::vector<std::thread> m_threads;
	static thread_local uint8_t m_thisThreadIndex;
	static uint8_t m_maxThreadIndex;
	static bool m_running;
	// The job currently running on this thread
	static thread_local Job* m_activeJob;
	// Null job, used as a placeholder activeJob to avoid some branches
	static Job m_nullJob;

	// Boolean that a thread can attempt to take for executing disk read jobs
	static thread_local bool m_thisThreadCanReadDisk;
	static char m_diskJobInProgress;
	static constexpr char CHAR_TRUE = 1;
	static constexpr char CHAR_FALSE = 0;

	// DEBUG THINGS
#if JOBS_COLLECT_METRICS
	// Number of jobs each thread has executed
	static std::vector<int> m_numJobsExecutedPerThread;
	// Number of steals each thread has performed
	static std::vector<int> m_numStolenJobsExecutedPerThread;
	// Number of own-thread jobs each thread has performed
	static std::vector<int> m_numOwnJobsExecutedPerThread;
#endif
};