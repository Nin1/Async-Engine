#pragma once
#include <stdint.h>

struct GLFWwindow;

// All data pertaining to one frame, for example:
// - Scratch memory for game logic
// - Scene data for render logic
// - Render data for GPU execution
// Multiple frames can be in flight at once, at different stages of execution.
template<typename DATA>
struct FrameData
{
public:
	/** Constructs a new FrameData<DATA> object. defaultData is copied into this frame's data object. */
	FrameData(int id, DATA defaultData)
		: m_myId(id)
		, m_data(defaultData)
	{ }

	DATA* GetData() { return &m_data; }

public:
	/**********
	 META DATA
	**********/

	/** The global ID of this frame. */
	const int m_myId;
	/** Debug - Is this frame currently being processed? */
	bool m_active = false;

	/***********
	 FRAME DATA
	***********/

	/** App-specific data */
	DATA m_data;
};

