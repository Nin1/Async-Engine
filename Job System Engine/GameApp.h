#pragma once
#include "FrameData.h"
#include "FramePipeline.h"
#include "Scene.h"
#include "SimpleRenderer.h"
#include <Jobs/JobDecl.h>

class GameApp
{
public:

	void Start();

	static void StartNewFrame();

private:
	// Jobs
	// Initialise systems
	DECLARE_CLASS_JOB(GameApp, Init);
	// Start processing frames
	DECLARE_CLASS_JOB(GameApp, StartMainLoop);

private:
	Scene m_scene;
	SimpleRenderer m_renderer;
	FramePipeline m_pipeline;

	// TODO: Singleton
	GLFWwindow* m_window;

	std::vector<FrameData> m_frames;
};

