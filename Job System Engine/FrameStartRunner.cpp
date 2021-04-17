#include "FrameStartRunner.h"
#include "FrameData.h"
#include "Assert.h"
#include <GLFW/glfw3.h>
#include <iostream>

void FrameStartRunner::RunJobInner(JobCounterPtr& jobCounter)
{
	// Reset this frameData
	ASSERT(m_frameData->m_stage == FrameStage::GPU_EXECUTION || m_frameData->m_stage == FrameStage::FRAME_START);
	m_frameData->Reset(m_frameCount);
	m_frameCount++;
	//std::cout << "Starting frame " << m_frameData->m_frameNumber << std::endl;

	// glfw events must be run on the main thread, so do that here
	Jobs::CreateJobOnMainThreadAndCount(FrameStartRunner::MainThreadTasks, this, jobCounter);
}

DEFINE_CLASS_JOB(FrameStartRunner, MainThreadTasks)
{
	glfwPollEvents();
	if (glfwWindowShouldClose(m_frameData->m_window))
	{
		std::cout << "Closing window" << std::endl;
		glfwDestroyWindow(m_frameData->m_window);
		glfwTerminate();
		Jobs::Stop();
	}
	else
	{
		//std::cout << "Frame start: Executing " << m_frameData->m_frameNumber << std::endl;
	}
}