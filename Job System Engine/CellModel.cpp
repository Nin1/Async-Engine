#include "CellModel.h"
#include <glm/gtx/transform.hpp>

bool CellModel::LoadObj(const char* filename)
{
	// Load base model
	if (m_rotShapes[0].Load(filename))
	{

		// Create all 24 rotations

		// Rotate base around Y axis
		glm::mat4x4 rot90 = glm::rotate(90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		m_rotShapes[1].Set(m_rotShapes[0].m_vertices, rot90);
		m_rotShapes[2].Set(m_rotShapes[1].m_vertices, rot90);
		m_rotShapes[3].Set(m_rotShapes[2].m_vertices, rot90);

		//etc.

		return true;
	}
	return false;
}