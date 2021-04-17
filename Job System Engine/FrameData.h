#pragma once
#include "FramePipeline.h"
#include <GLFW/glfw3.h>
#include <stdint.h>

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
	FrameData(int id, FramePipeline& pipeline, GLFWwindow* window)
		: m_myId(id)
		, m_pipeline(pipeline)
		, m_window(window)
	{ }

	void Reset(int64_t frameNumber);

public:
	/** The global ID of this frame. */
	const int m_myId;
	/** The GLFWwindow for this application. Only access on the main thread. */
	GLFWwindow* m_window;
	/** The pipeline that this frame is moving through. */
	FramePipeline& m_pipeline;
	/** The stage that this frame is in */
	FrameStage m_stage = FrameStage::FRAME_START;
	/** The number of frames drawn all-time up-to-and-including this frame */
	int64_t m_frameNumber = -1;
	/** Debug - Is this frame currently being processed? */
	bool m_active = false;
};

