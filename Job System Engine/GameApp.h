#pragma once
#include <GLFW/glfw3.h>

class GameApp
{
public:
	void Start();

private:
	// Jobs
	static void InitialiseWindowJob(void* app) { static_cast<GameApp*>(app)->InitialiseWindow(); }
	static void MainLoopJob(void* app) { static_cast<GameApp*>(app)->MainLoop(); }

	void InitialiseWindow();
	void MainLoop();

private:
	GLFWwindow* m_window;
};

