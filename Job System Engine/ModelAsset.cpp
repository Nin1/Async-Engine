#include "ModelAsset.h"
#include "Assert.h"
#include <Jobs/Jobs.h>
#include <GL/glew.h>

void ModelAsset::Load(const char* filename)
{
	ASSERT(m_state == LoadState::UNLOADED);
	m_state = LoadState::LOADING;
	m_filename = filename;
	Jobs::CreateJob(ModelAsset::LoadFromFile, this, JOBFLAG_DISKACCESS);
}

// TODO: Split mesh loading in two: Read from disk, and process
DEFINE_CLASS_JOB(ModelAsset, LoadFromFile)
{
	m_mesh = std::make_shared<Mesh>();
	if (m_mesh->Load(m_filename.c_str()))
	{
		m_state = LoadState::LOADED;
	}
	else
	{
		m_state = LoadState::FAILED;
	}
}

void ModelAsset::Upload()
{
	ASSERT(m_state == LoadState::LOADED);
	m_state = LoadState::UPLOADED;
	std::cout << "Uploading model: " << m_filename << std::endl;
	glGenVertexArrays(1, &m_vertexArrayID);
	glBindVertexArray(m_vertexArrayID);
	glGenBuffers(1, &m_vertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferID);
	glBufferData(GL_ARRAY_BUFFER, m_mesh->m_vertices.size() * sizeof(glm::vec3), &m_mesh->m_vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);
	m_vertexCount = (uint32_t)m_mesh->m_vertices.size();
	std::cout << "Uploaded model: " << m_filename << std::endl;
	// We don't need the mesh any more - If we're the only thing using it, this will free its memory
	m_mesh.reset();
}

void ModelAsset::PrepareForRendering() const
{
	glBindVertexArray(m_vertexArrayID);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferID);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
}

uint32_t ModelAsset::GetVertexBufferID()
{
	ASSERT(m_state == LoadState::UPLOADED);
	return m_vertexBufferID;
}

uint32_t ModelAsset::GetVertexCount()
{
	ASSERT(m_state == LoadState::UPLOADED);
	return m_vertexCount;
}