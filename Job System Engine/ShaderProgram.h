#pragma once
#include "ShaderLoader.h"
#include <memory>
#include <glm\glm.hpp>

enum class ShaderState
{
	LOADING,
	READY,
	FAILED
};

class ShaderProgram
{
public:
	~ShaderProgram();

	/** Kick off a load job for this shader. shaderName is the name of the shader files with no extension, assuming vs and fs are named the same. */
	void Load(const char* shaderName);
	/** Compiles the shader if it has loaded and not yet been compiled. Call continuously as long as GetState() == LOADING */
	void Compile();
	/** Returns the state of this shader program (LOADING, READY, or FAILED) */
	ShaderState GetState() const { return m_state; }
	/** Returns the program ID. Only valid it GetState() == READY. */
	GLuint GetProgramID() { return m_programID; }

	/** Set the value of the uniform with the given name
	  * @return true if successful */
	bool SetGlUniformMat4(const char* name, const glm::mat4& value);
	bool SetGlUniformVec2(const char* name, const glm::vec2& value);
	bool SetGlUniformVec3(const char* name, const glm::vec3& value);
	bool SetGlUniformFloat(const char* name, float value);
	bool SetGlUniformInt(const char* name, int value);
	bool SetGlUniformSampler2D(const char* name, GLuint value);
	bool SetGlUniformBool(const char* name, bool value);

private:
	GLuint FindUniformPositionFromName(const char* name);

private:
	ShaderState m_state;
	std::unique_ptr<ShaderLoader> m_vertex;
	std::unique_ptr<ShaderLoader> m_fragment;

	std::vector<ShaderUniform> m_uniforms;

	uint32_t m_programID = 0;
};

