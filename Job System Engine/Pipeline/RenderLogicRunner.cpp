#include "RenderLogicRunner.h"
#include "FrameData.h"
#include <Diagnostic/Assert.h>
#include <iostream>

void RenderLogicRunner::Init()
{
	// Load all cell shapes and their rotations
}

void RenderLogicRunner::RunJobInner(JobCounterPtr& jobCounter)
{
	// Run Render logic here

	// Determine what 'views' exist (game view, shadow map view, reflection view, etc.)

	// Compute visibility per view to get a list of things to render

	// Extract packets of render data per view

	ASSERT(m_frameData->m_stage == FrameStage::GAME_LOGIC);
	m_frameData->m_stage = FrameStage::RENDER_LOGIC;


}