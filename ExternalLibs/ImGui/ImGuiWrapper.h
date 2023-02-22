#pragma once
#include "imgui.h"
#include <functional>
#include <vector>

/**
 * CUSTOM WRAPPER FOR ASYNC ENGINE
 *
 * Wrapper for ImGui that collects ImGui calls during the execution of a frame in a FramePipeline.
 * This is necessary because ImGui uses global state and is not thread-safe, and FramePipeline executes multiple frames at once.
 * To work around this, we collect ImGui calls so they can all be executed during a single stage of the pipeline.
 * Put an instance of this in your FrameData.
 *
 * Note that pointers you pass to ImGui controls will be modified by the thread that calls 'Execute()', rather than the thread using those variables!
 * Pipeline:
 *		INPUT STAGE - No ImGui calls
 *		GAME LOGIC STAGE - Collect calls
 *		RENDER STAGE - Collect calls, call ImGui::NewFrame(), call ExecuteQueue(), and render
 */
class ImGuiFrameWrapper
{
public:
	/** Executes all queued functions and stores the draw data for them. */
	void ExecuteQueue()
	{
		// Execute collected calls
		for (const auto& call : m_calls)
		{
			call();
		}
	}

	/** 
	 * Queues a function to be executed later. Note that any variable pointers passed to an ImGui control
	 * may be modified by the executing thread, rather than the thread using those variables.
	 */
	template<typename FUNC, typename... ARGS>
	void Queue(FUNC func, ARGS... args)
	{
		m_calls.push_back([func, args...]()
			{
				(func)(args...);
			});
	}

	/** Queue a complex call to ImGui functions */
	void QueueComplex(std::function<void()> onCalled)
	{
		m_calls.push_back([=]()
			{
				onCalled();
			});
	}
	
	/** Queues a button with a label and callback */
	void QueueButton(const char* label, std::function<void()> onClicked)
	{
		m_calls.push_back([=]()
			{
				if (ImGui::Button(label))
				{
					onClicked();
				}
			});
	}

	void Clear()
	{
		m_calls.clear();
	}

private:
	/** List of ImGui function calls in order */
	std::vector<std::function<void()>> m_calls;
};
