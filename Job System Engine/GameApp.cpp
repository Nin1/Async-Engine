#include "GameApp.h"
#include "Assert.h"
#include "ClientFramePipeline/OpenGLRenderRunner.h"
#include "ClientFramePipeline/GameLogicRunner.h"
#include "ClientFramePipeline/FrameStartRunner.h"
#include <GLFW/glfw3.h>
#include <Jobs/Jobs.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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
	Jobs::CreateJobAndCount(GameApp::Init, this, JOBFLAG_MAINTHREAD, initCounter);
	// Run the main loop once all init jobs are completed
	Jobs::CreateJobWithDependency(GameApp::StartMainLoop, this, JOBFLAG_MAINTHREAD, initCounter);
}

DEFINE_CLASS_JOB(GameApp, Init)
{
	std::cout << "Initialising window" << std::endl;

	// Init GLFW and window
	if (glfwInit() == GLFW_FALSE)
	{
		Jobs::Stop();
		std::cout << "Error initialising GLFW" << std::endl;
		return;
	}
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	m_window = glfwCreateWindow(800, 600, "Engine", nullptr, nullptr);
	if (m_window == nullptr)
	{
		Jobs::Stop();
		std::cout << "Error creating window" << std::endl;
		return;
	}
	glfwMakeContextCurrent(m_window);
	glfwSwapInterval(0); //Disables vsync
	std::cout << "Window initialised" << std::endl;

	// Set up input callbacks - GLFW is a bit nasty in this regard. GLFW will call these during glfwPollEvents().
	glfwSetWindowUserPointer(m_window, this);
	glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			GameApp* thisApp = (GameApp*)glfwGetWindowUserPointer(window);
			thisApp->InputKeyListener(window, key, scancode, action, mods);
		});
	glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int key, int action, int mods)
		{
			GameApp* thisApp = (GameApp*)glfwGetWindowUserPointer(window);
			thisApp->InputMouseButtonListener(window, key, action, mods);
		});
	glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos)
		{
			GameApp* thisApp = (GameApp*)glfwGetWindowUserPointer(window);
			thisApp->InputMousePosListener(window, xpos, ypos);
		});

	// Init ImGui
	// Note: ImGui is not thread-safe, and as it has a single global state only one frame and one thread should do anything with it at one time.
	// To do this, we will need to use a wrapper within ClientFrameData to collect imgui calls throughout a frame.
	// At the end of the frame, we will then forward all inputs from our input state to it, execute the collected calls, and render it all in the Render stage.
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(m_window, false);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Initialise frame pipeline
	constexpr int NUM_SIMULTANEOUS_FRAMES = 4;
	std::vector<std::unique_ptr<FrameStageRunner<ClientFrameData>>> stages;
	stages.emplace_back(std::make_unique<FrameStartRunner>());
	stages.emplace_back(std::make_unique<GameLogicRunner>());
	stages.emplace_back(std::make_unique<OpenGLRenderRunner>());
	m_pipeline.Init(std::move(stages), NUM_SIMULTANEOUS_FRAMES, ClientFrameData(m_window, m_input));
}

DEFINE_CLASS_JOB(GameApp, StartMainLoop)
{
	m_pipeline.Start();
}