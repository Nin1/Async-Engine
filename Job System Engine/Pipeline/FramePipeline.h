#pragma once
#include "FrameStartRunner.h"
#include "GameLogicRunner.h"
#include "RenderLogicRunner.h"
#include "GPUExecutionRunner.h"

/**
 * The pipeline through which a frame ends up drawn to the screen.
 *
 * Stages that may be useful:
 * - Frame start
 * - Game scripts
 * - Physics simulation
 * - Visibility & occlusion
 * - Render data extraction
 * - Render data processing (& GPU command buffer generation)
 * - GPU execution
 *
 */
struct FramePipeline
{
	void Init()
	{
		m_startRunner.SetNextStage((FrameStageRunner*)&m_gameRunner);
		m_gameRunner.SetNextStage((FrameStageRunner*)&m_renderRunner);
		m_renderRunner.SetNextStage((FrameStageRunner*)&m_gpuRunner);
		m_gpuRunner.SetNextStage((FrameStageRunner*)&m_startRunner);

		m_startRunner.Init();
		m_gameRunner.Init();
		m_renderRunner.Init();
		m_gpuRunner.Init();
	}

	FrameStartRunner m_startRunner;
	GameLogicRunner m_gameRunner;
	RenderLogicRunner m_renderRunner;
	GPUExecutionRunner m_gpuRunner;
};