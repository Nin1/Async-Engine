#pragma once
#include <stdint.h>
#include "../CameraData.h"
#include "../ModelToRender.h"
#include "../Input.h"

struct GLFWwindow;
struct FramePipeline;

enum class FrameStage
{
	FRAME_START,
	GAME_LOGIC,
	RENDER_LOGIC,
	GPU_EXECUTION,
};

// All data pertaining to one frame:
// - Scratch memory for game logic
// - Scene data for render logic
// - Render data for GPU execution
// Multiple frames can be in flight at once, at different stages of execution.
struct FrameData
{
public:
	FrameData(int id, FramePipeline& pipeline, GLFWwindow* window, Input& input)
		: m_myId(id)
		, m_pipeline(pipeline)
		, m_window(window)
		, m_inputHandler(input)
	{ }

	void Reset(int64_t frameNumber);

public:
	/**********
	 META DATA
	**********/

	/** The global ID of this frame. */
	const int m_myId;
	/** The GLFWwindow for this application. Only access on the main thread. */
	GLFWwindow* m_window;
	/** The input handler for this application. */
	Input& m_inputHandler;
	/** The pipeline that this frame is moving through. */
	FramePipeline& m_pipeline;
	/** The stage that this frame is in */
	FrameStage m_stage = FrameStage::FRAME_START;
	/** The number of frames drawn all-time up-to-and-including this frame */
	int64_t m_frameNumber = -1;
	/** Debug - Is this frame currently being processed? */
	bool m_active = false;

	/***********
	 FRAME DATA
	***********/

	/** The input state at the start of this frame. Valid after FRAME_START. */
	InputState m_input;
	/** Matrices and other data about the camera. Valid after GAME_LOGIC. */
	CameraData m_camera;
	/** List of models to render. Valid after GAME_LOGIC. */
	std::vector<ModelToRender> m_modelsToRender;
};

