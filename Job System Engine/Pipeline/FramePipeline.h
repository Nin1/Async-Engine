#pragma once
#include "FrameStartRunner.h"
#include "GameLogicRunner.h"
#include "RenderLogicRunner.h"
#include "OpenGLRenderRunner.h"
#include "VulkanRenderRunner.h"

enum class RenderAPI
{
	OPENGL,
	VULKAN
};

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

	void Init(RenderAPI renderAPI)
	{
		// Set up pipeline order
		m_startRunner.SetNextStage((FrameStageRunner*)&m_gameRunner);
		switch (renderAPI)
		{
		case RenderAPI::OPENGL:
			m_gameRunner.SetNextStage((FrameStageRunner*)&m_openGLRunner);
			m_openGLRunner.SetNextStage((FrameStageRunner*)&m_startRunner);
			break;
		case RenderAPI::VULKAN:
			m_gameRunner.SetNextStage((FrameStageRunner*)&m_vulkanRunner);
			m_vulkanRunner.SetNextStage((FrameStageRunner*)&m_startRunner);
		}

		// Initialise pipeline stages
		m_startRunner.Init();
		m_gameRunner.Init();
		switch (renderAPI)
		{
		case RenderAPI::OPENGL:
			m_openGLRunner.Init();
			break;
		case RenderAPI::VULKAN:
			m_vulkanRunner.Init();
		}
	}

	FrameStartRunner m_startRunner;
	GameLogicRunner m_gameRunner;
	OpenGLRenderRunner m_openGLRunner;
	VulkanRenderRunner m_vulkanRunner;

};