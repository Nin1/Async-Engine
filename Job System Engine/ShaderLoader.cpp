#include "ShaderLoader.h"
#include "Assert.h"
#include <Jobs/Jobs.h>
#include <fstream>
#include <sstream>


void ShaderLoader::Load(const char* filename)
{
	ASSERT(m_state == LoadState::UNLOADED);
	m_state = LoadState::LOADING;
	m_filename = filename;
	Jobs::CreateJob(ShaderLoader::LoadFromFile, this, JOBFLAG_DISKACCESS);
}

DEFINE_CLASS_JOB(ShaderLoader, LoadFromFile)
{
	if (LoadShaderFromFile())
	{
		m_state = LoadState::LOADED;
	}
	else
	{
		m_state = LoadState::FAILED;
	}
}

uint32_t ShaderLoader::GetShaderID()
{
	ASSERT(m_state == LoadState::UPLOADED);
	return m_shaderID;
}

bool ShaderLoader::Compile(GLenum shaderType)
{
	ASSERT(m_state == LoadState::LOADED);

	m_shaderID = glCreateShader(shaderType);

	// Compile shader
	char const* sourcePointer = m_shaderSource.c_str();
	glShaderSource(m_shaderID, 1, &sourcePointer, NULL);
	glCompileShader(m_shaderID);

	// Check shader
	GLint result = GL_TRUE;
	int infoLogLength;
	glGetShaderiv(m_shaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(m_shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0)
	{
		char errorMessage[256];
		glGetShaderInfoLog(m_shaderID, infoLogLength, NULL, errorMessage);
		std::cout << errorMessage << std::endl;
	}
	if (result != GL_TRUE)
	{
		m_state = LoadState::FAILED;
		return false;
	}

	// We're done with the source
	m_shaderSource.clear();

	m_state = LoadState::UPLOADED;
	return true;
}

bool ShaderLoader::LoadShaderFromFile()
{
	std::ifstream shaderStream(m_filename, std::ios::in);
	if (shaderStream.is_open())
	{
		std::string line;
		while (getline(shaderStream, line))
		{
			m_shaderSource += "\n" + line;

			// Extract any uniforms from the shader
			CreateUniformFromLine(line);

		}
		shaderStream.close();
		return true;
	}
	else
	{
		std::cout << "Could not open shader source from " << m_filename << std::endl;
		return false;
	}
}

void ShaderLoader::CreateUniformFromLine(std::string& line)
{
	std::istringstream lineStream(line);
	std::string isUniform;
	std::getline(lineStream, isUniform, ' ');
	if (isUniform == "uniform")
	{
		// Create the ShaderUniform
		ShaderUniform uniform;

		// This is a uniform, find its type
		std::getline(lineStream, uniform.m_type, ' ');

		// Find its name
		if (lineStream.str().find(" =") != std::string::npos)
		{
			std::getline(lineStream, uniform.m_name, ' ');
		}
		else if (lineStream.str().find("=") != std::string::npos)
		{
			std::getline(lineStream, uniform.m_name, '=');
		}
		else
		{
			std::getline(lineStream, uniform.m_name, ';');
		}

		m_uniforms.push_back(std::move(uniform));
	}
}