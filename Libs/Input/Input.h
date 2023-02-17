#pragma once
#include "InputState.h"

/** Wrapper for the glfw input handler */
class Input
{
public:
	Input();
	~Input();

	void Init(GLFWwindow* window);

	/** Call once-per-frame to refresh the up/down key states */
	void RefreshInputs();

	/** Set the new state of a given key */
	void KeyListener(GLFWwindow* window, int key, int scancode, int action, int mods);
	/** Set the new state of a given mouse button */
	void MouseButtonListener(GLFWwindow* window, int button, int action, int mods);
	/** Set the new position of the mouse */
	void MousePosListener(GLFWwindow* window, double xpos, double ypos);

	/** Set the new position of the mouse (Does not update the mouse offset) */
	void WarpMousePos(int x, int y);

	/** Copies the input state into a struct and returns it */
	InputState ExtractInputState();

	static bool m_isWarping;

private:
	/** Key states for polling */
	std::array<bool, GLFW_KEY_LAST> m_keysDown;
	std::array<bool, GLFW_KEY_LAST> m_keysUp;
	std::array<bool, GLFW_KEY_LAST> m_keysHeld;

	/** Key events in the order they were received during the last frame */
	struct KeyEvent
	{
		int m_key;
		int m_action;
	};
	std::vector<KeyEvent> m_keyEvents;

	/** Mouse button states for polling */
	std::array<bool, GLFW_MOUSE_BUTTON_LAST> m_mouseButtonDown;
	std::array<bool, GLFW_MOUSE_BUTTON_LAST> m_mouseButtonUp;
	std::array<bool, GLFW_MOUSE_BUTTON_LAST> m_mouseButtonHeld;

	/** Mouse position this frame */
	glm::vec2 m_mousePos;
	/** Mouse position delta from last frame */
	glm::vec2 m_lastMouseOffset;
};

