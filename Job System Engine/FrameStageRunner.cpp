#include "FrameStageRunner.h"
#include "FrameData.h"
#include "Assert.h"
#include <Jobs/Jobs.h>
#include <atomic>
#include <iostream>

FrameStageRunner::FrameStageRunner(const char* name)
	: m_name(name)
{
	// Initialise frame queue
	FrameNodePtr nodePtr = AllocateFrameNode();
	FrameNode& node = *nodePtr.m_ptr;
	node.m_next.store({ nullptr, 0, -1 });
	m_queue.m_head = m_queue.m_tail = nodePtr;

}

// This should only ever be called by one thread at a time (unless the pipeline ends up branching and converging ever? Will need revisiting if that ever happens)
void FrameStageRunner::QueueFrame(FrameData& frame)
{
	// Add frame to queue
	EnqueueFrame(frame);

	// Try to start the next frame
	FrameData* nextFrame = nullptr;
	while (true)
	{
		State currentState = m_state;
		if (currentState.m_state == StateEnum::PROCESSING_FRAME)
		{
			// Already processing a frame
			break;
		}
		if (currentState.m_state == StateEnum::AWAITING_FRAME)
		{
			// Check state is consistent
			if (currentState == m_state)
			{
				// Try to acquire the state
				if (std::atomic_compare_exchange_strong(&m_state, &currentState, { StateEnum::LOADING_FRAME, currentState.m_count + 1 }))
				{
					// Dequeue the next frame and move on
					DequeueFrame(&nextFrame);
					if (nextFrame)
					{
						m_state = { StateEnum::PROCESSING_FRAME, currentState.m_count + 1 };
					}
					else
					{
						// Nothing to dequeue
						m_state = { StateEnum::AWAITING_FRAME, currentState.m_count + 1 };
						break;
					}
				}
			}
		}
	}
	// And attempt to start the next frame
	if (nextFrame)
	{
		StartFrame(*nextFrame);
	}
}

void FrameStageRunner::StartFrame(FrameData& frame)
{
	m_frameData = &frame;

	// Debugging
	m_framesBeingExecuted++;
	ASSERT(m_framesBeingExecuted.load() < 2);
	ASSERT(m_frameData->m_active == false);

	// Set this frame data active, with one dependency on FinishFrame so it won't run until we've updated the queue
	m_frameData->m_active = true;
	JobCounterPtr jobFinished = Jobs::GetNewJobCounter();
	jobFinished.m_counter->m_numJobs++;
	RunJob(jobFinished);

	// Remove one dependency on FinishJob
	jobFinished.m_counter->m_numJobs--;
}

void FrameStageRunner::RunJob(JobCounterPtr& jobFinished)
{
	RunJobInner(jobFinished);
	Jobs::CreateJobWithDependency(FinishFrame, this, jobFinished);
}

DEFINE_CLASS_JOB(FrameStageRunner, FinishFrame)
{
	// Queue frame in next stage
	ASSERT(m_nextStage != nullptr);
	m_frameData->m_active = false;
	m_nextStage->QueueFrame(*m_frameData);
	m_framesBeingExecuted--;

	// Attempt to start our next queued frame if we have one
	// Transition to LOADING_FRAME in case another thread queues something after the Dequeue below returns nullptr
	m_state = { StateEnum::LOADING_FRAME, 0 };
	FrameData* nextFrame = nullptr;
	DequeueFrame(&nextFrame);
	if (nextFrame)
	{
		m_state = { StateEnum::PROCESSING_FRAME, 0 };
		StartFrame(*nextFrame);
	}
	else
	{
		m_state = { StateEnum::AWAITING_FRAME, 0 };
	}
}

FrameStageRunner::FrameNodePtr FrameStageRunner::AllocateFrameNode()
{
	constexpr char IN_USE = 1;
	constexpr char NOT_IN_USE = 0;

	int index = 0;
	// Lock until we get a node
	while (_InterlockedCompareExchange8(&m_freeListInUse[index], IN_USE, NOT_IN_USE) == IN_USE)
	{
		index = (index + 1) % SIMULTANEOUS_FRAMES;
	}
	// Return the node
	return { &m_freeList[index], 0, index };
}

void FrameStageRunner::DeallocateFrameNode(int index)
{
	constexpr char IN_USE = 1;
	constexpr char NOT_IN_USE = 0;
	m_freeListInUse[index] = NOT_IN_USE;
}

void FrameStageRunner::EnqueueFrame(FrameData& frame)
{
	// Allocate node from free list
	FrameNodePtr nodePtr = AllocateFrameNode();
	FrameNode& node = *nodePtr.m_ptr;
	node.m_frame = &frame;
	node.m_next.store({ nullptr, 0 });

	while (true)
	{
		// Load tail
		FrameNodePtr tail = m_queue.m_tail;
		FrameNodePtr next = tail.m_ptr->m_next;
		// Check tail is consistent
		if (tail == m_queue.m_tail)
		{
			// Was tail pointing to the last node?
			if (next.m_ptr == nullptr)
			{
				// Try to link the new node to the end of the list
				if (std::atomic_compare_exchange_strong(
					&(tail.m_ptr->m_next),
					&next,
					{ &node, next.m_count + 1, nodePtr.m_index }))
				{
					// Enqueue successful - Try to point tail to the inserted node
					std::atomic_compare_exchange_strong(
						&(m_queue.m_tail),
						&tail,
						{ &node, tail.m_count + 1, nodePtr.m_index });

					break;
				}
			}
			else
			{
				// Tail wasn't pointing to the last node - try and correct it
				std::atomic_compare_exchange_strong(
					&(m_queue.m_tail),
					&tail,
					{ next.m_ptr, tail.m_count + 1, next.m_index });
			}
		}
	}
}

bool FrameStageRunner::DequeueFrame(FrameData** frameOut)
{
	while (true)
	{
		// Load relevant pointers
		FrameNodePtr head = m_queue.m_head;
		FrameNodePtr tail = m_queue.m_tail;
		FrameNodePtr next = head.m_ptr->m_next;
		// Is head consistent?
		if (head == m_queue.m_head)
		{
			// Is queue empty?
			if (head.m_ptr == tail.m_ptr)
			{
				if (next.m_ptr == nullptr)
				{
					// Queue is empty
					*frameOut = nullptr;
					return false;
				}
				// Tail wasn't pointing to last node - try to correct it
				std::atomic_compare_exchange_strong(
					&(m_queue.m_tail),
					&tail,
					{ next.m_ptr, tail.m_count + 1, next.m_index });
			}
			else
			{
				// Read value from next before moving on
				*frameOut = next.m_ptr->m_frame;
				// Try to move head to next node
				if (std::atomic_compare_exchange_strong(
					&(m_queue.m_head),
					&head,
					{ next.m_ptr, head.m_count + 1, next.m_index }))
				{
					// Dequeue successful - Free head
					if (head.m_index >= 0)
					{
						DeallocateFrameNode(head.m_index);
					}
					break;
				}
			}
		}
	}
	return true;
}
