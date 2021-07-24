#include <GL/glew.h>
#include "OpenGLRenderRunner.h"
#include "FrameData.h"
#include "../Assert.h"
#include "../Input.h"
#include <iostream>
#include <cstdio>
#include <GLFW/glfw3.h>

// Debug callback
static void APIENTRY openglCallbackFunction(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam
) {
	(void)source; (void)type; (void)id;
	(void)severity; (void)length; (void)userParam;
	fprintf(stderr, "%s\n", message);
	if (severity == GL_DEBUG_SEVERITY_HIGH) {
		fprintf(stderr, "Aborting...\n");
		abort();
	}
}

void OpenGLRenderRunner::Init()
{
	// Initialise glew
	GLenum success = glewInit();
	ASSERTM(success == GLEW_OK, "Error initialising glew");
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// OpenGL debug
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openglCallbackFunction, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);

	// Load shader
	m_solidColourShader.Load("Shaders/SolidColour");

	std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
}

void OpenGLRenderRunner::RunJobInner(JobCounterPtr& jobCounter)
{
	// Debug: Make sure frames are running in the correct order
	ASSERT(m_frameData->m_frameNumber == m_framesCompleted);

	m_frameData->m_stage = FrameStage::GPU_EXECUTION;
	if (m_frameData->m_frameNumber % 100 == 0)
	{
		std::printf("Completed frame %I64d \n", m_frameData->m_frameNumber);
	}

	// Make sure rendering all happens on the main thread
	Jobs::CreateJobAndCount(OpenGLRenderRunner::MainThreadTasks, this, JOBFLAG_MAINTHREAD, jobCounter);

	m_framesCompleted++;
}

DEFINE_CLASS_JOB(OpenGLRenderRunner, MainThreadTasks)
{
	/*************
	* PRE-RENDER *
	*************/

	// Compile any shaders that we need to
	if (m_solidColourShader.GetState() == ShaderState::LOADING)
	{
		m_solidColourShader.Compile();
	}

	// Upload any models that we need to
	for (auto& model : m_frameData->m_modelsToRender)
	{
		// If this model isn't uploaded yet, upload it
		if (model.m_model.GetLoadState() == LoadState::LOADED)
		{
			model.m_model.Upload();
		}
	}

	/*************
	*   RENDER   *
	*************/

	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (auto& model : m_frameData->m_modelsToRender)
	{
		// If this model is uploaded, and the shader it asks for is uploaded, render it
		if (model.m_model.GetLoadState() == LoadState::UPLOADED && m_solidColourShader.GetState() == ShaderState::READY)
		{
			// Eventually, get the shader from the model's material
			glUseProgram(m_solidColourShader.GetProgramID());

			model.m_model.PrepareForRendering();
			glm::mat4 modelViewProj = m_frameData->m_camera.m_projMatrix * m_frameData->m_camera.m_viewMatrix * model.m_transRotScale;
			m_solidColourShader.SetGlUniformMat4("mvp", modelViewProj);
			glDrawArrays(GL_TRIANGLES, 0, model.m_model.GetVertexCount());
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glfwSwapBuffers(m_frameData->m_window);
}