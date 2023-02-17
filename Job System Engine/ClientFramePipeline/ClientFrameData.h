#pragma once
#include <Graphics/Camera/CameraData.h>
#include <Graphics/Model/ModelToRender.h>
#include <Input/Input.h>

enum class FrameStage
{
	FRAME_START,
	GAME_LOGIC,
	RENDER_LOGIC,
	GPU_EXECUTION,
};

struct ClientFrameData
{
public:
	ClientFrameData(GLFWwindow* window, Input& input)
		: m_window(window)
		, m_inputHandler(input)
	{
	}

	void Reset(int64_t frameNumber)
	{
		m_stage = FrameStage::FRAME_START;
		m_frameNumber = frameNumber;
		m_modelsToRender.clear();
	}

	/** The stage that this frame is in */
	FrameStage m_stage = FrameStage::FRAME_START;
	/** The number of frames drawn all-time up-to-and-including this frame */
	int64_t m_frameNumber = -1;
	/** The GLFWwindow for this application. Only access on the main thread. */
	GLFWwindow* m_window;
	/** The input handler for this application. */
	Input& m_inputHandler;


	/** The input state at the start of this frame. Valid after FRAME_START. */
	InputState m_input;
	/** Matrices and other data about the camera. Valid after GAME_LOGIC. */
	CameraData m_camera;
	/** List of models to render. Valid after GAME_LOGIC. */
	std::vector<ModelToRender> m_modelsToRender;
};