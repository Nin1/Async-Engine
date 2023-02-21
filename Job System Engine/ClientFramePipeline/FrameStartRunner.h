#pragma once
#include "ClientFrameData.h"
#include <FramePipeline/FrameStageRunner.h>
#include <Jobs/JobDecl.h>

class FrameStartRunner : public FrameStageRunner<ClientFrameData>
{
public:
	FrameStartRunner() : FrameStageRunner("Frame Start") { }
protected:
	virtual void RunJobInner() override;

private:
	DECLARE_CLASS_JOB(FrameStartRunner, MainThreadTasks);

private:
	/** Number of frames that have been started */
	int m_frameCount = 0;
	/** Time at the start of the last frame (in seconds since the app started) */
	double m_lastFrameStartTime = 0.0;
	/** Test ImGui */
	bool m_showImGuiDemo = true;
};

