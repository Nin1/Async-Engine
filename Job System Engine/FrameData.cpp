#include "FrameData.h"


void FrameData::Reset(int64_t frameNumber)
{
	m_stage = FrameStage::FRAME_START;
	m_frameNumber = frameNumber;
}