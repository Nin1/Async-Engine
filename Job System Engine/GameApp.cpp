#include "GameApp.h"
#include "Assert.h"
#include <GLFW/glfw3.h>
#include <Jobs/Jobs.h>

// PIPELINE:
// 1. Gameplay logic
// 2. Render logic
// 3. GPU execution
//
// A frame cannot move to the next stage in a pipeline until that stage is free.
// When a frame moves into stage 2, a new frame is created.


void GameApp::Start()
{
	JobCounterPtr initCounter = Jobs::GetNewJobCounter();
	Jobs::CreateJobOnMainThreadAndCount(GameApp::Init, this, initCounter);
	// Run the main loop once all init jobs are completed
	Jobs::CreateJobOnMainThreadWithDependency(GameApp::StartMainLoop, this, initCounter);
}

DEFINE_CLASS_JOB(GameApp, Init)
{
	std::cout << "Initialising window" << std::endl;
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(800, 600, "Engine", nullptr, nullptr);
	glfwMakeContextCurrent(m_window);
	std::cout << "Window initialised" << std::endl;

	// Can now initialise renderer
	m_renderer.Init();

	// Initialise frame pipeline
	m_pipeline.Init();

	// Set up frames
	m_frames.reserve(FrameStageRunner::SIMULTANEOUS_FRAMES);
	for (int i = 0; i < FrameStageRunner::SIMULTANEOUS_FRAMES; i++)
	{
		m_frames.emplace_back(i, m_pipeline, m_window);
	}
}

DEFINE_CLASS_JOB(GameApp, StartMainLoop)
{
	// Kick off all frames
	for (int i = 0; i < FrameStageRunner::SIMULTANEOUS_FRAMES; i++)
	{
		m_pipeline.m_startRunner.QueueFrame(m_frames[i]);
	}
}