#include <GL/glew.h>
#include "OpenGLRenderRunner.h"
#include <Diagnostic/Assert.h>
#include <iostream>
#include <cstdio>
#include <GLFW/glfw3.h>
#include <imgui_impl_opengl3.h>

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
	if (severity == GL_DEBUG_SEVERITY_HIGH)
	{
		fprintf(stderr, "%s\n", message);
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

	// Make sure rendering all happens on the main thread (we can do things like visibility and occlusion on this thread though!
	Jobs::CreateJob(OpenGLRenderRunner::MainThreadTasks, this, JOBFLAG_MAINTHREAD | JOBFLAG_ISCHILD);
}

/** Process an ImGui frame from queued calls and render it */
void ProcessImGui(ClientFrameData& frameData)
{
	InputState& inputState = frameData.m_input;
	ImGui_ImplOpenGL3_NewFrame();
	ImGuiIO& io = ImGui::GetIO();
	// Setup display size (every frame to accommodate for window resizing)
	int w, h;
	int display_w, display_h;
	glfwGetWindowSize(frameData.m_window, &w, &h);
	glfwGetFramebufferSize(frameData.m_window, &display_w, &display_h);
	io.DisplaySize = ImVec2((float)w, (float)h);
	if (w > 0 && h > 0)
		io.DisplayFramebufferScale = ImVec2((float)display_w / (float)w, (float)display_h / (float)h);
	// Setup time step
	io.DeltaTime = (float)frameData.m_deltaTime;
	// Forward input events to ImGui (TODO: Input event queue at some point)
	for (int button = 0; button < inputState.GetNumMouseButtons(); button++)
	{
		if (button >= 0 && button < ImGuiMouseButton_COUNT)
		{
			if (inputState.GetMouseDown(button))
			{
				io.AddMouseButtonEvent(button, true);
			}
			else if (inputState.GetMouseUp(button))
			{
				io.AddMouseButtonEvent(button, false);
			}
		}
	}
	if (inputState.DidMouseMove())
	{
		auto& mousePos = inputState.GetMousePos();
		io.AddMousePosEvent(mousePos.x, mousePos.y);
	}
	ImGui::NewFrame();
	// Execute queued ImGui calls
	frameData.m_imgui.ExecuteQueue();
	// Render
	ImGui::Render();
	int backbufferWidth, backbufferHeight;
	glfwGetFramebufferSize(frameData.m_window, &backbufferWidth, &backbufferHeight);
	glViewport(0, 0, display_w, display_h);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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

	/*
	// Upload any models that we need to
	for (auto& model : frameData.m_modelsToRender)
	{
		// If this model isn't uploaded yet, upload it
		if (model.m_model != nullptr && model.m_model->GetLoadState() == LoadState::LOADED)
		{
			model.m_model->Upload();
		}
	}
	*/

	/*************
	*   RENDER   *
	*************/

	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Eventually, get the shader from the model's material

	if (m_solidColourShader.GetState() == ShaderState::READY)
	{
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
			break;
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	ProcessImGui(frameData);

	glfwSwapBuffers(frameData.m_window);

	if (frameData.m_frameNumber % 100 == 0)
	{
		std::printf("Completed frame %I64d \n", frameData.m_frameNumber);
	}
	m_framesCompleted++;
}