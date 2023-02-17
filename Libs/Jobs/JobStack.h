#pragma once
#include "Job.h"
#include <atomic>
#include <iostream>
#include <vector>

/**
 * JobStack
 * An array of jobs with a fixed max size.
 * Each JobStack belongs to a certain thread, which may push/pop from the stack.
 * However, another thread may "steal" jobs from this stack if it does not have any itself.
 * Jobs are not guaranteed to be run in any particular order.
 * NOTE: Size MUST be a power-of-two
 */
class JobStack
{
public:
    JobStack(int size)
    {
        m_jobs.resize(size);
        m_mask = size - 1u;
    }

    /** Push a job to the top of the stack. This may only be called safely from the owning thread. */
    void Push(JobPtr&& job)
    {
        m_jobs[m_top & m_mask] = job;
        // Suppress reordering of instructions by the compiler
        _ReadWriteBarrier();
        m_top++;
    }

    /** Pops the job from the top of the stack. This may only be called safely from the owning thread. Returns nullptr if no job exists. */
    JobPtr Pop()
    {
        // Decrement m_top before reading m_bottom, so that the stack appears 1 smaller to all other threads.
        // This is why m_top is read last in Steal()
        int64_t t = m_top - 1;
        _InterlockedExchange64(&m_top, t);

        int64_t b = m_bottom;
        if (b <= t)
        {
            // Stack size is >0, so take the top job.
            JobPtr job = m_jobs[t & m_mask];

            // If the stack size was >1 when we resized it, there is definitely still a job at the top
            // because we told all other threads that m_top changed, so they won't try to remove the job
            // at t.
            if (t != b)
            {
                m_numPops++;
                return job;
            }

            // However, a concurrent Steal() may have taken the same job if the stack size was only 1 when we resized it to 0.
            // In this case, we can do an atomic compare-and-swap to pull m_bottom up instead of moving m_top down.
            // If the comparison fails, then another thread has modified m_bottom which tells us that this job has already been stolen.
            if (_InterlockedCompareExchange64(&m_bottom, b + 1, b) != b)
            {
                // Last remaining job was stolen
                m_top = m_bottom;
                return JobPtr();
            }

            // Last remaining job successfully popped
            m_top = m_bottom;
            m_numPops++;
            return job;
        }

        // Stack is empty
        m_top = m_bottom;
        return JobPtr();
    }

    /** Steals a job from the bottom of the stack. This may be called from any thread. */
    JobPtr Steal()
    {
        int64_t b = m_bottom;
        // Suppress reordering of instructions by the compiler - top must be read last
        _ReadWriteBarrier();

        int64_t t = m_top;
        if (b < t)
        {
            // Stack size is >0, so we're not empty - Take the bottom job
            JobPtr job = m_jobs[b & m_mask];

            // We can't guarantee that this job was not already taken by another thread - perform an atomic compare-and-swap
            if (_InterlockedCompareExchange64(&m_bottom, b + 1, b) != b)
            {
                // m_bottom was changed by another thread since we last checked it, so abort this steal
                return JobPtr();
            }

            // We have guaranteed a successful steal without interference from another thread
            m_numSteals++;
            return job;
        }

        // Stack is empty
        return JobPtr();
    }

    /** Returns true if there have been enough pops since the last steal, such that there may be old jobs stuck at the bottom of the stack that actioning. Call only from the owning thread. */
    bool ShouldOwningThreadSteal()
    {
        // Jobs could be stolen concurrently here, but the result would be that we just try to steal from an empty stack and fail.
        // The owning thread should steal if:
        //  - The stack is larger than 1 item
        //  - Nothing has been stolen yet OR The number of pops is at-least twice the number of steals
        return (m_top - m_bottom > 1) && (m_numSteals == 0 || (m_numPops / m_numSteals >= 2));
    }

private:
    JobStack();

    /** Index of the bottom of the stack - Jobs will be stolen from here. */
    int64_t m_bottom = 0;
    /** Index of the top of the stack - Jobs will be pushed and popped from here by the owning thread. m_top is only ever modified by the owning thread. */
    int64_t m_top = 0;
    /** Mask used to wrap the top and bottom indices when they are outside the size bound. This requires the size to be a power-of-two. */
    unsigned int m_mask;
    /**
     * Counters for the number of pops (LIFO) vs the number of steals (FIFO), to make sure old jobs still get actioned even when the queue keeps filling up.
     * Don't really care about making these or the related checks atomic.
     */
    uint64_t m_numPops = 0;
    uint64_t m_numSteals = 0;
    /** List of jobs */
    std::vector<JobPtr> m_jobs;
};

