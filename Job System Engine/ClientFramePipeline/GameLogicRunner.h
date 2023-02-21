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
	virtual void RunJobInner() override;

private:
	/********
	  SCENE
	********/

	Camera m_camera;

	ModelAsset m_testModel;
	static constexpr int NUM_CUBES = 10000;
	static constexpr int CUBE_PARALLEL_CHUNK_SIZE = 500;
	static constexpr int CUBE_RANDOM_POS_RANGE = 50;
	std::array<Transform, NUM_CUBES> m_testModelTransforms;

	/********
	  DEBUG
	********/

};

