#include "FrameStartRunner.h"
#include "FrameData.h"
#include "../Assert.h"
#include "../Input.h"
#include <GLFW/glfw3.h>
#include <iostream>

void FrameStartRunner::RunJobInner(JobCounterPtr& jobCounter)
{
	// Reset this frameData
	m_frameData->Reset(m_frameCount);
	m_frameCount++;

	// glfw events must be run on the main thread, so do that here
	Jobs::CreateJobAndCount(FrameStartRunner::MainThreadTasks, this, JOBFLAG_MAINTHREAD, jobCounter);
}

DEFINE_CLASS_JOB(FrameStartRunner, MainThreadTasks)
{
	m_frameData->m_inputHandler.RefreshInputs();
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
		// Capture input state at the start of this frame
		m_frameData->m_input = m_frameData->m_inputHandler.ExtractInputState();
	}
}