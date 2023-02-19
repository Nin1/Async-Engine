#pragma once
#include "ModelAsset.h"

struct ModelToRender
{
	ModelAsset* m_model;
	glm::mat4 m_transRotScale;
};