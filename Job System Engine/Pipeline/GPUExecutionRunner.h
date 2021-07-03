#pragma once
#include "FrameStageRunner.h"
#include "../ShaderProgram.h"


class GPUExecutionRunner : public FrameStageRunner
{
public:
	GPUExecutionRunner() : FrameStageRunner("GPU Execution") { }

	virtual void Init() override;
protected:
	virtual void RunJobInner(JobCounterPtr& jobCounter) override;

private:
	DECLARE_CLASS_JOB(GPUExecutionRunner, MainThreadTasks);

private:
	int m_framesCompleted = 0;
	ShaderProgram m_solidColourShader;
};

