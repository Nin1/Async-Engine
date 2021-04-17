#pragma once
#include "FrameStageRunner.h"
#include <Jobs/JobDecl.h>

class FrameStartRunner : public FrameStageRunner
{
public:
	FrameStartRunner() : FrameStageRunner("Frame Start") { }
protected:
	virtual void RunJobInner(JobCounterPtr& jobCounter) override;

private:
	DECLARE_CLASS_JOB(FrameStartRunner, MainThreadTasks);

private:
	/** Number of frames that have been started */
	int m_frameCount = 0;
};

