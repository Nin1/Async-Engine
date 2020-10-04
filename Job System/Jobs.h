#pragma once
#include "Job.h"
#include "JobStack.h"
#include <array>
#include <thread>
#include <vector>

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



class Jobs
{
public:
	Jobs(JobFunc mainJob);
	/** Stops jobs from running */
	void Stop() { m_running = false; }

	/** Creates a counter for counting job dependencies.  */
	static JobCounterPtr GetNewJobCounter() { return AllocateCounter(); }

	/** Creates a job with no dependencies */
	static void CreateJob(JobFunc func, void* data);
	/** Create a job that will only be run once dependencyCounter is 0 */
	static void CreateJobWithDependency(JobFunc func, void* data, JobCounterPtr& dependencyCounter);
	/** Create a job that will add to jobCounter when created, and decrement it when complete */
	static void CreateJobAndCount(JobFunc func, void* data, JobCounterPtr& jobCounter);
	/** Create a job that will add to jobCounter when created, and decrement it when complete, but it will only execute once dependencyCounter is 0 */
	static void CreateJobWithDependencyAndCount(JobFunc func, void* data, JobCounterPtr& dependencyCounter, JobCounterPtr& jobCounter);

	// These probably need fibers to store and restore stacks and function pointers
	/** Creates a counter used for pausing a job until its current job is complete */
	//std::atomic<int>& CreateYieldCounter();
	/** Pause the current job and free this thread until the counter is 0 */
	//void YieldUntilCounter(std::atomic<int>& counter);

	bool IsRunning() { return m_running; }

private:
	/** Returns a pointer to an available Job. May return nullptr if there is no space. */
	static JobPtr AllocateJob();
	/** Frees a job from the job buffer so it may be allocated again later. */
	static void DeallocateJob(const JobPtr& job);
	/** Returns a Job to be actioned. */
	static JobPtr GetJob();
	/** Returns a reference to a counter for use with job dependencies */
	static JobCounterPtr AllocateCounter();
	/** Frees a counter from the counter buffer */
	static void DeallocateCounter(const JobCounterPtr& counter);

	static void Execute(const JobPtr& job);

private:
	static constexpr int MAX_JOBS_PER_THREAD = 2048;
	static constexpr int MAX_JOBS_PER_THREAD_MASK = MAX_JOBS_PER_THREAD - 1;

	static constexpr int MAX_COUNTERS_PER_THREAD = 128;
	static constexpr int MAX_COUNTERS_PER_THREAD_MASK = MAX_COUNTERS_PER_THREAD - 1;

	//static constexpr int MAX_PAUSED_JOBS_PER_THREAD = 128;

	// Ring-buffer for allocating jobs per thread
	static thread_local std::array<Job, MAX_JOBS_PER_THREAD> m_jobs;
	static std::vector<std::array<bool, MAX_JOBS_PER_THREAD>> m_jobInUse;	// per-thread, accessed from other threads
	static thread_local uint32_t m_jobBufferHead;

	// Ring-buffer for allocating counters
	static thread_local std::array<JobCounter, MAX_COUNTERS_PER_THREAD> m_counters;
	static std::vector<std::array<bool, MAX_COUNTERS_PER_THREAD>> m_counterInUse;	// per-thread, accessed from other threads
	static thread_local uint32_t m_counterBufferHead;

	// List of currently paused jobs
	//static std::vector<JobStack> m_pausedJobs;

	// Job queue per thread
	static std::vector<JobStack> m_jobQueues;
	// Threads
	static std::vector<std::thread> m_threads;
	static thread_local uint8_t m_thisThreadIndex;
	static bool m_running;
};