#pragma once
#include <glm\mat4x4.hpp>

/** Contains per-frame data about the camera that is needed to render the scene */
struct CameraData
{
	glm::mat4 m_projMatrix;
	glm::mat4 m_viewMatrix;

	float m_fieldOfView = 85.0f;
	float m_nearClipPlane = 0.01f;
	float m_farClipPlane = 1000.0f;
};