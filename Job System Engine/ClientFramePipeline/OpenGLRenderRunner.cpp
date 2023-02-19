#include <GL/glew.h>
#include "OpenGLRenderRunner.h"
#include <Diagnostic/Assert.h>
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

void OpenGLRenderRunner::RunJobInner()
{
	ClientFrameData& frameData = *m_frameData->GetData();
	// Debug: Make sure frames are running in the correct order
	ASSERT(frameData.m_frameNumber == m_framesCompleted);

	frameData.m_stage = FrameStage::GPU_EXECUTION;
	if (frameData.m_frameNumber % 100 == 0)
	{
		std::printf("Completed frame %I64d \n", frameData.m_frameNumber);
	}

	// Make sure rendering all happens on the main thread
	Jobs::CreateJob(OpenGLRenderRunner::MainThreadTasks, this, JOBFLAG_MAINTHREAD | JOBFLAG_ISCHILD);

	m_framesCompleted++;
}

DEFINE_CLASS_JOB(OpenGLRenderRunner, MainThreadTasks)
{
	ClientFrameData& frameData = *m_frameData->GetData();

	/*************
	* PRE-RENDER *
	*************/

	// Compile any shaders that we need to
	if (m_solidColourShader.GetState() == ShaderState::LOADING)
	{
		m_solidColourShader.Compile();
	}

	// Upload any models that we need to
	for (auto& model : frameData.m_modelsToRender)
	{
		// If this model isn't uploaded yet, upload it
		if (model.m_model != nullptr && model.m_model->GetLoadState() == LoadState::LOADED)
		{
			model.m_model->Upload();
		}
	}

	/*************
	*   RENDER   *
	*************/

	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Eventually, get the shader from the model's material
	glUseProgram(m_solidColourShader.GetProgramID());
	m_solidColourShader.SetGlUniformMat4("viewMat", frameData.m_camera.m_viewMatrix);
	m_solidColourShader.SetGlUniformMat4("projMat", frameData.m_camera.m_projMatrix);

	for (auto& model : frameData.m_modelsToRender)
	{
		// If this model is uploaded, and the shader it asks for is uploaded, render it
		if (model.m_model != nullptr && model.m_model->GetLoadState() == LoadState::UPLOADED && m_solidColourShader.GetState() == ShaderState::READY)
		{
			model.m_model->PrepareForRendering();
			m_solidColourShader.SetGlUniformMat4("modelMat", model.m_transRotScale);
			glDrawArrays(GL_TRIANGLES, 0, model.m_model->GetVertexCount());
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glfwSwapBuffers(frameData.m_window);
}