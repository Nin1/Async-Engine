#pragma once
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

/** Vertex data for a 3D model */
struct Mesh
{
	/** Load a mesh from a .obj file */
	bool Load(const char* filename);
	/** Set the vertices in this mesh and rotate them using the rotation matrix */
	bool Set(const std::vector<glm::vec3>& vertices, glm::mat4x4& rotation);

	std::vector<glm::vec3> m_vertices;
	uint32_t m_numFaces = 0;
};

