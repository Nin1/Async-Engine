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
	void Set(std::shared_ptr<Mesh>& mesh) { /* NYI */ }
	/** Uploads the mesh to VRAM. Only call from the main thread. Only valid if GetLoadState() == LOADED.
	  * This object's shared_ptr to the mesh will be cleared after this. */
	void Upload();

	void PrepareForRendering() const;

	/** Returns the vertex buffer ID. Only valid if GetLoadState() == UPLOADED */
	uint32_t GetVertexBufferID();
	uint32_t GetVertexCount();

	/** Returns the current state of this asset. This can be called at any time. */
	LoadState GetLoadState() const { return m_state; }

private:
	DECLARE_CLASS_JOB(ModelAsset, LoadFromFile);

private:
	/** State of this asset */
	LoadState m_state = LoadState::UNLOADED;
	/** Name of file to read */
	std::string m_filename;
	/** Mesh data. This is only once loaded, and before upload. */
	std::shared_ptr<Mesh> m_mesh;

	/** GLuint vertexBufferID. Only valid if m_state == UPLOADED */
	uint32_t m_vertexBufferID;
	/** GLuint vertexArrayID. Only valid if m_state == UPLOADED */
	uint32_t m_vertexArrayID;

	uint32_t m_vertexCount = 0;
};

