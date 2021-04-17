#include "RenderLogicRunner.h"
#include "FrameData.h"
#include "Assert.h"
#include <iostream>

void RenderLogicRunner::RunJobInner(JobCounterPtr& jobCounter)
{
	// Run Render logic here
	// We don't even have to use jobs if we don't want, but if we do we should use jobCounter
	//std::cout << "Render logic completed for frame " << m_frameData->m_frameNumber << std::endl;
	ASSERT(m_frameData->m_stage == FrameStage::GAME_LOGIC);
	m_frameData->m_stage = FrameStage::RENDER_LOGIC;
}