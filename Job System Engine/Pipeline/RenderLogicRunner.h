#pragma once
#include "FrameStageRunner.h"

class RenderLogicRunner : public FrameStageRunner
{
public:
	RenderLogicRunner() : FrameStageRunner("Render Logic") { }
	virtual void Init() override;
protected:
	virtual void RunJobInner(JobCounterPtr& jobCounter) override;
};

