#pragma once
#include "FrameStartRunner.h"
#include "GameLogicRunner.h"
#include "RenderLogicRunner.h"
#include "GPUExecutionRunner.h"

struct FramePipeline
{
	void Init()
	{
		m_startRunner.SetNextStage((FrameStageRunner*)&m_gameRunner);
		m_gameRunner.SetNextStage((FrameStageRunner*)&m_renderRunner);
		m_renderRunner.SetNextStage((FrameStageRunner*)&m_gpuRunner);
		m_gpuRunner.SetNextStage((FrameStageRunner*)&m_startRunner);
	}

	FrameStartRunner m_startRunner;
	GameLogicRunner m_gameRunner;
	RenderLogicRunner m_renderRunner;
	GPUExecutionRunner m_gpuRunner;
};