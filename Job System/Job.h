#pragma once
#include <atomic>

typedef void(*JobFunc)(void*);

/** Counter for handling job dependencies */
struct JobCounter
{
	std::atomic<int> m_numJobs = 0;
	std::atomic<int> m_numDependants = 0;
};

/** Pointer to a dependency counter. */
struct JobCounterPtr
{
	JobCounterPtr() : m_counter(nullptr), m_index(-1) { }
	JobCounterPtr(JobCounter& counter, int index, int thread) : m_counter(&counter), m_index(index), m_parentThread(thread) { }

	bool IsValid() const { return m_counter != nullptr && m_index >= 0; }

	/** Pointer to the counter */
	JobCounter* m_counter;
	/** Index of the counter in the thread-local ring buffer. Do not modify outside of job system. */
	int m_index = -1;
	/** Index of the thread that this counter was allocated on */
	int m_parentThread = -1;
};

struct Job
{
	/** Function pointer for execution */
	JobFunc m_func;
	/** Optional pointer to counter that will be decremented when the job completes */
	JobCounterPtr m_decCounter;
	/** Optional pointer to counter that must be zero before this job is executed */
	JobCounterPtr m_waitCounter;
	/** Pointer to job data */
	void* m_data;
	// TODO: Pad to cache line?
};

/** Pointer to a job. */
struct JobPtr
{
	JobPtr() : m_job(nullptr), m_index(-1) { }
	JobPtr(Job& job, int index, int thread) : m_job(&job), m_index(index), m_parentThread(thread) { }

	inline bool IsValid() const { return m_job != nullptr && m_index >= 0; }

	/** Pointer to the job itself */
	Job* m_job = nullptr;
	/** Index of the job in the thread-local ring buffer */
	int m_index = -1;
	/** Index of the thread that this job was allocated on */
	int m_parentThread = -1;
};