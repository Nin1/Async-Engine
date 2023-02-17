#pragma once
#include <Jobs/JobDecl.h>
#include <Jobs/Jobs.h>
#include <array>
#include <atomic>

template<typename DATA>
struct FrameData;

/** Base class for pipeline stages. A frame can be queued here and processed in order before being sent to the next stage. */
template<typename DATA>
class FrameStageRunner
{
public:
	static constexpr int SIMULTANEOUS_FRAMES = 6;

	FrameStageRunner();
	FrameStageRunner(const char* name);
	FrameStageRunner(const FrameStageRunner<DATA>&) = delete;
	FrameStageRunner& operator=(const FrameStageRunner<DATA>&) = delete;
	~FrameStageRunner() = default;

	/** Overridable initialisation that runs on app start-up */
	virtual void Init() { }

	/** Sets the stage runner that frames are sent to after this stage. Only set once on initialisation. */
	void SetNextStage(FrameStageRunner<DATA>* nextStage) { m_nextStage = nextStage; }
	/** Queues the frame to be run at some point in the future. */
	void QueueFrame(FrameData<DATA>& frame);

protected:
	/** Abstract method for executing whatever the stage wants. Any jobs that need to complete before this frame can finish must add to jobCounter. */
	virtual void RunJobInner(JobCounterPtr& jobCounter) = 0;

private:
	enum class StateEnum : char
	{
		AWAITING_FRAME,
		LOADING_FRAME,
		PROCESSING_FRAME,
	};

	struct State
	{
		StateEnum m_state;
		uint64_t m_count;

		bool operator==(const State& rhs)
		{
			return m_state == rhs.m_state && m_count == rhs.m_count;
		}
	};

	template<typename DATA>
	struct FrameNode;

	template<typename DATA>
	struct FrameNodePtr
	{
		FrameNode<DATA>* m_ptr;
		uint64_t m_count;
		int m_index;

		bool operator==(const FrameNodePtr& rhs)
		{
			return m_ptr == rhs.m_ptr && m_count == rhs.m_count;
		}
	};

	template<typename DATA>
	struct FrameNode
	{
		FrameData<DATA>* m_frame;
		std::atomic<FrameNodePtr<DATA>> m_next;
	};

	struct FrameQueue
	{
		std::atomic<FrameNodePtr<DATA>> m_head;
		std::atomic<FrameNodePtr<DATA>> m_tail;
	};

	/** Initialises the frame queue */
	void InitFrameQueue();
	/** Start the next frame */
	void StartFrame(FrameData<DATA>& frame);
	/** Calls the overridable RunJobInner, and creates the FinishFrame job for when the stage has finished */
	void RunJob(JobCounterPtr& jobFinished);
	/** The first job called when processing a frame */
	DECLARE_TEMPLATE_CLASS_JOB(DATA, FrameStageRunner, StartFrameJob);
	/** Runs just after a frame has finished processing, to reset m_thisFrame and attempt to start the next frame. */
	DECLARE_TEMPLATE_CLASS_JOB(DATA, FrameStageRunner, FinishFrameJob);

	/** Thread-safe enqueues a frame into the lock-free linkedlist */
	void EnqueueFrame(FrameData<DATA>& frame);
	/** Thread-safe dequeues a frame from the lock-free linkedlist */
	bool DequeueFrame(FrameData<DATA>** frameOut);
	/** Allocates a frame node from the free list */
	FrameNodePtr<DATA> AllocateFrameNode();
	/** Releases a frame node back to the free list */
	void DeallocateFrameNode(int index);

protected:
	/** Atomic state for handling transitions between frames */
	std::atomic<State> m_state;

	/** Quick accessor for the currently active frame data */
	FrameData<DATA>* m_frameData = nullptr;
	/** Job counter for the running frame - Only valid while a frame is processing */
	JobCounterPtr m_jobCounter;

private:
	/** Lock-free concurrent queue of frames to run */
	FrameQueue m_queue;
	/** Free list for allocating FrameNodes */
	std::array<FrameNode<DATA>, SIMULTANEOUS_FRAMES> m_freeList;
	std::array<char, SIMULTANEOUS_FRAMES> m_freeListInUse;
	/** Stage to send completed frames to */
	FrameStageRunner* m_nextStage = nullptr;

	// Debug
	const char* m_name;
	std::atomic<int> m_framesBeingExecuted = 0; // Should never be more than 1
};

#include "FrameStageRunner.inl"

