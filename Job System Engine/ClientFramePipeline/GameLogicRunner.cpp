#include "GameLogicRunner.h"
#include <Diagnostic/Assert.h>
#include <Input/InputState.h>
#include <iostream>

void GameLogicRunner::Init()
{
	// Set up a basic scene here
	m_testModel.Load("Assets/unitcube.obj");
	//m_testModelTransform.Translate(glm::vec3(0.0f, 0.0f, 10.0f));
	m_camera.m_transform.Translate(glm::vec3(1.0f, 0.0f, 2.0f));
	m_camera.m_transform.Rotate(glm::vec3(0.0f, -90.0f, 0.0f));
}

// GameLogicRunner holds the ONLY representation of the game scene. Anything needed for rendering is extracted into FrameData before we proceed to RenderLogic
void GameLogicRunner::RunJobInner(JobCounterPtr& jobCounter)
{
	// Run game logic here (scripts, simulation, etc.)
	ClientFrameData& frameData = *m_frameData->GetData();
	ASSERT(frameData.m_stage == FrameStage::FRAME_START);
	frameData.m_stage = FrameStage::GAME_LOGIC;

	// m_scene.RunFixedUpdate(); // Loop this however many times is needed for the fixed timestep
	// m_scene.RunFrameUpdate(); // Run once per frame
	// m_scene.ExtractFrameData(); // Returns data needed to render the scene

	// Move camera
	InputState& input = frameData.m_input;
	glm::vec3 moveDir = glm::vec3(0.0f);
	if (input.GetKeyHeld(GLFW_KEY_W))
	{
		moveDir += m_camera.m_transform.GetForwardVector();
	}
	if (input.GetKeyHeld(GLFW_KEY_S))
	{
		moveDir -= m_camera.m_transform.GetForwardVector();
	}
	if (input.GetKeyHeld(GLFW_KEY_D))
	{
		moveDir += m_camera.m_transform.GetRightVector();
	}
	if (input.GetKeyHeld(GLFW_KEY_A))
	{
		moveDir -= m_camera.m_transform.GetRightVector();
	}

	m_camera.m_transform.Translate(moveDir * 0.01f);

	// Rotate camera
	const glm::vec2& mouseDelta = input.GetLastMouseOffset();
	glm::vec3 rotation = m_camera.m_transform.GetLocalRotation();
	rotation.x = glm::clamp(rotation.x - mouseDelta.y, -80.0f, 80.0f);
	rotation.y += mouseDelta.x;
	if (rotation.y < 0) rotation.y += 360.0f;
	else if (rotation.y >= 360.0f) rotation.y -= 360.0f;
	rotation.z = 0.0f;
	m_camera.m_transform.SetLocalRotation(rotation);

	// Rotate model
	m_testModelTransform.Rotate(glm::vec3(0.1f, 0.1f, 0.1f));


	// Extract data into m_frameData
	frameData.m_camera = m_camera.GetFrameData();
	frameData.m_modelsToRender.push_back({ m_testModel, m_testModelTransform.GetTRS() });
}