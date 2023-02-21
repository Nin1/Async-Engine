#pragma once
#include "FrameStageRunner.h"
#include <Diagnostic/Assert.h>

template<typename DATA>
struct FrameData;

/**
 * The pipeline through which a frame ends up drawn to the screen. Multiple frames can be executed in parallel.
 * Create by passing a vector of stages in order into Init().
 * Call Start() to begin pushing frames through the pipeline. When they complete, they will return to the start of the pipeline.
 *
 * Stages that may be useful:
 * - Frame start
 * - Game scripts & Physics simulation
 * - Visibility & occlusion
 * - Render data extraction
 * - Render data processing (& GPU command buffer generation)
 * - GPU execution
 *
 */
template<typename DATA>
struct FramePipeline
{
	void Init(std::vector<std::unique_ptr<FrameStageRunner<DATA>>>&& pipelineStages, int numSimultaneousFrames, const DATA& defaultData)
	{
		m_stages = std::move(pipelineStages);

		// Link each stage to the next
		for (int i = 0; i < m_stages.size() - 1; i++)
		{
			m_stages[i]->SetNextStage(m_stages[i + 1].get());
		}
		// Final stage loops back to start
		m_stages[m_stages.size() - 1]->SetNextStage(m_stages[0].get());

		// Initialise each stage
		for (int i = 0; i < m_stages.size(); i++)
		{
			m_stages[i]->InitFrameQueue(numSimultaneousFrames);
			m_stages[i]->Init();
		}

		// Set up frames
		m_frames.reserve(numSimultaneousFrames);
		for (int i = 0; i < numSimultaneousFrames; i++)
		{
			m_frames.emplace_back(i, defaultData);
		}
	}

	void Start()
	{
		// Kick off all frames
		for (int i = 0; i < m_frames.size(); i++)
		{
			m_stages[0]->QueueFrame(m_frames[i]);
		}
	}

	/** Stages that a frame goes through in order */
	std::vector<std::unique_ptr<FrameStageRunner<DATA>>> m_stages;
	/** Objects containing transient data for each frame in flight */
	std::vector<FrameData<DATA>> m_frames;
};