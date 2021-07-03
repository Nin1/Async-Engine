#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

CameraData Camera::GetFrameData()
{
	CameraData data;
	data.m_projMatrix = CalculateCurrentProjMatrix();
	data.m_viewMatrix = CalculateCurrentViewMatrix();
	data.m_fieldOfView = m_fieldOfView;
	data.m_farClipPlane = m_farClipPlane;
	data.m_nearClipPlane = m_nearClipPlane;
	return data;
}

glm::mat4 Camera::CalculateCurrentProjMatrix()
{
	if (!m_orthographic)
	{
		auto fov = glm::radians(m_fieldOfView);
		return glm::perspective(fov, m_aspect, m_nearClipPlane, m_farClipPlane);
	}
	else
	{
		return glm::ortho(-650.0f, 650.0f, -350.0f, 300.0f, -0.0f, 1000.0f);
	}
}

glm::mat4 Camera::CalculateCurrentViewMatrix()
{
	const glm::vec3& cameraPosition = m_transform.GetWorldPosition();

	return glm::lookAt(
		cameraPosition,						// Camera position
		cameraPosition + m_transform.GetForwardVector(),	// Camera "look-at" point
		glm::vec3(0, 1, 0)					// Up vector
	);
}

float Camera::GetVerticalFoV()
{
	return (m_fieldOfView / 4) * 3; // 4:3
}
