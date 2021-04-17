#pragma once
#include "FrameStageRunner.h"

class GPUExecutionRunner : public FrameStageRunner
{
public:
	GPUExecutionRunner() : FrameStageRunner("GPU Execution") { }
protected:
	virtual void RunJobInner(JobCounterPtr& jobCounter) override;

private:
	int m_framesCompleted = 0;
};

