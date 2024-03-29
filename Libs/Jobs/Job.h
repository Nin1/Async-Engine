#pragma once
#include <atomic>

typedef void(*JobFunc)(void*);

/** Job property bitflags */
enum JobFlag
{
	JOBFLAG_NONE = 0,
	/** Job needs to run on the main thread */
	JOBFLAG_MAINTHREAD = 1 << 0,
	/** Job requires disk read/write access */
	JOBFLAG_DISKACCESS = 1 << 1,
	/** Job must complete before the job that created it is considered finished */
	JOBFLAG_ISCHILD = 1 << 2,
	/** For debugging, use this to mark jobs */
	JOBFLAG_DEBUG = 1 << 7,
};

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
	inline bool IsValid() const { return m_index >= 0; }
	/** Returns the JobCounter. Make sure it is valid before calling this. */
	JobCounter& Get() const { return *m_counter; }

	/** Pointer to the counter */
	JobCounter* m_counter;
	/** Index of the counter in the thread-local ring buffer. Do not modify outside of job system. */
	int m_index = -1;
	/** Index of the thread that this counter was allocated on */
	int m_parentThread = -1;
};

struct JobPtr;

struct Job
{
	/** Function pointer for execution */
	JobFunc m_func;
	/** Pointer to parent job */
	Job* m_parent = nullptr;
	/** Optional pointer to counter that will be decremented when the job completes */
	JobCounterPtr m_decCounter;
	/** Optional pointer to counter that must be zero before this job is executed */
	JobCounterPtr m_waitCounter;
	/** Pointer to job data */
	void* m_data = nullptr;
	/** Bitflag properties */
	uint8_t m_flags = 0;
	/** Count of the number of children this job has created that haven't yet finished. */
	std::atomic<int> m_children = 0;
	/** Returns true if this job has completed */
	inline bool IsComplete() const { return m_func == nullptr && m_children == 0; }
	/** Returns true if this job asked for disk reads */
	inline bool NeedsDiskActivity() const { return (m_flags & JOBFLAG_DISKACCESS) != 0; }
	// TODO: Pad to cache line?
};

/** Pointer to a job. */
struct JobPtr
{
	JobPtr() : m_job(nullptr), m_index(-1) { }
	JobPtr(Job& job, int index, int thread) : m_job(&job), m_index(index), m_parentThread(thread) { }

	inline bool IsValid() const { return m_index >= 0; }
	inline bool HasDependencies() const { return m_job && m_job->m_waitCounter.IsValid() && m_job->m_waitCounter.m_counter->m_numJobs > 0; }
	inline bool HasChildren() const { return m_job && m_job->m_children > 0; }

	/** Returns the Job. Make sure it is valid before calling this. */
	Job& Get() { return *m_job; }

	/** Pointer to the job itself */
	Job* m_job = nullptr;
	/** Index of the job in the thread-local ring buffer */
	int m_index = -1;
	/** Index of the thread that this job was allocated on */
	int m_parentThread = -1;
};