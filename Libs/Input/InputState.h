#pragma once
#include <array>
#include <vector>
#include <glm/vec2.hpp>
#include <GLFW/glfw3.h>

struct InputState
{
public:
	InputState() {}
	InputState(
		const std::array<bool, GLFW_KEY_LAST>& keysDown,
		const std::array<bool, GLFW_KEY_LAST>& keysHeld,
		const std::array<bool, GLFW_KEY_LAST>& keysUp,
		const std::array<bool, GLFW_MOUSE_BUTTON_LAST> mouseButtonDown,
		const std::array<bool, GLFW_MOUSE_BUTTON_LAST> mouseButtonHeld,
		const std::array<bool, GLFW_MOUSE_BUTTON_LAST> mouseButtonUp,
		const glm::vec2& mousePos,
		const glm::vec2& mouseDelta)
		: m_keysDown(keysDown)
		, m_keysHeld(keysHeld)
		, m_keysUp(keysUp)
		, m_mouseButtonDown(mouseButtonDown)
		, m_mouseButtonHeld(mouseButtonHeld)
		, m_mouseButtonUp(mouseButtonUp)
		, m_mousePos(mousePos)
		, m_lastMouseOffset(mouseDelta)
	{ }

	/** @return true if the given key is in the desired state */
	bool GetKeyDown(int keyCode) const { return m_keysDown[keyCode]; }
	bool GetKeyHeld(int keyCode) const { return m_keysHeld[keyCode]; }
	bool GetKeyUp(int keyCode) const { return m_keysUp[keyCode]; }

	/** @return true if the given mouse button is in the desired state */
	bool GetMouseDown(int button) const { return m_mouseButtonDown[button]; }
	bool GetMouseHeld(int button) const { return m_mouseButtonHeld[button]; }
	bool GetMouseUp(int button) const { return m_mouseButtonUp[button]; };

	/** @return the current mouse position */
	const glm::vec2& GetMousePos() const { return m_mousePos; }
	/** @return the difference in mouse position between this frame and the last */
	const glm::vec2& GetLastMouseOffset() const { return m_lastMouseOffset; }

private:
	/** Key states for polling */
	std::array<bool, GLFW_KEY_LAST> m_keysDown;
	std::array<bool, GLFW_KEY_LAST> m_keysUp;
	std::array<bool, GLFW_KEY_LAST> m_keysHeld;
	/** Mouse button states for polling */
	std::array<bool, GLFW_MOUSE_BUTTON_LAST> m_mouseButtonDown;
	std::array<bool, GLFW_MOUSE_BUTTON_LAST> m_mouseButtonUp;
	std::array<bool, GLFW_MOUSE_BUTTON_LAST> m_mouseButtonHeld;
	/** Mouse position this frame */
	glm::vec2 m_mousePos;
	/** Mouse position delta from last frame */
	glm::vec2 m_lastMouseOffset;
};
