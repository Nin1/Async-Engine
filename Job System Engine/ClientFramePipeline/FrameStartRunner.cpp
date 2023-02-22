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
	double currentTime = glfwGetTime();
	if (glfwWindowShouldClose(frameData.m_window))
	{
		std::cout << "Closing window" << std::endl;
		glfwDestroyWindow(frameData.m_window);
		glfwTerminate();
		Jobs::Stop();
		return;
	}

	// Capture input state at the start of this frame
	frameData.m_input = frameData.m_inputHandler.ExtractInputState();
	frameData.m_deltaTime = currentTime - m_lastFrameStartTime;
	m_lastFrameStartTime = currentTime;

#if JOBS_COLLECT_METRICS
	frameData.m_imgui.Queue(ImGui::Begin, "Job metrics", nullptr, 0);
	frameData.m_imgui.QueueComplex([]()
		{
			ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
		});
	frameData.m_imgui.QueueButton("Reset", []()
		{
			Jobs::ResetMetrics();
		});
	int totalJobsExecuted = 0;
	for (size_t thread = 0; thread < Jobs::GetNumThreads(); thread++)
	{
		// Get data from Jobs system
		int numExecuted = Jobs::GetNumExecutedLoops(thread);
		totalJobsExecuted += numExecuted;
		int numStarved = Jobs::GetNumStarvedLoops(thread);
		int totalLoops = numExecuted + numStarved;
		float percentageStarved = (float)numStarved * 100.0f / (float)totalLoops;
		int ownExecuted = Jobs::GetNumOwnJobs(thread);
		int stolenExecuted = Jobs::GetNumStolenJobs(thread);
		int totalExecuted = ownExecuted + stolenExecuted;
		float percentageStolen = (float)stolenExecuted * 100.0f / (float)totalExecuted;
		int jobsCreated = Jobs::GetNumJobsCreated(thread);
		int mainThreadJobsCreated = Jobs::GetNumMainThreadJobsCreated(thread);
		long long timeInJobsNS = Jobs::GetTimeInJobsNS(thread);
		long long timeNotInJobsNS = Jobs::GetTimeNotInJobsNS(thread);
		long long totalTimeNS = timeInJobsNS + timeNotInJobsNS;
		double timeInJobsS = double(timeInJobsNS) / 1000000000.0f;
		double timeNotInJobsS = double(timeNotInJobsNS) / 1000000000.0f;
		double totalTimeS = double(totalTimeNS) / 1000000000.0f;
		float percentageTimeInJobs = (float)timeInJobsS * 100.0f / (float)totalTimeS;


		// Format data in imgui
		if (thread == Jobs::GetMainThreadIndex())
		{
			frameData.m_imgui.Queue(ImGui::Text, "Thread %d (Main Thread)", thread);
		}
		else
		{
			frameData.m_imgui.Queue(ImGui::Text, "Thread %d", thread);
		}
		frameData.m_imgui.Queue(ImGui::Separator);
		frameData.m_imgui.Queue(ImGui::Text, "Total loops: %d", totalLoops);
		frameData.m_imgui.Queue(ImGui::Text, " - Execute:   %d", numExecuted);
		frameData.m_imgui.Queue(ImGui::Text, " - Starved:   %d", numStarved);
		frameData.m_imgui.Queue(ImGui::Text, " - Percentage starved: %f", percentageStarved);
		frameData.m_imgui.Queue(ImGui::Text, "Total executed: %d", totalExecuted);
		frameData.m_imgui.Queue(ImGui::Text, " - Own:    %d", ownExecuted);
		frameData.m_imgui.Queue(ImGui::Text, " - Stolen: %d", stolenExecuted);
		frameData.m_imgui.Queue(ImGui::Text, " - Percentage stolen: %f", percentageStolen);
		frameData.m_imgui.Queue(ImGui::Text, "Total created: %d", jobsCreated);
		frameData.m_imgui.Queue(ImGui::Text, " - Main thread: %d", mainThreadJobsCreated);
		frameData.m_imgui.Queue(ImGui::Text, "Total time: %f", totalTimeS);
		frameData.m_imgui.Queue(ImGui::Text, " - In jobs:    %f", timeInJobsS);
		frameData.m_imgui.Queue(ImGui::Text, " - Not in jobs: %f", timeNotInJobsS);
		frameData.m_imgui.Queue(ImGui::Text, " - Percentage in jobs: %f", percentageTimeInJobs);
		frameData.m_imgui.Queue(ImGui::Text, "");
	}
	frameData.m_imgui.Queue(ImGui::Text, "Total executed (all threads): %d", totalJobsExecuted);
	// Calculate jobs-per-second
	const auto& lastResetTime = Jobs::GetLastMetricResetTime();
	auto duration = std::chrono::high_resolution_clock::now() - lastResetTime;
	auto seconds = std::chrono::duration_cast<std::chrono::duration<float>>(duration);
	float jobsPerSecond = (float)totalJobsExecuted / seconds.count();
	frameData.m_imgui.Queue(ImGui::Text, "Jobs-per-second: %f", jobsPerSecond);

	frameData.m_imgui.Queue(ImGui::End);
#endif
}