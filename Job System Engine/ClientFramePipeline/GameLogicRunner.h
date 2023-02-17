#pragma once
#include "ClientFrameData.h"
#include <FramePipeline/FrameStageRunner.h>
#include <Graphics/Camera/Camera.h>
#include <Graphics/Model/ModelAsset.h>

class GameLogicRunner : public FrameStageRunner<ClientFrameData>
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
