#include "Input.h"
#include <functional>

bool Input::m_isWarping = false;

Input::Input()
{
}

Input::~Input()
{
}

void Input::Init(GLFWwindow* window)
{
}

void Input::RefreshInputs()
{
	m_keyEvents.clear();

	m_keysDown.fill(false);
	m_keysUp.fill(false);
	// Leave m_keysHeld as-is

	m_mouseButtonDown.fill(false);
	m_mouseButtonUp.fill(false);
	// Leave m_mouseButtonHeld as-is

	m_lastMouseOffset = glm::vec2(0, 0);
}

void Input::KeyListener(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	m_keyEvents.push_back({ action, GLFW_PRESS });

	switch (action)
	{
	case GLFW_PRESS:
		m_keysDown[key] = true;
		m_keysUp[key] = false;
		m_keysHeld[key] = true;
		break;
	case GLFW_REPEAT:
		m_keysHeld[key] = true;
		break;
	case GLFW_RELEASE:
		m_keysDown[key] = false;
		m_keysUp[key] = true;
		m_keysHeld[key] = false;
		break;
	}
}

void Input::MouseButtonListener(GLFWwindow* window, int button, int action, int mods)
{
	switch (action)
	{
	case GLFW_PRESS:
		m_mouseButtonDown[button] = true;
		m_mouseButtonUp[button] = false;
		m_mouseButtonHeld[button] = true;
		break;
	case GLFW_RELEASE:
		m_mouseButtonDown[button] = false;
		m_mouseButtonUp[button] = true;
		m_mouseButtonHeld[button] = false;
		break;
	}
}

void Input::MousePosListener(GLFWwindow* window, double xpos, double ypos)
{
	glm::vec2 newMousePos(xpos, ypos);
	m_lastMouseOffset = newMousePos - m_mousePos;
	m_mousePos = newMousePos;
}

void Input::WarpMousePos(int x, int y)
{
	//m_isWarping = true;
	//glutWarpPointer(x, y);
	//m_mousePos = glm::vec2(x, y);
}

InputState Input::ExtractInputState()
{
	return InputState(
		m_keysDown, m_keysHeld, m_keysUp,
		m_mouseButtonDown, m_mouseButtonHeld, m_mouseButtonUp,
		m_mousePos, m_lastMouseOffset);
}