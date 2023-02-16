#pragma once
#include "../GraphicsAsset.h"
#include <Jobs/JobDecl.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <GL/glew.h>

struct ShaderUniform
{
	std::string m_name;
	std::string m_type;
	GLuint m_pos;
};

/** A single shader that can be loaded from disk and compiled. */
class ShaderLoader
{
public:
	/** Kick off a load job to load this asset from a file. Poll GetLoadState() periodically to check the status. */
	void Load(const char* filename);
	/** Compiles this shader. shaderType is GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc. */
	bool Compile(GLenum shaderType);

	const std::vector<ShaderUniform>& GetUniforms() const { return m_uniforms; }

	/** Return the shader ID. Only valid if GetLoadState() == UPLOADED. */
	uint32_t GetShaderID();

	/** Returns the current state of this asset. This can be called at any time. */
	inline LoadState GetLoadState() const { return m_state; }

private:
	DECLARE_CLASS_JOB(ShaderLoader, LoadFromFile);

	bool LoadShaderFromFile();
	void CreateUniformFromLine(std::string& line);

private:
	/** State of this asset */
	LoadState m_state = LoadState::UNLOADED;
	/** Name of file to read */
	std::string m_filename;
	/** Shader source data. Only valid if m_state == LOADED. */
	std::string m_shaderSource;

	/** Uniforms for this shader */
	std::vector<ShaderUniform> m_uniforms;

	uint32_t m_shaderID;
};

