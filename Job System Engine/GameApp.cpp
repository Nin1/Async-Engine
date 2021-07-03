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

static void APIENTRY openglCallbackFunction(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam
) {
	(void)source; (void)type; (void)id;
	(void)severity; (void)length; (void)userParam;
	fprintf(stderr, "%s\n", message);
	if (severity == GL_DEBUG_SEVERITY_HIGH) {
		fprintf(stderr, "Aborting...\n");
		//abort();
	}
}

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
	glfwInit();
	m_window = glfwCreateWindow(800, 600, "Engine", nullptr, nullptr);
	glfwMakeContextCurrent(m_window);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	//glfwSwapInterval(0); //Disables vsync
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

	// Initialise frame pipeline
	m_pipeline.Init();

	// OpenGL debug
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openglCallbackFunction, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);

	// Set up frames
	m_frames.reserve(FrameStageRunner::SIMULTANEOUS_FRAMES);
	for (int i = 0; i < FrameStageRunner::SIMULTANEOUS_FRAMES; i++)
	{
		m_frames.emplace_back(i, m_pipeline, m_window, m_input);
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