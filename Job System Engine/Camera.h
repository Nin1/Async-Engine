#pragma once
#include "Transform.h"
#include "CameraData.h"

class Camera
{
public:
	Camera() {}
	~Camera() {}

	/** Returns matrices and other data about the camera for rendering */
	CameraData GetFrameData();

	void SetOrthographic(bool set) { m_orthographic = set; }

	float GetVerticalFoV();
	float GetNearClipPlane() { return m_nearClipPlane; }

private:
	/** Calculate and store the current projection matrix in m_projMatrix */
	glm::mat4 CalculateCurrentProjMatrix();
	/** Calculate and store the current view matrix in m_viewMatrix */
	glm::mat4 CalculateCurrentViewMatrix();

public:
	Transform m_transform;

private:
	/** Aspect ratio (width / height) */
	float m_aspect = 4.0f / 3.0f;

	bool m_orthographic = false;

	float m_fieldOfView = 85.0f;
	float m_nearClipPlane = 0.01f;
	float m_farClipPlane = 1000.0f;
};

