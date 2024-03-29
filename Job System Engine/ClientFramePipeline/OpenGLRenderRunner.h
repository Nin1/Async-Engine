#pragma once
#include "ClientFrameData.h"
#include <FramePipeline/FrameStageRunner.h>
#include <Graphics/Shader/ShaderProgram.h>


class OpenGLRenderRunner : public FrameStageRunner<ClientFrameData>
{
public:
	OpenGLRenderRunner() : FrameStageRunner("GPU Execution") { }

	virtual void Init() override;
protected:
	virtual void RunJobInner() override;

private:
	DECLARE_CLASS_JOB(OpenGLRenderRunner, MainThreadTasks);

private:
	int m_framesCompleted = 0;
	ShaderProgram m_solidColourShader;
};

