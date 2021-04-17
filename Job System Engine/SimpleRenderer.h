#pragma once
#include "FrameData.h"
#include <Jobs/JobDecl.h>
#include <GLFW/glfw3.h>

/**
 * Renders a scene
 */
class SimpleRenderer
{
public:
	/** Initialise the renderer */
	void Init();

	/** Prepare the frame data for GPU execution */
	void PrepareRenderData(FrameData& frameData);

	/** Execute rendering on the GPU */
	void Render(FrameData& frameData);

private:
	GLFWwindow* m_window;
};

