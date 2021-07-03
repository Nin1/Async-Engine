#include "ShaderProgram.h"
#include "Assert.h"
#include <iostream>

ShaderProgram::~ShaderProgram()
{
	glDeleteShader(m_programID);
}

void ShaderProgram::Load(const char* shaderName)
{
	std::cout << "Loading shader: " << shaderName << std::endl;

	std::string vsName = std::string(shaderName);
	vsName.append(".vs");
	m_vertex = std::make_unique<ShaderLoader>();
	m_vertex->Load(vsName.c_str());

	std::string fsName = std::string(shaderName);
	fsName.append(".fs");
	m_fragment = std::make_unique<ShaderLoader>();
	m_fragment->Load(fsName.c_str());

	m_state = ShaderState::LOADING;
}

void ShaderProgram::Compile()
{
	ASSERT(m_state != ShaderState::READY && m_state != ShaderState::FAILED);

	// Attempt to compile vertex shader
	if (m_vertex->GetLoadState() == LoadState::LOADED)
	{
		if (!m_vertex->Compile(GL_VERTEX_SHADER))
		{
			m_state = ShaderState::FAILED;
		}
	}

	// Attempt to compile fragment shader
	if (m_fragment->GetLoadState() == LoadState::LOADED)
	{
		if (!m_fragment->Compile(GL_FRAGMENT_SHADER))
		{
			m_state = ShaderState::FAILED;
		}
	}

	if (m_state == ShaderState::LOADING &&
		m_vertex->GetLoadState() == LoadState::UPLOADED &&
		m_fragment->GetLoadState() == LoadState::UPLOADED)
	{
		std::cout << "Compiling shader" << std::endl;

		// Ready to create shader program
		m_programID = glCreateProgram();
		GLuint vertID = m_vertex->GetShaderID();
		GLuint fragID = m_fragment->GetShaderID();
		glAttachShader(m_programID, vertID);
		glAttachShader(m_programID, fragID);
		glLinkProgram(m_programID);

		glDetachShader(m_programID, vertID);
		glDetachShader(m_programID, fragID);
		glDeleteShader(vertID);
		glDeleteShader(fragID);

		// Check compilation
		GLint result = GL_TRUE;
		int infoLogLength;
		glGetProgramiv(m_programID, GL_LINK_STATUS, &result);
		glGetProgramiv(m_programID, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength > 0)
		{
			char programErrorMessage[256];
			glGetProgramInfoLog(m_programID, infoLogLength, NULL, programErrorMessage);
			std::cout << programErrorMessage << std::endl;
		}
		if (result != GL_TRUE)
		{
			m_state = ShaderState::FAILED;
			m_vertex.reset();
			m_fragment.reset();
			return;
		}

		glUseProgram(m_programID);

		// Retrieve uniforms from shaders
		const std::vector<ShaderUniform>& vertUniforms = m_vertex->GetUniforms();
		const std::vector<ShaderUniform>& fragUniforms = m_fragment->GetUniforms();
		m_uniforms.reserve(vertUniforms.size() + fragUniforms.size());
		for (const auto& uniform : vertUniforms)
		{
			m_uniforms.push_back(uniform);
			m_uniforms.back().m_pos = glGetUniformLocation(m_programID, uniform.m_name.c_str());
		}
		for (const auto& uniform : fragUniforms)
		{
			m_uniforms.push_back(uniform);
			m_uniforms.back().m_pos = glGetUniformLocation(m_programID, uniform.m_name.c_str());
		}

		// Discard loaders
		m_vertex.reset();
		m_fragment.reset();
		m_state = ShaderState::READY;

		std::cout << "Shader compiled!" << std::endl;
	}
}

bool ShaderProgram::SetGlUniformMat4(const char* name, const glm::mat4& value)
{
	GLuint position = FindUniformPositionFromName(name);
	glUniformMatrix4fv(position, 1, GL_FALSE, &value[0][0]);
	return true;
}

bool ShaderProgram::SetGlUniformVec3(const char* name, const glm::vec3& value)
{
	GLuint position = FindUniformPositionFromName(name);
	glUniform3f(position, value.x, value.y, value.z);
	return true;
}

bool ShaderProgram::SetGlUniformVec2(const char* name, const glm::vec2& value)
{
	GLuint position = FindUniformPositionFromName(name);
	glUniform2f(position, value.x, value.y);
	return true;
}

bool ShaderProgram::SetGlUniformFloat(const char* name, float value)
{
	GLuint position = FindUniformPositionFromName(name);
	glUniform1f(position, value);
	return true;
}

bool ShaderProgram::SetGlUniformInt(const char* name, int value)
{
	GLuint position = FindUniformPositionFromName(name);
	glUniform1i(position, value);
	return true;
}

bool ShaderProgram::SetGlUniformSampler2D(const char* name, GLuint value)
{
	GLuint position = FindUniformPositionFromName(name);
	glUniform1i(position, value);
	return true;
}

bool ShaderProgram::SetGlUniformBool(const char* name, bool value)
{
	GLuint position = FindUniformPositionFromName(name);
	glUniform1i(position, value);
	return true;
}

GLuint ShaderProgram::FindUniformPositionFromName(const char* name)
{
	// If it's part of an array, find it manually
	std::string nameStr = name;
	GLuint pos = -1;
	if (nameStr.find('[') != std::string::npos)
	{
		pos = glGetUniformLocation(m_programID, name);
		if (pos != -1)
		{
			return pos;
		}
	}
	else
	{
		// Otherwise, return the stored position of the uniform if it exists
		for (const auto& uniform : m_uniforms)
		{
			if (uniform.m_name == name)
			{
				return uniform.m_pos;
			}
		}
	}

	std::cout << "Error: No uniform found with name " << name << ". It may have been optimised out." << std::endl;
	return pos;
}