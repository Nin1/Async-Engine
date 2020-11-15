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


void StartApp(void* app)
{
	static_cast<GameApp*>(app)->Start();
}

int main()
{
	GameApp app;
	Jobs jobs(StartApp, &app);

	return 0;
}