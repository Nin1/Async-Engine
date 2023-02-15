#include "Mesh.h"
#include <glm/vec2.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ostream>

/** Table of delimiters for reading from .obj files */
struct ObjLoadDelimiters : std::ctype<char> {
	ObjLoadDelimiters() : std::ctype<char>(get_table()) {}
	static mask const* get_table()
	{
		static mask rc[table_size];
		rc['/'] = std::ctype_base::space;
		rc['\n'] = std::ctype_base::space;
		rc[' '] = std::ctype_base::space;
		return &rc[0];
	}
};

uint8_t GetFaceAttributeCount(std::string face)
{
	// Count the number of '/'s in a face line, divide by 3 (to get one vertex), and add one
	return ((uint8_t)std::count(face.begin() + 1, face.end(), '/') / 3) + 1;
}

bool Mesh::Load(const char* modelPath)
{
	/** Load the model file */

	std::ifstream modelFile(modelPath, std::ios::in);
	if (!modelFile.is_open())
	{
		std::cout << "Error opening file: " << modelPath << std::endl;
		return false;
	}
	std::cout << "Loading model: " << modelPath << std::endl;

	/** Set up temporary containers */

	std::vector<glm::vec3> tempVerts;
	std::vector<glm::vec2> tempUVs;
	std::vector<glm::vec3> tempNormals;
	std::vector<glm::vec3> tempTangents;
	std::vector<glm::vec3> tempBitangents;
	std::vector<int> vertIndices, uvIndices, normalIndices;
	std::string line;
	uint8_t faceAttributeCount = 0;

	/** Parse the model file */

	while (std::getline(modelFile, line))
	{
		std::istringstream lineStream(line);
		lineStream.imbue(std::locale(lineStream.getloc(), new ObjLoadDelimiters));

		std::string mode;
		lineStream >> mode;

		if (mode == "v")
		{
			/** Vertex information */
			glm::vec3 vert;
			lineStream >> vert.x;
			lineStream >> vert.y;
			lineStream >> vert.z;
			tempVerts.push_back(vert);
		}
		if (mode == "vt")
		{
			/** TexCoord information */
			glm::vec2 texCoord;
			lineStream >> texCoord.x;
			lineStream >> texCoord.y;
			tempUVs.push_back(texCoord);
		}
		if (mode == "vn")
		{
			/** Vertex normal information */
			glm::vec3 normal;
			lineStream >> normal.x;
			lineStream >> normal.y;
			lineStream >> normal.z;
			tempNormals.push_back(normal);
		}
		if (mode == "f")
		{
			/** Face information */
			int vertIndex[3];	// Vertex indices
			int uvIndex[3];		// Texture indices
			int normalIndex[3];	// Normal indices

			// Count the number of attributes per face
			if (faceAttributeCount == 0)
			{
				faceAttributeCount = GetFaceAttributeCount(lineStream.str());
			}

			// Load attributes for each vertex in a face
			for (int i = 0; i < 3; ++i)
			{
				if (tempVerts.size() > 0)
				{
					lineStream >> vertIndex[i];
				}
				if (tempUVs.size() > 0)
				{
					if (faceAttributeCount >= 2)
					{
						lineStream >> uvIndex[i];
					}
					else
					{
						uvIndex[i] = vertIndex[i];
					}
				}
				if (tempNormals.size() > 0)
				{
					if (faceAttributeCount >= 3)
					{
						lineStream >> normalIndex[i];
					}
					else
					{
						normalIndex[i] = vertIndex[i];
					}
				}
			}

			// Store all vertex, uv, and normal indices for this face
			if (tempVerts.size() > 0)
			{
				vertIndices.push_back(vertIndex[0]);
				vertIndices.push_back(vertIndex[1]);
				vertIndices.push_back(vertIndex[2]);
			}
			if (tempUVs.size() > 0)
			{
				uvIndices.push_back(uvIndex[0]);
				uvIndices.push_back(uvIndex[1]);
				uvIndices.push_back(uvIndex[2]);
			}
			if (tempNormals.size() > 0)
			{
				normalIndices.push_back(normalIndex[0]);
				normalIndices.push_back(normalIndex[1]);
				normalIndices.push_back(normalIndex[2]);
			}
		}
	}
	modelFile.close();

	/** Convert to usable mesh data */

	// Generate a vertex mesh from the face data
	// (duplicate some vertices to use with multiple faces, TODO: share vertex data between faces)
	if (tempVerts.size() > 0)
	{
		m_vertices.reserve(vertIndices.size());

		for (const auto& vertexIndex : vertIndices)
		{
			glm::vec3 vertex = tempVerts[vertexIndex - 1];
			m_vertices.push_back(vertex);
		}

		m_numFaces = (uint32_t)m_vertices.size() / 3;
	}

	return true;
}

bool Mesh::Set(const std::vector<glm::vec3>& vertices, glm::mat4x4& rotation)
{
	for (const auto& vert : vertices)
	{
		m_vertices.push_back(rotation * glm::vec4(vert, 1.0f));
	}

	return true;
}