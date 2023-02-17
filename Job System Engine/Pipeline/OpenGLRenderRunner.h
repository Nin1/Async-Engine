#pragma once
#include "FrameStageRunner.h"
#include "../ClientFrameData.h"
#include <Graphics/Shader/ShaderProgram.h>


class OpenGLRenderRunner : public FrameStageRunner<ClientFrameData>
{
public:
	OpenGLRenderRunner() : FrameStageRunner("GPU Execution") { }

	virtual void Init() override;
protected:
	virtual void RunJobInner(JobCounterPtr& jobCounter) override;

private:
	DECLARE_CLASS_JOB(OpenGLRenderRunner, MainThreadTasks);

private:
	int m_framesCompleted = 0;
	ShaderProgram m_solidColourShader;
};

