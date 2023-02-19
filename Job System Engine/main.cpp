#include <Jobs/Jobs.h>
#include "GameApp.h"

/*
1. Initialise:
	window
	graphics
	input
	audio
2. Loop
*/



JOB_FUNCTION_START(StartApp, GameApp, app)
{
	app->Start();
}
JOB_FUNCTION_END()

int main()
{
	GameApp app;
	Jobs jobs(4, StartApp, &app);

	return 0;
}