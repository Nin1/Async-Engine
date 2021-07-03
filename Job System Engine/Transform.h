#pragma once
#include <glm\vec3.hpp>
#include <glm\mat4x4.hpp>

class Transform
{
public:
	Transform() {}
	Transform(Transform& parent);
	~Transform();

	/** Set the element of this transform in local space */
	void SetLocalPosition(glm::vec3 position) { m_localPosition = position; m_dirtyTRS = true; }
	void SetLocalRotation(glm::vec3 rotation) { m_localRotation = rotation; m_dirtyTRS = true; }
	void SetLocalScale(glm::vec3 scale) { m_localScale = scale; m_dirtyTRS = true; }

	/** @return the element of this transform in local space */
	const glm::vec3& GetLocalPosition() const { return m_localPosition; }
	const glm::vec3& GetLocalRotation() const { return m_localRotation; }
	const glm::vec3& GetLocalScale() const { return m_localScale; }

	/** @return the element of this transform in world space */
	glm::vec3 GetWorldPosition();
	glm::vec3 GetWorldRotation() const;
	glm::vec3 GetWorldRotationRadians() const;
	glm::vec3 GetWorldScale() const;
	glm::vec3 GetForwardVector() const;
	glm::vec3 GetRightVector() const;

	/** Apply a transformation to this transform in local space */
	void Translate(glm::vec3 translation) { m_localPosition += translation; m_dirtyTRS = true; }
	void Rotate(glm::vec3 rotation);
	void Scale(glm::vec3 scale) { m_localScale *= scale; m_dirtyTRS = true; }

	/** @return the transform-rotate-scale matrix of the transform */
	glm::mat4 GetTRS();
	bool IsDirty() { return m_dirtyTRS; }

private:
	Transform* m_parent = nullptr;

	glm::vec3 m_localPosition = glm::vec3(0.0f);
	glm::vec3 m_localRotation = glm::vec3(0.0f);;
	glm::vec3 m_localScale = glm::vec3(1.0f);;

	/** Cache transform-rotate-scale */
	glm::mat4 m_trs;
	bool m_dirtyTRS = true;
};

