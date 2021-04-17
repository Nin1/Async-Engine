#pragma once
#include "FrameStageRunner.h"

class GameLogicRunner : public FrameStageRunner
{
public:
	GameLogicRunner() : FrameStageRunner("Game Logic") { }
protected:
	virtual void RunJobInner(JobCounterPtr& jobCounter) override;
};

