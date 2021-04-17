#include "GPUExecutionRunner.h"
#include "FrameData.h"
#include "Assert.h"
#include <iostream>
#include <cstdio>

void GPUExecutionRunner::RunJobInner(JobCounterPtr& jobCounter)
{
	// Run GPU execution here
	// We don't even have to use jobs if we don't want, but if we do we should use jobCounter
	ASSERT(m_frameData->m_stage == FrameStage::RENDER_LOGIC);
	m_frameData->m_stage = FrameStage::GPU_EXECUTION;
	if (m_frameData->m_frameNumber % 10000 == 0)
	{
		std::printf("Completed frame %d \n", m_frameData->m_frameNumber);
	}

	// This assert ensures that all the frames are running in the correct order
	ASSERT(m_frameData->m_frameNumber == m_framesCompleted);
	m_framesCompleted++;
}