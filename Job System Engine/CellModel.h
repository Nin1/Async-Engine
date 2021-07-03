#pragma once
#include "CellShape.h"
#include "Mesh.h"
#include <array>

class CellModel
{
public:
	bool LoadObj(const char* filename);

private:
	/** Array of loaded mesh data, for every rotation. */
	std::array<Mesh, 24> m_rotShapes;
};

