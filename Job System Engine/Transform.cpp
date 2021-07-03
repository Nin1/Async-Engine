#include "Transform.h"
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>

Transform::Transform(Transform& parent)
	: m_parent(&parent)
{
	m_localScale = glm::vec3(1.0f);
	m_localRotation = glm::vec3(0.0f);
	m_localPosition = glm::vec3(0.0f);
}

Transform::~Transform()
{
}

void Transform::Rotate(glm::vec3 rotation)
{
	m_localRotation += rotation; m_dirtyTRS = true;
}

glm::mat4 Transform::GetTRS()
{
	// TODO: m_dirtyTRS doesn't get set when the parent updates
	if (m_dirtyTRS)
	{
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_localScale);
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_localPosition);
		glm::vec3 eulerAngles = m_localRotation * (3.14159f / 180.0f);
		glm::mat4 eulerRotation = glm::eulerAngleYXZ(eulerAngles.y, eulerAngles.x, eulerAngles.z);

		if (m_parent)
		{
			m_trs = m_parent->GetTRS() * translate * eulerRotation * scale;
		}
		else
		{
			m_trs = translate * eulerRotation * scale;
		}
		m_dirtyTRS = false;
	}
	return m_trs;
}

glm::vec3 Transform::GetWorldPosition()
{
	// TODO: Get position in relation to translate/rotate/scale of parent(s)
	return m_localPosition;
}

glm::vec3 Transform::GetWorldRotation() const
{
	glm::vec3 rot = m_localRotation;
	if (m_parent)
	{
		rot += m_parent->GetWorldRotation();
	}
	return rot;
}

glm::vec3 Transform::GetWorldRotationRadians() const
{
	return GetWorldRotation() / (180.0f / 3.14159f);
}

glm::vec3 Transform::GetWorldScale() const
{
	glm::vec3 scale = m_localScale;
	if (m_parent)
	{
		scale *= m_parent->GetWorldScale();
	}
	return scale;
}

glm::vec3 Transform::GetForwardVector() const
{
	glm::vec3 forward;
	const glm::vec3& rot = GetWorldRotationRadians();
	forward.x = cos(rot.x) * cos(rot.y);
	forward.y = sin(rot.x);
	forward.z = cos(rot.x) * sin(rot.y);
	return forward;
}

glm::vec3 Transform::GetRightVector() const
{
	return glm::normalize(glm::cross(GetForwardVector(), glm::vec3(0, 1, 0)));
}