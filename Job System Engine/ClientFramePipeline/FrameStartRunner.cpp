#include "FrameStartRunner.h"
#include <Input/Input.h>
#include <GLFW/glfw3.h>
#include <iostream>

void FrameStartRunner::RunJobInner()
{
	// Reset this frameData
	ClientFrameData& frameData = *m_frameData->GetData();
	frameData.Reset(m_frameCount);
	m_frameCount++;

	// glfw events must be run on the main thread, so do that here
	Jobs::CreateJob(FrameStartRunner::MainThreadTasks, this, JOBFLAG_MAINTHREAD | JOBFLAG_ISCHILD);
}

DEFINE_CLASS_JOB(FrameStartRunner, MainThreadTasks)
{
	ClientFrameData& frameData = *m_frameData->GetData();
	frameData.m_inputHandler.RefreshInputs();
	glfwPollEvents();
	if (glfwWindowShouldClose(frameData.m_window))
	{
		std::cout << "Closing window" << std::endl;
		glfwDestroyWindow(frameData.m_window);
		glfwTerminate();
		Jobs::Stop();
	}
	else
	{
		// Capture input state at the start of this frame
		frameData.m_input = frameData.m_inputHandler.ExtractInputState();
	}
}