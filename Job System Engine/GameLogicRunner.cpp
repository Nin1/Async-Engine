#include "GameLogicRunner.h"
#include "FrameData.h"
#include "Assert.h"
#include <iostream>

void GameLogicRunner::RunJobInner(JobCounterPtr& jobCounter)
{
	// Run game logic here
	// We don't even have to use jobs if we don't want, but if we do we should use jobCounter
	ASSERT(m_frameData->m_stage == FrameStage::FRAME_START);
	m_frameData->m_stage = FrameStage::GAME_LOGIC;
	//std::cout << "Game logic completed for frame " << m_frameData->m_frameNumber << std::endl;
}