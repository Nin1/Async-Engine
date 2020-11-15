#include "GameApp.h"
#include <GLFW/glfw3.h>
#include <Jobs/Jobs.h>

void GameApp::Start()
{
	JobCounterPtr initCounter = Jobs::GetNewJobCounter();
	// Windowing jobs must all run on main thread
	Jobs::CreateJobOnMainThreadAndCount(GameApp::InitialiseWindowJob, this, initCounter);

	// Run the main loop once all init jobs are completed
	Jobs::CreateJobOnMainThreadWithDependency(GameApp::MainLoopJob, this, initCounter);
}

void GameApp::InitialiseWindow()
{
	std::cout << "Initialising window" << std::endl;
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(800, 600, "Engine", nullptr, nullptr);
	std::cout << "Window initialised" << std::endl;
}

void GameApp::MainLoop()
{
	std::cout << "Main loop start" << std::endl;

	glfwPollEvents();

	if (!glfwWindowShouldClose(m_window))
	{
		// Kick off main loop jobs
		JobCounterPtr mainLoopCounter = Jobs::GetNewJobCounter();
		// Jobs::CreateJobAndCount(someJob, someData, mainLoopCounter);

		// Run next main loop once all main loop jobs complete
		Jobs::CreateJobOnMainThreadWithDependency(GameApp::MainLoopJob, this, mainLoopCounter);
	}
	else
	{
		std::cout << "Closing window" << std::endl;
		glfwDestroyWindow(m_window);
		glfwTerminate();
		Jobs::Stop();
	}
	std::cout << "Main loop end" << std::endl;
}