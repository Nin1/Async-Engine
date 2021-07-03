#pragma once
#include "FrameStageRunner.h"
#include "../Camera.h"
#include "../ModelAsset.h"

class GameLogicRunner : public FrameStageRunner
{
public:
	GameLogicRunner() : FrameStageRunner("Game Logic") { }
	virtual void Init() override;
protected:
	virtual void RunJobInner(JobCounterPtr& jobCounter) override;

private:
	/********
	  SCENE
	********/

	Camera m_camera;

	ModelAsset m_testModel;
	Transform m_testModelTransform;
};

