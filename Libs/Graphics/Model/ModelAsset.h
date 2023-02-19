#pragma once
#include "../GraphicsAsset.h"
#include "Mesh.h"
#include <Jobs/JobDecl.h>
#include <memory>
#include <string>


/**
 * A model that can be asynchronously loaded or set, and uploaded to the graphics card.
 * Loading happens in a job, during which other systems can poll GetLoadState() to check when that has completed.
 */
class ModelAsset
{
public:
	/** Kick off a load job to load this asset from a file. Poll GetLoadState() periodically to check the status. */
	void Load(const char* filename);
	/** Sets this model's data from pre-loaded data. */
	void Set(std::shared_ptr<Mesh>& mesh) { m_mesh = mesh; m_state = LoadState::LOADED; }
	/** Uploads the mesh to VRAM. Only call from the main thread. Only valid if GetLoadState() == LOADED. */
	void Upload();

	void PrepareForRendering() const;

	/** Returns the vertex buffer ID. Only valid if GetLoadState() == UPLOADED */
	uint32_t GetVertexBufferID();
	uint32_t GetVertexCount();
	/** Returns the mesh data */
	const std::shared_ptr<Mesh>& GetMesh() { return m_mesh; }

	/** Returns the current state of this asset. This can be called at any time. */
	LoadState GetLoadState() const { return m_state; }

private:
	DECLARE_CLASS_JOB(ModelAsset, LoadFromFile);

private:
	/** State of this asset */
	LoadState m_state = LoadState::UNLOADED;
	/** Name of file to read */
	std::string m_filename;
	/** Mesh data. Only valid if m_state >= LOADED */
	std::shared_ptr<Mesh> m_mesh;

	/** GLuint vertexBufferID. Only valid if m_state == UPLOADED */
	uint32_t m_vertexBufferID;
	/** GLuint vertexArrayID. Only valid if m_state == UPLOADED */
	uint32_t m_vertexArrayID;

	uint32_t m_vertexCount = 0;
};

