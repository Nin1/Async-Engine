#pragma once
#include <GL/glew.h>
#include <FramePipeline/FramePipeline.h>
#include <Jobs/JobDecl.h>
#include "ClientFramePipeline/ClientFrameData.h"
#include <Input/Input.h>

class GameApp
{
public:

	void Start();

	static void StartNewFrame();

	/** Forwarders for GLFW callback functions */
	void InputKeyListener(GLFWwindow* window, int key, int scancode, int action, int mods) { m_input.KeyListener(window, key, scancode, action, mods); }
	void InputMouseButtonListener(GLFWwindow* window, int key, int action, int mods) { m_input.MouseButtonListener(window, key, action, mods); }
	void InputMousePosListener(GLFWwindow* window, double xpos, double ypos) { m_input.MousePosListener(window, xpos, ypos); }

private:
	// Jobs
	// Initialise systems
	DECLARE_CLASS_JOB(GameApp, Init);
	// Start processing frames
	DECLARE_CLASS_JOB(GameApp, StartMainLoop);

private:
	Input m_input;
	FramePipeline<ClientFrameData> m_pipeline;

	// TODO: Singleton
	GLFWwindow* m_window;
};

